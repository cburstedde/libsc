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

/** \defgroup sc3 sc3
 * Version 3 of the sc library is a rewrite.
 *
 * The main goals compared to previous versions are to improve the error
 * handling and to remove any global/static variables.
 * In particular, functions do no longer call `MPI_Abort()` on failed
 * assertions but return error objects.
 * These error objects can be returned all the way up to the toplevel code.
 * We have also introduced RAII objects and reference counting.
 * Memory allocation is wrapped into allocators that keep internal counts.
 * We do not provide any locking, thus it is recommended to create one
 * allocator per thread, and generally to work with a hierarchy of allocators.
 * We provide various data containers, for example variable-length arrays.
 *
 * There is no `sc3.h` file to include everything.
 * To just pull in the configuration and system headers, include \ref
 * sc3_base.h.
 * We removed the global initialization and finalize functions.
 * There is no more global state to the library.
 *
 * We use a minimal reference counting mechanism declared in
 * \ref sc3_refcount.h and \ref sc3_refcount_internal.h.
 * (Internal header files are not part of the public API but allow access to
 * lower levels of the code.)  While the \ref sc3_refcount_t object is public,
 * most objects in sc3 are opaque and manipulated by API functions only.
 *
 * Allocators are declared in \ref sc3_alloc.h, and errors in \ref sc3_error.h.
 * These two files depend on each other and, combined with the reference
 * counters, constitute the basis of the code.
 *
 * Containers are declared in \ref sc3_array.h.
 *
 * The design we provide is not meant to reinvent object oriented programming.
 * It is a set of minimal conventions that establish a consistent call pattern
 * of the code.
 * - An object begins with its life with the `sc3_object_new` call and a
 *   reference count of 1.
 *   These functions usually take an allocator input parameter that is
 *   used to allocate the object and all future resources inside of it.
 * - After the `new` call, properties of the object may be specified by
 *   `sc3_object_set_*` functions.
 *   It is fine to call the same set function multiple times or to temporarily
 *   set conflicting values.
 *   In this stage, the object cannot yet be referenced, but it can be
 *   unreferenced and destroyed (which amount to the same result here).
 * - After setting a consistent set of parameters, call `sc3_object_setup`.
 *   This creates a usable object that can no longer be reparameterized,
 *   thus it is immutable and safe to be referenced and passed to other code.
 *   There may be exceptions to immutability, for example an array may be
 *   resized after setup, but the size of each element may not be changed.
 * - When removing an object's last reference with `sc3_object_unref`, it is
 *   deallocated together with all its internal resources.
 *   The `sc3_object_destroy` functions must only be called if the program
 *   ensures that at this point the reference count is 1.
 * - The `sc3_object_is_*` functions may be called on any pointer to this
 *   object type including `NULL`.
 *   They query properties of the object that are false if the pointer is
 *   `NULL`.
 *   One convention is to provide for `sc3_object_is_new`,
 *   `sc3_object_is_setup` and `sc3_object_is_valid`.
 *   The first two refer to the mutually exclusive phases of its life,
 *   while the third may be used any time to verify internal consistency.
 */

/** \file sc3_base.h
 * \ingroup sc3
 */

#ifndef SC3_BASE_H
#define SC3_BASE_H

#include <sc3_config.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SC3_NOOP do { ; } while (0)

#define SC3_INT_BITS (8 * SC_SIZEOF_INT)
#define SC3_INT_HPOW (1 << (SC3_INT_BITS - 2))

#define SC3_BUFSIZE 512
#define SC3_BUFZERO(b) do { memset (b, 0, SC3_BUFSIZE); } while (0)
#define SC3_BUFCOPY(b,s) \
          do { snprintf (b, SC3_BUFSIZE, "%s", s); } while (0)

#define SC3_MALLOC(typ,nmemb) ((typ *) malloc ((nmemb) * sizeof (typ)))
#define SC3_CALLOC(typ,nmemb) ((typ *) calloc (nmemb, sizeof (typ)))
#define SC3_FREE(p) do { free (p); } while (0)
#define SC3_REALLOC(p,typ,nmemb) ((typ *) realloc (p, (nmemb) * sizeof (typ)))

#define SC3_ISPOWOF2(a) ((a) > 0 && ((a) & ((a) - 1)) == 0)

#define SC3_MIN(in1,in2) ((in1) <= (in2) ? (in1) : (in2))
#define SC3_MAX(in1,in2) ((in1) >= (in2) ? (in1) : (in2))

#define SC3_SIZET_INT(s) ((s) <= INT_MAX ? (int) s : -1)

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

void                sc3_strcopy (char *dest, size_t size, const char *src);

void                sc3_snprintf (char *str, size_t size,
                                  const char *format, ...)
  __attribute__((format (printf, 3, 4)));

/** Determine the highest bit position of a positive integer.
 * \param [in] a, bits  The lowest *bits* bits of *a* are examined.
 *                      Higher bits are silently assumed to be zero.
 *                      *a* and *bits* must be positive.
 * \return              We return the zero-based position from the right
 *                      of the highest 1-bit of *a*,
 *                      or -1 if *a* or *bits* non-negative.
 */
int                 sc3_highbit (int a, int bits);
int                 sc3_log2_ceil (int a, int bits);

int                 sc3_intpow (int base, int exp);
long                sc3_longpow (long base, int exp);

/** Compute a cumulative partition cut by floor (Np / P).
 * The product NP must fit into a long integer.
 * For p <= 0 we return 0 and for p >= P we return N.
 * \param [in] N       Non-negative integer to divide between P slots.
 * \param [in] P       The total number of slots, a positive integer.
 * \param [in] p       Slot number is trimmed to satisfy 0 <= p <= P.
 * \return             floor (Np / P) in long integer arithmetic or
 *                     0 if any argument is invalid.
 */
int                 sc3_intcut (int N, int P, int p);

/** Compute a cumulative partition cut by floor (Np / P).
 * The product NP must fit into a long integer.
 * For p <= 0 we return 0 and for p >= P we return N.
 * \param [in] N       Non-negative integer to divide between P slots.
 * \param [in] P       The total number of slots, a positive integer.
 * \param [in] p       Slot number is trimmed to satisfy 0 <= p <= P.
 * \return             floor (Np / P) in long integer arithmetic or
 *                     0 if any argument is invalid.
 */
long                sc3_longcut (long N, int P, int p);

char               *sc3_basename (char *path);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_BASE_H */
