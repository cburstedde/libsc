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
  SC3_OPTION_SWITCH,
  SC3_OPTION_INT,
  SC3_OPTION_DOUBLE,
  SC3_OPTION_STRING,
  SC3_OPTION_TYPE_LAST
}
sc3_option_type_t;

static const char  *opt_disp[SC3_OPTION_TYPE_LAST] =
  { "SWITCH", "INT", "DOUBLE", "STRING" };

typedef struct sc3_option
{
  sc3_option_type_t   opt_type;
  char                opt_short;
  char               *opt_long_alloc;
  const char         *opt_long;
  size_t              opt_long_len;
  int                 opt_has_arg;
  const char         *opt_help;
  char              **opt_string_value;     /**< allocated string variable */
  union
  {
    /* address of current option value in caller memory */
    int                *var_int;
    double             *var_double;
    const char        **var_string;
  } v;
  sc3_options_t      *sub;
}
sc3_option_t;

typedef struct sc3_options_subopt
{
  sc3_options_t      *sub;
  const char         *prefix;
}
sc3_options_subopt_t;

struct sc3_options
{
  /* internal metadata */
  sc3_refcount_t      rc;
  sc3_allocator_t    *alloc;
  int                 setup;

  /* internal configuration */
  int                 spacing;          /**< Space for value and type. */
  sc3_array_t        *opts;             /**< Array of options. */

  /* list of sub-options to this one */
  sc3_array_t        *subs;             /**< FIFO-storage of sub-options. */
};

int
sc3_options_is_valid (const sc3_options_t * yy, char *reason)
{
  int                 i, len;
  sc3_option_t       *o;

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
    switch (o->opt_type) {
    case SC3_OPTION_SWITCH:
      SC3E_TEST (o->v.var_int != NULL, reason);
      SC3E_TEST (o->opt_string_value == NULL, reason);
      break;
    case SC3_OPTION_INT:
      SC3E_TEST (o->v.var_int != NULL, reason);
      SC3E_TEST (o->opt_string_value == NULL, reason);
      break;
    case SC3_OPTION_DOUBLE:
      SC3E_TEST (o->v.var_double != NULL, reason);
      SC3E_TEST (o->opt_string_value == NULL, reason);
      break;
    case SC3_OPTION_STRING:
      SC3E_TEST (o->v.var_string != NULL, reason);
      SC3E_TEST (o->opt_string_value != NULL, reason);
      break;
    default:
      SC3E_NO (reason, "Invalid option type");
    }
    if (o->sub != NULL) {
      SC3E_IS (sc3_options_is_setup, o->sub, reason);
    }
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
  yy->spacing = 16;

  /* internal arrays of options and sub-options */
  SC3E (sc3_array_new_size (alloc, &yy->opts, sizeof (sc3_option_t)));
  SC3E (sc3_array_new_size (alloc, &yy->subs, sizeof (sc3_options_subopt_t)));

  /* done with allocation */
  SC3A_IS (sc3_options_is_new, yy);
  *yyp = yy;
  return NULL;
}

sc3_error_t        *
sc3_options_set_spacing (sc3_options_t * yy, int spacing)
{
  SC3A_IS (sc3_options_is_new, yy);
  SC3A_CHECK (spacing >= 0);
  yy->spacing = spacing;
  return NULL;
}

static sc3_error_t *
sc3_options_add_common (sc3_options_t * yy, sc3_option_type_t tt,
                        char opt_short, const char *opt_long,
                        const char *opt_help, sc3_option_t ** oo)
{
  sc3_option_t       *o;

  /* initial checks */
  SC3E_RETVAL (oo, NULL);
  SC3A_IS (sc3_options_is_new, yy);
  SC3A_CHECK (0 <= tt && tt < SC3_OPTION_TYPE_LAST);
  SC3A_CHECK (opt_short != '-');
  SC3A_CHECK (opt_long == NULL || opt_long[0] != '-');

  SC3E (sc3_array_push (yy->opts, &o));

  /* set couple parameters of this option */
  o->opt_type = tt;
  o->opt_short = opt_short;
  o->opt_long_alloc = NULL;
  o->opt_long = opt_long;
  o->opt_long_len = opt_long != NULL ? strlen (opt_long) : 0;
  o->opt_has_arg = 0;
  o->opt_help = opt_help;
  o->opt_string_value = NULL;
  o->sub = NULL;

  /* return partially initialized option */
  *oo = o;
  return NULL;
}

sc3_error_t        *
sc3_options_add_switch (sc3_options_t * yy,
                        char opt_short, const char *opt_long,
                        const char *opt_help, int *opt_variable)
{
  sc3_option_t       *o;

  SC3A_CHECK (opt_variable != NULL);
  SC3E (sc3_options_add_common (yy, SC3_OPTION_SWITCH,
                                opt_short, opt_long, opt_help, &o));

  *(o->v.var_int = opt_variable) = 0;
  return NULL;
}

sc3_error_t        *
sc3_options_add_int (sc3_options_t * yy,
                     char opt_short, const char *opt_long,
                     const char *opt_help, int *opt_variable, int opt_value)
{
  sc3_option_t       *o;

  SC3A_CHECK (opt_variable != NULL);
  SC3E (sc3_options_add_common (yy, SC3_OPTION_INT,
                                opt_short, opt_long, opt_help, &o));

  /* assign default value */
  *(o->v.var_int = opt_variable) = opt_value;
  o->opt_has_arg = 1;
  return NULL;
}

sc3_error_t        *
sc3_options_add_double (sc3_options_t * yy,
                        char opt_short, const char *opt_long,
                        const char *opt_help,
                        double *opt_variable, double opt_value)
{
  sc3_option_t       *o;

  SC3A_CHECK (opt_variable != NULL);
  SC3E (sc3_options_add_common (yy, SC3_OPTION_DOUBLE,
                                opt_short, opt_long, opt_help, &o));

  /* assign default value */
  *(o->v.var_double = opt_variable) = opt_value;
  o->opt_has_arg = 1;
  return NULL;
}

sc3_error_t        *
sc3_options_add_string (sc3_options_t * yy,
                        char opt_short, const char *opt_long,
                        const char *opt_help, const char **opt_variable,
                        const char *opt_value)
{
  sc3_option_t       *o;

  SC3A_CHECK (opt_variable != NULL);
  SC3E (sc3_options_add_common (yy, SC3_OPTION_STRING,
                                opt_short, opt_long, opt_help, &o));

  /* deep copy default value */
  SC3E (sc3_allocator_malloc (yy->alloc,
                              sizeof (char *), &o->opt_string_value));
  SC3E (sc3_allocator_strdup (yy->alloc, opt_value, o->opt_string_value));
  *(o->v.var_string = opt_variable) = *o->opt_string_value;
  o->opt_has_arg = 1;
  return NULL;
}

sc3_error_t        *
sc3_options_add_sub (sc3_options_t * yy,
                     sc3_options_t * sub, const char *prefix)
{
  int                 i, len;
  char                combine[SC3_BUFSIZE];
  sc3_option_t       *src, *dest;
  sc3_options_subopt_t *so;

  SC3A_IS (sc3_options_is_new, yy);
  SC3A_IS (sc3_options_is_setup, sub);

  /* loop through entries of sub options object */
  SC3E (sc3_array_get_elem_count (sub->opts, &len));
  for (i = 0; i < len; ++i) {
    SC3E (sc3_array_index (sub->opts, i, &src));
    if (src->opt_short == '\0' && src->opt_long_len == 0) {
      continue;
    }

    /* copy each of the sub options into current options object */
    SC3E (sc3_array_push (yy->opts, &dest));
    *dest = *src;
    dest->opt_long_alloc = NULL;
    dest->sub = src->sub != NULL ? src->sub : sub;

    /* allocate combined long option name */
    if (prefix != NULL && prefix[0] != '\0') {
      if (src->opt_long_len > 0) {
        sc3_snprintf (combine, SC3_BUFSIZE, "%s:%s", prefix, src->opt_long);
      }
      else {
        SC3A_CHECK (src->opt_short != '\0');
        sc3_snprintf (combine, SC3_BUFSIZE, "%s:-%c", prefix, src->opt_short);
      }
      SC3E (sc3_allocator_strdup (yy->alloc, combine, &dest->opt_long_alloc));
      dest->opt_long_len = strlen (dest->opt_long = dest->opt_long_alloc);
      dest->opt_short = '\0';
    }
  }

  /* remember new sub options in array */
  SC3E (sc3_array_push (yy->subs, &so));
  so->sub = sub;
  so->prefix = sc3_strpass (prefix);

  /* reference sub-options since we rely on their const char * parameters */
  SC3E (sc3_options_ref (sub));
  return NULL;
}

sc3_error_t        *
sc3_options_setup (sc3_options_t * yy)
{
  SC3A_IS (sc3_options_is_new, yy);

  /* finalize internal state */
  SC3E (sc3_array_freeze (yy->opts));
  SC3E (sc3_array_freeze (yy->subs));

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
    int                 i, len;
    sc3_option_t       *o;
    sc3_options_subopt_t *so;

    *yyp = NULL;
    alloc = yy->alloc;

    /* deallocate internal state: free option entries */
    SC3E (sc3_array_get_elem_count (yy->opts, &len));
    for (i = 0; i < len; ++i) {
      SC3E (sc3_array_index (yy->opts, i, &o));
      SC3E (sc3_allocator_free (alloc, &o->opt_long_alloc));

      /* use the original allocator for the string's value */
      if (o->opt_type == SC3_OPTION_STRING && o->sub == NULL) {
        SC3E (sc3_allocator_free (alloc, o->opt_string_value));
        SC3E (sc3_allocator_free (alloc, &o->opt_string_value));
      }
    }
    SC3E (sc3_array_destroy (&yy->opts));

    /* deallocate internal state: free sub-options */
    SC3E (sc3_array_get_elem_count (yy->subs, &len));
    for (i = 0; i < len; ++i) {
      SC3E (sc3_array_index (yy->subs, i, &so));
      SC3E (sc3_options_unref (&so->sub));
    }
    SC3E (sc3_array_destroy (&yy->subs));

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

static sc3_error_t *
process_without_arg (sc3_option_t * o)
{
  SC3A_CHECK (o != NULL && !o->opt_has_arg);

  switch (o->opt_type) {
  case SC3_OPTION_SWITCH:
    ++*o->v.var_int;
    break;
  default:
    SC3E_UNREACH ("Invalid non-argument option");
  }
  return NULL;
}

static sc3_error_t *
process_with_arg (sc3_options_t * yy,
                  sc3_option_t * o, const char *at, int *success)
{
  long                lo;
  double              dr;
  char               *endptr;
  sc3_allocator_t    *subal;

  SC3A_CHECK (o != NULL && o->opt_has_arg && at != NULL);

  /* process the argument */
  switch (o->opt_type) {
  case SC3_OPTION_INT:
    lo = strtol (at, &endptr, 10);
    if (at[0] != '\0' && endptr[0] == '\0') {
#ifdef SC_HAVE_LIMITS_H
      if (!(lo >= INT_MIN && lo <= INT_MAX)) {
        /* unsuccessful match: variable out of bounds */
        return NULL;
      }
#endif
      *o->v.var_int = (int) lo;
      *success = 1;
    }
    break;
  case SC3_OPTION_DOUBLE:
    dr = strtod (at, &endptr);
    if (at[0] != '\0' && endptr[0] == '\0') {
      *o->v.var_double = dr;
      *success = 1;
    }
    break;
  case SC3_OPTION_STRING:
    /* use the original allocator for the string's value */
    subal = o->sub != NULL ? o->sub->alloc : yy->alloc;
    SC3E (sc3_allocator_free (subal, o->opt_string_value));
    SC3E (sc3_allocator_strdup (subal, at, o->opt_string_value));
    *o->v.var_string = *o->opt_string_value;
    *success = 1;
    break;
  default:
    SC3E_UNREACH ("Invalid non-argument option");
  }
  return NULL;
}

static sc3_error_t *
process_arg (sc3_options_t * yy,
             sc3_option_t * o, const char *at, int *success)
{
  SC3E_RETVAL (success, 0);
  SC3A_IS (sc3_options_is_setup, yy);
  SC3A_CHECK (o != NULL);
  SC3A_CHECK (!o->opt_has_arg == (at == NULL));

  if (at == NULL) {
    SC3E (process_without_arg (o));
    *success = 1;
  }
  else {
    SC3E (process_with_arg (yy, o, at, success));
  }
  return NULL;
}

static sc3_error_t *
sc3_options_parse_single (sc3_options_t * yy, int argc, char **argv,
                          int *argp, int *stop, int *result)
{
  int                 len, i;
  int                 success;
  size_t              lz, oll;
  const char         *at;
  sc3_option_t       *o;

  SC3A_IS (sc3_options_is_setup, yy);
  SC3A_CHECK (0 <= argc);
  SC3A_CHECK (argv != NULL);
  SC3A_CHECK (argp != NULL);
  SC3A_CHECK (0 <= *argp && *argp < argc);
  SC3A_CHECK (result != NULL);
  SC3A_CHECK (*result == 0);

  /* access all options in this container */
  SC3E (sc3_array_get_elem_count (yy->opts, &len));

  /* loop over command line arguments */
  for (; *argp < argc; ++*argp) {
    if ((at = argv[*argp]) == NULL) {
      /* treat NULL arguments as errors */
      *result = -1;
      return NULL;
    }
    if (stop != NULL && *stop) {
      /* we are not looking for options currently */
      return NULL;
    }
    lz = strlen (at);
    if (lz < 2 || at[0] != '-') {
      /* this is no kind of option, we have no match */
      return NULL;
    }

    /* long options start with a double dash */
    SC3A_CHECK (lz >= 2);
    if (!strncmp (at, "--", 2)) {
      if (lz == 2) {
        if (stop != NULL) {
          /* honor stop option */
          *stop = 1;
          ++*argp;
        }
        /* return due to stop or due to no match */
        return NULL;
      }
      SC3A_CHECK (lz > 2);
      lz -= 2;
      at += 2;

      /* parse long option */
      for (i = 0; i < len; ++i) {
        SC3E (sc3_array_index (yy->opts, i, &o));
        if ((oll = o->opt_long_len) == 0) {
          continue;
        }
        if (strncmp (at, o->opt_long, oll)) {
          continue;
        }
        SC3A_CHECK (lz >= oll);

        /* long option name too short.  Maybe another name matches */
        if (lz > oll && (!o->opt_has_arg || at[oll] != '=')) {
          continue;
        }
        lz -= oll;
        at += oll;

        /* long option is either invalid or successfully matched */
        if (!o->opt_has_arg) {
          /* first possibility: long option has no argument */
          SC3A_CHECK (lz == 0);
          at = NULL;
        }
        else if (lz > 0) {
          /* second possibility: long option has argument with '=' */
          SC3A_CHECK (at[0] == '=');
          --lz;
          ++at;
        }
        else {
          /* third possibility: long option has argument in next argument */
          SC3A_CHECK (lz == 0);
          SC3A_CHECK (*argp < argc);
          if (*argp + 1 == argc || (at = argv[++*argp]) == NULL) {
            /* argument expected but no valid argument coming */
            *result = -1;
            return NULL;
          }
        }

        /* error out earlier or now or match successfully */
        SC3E (process_arg (yy, o, at, &success));
        if (!success) {
          *result = -1;
          return NULL;
        }
        break;
      }
      if (i < len) {
        /* options loop above has found a match */
        ++*result;
#if 0
        continue;
#else
        /* we stop the loop after one successful match */
        ++*argp;
        return NULL;
#endif
      }

      /* invalid long option */
      *result = -1;
      return NULL;
    }
    else {
      /* parse short option */
      SC3A_CHECK (at[0] == '-');
      --lz;
      ++at;
      do {
        SC3A_CHECK (lz > 0);
        for (i = 0; i < len; ++i) {
          SC3E (sc3_array_index (yy->opts, i, &o));
          if (at[0] != o->opt_short) {
            continue;
          }
          --lz;
          ++at;

          /* first possibility: short option has no argument */
          if (!o->opt_has_arg) {
            SC3E (process_without_arg (o));
            break;
          }

          /* second possibility: short option with argument */
          if (lz == 0) {
            SC3A_CHECK (*argp < argc);
            if (*argp + 1 == argc || (at = argv[++*argp]) == NULL) {
              /* argument expected but no valid argument coming */
              *result = -1;
              return NULL;
            }
          }
          SC3E (process_arg (yy, o, at, &success));
          if (!success) {
            *result = -1;
            return NULL;
          }
          lz = 0;
          break;
        }
        if (i < len) {
          /* matched (another) short option */
          ++*result;
          continue;
        }

        /* invalid short option */
        *result = -1;
        return NULL;
      }
      while (lz > 0);
    }
    /* found a valid set of short options and proceed with next argument */
    SC3A_CHECK (*result > 0);

#if 1
    /* we stop the loop after one successful match */
    ++*argp;
    return NULL;
#endif
  }
  return NULL;
}

sc3_error_t        *
sc3_options_parse (sc3_options_t * yy, int argc, char **argv,
                   sc3_options_arg_t arg_cb, sc3_options_arg_t err_cb,
                   void *cb_user)
{
  int                 ccontin;
  int                 argp;
#ifdef SC_ENABLE_DEBUG
  int                 argp_in;
#endif
  int                 stop = 0;
  int                 result = 0;

  /* we move the argument index forward inside the loop */
  argp = 1;
  while (argp < argc) {
#ifdef SC_ENABLE_DEBUG
    argp_in = argp;
#endif

    SC3E (sc3_options_parse_single (yy, argc, argv, &argp, &stop, &result));
    SC3A_CHECK (argp <= argc);
    if (result <= 0 && argp < argc) {

      ccontin = 1;
      if (result == 0) {
        /* process one argument */
        if (arg_cb != NULL) {
          SC3E (arg_cb (&ccontin, argp, argv, cb_user));
        }
      }
      else {
        /* process one error */
        if (argv[argp] != NULL && err_cb != NULL) {
          SC3E (err_cb (&ccontin, argp, argv, cb_user));
        }
      }
      ++argp;
      if (!ccontin) {
        /* stop processing if indicated so by callback */
        break;
      }
    }
    result = 0;

    /* make sure we do not loop infinitely */
    SC3A_CHECK (argp > argp_in);
  }
  return NULL;
}

static sc3_error_t *
print_value (char *buf, int len, sc3_option_t * o)
{
  const char         *s;

  SC3A_CHECK (buf != NULL);
  SC3A_CHECK (o != NULL);

  /* process the argument */
  switch (o->opt_type) {
  case SC3_OPTION_SWITCH:
  case SC3_OPTION_INT:
    sc3_snprintf (buf, len, "%d", *o->v.var_int);
    break;
  case SC3_OPTION_DOUBLE:
    sc3_snprintf (buf, len, "%g", *o->v.var_double);
    break;
  case SC3_OPTION_STRING:
    s = *o->v.var_string;
    sc3_snprintf (buf, len, "%s", s == NULL ? "" : s);
    break;
  default:
    SC3E_UNREACH ("Invalid option type");
  }
  return NULL;
}

static sc3_error_t *
print_help (char *buf, int len, sc3_option_t * o)
{
  SC3A_CHECK (buf != NULL);
  SC3A_CHECK (o != NULL);
  SC3A_CHECK (0 <= o->opt_type && o->opt_type < SC3_OPTION_TYPE_LAST);

  sc3_snprintf (buf, len, "%-7s %s", opt_disp[o->opt_type],
                o->opt_help == NULL ? "" : o->opt_help);
  return NULL;
}

sc3_error_t        *
sc3_options_log_summary_help (sc3_options_t * yy,
                              sc3_log_t * logger, sc3_log_level_t lev,
                              int which)
{
  int                 len, i;
  char                lshort[3], llong[80], lvalue[160];
  sc3_option_t       *o;

  SC3A_IS (sc3_options_is_setup, yy);
  if (logger == NULL) {
    logger = sc3_log_new_static ();
  }
  SC3A_IS (sc3_log_is_setup, logger);
  SC3A_CHECK (0 <= lev && lev < SC3_LOG_LEVEL_LAST);
  SC3A_CHECK (which == 0 || which == 1);

  /* access all options in this container */
  SC3E (sc3_array_get_elem_count (yy->opts, &len));
  for (i = 0; i < len; ++i) {
    SC3E (sc3_array_index (yy->opts, i, &o));
    if (o->opt_short == '\0' && o->opt_long_len == 0) {
      continue;
    }

    /* prepare short option */
    if (o->opt_short == '\0') {
      sc3_snprintf (lshort, 3, "%2s", "");
    }
    else {
      sc3_snprintf (lshort, 3, "-%c", o->opt_short);
    }

    /* prepare long option */
    if (o->opt_long_len == 0) {
      sc3_snprintf (llong, 80, "%*s", yy->spacing + 3, "");
    }
    else {
      sc3_snprintf (llong, 80, "--%-*s", yy->spacing + 1, o->opt_long);
    }

    /* print the whole line */
    if (!which) {
      print_value (lvalue, 160, o);
    }
    else {
      print_help (lvalue, 160, o);
    }
    sc3_logf (logger, SC3_LOG_GLOBAL, lev, 0, "%s %s %s",
              lshort, llong, lvalue);
  }

  return NULL;
}

sc3_error_t        *
sc3_options_log_summary (sc3_options_t * yy,
                         sc3_log_t * logger, sc3_log_level_t lev)
{
  SC3E (sc3_options_log_summary_help (yy, logger, lev, 0));
  return NULL;
}

sc3_error_t        *
sc3_options_log_help (sc3_options_t * yy,
                      sc3_log_t * logger, sc3_log_level_t lev)
{
  SC3E (sc3_options_log_summary_help (yy, logger, lev, 1));
  return NULL;
}
