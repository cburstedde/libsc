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
 * We use a minimal reference counting mechanism declared in \ref
 * sc3_refcount.h.  While the \ref sc3_refcount_t object is public, most
 * objects in \ref sc3 are opaque and manipulated by API functions only.
 *
 * Allocators are declared in \ref sc3_alloc.h, and errors in \ref sc3_error.h.
 * These two files depend on each other and, combined with the reference
 * counters, constitute the basis of the code.
 *
 * Containers are declared in \ref sc3_array.h.
 *
 * We use the C convention for boolean variables:
 * They are of type `int` and false if and only of their value is 0.
 * An application must be careful to interpret any non-negative number as true.
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
 * - An object that is setup may be queried for properties set with
 *   sc3_object_get_*.
 *   These functions are generally tolerant to NULL output arguments.
 * - When removing an object's last reference with `sc3_object_unref`, it is
 *   deallocated together with all its internal resources.
 *   When calling unref, do not assume anything about the input reference
 *   count since other references may have been created and passed around.
 *   Likewise, it is generally unsafe to rely on the inout argument to unref,
 *   and best to consider an unrefd pointer to be no longer valid.
 * - The `sc3_object_destroy` functions must only be called if the program
 *   can guarantee that at this point the reference count is 1.
 *   In some situations, when we do not pass references to the object to
 *   other parts of the code, this is easy to enforce.
 *   Otherwise, it may be best not to use destroy and rely on unref entirely.
 * - There is no means to query the present reference count of an object.
 *   This is on purpose:  The whole point of reference counting is that
 *   different references to the object can be used independently.
 *   Again, this means that destroy functions should only be used with reason.
 * - The `sc3_object_is_*` functions implement queries that may be called for
 *   any pointer to this object type, including `NULL`, at any stage.
 *   They query properties of the object and always return false if the pointer
 *   is `NULL`.
 *   One convention is to generally provide for `sc3_object_is_new`,
 *   `sc3_object_is_setup` and `sc3_object_is_valid`, plus more as needed.
 *   The first two refer to the mutually exclusive phases of its lifetime,
 *   while the third may be used anytime to verify internal consistency.
 */

/** \defgroup sc3_internal sc3 internal
 * We keep some declarations for internal library use in separate files.
 * Applications may use them, but this is not usually recommended.
 */

/** \file sc3_base.h \ingroup sc3
 * This file includes configuration definitions and
 * provides some generic helper functions.
 *
 * It is included by all other sc3 header files, thus usually not needed
 * to be explicitly included.
 * When starting new and independent functionality however, include this file
 * first.
 */

#ifndef SC3_BASE_H
#define SC3_BASE_H

#include <sc3_config.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** This macro is usable as a no-operation statement. */
#define SC3_NOOP do { ; } while (0)

/** The number of bits in an int variable on this architecture. */
#define SC3_INT_BITS (8 * SC_SIZEOF_INT)

/** The highest power of two representable in an int variable. */
#define SC3_INT_HPOW (1 << (SC3_INT_BITS - 2))

/** Standard buffer size for string handling in the library. */
#define SC3_BUFSIZE 512

/** Set a buffer of standard size to all zeros. */
#define SC3_BUFZERO(b) do { memset (b, 0, SC3_BUFSIZE); } while (0)

/** Cope a null-terminated string into a buffer of standard size.*/
#define SC3_BUFCOPY(b,s) \
          do { snprintf (b, SC3_BUFSIZE, "%s", s); } while (0)

/** Type-safe wrapper of the stdlib malloc function. */
#define SC3_MALLOC(typ,nmemb) ((typ *) malloc ((nmemb) * sizeof (typ)))

/** Type-safe wrapper of the stdlib calloc function. */
#define SC3_CALLOC(typ,nmemb) ((typ *) calloc (nmemb, sizeof (typ)))

/** Wrap the stdlib free function for consistency. */
#define SC3_FREE(p) (free (p))

/** Type-safe wrapper of the stdlib realloc function. */
#define SC3_REALLOC(p,typ,nmemb) ((typ *) realloc (p, (nmemb) * sizeof (typ)))

/** Return whether a non-negative integer is a power of two. */
#define SC3_ISPOWOF2(a) ((a) > 0 && ((a) & ((a) - 1)) == 0)

/** Return the minimum of two values. */
#define SC3_MIN(in1,in2) ((in1) <= (in2) ? (in1) : (in2))

/** Return the maximum of two values. */
#define SC3_MAX(in1,in2) ((in1) >= (in2) ? (in1) : (in2))

/** Turn a size_t variable into an integer,
    unless it is too large and we return -1. */
#define SC3_SIZET_INT(s) ((s) <= INT_MAX ? (int) s : -1)

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** Provide a string copy function.
 * \param [out] dest    Buffer of length at least \a size.
 *                      On output, not touched if NULL or \a size == 0.
 * \param [in] size     Allocation length of \a dest.
 * \param [in] src      Null-terminated string.
 * \return              Equivalent to \ref
 *                      sc3_snprintf (dest, size, "%s", src).
 */
void                sc3_strcopy (char *dest, size_t size, const char *src);

/** Wrap the system snprintf function, allowing for truncation.
 * The snprintf function may truncate the string written to the specified length.
 * In some cases, compilers warn when this may occur.
 * For the usage in sc3, this is permitted behavior and we avoid the warning.
 * \param [out] str     Buffer of length at least \a size.
 *                      On output, not touched if NULL or \a size == 0.
 *                      Otherwise, "" on snprintf error or the proper result.
 * \param [in] size     Allocation length of \a str.
 * \param [in] format   Format string as in man (3) snprintf.
 */
void                sc3_snprintf (char *str, size_t size,
                                  const char *format, ...)
  __attribute__((format (printf, 3, 4)));

/** Determine the highest bit position of a positive integer.
 * \param [in] a, bits  The lowest *bits* bits of \a a are examined.
 *                      Higher bits are silently assumed to be zero.
 *                      \a a and \a bits are assumed positive.
 * \return              We return the zero-based position from the right
 *                      of the highest 1-bit of \a a,
 *                      or -1 if \a a or \a bits are zero or negative.
 */
int                 sc3_highbit (int a, int bits);

/** Return the base-2 logarithm of an integer rounded up.
 * \param [in] a, bits  The lowest *bits* bits of \a a are examined.
 *                      Higher bits are silently assumed to be zero.
 *                      \a a and \a bits are assumed positive.
 * \return              The rounded-up binary logarithm of
 *                      the first \a bits of the argument \a a.
 */
int                 sc3_log2_ceil (int a, int bits);

/** Fast algorithm to compute integer exponentials.
 * Compute \a base to the power of \a exp.
 * \param [in] base     This number can be arbitrary.
 * \param [in] exp      If negative, the function returns 0.
 * \return \a base ** \a exp except 0 when the exponent is -1.
 */
int                 sc3_intpow (int base, int exp);

/** Fast algorithm to compute integer exponentials.
 * Compute \a base to the power of \a exp.
 * \param [in] base     This number can be arbitrary.
 * \param [in] exp      If negative, the function returns 0.
 * \return \a base ** \a exp except 0 when the exponent is -1.
 */
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

/** Extract the basename of a path.
 * This function uses the system's `basename` function if available
 * and falls back to returning the input string otherwise.
 * If the input string is NULL or empty, return ".".
 * This function is thread safe if the system baseline is.
 * \param [in,out] path   If this is NULL, the function returns ".".
 *                        Otherwise, it must be a null-terminated string
 *                        that may be modified by this function.
 * \return                Pointer to statically allocated memory,
 *                        overwritten by subsequent calls.
 */
char               *sc3_basename (char *path);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_BASE_H */
