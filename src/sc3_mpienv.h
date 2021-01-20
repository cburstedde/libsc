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

/** \file sc3_mpienv.h \ingroup sc3
 * Create subcommunicators on shared memory nodes.
 *
 * This MPI environment structure is initialized from a main communicator,
 * usually \ref SC3_MPI_COMM_WORLD.  We derive one communicator for each shared
 * memory node by splitting the input communicator.
 * We share the node communicator sizes of all nodes and their offsets with
 * respect to the main communicator in an MPI 3 shared window.
 * In addition, we identify the first rank of each node communicator and create
 * one communicator over all of these first ranks, called the head communicator.
 *
 * When MPI shared windows are not supported or not enabled, we understand each
 * rank to be its own node, and the head communicator equals the main one.
 * The windows we allocate then use a non-MPI replacement, which can be faster.
 */

#ifndef SC3_MPI_ENV_H
#define SC3_MPI_ENV_H

#include <sc3_mpi.h>

/** The mpi environment is an opaque struct. */
typedef struct sc3_mpienv sc3_mpienv_t;

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Query whether an mpi environment is not NULL and internally consistent.
 * The mpi environment may be valid in both its setup and usage phases.
 * \param [in] m        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer is not NULL and mpi environment consistent.
 */
int                 sc3_mpienv_is_valid (const sc3_mpienv_t * m,
                                         char *reason);

/** Query whether an mpi environment is not NULL, consistent and not setup.
 * This means that the mpi environment is not (yet) in its usage phase.
 * \param [in] m        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, consistent, not setup.
 */
int                 sc3_mpienv_is_new (const sc3_mpienv_t * m, char *reason);

/** Query whether an mpi environment is not NULL, internally consistent and setup.
 * This means that the mpi environment is in its usage phase.
 * \param [in] m        Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True iff pointer not NULL, consistent and setup.
 */
int                 sc3_mpienv_is_setup (const sc3_mpienv_t * m,
                                         char *reason);

/** Create a new mpi environment in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_mpienv_setup to change the environment into its usage phase.
 * After that, no more parameters may be set.
 * The defaults are documented in the sc3_mpienv_set_* calls.
 * \param [in,out] mator    An allocator that is setup.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on environment destruction.
 * \param [out] mp      Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_new (sc3_allocator_t * mator,
                                    sc3_mpienv_t ** mp);

/** Provide an MPI communicator to use.
 * The default after \ref sc3_mpienv_new is \c SC3_MPI_COMM_WORLD.
 * \param [in,out] m        The mpi environment must not have been setup.
 * \param [in] comm         This communicator replaces any previous one.
 *                          If it is dupd, we also set it to return errors.
 * \param [in] dup          If true, the input communicator is dupd
 *                          and set to return errors.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_set_comm (sc3_mpienv_t * m,
                                         sc3_MPI_Comm_t comm, int dup);

/** Specify whether we split the communicator by node.
 * This allows the use of shared memory by the MPI window functions.
 * The default is false iff MPI windows are not configured or not supported.
 * If specifying true here and it turns out MPI windows are not supported,
 * we will silently turn them off.  Check with \ref sc3_mpienv_get_shared.
 * \param [in,out] m        The mpi environment must not yet be setup.
 * \param [in] shared       Boolean to enable sharing.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_set_shared (sc3_mpienv_t * m, int shared);

/** Specify whether the shared memory windows allocated shall be contigouus.
 * The default is false since this may be faster.
 * \param [in,out] m        The mpi environment must not yet be setup.
 * \param [in] contiguous   Boolean to enable contiguous window allocation.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_set_contiguous (sc3_mpienv_t * m,
                                               int contiguous);

/** Setup an mpi environment and change it into its usable phase.
 * \param [in,out] m    This mpi environment must not yet be setup.
 *                      Internal storage is allocated, the setup phase ends,
 *                      and the mpi environment is put into its usable phase.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_setup (sc3_mpienv_t * m);

/** Increase the reference count on an mpi environment by one.
 * This is only allowed after the mpi environment has been setup.
 * \param [in,out] m    Setup environment.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_ref (sc3_mpienv_t * m);

/** Decrease the reference count on an mpi environment by one.
 * If the reference count drops to zero, the mpi environment is deallocated.
 * \param [in,out] mp   The pointer must not be NULL and the mpi environment valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the mpi environment is freed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_unref (sc3_mpienv_t ** mp);

/** Destroy an mpi environment with a reference count of one.
 * It is a leak error to destroy an mpi environment that is multiply referenced.
 * We unref its internal allocator, which may cause a leak error if that
 * allocator has been used against specification elsewhere in the code.
 * \param [in,out] mp   Mpi environment must be valid and have a refcount of one.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 *                      When the mpi environment had more than one reference,
 *                      return an error of kind \ref SC3_ERROR_LEAK.
 */
sc3_error_t        *sc3_mpienv_destroy (sc3_mpienv_t ** mp);

/** Query whether the environment supports MPI shared windows.
 * \param [in] m        Valid and setup mpi environment.
 * \param [out] shared  Pointer must not be NULL.  Output shared status.
 *                      If the node communicator has size one, and sharing has
 *                      is enabled and supported, we output true but use
 *                      a faster non-MPI replacement for allocating windows.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_mpienv_get_shared (sc3_mpienv_t * m, int *shared);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_MPI_ENV_H */
