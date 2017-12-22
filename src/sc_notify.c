/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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
#include <sc_functions.h>
#include <sc_notify.h>

int                 sc_notify_nary_ntop = 2;
int                 sc_notify_nary_nint = 2;
int                 sc_notify_nary_nbot = 2;

#if 0

static void
sc_array_init_invalid (sc_array_t * array)
{
  SC_ASSERT (array != NULL);

  array->elem_size = (size_t) ULONG_MAX;
  array->elem_count = (size_t) ULONG_MAX;
  array->byte_alloc = -1;
  array->array = NULL;
}

static int
sc_array_is_invalid (sc_array_t * array)
{
  SC_ASSERT (array != NULL);

  return (array->elem_size == (size_t) ULONG_MAX &&
          array->elem_count == (size_t) ULONG_MAX) &&
    (array->byte_alloc == -1 && array->array == NULL);
}

#endif

int
sc_notify_allgather (int *receivers, int num_receivers,
                     int *senders, int *num_senders, sc_MPI_Comm mpicomm)
{
  int                 i, j;
  int                 found_num_senders;
  int                 mpiret;
  int                 mpisize, mpirank;
  int                 total_num_receivers;
  int                *procs_num_receivers;
  int                *offsets_num_receivers;
  int                *all_receivers;

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  procs_num_receivers = SC_ALLOC (int, mpisize);
  mpiret = sc_MPI_Allgather (&num_receivers, 1, sc_MPI_INT,
                             procs_num_receivers, 1, sc_MPI_INT, mpicomm);
  SC_CHECK_MPI (mpiret);

  offsets_num_receivers = SC_ALLOC (int, mpisize);
  total_num_receivers = 0;
  for (i = 0; i < mpisize; ++i) {
    offsets_num_receivers[i] = total_num_receivers;
    total_num_receivers += procs_num_receivers[i];
  }
  all_receivers = SC_ALLOC (int, total_num_receivers);
  mpiret = sc_MPI_Allgatherv (receivers, num_receivers, sc_MPI_INT,
                              all_receivers, procs_num_receivers,
                              offsets_num_receivers, sc_MPI_INT, mpicomm);
  SC_CHECK_MPI (mpiret);

  SC_ASSERT (procs_num_receivers[mpirank] == num_receivers);
  found_num_senders = 0;
  for (i = 0; i < mpisize; ++i) {
    for (j = 0; j < procs_num_receivers[i]; ++j) {
      if (all_receivers[offsets_num_receivers[i] + j] == mpirank) {
        senders[found_num_senders++] = i;
        break;
      }
    }
  }
  *num_senders = found_num_senders;
  SC_FREE (procs_num_receivers);
  SC_FREE (offsets_num_receivers);
  SC_FREE (all_receivers);

  return sc_MPI_SUCCESS;
}

int
sc_notify_nary (int *receivers, int num_receivers,
                int *senders, int *num_senders, sc_MPI_Comm mpicomm)
{
  sc_notify_ext (receivers, num_receivers, senders, num_senders,
                 sc_notify_nary_ntop, sc_notify_nary_nint,
                 sc_notify_nary_nbot, mpicomm);

  return sc_MPI_SUCCESS;
}

/** Encode the receiver list into an array for input.
 * \param [out] input       This function initializes the array.
 *                          All prior content will be overwritten and lost.
 *                          On output, array holding integers.
 * \param [in] receivers        See \ref sc_notify.
 * \param [in] num_receivers    See \ref sc_notify.
 * \param [in] mpisize          Number of MPI processes.
 * \param [in] mpirank          MPI rank of this process.
 */
static void
sc_notify_init_input (sc_array_t * input, int *receivers, int num_receivers,
                      int mpisize, int mpirank)
{
  int                 rec;
  int                 i;
  int                *pint;

  SC_ASSERT (input != NULL);

  SC_ASSERT (num_receivers >= 0);
  SC_ASSERT (num_receivers == 0 || receivers != NULL);

  sc_array_init_size (input, sizeof (int), 3 * num_receivers);
  rec = -1;
  for (i = 0; i < num_receivers; ++i) {
    SC_ASSERT (rec < receivers[i]);
    rec = receivers[i];
    SC_ASSERT (0 <= rec && rec < mpisize);
    pint = (int *) sc_array_index_int (input, 3 * i);
    pint[0] = rec;
    pint[1] = 1;
    pint[2] = mpirank;
  }
}

/** Decode sender list into an array for output.
 * \param [in] output       This array holds integer data in prescribed format.
 *                          This function calls \ref sc_array_reset on it.
 * \param [in] senders      See \ref sc_notify.
 * \param [in] num_senders  See \ref sc_notify.
 * \param [in] mpisize      Number of MPI processes.
 * \param [in] mpirank      MPI rank of this process.
 */
static void
sc_notify_reset_output (sc_array_t * output, int *senders, int *num_senders,
                        int mpisize, int mpirank)
{
  int                 found_num_senders;
  int                 i;
  int                *pint;

  SC_ASSERT (output != NULL);
  SC_ASSERT (output->elem_size == sizeof (int));

  SC_ASSERT (senders != NULL);
  SC_ASSERT (num_senders != NULL);

  found_num_senders = 0;
  if (output->elem_count > 0) {
    pint = (int *) sc_array_index_int (output, 0);
    SC_ASSERT (pint[0] == mpirank);
    found_num_senders = pint[1];
    SC_ASSERT (found_num_senders > 0);
    SC_ASSERT (output->elem_count == 2 + (size_t) found_num_senders);
    for (i = 0; i < found_num_senders; ++i) {
      senders[i] = pint[2 + i];
    }
  }
  *num_senders = found_num_senders;

  sc_array_reset (output);
}

/*
 * Format of variable-length records:
 * forall(torank): (torank, howmanyfroms, listoffromranks).
 * The records must be ordered ascending by torank.
 */
static void
sc_notify_merge (sc_array_t * output, sc_array_t * input, sc_array_t * second)
{
  int                 i, ir, j, jr, k;
  int                 torank, numfroms;
  int                *pint, *psec, *pout;

  SC_ASSERT (input->elem_size == sizeof (int));
  SC_ASSERT (second->elem_size == sizeof (int));
  SC_ASSERT (output->elem_size == sizeof (int));
  SC_ASSERT (output->elem_count == 0);

  i = ir = 0;
  torank = -1;
  for (;;) {
    pint = psec = NULL;
    while (i < (int) input->elem_count) {
      /* ignore data that was sent to the peer earlier */
      pint = (int *) sc_array_index_int (input, i);
      if (pint[0] == -1) {
        i += 2 + pint[1];
        SC_ASSERT (i <= (int) input->elem_count);
        pint = NULL;
      }
      else {
        break;
      }
    }
    if (ir < (int) second->elem_count) {
      psec = (int *) sc_array_index_int (second, ir);
    }
    if (pint == NULL && psec == NULL) {
      /* all data is processed and we are done */
      break;
    }
    else if (pint != NULL && psec == NULL) {
      /* copy input to output */
    }
    else if (pint == NULL && psec != NULL) {
      /* copy second to output */
    }
    else {
      /* both arrays have remaining elements and need to be compared */
      if (pint[0] < psec[0]) {
        /* copy input to output */
        psec = NULL;
      }
      else if (pint[0] > psec[0]) {
        /* copy second to output */
        pint = NULL;
      }
      else {
        /* both arrays have data for the same processor */
        SC_ASSERT (torank < pint[0] && pint[0] == psec[0]);
        torank = pint[0];
        SC_ASSERT (pint[1] > 0 && psec[1] > 0);
        SC_ASSERT (i + 2 + pint[1] <= (int) input->elem_count);
        SC_ASSERT (ir + 2 + psec[1] <= (int) second->elem_count);
        numfroms = pint[1] + psec[1];
        pout = (int *) sc_array_push_count (output, 2 + numfroms);
        pout[0] = torank;
        pout[1] = numfroms;
        k = 2;
        j = jr = 0;

        while (j < pint[1] || jr < psec[1]) {
          SC_ASSERT (j >= pint[1] || jr >= psec[1]
                     || pint[2 + j] != psec[2 + jr]);
          if (j < pint[1] && (jr >= psec[1] || pint[2 + j] < psec[2 + jr])) {
            pout[k++] = pint[2 + j++];
          }
          else {
            SC_ASSERT (jr < psec[1]);
            pout[k++] = psec[2 + jr++];
          }
        }
        SC_ASSERT (k == 2 + numfroms);
        i += 2 + pint[1];
        ir += 2 + psec[1];
        continue;
      }
    }

    /* we need to copy exactly one buffer to the output array */
    if (psec == NULL) {
      SC_ASSERT (pint != NULL);
      SC_ASSERT (torank < pint[0]);
      torank = pint[0];
      SC_ASSERT (i + 2 + pint[1] <= (int) input->elem_count);
      numfroms = pint[1];
      SC_ASSERT (numfroms > 0);
      pout = (int *) sc_array_push_count (output, 2 + numfroms);
      memcpy (pout, pint, (2 + numfroms) * sizeof (int));
      i += 2 + numfroms;
    }
    else {
      SC_ASSERT (pint == NULL);
      SC_ASSERT (torank < psec[0]);
      torank = psec[0];
      SC_ASSERT (ir + 2 + psec[1] <= (int) second->elem_count);
      numfroms = psec[1];
      SC_ASSERT (numfroms > 0);
      pout = (int *) sc_array_push_count (output, 2 + numfroms);
      memcpy (pout, psec, (2 + numfroms) * sizeof (int));
      ir += 2 + numfroms;
    }
  }
  SC_ASSERT (i == (int) input->elem_count);
  SC_ASSERT (ir == (int) second->elem_count);
}

/** Internally used function to execute the sc_notify recursion.
 * The internal data format of the input and output arrays is as follows:
 * forall(torank): (torank, howmanyfroms, listoffromranks).
 * \param [in] mpicomm      Communicator to use.
 * \param [in] start        Offset of range of ranks.
 * \param [in] me           Current MPI process in range of ranks.
 * \param [in] length       Next-biggest or equal power of 2 for range.
 * \param [in] groupsize    Global count of ranks.
 * \param [in,out] array    Input/output array of integers.
 */
static void
sc_notify_recursive (sc_MPI_Comm mpicomm, int start, int me, int length,
                     int groupsize, sc_array_t * array)
{
  int                 i;
  int                 mpiret;
  int                 num_ta;
  int                 length2, half;
  int                 torank, numfroms;
#ifdef SC_ENABLE_DEBUG
  int                 j, fromrank, num_out;
#endif
  int                 peer, peer2, source;
  int                 tag, count;
  int                *pint, *pout;
  sc_array_t         *sendbuf, *recvbuf, morebuf;
  sc_MPI_Request      outrequest;
  sc_MPI_Status       instatus;

  tag = SC_TAG_NOTIFY_RECURSIVE + SC_LOG2_32 (length);
  length2 = length / 2;
  SC_ASSERT (start <= me && me < start + length && me < groupsize);
  SC_ASSERT (start % length == 0);
  SC_ASSERT (array != NULL && array->elem_size == sizeof (int));

  if (length > 1) {
    /* execute recursion in-place */
    if (me < start + length2) {
      half = 0;
      sc_notify_recursive (mpicomm, start, me, length2, groupsize, array);
    }
    else {
      half = 1;
      sc_notify_recursive (mpicomm, start + length2, me, length2,
                           groupsize, array);
    }

    /* determine communication pattern */
    peer = me ^ length2;
    SC_ASSERT (start <= peer && peer < start + length);
    if (peer < groupsize) {
      /* peer exists even when mpisize is not a power of 2 */
      SC_ASSERT ((!half && me < peer) || (half && me > peer));
    }
    else {
      /* peer does not exist; send to a lower processor if nonnegative */
      SC_ASSERT (!half && me < peer);
      peer -= length;
      SC_ASSERT (start - length2 <= peer && peer < start);
    }
    peer2 = me + length2;
    if (half && peer2 < groupsize && (peer2 ^ length2) >= groupsize) {
      /* we will receive from peer2 who has no peer itself */
      SC_ASSERT (start + length <= peer2 && !(peer2 & length2));
    }
    else {
      peer2 = -1;
    }
    SC_ASSERT (peer >= 0 || peer2 == -1);

    sendbuf = sc_array_new (sizeof (int));
    if (peer >= 0) {
      /* send one message */
      num_ta = (int) array->elem_count;
      torank = -1;
      for (i = 0; i < num_ta;) {
        pint = (int *) sc_array_index_int (array, i);
        SC_ASSERT (torank < pint[0]);
        torank = pint[0];
        SC_ASSERT (torank % length == me % length ||
                   torank % length == peer % length);
        numfroms = pint[1];
        SC_ASSERT (numfroms > 0);
        if (torank % length != me % length) {
          /* this set needs to be sent and is marked invalid in the array */
          pout = (int *) sc_array_push_count (sendbuf, 2 + numfroms);
          memcpy (pout, pint, (2 + numfroms) * sizeof (int));
          pint[0] = -1;
        }
        else {
          /* this set remains local and valid in the array */
        }
        i += 2 + numfroms;
      }
      mpiret = sc_MPI_Isend (sendbuf->array, (int) sendbuf->elem_count,
                             sc_MPI_INT, peer, tag, mpicomm, &outrequest);
      SC_CHECK_MPI (mpiret);
    }

    recvbuf = sc_array_new (sizeof (int));
    if (peer >= start) {
      /* receive one message */
      mpiret = sc_MPI_Probe (sc_MPI_ANY_SOURCE, tag, mpicomm, &instatus);
      SC_CHECK_MPI (mpiret);
      source = instatus.MPI_SOURCE;
      SC_ASSERT (source >= 0 && (source == peer || source == peer2));
      mpiret = sc_MPI_Get_count (&instatus, sc_MPI_INT, &count);
      SC_CHECK_MPI (mpiret);
      sc_array_resize (recvbuf, (size_t) count);
      mpiret = sc_MPI_Recv (recvbuf->array, count, sc_MPI_INT, source,
                            tag, mpicomm, sc_MPI_STATUS_IGNORE);
      SC_CHECK_MPI (mpiret);

      if (peer2 >= 0) {
        /* merge the owned and received arrays */
        sc_array_init (&morebuf, sizeof (int));
        sc_notify_merge (&morebuf, array, recvbuf);
        sc_array_reset (array);

        /* receive second message */
        source = (source == peer2 ? peer : peer2);
        mpiret = sc_MPI_Probe (source, tag, mpicomm, &instatus);
        SC_CHECK_MPI (mpiret);
        mpiret = sc_MPI_Get_count (&instatus, sc_MPI_INT, &count);
        SC_CHECK_MPI (mpiret);
        sc_array_resize (recvbuf, (size_t) count);
        mpiret = sc_MPI_Recv (recvbuf->array, count, sc_MPI_INT, source,
                              tag, mpicomm, sc_MPI_STATUS_IGNORE);
        SC_CHECK_MPI (mpiret);

        /* merge the second received array */
        sc_notify_merge (array, &morebuf, recvbuf);
        sc_array_reset (&morebuf);
      }
    }
    if (peer2 == -1) {
      sc_array_init (&morebuf, sizeof (int));
      sc_notify_merge (&morebuf, array, recvbuf);
      sc_array_reset (array);
      *array = morebuf;
    }
    sc_array_destroy (recvbuf);

    if (peer >= 0) {
      /* complete send call */
      mpiret = sc_MPI_Wait (&outrequest, sc_MPI_STATUS_IGNORE);
      SC_CHECK_MPI (mpiret);
    }
    sc_array_destroy (sendbuf);
  }
  else {
    /* end of recursion with nothing to do */
  }

#ifdef SC_ENABLE_DEBUG
  /* verify recursion invariant */
  num_out = (int) array->elem_count;
  torank = -1;
  for (i = 0; i < num_out;) {
    pint = (int *) sc_array_index_int (array, i);
    SC_ASSERT (torank < pint[0]);
    torank = pint[0];
    SC_ASSERT (torank % length == me % length);
    numfroms = pint[1];
    SC_ASSERT (numfroms > 0);
    fromrank = -1;
    for (j = 0; j < numfroms; ++j) {
      SC_ASSERT (fromrank < pint[2 + j]);
      fromrank = pint[2 + j];
    }
    i += 2 + numfroms;
  }
#endif
}

int
sc_notify (int *receivers, int num_receivers,
           int *senders, int *num_senders, sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 mpisize, mpirank;
  int                 pow2length;
  sc_array_t          array;

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  pow2length = SC_ROUNDUP2_32 (mpisize);
  SC_ASSERT (num_receivers >= 0);
  SC_ASSERT (senders != NULL && num_senders != NULL);
  SC_ASSERT (pow2length / 2 < mpisize && mpisize <= pow2length);

  /* convert input variables into internal format */
  sc_notify_init_input (&array, receivers, num_receivers, mpisize, mpirank);

  /* execute the recursive algorithm */
  sc_notify_recursive (mpicomm, 0, mpirank, pow2length, mpisize, &array);

  /* convert internal format to output variables */
  sc_notify_reset_output (&array, senders, num_senders, mpisize, mpirank);

  return sc_MPI_SUCCESS;
}

typedef struct sc_notify_nary
{
  sc_MPI_Comm         mpicomm;
  int                 mpisize;
  int                 mpirank;
  int                 ntop, nint, nbot;
  int                 depth;
}
sc_notify_nary_t;

/** Internally used function to execute the sc_notify recursion.
 * The internal data format of the input and output arrays is as follows:
 * forall(torank): (torank, howmanyfroms, listoffromranks).
 * \param [in] mpicomm      Communicator to use.
 * \param [in] start        Offset of range of ranks.
 * \param [in] me           Current MPI process in range of ranks.
 * \param [in] length       Next-biggest or equal power of 2 for range.
 * \param [in] groupsize    Global count of ranks.
 * \param [in,out] array    Input/Output array of integers.
 */
static void
sc_notify_recursive_nary (const sc_notify_nary_t * nary, int level,
                          int start, int length, sc_array_t * array)
{
  int                 i, j;
  int                 mpiret;
  int                 num_ta;
  int                 torank, numfroms;
#ifdef SC_ENABLE_DEBUG
  int                 fromrank, num_out, missing;
  int                 freed;
#endif
  int                 peer, source;
  int                 tag, count;
  int                *pint, *pout;
  sc_array_t         *sendbuf, *recvbuf;
  sc_MPI_Request     *sendreq;
  sc_MPI_Status       instatus;

  sc_MPI_Comm         mpicomm;
  int                 me, groupsize;
  int                 depth;
  int                 divn;
  int                 lengthn;
  int                 mypart, topart, hipart;
  int                 remaining;
  int                 nsent, nrecv;
  int                 expon, power;
  sc_array_t          sendbufs, recvbufs;
  sc_array_t          sendreqs;
  sc_array_t         *aone, *atwo;

  SC_ASSERT (nary != NULL);

  mpicomm = nary->mpicomm;
  me = nary->mpirank;
  depth = nary->depth;
  groupsize = nary->mpisize;
  SC_ASSERT (0 <= me && me < groupsize);

  SC_ASSERT (0 <= level && level <= depth);
  SC_ASSERT (0 <= start && start <= me && me < start + length);
  SC_ASSERT (start % length == 0);

  SC_ASSERT (array != NULL && array->elem_size == sizeof (int));

  if (length > 1) {
    SC_ASSERT (level < depth);
    tag = SC_TAG_NOTIFY_NARY + level;

    /* find my position within child branch */
    divn =
      (level == depth - 1) ? nary->nbot :
      level == 0 ? nary->ntop : nary->nint;
    SC_ASSERT (length % divn == 0);
    lengthn = length / divn;
    mypart = (me - start) / lengthn;
    SC_ASSERT (0 <= mypart && mypart < divn);

    /* execute recursion on child branch */
    sc_notify_recursive_nary (nary, level + 1,
                              start + mypart * lengthn, lengthn, array);

    /* determine number of messages to receive */
    hipart = mypart + (groupsize - 1 - me) / lengthn;
    if (hipart < divn) {
      /* our group is short and we receive less messages
         or it is full and the next one is empty */
      nrecv = hipart;
    }
    else {
      nrecv = divn - 1;
      if (hipart < divn + mypart) {
        /* the next group is short and we receive more messages */
        remaining = hipart - divn + 1;
#ifdef SC_ENABLE_DEBUG
        missing = 2 * divn - 1 - hipart;
        SC_ASSERT (missing > 0);
#endif
        nrecv += remaining;
      }
    }
    SC_ASSERT (nrecv >= mypart);

    /* make room for one direct assignment buffer on same proc */
    sc_array_init_size (&recvbufs, sizeof (sc_array_t), nrecv + 1);

    /* prepare send buffers */
    nsent = 0;
    sc_array_init_size (&sendbufs, sizeof (sc_array_t), divn);
    sc_array_init_size (&sendreqs, sizeof (sc_MPI_Request), divn);
    for (j = 0; j < divn; ++j) {
      /* all send buffers are initialized empty */
      sendbuf = (sc_array_t *) sc_array_index_int (&sendbufs, j);
      sc_array_init (sendbuf, sizeof (int));

      /* unused requests must be set to null */
      sendreq = (sc_MPI_Request *) sc_array_index_int (&sendreqs, j);

      /* does our peer exist? */
      peer = me + (j - mypart) * lengthn;
      SC_ASSERT (start <= peer && peer < start + length);
      if (peer >= groupsize) {
        peer -= length;
        if (peer < 0) {
          /* leave send buffer array undefined */
          *sendreq = sc_MPI_REQUEST_NULL;
          continue;
        }
      }

      /* we do not send to ourselves but fake it */
      if (j == mypart) {
        /* we identify send buffer and receive buffer on same proc */
        sendbuf = (sc_array_t *) sc_array_index_int (&recvbufs, j);
        sc_array_init (sendbuf, sizeof (int));
        *sendreq = sc_MPI_REQUEST_NULL;
        continue;
      }

      ++nsent;
    }
    SC_ASSERT (nsent < divn);

    /* go through current data and construct messages */
    num_ta = (int) array->elem_count;
    torank = -1;
    for (i = 0; i < num_ta;) {
      pint = (int *) sc_array_index_int (array, i);
      SC_ASSERT (torank < pint[0]);
      torank = pint[0];
      SC_ASSERT (0 <= torank && torank < groupsize);
      SC_ASSERT (torank % lengthn == me % lengthn);
      numfroms = pint[1];
      SC_ASSERT (numfroms > 0);
      topart = (torank % length) / lengthn;
      sendbuf = (sc_array_t *) sc_array_index_int
        (topart == mypart ? &recvbufs : &sendbufs, topart);
      pout = (int *) sc_array_push_count (sendbuf, 2 + numfroms);
      memcpy (pout, pint, (2 + numfroms) * sizeof (int));
      i += 2 + numfroms;
    }
    SC_ASSERT (i == num_ta);
    sc_array_reset (array);

    /* send messages */
    i = 0;
    for (j = 0; j < divn; ++j) {
      sendbuf = (sc_array_t *) sc_array_index_int (&sendbufs, j);
      sendreq = (sc_MPI_Request *) sc_array_index_int (&sendreqs, j);
      if (j == mypart) {
        SC_ASSERT (sendbuf->elem_count == 0);
        SC_ASSERT (*sendreq == sc_MPI_REQUEST_NULL);
        continue;
      }
      peer = me + (j - mypart) * lengthn;
      SC_ASSERT (start <= peer && peer < start + length);
      if (peer >= groupsize) {
        peer -= length;
        if (peer < 0) {
          SC_ASSERT (sendbuf->elem_count == 0);
          SC_ASSERT (*sendreq == sc_MPI_REQUEST_NULL);
          continue;
        }
      }
      mpiret = sc_MPI_Isend (sendbuf->array, (int) sendbuf->elem_count,
                             sc_MPI_INT, peer, tag, mpicomm, sendreq);
      SC_CHECK_MPI (mpiret);
      ++i;
    }
    SC_ASSERT (i == nsent);

    /* receive all messages */
    for (i = 0; i < nrecv; ++i) {
      mpiret = sc_MPI_Probe (sc_MPI_ANY_SOURCE, tag, mpicomm, &instatus);
      SC_CHECK_MPI (mpiret);
      source = instatus.MPI_SOURCE;
      SC_ASSERT (start <= source && source < start + 2 * length - 1);

#if 0
      SC_LDEBUGF ("Length %d lengthn %d me %d source %d\n",
                  length, lengthn, me, source);
#endif

      SC_ASSERT (source != me && (source - me + length) % lengthn == 0);
      if (source < me) {
        j = mypart - (me - source) / lengthn;
        SC_ASSERT (0 <= j && j < mypart);
      }
      else if (source < start + length) {
        j = mypart + (source - me) / lengthn;
        SC_ASSERT (mypart < j && j < divn);
      }
      else {
        j = divn + (source % length) / lengthn;
        SC_ASSERT (divn <= j && j < nrecv + 1);
      }
      mpiret = sc_MPI_Get_count (&instatus, sc_MPI_INT, &count);
      SC_CHECK_MPI (mpiret);
      recvbuf = (sc_array_t *) sc_array_index_int (&recvbufs, j);
      sc_array_init_size (recvbuf, sizeof (int), (size_t) count);
      mpiret = sc_MPI_Recv (recvbuf->array, count, sc_MPI_INT, source,
                            tag, mpicomm, sc_MPI_STATUS_IGNORE);
      SC_CHECK_MPI (mpiret);
    }

    /* run binary tree for a recursive merge of received data arrays */
#ifdef SC_ENABLE_DEBUG
    freed = 0;
#endif
    count = nrecv + 1;
    expon = 0;
    power = 1 << expon;
    while (0 + power < count) {
      for (i = 0; i + power < count; i += power << 1) {
        aone = (sc_array_t *) sc_array_index_int (&recvbufs, i);
        atwo = (sc_array_t *) sc_array_index_int (&recvbufs, i + power);
        sc_array_init (array, sizeof (int));
        sc_notify_merge (array, aone, atwo);

        /* the surviving array lives on in aone */
        sc_array_reset (aone);
        sc_array_reset (atwo);
        *aone = *array;
#ifdef SC_ENABLE_DEBUG
        ++freed;
#endif
      }
      power = 1 << ++expon;
    }
    SC_ASSERT (freed == nrecv);
    *array = *(sc_array_t *) sc_array_index_int (&recvbufs, 0);
    sc_array_reset (&recvbufs);

    /* wait for asynchronous send to complete */
    SC_ASSERT (sendreqs.elem_count == (size_t) divn);
    mpiret = sc_MPI_Waitall
      (divn, (sc_MPI_Request *) sendreqs.array, sc_MPI_STATUS_IGNORE);
    SC_CHECK_MPI (mpiret);
    sc_array_reset (&sendreqs);

    /* clean up send buffers */
    for (j = 0; j < divn; ++j) {
      sendbuf = (sc_array_t *) sc_array_index_int (&sendbufs, j);
      sc_array_reset (sendbuf);
    }
    sc_array_reset (&sendbufs);
  }
  else {
    /* end of recursion with nothing to do */
    SC_ASSERT (level == depth);
  }

#ifdef SC_ENABLE_DEBUG
  /* verify recursion invariant */
  num_out = (int) array->elem_count;
  torank = -1;
  for (i = 0; i < num_out;) {
    pint = (int *) sc_array_index_int (array, i);
    SC_ASSERT (torank < pint[0]);
    torank = pint[0];
    SC_ASSERT (torank % length == me % length);
    numfroms = pint[1];
    SC_ASSERT (numfroms > 0);
    fromrank = -1;
    for (j = 0; j < numfroms; ++j) {
      SC_ASSERT (fromrank < pint[2 + j]);
      fromrank = pint[2 + j];
    }
    i += 2 + numfroms;
  }
#endif
}

void
sc_notify_ext (int *receivers, int num_receivers,
               int *senders, int *num_senders,
               int ntop, int nint, int nbot, sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 mpisize, mpirank;
  int                 depth, prod;
  sc_array_t          sarray, *array = &sarray;
  sc_notify_nary_t    snary, *nary = &snary;

  SC_ASSERT (num_receivers >= 0);
  SC_ASSERT (senders != NULL && num_senders != NULL);

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  /* determine depth of tree */
  if (mpisize == 1) {
    /* depth = 0; */
    if (num_receivers > 0) {
      SC_ASSERT (num_receivers == 1);
      SC_ASSERT (receivers[0] == 0);
      senders[0] = 0;
    }
    *num_senders = num_receivers;

    /* we return if there is only one process */
    return;
  }
  if (mpisize <= nbot) {
    depth = 1;
    prod = nbot;
  }
  else {
    depth = 2;
    for (prod = nbot * ntop; prod < mpisize; prod *= nint) {
      ++depth;
    }
    SC_ASSERT (prod == ntop * sc_intpow (nint, depth - 2) * nbot);
    SC_ASSERT (depth < 3
               || mpisize > ntop * sc_intpow (nint, depth - 3) * nbot);
  }
  SC_ASSERT (mpisize <= prod);

#if 0
  SC_GLOBAL_LDEBUGF ("Depth %d prod %d\n", depth, prod);
  SC_GLOBAL_LDEBUGF ("ntop %d nint %d nbot %d\n", ntop, nint, nbot);
#endif

  /* assign context data for recursion */
  nary->mpicomm = mpicomm;
  nary->mpisize = mpisize;
  nary->mpirank = mpirank;
  nary->ntop = ntop;
  nary->nint = nint;
  nary->nbot = nbot;
  nary->depth = depth;

  /* convert input variables into internal format */
  sc_notify_init_input (array, receivers, num_receivers, mpisize, mpirank);

  /* the recursive algorithm works in-place */
  sc_notify_recursive_nary (nary, 0, 0, prod, array);

  /* convert internal format to output variables */
  sc_notify_reset_output (array, senders, num_senders, mpisize, mpirank);
}
