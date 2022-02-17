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
  SC3_OPTION_STRING,
  SC3_OPTION_TYPE_LAST
}
sc3_option_type_t;

typedef struct sc3_option
{
  sc3_option_type_t   opt_type;
  char                opt_short;
  const char         *opt_long;
  size_t              opt_long_len;
  int                 opt_has_arg;
  const char         *opt_help;
  union
  {
    int                *var_int;
    char               *var_string;
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
  int                 allow_pack;       /**< Accept short options '-abc'. */
  int                *var_stop;         /**< Output variable for '--'. */
  sc3_array_t        *opts;
};

int
sc3_options_is_valid (const sc3_options_t * yy, char *reason)
{
  int                    i, len;
  sc3_option_t          *o;

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
  SC3E_DO (sc3_array_get_elem_count (yy->opts, &len), reason);
  for (i = 0; i < len; ++i) {
    SC3E_DO (sc3_array_index (yy->opts, i, &o), reason);
    SC3E_TEST (0 <= o->opt_type && o->opt_type < SC3_OPTION_TYPE_LAST,
               reason);
  }

  /* everything checked out */
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

sc3_error_t        *
sc3_options_set_stop (sc3_options_t * yy, int *var_stop)
{
  SC3A_IS (sc3_options_is_new, yy);
  if ((yy->var_stop = var_stop) != NULL) {
    *var_stop = 0;
  }
  return NULL;
}

sc3_error_t        *
sc3_options_add_int (sc3_options_t * yy,
                     char opt_short, const char *opt_long,
                     const char *opt_help, int *opt_variable, int opt_value)
{
  sc3_option_t       *o;

  SC3A_IS (sc3_options_is_new, yy);
  SC3A_CHECK (opt_variable != NULL);

  SC3E (sc3_array_push (yy->opts, &o));
  o->opt_type = SC3_OPTION_INT;
  o->opt_short = opt_short;
  o->opt_long = opt_long;
  o->opt_long_len = opt_long != NULL ? strlen (opt_long) : 0;
  o->opt_has_arg = 1;
  o->opt_help = opt_help;
  *(o->v.var_int = opt_variable) = opt_value;

  return NULL;
}

sc3_error_t        *
sc3_options_add_string (sc3_options_t * yy,
                        char opt_short, const char *opt_long,
                        const char *opt_help, const char **opt_variable,
                        const char *opt_value)
{
  sc3_option_t       *o;

  SC3A_IS (sc3_options_is_new, yy);
  SC3A_CHECK (opt_variable != NULL);

  SC3E (sc3_array_push (yy->opts, &o));
  o->opt_type = SC3_OPTION_STRING;
  o->opt_short = opt_short;
  o->opt_long = opt_long;
  o->opt_long_len = opt_long != NULL ? strlen (opt_long) : 0;
  o->opt_has_arg = 1;
  o->opt_help = opt_help;

  /* deep copy default value */
  SC3E (sc3_allocator_strdup (yy->alloc, opt_value, &o->v.var_string));
  *opt_variable = o->v.var_string;

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
    int              i, len;
    sc3_option_t    *o;

    *yyp = NULL;
    alloc = yy->alloc;

    /* deallocate internal state */
    SC3E (sc3_array_get_elem_count (yy->opts, &len));
    for (i = 0; i < len; ++i) {
      SC3E (sc3_array_index (yy->opts, i, &o));
      if (o->opt_type == SC3_OPTION_TYPE_LAST) {
        SC3E (sc3_allocator_free (yy->alloc, &o->v.var_string));
      }
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
sc3_options_parse (sc3_options_t * yy, int argc, char **argv,
                   int *arg_pos, int *result)
{
  int                 pos;
  size_t              lz;
  const char         *at;

  SC3E_RETVAL (result, -1);
  SC3A_IS (sc3_options_is_setup, yy);
  SC3A_CHECK (0 <= argc);
  SC3A_CHECK (argv != NULL);
  SC3A_CHECK (arg_pos != NULL);
  SC3A_CHECK (0 <= *arg_pos && *arg_pos < argc);

  /* preliminary checks */
  pos = *arg_pos;
  at = argv[pos];
  if (at == NULL) {
    /* erroneous argv parameter */
    /* result remains at -1 */
    return NULL;
  }
  lz = strlen (at);
  if (lz < 2 || at[0] != '-') {
    /* this is no kind of option */
    *result = 0;
    return NULL;
  }

  /* honor stop option */
  if (yy->var_stop != NULL && !strcmp (at, "--")) {
    *yy->var_stop = 1;
    *arg_pos += (*result = 1);
    return NULL;
  }

  /* parse short options */

  /* parse long options */

  ++*arg_pos;
  *result = 1;

  return NULL;
}
