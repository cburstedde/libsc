/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include <sc_containers.h>
#include <sc_sort.h>

typedef struct sc_psort_peer
{
  int                 received;
  int                 prank;
  size_t              length;
  char               *buffer;
  char               *my_start;
}
sc_psort_peer_t;

typedef struct sc_psort
{
  sc_MPI_Comm         mpicomm;
  int                 num_procs, rank;
  size_t              size;
  size_t              my_lo, my_hi, my_count;
  size_t             *gmemb;
  char               *my_base;
}
sc_psort_t;

/* qsort is not reentrant, so we do the inverse static */
static int          (*sc_compare) (const void *, const void *);
static int
sc_icompare (const void *v1, const void *v2)
{
  return sc_compare (v2, v1);
}

static              size_t
sc_bsearch_cumulative (const size_t * cumulative, size_t nmemb,
                       size_t pos, size_t guess)
{
  size_t              proc_low, proc_high;

  proc_low = 0;
  proc_high = nmemb - 1;

  for (;;) {
    SC_ASSERT (proc_low <= proc_high);
    SC_ASSERT (proc_low < nmemb && proc_high < nmemb);
    SC_ASSERT (proc_low <= guess && guess <= proc_high);

    /* check if pos is on a lower owner than guess */
    if (pos < cumulative[guess]) {
      proc_high = guess - 1;
      guess = (proc_low + proc_high + 1) / 2;
      continue;
    }

    /* check if pos is on a higher owner than guess */
    if (cumulative[guess + 1] <= pos) {
      proc_low = guess + 1;
      guess = (proc_low + proc_high) / 2;
      continue;
    }

    /* otherwise guess is the correct owner */
    break;
  }

  /* make sure we found a valid owner with nonzero count */
  SC_ASSERT (guess < nmemb);
  SC_ASSERT (cumulative[guess] <= pos && pos < cumulative[guess + 1]);
  return guess;
}

static void
sc_merge_bitonic (sc_psort_t * pst, size_t lo, size_t hi, int dir)
{
  const size_t        n = hi - lo;

  if (n > 1 && pst->my_hi > lo && pst->my_lo < hi) {
    const int           rank = pst->rank;
    const size_t        size = pst->size;
    size_t              k, n2;
    size_t              lo_end, hi_beg;
    size_t              lo_length, hi_length;
    size_t              offset, max_length;
    int                 lo_owner, hi_owner;
    int                 num_peers, remaining, outcount;
    int                 mpiret;
    sc_array_t          a, *pa = &a;
    sc_array_t          r, *pr = &r;
    sc_array_t          s, *ps = &s;
    int                *wait_indices;
    sc_MPI_Status      *recv_statuses;
    sc_psort_peer_t    *peer;

    for (k = 1; k < n;) {
      k = k << 1;
    }
    n2 = k >> 1;
    SC_ASSERT (n2 >= n / 2 && n2 < n);

    lo_end = lo + n - n2;
    hi_beg = lo + n2;
    SC_ASSERT (lo_end <= hi_beg && lo_end - lo == hi - hi_beg);

    sc_array_init (pa, sizeof (sc_psort_peer_t));
    sc_array_init (pr, sizeof (sc_MPI_Request));
    sc_array_init (ps, sizeof (sc_MPI_Request));

    /* loop 1: initiate communication */
    lo_owner = hi_owner = rank;
    offset = max_length = 0;
    for (offset = 0; offset < lo_end - lo; offset += max_length) {
      lo_owner =
        (int) sc_bsearch_cumulative (pst->gmemb, (size_t) pst->num_procs,
                                     lo + offset, (size_t) lo_owner);
      lo_length = pst->gmemb[lo_owner + 1] - (lo + offset);
      hi_owner =
        (int) sc_bsearch_cumulative (pst->gmemb, (size_t) pst->num_procs,
                                     hi_beg + offset, (size_t) hi_owner);
      hi_length = pst->gmemb[hi_owner + 1] - (hi_beg + offset);
      max_length = lo_end - (lo + offset);
      max_length = SC_MIN (max_length, SC_MIN (lo_length, hi_length));
      SC_ASSERT (max_length > 0);

      if (lo_owner == rank && hi_owner != rank) {
        char               *lo_data;
        sc_MPI_Request     *rreq, *sreq;
        const int           bytes = (int) (max_length * size);

        /* receive high part, send low part */
        peer = (sc_psort_peer_t *) sc_array_push (pa);
        rreq = (sc_MPI_Request *) sc_array_push (pr);
        sreq = (sc_MPI_Request *) sc_array_push (ps);
        lo_data = pst->my_base + (lo + offset - pst->my_lo) * size;

        peer->received = 0;
        peer->prank = hi_owner;
        peer->length = max_length;
        peer->buffer = SC_ALLOC (char, bytes);
        peer->my_start = lo_data;
        mpiret = sc_MPI_Irecv (peer->buffer, bytes, sc_MPI_BYTE,
                            peer->prank, SC_TAG_PSORT_HI, pst->mpicomm, rreq);
        SC_CHECK_MPI (mpiret);

        SC_ASSERT (lo_data >= pst->my_base);
        SC_ASSERT (lo_data + bytes <= pst->my_base + pst->my_count * size);
        mpiret = sc_MPI_Isend (lo_data, bytes, sc_MPI_BYTE,
                            peer->prank, SC_TAG_PSORT_LO, pst->mpicomm, sreq);
        SC_CHECK_MPI (mpiret);
      }
      else if (lo_owner != rank && hi_owner == rank) {
        char               *hi_data;
        sc_MPI_Request     *rreq, *sreq;
        const int           bytes = (int) (max_length * size);

        /* receive low part, send high part */
        peer = (sc_psort_peer_t *) sc_array_push (pa);
        rreq = (sc_MPI_Request *) sc_array_push (pr);
        sreq = (sc_MPI_Request *) sc_array_push (ps);
        hi_data = pst->my_base + (hi_beg + offset - pst->my_lo) * size;

        peer->received = 0;
        peer->prank = lo_owner;
        peer->length = max_length;
        peer->buffer = SC_ALLOC (char, bytes);
        peer->my_start = hi_data;

        mpiret = sc_MPI_Irecv (peer->buffer, bytes, sc_MPI_BYTE,
                            peer->prank, SC_TAG_PSORT_LO, pst->mpicomm, rreq);
        SC_CHECK_MPI (mpiret);

        SC_ASSERT (hi_data >= pst->my_base);
        SC_ASSERT (hi_data + bytes <= pst->my_base + pst->my_count * size);
        mpiret = sc_MPI_Isend (hi_data, bytes, sc_MPI_BYTE,
                            peer->prank, SC_TAG_PSORT_HI, pst->mpicomm, sreq);
        SC_CHECK_MPI (mpiret);
      }
    }

    /* loop 2: local computation */
    lo_owner = hi_owner = rank;
    offset = max_length = 0;
    for (offset = 0; offset < lo_end - lo; offset += max_length) {
      lo_owner =
        (int) sc_bsearch_cumulative (pst->gmemb, (size_t) pst->num_procs,
                                     lo + offset, (size_t) lo_owner);
      lo_length = pst->gmemb[lo_owner + 1] - (lo + offset);
      hi_owner =
        (int) sc_bsearch_cumulative (pst->gmemb, (size_t) pst->num_procs,
                                     hi_beg + offset, (size_t) hi_owner);
      hi_length = pst->gmemb[hi_owner + 1] - (hi_beg + offset);
      max_length = lo_end - (lo + offset);
      max_length = SC_MIN (max_length, SC_MIN (lo_length, hi_length));
      SC_ASSERT (max_length > 0);

      if (lo_owner == rank && hi_owner == rank) {
        size_t              zz;
        char               *lo_data, *hi_data;
        char               *temp = SC_ALLOC (char, size);

        /* local comparisons only */
        lo_data = pst->my_base + (lo + offset - pst->my_lo) * size;
        hi_data = pst->my_base + (hi_beg + offset - pst->my_lo) * size;
        for (zz = 0; zz < max_length; ++zz) {
          if (dir == (sc_compare (lo_data, hi_data) > 0)) {
            memcpy (temp, lo_data, size);
            memcpy (lo_data, hi_data, size);
            memcpy (hi_data, temp, size);
          }
          lo_data += size;
          hi_data += size;
        }
        SC_FREE (temp);
      }
    }

    /* loop 3: receive and compute with received data */
    outcount = 0;
    num_peers = (int) pa->elem_count;
    wait_indices = SC_ALLOC (int, num_peers);
    recv_statuses = SC_ALLOC (sc_MPI_Status, num_peers);
    for (remaining = num_peers; remaining > 0; remaining -= outcount) {
      int                 i;

      mpiret = sc_MPI_Waitsome (num_peers, (sc_MPI_Request *) pr->array,
                             &outcount, wait_indices, recv_statuses);
      SC_CHECK_MPI (mpiret);
      SC_ASSERT (outcount != sc_MPI_UNDEFINED);
      SC_ASSERT (outcount > 0);
      for (i = 0; i < outcount; ++i) {
        size_t              zz;
        char               *lo_data, *hi_data;
#ifdef SC_DEBUG
        sc_MPI_Status      *jstatus;

        jstatus = &recv_statuses[i];
#endif

        /* retrieve peer information */
        peer = (sc_psort_peer_t *) sc_array_index_int (pa, wait_indices[i]);
        SC_ASSERT (!peer->received);
        SC_ASSERT (peer->prank != rank);
        SC_ASSERT (peer->prank == jstatus->sc_MPI_SOURCE);

        /* comparisons with remote peer */
        if (rank < peer->prank) {
          lo_data = peer->my_start;
          hi_data = peer->buffer;
          for (zz = 0; zz < peer->length; ++zz) {
            if (dir == (sc_compare (lo_data, hi_data) > 0)) {
              memcpy (lo_data, hi_data, size);
            }
            lo_data += size;
            hi_data += size;
          }
        }
        else {
          lo_data = peer->buffer;
          hi_data = peer->my_start;
          for (zz = 0; zz < peer->length; ++zz) {
            if (dir == (sc_compare (lo_data, hi_data) > 0)) {
              memcpy (hi_data, lo_data, size);
            }
            lo_data += size;
            hi_data += size;
          }
        }

        /* close down this peer */
        SC_FREE (peer->buffer);
        peer->received = 1;
      }
    }
    SC_ASSERT (remaining == 0);
    SC_FREE (recv_statuses);
    SC_FREE (wait_indices);

    /* clean up */
    if (num_peers > 0) {
      mpiret = sc_MPI_Waitall (num_peers, (sc_MPI_Request *) ps->array,
                            sc_MPI_STATUSES_IGNORE);
      SC_CHECK_MPI (mpiret);
    }
    sc_array_reset (pa);
    sc_array_reset (pr);
    sc_array_reset (ps);

    /* recursive merge */
    sc_merge_bitonic (pst, lo, lo + n2, dir);
    sc_merge_bitonic (pst, lo + n2, hi, dir);
  }
}

static void
sc_psort_bitonic (sc_psort_t * pst, size_t lo, size_t hi, int dir)
{
  const size_t        n = hi - lo;

  if (n > 1 && pst->my_hi > lo && pst->my_lo < hi) {
    if (lo >= pst->my_lo && hi <= pst->my_hi) {
      qsort (pst->my_base + (lo - pst->my_lo) * pst->size,
             n, pst->size, dir ? sc_compare : sc_icompare);
    }
    else {
      const size_t        n2 = n / 2;

      sc_psort_bitonic (pst, lo, lo + n2, !dir);
      sc_psort_bitonic (pst, lo + n2, hi, dir);
      sc_merge_bitonic (pst, lo, hi, dir);
    }
  }
}

void
sc_psort (sc_MPI_Comm mpicomm, void *base, size_t * nmemb, size_t size,
          int (*compar) (const void *, const void *))
{
  int                 mpiret;
  int                 num_procs, rank;
  int                 i;
  size_t              total;
  size_t             *gmemb;
  sc_psort_t          pst;

  SC_ASSERT (sc_compare == NULL);

#ifndef SC_DEBUG
  SC_ABORT ("sc_psort is still buggy, don't use it yet");
#endif

  /* get basic MPI information */
  mpiret = sc_MPI_Comm_size (mpicomm, &num_procs);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  /* alloc global offset array */
  gmemb = SC_ALLOC (size_t, num_procs + 1);
  gmemb[0] = 0;
  for (i = 0; i < num_procs; ++i) {
    gmemb[i + 1] = gmemb[i] + nmemb[i];
  }

  /* set up internal state and call recursion */
  pst.mpicomm = mpicomm;
  pst.num_procs = num_procs;
  pst.rank = rank;
  pst.size = size;
  pst.my_lo = gmemb[rank];
  pst.my_hi = gmemb[rank + 1];
  pst.my_count = nmemb[rank];
  SC_ASSERT (pst.my_lo + pst.my_count == pst.my_hi);
  pst.gmemb = gmemb;
  pst.my_base = (char *) base;
  sc_compare = compar;
  total = gmemb[num_procs];
  SC_GLOBAL_LDEBUGF ("Total values to sort %lld\n", (long long) total);
  sc_psort_bitonic (&pst, 0, total, 1);

  /* clean up and free memory */
  sc_compare = NULL;
  SC_FREE (gmemb);
}
