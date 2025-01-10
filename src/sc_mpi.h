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

/** \file sc_mpi.h
 *
 * Provide a consistent MPI interface with and without MPI configured.
 *
 * \ingroup sc_parallelism
 *
 * When MPI is configured, we redefine many MPI functions and objects.
 * Without MPI, we emulate collective MPI routines to work as expected.
 *
 * The goal is to make code compile and execute cleanly when `--enable-mpi` is
 * not given on the configure line.  To this end, several MPI routines that are
 * meaningful to call on one processor are provided with the prefix `sc_MPI_`,
 * as well as necessary types and defines.  If `--enable-mpi` is given, this
 * file provides macros that map the `sc_`-prefixed form to the standard
 * form of the symbols.
 *
 * When including this file in your code, everything inside \#ifdef
 * SC_ENABLE_MPI may as well use the standard MPI API.  Outside of this
 * definition block, the sc_MPI_* routines specified here are allow
 * to seamlessly use MPI calls without breaking non-MPI code.
 *
 * Some send and receive routines are wrapped.  They can thus be used
 * in code outside of \#ifdef SC_ENABLE_MPI even though they will abort.  If
 * no messages are sent to the same processor when mpisize == 1, such aborts
 * will not occur.  The `MPI_Wait*` routines are safe to call as long as no or
 * only MPI_REQUEST_NULL requests are passed in.
 */

/**
 * \defgroup sc_parallelism Parallelism
 *
 * The sc library provides several mechanisms to work with MPI.
 * The most important one is a wrapper that looks the same to the user
 * whether MPI has been configured or not.  This allows to use MPI commands
 * in code without protecting \#defines.
 *
 * In addition, we provide MPI reduce, allreduce replacements with
 * reproducible associativity as well as an allgather replacement.
 *
 * \ingroup sc
 */

#ifndef SC_MPI_H
#define SC_MPI_H

/* this works both standalone and when included from sc.h */
#include <sc_config.h>
#ifdef SC_ENABLE_MPI
#include <mpi.h>
#endif

SC_EXTERN_C_BEGIN;

/** Enumerate all MPI tags used internally to the sc library. */
typedef enum
{
  SC_TAG_FIRST = 's' + 'c',     /**< Anything really. */
  SC_TAG_AG_ALLTOALL = SC_TAG_FIRST,    /**< Used in MPI alltoall replacement. */
  SC_TAG_AG_RECURSIVE_A,        /**< Internal tag; do not use. */
  SC_TAG_AG_RECURSIVE_B,        /**< Internal tag; do not use. */
  SC_TAG_AG_RECURSIVE_C,        /**< Internal tag; do not use. */
  SC_TAG_NOTIFY_CENSUS,         /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_CENSUSV,        /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_NBX,            /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_NBXV,           /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_WRAPPER,        /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_WRAPPERV,       /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_RANGES,         /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_PAYLOAD,        /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_SUPER_TRUE,     /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_SUPER_EXTRA,    /**< Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_RECURSIVE,      /**< Internal tag to \ref sc_notify. */
  /** Internal tag to \ref sc_notify. */
  SC_TAG_NOTIFY_NARY = SC_TAG_NOTIFY_RECURSIVE + 32,
  SC_TAG_REDUCE = SC_TAG_NOTIFY_NARY + 32,  /**< Used in MPI reduce replacement. */
  SC_TAG_PSORT_LO,              /**< Internal tag to \ref sc_psort. */
  SC_TAG_PSORT_HI,              /**< Internal tag to \ref sc_psort. */
  SC_TAG_LAST                   /**< End marker of tag enumeration. */
}
sc_tag_t;

#ifdef SC_ENABLE_MPI

/* constants */
#define sc_MPI_SUCCESS             MPI_SUCCESS
#define sc_MPI_ERR_ARG             MPI_ERR_ARG
#define sc_MPI_ERR_COUNT           MPI_ERR_COUNT
#define sc_MPI_ERR_UNKNOWN         MPI_ERR_UNKNOWN
#define sc_MPI_ERR_OTHER           MPI_ERR_OTHER
#define sc_MPI_ERR_NO_MEM          MPI_ERR_NO_MEM
#define sc_MPI_MAX_ERROR_STRING    MPI_MAX_ERROR_STRING

#ifdef SC_ENABLE_MPIIO

#define sc_MPI_ERR_FILE                   MPI_ERR_FILE
#define sc_MPI_ERR_NOT_SAME               MPI_ERR_NOT_SAME
#define sc_MPI_ERR_AMODE                  MPI_ERR_AMODE
#define sc_MPI_ERR_UNSUPPORTED_DATAREP    MPI_ERR_UNSUPPORTED_DATAREP
#define sc_MPI_ERR_UNSUPPORTED_OPERATION  MPI_ERR_UNSUPPORTED_OPERATION
#define sc_MPI_ERR_NO_SUCH_FILE           MPI_ERR_NO_SUCH_FILE
#define sc_MPI_ERR_FILE_EXISTS            MPI_ERR_FILE_EXISTS
#define sc_MPI_ERR_BAD_FILE               MPI_ERR_BAD_FILE
#define sc_MPI_ERR_ACCESS                 MPI_ERR_ACCESS
#define sc_MPI_ERR_NO_SPACE               MPI_ERR_NO_SPACE
#define sc_MPI_ERR_QUOTA                  MPI_ERR_QUOTA
#define sc_MPI_ERR_READ_ONLY              MPI_ERR_READ_ONLY
#define sc_MPI_ERR_FILE_IN_USE            MPI_ERR_FILE_IN_USE
#define sc_MPI_ERR_DUP_DATAREP            MPI_ERR_DUP_DATAREP
#define sc_MPI_ERR_CONVERSION             MPI_ERR_CONVERSION
#define sc_MPI_ERR_IO                     MPI_ERR_IO

#define sc_MPI_ERR_LASTCODE               MPI_ERR_LASTCODE

/* MPI 2.0 type */
#define sc_MPI_Aint                MPI_Aint

#else /* !SC_ENABLE_MPIIO */

typedef enum sc_MPI_IO_Errorcode
{
  /* only MPI I/O error classes */
  /* WARNING: This enum is only used in the deprecated case of activated MPI but
   * deactivated MPI I/O.
   */
  sc_MPI_ERR_FILE = MPI_ERR_LASTCODE,
  sc_MPI_ERR_NOT_SAME,
  sc_MPI_ERR_AMODE,
  sc_MPI_ERR_UNSUPPORTED_DATAREP,
  sc_MPI_ERR_UNSUPPORTED_OPERATION,
  sc_MPI_ERR_NO_SUCH_FILE,
  sc_MPI_ERR_FILE_EXISTS,
  sc_MPI_ERR_BAD_FILE,
  sc_MPI_ERR_ACCESS,
  sc_MPI_ERR_NO_SPACE,
  sc_MPI_ERR_QUOTA,
  sc_MPI_ERR_READ_ONLY,
  sc_MPI_ERR_FILE_IN_USE,
  sc_MPI_ERR_DUP_DATAREP,
  sc_MPI_ERR_CONVERSION,
  sc_MPI_ERR_IO,
  sc_MPI_ERR_LASTCODE
}
sc_MPI_IO_Errorcode_t;

#define sc_MPI_Aint                sc3_MPI_Aint_t

#endif /* !SC_ENABLE_MPIIO */

#define sc_MPI_COMM_NULL           MPI_COMM_NULL
#define sc_MPI_COMM_WORLD          MPI_COMM_WORLD
#define sc_MPI_COMM_SELF           MPI_COMM_SELF
#define sc_MPI_COMM_TYPE_SHARED    MPI_COMM_TYPE_SHARED

#define sc_MPI_GROUP_NULL          MPI_GROUP_NULL
#define sc_MPI_GROUP_EMPTY         MPI_GROUP_EMPTY

#define sc_MPI_IDENT               MPI_IDENT
#define sc_MPI_CONGRUENT           MPI_CONGRUENT
#define sc_MPI_SIMILAR             MPI_SIMILAR
#define sc_MPI_UNEQUAL             MPI_UNEQUAL

#define sc_MPI_ANY_SOURCE          MPI_ANY_SOURCE
#define sc_MPI_ANY_TAG             MPI_ANY_TAG
#define sc_MPI_STATUS_IGNORE       MPI_STATUS_IGNORE
#define sc_MPI_STATUSES_IGNORE     MPI_STATUSES_IGNORE

#define sc_MPI_REQUEST_NULL        MPI_REQUEST_NULL

#define sc_MPI_DATATYPE_NULL       MPI_DATATYPE_NULL
#define sc_MPI_BYTE                MPI_BYTE
#define sc_MPI_CHAR                MPI_CHAR
#define sc_MPI_UNSIGNED_CHAR       MPI_UNSIGNED_CHAR
#define sc_MPI_SHORT               MPI_SHORT
#define sc_MPI_UNSIGNED_SHORT      MPI_UNSIGNED_SHORT
#define sc_MPI_INT                 MPI_INT
#define sc_MPI_UNSIGNED            MPI_UNSIGNED
#define sc_MPI_LONG                MPI_LONG
#define sc_MPI_UNSIGNED_LONG       MPI_UNSIGNED_LONG
/* Not an MPI 1.3 data type */
#ifdef SC_HAVE_MPI_UNSIGNED_LONG_LONG
#define sc_MPI_UNSIGNED_LONG_LONG  MPI_UNSIGNED_LONG_LONG
#else
/* MPI data type is not supported */
#define sc_MPI_UNSIGNED_LONG_LONG  MPI_DATATYPE_NULL
#endif
/* Not an MPI 1.3 data type */
#ifdef SC_HAVE_MPI_SIGNED_CHAR
#define sc_MPI_SIGNED_CHAR         MPI_SIGNED_CHAR
#else
/* MPI data type is not supported */
#define sc_MPI_SIGNED_CHAR         MPI_DATATYPE_NULL
#endif
/* Not an MPI 1.3 data type */
#ifdef SC_HAVE_MPI_INT8_T
#define sc_MPI_INT8_T              MPI_INT8_T
#else
/* MPI data type is not supported */
#define sc_MPI_INT8_T              MPI_DATATYPE_NULL
#endif
#define sc_MPI_LONG_LONG_INT       MPI_LONG_LONG_INT
#define sc_MPI_FLOAT               MPI_FLOAT
#define sc_MPI_DOUBLE              MPI_DOUBLE
#define sc_MPI_LONG_DOUBLE         MPI_LONG_DOUBLE
#define sc_MPI_2INT                MPI_2INT
#define sc_MPI_DOUBLE_INT          MPI_DOUBLE_INT
#define sc_MPI_PACKED              MPI_PACKED

#define sc_MPI_OP_NULL             MPI_OP_NULL
#define sc_MPI_MAX                 MPI_MAX
#define sc_MPI_MIN                 MPI_MIN
#define sc_MPI_LAND                MPI_LAND
#define sc_MPI_BAND                MPI_BAND
#define sc_MPI_LOR                 MPI_LOR
#define sc_MPI_BOR                 MPI_BOR
#define sc_MPI_LXOR                MPI_LXOR
#define sc_MPI_BXOR                MPI_BXOR
#define sc_MPI_MINLOC              MPI_MINLOC
#define sc_MPI_MAXLOC              MPI_MAXLOC
#define sc_MPI_REPLACE             MPI_REPLACE
#define sc_MPI_SUM                 MPI_SUM
#define sc_MPI_PROD                MPI_PROD

#define sc_MPI_UNDEFINED           MPI_UNDEFINED

#define sc_MPI_KEYVAL_INVALID      MPI_KEYVAL_INVALID

/* types */

#define sc_MPI_Comm                MPI_Comm
#define sc_MPI_Group               MPI_Group
#define sc_MPI_Datatype            MPI_Datatype
#define sc_MPI_Op                  MPI_Op
#define sc_MPI_Request             MPI_Request
#define sc_MPI_Status              MPI_Status
#define sc_MPI_Info                MPI_Info

/* MPI info arguments */

#define sc_MPI_INFO_NULL           MPI_INFO_NULL

/* MPI functions */

#define sc_MPI_Init                MPI_Init
/*      sc_MPI_Init_thread is handled below */
#define sc_MPI_Finalize            MPI_Finalize
#define sc_MPI_Abort               MPI_Abort
#define sc_MPI_Alloc_mem           MPI_Alloc_mem
#define sc_MPI_Free_mem            MPI_Free_mem
#define sc_MPI_Comm_set_attr       MPI_Comm_set_attr
#define sc_MPI_Comm_get_attr       MPI_Comm_get_attr
#define sc_MPI_Comm_delete_attr    MPI_Comm_delete_attr
#define sc_MPI_Comm_create_keyval  MPI_Comm_create_keyval
#define sc_MPI_Comm_dup            MPI_Comm_dup
#define sc_MPI_Comm_create         MPI_Comm_create
#define sc_MPI_Comm_split          MPI_Comm_split
#define sc_MPI_Comm_split_type     MPI_Comm_split_type
#define sc_MPI_Comm_free           MPI_Comm_free
#define sc_MPI_Comm_size           MPI_Comm_size
#define sc_MPI_Comm_rank           MPI_Comm_rank
#define sc_MPI_Comm_compare        MPI_Comm_compare
#define sc_MPI_Comm_group          MPI_Comm_group
#define sc_MPI_Group_free          MPI_Group_free
#define sc_MPI_Group_size          MPI_Group_size
#define sc_MPI_Group_rank          MPI_Group_rank
#define sc_MPI_Group_translate_ranks MPI_Group_translate_ranks
#define sc_MPI_Group_compare       MPI_Group_compare
#define sc_MPI_Group_union         MPI_Group_union
#define sc_MPI_Group_intersection  MPI_Group_intersection
#define sc_MPI_Group_difference    MPI_Group_difference
#define sc_MPI_Group_incl          MPI_Group_incl
#define sc_MPI_Group_excl          MPI_Group_excl
#define sc_MPI_Group_range_incl    MPI_Group_range_incl
#define sc_MPI_Group_range_excl    MPI_Group_range_excl
#define sc_MPI_Barrier             MPI_Barrier
#define sc_MPI_Bcast               MPI_Bcast
#define sc_MPI_Gather              MPI_Gather
#define sc_MPI_Gatherv             MPI_Gatherv
#define sc_MPI_Allgather           MPI_Allgather
#define sc_MPI_Allgatherv          MPI_Allgatherv
#define sc_MPI_Alltoall            MPI_Alltoall
#define sc_MPI_Reduce              MPI_Reduce
#define sc_MPI_Reduce_scatter_block MPI_Reduce_scatter_block
#define sc_MPI_Allreduce           MPI_Allreduce
#define sc_MPI_Scan                MPI_Scan
#define sc_MPI_Exscan              MPI_Exscan
#define sc_MPI_Recv                MPI_Recv
#define sc_MPI_Irecv               MPI_Irecv
#define sc_MPI_Send                MPI_Send
#define sc_MPI_Isend               MPI_Isend
#define sc_MPI_Probe               MPI_Probe
#define sc_MPI_Iprobe              MPI_Iprobe
#define sc_MPI_Get_count           MPI_Get_count
#define sc_MPI_Wtime               MPI_Wtime
#define sc_MPI_Wait                MPI_Wait
/* The MPI_Waitsome, MPI_Waitall and MPI_Testall functions are wrapped. */
#define sc_MPI_Type_size           MPI_Type_size
#define sc_MPI_Pack                MPI_Pack
#define sc_MPI_Unpack              MPI_Unpack
#define sc_MPI_Pack_size           MPI_Pack_size
#ifdef SC_HAVE_AINT_DIFF
/* MPI 3.0 function */
#define sc_MPI_Aint_diff           MPI_Aint_diff
#else
/* Replacement by standard subtraction.
 *
 * MPI 2.0 supports MPI_Aint but does not support MPI_Aint_diff.
 * In the MPI 2.0 standard document
 * (https://www.mpi-forum.org/docs/mpi-2.0/mpi2-report.pdf) on page 283 is
 * an example of calculating MPI_Aint displacements by standard subtraction.
 * Therefore, we also use standard subtraction in the case MPI_Aint_diff is
 * not available.
 */
sc_MPI_Aint         sc_MPI_Aint_diff (sc_MPI_Aint a, sc_MPI_Aint b);
#endif

#else /* !SC_ENABLE_MPI */
#include <sc3_mpi_types.h>

/* constants */

#define sc_MPI_SUCCESS          SC3_MPI_SUCCESS         /**< Emulate \c SC_MPI_SUCCESS. */
#define sc_MPI_ERR_ARG          SC3_MPI_ERR_ARG         /**< Emulate \c SC_MPI_ERR_ARG. */
#define sc_MPI_ERR_COUNT        SC3_MPI_ERR_COUNT       /**< Emulate \c SC_MPI_ERR_COUNT. */
#define sc_MPI_ERR_UNKNOWN      SC3_MPI_ERR_UNKNOWN     /**< Emulate \c SC_MPI_ERR_UNKNOWN. */
#define sc_MPI_ERR_OTHER        SC3_MPI_ERR_OTHER       /**< Emulate \c SC_MPI_ERR_OTHER. */
#define sc_MPI_ERR_NO_MEM       SC3_MPI_ERR_NO_MEM      /**< Emulate \c SC_MPI_ERR_NO_MEM. */
#define sc_MPI_ERR_FILE         SC3_MPI_ERR_FILE        /**< Emulate \c SC_MPI_ERR_FILE. */
#define sc_MPI_ERR_NOT_SAME     SC3_MPI_ERR_NOT_SAME    /**< Emulate \c SC_MPI_ERR_NOT_SAME. */
#define sc_MPI_ERR_AMODE        SC3_MPI_ERR_AMODE       /**< Emulate \c SC_MPI_ERR_AMODE . */
/** Emulate \c SC_MPI_ERR_UNSUPPORTED_DATAREP. */
#define sc_MPI_ERR_UNSUPPORTED_DATAREP    SC3_MPI_ERR_UNSUPPORTED_DATAREP
/** Emulate \c SC_MPI_ERR_UNSUPPORTED_OPERATION. */
#define sc_MPI_ERR_UNSUPPORTED_OPERATION  SC3_MPI_ERR_UNSUPPORTED_OPERATION
/** Emulate \c SC_MPI_ERR_NO_SUCH_FILE. */
#define sc_MPI_ERR_NO_SUCH_FILE           SC3_MPI_ERR_NO_SUCH_FILE
#define sc_MPI_ERR_FILE_EXISTS  SC3_MPI_ERR_FILE_EXISTS /**< Emulate \c SC_MPI_ERR_FILE_EXISTS. */
#define sc_MPI_ERR_BAD_FILE     SC3_MPI_ERR_BAD_FILE    /**< Emulate \c SC_MPI_ERR_BAD_FILE. */
#define sc_MPI_ERR_ACCESS       SC3_MPI_ERR_ACCESS      /**< Emulate \c SC_MPI_ERR_ACCESS. */
#define sc_MPI_ERR_NO_SPACE     SC3_MPI_ERR_NO_SPACE    /**< Emulate \c SC_MPI_ERR_NO_SPACE. */
#define sc_MPI_ERR_QUOTA        SC3_MPI_ERR_QUOTA       /**< Emulate \c SC_MPI_ERR_QUOTA. */
#define sc_MPI_ERR_READ_ONLY    SC3_MPI_ERR_READ_ONLY   /**< Emulate \c SC_MPI_ERR_READ_ONLY. */
#define sc_MPI_ERR_FILE_IN_USE  SC3_MPI_ERR_FILE_IN_USE /**< Emulate \c SC_MPI_ERR_FILE_IN_USE. */
#define sc_MPI_ERR_DUP_DATAREP  SC3_MPI_ERR_DUP_DATAREP /**< Emulate \c SC_MPI_ERR_DUP_DATAREP. */
#define sc_MPI_ERR_CONVERSION   SC3_MPI_ERR_CONVERSION  /**< Emulate \c SC_MPI_ERR_CONVERSION. */
#define sc_MPI_ERR_IO           SC3_MPI_ERR_IO          /**< Emulate \c SC_MPI_ERR_IO. */
#define sc_MPI_ERR_LASTCODE     SC3_MPI_ERR_LASTCODE    /**< Emulate \c SC_MPI_ERR_LASTCODE. */

/** Emulate \c MPI_MAX_ERROR_STRING. */
#define sc_MPI_MAX_ERROR_STRING    SC3_MPI_MAX_ERROR_STRING

#define sc_MPI_COMM_NULL        SC3_MPI_COMM_NULL       /**< Emulate the null communicator. */
#define sc_MPI_COMM_WORLD       SC3_MPI_COMM_WORLD      /**< Emulate the world communicator. */
#define sc_MPI_COMM_SELF        SC3_MPI_COMM_SELF       /**< Emulate the self communicator. */

/** Emulate the null group.  Group operations are not supported without MPI. */
#define sc_MPI_GROUP_NULL          ((sc_MPI_Group) 0x54000000)  /* TODO change val */
/** Emulate the empty group.  Group operations are not supported without MPI. */
#define sc_MPI_GROUP_EMPTY         ((sc_MPI_Group) 0x54000001)  /* TODO change val */

/** \cond MPI_DOCUMENT_ALL */
#define sc_MPI_IDENT               (1)  /* TODO change val */
#define sc_MPI_CONGRUENT           (2)  /* TODO change val */
#define sc_MPI_SIMILAR             (3)  /* TODO change val */
#define sc_MPI_UNEQUAL             (-1) /* TODO change val */

#define sc_MPI_ANY_SOURCE          (-2)
#define sc_MPI_ANY_TAG             (-1)
#define sc_MPI_STATUS_IGNORE       (sc_MPI_Status *) 1
#define sc_MPI_STATUSES_IGNORE     (sc_MPI_Status *) 1

#define sc_MPI_REQUEST_NULL        ((sc_MPI_Request) 0x2c000000)
#define sc_MPI_DATATYPE_NULL       SC3_MPI_DATATYPE_NULL

#define sc_MPI_CHAR                ((sc_MPI_Datatype) 0x4c000101)
#define sc_MPI_SIGNED_CHAR         ((sc_MPI_Datatype) 0x4c000118)
#define sc_MPI_UNSIGNED_CHAR       ((sc_MPI_Datatype) 0x4c000102)
#define sc_MPI_BYTE                SC3_MPI_BYTE
#define sc_MPI_SHORT               ((sc_MPI_Datatype) 0x4c000203)
#define sc_MPI_UNSIGNED_SHORT      ((sc_MPI_Datatype) 0x4c000204)
#define sc_MPI_INT                 SC3_MPI_INT
#define sc_MPI_INT8_T              ((sc_MPI_Datatype) 0x4c000205)
#define sc_MPI_2INT                SC3_MPI_2INT
#define sc_MPI_UNSIGNED            SC3_MPI_UNSIGNED
#define sc_MPI_LONG                SC3_MPI_LONG
#define sc_MPI_UNSIGNED_LONG       ((sc_MPI_Datatype) 0x4c000408)
#define sc_MPI_LONG_LONG_INT       SC3_MPI_LONG_LONG
#define sc_MPI_UNSIGNED_LONG_LONG  ((sc_MPI_Datatype) 0x4c000409)
#define sc_MPI_FLOAT               SC3_MPI_FLOAT
#define sc_MPI_DOUBLE              SC3_MPI_DOUBLE
#define sc_MPI_DOUBLE_INT          SC3_MPI_DOUBLE_INT
#define sc_MPI_LONG_DOUBLE         ((sc_MPI_Datatype) 0x4c000c0c)
#define sc_MPI_PACKED              ((sc_MPI_Datatype) 0x4c001001)
#define sc_MPI_Aint                sc3_MPI_Aint_t

#define sc_MPI_OP_NULL             SC3_MPI_OP_NULL
#define sc_MPI_MINLOC              SC3_MPI_MINLOC
#define sc_MPI_MAXLOC              SC3_MPI_MAXLOC
#define sc_MPI_LOR                 SC3_MPI_LOR
#define sc_MPI_LAND                SC3_MPI_LAND
#define sc_MPI_LXOR                SC3_MPI_LXOR
#define sc_MPI_BOR                 SC3_MPI_BOR
#define sc_MPI_BAND                SC3_MPI_BAND
#define sc_MPI_BXOR                SC3_MPI_BXOR
#define sc_MPI_REPLACE             SC3_MPI_REPLACE
#define sc_MPI_PROD                SC3_MPI_PROD
/** \endcond  */

#define sc_MPI_MIN          SC3_MPI_MIN     /**< Minimum operator. */
#define sc_MPI_MAX          SC3_MPI_MAX     /**< Maximum operator. */
#define sc_MPI_SUM          SC3_MPI_SUM     /**< Summation operator. */

/** Emulate \c MPI_UNDEFINED. */
#define sc_MPI_UNDEFINED           SC3_MPI_UNDEFINED

/* types */

typedef int         sc_MPI_Group;       /**< Emulate an MPI group. */
typedef int         sc_MPI_Request;     /**< Emulate an MPI request. */
typedef sc3_MPI_Comm_t sc_MPI_Comm;     /**< Emulate an MPI communicator. */
typedef sc3_MPI_Info_t sc_MPI_Info;     /**< Emulate an MPI Info object. */
typedef sc3_MPI_Datatype_t sc_MPI_Datatype;     /**< Emulate MPI datatypes. */
typedef sc3_MPI_Op_t sc_MPI_Op;                 /**< Emulate MPI operations. */

/** Replacement of \c MPI_Status for non-MPI configuration. */
typedef struct sc_MPI_Status
{
  int                 count;            /**< Bytes sent/received. */
  int                 cancelled;        /**< Cancellation occurred. */
  int                 MPI_SOURCE;       /**< Rank of the source. */
  int                 MPI_TAG;          /**< Tag of the message. */
  int                 MPI_ERROR;        /**< Error occurred. */
}
sc_MPI_Status;

/* MPI info arguments */

#define sc_MPI_INFO_NULL           NULL     /**< Emulate null Info */

/* These functions are valid and functional for a single process. */

/** MPI initialization.
 * \param [in,out] argc     Command line argument count.
 * \param [in,out] argv     Command line arguments.
 */
int                 sc_MPI_Init (int *argc, char ***argv);

/** MPI finalization. */
int                 sc_MPI_Finalize (void);

/** Abort an MPI program.
 * \param [in] mpicomm      Communicator across which to abort.
 * \param [in] ecode        Error code returned to the system.
 */
int                 sc_MPI_Abort (sc_MPI_Comm mpicomm, int ecode)
  __attribute__ ((noreturn));

/** Duplicate an MPI communicator.
 * \param [in] mpicomm      Communicator to duplicate.
 * \param [out] dupcomm     Duplicated communicator.
 */
int                 sc_MPI_Comm_dup (sc_MPI_Comm mpicomm,
                                     sc_MPI_Comm *dupcomm);

/** Free a previously created MPI communicator.
 * \param [out] freecomm    Communicator to free.
 */
int                 sc_MPI_Comm_free (sc_MPI_Comm *freecomm);

/** Return size of an MPI datatype.
 * \param [in] datatype     Valid MPI datatype.
 * \param [out] size        Size of MPI datatype in bytes.
 * \return                  MPI_SUCCESS on success.
 */
int                 sc_MPI_Type_size (sc_MPI_Datatype datatype, int *size);

/** Pack several instances of the same datatype into contiguous memory.
 * \param [in] inbuf          Buffer of elements of type \b datatype.
 * \param [in] incount        Number of elements in \b inbuf.
 * \param [in] datatype       Datatype of elements in \b inbuf.
 * \param [out] outbuf        Output buffer in which elements are packed.
 * \param [in] outsize        Size of output buffer in bytes.
 * \param [in, out] position  The current position in the output buffer.
 * \param [in] comm           Valid MPI communicator.
 * \return                    MPI_SUCCESS on success.
 */
int                 sc_MPI_Pack (const void *inbuf, int incount,
                                 sc_MPI_Datatype datatype, void *outbuf,
                                 int outsize, int *position,
                                 sc_MPI_Comm comm);

/** Unpack contiguous memory into several instances of the same datatype.
 * \param [in] inbuf          Buffer of packed data.
 * \param [in] insize         Number of bytes in \b inbuf
 * \param [in, out] position  The current position in the input buffer.
 * \param [out] outbuf        Output buffer in which elements are unpacked.
 * \param [in] outcount       Number of elements to unpack.
 * \param [in] datatype       Datatype of elements to be unpacked.
 * \param [in] comm           Valid MPI communicator.
 * \return                    MPI_SUCCESS on success.
 */
int                 sc_MPI_Unpack (const void *inbuf, int insize,
                                   int *position, void *outbuf, int outcount,
                                   sc_MPI_Datatype datatype,
                                   sc_MPI_Comm comm);

/** Determine space needed to pack several instances of the same datatype.
 * \param [in] incount        Number of elements to pack.
 * \param [in] datatype       Datatype of elements to pack.
 * \param [in] comm           Valid MPI communicator.
 * \param [out] size          Number of bytes needed to packed \b incount
 *                            instances of \b datatype.
 * \return                    MPI_SUCCESS on success.
 */
int                 sc_MPI_Pack_size (int incount, sc_MPI_Datatype datatype,
                                      sc_MPI_Comm comm, int *size);

/** Query size of an MPI communicator.
 * \param [in] mpicomm      Valid MPI communicator.
 * \param [out] mpisize     Without MPI this is always 1.
 */
int                 sc_MPI_Comm_size (sc_MPI_Comm mpicomm, int *mpisize);

/** Query rank of an MPI process within a communicator.
 * \param [in] mpicomm      Valid MPI communicator.
 * \param [out] mpirank     Without MPI this is always 0.
 */
int                 sc_MPI_Comm_rank (sc_MPI_Comm mpicomm, int *mpirank);

/** Query size of an MPI group.
 * \param [in] mpigroup     Valid MPI group.
 * \param [out] size        Without MPI this is always 1.
 */
int                 sc_MPI_Group_size (sc_MPI_Group mpigroup, int *size);

/** Query rank of an MPI process within a group.
 * \param [in] mpigroup     Valid MPI group.
 * \param [out] rank        Without MPI this is always 0.
 */
int                 sc_MPI_Group_rank (sc_MPI_Group mpigroup, int *rank);

/** Execute a parallel barrier.
 * \param [in] mpicomm      Valid communicator.
 */
int                 sc_MPI_Barrier (sc_MPI_Comm mpicomm);

/** Execute the MPI_Bcast algorithm. */
int                 sc_MPI_Bcast (void *, int, sc_MPI_Datatype, int,
                                  sc_MPI_Comm);

int                 sc_MPI_Gather (void *, int, sc_MPI_Datatype, void *, int,
                                   sc_MPI_Datatype, int, sc_MPI_Comm);
int                 sc_MPI_Gatherv (void *, int, sc_MPI_Datatype, void *,
                                    int *, int *, sc_MPI_Datatype, int,
                                    sc_MPI_Comm);

/** Execute the MPI_Allgather algorithm. */
int                 sc_MPI_Allgather (void *, int, sc_MPI_Datatype, void *,
                                      int, sc_MPI_Datatype, sc_MPI_Comm);

/** Execute the MPI_Allgatherv algorithm. */
int                 sc_MPI_Allgatherv (void *, int, sc_MPI_Datatype, void *,
                                       int *, int *, sc_MPI_Datatype,
                                       sc_MPI_Comm);

/** Execute the MPI_Alltoall algorithm. */
int                 sc_MPI_Alltoall (void *, int, sc_MPI_Datatype, void *,
                                     int, sc_MPI_Datatype, sc_MPI_Comm);

/** Execute the MPI_Reduce algorithm. */
int                 sc_MPI_Reduce (void *, void *, int, sc_MPI_Datatype,
                                   sc_MPI_Op, int, sc_MPI_Comm);

int                 sc_MPI_Reduce_scatter_block (void *, void *,
                                                 int, sc_MPI_Datatype,
                                                 sc_MPI_Op, sc_MPI_Comm);

/** Execute the MPI_Allreduce algorithm. */
int                 sc_MPI_Allreduce (void *, void *, int, sc_MPI_Datatype,
                                      sc_MPI_Op, sc_MPI_Comm);

/** Execute the MPI_Scan algorithm. */
int                 sc_MPI_Scan (void *, void *, int, sc_MPI_Datatype,
                                 sc_MPI_Op, sc_MPI_Comm);

/** Execute the MPI_Exscan algorithm. */
int                 sc_MPI_Exscan (void *, void *, int, sc_MPI_Datatype,
                                   sc_MPI_Op, sc_MPI_Comm);

/* Replacement by standard subtraction.
 *
 * MPI 2.0 supports MPI_Aint but does not support MPI_Aint_diff.
 * In the MPI 2.0 standard document
 * (https://www.mpi-forum.org/docs/mpi-2.0/mpi2-report.pdf) on page 283 is
 * an example of calculating MPI_Aint displacements by standard subtraction.
 * Therefore, we also use standard subtraction in the case MPI_Aint_diff is
 * not available.
 */
sc_MPI_Aint         sc_MPI_Aint_diff (sc_MPI_Aint a, sc_MPI_Aint b);

/** Execute the MPI_Wtime function.
 * \return          Number of seconds since the epoch. */
double              sc_MPI_Wtime (void);

/** Return the input communicator in lieu of splitting.
 * \return          MPI_SUCCESS.
 */
int                 sc_MPI_Comm_split (sc_MPI_Comm, int, int, sc_MPI_Comm *);

/* These functions will run but their results/actions are not defined. */

int                 sc_MPI_Comm_create (sc_MPI_Comm, sc_MPI_Group,
                                        sc_MPI_Comm *);
int                 sc_MPI_Comm_compare (sc_MPI_Comm, sc_MPI_Comm, int *);
int                 sc_MPI_Comm_group (sc_MPI_Comm, sc_MPI_Group *);

int                 sc_MPI_Group_free (sc_MPI_Group *);
int                 sc_MPI_Group_translate_ranks (sc_MPI_Group, int, int *,
                                                  sc_MPI_Group, int *);
int                 sc_MPI_Group_compare (sc_MPI_Group, sc_MPI_Group, int *);
int                 sc_MPI_Group_union (sc_MPI_Group, sc_MPI_Group,
                                        sc_MPI_Group *);
int                 sc_MPI_Group_intersection (sc_MPI_Group, sc_MPI_Group,
                                               sc_MPI_Group *);
int                 sc_MPI_Group_difference (sc_MPI_Group, sc_MPI_Group,
                                             sc_MPI_Group *);
int                 sc_MPI_Group_incl (sc_MPI_Group, int, int *,
                                       sc_MPI_Group *);
int                 sc_MPI_Group_excl (sc_MPI_Group, int, int *,
                                       sc_MPI_Group *);
int                 sc_MPI_Group_range_incl (sc_MPI_Group, int,
                                             int ranges[][3], sc_MPI_Group *);
int                 sc_MPI_Group_range_excl (sc_MPI_Group, int,
                                             int ranges[][3], sc_MPI_Group *);

/* These functions will abort. */

int                 sc_MPI_Recv (void *, int, sc_MPI_Datatype, int, int,
                                 sc_MPI_Comm, sc_MPI_Status *);
int                 sc_MPI_Irecv (void *, int, sc_MPI_Datatype, int, int,
                                  sc_MPI_Comm, sc_MPI_Request *);
int                 sc_MPI_Send (void *, int, sc_MPI_Datatype, int, int,
                                 sc_MPI_Comm);
int                 sc_MPI_Isend (void *, int, sc_MPI_Datatype, int, int,
                                  sc_MPI_Comm, sc_MPI_Request *);
int                 sc_MPI_Probe (int, int, sc_MPI_Comm, sc_MPI_Status *);
int                 sc_MPI_Iprobe (int, int, sc_MPI_Comm, int *,
                                   sc_MPI_Status *);
int                 sc_MPI_Get_count (sc_MPI_Status *, sc_MPI_Datatype,
                                      int *);

/* This function is only allowed to be called with NULL requests. */

int                 sc_MPI_Wait (sc_MPI_Request *, sc_MPI_Status *);

#endif /* !SC_ENABLE_MPI */

/* Without SC_ENABLE_MPI, only allowed to be called with NULL requests. */

int                 sc_MPI_Waitsome (int, sc_MPI_Request *,
                                     int *, int *, sc_MPI_Status *);
int                 sc_MPI_Waitall (int, sc_MPI_Request *, sc_MPI_Status *);
int                 sc_MPI_Testall (int, sc_MPI_Request *, int *,
                                    sc_MPI_Status *);

#if defined SC_ENABLE_MPI && defined SC_ENABLE_MPITHREAD

#define sc_MPI_THREAD_SINGLE       MPI_THREAD_SINGLE
#define sc_MPI_THREAD_FUNNELED     MPI_THREAD_FUNNELED
#define sc_MPI_THREAD_SERIALIZED   MPI_THREAD_SERIALIZED
#define sc_MPI_THREAD_MULTIPLE     MPI_THREAD_MULTIPLE

#define sc_MPI_Init_thread         MPI_Init_thread

#else

/** \cond MPI_DOCUMENT_ALL */
#define sc_MPI_THREAD_SINGLE       0
#define sc_MPI_THREAD_FUNNELED     1
#define sc_MPI_THREAD_SERIALIZED   2
#define sc_MPI_THREAD_MULTIPLE     3
/** \endcond */

int                 sc_MPI_Init_thread (int *argc, char ***argv,
                                        int required, int *provided);

#endif /* !(SC_ENABLE_MPI && SC_ENABLE_MPITHREAD) */

#ifdef SC_ENABLE_MPIIO

/* file access modes */

#define sc_MPI_MODE_RDONLY         MPI_MODE_RDONLY
#define sc_MPI_MODE_RDWR           MPI_MODE_RDWR
#define sc_MPI_MODE_WRONLY         MPI_MODE_WRONLY
#define sc_MPI_MODE_CREATE         MPI_MODE_CREATE
#define sc_MPI_MODE_EXCL           MPI_MODE_EXCL
#define sc_MPI_MODE_DELETE_ON_CLOSE MPI_MODE_DELETE_ON_CLOSE
#define sc_MPI_MODE_UNIQUE_OPEN    MPI_MODE_UNIQUE_OPEN
#define sc_MPI_MODE_SEQUENTIAL     MPI_MODE_SEQUENTIAL
#define sc_MPI_MODE_APPEND         MPI_MODE_APPEND

/* file seek parameters */

#define sc_MPI_SEEK_SET            MPI_SEEK_SET
#define sc_MPI_SEEK_CUR            MPI_SEEK_CUR
#define sc_MPI_SEEK_END            MPI_SEEK_END

/* MPI I/O related types and functions */

#define sc_MPI_Offset              MPI_Offset

#define sc_MPI_File                MPI_File
#define sc_MPI_FILE_NULL           MPI_FILE_NULL

#define sc_MPI_File_open           MPI_File_open
#define sc_MPI_File_close          MPI_File_close

#define sc_MPI_File_get_view       MPI_File_get_view
#define sc_MPI_File_set_view       MPI_File_set_view

#define sc_MPI_File_write_all      MPI_File_write_all
#define sc_MPI_File_read_all       MPI_File_read_all

#define sc_MPI_File_write_at_all   MPI_File_write_at_all
#define sc_MPI_File_read_at_all    MPI_File_read_at_all

#define sc_MPI_File_get_size       MPI_File_get_size
#define sc_MPI_File_set_size       MPI_File_set_size

#else

typedef long        sc_MPI_Offset;      /**< Emulate the MPI offset type. */

/** Replacement structure for \c MPI_File.
 * When MPI I/O is not enabled, this is used as file object.
 */
struct sc_no_mpiio_file
{
  sc_MPI_Comm         mpicomm;          /**< The MPI communicator. */
  const char         *filename;         /**< Name of the file. */
  FILE               *file;             /**< Underlying file object. */
  int                 mpisize;          /**< Ranks in communicator. */
  int                 mpirank;          /**< Rank of this process. */
};

/** Replacement object for an MPI file. */
typedef struct sc_no_mpiio_file *sc_MPI_File;

#define sc_MPI_FILE_NULL           NULL     /**< The null MPI file. */

#endif /* !SC_ENABLE_MPIIO */

/** Turn an MPI error code into its error class.
 * When MPI is enabled, we pass version 1.1 errors to MPI_Error_class.
 * When MPI I/O is not enabled, we process file errors outside of MPI.
 * Thus, within libsc, it is always legal to call this function with
 * any errorcode defined above in this header file.
 *
 * \param [in] errorcode        Returned from a direct MPI call or libsc.
 * \param [out] errorclass      Non-NULL pointer.  Filled with matching
 *                              error class on success.
 * \return                      sc_MPI_SUCCESS on successful conversion,
 *                              Other MPI error code otherwise.
 */
int                 sc_MPI_Error_class (int errorcode, int *errorclass);

/** Turn MPI error code into a string.
 * \param [in] errorcode        This (MPI) error code is converted.
 * \param [in,out] string       At least sc_MPI_MAX_ERROR_STRING bytes.
 * \param [out] resultlen       Length of string on return.
 * \return                      sc_MPI_SUCCESS on success or
 *                              other MPI error cocde on invalid arguments.
 */
int                 sc_MPI_Error_string (int errorcode, char *string,
                                         int *resultlen);

/** Return the size of MPI datatypes.
 * \param [in] t    MPI datatype.
 * \return          Returns the size in bytes.
 */
size_t              sc_mpi_sizeof (sc_MPI_Datatype t);

/** \cond MPI_DOCUMENT_NODE_COMM */

/** Compute ``sc_intranode_comm'' and ``sc_internode_comm''
 * communicators and attach them to the current communicator.  This split
 * takes \a processes_per_node passed by the user at face value: there is no
 * hardware checking to see if this is the true affinity.
 *
 * This function does nothing if MPI_Comm_split_type is not found.
 *
 * \param [in/out] comm                 MPI communicator
 * \param [in]     processes_per_node   the size of the intranode
 *                                      communicators. if < 1,
 *                                      sc will try to determine the correct
 *                                      shared memory communicators.
 */
void                sc_mpi_comm_attach_node_comms (sc_MPI_Comm comm,
                                                   int processes_per_node);

/** Destroy ``sc_intranode_comm'' and ``sc_internode_comm''
 * communicators that are stored as attributes to communicator ``comm''.
 * This routine enforces a call to the destroy callback for these attributes.
 *
 * This function does nothing if MPI_Comm_split_type is not found.
 *
 * \param [in/out] comm                 MPI communicator
 */
void                sc_mpi_comm_detach_node_comms (sc_MPI_Comm comm);

/** Get the communicators computed in sc_mpi_comm_attach_node_comms() if they
 * exist; return sc_MPI_COMM_NULL otherwise.
 *
 * \param[in] comm            Super communicator
 * \param[out] intranode      intranode communicator
 * \param[out] internode      internode communicator
 */
void                sc_mpi_comm_get_node_comms (sc_MPI_Comm comm,
                                                sc_MPI_Comm * intranode,
                                                sc_MPI_Comm * internode);

/** Convenience function to get a node comm and attach it as an attribute.
 * \param [in,out] comm       As in \ref sc_mpu_comm_attach_node_comms.
 * \return                    If the intranode communicator cannot be
 *                            obtained, return 0.
 *                            Otherwise return size of intranode communicator.
 */
int                 sc_mpi_comm_get_and_attach (sc_MPI_Comm comm);

/** \endcond */

SC_EXTERN_C_END;

#endif /* !SC_MPI_H */
