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

/** \file sc3_mpi.h \ingroup sc3
 * We provide MPI wrappers for configuring both with or without MPI.
 *
 * If we configure --enable-mpi, the wrappers call the original MPI
 * functions and translate their return values into \ref sc3_error_t
 * pointers.
 * If the error is due to an MPI function, we create a return error of
 * kind \ref SC3_ERROR_NETWORK.
 * This kind is considered fatal in our standard error macros.
 * The application is free to catch such errors and handle und unref them.
 * On failed assertions and pre/post-conditions, we return some fatal kind.
 * Without --enable-mpi, the wrappers present communicators of size one that
 * are suitable for size and rank queries and most collective communication.
 * The advantage is that the code needs only few #`ifdef SC_ENABLE_MPI`.
 *
 * MPI 3 shared window creation is thinly wrapped if it exists.
 * Otherwise the wrappers present a shared communicator of size one.
 *
 * Presently, we do not wrap point-to-point messages.
 */

#ifndef SC3_MPI_H
#define SC3_MPI_H

#include <sc3_error.h>
#include <sc3_mpi_types.h>

/** Execute an MPI call and translate its return into an \ref sc3_error_t.
 *
 * This macro is primarily intended for use inside our wrappers.
 * The MPI function \a f must exist, thus we call this inside
 * an #`ifdef SC_ENABLE_MPI` macro.
 *
 * If the MPI function returns MPI_SUCCESS, this macro does nothing.
 * Otherwise, we query the MPI error string and return a proper sc3 error.
 */
#define SC3E_MPI(f) do {                                                \
  int _mpiret = (f);                                                    \
  if (_mpiret != SC3_MPI_SUCCESS) {                                     \
    int _errlen;                                                        \
    char _errstr[SC3_MPI_MAX_ERROR_STRING];                             \
    char _errmsg[SC3_BUFSIZE];                                          \
    sc3_MPI_Error_string (_mpiret, _errstr, &_errlen);                  \
    sc3_snprintf (_errmsg, SC3_BUFSIZE, "%s: %s", #f, _errstr);         \
    return sc3_error_new_kind (SC3_ERROR_NETWORK,                       \
                               __FILE__, __LINE__, _errmsg);            \
  }} while (0)

/** Return an MPI usage error.
 *
 * If a wrapped MPI function is called inappropriately, use this to return.
 * For example, if MPI shared windows are not available and the user requests
 * a non-trivial one (with communicator size greater one), we invoke this macro.
 */
#define SC3E_MPI_USAGE(s) do {                                          \
  char _errmsg[SC3_BUFSIZE];                                            \
  sc3_snprintf (_errmsg, SC3_BUFSIZE, "MPI usage: %s", s);              \
  return sc3_error_new_kind (SC3_ERROR_NETWORK,                         \
                             __FILE__, __LINE__, _errmsg);              \
} while (0)

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Wrap MPI_Error_class.
 * This function is always successful.
 * \param [in] errorcode    An error code returned by an MPI function.
 * \param [out] errorclass  The error class of the code.
 *                          The class is out of a smaller set of error codes.
 */
void                sc3_MPI_Error_class (int errorcode, int *errorclass);

/** Wrap MPI_Error_string.
 * This function is always successful.
 * \param [in] errorcode    An error code returned by an MPI function.
 * \param [out] errstr      Buffer size at least SC3_MPI_MAX_ERROR_STRING.
 *                          If NULL, we do nothing.
 *                          Otherwise, we prepend "MPI" and remove
 *                          newlines from the original MPI error string.
 * \param [out] errlen      If NULL, we set \a errstr to "".
 *                          Otherwise, the length of \a errstr without
 *                          the trailing null byte.
 */
void                sc3_MPI_Error_string (int errorcode,
                                          char *errstr, int *errlen);

/** Wrap MPI_Init.
 * Without --enable-mpi, this function noops.
 * \param [in] argc     Pointer to number of command line arguments.
 * \param [in] argv     Pointer to command line argument vector.
 * \return          NULL on success, \ref SC3_ERROR_NETWORK error otherwise.
 */
sc3_error_t        *sc3_MPI_Init (int *argc, char ***argv);

/** Wrap MPI_Finalize.
 * Without --enable-mpi, this function noops.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Finalize (void);

/** Wrap MPI_Abort.
 * Without --enable-mpi, call abort (3).
 * \param [in] comm         Valid MPI communicator.
 * \param [in] errorcode    Well-defined MPI error code.
 * \return                  NULL on success, error object otherwise.
 *                          This function may not return though.
 */
sc3_error_t        *sc3_MPI_Abort (sc3_MPI_Comm_t comm, int errorcode);

/** Wrap MPI_Wtime.
 * \return          Time in seconds since an arbitrary time in the past.
 */
double              sc3_MPI_Wtime (void);

/** Wrap MPI_Comm_set_errhandler.
 * \param [in] comm     Valid MPI communicator.
 * \param [in] errh     Valid MPI errror handler object.
 *                      May use \ref SC3_MPI_ERRORS_RETURN.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_set_errhandler (sc3_MPI_Comm_t comm,
                                                 sc3_MPI_Errhandler_t errh);

/** Wrap MPI_Comm_size.
 * \param [in] comm     Valid MPI communicator.
 * \param [out] size    Without --enable-mpi, return 1.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_size (sc3_MPI_Comm_t comm, int *size);

/** Wrap MPI_Comm_rank.
 * \param [in] comm     Valid MPI communicator.
 * \param [out] rank    Without --enable-mpi, return 0.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_rank (sc3_MPI_Comm_t comm, int *rank);

/** Wrap MPI_Comm_dup.
 * \param [in] comm     Valid MPI communicator.
 * \param [out] newcomm Valid MPI communicator.
 *                      Without --enable-mpi, the same as \a comm.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_dup (sc3_MPI_Comm_t comm,
                                      sc3_MPI_Comm_t * newcomm);

/** Wrap MPI_Comm_split.
 * \param [in] comm     Valid MPI communicator.
 * \param [in] color    See original function.
 *                      Without --enable-mpi, if color is \ref SC3_MPI_UNDEFINED,
 *                      we set \a newcomm to the invalid \ref SC3_MPI_COMM_NULL.
 *                      Otherwise, we set \a newcomm to \a comm.
 * \param [in] key      See original function.  Ignored without --enable-mpi.
 * \param [out] newcomm Valid MPI communicator.  Without --enable-mpi and
 *                      with valid \a color, the same as \a comm.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_split (sc3_MPI_Comm_t comm, int color,
                                        int key, sc3_MPI_Comm_t * newcomm);

/** Split a communicator into sub-communicators by type.
 * If MPI_Comm_split_type or MPI_Win_allocate_shared are missing,
 * then we split by rank unless split_type is \ref SC3_MPI_UNDEFINED.
 * \param [in] comm     Valid MPI communicator.
 * \param [in] split_type   Passed to the original MPI function.
 *                      The wrappers only know \ref SC3_MPI_COMM_TYPE_SHARED.
 * \param [in] key      Passed to wrapped MPI function.
 * \param [in] info     Valid SC3_MPI_Info object.
 * \param [out] newcomm Without --enable-mpi, the wrapper always returns
 *                      a size 1, rank 0 communicator.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_split_type (sc3_MPI_Comm_t comm,
                                             int split_type, int key,
                                             sc3_MPI_Info_t info,
                                             sc3_MPI_Comm_t * newcomm);

/** Wrap MPI_Comm_free.
 * \param [in] comm     Valid MPI communicator.
 *                      On output, \ref SC3_MPI_COMM_NULL.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Comm_free (sc3_MPI_Comm_t * comm);

/** Wrap MPI_Info_create.
 * \param [out] info    Valid MPI info object.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Info_create (sc3_MPI_Info_t * info);

/** Wrap MPI_Info_set.
 * Without --enable-mpi, do nothing.
 * \param [in] info         Valid MPI info object.
 * \param [in] key, value   See original function.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Info_set (sc3_MPI_Info_t info,
                                      const char *key, const char *value);

/** Wrap MPI_Info_free.
 * \param [in] info Valid MPI info object.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Info_free (sc3_MPI_Info_t * info);

/** Return whether an MPI window is valid.
 * We wrap the MPI window into a separate sc3 wrapper structure.
 * This wrapper object must not be mixed with the real MPI window object.
 * We do this to make sure that we use a fast implementation for mpisize 1.
 * \param [in] win           A pointer that is NULL or an sc3_MPI_Win_t.
 * \param [in,out] reason    Pointer, if not NULL, filled with information.
 * \return                    True is valid, false otherwise.
 */
int                 sc3_MPI_Win_is_valid (sc3_MPI_Win_t win, char *reason);

/** Wrap MPI_Win_allocate_shared.
 * \param [in] size, disp_unit, info    See original function.
 * \param [in] comm     Valid MPI communicator.
 *                      If configure does not #define SC_ENABLE_MPICOMMSHARED,
 *                      function must only be called with a size 1 communicator.
 *                      If, on the other hand, configure finds that MPI shared
 *                      windows work, we use a fast replacement for size 1
 *                      and invoke original MPI calls only for sizes > 1.
 * \param [out] baseptr Start of window memory.
 * \param [out] win     New valid MPI window.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Win_allocate_shared
  (sc3_MPI_Aint_t size, int disp_unit, sc3_MPI_Info_t info,
   sc3_MPI_Comm_t comm, void *baseptr, sc3_MPI_Win_t * win);

/** Wrap MPI_Win_shared_query.
 * \param [in] win      New valid MPI window.
 * \param [in] rank     Without --enable-mpi, must match the rank of
 *                      the communicator used for creating the window.
 * \param [out] size    Size of this rank's window allocation.
 * \param [out] disp_unit   The unit provided on window creation.
 * \param [out] baseptr     Start of the rank's window memory.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Win_shared_query
  (sc3_MPI_Win_t win, int rank, sc3_MPI_Aint_t * size, int *disp_unit,
   void *baseptr);

/** Wrap MPI_Win_lock.
 * Without --enable-mpi, we verify that lock and unlock have correct sequence.
 * \param [in] lock_type, assert    Without --enable-mpi, must be one of
 *                                  the enumeration values defined above.
 * \param [in] rank     Rank in window to lock.
 * \param [in] win      MPI window to unlock.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Win_lock (int lock_type, int rank,
                                      int assert, sc3_MPI_Win_t win);

/** Wrap MPI_Win_unlock.
 * Without --enable-mpi, we verify that lock and unlock have correct sequence.
 * \param [in] rank     Rank in window to lock.
 * \param [in] win      MPI window to unlock.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Win_unlock (int rank, sc3_MPI_Win_t win);

/** Wrap MPI_Win_sync.
 * \param [in] win      Valid MPI window.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Win_sync (sc3_MPI_Win_t win);

/** Wrap MPI_Win_free.
 * \param [in] win      Without --enable-mpi, we verify that this MPI window
 *                      is valid and unlocked.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Win_free (sc3_MPI_Win_t * win);

/** Wrap MPI_Allgather.
 * \param [in] comm     Valid MPI communicator.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Barrier (sc3_MPI_Comm_t comm);

/** Wrap MPI_Allgather.
 * \param [in] sendbuf, sendcount, sendtype     See original function.
 * \param [in] recvbuf, recvcount, recvtype     See original function.
 *                      Without --enable-mpi, it represents one rank.
 * \param [in] comm     Valid MPI communicator.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Allgather (void *sendbuf, int sendcount,
                                       sc3_MPI_Datatype_t sendtype,
                                       void *recvbuf, int recvcount,
                                       sc3_MPI_Datatype_t recvtype,
                                       sc3_MPI_Comm_t comm);

/** Wrap MPI_Allgatherv.
 * \param [in] sendbuf, sendcount, sendtype     See original function.
 * \param [in] recvbuf, recvcounts, displs, recvtype     See original.
 *                      Without --enable-mpi, it represents one rank.
 * \param [in] comm     Valid MPI communicator.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Allgatherv (void *sendbuf, int sendcount,
                                        sc3_MPI_Datatype_t sendtype,
                                        void *recvbuf, int *recvcounts,
                                        int *displs,
                                        sc3_MPI_Datatype_t recvtype,
                                        sc3_MPI_Comm_t comm);

/** Wrap MPI_Allreduce.
 * \param [in] sendbuf, recvbuf, count, datatype    See original function.
 * \param [in] op       Valid MPI operation type.
 * \param [in] comm     Valid MPI communicator.
 * \return          NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_MPI_Allreduce (void *sendbuf, void *recvbuf,
                                       int count, sc3_MPI_Datatype_t datatype,
                                       sc3_MPI_Op_t op, sc3_MPI_Comm_t comm);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_MPI_H */
