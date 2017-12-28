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

#ifndef SC_NOTIFY_H
#define SC_NOTIFY_H

#include <sc_containers.h>

SC_EXTERN_C_BEGIN;

/** Number of children at root node of nary tree; initialized to 2 */
extern int          sc_notify_nary_ntop;

/** Number of children at intermediate tree nodes; initialized to 2 */
extern int          sc_notify_nary_nint;

/** Number of children at deepest level of tree; initialized to 2 */
extern int          sc_notify_nary_nbot;

/** Collective call to notify a set of receiver ranks of current rank.
 * This version uses one call to sc_MPI_Allgather and one to sc_MPI_Allgatherv.
 * We provide hand-coded alternatives \ref sc_notify and \ref sc_notify_nary.
 * \param [in] receivers        Array of MPI ranks to inform.
 * \param [in] num_receivers    Count of ranks contained in receivers.
 * \param [in,out] senders      Array of at least size sc_MPI_Comm_size.
 *                              On output it contains the notifying ranks.
 * \param [out] num_senders     On output the number of notifying ranks.
 * \param [in] mpicomm          MPI communicator to use.
 * \return                      Aborts on MPI error or returns sc_MPI_SUCCESS.
 */
int                 sc_notify_allgather (int *receivers, int num_receivers,
                                         int *senders, int *num_senders,
                                         sc_MPI_Comm mpicomm);

/** Collective call to notify a set of receiver ranks of current rank.
 * This implementation uses an n-ary tree for reduced latency.
 * It chooses external global parameters to call \ref sc_notify_ext, namely
 * \ref sc_notify_nary_ntop, \ref sc_notify_nary_nint, \ref
 * sc_notify_nary_nbot.  These may be overridden by the user.
 * This function serves as backwards-compatible replacement for \ref sc_notify.
 * \param [in] receivers        Sorted and unique array of MPI ranks to inform.
 * \param [in] num_receivers    Count of ranks contained in receivers.
 * \param [in,out] senders      Array of at least size sc_MPI_Comm_size.
 *                              On output it contains the notifying ranks,
 *                              whose number is returned in \b num_senders.
 * \param [out] num_senders     On output the number of notifying ranks.
 * \param [in] mpicomm          MPI communicator to use.
 * \return                      Aborts on MPI error or returns sc_MPI_SUCCESS.
 */
int                 sc_notify_nary (int *receivers, int num_receivers,
                                    int *senders, int *num_senders,
                                    sc_MPI_Comm mpicomm);

/** Collective call to notify a set of receiver ranks of current rank.
 * More generally, this function serves to transpose the nonzero pattern of a
 * matrix, where each row and column corresponds to an MPI rank in order.
 * We use a binary tree to construct the communication pattern.
 * This minimizes the number of messages at the cost of more messages
 * compared to a fatter tree such as configurable with \ref sc_notify_nary.
 * \param [in] receivers        Sorted and unique array of MPI ranks to inform.
 * \param [in] num_receivers    Count of ranks contained in receivers.
 * \param [in,out] senders      Array of at least size sc_MPI_Comm_size.
 *                              On output it contains the notifying ranks,
 *                              whose number is returned in \b num_senders.
 * \param [out] num_senders     On output the number of notifying ranks.
 * \param [in] mpicomm          MPI communicator to use.
 * \return                      Aborts on MPI error or returns sc_MPI_SUCCESS.
 */
int                 sc_notify (int *receivers, int num_receivers,
                               int *senders, int *num_senders,
                               sc_MPI_Comm mpicomm);

/** Collective call to notify a set of receiver ranks of current rank.
 * This implementation uses a configurable n-ary tree for reduced latency.
 * This function allows to configure the mode of operation in detail.
 * It is possible to use
 * \param [in,out] receivers    On input, sorted and uniqued array of type int.
 *                              Contains the MPI ranks to inform.
 *                              If \b senders is not NULL, treated read-only.
 *                              If \b senders is NULL, takes its role on output.
 *                              In this case it must not be a view.
 * \param [in,out] senders      If NULL, the result is placed in \b receivers
 *                              as written below for the non-NULL case.
 *                              If not NULL, array of type int and any length.
 *                              Its entries on input are ignored and overwritten.
 *                              On output it is resized to the number of
 *                              notifying ranks, which it contains in order.
 *                              Thus, it must not be a view.
 * \param [in,out] payload      This array pointer may be NULL.
 *                              If not, it must not be a view and have
 *                              \b num_receivers entries of sizeof (int) on input.
 *                              This data will be communicated to the receivers.
 *                              On output, the array is resized to \b
 *                              *num_senders and contains the result.
 * \param [in] ntop             Number of children of the root node.
 *                              Only used if \b nbot leads to depth >= 2.
 * \param [in] nint             Number of children of intermediate tree node.
 *                              Only used if \b ntop and \b nbot lead to
 *                              depth >= 3.
 * \param [in] nbot             Number of children at the deepest level.
 *                              Used first in determining depth of tree.
 * \param [in] mpicomm          MPI communicator to use.
 */
void                sc_notify_ext (sc_array_t * receivers,
                                   sc_array_t * senders, sc_array_t * payload,
                                   int ntop, int nint, int nbot,
                                   sc_MPI_Comm mpicomm);

SC_EXTERN_C_END;

#endif /* !SC_NOTIFY_H */
