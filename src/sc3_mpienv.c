/*
  This file is part of the SC Library, version 3.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2019 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc3_mpienv.h>
#include <sc3_refcount.h>

struct sc3_mpienv
{
  sc3_refcount_t      rc;
  sc3_allocator_t    *mator;
  int                 setup;

  /* parameters fixed before setup call */
  sc3_MPI_Comm_t      mpicomm;
  int                 commdup;
  int                 shared;
  int                 contiguous;

  /* member variables initialized in setup call */
  sc3_MPI_Comm_t      nodecomm;         /**< All ranks of shared memory node. */
  sc3_MPI_Comm_t      headcomm;         /**< Contains first rank of each node. */
  sc3_MPI_Info_t      info_noncontig;   /**< Key "alloc_shared_noncontig" set. */
  sc3_MPI_Win_t       nodesizewin;      /**< Shared memory segment allocated
                                             on first rank of a node, available
                                             to all ranks on that node.  Its
                                             element count is (2 + 2 * \ref
                                             num_nodes + 1) integers.
                                             Its contents hold
 *                                  * number of nodes for this run
 *                                  * zero-based number of this node
 *                                  * for each node number of ranks on it
 *                                  * for each node and one beyond the
 *                                    number of ranks before it
 */
  int                 mpisize;          /**< Size of forest communicator. */
  int                 mpirank;          /**< Rank in forest communicator. */
  int                 nodesize;         /**< Size of node communicator. */
  int                 noderank;         /**< Rank in node communicator. */
  int                 num_nodes;        /**< Number of shared memory nodes. */
  int                 node_num;         /**< Zero-based node number. */
  int                 node_frank;       /**< Rank within forest communicator
                                             of first rank on this node. */
  int                *node_sizes;       /**< For each node, number of its ranks. */
  int                *node_offsets;     /**< For each node and one beyond, the
                                             number of ranks before it. */
};

int
sc3_mpienv_is_valid (const sc3_mpienv_t * m, char *reason)
{
  SC3E_TEST (m != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &m->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, m->mator, reason);

  /* specific checks here */
  SC3E_TEST (!m->setup || m->mpicomm != SC3_MPI_COMM_NULL, reason);

  SC3E_YES (reason);
}

int
sc3_mpienv_is_new (const sc3_mpienv_t * m, char *reason)
{
  SC3E_IS (sc3_mpienv_is_valid, m, reason);
  SC3E_TEST (!m->setup, reason);
  SC3E_YES (reason);
}

int
sc3_mpienv_is_setup (const sc3_mpienv_t * m, char *reason)
{
  SC3E_IS (sc3_mpienv_is_valid, m, reason);
  SC3E_TEST (m->setup, reason);
  SC3E_YES (reason);
}

sc3_error_t        *
sc3_mpienv_new (sc3_allocator_t * mator, sc3_mpienv_t ** mp)
{
  sc3_mpienv_t       *m;

  SC3E_RETVAL (mp, NULL);
  SC3A_IS (sc3_allocator_is_setup, mator);

  SC3E (sc3_allocator_ref (mator));
  SC3E (sc3_allocator_calloc_one (mator, sizeof (sc3_mpienv_t), &m));
  SC3E (sc3_refcount_init (&m->rc));
  m->mator = mator;

  /* set defaults here whenever not zero/null */
  m->mpicomm = SC3_MPI_COMM_WORLD;
#ifdef SC3_ENABLE_MPI3
  m->shared = 1;
#endif

  SC3A_IS (sc3_mpienv_is_new, m);
  *mp = m;
  return NULL;
}

sc3_error_t        *
sc3_mpienv_set_comm (sc3_mpienv_t * m, sc3_MPI_Comm_t comm, int dup)
{
  SC3A_IS (sc3_mpienv_is_new, m);
  SC3A_CHECK (comm != SC3_MPI_COMM_NULL);

  /* remove previous communicator */
  if (m->commdup) {
    SC3E (sc3_MPI_Comm_free (&m->mpicomm));
  }

  /* register new communicator */
  if (dup) {
    SC3E (sc3_MPI_Comm_dup (comm, &m->mpicomm));
    SC3E (sc3_MPI_Comm_set_errhandler (m->mpicomm, SC3_MPI_ERRORS_RETURN));
  }
  else {
    m->mpicomm = comm;
  }
  m->commdup = dup;
  return NULL;
}

sc3_error_t        *
sc3_mpienv_set_shared (sc3_mpienv_t * m, int shared)
{
  SC3A_IS (sc3_mpienv_is_new, m);
#ifdef SC3_ENABLE_MPI3
  m->shared = shared;
#endif
  return NULL;
}

sc3_error_t        *
sc3_mpienv_set_contiguous (sc3_mpienv_t * m, int contiguous)
{
  SC3A_IS (sc3_mpienv_is_new, m);
  m->contiguous = contiguous;
  return NULL;
}

static sc3_error_t *
mpienv_setup_nodemem (sc3_mpienv_t * m)
{
  int                 headsize, headrank;
  int                 p, next, *ofs;
  int                 dispunit;
  int                *nodesizemem;
  sc3_MPI_Aint_t      nodeabytes;

  /* specify allocation of node size information */
  if (m->noderank == 0) {
    SC3E (sc3_MPI_Comm_size (m->headcomm, &headsize));
    SC3E (sc3_MPI_Comm_rank (m->headcomm, &headrank));
    nodeabytes = (2 + 2 * headsize + 1) * sizeof (int);
  }
  else {
    headsize = headrank = 0;
    nodeabytes = 0;
  }

  /* create info structure to allow for per-rank allocation */
  SC3E (sc3_MPI_Info_create (&m->info_noncontig));
  SC3E (sc3_MPI_Info_set
        (m->info_noncontig, "alloc_shared_noncontig",
         m->contiguous ? "false" : "true"));

  /* allocate shared memory for information on node and head communicators */
  SC3E (sc3_MPI_Win_allocate_shared
        (nodeabytes, sizeof (int),
         m->info_noncontig, m->nodecomm, &nodesizemem, &m->nodesizewin));
  if (m->noderank == 0) {
    SC3E (sc3_MPI_Win_lock (SC3_MPI_LOCK_EXCLUSIVE, 0, SC3_MPI_MODE_NOCHECK,
                            m->nodesizewin));
    nodesizemem[0] = m->num_nodes = headsize;
    nodesizemem[1] = m->node_num = headrank;
    m->node_sizes = &nodesizemem[2];
    m->node_frank = m->mpirank;

    /* allgather information about all nodes and compute offsets */
    SC3E (sc3_MPI_Allgather (&m->nodesize, 1, SC3_MPI_INT,
                             m->node_sizes, 1, SC3_MPI_INT, m->headcomm));
    *(ofs = m->node_offsets = &nodesizemem[2 + headsize]) = 0;
    for (p = 0; p < headsize; ++p) {
      next = *ofs + m->node_sizes[p];
      *++ofs = next;
    }
    SC3A_CHECK (m->node_offsets[headrank] == m->mpirank);
    SC3A_CHECK (m->node_offsets[headsize] == m->mpisize);
    SC3A_CHECK (m->node_frank == m->node_offsets[m->node_num]);

    /* make sure shared memory contents are consistent */
    SC3E (sc3_MPI_Win_unlock (0, m->nodesizewin));
    SC3E (sc3_MPI_Barrier (m->nodecomm));
    SC3E (sc3_MPI_Win_lock (SC3_MPI_LOCK_SHARED, 0,
                            SC3_MPI_MODE_NOCHECK, m->nodesizewin));
  }
  else {
    SC3E (sc3_MPI_Win_shared_query (m->nodesizewin, 0,
                                    &nodeabytes, &dispunit, &nodesizemem));
    SC3A_CHECK (nodeabytes >= (sc3_MPI_Aint_t) sizeof (int));
    SC3A_CHECK (dispunit == (int) sizeof (int));
    SC3A_CHECK (nodesizemem != NULL);

    /* access shared memory written by other process */
    SC3E (sc3_MPI_Barrier (m->nodecomm));
    SC3E (sc3_MPI_Win_lock (SC3_MPI_LOCK_SHARED, m->noderank,
                            SC3_MPI_MODE_NOCHECK, m->nodesizewin));
    m->num_nodes = nodesizemem[0];
    SC3A_CHECK (nodeabytes >=
                (sc3_MPI_Aint_t) ((2 + 2 * m->num_nodes + 1) * sizeof (int)));
    m->node_num = nodesizemem[1];
    m->node_sizes = &nodesizemem[2];
    m->node_offsets = &nodesizemem[2 + m->num_nodes];
    m->node_frank = m->node_offsets[m->node_num];
  }
  return NULL;
}

sc3_error_t        *
sc3_mpienv_setup (sc3_mpienv_t * m)
{
  /* verify call and input */
  SC3A_IS (sc3_mpienv_is_new, m);

  /* query input communicator */
  SC3E (sc3_MPI_Comm_size (m->mpicomm, &m->mpisize));
  SC3E (sc3_MPI_Comm_rank (m->mpicomm, &m->mpirank));

  /* create one communicator on each shared-memory node */
  if (!m->shared) {
    m->nodecomm = SC3_MPI_COMM_SELF;
  }
  else {
    SC3E (sc3_MPI_Comm_split_type (m->mpicomm, SC3_MPI_COMM_TYPE_SHARED,
                                   0, SC3_MPI_INFO_NULL, &m->nodecomm));
  }
  SC3E (sc3_MPI_Comm_size (m->nodecomm, &m->nodesize));
  SC3E (sc3_MPI_Comm_rank (m->nodecomm, &m->noderank));

  /* create communicator that contains the first rank on each node */
  if (!m->shared) {
    SC3A_CHECK (m->nodesize == 1);
    SC3A_CHECK (m->noderank == 0);
    m->headcomm = m->mpicomm;
  }
  else {
    SC3E (sc3_MPI_Comm_split (m->mpicomm, m->noderank == 0 ? 0 :
                              SC3_MPI_UNDEFINED, 0, &m->headcomm));
  }
  SC3A_CHECK ((m->noderank != 0) == (m->headcomm == SC3_MPI_COMM_NULL));

  /* create shared information on all node sizes */
  if (!m->shared) {
    m->num_nodes = m->mpisize;
    m->node_num = m->node_frank = m->mpirank;
  }
  else {
    SC3E (mpienv_setup_nodemem (m));
  }

  /* set mpienv to setup state */
  m->setup = 1;
  SC3A_IS (sc3_mpienv_is_setup, m);
  return NULL;
}

sc3_error_t        *
sc3_mpienv_ref (sc3_mpienv_t * m)
{
  SC3E (sc3_refcount_ref (&m->rc));
  return NULL;
}

sc3_error_t        *
sc3_mpienv_unref (sc3_mpienv_t ** mp)
{
  int                 waslast;
  sc3_allocator_t    *mator;
  sc3_mpienv_t       *m;
  sc3_error_t        *leak = NULL;

  SC3E_INOUTP (mp, m);
  SC3A_IS (sc3_mpienv_is_valid, m);
  SC3E (sc3_refcount_unref (&m->rc, &waslast));
  if (waslast) {
    *mp = NULL;
    mator = m->mator;

    if (m->setup) {
      /* deallocate data created on setup here */
      if (m->shared) {
        SC3E (sc3_MPI_Win_unlock (m->noderank, m->nodesizewin));
        SC3E (sc3_MPI_Win_free (&m->nodesizewin));
        if (m->noderank == 0) {
          SC3E (sc3_MPI_Comm_free (&m->headcomm));
        }
        SC3E (sc3_MPI_Comm_free (&m->nodecomm));
        SC3E (sc3_MPI_Info_free (&m->info_noncontig));
      }
    }

    /* deallocate data knonw on setup here */
    if (m->commdup) {
      SC3E (sc3_MPI_Comm_free (&m->mpicomm));
    }

    SC3E (sc3_allocator_free (mator, m));
    SC3L (&leak, sc3_allocator_unref (&mator));
  }
  return leak;
}

sc3_error_t        *
sc3_mpienv_destroy (sc3_mpienv_t ** mp)
{
  sc3_error_t        *leak = NULL;
  sc3_mpienv_t       *m;

  SC3E_INULLP (mp, m);
  SC3L_DEMAND (&leak, sc3_refcount_is_last (&m->rc, NULL));
  SC3L (&leak, sc3_mpienv_unref (&m));

  SC3A_CHECK (m == NULL || leak != NULL);
  return leak;
}

sc3_error_t        *
sc3_mpienv_get_shared (sc3_mpienv_t * m, int *shared)
{
  SC3E_RETVAL (shared, 0);
  SC3A_IS (sc3_mpienv_is_setup, m);

  *shared = m->shared;
  return NULL;
}

sc3_error_t        *
sc3_mpienv_get_noderank (sc3_mpienv_t * m, int *noderank)
{
  SC3E_RETVAL (noderank, -1);
  SC3A_IS (sc3_mpienv_is_setup, m);

  *noderank = m->noderank;
  return NULL;
}

sc3_error_t        *
sc3_mpienv_get_nodesize (sc3_mpienv_t * m, int *nodesize)
{
  SC3E_RETVAL (nodesize, -1);
  SC3A_IS (sc3_mpienv_is_setup, m);

  *nodesize = m->nodesize;
  return NULL;
}
