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
#include <sc_notify.h>

/** Internally used function to execute the sc_notify recursion.
 * The internal data format of the input and output arrays is as follows:
 * forall(torank): (torank, howmanyfroms, listoffromranks).
 * \param [in] start        Offset of range of ranks.
 * \param [in] end          End (exclusive) of range of ranks.
 * \param [in] me           Current MPI process in range of ranks.
 * \param [in] length       Next-biggest or equal power of 2 for (end - start).
 * \param [in,out] input    Array of integers.  Overwritten for work space.
 * \param [in,out] output   Array of integers, must initially be empty.
 * \param [in] mpicomm      Communicator to use.
 */
static void
sc_notify_recursive (int start, int end, int me, int length,
                     sc_array_t * input, sc_array_t * output,
                     MPI_Comm mpicomm)
{
  int                 i, ir, j, jr, k;
  int                 mpiret;
  int                 num_ta, num_out;
  int                 length2, half;
  int                 torank, fromrank, numfroms;
  int                 peer;
  int                 count;
  int                *pint, *pout, *precv;
  sc_array_t         *temparr;
  sc_array_t         *sendbuf, *recvbuf;
  MPI_Request         outrequest;
  MPI_Status          instatus;

  SC_LDEBUGF ("Call recursion start %d end %d length %d\n",
              start, end, length);

  length2 = length / 2;
  SC_ASSERT (start <= me && me < end);
  SC_ASSERT (start % length == 0);
  SC_ASSERT (start < end && end <= start + length);
  SC_ASSERT (input->elem_size == sizeof (int));
  SC_ASSERT (output->elem_size == sizeof (int));
  SC_ASSERT (output->elem_count == 0);

  if (length > 1) {
    /* execute recursion */
    temparr = sc_array_new (sizeof (int));
    if (me < start + length2) {
      half = 0;
      sc_notify_recursive (start, start + length2, me, length2,
                           input, temparr, mpicomm);
    }
    else {
      half = 1;
      sc_notify_recursive (start + length2, end, me, length2,
                           input, temparr, mpicomm);
    }
    /* the input array is now invalid and all data is in temparr */

    SC_LDEBUGF ("Done recursion start %d end %d length %d\n", start, end,
                length);

    /* communicate results */
    peer = me ^ length2;
    SC_ASSERT (start <= peer && peer < start + length);
    if (peer < end) {
      SC_ASSERT ((!half && me < peer) || (half && me > peer));
      num_ta = (int) temparr->elem_count;
      sendbuf = sc_array_new (sizeof (int));

      /* fill send buffer */
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

      /* send message non-blocking */
      mpiret = MPI_Isend (sendbuf->array, (int) sendbuf->elem_count, MPI_INT,
                          peer, SC_TAG_NOTIFY_RECURSIVE, mpicomm,
                          &outrequest);
      SC_CHECK_MPI (mpiret);

      /* receive message */
      mpiret = MPI_Probe (peer, SC_TAG_NOTIFY_RECURSIVE, mpicomm, &instatus);
      SC_CHECK_MPI (mpiret);
      mpiret = MPI_Get_count (&instatus, MPI_INT, &count);
      SC_CHECK_MPI (mpiret);
      recvbuf = sc_array_new_size (sizeof (int), (size_t) count);
      mpiret = MPI_Recv (recvbuf->array, count, MPI_INT, peer,
                         SC_TAG_NOTIFY_RECURSIVE, mpicomm, MPI_STATUS_IGNORE);
      SC_CHECK_MPI (mpiret);

      /* merge the temparr and recvbuf arrays */
      i = ir = 0;
      torank = -1;
      for (;;) {
        pint = precv = NULL;
        while (i < (int) temparr->elem_count) {
          /* ignore data that was sent to the peer earlier */
          pint = (int *) sc_array_index_int (temparr, i);
          if (pint[0] == -1) {
            i += 2 + pint[1];
            SC_ASSERT (i <= (int) temparr->elem_count);
            pint = NULL;
          }
          else {
            break;
          }
        }
        if (ir < (int) recvbuf->elem_count) {
          precv = (int *) sc_array_index_int (recvbuf, ir);
        }
        if (pint == NULL && precv == NULL) {
          /* all data is processed and we are done */
          break;
        }
        else if (pint != NULL && precv == NULL) {
          /* copy temparr to output */
        }
        else if (pint == NULL && precv != NULL) {
          /* copy recvbuf to output */
        }
        else {
          /* both arrays have remaining elements and need to be compared */
          if (pint[0] < precv[0]) {
            /* copy temparr to output */
            precv = NULL;
          }
          else if (pint[0] > precv[0]) {
            /* copy recvbuf to output */
            pint = NULL;
          }
          else {
            /* both arrays have data for the same processor */
            SC_ASSERT (torank < pint[0] && pint[0] == precv[0]);
            torank = pint[0];
            SC_ASSERT (pint[1] > 0 && precv[1] > 0);
            SC_ASSERT (i + 2 + pint[1] <= (int) temparr->elem_count);
            SC_ASSERT (ir + 2 + precv[1] <= (int) recvbuf->elem_count);
            numfroms = pint[1] + precv[1];
            pout = (int *) sc_array_push_count (output, 2 + numfroms);
            pout[0] = torank;
            pout[1] = numfroms;
            k = 2;
            j = jr = 0;
            while (j < pint[1] || jr < precv[1]) {
              SC_ASSERT (j >= pint[1] || jr >= precv[1]
                         || pint[2 + j] != precv[2 + jr]);
              if (j < pint[1] &&
                  (jr >= precv[1] || pint[2 + j] < precv[2 + jr])) {
                pout[k++] = pint[2 + j++];
              }
              else {
                SC_ASSERT (jr < precv[1]);
                pout[k++] = precv[2 + jr++];
              }
            }
            SC_ASSERT (k == 2 + numfroms);
            i += 2 + pint[1];
            ir += 2 + precv[1];
            continue;
          }
        }

        SC_LDEBUGF ("Between with iir %d %d\n", i, ir);

        /* we need to copy exactly one buffer to the output array */
        if (precv == NULL) {
          SC_ASSERT (pint != NULL);
          SC_ASSERT (torank < pint[0]);
          torank = pint[0];
          SC_ASSERT (i + 2 + pint[1] <= (int) temparr->elem_count);
          numfroms = pint[1];
          SC_ASSERT (numfroms > 0);
          pout = (int *) sc_array_push_count (output, 2 + numfroms);
          memcpy (pout, pint, (2 + numfroms) * sizeof (int));
          i += 2 + numfroms;
        }
        else {
          SC_ASSERT (pint == NULL);
          SC_ASSERT (torank < precv[0]);
          torank = precv[0];
          SC_ASSERT (ir + 2 + precv[1] <= (int) recvbuf->elem_count);
          numfroms = precv[1];
          SC_ASSERT (numfroms > 0);
          pout = (int *) sc_array_push_count (output, 2 + numfroms);
          memcpy (pout, precv, (2 + numfroms) * sizeof (int));
          ir += 2 + numfroms;
        }
      }
      SC_ASSERT (i == (int) temparr->elem_count);
      SC_ASSERT (ir == (int) recvbuf->elem_count);
      sc_array_destroy (recvbuf);

      /* complete send call */
      mpiret = MPI_Wait (&outrequest, MPI_STATUS_IGNORE);
      SC_CHECK_MPI (mpiret);
      sc_array_destroy (sendbuf);
    }
    else {
      /* the length is not a power of two and there is no valid peer */
      SC_ASSERT (!half && me < peer);
      sc_array_copy (output, temparr);
    }
    sc_array_destroy (temparr);
  }
  else {
    /* end of recursion: copy input to output unchanged */
    sc_array_copy (output, input);
  }

  SC_LDEBUGF ("Into verify start %d end %d length %d\n", start, end, length);

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

  SC_LDEBUGF ("Done verify start %d end %d length %d\n", start, end, length);
}

int
sc_notify (int *receivers, int num_receivers,
           int *senders, int *num_senders, MPI_Comm mpicomm)
{
  int                 i;
  int                 mpiret;
  int                 mpisize, mpirank;
  int                 pow2length;
  int                 rec;
  int                 found_num_senders;
  int                *pint;
  sc_array_t          input, output;

  mpiret = MPI_Comm_size (mpicomm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = MPI_Comm_rank (mpicomm, &mpirank);
  SC_CHECK_MPI (mpiret);

  pow2length = SC_ROUNDUP2_32 (mpisize);
  SC_ASSERT (num_receivers >= 0);
  SC_ASSERT (senders != NULL && num_senders != NULL);
  SC_ASSERT (pow2length / 2 < mpisize && mpisize <= pow2length);

  sc_array_init (&input, sizeof (int));
  sc_array_resize (&input, 3 * num_receivers);
  sc_array_init (&output, sizeof (int));
  rec = -1;
  for (i = 0; i < num_receivers; ++i) {
    SC_ASSERT (rec < receivers[i]);
    rec = receivers[i];
    SC_ASSERT (rec < mpisize);
    pint = (int *) sc_array_index_int (&input, 3 * i);
    pint[0] = rec;
    pint[1] = 1;
    pint[2] = mpirank;
  }

  sc_notify_recursive (0, mpisize, mpirank, pow2length, &input, &output,
                       mpicomm);
  sc_array_reset (&input);

  found_num_senders = 0;
  if (output.elem_count > 0) {
    pint = (int *) sc_array_index_int (&output, 0);
    SC_ASSERT (pint[0] == mpirank);
    found_num_senders = pint[1];
    SC_ASSERT (found_num_senders > 0);
    SC_ASSERT (output.elem_count == 2 + (size_t) found_num_senders);
    for (i = 0; i < found_num_senders; ++i) {
      senders[i] = pint[2 + i];
    }
  }
  *num_senders = found_num_senders;
  sc_array_reset (&output);

  return MPI_SUCCESS;
}
