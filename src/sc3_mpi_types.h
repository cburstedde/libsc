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

/** \file sc3_mpi_types.h
 *
 * We provide MPI replacement data types for configuring without MPI.
 * Included from both \ref sc_mpi.h and the future \c sc3_mpi.h.
 *
 * This header file provides definitions of MPI data types for the case
 * that no MPI implementation is available or MPI is not configured.
 *
 * \ingroup sc_parallelism
 */

#ifndef SC3_MPI_TYPES_H
#define SC3_MPI_TYPES_H

/* this works both standalone and when included from sc.h */
#include <sc_config.h>
#ifdef SC_ENABLE_MPI
#include <mpi.h>

typedef MPI_Errhandler sc3_MPI_Errhandler_t;
typedef MPI_Comm    sc3_MPI_Comm_t;
typedef MPI_Info    sc3_MPI_Info_t;
typedef MPI_Datatype sc3_MPI_Datatype_t;
typedef MPI_Op      sc3_MPI_Op_t;

#define SC3_MPI_UNDEFINED MPI_UNDEFINED

#define SC3_MPI_DATATYPE_NULL MPI_DATATYPE_NULL
#define SC3_MPI_BYTE MPI_BYTE
#define SC3_MPI_INT MPI_INT
#define SC3_MPI_2INT MPI_2INT
#define SC3_MPI_UNSIGNED MPI_UNSIGNED
#define SC3_MPI_LONG MPI_LONG
#define SC3_MPI_LONG_LONG MPI_LONG_LONG_INT
#define SC3_MPI_FLOAT MPI_FLOAT
#define SC3_MPI_DOUBLE MPI_DOUBLE
#define SC3_MPI_DOUBLE_INT MPI_DOUBLE_INT

#define SC3_MPI_OP_NULL MPI_OP_NULL
#define SC3_MPI_MIN MPI_MIN
#define SC3_MPI_MAX MPI_MAX
#define SC3_MPI_MINLOC MPI_MINLOC
#define SC3_MPI_MAXLOC MPI_MAXLOC
#define SC3_MPI_LOR MPI_LOR
#define SC3_MPI_LAND MPI_LAND
#define SC3_MPI_LXOR MPI_LXOR
#define SC3_MPI_BOR MPI_BOR
#define SC3_MPI_BAND MPI_BAND
#define SC3_MPI_BXOR MPI_BXOR
#define SC3_MPI_REPLACE MPI_REPLACE
#define SC3_MPI_SUM MPI_SUM
#define SC3_MPI_PROD MPI_PROD

#define SC3_MPI_ERRORS_RETURN MPI_ERRORS_RETURN
#define SC3_MPI_COMM_WORLD MPI_COMM_WORLD
#define SC3_MPI_COMM_SELF MPI_COMM_SELF
#define SC3_MPI_COMM_NULL MPI_COMM_NULL
#define SC3_MPI_INFO_NULL MPI_INFO_NULL

#define SC3_MPI_MAX_ERROR_STRING MPI_MAX_ERROR_STRING
#define SC3_MPI_SUCCESS MPI_SUCCESS
#define SC3_MPI_ERR_ARG MPI_ERR_ARG
#define SC3_MPI_ERR_COUNT MPI_ERR_COUNT
#define SC3_MPI_ERR_UNKNOWN MPI_ERR_UNKNOWN
#define SC3_MPI_ERR_OTHER MPI_ERR_OTHER
#define SC3_MPI_ERR_NO_MEM MPI_ERR_NO_MEM

#ifdef SC_ENABLE_MPIIO

#define SC_MPI_ERR_FILE                   MPI_ERR_FILE
#define SC_MPI_ERR_NOT_SAME               MPI_ERR_NOT_SAME
#define SC_MPI_ERR_AMODE                  MPI_ERR_AMODE
#define SC_MPI_ERR_UNSUPPORTED_DATAREP    MPI_ERR_UNSUPPORTED_DATAREP
#define SC_MPI_ERR_UNSUPPORTED_OPERATION  MPI_ERR_UNSUPPORTED_OPERATION
#define SC_MPI_ERR_NO_SUCH_FILE           MPI_ERR_NO_SUCH_FILE
#define SC_MPI_ERR_FILE_EXISTS            MPI_ERR_FILE_EXISTS
#define SC_MPI_ERR_BAD_FILE               MPI_ERR_BAD_FILE
#define SC_MPI_ERR_ACCESS                 MPI_ERR_ACCESS
#define SC_MPI_ERR_NO_SPACE               MPI_ERR_NO_SPACE
#define SC_MPI_ERR_QUOTA                  MPI_ERR_QUOTA
#define SC_MPI_ERR_READ_ONLY              MPI_ERR_READ_ONLY
#define SC_MPI_ERR_FILE_IN_USE            MPI_ERR_FILE_IN_USE
#define SC_MPI_ERR_DUP_DATAREP            MPI_ERR_DUP_DATAREP
#define SC_MPI_ERR_CONVERSION             MPI_ERR_CONVERSION
#define SC_MPI_ERR_IO                     MPI_ERR_IO

#define SC3_MPI_ERR_LASTCODE              MPI_ERR_LASTCODE

#else

typedef enum sc3_MPI_IO_Errorcode
{
  /* only MPI I/O error classes */
  /* WARNING: This enum is only used in the deprecated case of activated MPI but
   * deactivated MPI I/O.
   */
  SC3_MPI_ERR_FILE = MPI_ERR_LASTCODE,
  SC3_MPI_ERR_NOT_SAME,
  SC3_MPI_ERR_AMODE,
  SC3_MPI_ERR_UNSUPPORTED_DATAREP,
  SC3_MPI_ERR_UNSUPPORTED_OPERATION,
  SC3_MPI_ERR_NO_SUCH_FILE,
  SC3_MPI_ERR_FILE_EXISTS,
  SC3_MPI_ERR_BAD_FILE,
  SC3_MPI_ERR_ACCESS,
  SC3_MPI_ERR_NO_SPACE,
  SC3_MPI_ERR_QUOTA,
  SC3_MPI_ERR_READ_ONLY,
  SC3_MPI_ERR_FILE_IN_USE,
  SC3_MPI_ERR_DUP_DATAREP,
  SC3_MPI_ERR_CONVERSION,
  SC3_MPI_ERR_IO,
  SC3_MPI_ERR_LASTCODE
}
sc3_MPI_IO_Errorcode_t;

#endif /* !SC_ENABLE_MPIIO */

#ifdef SC_ENABLE_MPICOMMSHARED
#ifdef SC_ENABLE_MPIWINSHARED
#define SC3_ENABLE_MPI3
#endif /* SC_ENABLE_MPIWINSHARED */
#endif /* SC_ENABLE_MPICOMMSHARED */
#else /* !SC_ENABLE_MPI */

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
  SC3_MPI_DATATYPE_NULL = 0,    /**< The invalid data type. */
  SC3_MPI_BYTE,         /**< Same as in original MPI.  1 byte.
                         * It is preferred over MPI_CHAR since the latter
                         * may amount to multiple bytes for wide chars. */
  SC3_MPI_INT,          /**< Same as in original MPI.  System int type. */
  SC3_MPI_2INT,         /**< Same as in original MPI.  Two system ints. */
  SC3_MPI_UNSIGNED,     /**< Same as in original MPI.  System unsigned. */
  SC3_MPI_LONG,         /**< Same as in original MPI.  System long type. */
  SC3_MPI_LONG_LONG,    /**< Same as in original MPI.  System long long. */
  SC3_MPI_FLOAT,        /**< Same as in original MPI. */
  SC3_MPI_DOUBLE,       /**< Same as in original MPI. */
  SC3_MPI_DOUBLE_INT    /**< Same as in original MPI.  Double and int. */
}
sc3_MPI_Datatype_t;

/** We wrap the MPI operation types we use. */
typedef enum sc3_MPI_Op
{
  SC3_MPI_OP_NULL = 0,  /**< The invalid operation. */
  SC3_MPI_MIN,          /**< The usual minimum reduction operation. */
  SC3_MPI_MAX,          /**< The usual maximum reduction operation. */
  SC3_MPI_MINLOC,       /**< Find minimum value and its rank. */
  SC3_MPI_MAXLOC,       /**< Find maximum value and its rank. */
  SC3_MPI_LOR,          /**< Logical OR */
  SC3_MPI_LAND,         /**< Logical AND */
  SC3_MPI_LXOR,         /**< Logical XOR */
  SC3_MPI_BOR,          /**< Bitwise OR */
  SC3_MPI_BAND,         /**< Bitwise AND */
  SC3_MPI_BXOR,         /**< Bitwise XOR */
  SC3_MPI_REPLACE,      /**< The MPI replace operation */
  SC3_MPI_SUM,          /**< The usual sum reduction operation. */
  SC3_MPI_PROD          /**< The usual product reduction operation. */
}
sc3_MPI_Op_t;

/** We wrap some MPI error codes and the I/O error classes. */
typedef enum sc3_MPI_Errorcode
{
  SC3_MPI_SUCCESS = 0,  /**< An MPI function has exited successfully. */
  SC3_MPI_ERR_ARG = 14000, /**< An MPI function encountered invalid arguments. */
  SC3_MPI_ERR_COUNT,     /**< An MPI function encountered an invalid count argument. */
  SC3_MPI_ERR_UNKNOWN,  /**< An MPI function has produced an unknown error. */
  SC3_MPI_ERR_OTHER,    /**< An MPI function has produced some known error. */
  SC3_MPI_ERR_NO_MEM,   /**< An MPI function ran out of memory. */

  /* add MPI I/O error classes */
  SC3_MPI_ERR_FILE,
  SC3_MPI_ERR_NOT_SAME,
  SC3_MPI_ERR_AMODE,
  SC3_MPI_ERR_UNSUPPORTED_DATAREP,
  SC3_MPI_ERR_UNSUPPORTED_OPERATION,
  SC3_MPI_ERR_NO_SUCH_FILE,
  SC3_MPI_ERR_FILE_EXISTS,
  SC3_MPI_ERR_BAD_FILE,
  SC3_MPI_ERR_ACCESS,
  SC3_MPI_ERR_NO_SPACE,
  SC3_MPI_ERR_QUOTA,
  SC3_MPI_ERR_READ_ONLY,
  SC3_MPI_ERR_FILE_IN_USE,
  SC3_MPI_ERR_DUP_DATAREP,
  SC3_MPI_ERR_CONVERSION,
  SC3_MPI_ERR_IO,
  SC3_MPI_ERR_LASTCODE
}
sc3_MPI_Errorcode_t;

/** We need several parameters to MPI functions. */
typedef enum sc3_MPI_Enum
{
  SC3_MPI_UNDEFINED = 0x11  /**< An input parameter with undefined value. */
}
sc3_MPI_Enum_t;

/** Wrap the parameter to return on MPI errors. */
#define SC3_MPI_ERRORS_RETURN NULL

/** Create a wrapped version of the maximum error string length. */
#define SC3_MPI_MAX_ERROR_STRING BUFSIZ

/** Wrapped invalid MPI communicator. */
#define SC3_MPI_COMM_NULL ((sc3_MPI_Comm_t) NULL)

/** Without --enable-mpi, a communicator of size 1 and rank 0. */
#define SC3_MPI_COMM_WORLD ((sc3_MPI_Comm_t) 0x40)

/** Without --enable-mpi, a communicator of size 1 and rank 0. */
#define SC3_MPI_COMM_SELF ((sc3_MPI_Comm_t) 0x80)

/** Wrapped invalid MPI info object. */
#define SC3_MPI_INFO_NULL ((sc3_MPI_Info_t) NULL)

#endif /* !SC_ENABLE_MPI */

#ifndef SC_ENABLE_MPICOMMSHARED
/* It is possible that MPI exists but the shared communicators do not. */

/** We wrap MPI 3 shared memory communicators.
 * Without --enable-mpi, they have size 1 and rank 0. */
typedef enum sc3_MPI_Comm_type
{
  SC3_MPI_COMM_TYPE_SHARED = 0x12 /**< MPI 3 shared window communicator. */
}
sc3_MPI_Comm_type_t;

#else
/* We know that MPI is generally available. */

#ifdef SC_ENABLE_OMPICOMMSOCKET
/* We can split by socket */
#define SC3_MPI_COMM_TYPE_SHARED OMPI_COMM_TYPE_SOCKET
#else
#define SC3_MPI_COMM_TYPE_SHARED MPI_COMM_TYPE_SHARED
#endif /* SC_ENABLE_OMPICOMMSOCKET */

#endif /* SC_ENABLE_MPICOMMSHARED */

/** We wrap the MPI 3 window object.
 * With MPI 3 windows available, the functionality is unchanged.
 * Without MPI 3 available, it is primitive but allows basic use.
 *
 * When created with a communicator size of one, or when `configure` does not
 * define SC_ENABLE_MPIWINSHARED, we use the wrapper and do not go through MPI.
 * When shared windows are not supported and the communicator has size > 1,
 * the wrapper cannot query or lock and unlock the window of remote ranks.
 * Any such attempt will result in a fatal error returned.
 */
typedef struct sc3_MPI_Win *sc3_MPI_Win_t;

/** Invalid MPI 3 window object. */
#define SC3_MPI_WIN_NULL ((sc3_MPI_Win_t) NULL)

#ifndef SC_ENABLE_MPIWINSHARED
/* It is possible that MPI exists but the shared windows do not. */

/** We wrap the MPI address integer type. */
typedef long        sc3_MPI_Aint_t;

/** Wrap MPI 3 window lock modes. */
typedef enum sc3_MPI_Win_mode
{
  SC3_MPI_LOCK_SHARED = 0x13,     /**< Shared (multiple readers) lock. */
  SC3_MPI_LOCK_EXCLUSIVE = 0x14,  /**< Exclusive (usually reader) lock. */
  SC3_MPI_MODE_NOCHECK = 0x15     /**< Option to \c MPI_Win_lock. */
}
sc3_MPI_Win_mode_t;

#else
/* We know that MPI is generally available. */

typedef MPI_Aint    sc3_MPI_Aint_t;

#define SC3_MPI_LOCK_SHARED MPI_LOCK_SHARED
#define SC3_MPI_LOCK_EXCLUSIVE MPI_LOCK_EXCLUSIVE
#define SC3_MPI_MODE_NOCHECK MPI_MODE_NOCHECK

#endif /* SC_ENABLE_MPIWINSHARED */

#endif /* !SC3_MPI_TYPES_H */
