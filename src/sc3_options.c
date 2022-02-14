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

#include <sc3_array.h>
#include <sc3_options.h>
#include <sc3_refcount.h>

typedef enum sc3_option_type
{
  SC3_OPTION_INT,
  SC3_OPTION_TYPE_LAST
}
sc3_option_type_t;

typedef struct sc3_option
{
  sc3_option_type_t   opt_type;
  int                 opt_short;
  const char         *opt_long;
  size_t              opt_long_len;
  const char         *opt_help;
  union
  {
    int                *var_int;
  } v;
}
sc3_option_t;

struct sc3_options
{
  /* internal metadata */
  sc3_refcount_t      rc;
  sc3_allocator_t    *alloc;
  int                 setup;

  /* internal configuration */
  sc3_array_t        *opts;

#if 0
  /* parameters set before and fixed after setup */
  int                 dummy;

  /* member variables initialized during setup */
  int                *member;
#endif
};

int
sc3_options_is_valid (const sc3_options_t * yy, char *reason)
{
  SC3E_TEST (yy != NULL, reason);
  SC3E_IS (sc3_refcount_is_valid, &yy->rc, reason);
  SC3E_IS (sc3_allocator_is_setup, yy->alloc, reason);

  /* check options array */
  if (!yy->setup) {
    SC3E_IS (sc3_array_is_resizable, yy->opts, reason);
  }
  else {
    SC3E_IS (sc3_array_is_unresizable, yy->opts, reason);
  }

  /* go through individial options */

  SC3E_YES (reason);
}

int
sc3_options_is_new (const sc3_options_t * yy, char *reason)
{
  SC3E_IS (sc3_options_is_valid, yy, reason);
  SC3E_TEST (!yy->setup, reason);
  SC3E_YES (reason);
}

int
sc3_options_is_setup (const sc3_options_t * yy, char *reason)
{
  SC3E_IS (sc3_options_is_valid, yy, reason);
  SC3E_TEST (yy->setup, reason);
  SC3E_YES (reason);
}

#if 0

int
sc3_options_is_dummy (const sc3_options_t * yy, char *reason)
{
  SC3E_IS (sc3_options_is_setup, yy, reason);
  SC3E_TEST (yy->dummy, reason);
  SC3E_YES (reason);
}

#endif

sc3_error_t        *
sc3_options_new (sc3_allocator_t * alloc, sc3_options_t ** yyp)
{
  sc3_options_t      *yy;

  SC3E_RETVAL (yyp, NULL);

  /* allocator to be used */
  if (alloc == NULL) {
    alloc = sc3_allocator_new_static ();
  }
  SC3A_IS (sc3_allocator_is_setup, alloc);

  /* internal metadata */
  SC3E (sc3_allocator_ref (alloc));
  SC3E (sc3_allocator_calloc (alloc, 1, sizeof (sc3_options_t), &yy));
  SC3E (sc3_refcount_init (&yy->rc));
  yy->alloc = alloc;

  /* internal array of options */
  SC3E (sc3_array_new (alloc, &yy->opts));
  SC3E (sc3_array_set_elem_size (yy->opts, sizeof (sc3_option_t)));
  SC3E (sc3_array_set_tighten (yy->opts, 1));
  SC3E (sc3_array_setup (yy->opts));

  /* done with allocation */
  SC3A_IS (sc3_options_is_new, yy);
  *yyp = yy;
  return NULL;
}

#if 0

sc3_error_t        *
sc3_options_set_dummy (sc3_options_t * yy, int dummy)
{
  SC3A_IS (sc3_options_is_new, yy);
  yy->dummy = dummy;
  return NULL;
}

#endif

sc3_error_t        *
sc3_options_add_int (sc3_options_t * yy,
                     int opt_short, const char *opt_long,
                     const char *opt_help, int *opt_variable, int opt_value)
{
  sc3_option_t       *o;

  SC3A_IS (sc3_options_is_new, yy);
  SC3A_CHECK (opt_variable != NULL);

  SC3E (sc3_array_push (yy->opts, &o));
  o->opt_type = SC3_OPTION_INT;
  o->opt_short = opt_short;
  o->opt_long = opt_long;
  o->opt_long_len = strlen (opt_long);
  o->opt_help = opt_help;
  *(o->v.var_int = opt_variable) = opt_value;

  return NULL;
}

sc3_error_t        *
sc3_options_setup (sc3_options_t * yy)
{
  SC3A_IS (sc3_options_is_new, yy);

  /* finalize internal state */
  SC3E (sc3_array_freeze (yy->opts));

  /* done with setup */
  yy->setup = 1;
  SC3A_IS (sc3_options_is_setup, yy);
  return NULL;
}

sc3_error_t        *
sc3_options_ref (sc3_options_t * yy)
{
  SC3A_IS (sc3_options_is_setup, yy);
  SC3E (sc3_refcount_ref (&yy->rc));
  return NULL;
}

sc3_error_t        *
sc3_options_unref (sc3_options_t ** yyp)
{
  int                 waslast;
  sc3_allocator_t    *alloc;
  sc3_options_t      *yy;

  SC3E_INOUTP (yyp, yy);
  SC3A_IS (sc3_options_is_valid, yy);
  SC3E (sc3_refcount_unref (&yy->rc, &waslast));
  if (waslast) {
    *yyp = NULL;
    alloc = yy->alloc;

    /* deallocate internal state */
    if (yy->setup) {
    }
    SC3E (sc3_array_destroy (&yy->opts));

    /* deallocate array itself */
    SC3E (sc3_allocator_free (alloc, &yy));
    SC3E (sc3_allocator_unref (&alloc));
  }
  return NULL;
}

sc3_error_t        *
sc3_options_destroy (sc3_options_t ** yyp)
{
  sc3_options_t      *yy;

  SC3E_INULLP (yyp, yy);
  SC3E_DEMIS (sc3_refcount_is_last, &yy->rc, SC3_ERROR_REF);
  SC3E (sc3_options_unref (&yy));

  SC3A_CHECK (yy == NULL);
  return NULL;
}

#if 0

sc3_error_t        *
sc3_options_get_dummy (const sc3_options_t * yy, int *dummy)
{
  SC3E_RETVAL (dummy, 0);
  SC3A_IS (sc3_options_is_setup, yy);

  *dummy = yy->dummy;
  return NULL;
}

#endif

sc3_error_t        *
sc3_options_parse (sc3_options_t * yy, int argc, const char **argv,
                   int *arg_pos, int *result)
{
  SC3E_RETVAL (result, -1);
  SC3A_IS (sc3_options_is_setup, yy);
  SC3A_CHECK (0 <= argc);
  SC3A_CHECK (argv != NULL);
  SC3A_CHECK (arg_pos != NULL);
  SC3A_CHECK (0 <= *arg_pos && *arg_pos < argc);

  *result = 0;

  return NULL;
}
