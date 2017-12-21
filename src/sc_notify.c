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

int                 sc_notify_nary_ntop = 4;
int                 sc_notify_nary_nint = 4;
int                 sc_notify_nary_nbot = 4;

static void
sc_array_init_invalid (sc_array_t * array)
{
  SC_ASSERT (array != NULL);

  array->elem_size = (size_t) ULONG_MAX;
  array->elem_count = (size_t) ULONG_MAX;
  array->byte_alloc = -1;
  array->array = NULL;
}

#ifdef SC_ENABLE_DEBUG

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
sc_notify_input (sc_array_t * input,
                 int *receivers, int num_receivers, int mpisize, int mpirank)
{
  int                 rec;
  int                 i;
  int                *pint;

  SC_ASSERT (num_receivers >= 0);
  SC_ASSERT (num_receivers == 0 || receivers != NULL);

  SC_ASSERT (sc_array_is_invalid (input));
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
 * \param [in] output       This function initializes the array.
 *                          All prior content will be overwritten and lost.
 *                          Empty array to hold int numbers.
 * \param [in] senders      See \ref sc_notify.
 * \param [in] num_senders  See \ref sc_notify.
 * \param [in] mpisize      Number of MPI processes.
 * \param [in] mpirank      MPI rank of this process.
 */
static void
sc_notify_output (sc_array_t * output,
                  int *senders, int *num_senders, int mpisize, int mpirank)
{
  int                 found_num_senders;
  int                 i;
  int                *pint;

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
 * \param [in,out] input    Array of integers.  Overwritten for work space.
 * \param [in,out] output   Array of integers, must initially be empty.
 */
static void
sc_notify_recursive (sc_MPI_Comm mpicomm, int start, int me, int length,
                     int groupsize, sc_array_t * input, sc_array_t * output)
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
  sc_array_t         *temparr, *morebuf;
  sc_array_t         *sendbuf, *recvbuf;
  sc_MPI_Request      outrequest;
  sc_MPI_Status       instatus;

  tag = SC_TAG_NOTIFY_RECURSIVE + SC_LOG2_32 (length);
  length2 = length / 2;
  SC_ASSERT (start <= me && me < start + length && me < groupsize);
  SC_ASSERT (start % length == 0);
  SC_ASSERT (input->elem_size == sizeof (int));
  SC_ASSERT (output->elem_size == sizeof (int));
  SC_ASSERT (output->elem_count == 0);

  if (length > 1) {
    /* execute recursion */
    temparr = sc_array_new (sizeof (int));
    if (me < start + length2) {
      half = 0;
      sc_notify_recursive (mpicomm, start, me, length2,
                           groupsize, input, temparr);
    }
    else {
      half = 1;
      sc_notify_recursive (mpicomm, start + length2, me, length2,
                           groupsize, input, temparr);
    }
    /* the input array is now invalid and all data is in temparr */

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
      num_ta = (int) temparr->elem_count;
      torank = -1;
      for (i = 0; i < num_ta;) {
        pint = (int *) sc_array_index_int (temparr, i);
        SC_ASSERT (torank < pint[0]);
        torank = pint[0];
        SC_ASSERT (torank % length == me % length ||
                   torank % length == peer % length);
        numfroms = pint[1];
        SC_ASSERT (numfroms > 0);
        if (torank % length != me % length) {
          /* this set needs to be sent and is marked invalid in temparr */
          pout = (int *) sc_array_push_count (sendbuf, 2 + numfroms);
          memcpy (pout, pint, (2 + numfroms) * sizeof (int));
          pint[0] = -1;
        }
        else {
          /* this set remains local and valid in temparr */
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
        /* merge the temparr and recvbuf arrays */
        morebuf = sc_array_new (sizeof (int));
        sc_notify_merge (morebuf, temparr, recvbuf);

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
        sc_notify_merge (output, morebuf, recvbuf);
        sc_array_destroy (morebuf);
      }
    }
    if (peer2 == -1) {
      sc_notify_merge (output, temparr, recvbuf);
    }
    sc_array_destroy (recvbuf);
    sc_array_destroy (temparr);

    if (peer >= 0) {
      /* complete send call */
      mpiret = sc_MPI_Wait (&outrequest, sc_MPI_STATUS_IGNORE);
      SC_CHECK_MPI (mpiret);
    }
    sc_array_destroy (sendbuf);
  }
  else {
    /* end of recursion: copy input to output unchanged */
    sc_array_copy (output, input);
  }

#ifdef SC_ENABLE_DEBUG
  /* verify recursion invariant */
  num_out = (int) output->elem_count;
  torank = -1;
  for (i = 0; i < num_out;) {
    pint = (int *) sc_array_index_int (output, i);
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
  sc_array_t          input, output;

  mpiret = sc_MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  pow2length = SC_ROUNDUP2_32 (mpisize);
  SC_ASSERT (num_receivers >= 0);
  SC_ASSERT (senders != NULL && num_senders != NULL);
  SC_ASSERT (pow2length / 2 < mpisize && mpisize <= pow2length);

  /* convert input variables into internal format */
  sc_array_init_invalid (&input);
  sc_notify_input (&input, receivers, num_receivers, mpisize, mpirank);

  /* execute the recursive algorithm */
  sc_array_init (&output, sizeof (int));
  sc_notify_recursive (mpicomm, 0, mpirank, pow2length,
                       mpisize, &input, &output);
  sc_array_reset (&input);

  /* convert internal format to output variables */
  sc_notify_output (&output, senders, num_senders, mpisize, mpirank);
  sc_array_reset (&output);

  return sc_MPI_SUCCESS;
}

int
sc_notify_nary_ext (int *receivers, int num_receivers,
                    int *senders, int *num_senders,
                    int ntop, int nint, int nbot, sc_MPI_Comm mpicomm)
{
  int                 size, rank;
  int                 mpiret;
  int                 depth;
  int                 prod, i;
  int                 gsize, grank, group;
  int                 q;

  SC_ASSERT (receivers != NULL);
  SC_ASSERT (senders != NULL);
  SC_ASSERT (num_senders != NULL);

  mpiret = sc_MPI_Comm_size (mpicomm, &size);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);

  SC_ASSERT (0 <= num_receivers && num_receivers <= size);
  SC_ASSERT (ntop >= 2 && nint >= 2 && nbot >= 2);

  /* determine depth of tree */
  if (size == 1) {
    /* depth = 0; */
    if (num_receivers > 0) {
      SC_ASSERT (num_receivers == 1);
      SC_ASSERT (receivers[0] == 0);
      senders[0] = 0;
    }
    *num_senders = num_receivers;

    /* we return when there is only one process */
    return MPI_SUCCESS;
  }
  if (size <= nbot) {
    depth = 1;
    prod = nbot;
  }
  else {
    depth = 2;
    for (prod = nbot * ntop; prod < size; prod *= nint) {
      ++depth;
    }
    SC_ASSERT (size <= prod);
    SC_ASSERT (prod == ntop * sc_intpow (nint, depth - 2) * nbot);
    SC_ASSERT (depth < 3 || size > ntop * sc_intpow (nint, depth - 3) * nbot);
  }

  /* send top round */
  SC_ASSERT (prod % ntop == 0);
  gsize = prod / ntop;
  group = rank / gsize;
  SC_ASSERT (0 <= group && group < ntop);
  grank = rank - group * gsize;
  SC_ASSERT (0 <= grank && grank < gsize);
  for (i = 0; i < ntop; ++i) {
    if (i == group) {
      /* don't send to my own group */
      continue;
    }
    q = rank + (i - group) * gsize;
    SC_ASSERT (0 <= q && q < prod);
    if (q >= size) {
      /* don't send to nonexisting receiver ranks */ ;
      continue;
    }

    /* pack data into message and send it to q */
  }

  /* loop over probe and receive messages; send again when ready */

  /* done */
  SC_ABORT ("Function sc_notify_nary_ext not yet implemented");
}

int
sc_notify_nary (int *receivers, int num_receivers,
                int *senders, int *num_senders, sc_MPI_Comm mpicomm)
{
  return sc_notify_nary_ext (receivers, num_receivers,
                             senders, num_senders,
                             sc_notify_nary_ntop,
                             sc_notify_nary_nint,
                             sc_notify_nary_nbot, mpicomm);
}
