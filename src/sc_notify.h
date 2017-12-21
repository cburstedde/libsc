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

#include <sc.h>

SC_EXTERN_C_BEGIN;

/** Number of children at root node of nary tree; default is 2 */
extern int          sc_notify_nary_ntop;

/** Number of children at intermediate tree nodes; default is 2 */
extern int          sc_notify_nary_nint;

/** Number of children at deepest level of tree; default is 2 */
extern int          sc_notify_nary_nbot;

/** Collective call to notify a set of receiver ranks of current rank.
 * This version uses one call to sc_MPI_Allgather and one to sc_MPI_Allgatherv.
 * \see sc_notify
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
 * This function allows to configure the mode of operation in detail.
 * \param [in] receivers        Sorted and unique array of MPI ranks to inform.
 * \param [in] num_receivers    Count of ranks contained in receivers.
 * \param [in,out] senders      Array of at least size sc_MPI_Comm_size.
 *                              On output it contains the notifying ranks,
 *                              whose number is returned in \b num_senders.
 * \param [out] num_senders     On output the number of notifying ranks.
 * \param [in] ntop             Number of children of the root node.
 *                              Only used if \b nbot leads to depth >= 2.
 * \param [in] nint             Number of children of intermediate tree node.
 *                              Only used if \b ntop and \b nbot lead to
 *                              depth >= 3.
 * \param [in] nbot             Number of children at the deepest level.
 *                              Used first in determining depth of tree.
 * \param [in] mpicomm          MPI communicator to use.
 * \return                      Aborts on MPI error or returns sc_MPI_SUCCESS.
 */
int                 sc_notify_nary_ext (int *receivers, int num_receivers,
                                        int *senders, int *num_senders,
                                        int ntop, int nint, int nbot,
                                        sc_MPI_Comm mpicomm);

/** Collective call to notify a set of receiver ranks of current rank.
 * This implementation uses an n-ary tree for reduced latency.
 * It chooses selected parameters for \ref sc_notify_nary_ext, namely
 * \ref sc_notify_nary_ntop, \ref sc_notify_nary_nint, \ref sc_notify_nary_nbot.
 * These may be overridden by the user.
 * This function serves as drop-in replacement for \ref sc_notify.
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

SC_EXTERN_C_END;

#endif /* !SC_NOTIFY_H */