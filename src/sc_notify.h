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

/** \file sc_notify.h
 * We provide various algorithms to invert the communication pattern.
 *
 * The task at hand is: given an individual set of receiver ranks for each
 * process in a sender role, compute efficiently for the receiver role of
 * each rank the set of senders to it.
 *
 * This file implements two approaches that solve the problem scalably.
 *
 * 1. The first and original historic one offers the choice between a binary
 *    tree recursion, an n-ary tree recursion, optionally with payload data,
 *    and an allgather version.  The n-ary tree version contains the binary
 *    one as a special case.  It is called \ref sc_notify_nary and can be
 *    configured by the global variables
 *
 *     * \ref sc_notify_nary_ntop_default; \#children of the root tree node,
 *     * \ref sc_notify_nary_nint_default; \#children of intermediate nodes,
 *     * \ref sc_notify_nary_nbot_default; \#siblings of each leaf node.
 *
 *    Thus it can be tailored to match for example distributed NUMA architectures.
 *    The default for each variable is a conservative 2 to choose a binary tree.
 *
 *    Another addition to the family is \ref sc_notify_ext for the best
 *    known default, which is currently the PEX algorithm.
 *
 * 2. The more recent and general variant allows to choose between practically
 *    all currently known algorithms, enumerated by \ref sc_notify_type_t.
 *    This is managed by first creating a configuration object \ref sc_notify_t,
 *    setting its type and further parameters, and passing it into the general
 *    \ref sc_notify_payload or \ref sc_notify_payloadv functions.
 *
 * \ingroup sc_parallelism
 */

#ifndef SC_NOTIFY_H
#define SC_NOTIFY_H

#include <sc_statistics.h>

SC_EXTERN_C_BEGIN;

/** Default number of children at root node of nary tree; initialized to 2 */
extern int          sc_notify_nary_ntop_default;

/** Default number of children at intermediate tree nodes; initialized to 2 */
extern int          sc_notify_nary_nint_default;

/** Default number of children at deepest level of tree; initialized to 2 */
extern int          sc_notify_nary_nbot_default;

/** @{ \name Classic, simple interface. */

/** Collective call to notify a set of receiver ranks of current rank.
 * This version uses one call to sc_MPI_Allgather and one to sc_MPI_Allgatherv.
 * We provide hand-coded alternatives \ref sc_notify and \ref sc_notify_nary.
 * \param [in] receivers        Sorted and unique array of MPI ranks to inform.
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

/** The n-ary tree implementation of the notify functionality.
 * This function optionally transfers a fixed-size payload per rank.
 * \param [in,out] receivers    Sorted, unique array of int members.
 *                              If \a senders is NULL, repurposed as output.
 * \param [in,out] senders      Aarray of int members, resized on output.
 *                              If NULL, the output is placed in \a receivers.
 * \param [in,out] in_payload   Array of same length as \a receivers with
 *                              arbitrary data to be transmitted to senders.
 *                              If \a out_payload is NULL, repurposed as output.
 *                              Both payload arrays may be NULL altogether.
 *                              If \a out_payload is not NULL, \a in_payload
 *                              must not be NULL either.
 * \param [in,out] out_payload  Of same type as \a in_payload, resized on output.
 *                              If NULL, the result is placed in \a in_payload.
 * \param [in] mpicomm          MPI communicator to use.
 * \return                      Aborts on MPI error or returns sc_MPI_SUCCESS.
 */
void                sc_notify_nary (sc_array_t * receivers,
                                    sc_array_t * senders,
                                    sc_array_t * in_payload,
                                    sc_array_t * out_payload,
                                    sc_MPI_Comm mpicomm);

/** The default implementation of the notify functionality, currently PEX.
 * This function optionally transfers a fixed-size payload per rank.
 * \param [in,out] receivers    Sorted, unique array of int members.
 *                              If \a senders is NULL, repurposed as output.
 * \param [in,out] senders      Aarray of int members, resized on output.
 *                              If NULL, the output is placed in \a receivers.
 * \param [in,out] in_payload   Array of same length as \a receivers with
 *                              arbitrary data to be transmitted to senders.
 *                              If \a out_payload is NULL, repurposed as output.
 *                              Both payload arrays may be NULL altogether.
 * \param [in,out] out_payload  Of same type as \a in_payload, resized on output.
 *                              If NULL, the result is placed in \a in_payload.
 * \param [in] mpicomm          MPI communicator to use.
 * \return                      Aborts on MPI error or returns sc_MPI_SUCCESS.
 */
void                sc_notify_ext (sc_array_t * receivers,
                                   sc_array_t * senders,
                                   sc_array_t * in_payload,
                                   sc_array_t * out_payload,
                                   sc_MPI_Comm mpicomm);

/** @} */

/** Opaque object used for controlling notification
 * (AKA dynamic sparse data exchange) operations.
 */
typedef struct sc_notify_s sc_notify_t;

/** Type of callback function for the \ref SC_NOTIFY_SUPERSET variant. */
typedef void        (*sc_compute_superset_t) (sc_array_t *, sc_array_t *,
                                              sc_array_t *, sc_notify_t *,
                                              void *);

/** Various equivalent implementations of the functionality. */
typedef enum
{
  SC_NOTIFY_DEFAULT = -1,  /**< Choose whatever type is stored in sc_notify_type_default. */
  SC_NOTIFY_ALLGATHER = 0, /**< Choose allgather algorithm.  Likely suboptimal. */
  SC_NOTIFY_BINARY,        /**< Choose simple binary recursion. */
  SC_NOTIFY_NARY,          /**< Choose nary (k-way) recursion. */
  SC_NOTIFY_PEX,           /**< Choose alltoall algorithm (AKA personalized exchange).
                                Current best choice. */
  SC_NOTIFY_PCX,           /**< Choose reduce_scatter algorithm (AKA personalized census). */
  SC_NOTIFY_RSX,           /**< Choose remote summation algorithm. */
  SC_NOTIFY_NBX,           /**< Choose non-blocking consensus algorithm. */
  SC_NOTIFY_RANGES,        /**< Use the sc_ranges functionality.  Likely suboptimal. */
  SC_NOTIFY_SUPERSET,      /**< Use a computable superset of communicators, computed by
                                a callback function. */
  SC_NOTIFY_NUM_TYPES      /**< End of list marker for notify algorithms. */
}
sc_notify_type_t;

#define SC_NOTIFY_STR_ALLGATHER "allgather" /**< String for the allgather variant. */
#define SC_NOTIFY_STR_BINARY "binary"       /**< String for the binary tree variant. */
#define SC_NOTIFY_STR_NARY "nary"           /**< String for the n-ary tree variant. */
#define SC_NOTIFY_STR_PEX "pex"             /**< String for the PEX variant. */
#define SC_NOTIFY_STR_PCX "pcx"             /**< String for the PCX variant. */
#define SC_NOTIFY_STR_RSX "rsx"             /**< String for the RSX variant. */
#define SC_NOTIFY_STR_NBX "nbx"             /**< String for the NBX variant. */
#define SC_NOTIFY_STR_RANGES "ranges"       /**< String for the ranges variant. */
#define SC_NOTIFY_STR_SUPERSET "superset"   /**< String for the superset variant. */

/** Names for each notify method */
extern const char  *sc_notify_type_strings[SC_NOTIFY_NUM_TYPES];

/** The default type used when constructing a notify controller;
 * initialized to \ref SC_NOTIFY_PEX. */
extern sc_notify_type_t sc_notify_type_default;

/** The default threshold for payload sizes (in bytes) that are communicated
 * with the notification packet.  Initialized to 1024 (2^10) */
extern size_t       sc_notify_eager_threshold_default;

/** @{ \name Optional, most general interface. */

/** Create a notify controller that can be used in \ref sc_notify_payload
 * and \ref sc_notify_payloadv.
 *
 * \param[in] mpicomm     The MPI communicator over which the notification occurs.
 * \return                Pointer to a notify controller that should be
 *                        destroyed with \ref sc_notify_destroy.
 */
sc_notify_t        *sc_notify_new (sc_MPI_Comm mpicomm);

/** Destroy a notify controller constructed with \ref sc_notify_new.
 *
 * \param[in,out] notify  The notify controller that will be
 *                        destroyed.  Pointer is invalid on completion.
 */
void                sc_notify_destroy (sc_notify_t * notify);

/** Get the payload size above which payloads are no longer transferred with
 * notification packets in \ref sc_notify_payload.
 *
 * \param[in] notify      The notify controller.
 * \return                The size in bytes of the maximum eager payload size.
 */
size_t              sc_notify_get_eager_threshold (sc_notify_t * notify);

/** Get the payload size above which payloads are no longer transferred with
 * notification packets in \ref sc_notify_payload.
 *
 * \param[in,out] notify      The notify controller.
 * \param[in]     thresh      The size in bytes of the maximum eager payload
 *                            size.
 */
void                sc_notify_set_eager_threshold (sc_notify_t * notify,
                                                   size_t thresh);

/** Get the \ref sc_statistics_t object for logging runtimes (added by
 * function name).
 *
 * \param[in,out] notify      The notify controller.
 * \return                    The sc_statistics_t * object, may be NULL.
 */
sc_statistics_t    *sc_notify_get_stats (sc_notify_t * notify);

/** Set a \ref sc_statistics_t object for logging runtimes (added by
 * function name).
 *
 * \param[in,out] notify      The notify controller.
 * \param[in]     stats       The \ref sc_statistics_t object.  The notify
 *                            controller will add timings for functions
 *                            to the object, listed under their function
 *                            names.
 */
void                sc_notify_set_stats (sc_notify_t * notify,
                                         sc_statistics_t * stats);

/** Get the MPI communicator of a notify controller.
 *
 * \param[in] notify   The notify controller.
 * \return             The mpi communicator over which the notification
 *                     occurs.
 */
sc_MPI_Comm         sc_notify_get_comm (sc_notify_t * notify);

/** Query whether \ref sc_notify_set_type supports a given type.
 * \param [in] type        Notify algorithm type from \ref sc_notify_type_t.
 * \return                 True if supported, false if not.
 */
int                 sc_notify_supports_type (sc_notify_type_t type);

/** Get the type of a notify controller.
 *
 * \param[in] notify   The notify controller.
 * \return             The type of the notify controller (see \a sc_notify_type_t).
 */
sc_notify_type_t    sc_notify_get_type (sc_notify_t * notify);

/** Set the type of a notify controller.
 *
 * \param[in,out] notify   The notify controller.
 * \param[in]     type     The type of algorithm used to affect the dynamic
 *                         sparse data exchange.
 *
 * \return                 false if the type is supported by the current MPI
 *                         version, true otherwise.
 */
int                 sc_notify_set_type (sc_notify_t * notify,
                                        sc_notify_type_t type);

/** For a notify of type \ref SC_NOTIFY_NARY, get the branching widths of the
 * recursive algorithm.
 *
 * \param[in] notify  The notify controller must be of type \ref SC_NOTIFY_NARY.
 * \param[out] ntop   The number of children at root node of nary tree.
 * \param[out] nint   The number of children at intermediate tree nodes.
 * \param[out] nbot   The number of children at deepest level of the tree.
 */
void                sc_notify_nary_get_widths (sc_notify_t * notify,
                                               int *ntop, int *nint,
                                               int *nbot);

/** For a notify of type \ref SC_NOTIFY_NARY, set the branching widths of the
 * recursive algorithm.
 *
 * \param[in,out] notify  The notify controller must be of type \ref SC_NOTIFY_NARY.
 * \param[in] ntop   The number of children at root node of nary tree.
 * \param[in] nint   The number of children at intermediate tree nodes.
 * \param[in] nbot   The number of children at deepest level of the tree.
 */
void                sc_notify_nary_set_widths (sc_notify_t * notify, int ntop,
                                               int nint, int nbot);

/** Query the number of ranges for the \ref SC_NOTIFY_RANGES method.
 * \param [in] notify       Must be of type \ref SC_NOTIFY_RANGES.
 * \return                  Number of ranges.
 */
int                 sc_notify_ranges_get_num_ranges (sc_notify_t * notify);

/** Set the number of ranges for the \ref SC_NOTIFY_RANGES method.
 * \param [in,out] notify   Must be of type \ref SC_NOTIFY_RANGES.
 * \param [in] num_ranges   Number of ranges.
 */
void                sc_notify_ranges_set_num_ranges (sc_notify_t * notify,
                                                     int num_ranges);

/** \cond NOTIFY_DOCUMENT_RANGES */
/** Query the package ID for the \ref SC_NOTIFY_RANGES method.
 * \param [in] notify       Must be of type \ref SC_NOTIFY_RANGES.
 * \return                  The internal package ID.
 */
int                 sc_notify_ranges_get_package_id (sc_notify_t * notify);

/** Set the package ID for the \ref SC_NOTIFY_RANGES method.
 * \param [in,out] notify   Must be of type \ref SC_NOTIFY_RANGES.
 * \param [in] package_id   The internal package ID.
 */
void                sc_notify_ranges_set_package_id (sc_notify_t * notify,
                                                     int package_id);
const int          *sc_notify_ranges_get_procs (sc_notify_t * notify,
                                                int *num_procs);
void                sc_notify_ranges_set_procs (sc_notify_t * notify,
                                                int num_procs,
                                                const int *procs);
void                sc_notify_ranges_get_peer_range (sc_notify_t * notify,
                                                     int *first_peer,
                                                     int *last_peer);
void                sc_notify_ranges_set_peer_range (sc_notify_t * notify,
                                                     int first_peer,
                                                     int last_peer);

/** \endcond */

/** Query the callback for the \ref SC_NOTIFY_SUPERSET method.
 * \param [in] notify           Must be of type \ref SC_NOTIFY_SUPERSET.
 * \param [out] compute_superset    Output the callback function.
 * \param [out] ctx                 Output the callback Context.
 */
void                sc_notify_superset_get_callback
  (sc_notify_t * notify, sc_compute_superset_t * compute_superset, void *ctx);

/** Set the callback for the \ref SC_NOTIFY_SUPERSET method.
 * \param [in,out] notify       Must be of type \ref SC_NOTIFY_SUPERSET.
 * \param [in] compute_superset Callback function.
 * \param [in] ctx              Context passed to callback function.
 */
void                sc_notify_superset_set_callback
  (sc_notify_t * notify, sc_compute_superset_t compute_superset, void *ctx);

/** Collective call to notify a set of receiver ranks of current rank.
 * This function aborts on MPI error.
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
 * \param [in,out] in_payload   This array pointer may be NULL.
 *                              If not NULL, it must have \b num_receivers
 *                              entries that are the same size on every
 *                              process.  If not NULL and \b out_payload is
 *                              NULL, it must not be a view, and it will
 *                              be resized to contain the output and have
 *                              \b *num_senders entries.
 * \param [in,out] out_payload  This array pointer may be NULL.
 *                              If not, it must not be a view, and
 *                              on output will have \b * num_senders entries.
 * \param [in] sorted           whether \b receivers and \b senders
 *                              are required to be sorted by MPI rank.
 * \param [in] notify           Notify controller to use.
 */
void                sc_notify_payload (sc_array_t * receivers,
                                       sc_array_t * senders,
                                       sc_array_t * in_payload,
                                       sc_array_t * out_payload,
                                       int sorted, sc_notify_t * notify);

/** Collective call to notify a set of receiver ranks of current rank
 * and send a variable size message to the receiver.
 * This function aborts on MPI error.
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
 * \param [in,out] in_payload   This array pointer may be NULL.
 *                              If not, it must not be a view and have
 *                              entries that are the same size on every process.
 *                              This data will be communicated to the receivers.
 *                              If \b out_payload is NULL, this will be
 *                              resized to contain the received payload.
 * \param [in,out] out_payload  This array pointer may be NULL.
 *                              If not, it must not be a view and have
 *                              entries that are the same size on every process.
 *                              If not NULL, this array will be resized to
 *                              contain the received payload.
 * \param [in,out] in_offsets   If \b in_payload is not NULL,
 *                              this is an array of int's of size
 *                              \b num_receivers + 1,
 *                              giving offsets for the portion of the
 *                              payload to be sent to every receiver.
 *                              If \b out_offsets is NULL,
 *                              it will be resized to give
 *                              offsets of the received payload.
 * \param [in,out] out_offsets  This array pointer may be NULL.
 *                              If \b in_payload is not NULL,
 *                              it is an array of int's that will be resized
 *                              to \b num_senders + 1,
 *                              giving offsets for the portion of the
 *                              payload received from each sender.
 * \param [in] sorted           whether \b receivers and \b senders
 *                              (and thus \b in_offsets and \b out_offsets)
 *                              are required to be sorted by MPI rank.
 * \param [in] notify           Notify controller to use.
 */
void                sc_notify_payloadv (sc_array_t * receivers,
                                        sc_array_t * senders,
                                        sc_array_t * out_payload,
                                        sc_array_t * in_payload,
                                        sc_array_t * out_offsets,
                                        sc_array_t * in_offsets,
                                        int sorted, sc_notify_t * notify);

/** @} */

/** For the \ref SC_NOTIFY_RANGES method, the default is 25. */
extern int          sc_notify_ranges_num_ranges_default;

SC_EXTERN_C_END;

#endif /* !SC_NOTIFY_H */
