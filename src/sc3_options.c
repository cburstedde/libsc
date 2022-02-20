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
  SC3_OPTION_STRING,
  SC3_OPTION_TYPE_LAST
}
sc3_option_type_t;

static const char  *opt_disp[SC3_OPTION_TYPE_LAST] =
  { "SWITCH", "INT", "STRING" };

typedef struct sc3_option
{
  sc3_option_type_t   opt_type;
  char                opt_short;
  const char         *opt_long;
  size_t              opt_long_len;
  int                 opt_has_arg;
  const char         *opt_help;
  char               *opt_string_value;     /**< allocated string variable */
  union
  {
    /* address of current option value in caller memory */
    int                *var_int;
    const char        **var_string;
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
  int                 spacing;          /**< Space for value and type. */
  int                 allow_pack;       /**< Accept short options '-abc'. */
  int                *var_stop;         /**< Output variable for '--'. */
  sc3_array_t        *opts;
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
    case SC3_OPTION_STRING:
      SC3E_TEST (o->v.var_string != NULL, reason);
      SC3E_TEST (o->opt_string_value == *o->v.var_string, reason);
      break;
    default:
      SC3E_NO (reason, "Invalid option type");
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

  /* internal array of options */
  SC3E (sc3_array_new (alloc, &yy->opts));
  SC3E (sc3_array_set_elem_size (yy->opts, sizeof (sc3_option_t)));
  SC3E (sc3_array_set_initzero (yy->opts, 1));
  SC3E (sc3_array_setup (yy->opts));

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

sc3_error_t        *
sc3_options_set_stop (sc3_options_t * yy, int *var_stop)
{
  SC3A_IS (sc3_options_is_new, yy);
  if ((yy->var_stop = var_stop) != NULL) {
    *var_stop = 0;
  }
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

  /* array initializes to all zeros */
  SC3E (sc3_array_push (yy->opts, &o));
  SC3A_CHECK (!o->opt_has_arg);
  SC3A_CHECK (o->opt_string_value == NULL);

  /* set couple parameters of this option */
  o->opt_type = tt;
  o->opt_short = opt_short;
  o->opt_long = opt_long;
  o->opt_long_len = opt_long != NULL ? strlen (opt_long) : 0;
  o->opt_help = opt_help;

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
  SC3E (sc3_allocator_strdup (yy->alloc, opt_value, &o->opt_string_value));
  *(o->v.var_string = opt_variable) = o->opt_string_value;
  o->opt_has_arg = 1;
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
    int                 i, len;
    sc3_option_t       *o;

    *yyp = NULL;
    alloc = yy->alloc;

    /* deallocate internal state */
    SC3E (sc3_array_get_elem_count (yy->opts, &len));
    for (i = 0; i < len; ++i) {
      SC3E (sc3_array_index (yy->opts, i, &o));
      if (o->opt_type == SC3_OPTION_STRING) {
        SC3E (sc3_allocator_free (yy->alloc, &o->opt_string_value));
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
  char               *endptr;

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
  case SC3_OPTION_STRING:
    SC3E (sc3_allocator_free (yy->alloc, &o->opt_string_value));
    SC3E (sc3_allocator_strdup (yy->alloc, at, &o->opt_string_value));
    *o->v.var_string = o->opt_string_value;
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

sc3_error_t        *
sc3_options_parse (sc3_options_t * yy, int argc, char **argv,
                   int *argp, int *result)
{
  int                 len, i;
  int                 success;
  size_t              lz, oll;
  const char         *at;
  sc3_option_t       *o;

  SC3E_RETVAL (result, 0);
  SC3A_IS (sc3_options_is_setup, yy);
  SC3A_CHECK (0 <= argc);
  SC3A_CHECK (argv != NULL);
  SC3A_CHECK (argp != NULL);
  SC3A_CHECK (0 <= *argp && *argp < argc);

  /* access all options in this container */
  SC3E (sc3_array_get_elem_count (yy->opts, &len));

  /* loop over command line arguments */
  for (; *argp < argc; ++*argp) {
    if ((at = argv[*argp]) == NULL) {
      /* impossible parameter string is ignored */
      continue;
    }
    lz = strlen (at);
    if (lz < 2 || at[0] != '-') {
      /* this is no kind of option, we have no match and return */
      return NULL;
    }

    /* long options start with a double dash */
    SC3A_CHECK (lz >= 2);
    if (!strncmp (at, "--", 2)) {
      if (lz == 2) {
        if (yy->var_stop != NULL) {
          /* honor stop option */
          *yy->var_stop = 1;
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
          ++*argp;
          SC3A_CHECK (*argp <= argc);
          if (*argp == argc || (at = argv[*argp]) == NULL) {
            /* argument expected but no valid arguments left */
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
        continue;
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
            ++*argp;
            SC3A_CHECK (*argp <= argc);
            if (*argp == argc || (at = argv[*argp]) == NULL) {
              /* argument expected but no valid arguments left */
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
      sc3_snprintf (lshort, 3, "%3s", "");
    }
    else {
      sc3_snprintf (lshort, 3, "-%c", o->opt_short);
    }

    /* prepare long option */
    if (o->opt_long_len == 0) {
      sc3_snprintf (llong, 80, "%*s", yy->spacing + 2, "");
    }
    else {
      sc3_snprintf (llong, 80, "--%-*s", yy->spacing, o->opt_long);
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
