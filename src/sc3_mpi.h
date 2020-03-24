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

#ifndef SC_ENABLE_MPI

/** Wrapped MPI error handler object. */
typedef struct sc3_MPI_Errhandler *sc3_MPI_Errhandler_t;

/** Wrapped MPI communicator.
 * Without --enable-mpi, it reports size 1 and rank 0. */
typedef struct sc3_MPI_Comm *sc3_MPI_Comm_t;

/** Wrapped MPI info object.
 * Without --enable-mpi, the set and get methods do nothing. */
typedef struct sc3_MPI_Info *sc3_MPI_Info_t;

/** We wrap the MPI datatypes we use. */
typedef enum sc3_MPI_Datatype
{
  SC3_MPI_BYTE,         /**< Same as in original MPI.  1 byte.
                         * It is preferred over MPI_CHAR since the latter
                         * may amount to multiple bytes. */
  SC3_MPI_INT,          /**< Same as in original MPI.  System int type. */
  SC3_MPI_LONG,         /**< Same as in original MPI.  System long type. */
  SC3_MPI_FLOAT,        /**< Same as in original MPI. */
  SC3_MPI_DOUBLE        /**< Same as in original MPI. */
}
sc3_MPI_Datatype_t;

/** We wrap the MPI operation types we use. */
typedef enum sc3_MPI_Op
{
  SC3_MPI_MIN,          /**< The usual minimum reduction operation. */
  SC3_MPI_MAX,          /**< The usual maximum reduction operation. */
  SC3_MPI_SUM           /**< The usual sum reduction operation. */
}
sc3_MPI_Op_t;

/** We wrap two MPI error codes. */
typedef enum sc3_MPI_Errorcode
{
  SC3_MPI_SUCCESS,      /**< An MPI function has exited successfully. */
  SC3_MPI_ERR_OTHER     /**< An MPI function has produced an error. */
}
sc3_MPI_Errorcode_t;

/** We need several parameters to MPI functions. */
typedef enum sc3_MPI_Enum
{
  SC3_MPI_UNDEFINED = -1    /**< An input parameter with undefined value. */
}
sc3_MPI_Enum_t;

/** Wrap the parameter to return on MPI errors. */
#define SC3_MPI_ERRORS_RETURN NULL

/** Create a wrapped version of the maximum error string length. */
#define SC3_MPI_MAX_ERROR_STRING SC3_BUFSIZE

/** Without --enable-mpi, a communicator of size 1 and rank 0. */
extern sc3_MPI_Comm_t SC3_MPI_COMM_WORLD;

/** Without --enable-mpi, a communicator of size 1 and rank 0. */
extern sc3_MPI_Comm_t SC3_MPI_COMM_SELF;

/** Wrapped invalid MPI communicator. */
extern sc3_MPI_Comm_t SC3_MPI_COMM_NULL;

/** Wrapped invalid MPI info object. */
extern sc3_MPI_Info_t SC3_MPI_INFO_NULL;

#else
#include <mpi.h>

typedef MPI_Errhandler sc3_MPI_Errhandler_t;
typedef MPI_Comm    sc3_MPI_Comm_t;
typedef MPI_Info    sc3_MPI_Info_t;
typedef MPI_Datatype sc3_MPI_Datatype_t;
typedef MPI_Op      sc3_MPI_Op_t;

#define SC3_MPI_BYTE MPI_BYTE
#define SC3_MPI_INT MPI_INT
#define SC3_MPI_LONG MPI_LONG
#define SC3_MPI_FLOAT MPI_FLOAT
#define SC3_MPI_DOUBLE MPI_DOUBLE

#define SC3_MPI_MIN MPI_MIN
#define SC3_MPI_MAX MPI_MAX
#define SC3_MPI_SUM MPI_SUM

#define SC3_MPI_ERRORS_RETURN MPI_ERRORS_RETURN
#define SC3_MPI_COMM_WORLD MPI_COMM_WORLD
#define SC3_MPI_COMM_SELF MPI_COMM_SELF
#define SC3_MPI_COMM_NULL MPI_COMM_NULL
#define SC3_MPI_INFO_NULL MPI_INFO_NULL

#define SC3_MPI_MAX_ERROR_STRING MPI_MAX_ERROR_STRING
#define SC3_MPI_SUCCESS MPI_SUCCESS
#define SC3_MPI_ERR_OTHER MPI_ERR_OTHER
#define SC3_MPI_UNDEFINED MPI_UNDEFINED

#ifdef SC_ENABLE_MPICOMMSHARED
#ifdef SC_ENABLE_MPIWINSHARED
#define SC3_ENABLE_MPI3
#endif /* SC_ENABLE_MPIWINSHARED */
#endif /* SC_ENABLE_MPICOMMSHARED */

#endif /* SC_ENABLE_MPI */

#ifndef SC_ENABLE_MPICOMMSHARED

/** We wrap MPI 3 shared memory communicators.
 * Without --enable-mpi, they have size 1 and rank 0. */
typedef enum sc3_MPI_Comm_type
{
  SC3_MPI_COMM_TYPE_SHARED = -2 /**< MPI 3 shared window communicator. */
}
sc3_MPI_Comm_type_t;

#else

#define SC3_MPI_COMM_TYPE_SHARED MPI_COMM_TYPE_SHARED

#endif /* SC_ENABLE_MPICOMMSHARED */

#ifndef SC_ENABLE_MPIWINSHARED

/** We wrap the MPI address integer type. */
typedef long        sc3_MPI_Aint_t;

/** We wrap the MPI 3 window object.
 * Without MPI 3 available, it is primitive but allows basic use. */
typedef struct sc3_MPI_Win *sc3_MPI_Win_t;

/** Wrap MPI 3 window lock modes. */
typedef enum sc3_MPI_Win_mode
{
  SC3_MPI_LOCK_SHARED = -3,     /**< Shared (multiple readers) lock. */
  SC3_MPI_LOCK_EXCLUSIVE = -4,  /**< Exclusive (usually reader) lock. */
  SC3_MPI_MODE_NOCHECK = -5,    /**< Option to \ref sc3_MPI_Win_lock. */
}
sc3_MPI_Win_mode_t;

/** Invalid MPI 3 window object. */
extern sc3_MPI_Win_t SC3_MPI_WIN_NULL;

#else

typedef MPI_Aint    sc3_MPI_Aint_t;
typedef MPI_Win     sc3_MPI_Win_t;

#define SC3_MPI_LOCK_SHARED MPI_LOCK_SHARED
#define SC3_MPI_LOCK_EXCLUSIVE MPI_LOCK_EXCLUSIVE
#define SC3_MPI_MODE_NOCHECK MPI_MODE_NOCHECK

#define SC3_MPI_WIN_NULL MPI_WIN_NULL

#endif /* SC_ENABLE_MPIWINSHARED */

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

/** Wrap MPI_Comm_set_errhandler.
 * \param [in] comm     Valid MPI communicator.
 * \param [in] errh     Valid MPI errror handler object.
 *                      May use it \ref SC3_MPI_ERRORS_RETURN.
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

/** Wrap MPI_Win_allocate_shared.
 * \param [in] size, disp_unit, info    See original function.
 * \param [in] comm     Valid MPI communicator.
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
