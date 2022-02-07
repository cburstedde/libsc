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

#include <sc3_error.h>
#include <sc3_refcount.h>

struct sc3_error
{
  sc3_refcount_t      rc;
  int                 setup;

  sc3_error_kind_t    kind;
#if 0
  sc3_error_sync_t    syn;
#endif
  char                errmsg[SC3_BUFSIZE];
  char                filename[SC3_BUFSIZE];
  int                 line;
  int                 alloced;
  sc3_error_t        *stack;

  int                 accessed_locations;
  int                 accessed_messages;
};

const char          sc3_error_kind_char[SC3_ERROR_KIND_LAST] =
  { 'F', 'A', 'L', 'R', 'M', 'N', 'U' };

static sc3_error_t  enomem = {
  {SC3_REFCOUNT_MAGIC, 1}, 1, SC3_ERROR_MEMORY,
  "Out of memory", __FILE__, __LINE__, 0, NULL, 0, 0
};

sc3_error_t        *
sc3_strdup (const char *src, char **dest)
{
  char               *s;

  SC3A_CHECK (src != NULL);
  SC3A_CHECK (dest != NULL);

  if ((s = strdup (src)) == NULL) {
    return &enomem;
  }

  *dest = s;
  return NULL;
}

sc3_error_t        *
sc3_malloc (size_t size, void *pmem)
{
  void               *p;

  SC3A_CHECK (pmem != NULL);

  p = malloc (size);
  if (size != 0 && p == NULL) {
    return &enomem;
  }

  *(void **) pmem = p;
  return NULL;
}

sc3_error_t        *
sc3_calloc (size_t nmemb, size_t size, void *pmem)
{
  void               *p;

  SC3A_CHECK (pmem != NULL);

  p = calloc (nmemb, size);
  if (nmemb != 0 && size != 0 && p == NULL) {
    return &enomem;
  }

  *(void **) pmem = p;
  return NULL;
}

sc3_error_t        *
sc3_realloc (void *pmem, size_t size)
{
  void               *p;

  SC3A_CHECK (pmem != NULL);

  p = realloc (*(void **) pmem, size);
  if (size != 0 && p == NULL) {
    return &enomem;
  }

  *(void **) pmem = p;
  return NULL;
}

sc3_error_t        *
sc3_free (void *pmem)
{
  SC3A_CHECK (pmem != NULL);

  free (*(void **) pmem);
  *(void **) pmem = NULL;
  return NULL;
}

int
sc3_error_is_valid (const sc3_error_t * e, char *reason)
{
  SC3E_TEST (e != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &e->rc, reason);
  if (e->stack != NULL) {
    SC3E_IS (sc3_error_is_setup, e->stack, reason);
  }
  SC3E_TEST (0 <= e->kind && e->kind < SC3_ERROR_KIND_LAST, reason);
#if 0
  SC3E_TEST (0 <= e->syn && e->syn < SC3_ERROR_SYNC_LAST, reason);
#endif
  SC3E_TEST (0 <= e->accessed_locations, reason);
  SC3E_TEST (0 <= e->accessed_messages, reason);
  SC3E_YES (reason);
}

int
sc3_error_is_new (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_valid, e, reason);
  SC3E_TEST (!e->setup, reason);
  SC3E_YES (reason);
}

int
sc3_error_is_setup (const sc3_error_t * e, char *reason)
{
  SC3E_IS (sc3_error_is_valid, e, reason);
  SC3E_TEST (e->setup, reason);
  SC3E_YES (reason);
}

int
sc3_error_is2_kind (const sc3_error_t * e, sc3_error_kind_t kind,
                    char *reason)
{
  SC3E_IS (sc3_error_is_setup, e, reason);
  SC3E_TEST (e->kind == kind, reason);
  SC3E_YES (reason);
}

static void
sc3_error_defaults (sc3_error_t * e, sc3_error_t * stack,
                    int setup, int inherit)
{
  memset (e, 0, sizeof (sc3_error_t));
  sc3_refcount_init (&e->rc);
  e->setup = setup;

  e->kind = SC3_ERROR_FATAL;
#if 0
  e->syn = SC3_ERROR_SYNC_LAST;
#endif
  e->alloced = 1;
  e->stack = stack;
  if (inherit && stack != NULL) {
    e->kind = stack->kind;
  }
}

sc3_error_t        *
sc3_error_new (sc3_error_t ** ep)
{
  sc3_error_t        *e;

  SC3E_RETVAL (ep, NULL);

  SC3E (sc3_malloc (sizeof (sc3_error_t), &e));
  sc3_error_defaults (e, NULL, 0, 0);
  SC3A_IS (sc3_error_is_new, e);

  *ep = e;
  return NULL;
}

sc3_error_t        *
sc3_error_set_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  sc3_error_t        *stack;

  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (pstack != NULL);
  if ((stack = *pstack) != NULL) {
    *pstack = NULL;
    SC3A_IS (sc3_error_is_setup, stack);
  }
  if (e->stack != NULL) {
    SC3E (sc3_error_unref (&e->stack));
  }
  e->stack = stack;
  return NULL;
}

sc3_error_t        *
sc3_error_set_location (sc3_error_t * e, const char *filename, int line)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (filename != NULL);

  SC3_BUFCOPY (e->filename, filename);
  e->line = line;
  return NULL;
}

sc3_error_t        *
sc3_error_set_message (sc3_error_t * e, const char *errmsg)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (errmsg != NULL);

  SC3_BUFCOPY (e->errmsg, errmsg);
  return NULL;
}

sc3_error_t        *
sc3_error_set_messagef (sc3_error_t * e, const char *fmt, ...)
{
  va_list             ap;

  va_start (ap, fmt);
  SC3E (sc3_error_set_messagev (e, fmt, ap));
  va_end (ap);

  return NULL;
}

sc3_error_t        *
sc3_error_set_messagev (sc3_error_t * e, const char *fmt, va_list ap)
{
  char                buf[SC3_BUFSIZE];
  const char         *msg = NULL;

  SC3A_CHECK (fmt != NULL);
  if (0 <= vsnprintf (buf, SC3_BUFSIZE, fmt, ap)) {
    msg = buf;
  }
  else {
    msg = "Message format error";
  }

  SC3E (sc3_error_set_message (e, msg));
  return NULL;
}

sc3_error_t        *
sc3_error_set_kind (sc3_error_t * e, sc3_error_kind_t kind)
{
  SC3A_IS (sc3_error_is_new, e);
  SC3A_CHECK (0 <= kind && kind < SC3_ERROR_KIND_LAST);

  e->kind = kind;
  return NULL;
}

sc3_error_t        *
sc3_error_setup (sc3_error_t * e)
{
  SC3A_IS (sc3_error_is_new, e);

  /* we are done with setup */
  e->setup = 1;
  SC3A_IS (sc3_error_is_setup, e);
  return NULL;
}

sc3_error_t        *
sc3_error_ref (sc3_error_t * e)
{
  SC3A_IS (sc3_error_is_setup, e);
  if (e->alloced) {
    SC3E (sc3_refcount_ref (&e->rc));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_unref (sc3_error_t ** ep)
{
  int                 waslast;
  sc3_error_t        *e;

  SC3E_INOUTP (ep, e);
  SC3A_IS (sc3_error_is_valid, e);

  if (!e->alloced) {
    /* It is our convention that non-alloced errors must not have a stack. */
    SC3A_CHECK (e->stack == NULL);
    return NULL;
  }

  SC3E (sc3_refcount_unref (&e->rc, &waslast));
  if (waslast) {
    *ep = NULL;

    SC3E_DEMAND (e->accessed_locations == 0, SC3_ERROR_REF);
    SC3E_DEMAND (e->accessed_messages == 0, SC3_ERROR_REF);
    if (e->stack != NULL) {
      SC3E (sc3_error_unref (&e->stack));
    }
    SC3E (sc3_free (&e));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_destroy (sc3_error_t ** ep)
{
  sc3_error_t        *e;

  SC3E_INULLP (ep, e);
  SC3E_DEMIS (sc3_refcount_is_last, &e->rc, SC3_ERROR_REF);
  SC3E (sc3_error_unref (&e));

  SC3A_CHECK (e == NULL || !e->alloced);
  return NULL;
}

/* Take over one reference to the stack, but NULL argument is ok, too. */
static sc3_error_t *
sc3_error_new_build (sc3_error_t ** pstack, sc3_error_kind_t kind,
                     const char *filename, int line, const char *errmsg,
                     sc3_error_t ** ep)
{
  sc3_error_t        *e;

  /* special behavior to return early when memory allocation failed earlier */
  if (pstack != NULL && sc3_error_is_setup (*pstack, NULL) &&
      !(*pstack)->alloced) {
    /* The stack passed in is a static predefined error.
     * Likely the allocation of errors is no longer working.
     * Output the stack without further ado to prevent more errors.
     */
    if (ep != NULL) {
      /* intended use of this function is passing through the stack */
      *ep = *pstack;
      *pstack = NULL;
      return NULL;
    }
    else {
      /* this is wrapping the input stack into an error return */
      e = *pstack;
      *pstack = NULL;
      return e;
    }
  }

  /* delayed check of preconditions */
  SC3E_RETVAL (ep, NULL);
  SC3A_CHECK (0 <= kind && kind < SC3_ERROR_KIND_LAST);
  SC3A_CHECK (filename != NULL);
  SC3A_CHECK (line >= 0);
  SC3A_CHECK (errmsg != NULL);

  /* normal construction procedure */
  SC3E (sc3_error_new (&e));
  if (pstack != NULL) {
    SC3E (sc3_error_set_stack (e, pstack));
  }
  SC3E (sc3_error_set_kind (e, kind));
  SC3E (sc3_error_set_location (e, filename, line));
  SC3E (sc3_error_set_message (e, errmsg));
  SC3E (sc3_error_setup (e));

  SC3A_IS (sc3_error_is_setup, e);

  /* *ep is not NULL if and only this function returns NULL */
  *ep = e;
  return NULL;
}

sc3_error_t        *
sc3_error_new_assert (const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *eout;

  SC3E (sc3_error_new_build
        (NULL, SC3_ERROR_ASSERT, filename, line, errmsg, &eout));
  return eout;
}

sc3_error_t        *
sc3_error_new_kind (sc3_error_kind_t kind,
                    const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *eout;

  SC3E (sc3_error_new_build (NULL, kind, filename, line, errmsg, &eout));
  return eout;
}

sc3_error_t        *
sc3_error_new_stack (sc3_error_t ** pstack,
                     const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *eout;

  SC3E (sc3_error_new_build
        (pstack, SC3_ERROR_FATAL, filename, line, errmsg, &eout));
  return eout;
}

#if 0

sc3_error_t        *
sc3_error_flatten (sc3_error_t ** pe, const char *prefix, char *flatmsg)
{
  int                 stline;
  int                 remain, result;
  char                tempmsg[SC3_BUFSIZE];
  char                stbname[SC3_BUFSIZE];
  char               *pos, *out;
  const char         *stfilename, *stmsg;
  sc3_error_t        *e, *stack;
  sc3_error_kind_t    kind;

  /* We take ownership of *pe and expect an existing error and buffer */
  SC3E_INULLP (pe, e);
  SC3A_CHECK (flatmsg != NULL);

  /* So now we can go to work */
  SC3A_IS (sc3_error_is_setup, e);
  out = (prefix != NULL ? tempmsg : flatmsg);

  /* go through error stack's messages */
  remain = SC3_BUFSIZE;
  *(pos = out) = '\0';
  do {
    if (remain > 0) {
      /* access information on current stack top */
      SC3E (sc3_error_access_location (e, &stfilename, &stline));
      sc3_strcopy (stbname, SC3_BUFSIZE, stfilename);
      SC3E (sc3_error_access_message (e, &stmsg));
      SC3E (sc3_error_get_kind (e, &kind));

      /* append error location and message to output string */
      result = snprintf (pos, remain,
#if 0
                         "%s%s:%d:%c %s", pos == out ? "" : ": ",
#else
                         "%s(%s:%d:%c %s)", pos == out ? "" : " ",
#endif
                         sc3_basename (stbname), stline,
                         sc3_error_kind_char[kind], stmsg);
      SC3E (sc3_error_restore_location (e, stfilename, stline));
      SC3E (sc3_error_restore_message (e, stmsg));
      if (result < 0 || result >= remain) {
        pos = NULL;
        remain = 0;
      }
      else {
        pos += result;
        remain -= result;
      }
    }

    /* do down the error stack, we get a stack with a bumped reference */
    SC3E (sc3_error_ref_stack (e, &stack));

    /* leak errors in this function are fatal to avoid infinite recursion */
    SC3E (sc3_error_unref (&e));
    e = stack;
  }
  while (e != NULL);

  /* finish construction of new flat error string */
  if (prefix != NULL) {
    sc3_snprintf (flatmsg, SC3_BUFSIZE, "%s %s", prefix, tempmsg);
  }
  return NULL;
}

sc3_error_t        *
sc3_error_accum_kind (sc3_allocator_t * alloc,
                      sc3_error_t ** pcollect, sc3_error_kind_t kind,
                      const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *c, *e;

  /* check call convention */
  SC3A_IS (sc3_allocator_is_setup, alloc);
  SC3A_CHECK (pcollect != NULL);
  SC3A_CHECK (0 <= kind && kind < SC3_ERROR_KIND_LAST);

  /* Construct a new error to hold the flat message. */
  SC3E (sc3_error_new (&e));
  SC3E (sc3_error_set_location (e, filename, line));
  SC3E (sc3_error_set_message (e, errmsg));
  SC3E (sc3_error_set_kind (e, kind));

  /* if the collection is not empty, take it with us as stack */
  if ((c = *pcollect) != NULL) {
    *pcollect = NULL;
    SC3E (sc3_error_set_stack (e, &c));
  }

  /* Finalize the error object and place it as new collection. */
  SC3E (sc3_error_setup (e));
  *pcollect = e;
  return NULL;
}

sc3_error_t        *
sc3_error_accumulate (sc3_allocator_t * alloc,
                      sc3_error_t ** pcollect, sc3_error_t ** pe,
                      const char *filename, int line, const char *errmsg)
{
  sc3_error_t        *e;
  sc3_error_kind_t    kind;
  char                flatmsg[SC3_BUFSIZE];

  /* check call convention */
  SC3A_IS (sc3_allocator_is_setup, alloc);
  SC3A_CHECK (pcollect != NULL);
  SC3A_CHECK (pe != NULL);

  /* If no error comes in, there is nothing to do. */
  if ((e = *pe) == NULL) {
    return NULL;
  }

  /* The input error is owned and flattened. */
  *pe = NULL;
  SC3E (sc3_error_get_kind (e, &kind));
  SC3E (sc3_error_flatten (&e, errmsg, flatmsg));

  /* Accumulate a new error to hold the flat message. */
  SC3E (sc3_error_accum_kind (alloc, pcollect,
                              kind, filename, line, flatmsg));
  return NULL;
}

/** Accumulate a newly created error into a collection of leaks.
 * This function is a restricted version of \ref sc3_error_accum_kind.
 * We use a static allocator, which is ok for macros without context.
 * \param [in,out] leak     Pointer to error collection must not be NULL.
 *                          If the pointed-to error is not NULL, it must be
 *                          of kind \ref SC3_ERROR_LEAK or we return fatal.
 *                          On output, new leak error with input as stack.
 * \param [in] filename     File name to report in the new error object.
 * \param [in] line         Line number to report.
 * \param [in] errmsg       The message to report.
 * \return              NULL on success, fatal error otherwise.
 */
static sc3_error_t *
sc3_error_accum_leak (sc3_error_t ** leak,
                      const char *filename, int line, const char *errmsg)
{
  SC3A_CHECK (leak != NULL);
  SC3A_IS (sc3_error_is_null_or_leak, *leak);

  SC3E (sc3_error_accum_kind (sc3_allocator_nocount (), leak,
                              SC3_ERROR_LEAK, filename, line, errmsg));
  return NULL;
}

sc3_error_t        *
sc3_error_leak (sc3_error_t ** leak, sc3_error_t * e,
                const char *filename, int line, const char *errmsg)
{
  SC3A_CHECK (leak != NULL);
  SC3A_IS (sc3_error_is_null_or_leak, *leak);

  if (e != NULL) {
    char                flatmsg[SC3_BUFSIZE];

    /* This function is not supposed to be passed a fatal error e.
       If this happens, we call it a bug and return it for stacking. */
    if (sc3_error_is_fatal (e, NULL)) {
      return e;
    }

    /* There should be no fatal errors here, but if so, return them. */
    SC3E (sc3_error_flatten (&e, errmsg, flatmsg));
    SC3E (sc3_error_accum_leak (leak, filename, line, flatmsg));
  }
  return NULL;
}

#endif

sc3_error_t        *
sc3_error_access_location (sc3_error_t * e, const char **filename, int *line)
{
  SC3E_RETVAL (filename, NULL);
  SC3E_RETVAL (line, 0);
  SC3A_IS (sc3_error_is_setup, e);

  *filename = e->filename;
  *line = e->line;
  if (e->alloced) {
    ++e->accessed_locations;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_restore_location (sc3_error_t * e, const char *filename, int line)
{
  SC3A_IS (sc3_error_is_setup, e);
  SC3A_CHECK (filename == e->filename);
  SC3A_CHECK (line == e->line);

  if (e->alloced) {
    SC3A_CHECK (e->accessed_locations > 0);
    --e->accessed_locations;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_access_message (sc3_error_t * e, const char **errmsg)
{
  SC3E_RETVAL (errmsg, NULL);
  SC3A_IS (sc3_error_is_setup, e);

  *errmsg = e->errmsg;
  if (e->alloced) {
    ++e->accessed_messages;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_restore_message (sc3_error_t * e, const char *errmsg)
{
  SC3A_IS (sc3_error_is_setup, e);
  SC3A_CHECK (errmsg == e->errmsg);

  if (e->alloced) {
    SC3A_CHECK (e->accessed_messages > 0);
    --e->accessed_messages;
  }
  return NULL;
}

sc3_error_t        *
sc3_error_get_kind (const sc3_error_t * e, sc3_error_kind_t * kind)
{
  SC3E_RETVAL (kind, SC3_ERROR_FATAL);
  SC3A_IS (sc3_error_is_setup, e);

  *kind = e->kind;
  return NULL;
}

sc3_error_t        *
sc3_error_ref_stack (sc3_error_t * e, sc3_error_t ** pstack)
{
  SC3E_RETVAL (pstack, NULL);
  SC3A_IS (sc3_error_is_setup, e);

  if (e->stack != NULL) {
    SC3E (sc3_error_ref (*pstack = e->stack));
  }
  return NULL;
}

/** Turn an error stack into a (multiline) error message.
 * \param [in] bwork        Either NULL or must hold SC3_BUFSIZE many bytes.
 * \param [in,out] buffer   Must not be NULL and hold at least \a bufrem bytes.
 * \param [in,out] bufrem   Value holds number of available bytes on input,
 *                          the number of bytes remaining on output.
 */
static sc3_error_t *
sc3_error_copy_text_rec (sc3_error_t * e, sc3_error_recursion_t recursion,
                         int rdepth, char *bwork,
                         char *buffer, size_t *bufrem)
{
  int                 printed;
  int                 eline;
  size_t              bufin;
  char                pref[8];
  const char         *bname;
  const char         *efile;
  const char         *emsg;
  sc3_error_kind_t    ekind;
  sc3_error_t        *stack;

  SC3A_IS (sc3_error_is_valid, e);
  SC3A_CHECK (0 <= recursion && recursion < SC3_ERROR_RECURSION_LAST);
  SC3A_CHECK (buffer != NULL);
  SC3A_CHECK (bufrem != NULL);

  /* The function relies on this precondition. */
  SC3A_CHECK (*bufrem > 0);

  /* see if there is reason for recursion */
  stack = NULL;
  if (recursion != SC3_ERROR_RECURSION_NONE) {
    SC3E (sc3_error_ref_stack (e, &stack));
  }

  if (stack != NULL && recursion == SC3_ERROR_RECURSION_POSTORDER) {
    /*
     * By the precondition of this function, at least one byte may be written.
     * The recursion will definitely write at least one byte.
     * Thus, we are allowed to rewind the buffer by one byte.
     */
    bufin = *bufrem;
    SC3E (sc3_error_copy_text_rec (stack, recursion, rdepth + 1,
                                   bwork, buffer, bufrem));
    SC3A_CHECK (*bufrem < bufin);
    buffer += bufin - *bufrem;

    if (*bufrem > 0) {
      /* we replace the terminating NUL with a line break to continue */
      SC3A_CHECK (buffer[-1] == '\0');
      buffer[-1] = '\n';
    }
  }

  /* print stuff into buffer, move buffer pointer and decrease *bufrem */

  if (*bufrem > 0) {
    SC3E (sc3_error_access_location (e, &efile, &eline));
    SC3E (sc3_error_access_message (e, &emsg));
    SC3E (sc3_error_get_kind (e, &ekind));
    SC3A_CHECK (0 <= ekind && ekind < SC3_ERROR_KIND_LAST);

    if (recursion == SC3_ERROR_RECURSION_NONE) {
      sc3_strcopy (pref, 8, "ET ");
    }
    else {
      sc3_snprintf (pref, 8, "E%d ", rdepth);
    }
    if (bwork == NULL) {
      bname = efile;
    }
    else {
      sc3_strcopy (bwork, SC3_BUFSIZE, efile);
      bname = sc3_basename (bwork);
    }
    printed = snprintf (buffer, *bufrem, "%s%s:%d %c:%s", pref,
                        bname, eline, sc3_error_kind_char[ekind], emsg);

    SC3E (sc3_error_restore_location (e, efile, eline));
    SC3E (sc3_error_restore_message (e, emsg));

    /* determine the number of bytes printed including the terminating Nul */
    if (printed < 0) {
      /* something went wrong with snprintf */
      buffer[0] = '\0';
      printed = 1;
    }
    else if ((size_t) printed >= *bufrem) {
      /* output was truncated, count the terminating Nul */
      printed = *bufrem;
    }
    else {
      /* count terminating NUL, which snprintf does not */
      ++printed;
    }
    buffer += printed;
    *bufrem -= printed;
  }

  if (stack != NULL && recursion == SC3_ERROR_RECURSION_PREORDER) {
    /*
     * We did not enter the preorder recursion call.
     * By precondition of this function, *bufrem was positive, so
     * we definitely entered the print block above.  Thus, we
     * printed at least one byte and advanced the buffer pointer.
     * This means that we are allowed to rewind by one byte.
     */
    if (*bufrem > 0) {
      /* we replace the terminating NUL with a line break to continue */
      SC3A_CHECK (buffer[-1] == '\0');
      buffer[-1] = '\n';
      SC3E (sc3_error_copy_text_rec (stack, recursion, rdepth + 1,
                                     bwork, buffer, bufrem));
    }
  }

  /* clean up and return */
  if (stack != NULL) {
    SC3E (sc3_error_unref (&stack));
  }
  return NULL;
}

sc3_error_t        *
sc3_error_copy_text (sc3_error_t * e, sc3_error_recursion_t recursion,
                     int dobasename, char *buffer, size_t buflen)
{
  /* preconditions and early return */
  SC3A_IS (sc3_error_is_valid, e);
  if (buffer == NULL || buflen == 0) {
    return NULL;
  }

  /* compute number of remaining bytes but do not return it to the caller */
  if (!dobasename) {
    SC3E (sc3_error_copy_text_rec (e, recursion, 0, NULL, buffer, &buflen));
  }
  else {
    char                bwork[SC3_BUFSIZE];
    SC3E (sc3_error_copy_text_rec (e, recursion, 0, bwork, buffer, &buflen));
  }
  return NULL;
}

void
sc3_error_unref_noerr (sc3_error_t * e)
{
  if (e == NULL) {
    return;
  }

  /* unref recursively (survives NULL stack) */
  sc3_error_unref_noerr (e->stack);

  /* if allocated, drop a reference and possibly free */
  if (e->alloced && --e->rc.rc == 0) {
    sc3_free (&e);
  }
}

int
sc3_error_check (char *buffer, size_t buflen, sc3_error_t * e)
{
  /* address the error object passed in */
  if (e != NULL) {
    sc3_error_t        *e2;

    /* access message of error and unref without checking */
    e2 = sc3_error_copy_text (e, SC3_ERROR_RECURSION_POSTORDER,
                              1, buffer, buflen);
    sc3_error_unref_noerr (e);

    /* error message appears is in the output buffer, or worse */
    if (e2 != NULL) {
      sc3_error_unref_noerr (e2);
      if (buffer != NULL && buflen > 0) {
        sc3_strcopy (buffer, buflen, "Invalid error text");
      }
    }

    /* error condition returns true */
    return -1;
  }

  /* do nothing: buffer contents are left undefined */
  return 0;
}
