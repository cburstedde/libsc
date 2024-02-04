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

#include <sc_builtin/sc_getopt.h>

#include <sc_io.h>
#include <sc_options.h>
#include <sc_refcount.h>
#include <iniparser.h>

#include <errno.h>
#ifdef SC_HAVE_JSON
#include <jansson.h>
#endif

#define SC_OPTION_CALLBACK_NULL ((sc_options_callback_t) 0)

typedef enum
{
  SC_OPTION_SWITCH,
  SC_OPTION_BOOL,
  SC_OPTION_INT,
  SC_OPTION_SIZE_T,
  SC_OPTION_DOUBLE,
  SC_OPTION_STRING,
  SC_OPTION_INIFILE,
  SC_OPTION_JSONFILE,
  SC_OPTION_CALLBACK,
  SC_OPTION_KEYVALUE
}
sc_option_type_t;

typedef struct
{
  const char        **string_var;
  char               *string_value;
  sc_refcount_t       rc;
}
sc_option_string_t;

typedef struct
{
  sc_option_type_t    opt_type;
  int                 opt_char;
  const char         *opt_name;
  void               *opt_var;
  sc_options_callback_t opt_fn;
  int                 has_arg;
  int                 called;           /**< set to 0 and ignored */
  const char         *help_string;
  char               *string_value;     /**< set on call but ignored */
  void               *user_data;
}
sc_option_item_t;

struct sc_options
{
  char                program_path[BUFSIZ];
  const char         *program_name;
  sc_array_t         *option_items;
  int                 max_bytes;
  int                 collective;
  int                 set_collective_explicit;
  int                 space_type;
  int                 space_help;
  int                 args_alloced;
  int                 first_arg;
  int                 argc;
  char              **argv;
  sc_array_t         *subopt_names;
};

static char        *sc_iniparser_invalid_key = (char *) -1;

static const int    sc_options_space_type = 20;
static const int    sc_options_space_help = 32;
static const int    sc_options_max_bytes = 1 << 20;

static int
sc_options_log_category (sc_options_t *opt)
{
  /* decide whether all ranks print a message or only root does */
  int                 log_category;

  /* check proper calling and consistent collective settings */
  SC_ASSERT (opt != NULL);

  /* Historically the collective default is false.
     It may only be changed to true by explicitly setting it. */
  SC_ASSERT (!opt->collective || opt->set_collective_explicit);

  /* log on root rank or replicated on all: historic default */
  log_category = SC_LC_GLOBAL;
  if (opt->set_collective_explicit) {
    /* only when using the updated interface do we change the default */
    if (!opt->collective) {
      log_category = SC_LC_NORMAL;
    }
  }

  /* this category is for use with functions that may act collectively */
  return log_category;
}

static int
sc_options_get_collective (sc_options_t *opt)
{
  SC_ASSERT (opt != NULL);

  /* we act truly collective only when it is explicitly enabled */
  return opt->set_collective_explicit && opt->collective;
}

static int
sc_iniparser_getint (dictionary * d, const char *key, int notfound,
                     int *iserror)
{
  char               *str;
  long                l;

  str = iniparser_getstring (d, key, sc_iniparser_invalid_key);
  if (str == sc_iniparser_invalid_key) {
    return notfound;
  }
  l = strtol (str, NULL, 0);
  if (iserror != NULL) {
    *iserror = (errno == ERANGE);
  }
  if (l < (long) INT_MIN) {
    if (iserror != NULL) {
      *iserror = 1;
    }
    return INT_MIN;
  }
  if (l > (long) INT_MAX) {
    if (iserror != NULL) {
      *iserror = 1;
    }
    return INT_MAX;
  }
  return (int) l;
}

static size_t
sc_iniparser_getsizet (dictionary * d, const char *key, size_t notfound,
                       int *iserror)
{
  char               *str;
  long long           ll;

  str = iniparser_getstring (d, key, sc_iniparser_invalid_key);
  if (str == sc_iniparser_invalid_key) {
    return notfound;
  }
#ifndef SC_HAVE_STRTOLL
  ll = (long long) strtol (str, NULL, 0);
#else
  ll = strtoll (str, NULL, 0);
#endif
  if (iserror != NULL) {
    *iserror = (errno == ERANGE);
  }
  if (ll < 0LL) {
    if (iserror != NULL) {
      *iserror = 1;
    }
    return 0;
  }
  return (size_t) ll;
}

static double
sc_iniparser_getdouble (dictionary * d, const char *key, double notfound,
                        int *iserror)
{
  char               *str;
  double              dbl;

  str = iniparser_getstring (d, key, sc_iniparser_invalid_key);
  if (str == sc_iniparser_invalid_key) {
    return notfound;
  }
  dbl = strtod (str, NULL);
  if (iserror != NULL) {
    *iserror = (errno == ERANGE);
  }
  return dbl;
}

static sc_option_string_t *
sc_options_string_new (const char **variable, const char *init_value)
{
  sc_option_string_t *s = SC_ALLOC (sc_option_string_t, 1);

  /* init_value may be NULL */
  *(s->string_var = variable) = s->string_value = SC_STRDUP (init_value);
  sc_refcount_init (&s->rc, sc_get_package_id ());

  return s;
}

static sc_option_string_t *
sc_options_string_ref (sc_option_string_t *s)
{
  SC_ASSERT (s != NULL);
  SC_ASSERT (sc_refcount_is_active (&s->rc));

  sc_refcount_ref (&s->rc);
  return s;
}

static void
sc_options_string_unref (sc_option_string_t *s)
{
  SC_ASSERT (s != NULL);
  SC_ASSERT (sc_refcount_is_active (&s->rc));

  if (sc_refcount_unref (&s->rc)) {
    *s->string_var = "corresponding options structure has been destroyed";
    SC_FREE (s->string_value);
    SC_FREE (s);
  }
}

static const char *
sc_options_string_get (sc_option_string_t *s)
{
  SC_ASSERT (s != NULL);
  SC_ASSERT (sc_refcount_is_active (&s->rc));

  /* the pointed-to variable may have been modified externally */
  if ((*s->string_var != NULL) != (s->string_value != NULL) ||
      (*s->string_var != NULL && s->string_value != NULL &&
       strcmp (*s->string_var, s->string_value))) {
    SC_FREE (s->string_value);
    s->string_value = SC_STRDUP (*s->string_var);
  }
  return s->string_value;
}

static void
sc_options_string_set (sc_option_string_t *s, const char *newval)
{
  SC_ASSERT (s != NULL);
  SC_ASSERT (sc_refcount_is_active (&s->rc));

  SC_FREE (s->string_value);
  *s->string_var = s->string_value = SC_STRDUP (newval);
}

static void
sc_options_free_args (sc_options_t * opt)
{
  int                 i;

  if (opt->args_alloced) {
    SC_ASSERT (opt->first_arg == 0);
    for (i = 0; i < opt->argc; ++i) {
      SC_FREE (opt->argv[i]);
    }
    SC_FREE (opt->argv);
  }

  opt->args_alloced = 0;
  opt->first_arg = 0;
  opt->argc = 0;
  opt->argv = NULL;
}

sc_options_t       *
sc_options_new (const char *program_path)
{
  sc_options_t       *opt;

  opt = SC_ALLOC_ZERO (sc_options_t, 1);

  snprintf (opt->program_path, BUFSIZ, "%s", program_path);
#ifdef SC_HAVE_LIBGEN_H
  opt->program_name = basename (opt->program_path);
#else
  opt->program_name = opt->program_path;
#endif
  opt->option_items = sc_array_new (sizeof (sc_option_item_t));
  opt->subopt_names = sc_array_new (sizeof (char *));
  opt->args_alloced = 0;
  opt->first_arg = -1;
  opt->argc = 0;
  opt->argv = NULL;

  /* set default number of bytes to accept in .ini/JSON files */
  opt->max_bytes = sc_options_max_bytes;

  /* set backwards compatible defaults.
   * we activate new functionality when explicitly setting collective,
   * no matter to which value, by calling sc_options_set_collective. */
  opt->collective = 0;
  opt->set_collective_explicit = 0;

  /* set default spacing for printing option summary */
  sc_options_set_spacing (opt, -1, -1);

  return opt;
}

static void
sc_options_destroy_internal (sc_options_t * opt, int deep)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  sc_array_t         *names = opt->subopt_names;
  char               *name;

  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);

    /* special handling for key-value pairs */
    if (deep && item->opt_type == SC_OPTION_KEYVALUE) {
      sc_keyvalue_destroy ((sc_keyvalue_t *) item->user_data);
    }
    SC_FREE (item->string_value);

    /* the string type is reference counted */
    if (item->opt_type == SC_OPTION_STRING) {
      sc_option_string_t *s = (sc_option_string_t *) item->opt_var;
      SC_ASSERT (s != NULL && sc_refcount_is_active (&s->rc));
      sc_options_string_unref (s);
      SC_ASSERT (item->string_value == NULL);
    }
  }

  sc_options_free_args (opt);
  sc_array_destroy (opt->option_items);

  count = names->elem_count;
  for (iz = 0; iz < count; iz++) {
    name = *((char **) sc_array_index (names, iz));
    SC_FREE (name);
  }
  sc_array_destroy (opt->subopt_names);

  SC_FREE (opt);
}

void
sc_options_destroy_deep (sc_options_t * opt)
{
  sc_options_destroy_internal (opt, 1);
}

void
sc_options_destroy (sc_options_t * opt)
{
  sc_options_destroy_internal (opt, 0);
}

void
sc_options_set_collective (sc_options_t * opt, int enable)
{
  SC_ASSERT (opt != NULL);

  /* this function is newly added to enable a consistent collective mode */
  opt->collective = enable;
  opt->set_collective_explicit = 1;
}

void
sc_options_set_spacing (sc_options_t * opt, int space_type, int space_help)
{
  SC_ASSERT (opt != NULL);

  opt->space_type = space_type < 0 ? sc_options_space_type : space_type;
  opt->space_help = space_help < 0 ? sc_options_space_help : space_help;
}

static sc_option_item_t *
sc_options_add_item (sc_options_t * opt, int opt_char, const char *opt_name,
                     sc_option_type_t opt_type, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt != NULL);
  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);
  memset (item, 0, sizeof (sc_option_item_t));

  item->opt_type = opt_type;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->help_string = help_string;

  return item;
}

void
sc_options_add_switch (sc_options_t * opt, int opt_char,
                       const char *opt_name,
                       int *variable, const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_SWITCH,
                                                  help_string);

  item->opt_var = variable;
  *variable = 0;
}

void
sc_options_add_bool (sc_options_t * opt, int opt_char,
                     const char *opt_name,
                     int *variable, int init_value, const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_BOOL,
                                                  help_string);

  item->has_arg = 2;
  item->opt_var = variable;
  *variable = init_value;
}

void
sc_options_add_int (sc_options_t * opt, int opt_char, const char *opt_name,
                    int *variable, int init_value, const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_INT, help_string);

  item->has_arg = 1;
  item->opt_var = variable;
  *variable = init_value;
}

void
sc_options_add_size_t (sc_options_t * opt, int opt_char, const char *opt_name,
                       size_t *variable, size_t init_value,
                       const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_SIZE_T,
                                                  help_string);

  item->has_arg = 1;
  item->opt_var = variable;
  *variable = init_value;
}

void
sc_options_add_double (sc_options_t * opt, int opt_char,
                       const char *opt_name,
                       double *variable, double init_value,
                       const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_DOUBLE,
                                                  help_string);

  item->has_arg = 1;
  item->opt_var = variable;
  *variable = init_value;
}

static void
sc_options_add_item_string (sc_options_t * opt,
                            int opt_char, const char *opt_name,
                            sc_option_string_t *s, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (s != NULL && sc_refcount_is_active (&s->rc));
  item = sc_options_add_item (opt, opt_char, opt_name,
                              SC_OPTION_STRING, help_string);
  item->has_arg = 1;
  item->opt_var = s;
}

void
sc_options_add_string (sc_options_t * opt, int opt_char,
                       const char *opt_name, const char **variable,
                       const char *init_value, const char *help_string)
{
  sc_options_add_item_string (opt, opt_char, opt_name,
                              sc_options_string_new (variable, init_value),
                              help_string);
}

void
sc_options_add_inifile (sc_options_t * opt, int opt_char,
                        const char *opt_name, const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_INIFILE,
                                                  help_string);
  item->has_arg = 1;
}

void
sc_options_add_jsonfile (sc_options_t * opt, int opt_char,
                         const char *opt_name, const char *help_string)
{
  sc_option_item_t   *item = sc_options_add_item (opt, opt_char, opt_name,
                                                  SC_OPTION_JSONFILE,
                                                  help_string);
  item->has_arg = 1;
}

void
sc_options_add_callback (sc_options_t * opt, int opt_char,
                         const char *opt_name, int has_arg,
                         sc_options_callback_t fn, void *data,
                         const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (fn != SC_OPTION_CALLBACK_NULL);

  item = sc_options_add_item (opt, opt_char, opt_name,
                              SC_OPTION_CALLBACK, help_string);
  item->has_arg = has_arg;
  item->opt_fn = fn;
  item->user_data = data;
}

void
sc_options_add_keyvalue (sc_options_t * opt,
                         int opt_char, const char *opt_name,
                         int *variable, const char *init_value,
                         sc_keyvalue_t * keyvalue, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  /* we do not accept an invalid default value */
  SC_ASSERT (variable != NULL);
  SC_ASSERT (init_value != NULL);
  SC_ASSERT (keyvalue != NULL);

  item = sc_options_add_item (opt, opt_char, opt_name,
                              SC_OPTION_KEYVALUE, help_string);
  item->has_arg = 1;
  item->opt_var = variable;
  item->user_data = keyvalue;

  /* we expect that the key points to a valid integer entry by design */
  *variable = sc_keyvalue_get_int_check (keyvalue, init_value, NULL);
  item->string_value = SC_STRDUP (init_value);
}

void
sc_options_add_suboptions (sc_options_t * opt,
                           sc_options_t * subopt, const char *prefix)
{
  sc_array_t         *subopt_names = opt->subopt_names;
  sc_array_t         *items = subopt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  sc_option_string_t *s;
  size_t              iz;
  int                 prefixlen = strlen (prefix);
  int                 namelen;
  char              **name;

  for (iz = 0; iz < count; iz++) {
    item = (sc_option_item_t *) sc_array_index (items, iz);

    namelen = prefixlen +
      ((item->opt_name != NULL) ? (strlen (item->opt_name) + 2) : 4);
    name = (char **) sc_array_push (subopt_names);
    *name = SC_ALLOC (char, namelen);
    if (item->opt_name != NULL) {
      snprintf (*name, namelen, "%s:%s", prefix, item->opt_name);
    }
    else {
      snprintf (*name, namelen, "%s:-%c", prefix, item->opt_char);
    }

    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      sc_options_add_switch (opt, '\0', *name, (int *) item->opt_var,
                             item->help_string);
      break;
    case SC_OPTION_BOOL:
      sc_options_add_bool (opt, '\0', *name, (int *) item->opt_var,
                           *((int *) item->opt_var), item->help_string);
      break;
    case SC_OPTION_INT:
      sc_options_add_int (opt, '\0', *name, (int *) item->opt_var,
                          *((int *) item->opt_var), item->help_string);
      break;
    case SC_OPTION_SIZE_T:
      sc_options_add_size_t (opt, '\0', *name, (size_t *) item->opt_var,
                             *((size_t *) item->opt_var), item->help_string);
      break;
    case SC_OPTION_DOUBLE:
      sc_options_add_double (opt, '\0', *name, (double *) item->opt_var,
                             *((double *) item->opt_var), item->help_string);
      break;
    case SC_OPTION_STRING:
      s = sc_options_string_ref ((sc_option_string_t *) item->opt_var);
      sc_options_add_item_string (opt, '\0', *name, s, item->help_string);
      break;
    case SC_OPTION_INIFILE:
      sc_options_add_inifile (opt, '\0', *name, item->help_string);
      break;
    case SC_OPTION_JSONFILE:
      sc_options_add_jsonfile (opt, '\0', *name, item->help_string);
      break;
    case SC_OPTION_CALLBACK:
      sc_options_add_callback (opt, '\0', *name,
                               item->has_arg, item->opt_fn,
                               item->user_data, item->help_string);
      break;
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      sc_options_add_keyvalue (opt, '\0', *name,
                               (int *) item->opt_var, item->string_value,
                               (sc_keyvalue_t *) item->user_data,
                               item->help_string);
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
  }
}

void
sc_options_print_usage (int package_id, int log_priority,
                        sc_options_t * opt, const char *arg_usage)
{
  /* this function may implictly or explicitly print collectively */
  const int           log_category = sc_options_log_category (opt);
  int                 printed;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  const char         *provide;
  const char         *separator;
  char                outbuf[BUFSIZ];
  char               *copy, *tok;

  /* begin outputting a block of usage message */
  SC_GEN_LOGF (package_id, log_category, log_priority,
               "Usage: %s%s%s\n", opt->program_name,
               count == 0 ? "" : " <OPTIONS>",
               arg_usage == NULL ? "" : " <ARGUMENTS>");
  if (count > 0) {
    SC_GEN_LOG (package_id, log_category, log_priority, "Options:\n");
  }

  /* we print one line for each option variable */
  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    provide = "";
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      break;
    case SC_OPTION_BOOL:
      provide = "[0fFnN1tTyY]";
      break;
    case SC_OPTION_INT:
      provide = "<INT>";
      break;
    case SC_OPTION_SIZE_T:
      provide = "<SIZE_T>";
      break;
    case SC_OPTION_DOUBLE:
      provide = "<REAL>";
      break;
    case SC_OPTION_STRING:
      provide = "<STRING>";
      break;
    case SC_OPTION_INIFILE:
      provide = "<INIFILE>";
      break;
    case SC_OPTION_JSONFILE:
      provide = "<JSONFILE>";
      break;
    case SC_OPTION_CALLBACK:
      if (item->has_arg) {
        provide = item->has_arg == 2 ? "[<ARG>]" : "<ARG>";
      }
      break;
    case SC_OPTION_KEYVALUE:
      provide = "<CHOICE>";
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
#if 0
    separator = item->has_arg ? "=" : "";
#else
    separator = "";
#endif
    outbuf[0] = '\0';
    printed = 0;
    if (item->opt_char != '\0' && item->opt_name != NULL) {
      printed +=
        snprintf (outbuf + printed, BUFSIZ - printed, "   -%c | --%s%s",
                  item->opt_char, item->opt_name, separator);
    }
    else if (item->opt_char != '\0') {
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "   -%c",
                           item->opt_char);
    }
    else if (item->opt_name != NULL) {
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "   --%s%s",
                           item->opt_name, separator);
    }
    else {
      SC_ABORT_NOT_REACHED ();
    }
    printed += snprintf (outbuf + printed, BUFSIZ - printed, "%*s%s",
                         SC_MAX (1, opt->space_type - printed), "", provide);
    if (item->help_string != NULL) {
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "%*s%s",
                           SC_MAX (1, opt->space_help - printed), "",
                           item->help_string);
    }
    SC_GEN_LOGF (package_id, log_category, log_priority, "%s\n", outbuf);
  }

  /* we also print usage for the non-option arguments */
  if (arg_usage != NULL && arg_usage[0] != '\0') {
    SC_GEN_LOG (package_id, log_category, log_priority, "Arguments:\n");
    copy = SC_STRDUP (arg_usage);
    for (tok = strtok (copy, "\n\r"); tok != NULL;
         tok = strtok (NULL, "\n\r")) {
      SC_GEN_LOGF (package_id, log_category, log_priority, "   %s\n", tok);
    }
    SC_FREE (copy);
  }
}

void
sc_options_print_summary (int package_id, int log_priority,
                          sc_options_t * opt)
{
  /* this function may implictly or explicitly print collectively */
  const int           log_category = sc_options_log_category (opt);
  int                 i;
  int                 bvalue, printed;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  const char         *s;
  char                outbuf[BUFSIZ];

  /* begin printing a multi-line block of option variable values */
  SC_GEN_LOG (package_id, log_category, log_priority, "Options:\n");

  /* we print one line for each option variable */
  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE ||
        item->opt_type == SC_OPTION_JSONFILE ||
        item->opt_type == SC_OPTION_CALLBACK) {
      continue;
    }
    printed = 0;
    if (item->opt_name == NULL) {
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "   -%c",
                           item->opt_char);
    }
    else {
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "   %s",
                           item->opt_name);
    }
    printed += snprintf (outbuf + printed, BUFSIZ - printed, "%*s",
                         SC_MAX (1, opt->space_type - printed), "");
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      bvalue = *(int *) item->opt_var;
      if (bvalue <= 1)
        printed += snprintf (outbuf + printed, BUFSIZ - printed,
                             "%s", bvalue ? "true" : "false");
      else
        printed += snprintf (outbuf + printed, BUFSIZ - printed,
                             "%d", bvalue);
      break;
    case SC_OPTION_BOOL:
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%s", *(int *) item->opt_var ? "true" : "false");
      break;
    case SC_OPTION_INT:
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%d", *(int *) item->opt_var);
      break;
    case SC_OPTION_SIZE_T:
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "%llu",
                           (unsigned long long) *(size_t *) item->opt_var);
      break;
    case SC_OPTION_DOUBLE:
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%g", *(double *) item->opt_var);
      break;
    case SC_OPTION_STRING:
      s = sc_options_string_get ((sc_option_string_t *) item->opt_var);
      if (s == NULL) {
        s = "<unspecified>";
      }
      printed += snprintf (outbuf + printed, BUFSIZ - printed, "%s", s);
      break;
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%s", item->string_value);
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
    SC_GEN_LOGF (package_id, log_category, log_priority, "%s\n", outbuf);
  }

  /* print a summary for each non-option argument */
  if (opt->first_arg < 0) {
    SC_GEN_LOG (package_id, log_category, log_priority,
                "Arguments: not parsed\n");
  }
  else {
    if (opt->first_arg == opt->argc) {
      SC_GEN_LOG (package_id, log_category, log_priority,
                  "Arguments: none\n");
    }
    else {
      SC_GEN_LOG (package_id, log_category, log_priority, "Arguments:\n");
    }
    for (i = opt->first_arg; i < opt->argc; ++i) {
      SC_GEN_LOGF (package_id, log_category, log_priority, "   %d: %s\n",
                   i - opt->first_arg, opt->argv[i]);
    }
  }
}

static dictionary  *
sc_iniparser_load (const char *inifile, int max_bytes, int collective)
{
  /* string holds name of file to load */
  SC_ASSERT (inifile != NULL);

  /* the collective switch is newly added.  If true, assume it's intentional */
  if (!collective) {
    /* each process reads the same file at the same time */
    return iniparser_load (inifile);
  }
  else {
    /* in this block we can expect to be called collectively */
    sc_array_t          arr;
    dictionary         *dic;

    /* read file on rank zero and broadcast contents */
    sc_array_init (&arr, 1);
    if (sc_io_file_bcast (inifile, &arr, max_bytes, 0, sc_get_comm ())) {
      SC_GLOBAL_LERRORF ("Error bcasting file %s\n", inifile);
      sc_array_reset (&arr);
      return NULL;
    }

    /* populate dictionary identically and free buffer afterwards */
    dic = iniparser_load_buffer ((const char *) sc_array_index (&arr, 0),
                                 arr.elem_count, inifile);
    sc_array_reset (&arr);
    return dic;
  }
}

int
sc_options_load (int package_id, int err_priority,
                 sc_options_t * opt, const char *file)
{
  /* expect the .ini format if nothing else changes the historic default */
  return sc_options_load_ini (package_id, err_priority, opt, file, NULL);
}

int
sc_options_load_ini (int package_id, int err_priority,
                     sc_options_t * opt, const char *inifile, void *re)
{
  /* this function can be configured wrt. collective calling */
  const int           log_category = sc_options_log_category (opt);
  int                 found_short, found_long;
  size_t              iz;
  size_t              count;
  sc_array_t         *items;
  sc_option_item_t   *item;
  dictionary         *dict;
  int                 iserror;
  int                 bvalue;
  int                *ivalue;
  double             *dvalue;
  size_t             *zvalue;
  const char         *s, *key;
  char                skey[BUFSIZ], lkey[BUFSIZ];

  /* this function is a historic entry point */
  SC_ASSERT (opt != NULL);
  SC_ASSERT (inifile != NULL);

  /* prepare for runtime error checking implementation */
  SC_ASSERT (re == NULL);

  /* read .ini file in one go */
  dict = sc_iniparser_load (inifile, opt->max_bytes,
                            sc_options_get_collective (opt));
  if (dict == NULL) {
    SC_GEN_LOG (package_id, log_category, err_priority,
                "Could not load or parse .ini file\n");
    return -1;
  }

  /* loop through option items */
  items = opt->option_items;
  count = items->elem_count;
  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE ||
        item->opt_type == SC_OPTION_JSONFILE ||
        item->opt_type == SC_OPTION_CALLBACK) {
      continue;
    }

    /* check existence of key */
    key = NULL;
    skey[0] = lkey[0] = '\0';
    found_short = found_long = 0;
    if (item->opt_char != '\0') {
      snprintf (skey, BUFSIZ, "Options:-%c", item->opt_char);
      found_short = iniparser_find_entry (dict, skey);
    }
    if (item->opt_name != NULL) {
      /* if the name contains a section prefix, don't add "Options:" */
      if (strchr (item->opt_name, ':') != NULL) {
        SC_ASSERT (item->opt_char == '\0');
        snprintf (lkey, BUFSIZ, "%s", item->opt_name);
      }
      else {
        snprintf (lkey, BUFSIZ, "Options:%s", item->opt_name);
      }
      found_long = iniparser_find_entry (dict, lkey);
    }
    if (found_short && found_long) {
      SC_GEN_LOGF (package_id, log_category, err_priority,
                   "Duplicates %s %s in file: %s\n", skey, lkey, inifile);
      iniparser_freedict (dict);
      return -1;
    }
    else if (found_long) {
      key = lkey;
    }
    else if (found_short) {
      key = skey;
    }
    else {
      continue;
    }

    /* access value by key */
    ++item->called;
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      bvalue = iniparser_getboolean (dict, key, -1);
      if (bvalue == -1) {
        bvalue = sc_iniparser_getint (dict, key, 0, &iserror);
        if (bvalue <= 0 || iserror) {
          SC_GEN_LOGF (package_id, log_category, err_priority,
                       "Invalid switch %s in file: %s\n", key, inifile);
          iniparser_freedict (dict);
          return -1;
        }
      }
      *(int *) item->opt_var = bvalue;
      break;
    case SC_OPTION_BOOL:
      bvalue = iniparser_getboolean (dict, key, -1);
      if (bvalue == -1) {
        SC_GEN_LOGF (package_id, log_category, err_priority,
                     "Invalid boolean %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      *(int *) item->opt_var = bvalue;
      break;
    case SC_OPTION_INT:
      ivalue = (int *) item->opt_var;
      *ivalue = sc_iniparser_getint (dict, key, *ivalue, &iserror);
      if (iserror) {
        SC_GEN_LOGF (package_id, log_category, err_priority,
                     "Invalid int %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      break;
    case SC_OPTION_SIZE_T:
      zvalue = (size_t *) item->opt_var;
      *zvalue = sc_iniparser_getsizet (dict, key, *zvalue, &iserror);
      if (iserror) {
        SC_GEN_LOGF (package_id, log_category, err_priority,
                     "Invalid size_t %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      break;
    case SC_OPTION_DOUBLE:
      dvalue = (double *) item->opt_var;
      *dvalue = sc_iniparser_getdouble (dict, key, *dvalue, &iserror);
      if (iserror) {
        SC_GEN_LOGF (package_id, log_category, err_priority,
                     "Invalid double %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      break;
    case SC_OPTION_STRING:
      s = iniparser_getstring (dict, key, NULL);
      if (s != NULL) {
        sc_options_string_set ((sc_option_string_t *) item->opt_var, s);
      }
      break;
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      s = iniparser_getstring (dict, key, NULL);
      if (s != NULL) {
        /* lookup the key and see if the result is valid */
        iserror = *(ivalue = (int *) item->opt_var);
        *ivalue = sc_keyvalue_get_int_check ((sc_keyvalue_t *)
                                             item->user_data, s, &iserror);
        if (iserror) {
          /* key not found or of the wrong type; this cannot be ignored */
          SC_GEN_LOGF (package_id, log_category, err_priority,
                       "Invalid keyvalue %s for option %s in file: %s\n",
                       s, key, inifile);
          iniparser_freedict (dict);
          return -1;
        }
        SC_FREE (item->string_value);
        item->string_value = SC_STRDUP (s);
      }
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
  }

  iniparser_freedict (dict);
  return 0;
}

#ifdef SC_HAVE_JSON

/** Look up a key, possibly with ':' hierarchy markers, in a JSON object.
 *
 * We look up each substring between ':'s as key to a sub-object and
 * continue recursively.  If a key is not found, we try the concatenation
 * with the next substring as key, and so forth.
 * Values found at deeper levels of recursion take precedence.
 *
 * This allows for both a hierarchical and a flat JSON representation.
 *
 * \param [in] object   Must be a JSON object.
 * \param [in] key      NUL-terminated string.
 * \return              NULL if key not found in \a object
 *                      and a borrowed reference to the value otherwise.
 */
static json_t      *
sc_options_json_lookup (json_t *object, const char *keystring)
{
  int                 ended;
  size_t              len;
  const char         *begp, *midp, *endp;
  char               *key;
  json_t             *entry, *recurse;

  /* initial consistency checks */
  SC_ASSERT (json_is_object (object));
  SC_ASSERT (keystring != NULL);

  /* setup first sub string and loop over contents */
  begp = midp = keystring;
  ended = 0;
  for (;;) {
    /* search substring ending before the next ':' */
    if ((endp = strchr (midp, ':')) == NULL) {
      len = strlen (begp);
      ended = 1;
    }
    else {
      len = (size_t) (endp - begp);
      SC_ASSERT (len < strlen (begp));
      SC_ASSERT (endp[0] != '\0');
    }

    /* consider substring as key */
    if (len == 0) {
      /* empty keys are not considered */
      entry = NULL;
    }
    else {
      /* conduct proper member search */
      key = SC_ALLOC (char, len + 1);
      sc_strcopy (key, len + 1, begp);
      entry = json_object_get (object, key);
      SC_FREE (key);
    }

    /* deal with result of lookup */
    if (entry == NULL) {
      if (ended) {
        /* an empty string cannot be found */
        return NULL;
      }
      else {
        /* concatenate same string across the ':' symbol */
        ++midp;
        continue;
      }
    }
    else {
      /* key exists in object */
      if (ended) {
        /* we have retrieved a value from the entire key */
        return entry;
      }
      else {
        /* examine the value just found */
        if (!json_is_object (entry)) {
          /* key must be an object in order to continue searching */
          return NULL;
        }
        else if ((recurse = sc_options_json_lookup (entry, endp + 1))
                 != NULL) {
          /* recursive lookup has succeeded */
          return recurse;
        }
        else {
          /* continue with next loop iteration and advanced key */
          SC_ASSERT (endp != NULL);
          midp = endp + 1;
        }
      }
    }
  }
}

#endif /* SC_HAVE_JSON */

int
sc_options_load_json (int package_id, int err_priority,
                      sc_options_t *opt, const char *jsonfile, void *re)
{
  int                 retval = -1;
#ifndef SC_HAVE_JSON
  SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
              "JSON not configured: could not parse input file\n");
#else
  int                 iserror;
  int                 bvalue, ivalue;
  double              dvalue;
  size_t              iz, zvalue;
  size_t              count;
  sc_array_t         *items;
  sc_option_item_t   *item;
  json_t             *file;
  json_t             *jopt;
  json_t             *jval, *jv2;
  json_int_t          jint;
  json_error_t        jerr;
  const char         *s, *key;
  char                skey[BUFSIZ];

  SC_ASSERT (opt != NULL);
  SC_ASSERT (jsonfile != NULL);

  /* prepare for runtime error checking implementation */
  SC_ASSERT (re == NULL);

  /* read JSON file in one go */
  file = NULL;
  if ((file = json_load_file (jsonfile, 0, &jerr)) == NULL) {
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                 "Could not load or parse JSON file %s line %d column %d\n",
                 jerr.source, jerr.line, jerr.column);
    goto load_json_error;
  }
  if ((jopt = json_object_get (file, "Options")) == NULL) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Could not find options entry\n");
    goto load_json_error;
  }
  if (!json_is_object (jopt)) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Could not access options object\n");
    goto load_json_error;
  }

  /* loop through option items */
  items = opt->option_items;
  count = items->elem_count;
  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE ||
        item->opt_type == SC_OPTION_JSONFILE ||
        item->opt_type == SC_OPTION_CALLBACK) {
      continue;
    }

    /* try to retrieve by short option character, then by long name */
    /* since JSON generally allows for duplicate keys, we take the same
     * approach: we look for short option and possibly replace by long */
    key = NULL;
    jval = NULL;
    if (item->opt_char != '\0') {
      snprintf (skey, BUFSIZ, "-%c", item->opt_char);
      key = skey;
      jval = json_object_get (jopt, key);
    }
    if (item->opt_name != NULL) {
      key = item->opt_name;
      if ((jv2 = sc_options_json_lookup (jopt, key)) != NULL) {
        jval = jv2;
      }
    }
    if (jval == NULL) {
      continue;
    }
    SC_ASSERT (key != NULL);

    /* each option type has an individual interpretation of the value */
    ++item->called;
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      if (json_is_boolean (jval)) {
        bvalue = json_is_true (jval);
      }
      else if (json_is_integer (jval) &&
               (bvalue = (int) json_integer_value (jval)) >= 0) {
        /* switch may have values larger than 1 */
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid switch %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      *(int *) item->opt_var = bvalue;
      break;
    case SC_OPTION_BOOL:
      if (json_is_boolean (jval)) {
        bvalue = json_is_true (jval);
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid boolean %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      *(int *) item->opt_var = bvalue;
      break;
    case SC_OPTION_INT:
      if (json_is_integer (jval) &&
          (jint = json_integer_value (jval)) >= (json_int_t) INT_MIN &&
          jint <= (json_int_t) INT_MAX) {
        ivalue = (int) jint;
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid int %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      *(int *) item->opt_var = ivalue;
      break;
    case SC_OPTION_SIZE_T:
      if (json_is_integer (jval) && (jint = json_integer_value (jval)) >= 0) {
        zvalue = (size_t) jint;
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid size_t %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      *(size_t *) item->opt_var = zvalue;
      break;
    case SC_OPTION_DOUBLE:
      if (json_is_number (jval)) {
        if (json_is_integer (jval)) {
          dvalue = (double) json_integer_value (jval);
        }
        else {
          SC_ASSERT (json_is_real (jval));
          dvalue = json_real_value (jval);
        }
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid double %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      *(double *) item->opt_var = dvalue;
      break;
    case SC_OPTION_STRING:
      if (json_is_null (jval)) {
        s = NULL;
      }
      else if (json_is_string (jval)) {
        s = json_string_value (jval);
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid string %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      sc_options_string_set ((sc_option_string_t *) item->opt_var, s);
      break;
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      if (json_is_string (jval)) {
        /* we must find a string and not the null value */
        s = json_string_value (jval);
        SC_ASSERT (s != NULL);

        /* lookup the key and see if the result is valid */
        iserror = *(int *) item->opt_var;
        ivalue = sc_keyvalue_get_int_check ((sc_keyvalue_t *)
                                            item->user_data, s, &iserror);
        if (iserror) {
          /* key not found or of the wrong type; this cannot be ignored */
          SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                       "Invalid keyvalue %s for option %s in file: %s\n",
                       s, key, jsonfile);
          goto load_json_error;
        }
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid key %s in file: %s\n", key, jsonfile);
        goto load_json_error;
      }
      SC_FREE (item->string_value);
      item->string_value = SC_STRDUP (s);
      *(int *) item->opt_var = ivalue;
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
  }

  /* clean return under all circmstances */
  retval = 0;
load_json_error:
  /* free non-borrowed references */
  if (file != NULL) {
    json_decref (file);
  }
#endif
  return retval;
}

int
sc_options_save (int package_id, int err_priority,
                 sc_options_t * opt, const char *inifile)
{
  int                 retval;
  int                 i;
  int                 bvalue;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  FILE               *file;
  const char         *default_prefix = "Options";
  const char         *last_prefix;
  const char         *this_prefix;
  const char         *base_name;
  const char         *s;
  size_t              last_n;
  size_t              this_n;

  /* this routine must only be called after successful option parsing */
  SC_ASSERT (opt->argc >= 0 && opt->first_arg >= 0);
  SC_ASSERT (opt->first_arg <= opt->argc);

  file = fopen (inifile, "wb");
  if (file == NULL) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority, "File open failed\n");
    return -1;
  }

  retval = fprintf (file, "# written by sc_options_save\n");
  if (retval < 0) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Write title 1 failed\n");
    fclose (file);
    return -1;
  }

  this_prefix = NULL;
  last_prefix = NULL;
  this_n = last_n = 0;

  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE ||
        item->opt_type == SC_OPTION_JSONFILE ||
        item->opt_type == SC_OPTION_CALLBACK) {
      continue;
    }

    base_name = NULL;
    if (item->opt_name != NULL) {
      this_prefix = strrchr (item->opt_name, ':');
      if (this_prefix == NULL) {
        base_name = item->opt_name;
        this_prefix = default_prefix;
        this_n = strlen (default_prefix);
      }
      else {
        /* base name is whatever is to the right of the last colon */
        base_name = this_prefix + 1;
        this_n = this_prefix - item->opt_name;
        this_prefix = item->opt_name;
      }
    }

    if (this_prefix != NULL &&
        (last_prefix == NULL || this_n != last_n ||
         strncmp (this_prefix, last_prefix, this_n) != 0)) {
      retval = fprintf (file, "[%.*s]\n", (int) this_n, this_prefix);
      if (retval < 0) {
        SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                    "Write section heading failed\n");
        fclose (file);
        return -1;
      }
      last_prefix = this_prefix;
      last_n = this_n;
    }

    retval = 0;
    if (base_name != NULL) {
      retval = fprintf (file, "        %s = ", base_name);
    }
    else if (item->opt_char != '\0') {
      retval = fprintf (file, "        -%c = ", item->opt_char);
    }
    else {
      SC_ABORT_NOT_REACHED ();
    }
    if (retval < 0) {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                  "Write key failed\n");
      fclose (file);
      return -1;
    }

    retval = 0;
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      bvalue = *(int *) item->opt_var;
      if (bvalue <= 1)
        retval = fprintf (file, "%s\n", bvalue ? "true" : "false");
      else
        retval = fprintf (file, "%d\n", bvalue);
      break;
    case SC_OPTION_BOOL:
      retval = fprintf (file, "%s\n",
                        *(int *) item->opt_var ? "true" : "false");
      break;
    case SC_OPTION_INT:
      retval = fprintf (file, "%d\n", *(int *) item->opt_var);
      break;
    case SC_OPTION_SIZE_T:
      retval = fprintf (file, "%llu\n",
                        (unsigned long long) *(size_t *) item->opt_var);
      break;
    case SC_OPTION_DOUBLE:
      retval = fprintf (file, "%.16g\n", *(double *) item->opt_var);
      break;
    case SC_OPTION_STRING:
      s = sc_options_string_get ((sc_option_string_t *) item->opt_var);
      if (s != NULL) {
        retval = fprintf (file, "%s\n", s);
      }
      break;
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      retval = fprintf (file, "%s\n", item->string_value);
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
    if (retval < 0) {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                  "Write value failed\n");
      fclose (file);
      return -1;
    }
  }

  retval = fprintf (file, "[Arguments]\n        count = %d\n",
                    opt->argc - opt->first_arg);
  if (retval < 0) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Write title 2 failed\n");
    fclose (file);
    return -1;
  }
  for (i = opt->first_arg; i < opt->argc; ++i) {
    retval = fprintf (file, "        %d = %s\n",
                      i - opt->first_arg, opt->argv[i]);
    if (retval < 0) {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                  "Write argument failed\n");
      fclose (file);
      return -1;
    }
  }

  retval = fclose (file);
  if (retval) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "File close failed\n");
    return -1;
  }

  return 0;
}

int
sc_options_parse (int package_id, int err_priority, sc_options_t * opt,
                  int argc, char **argv)
{
  int                 retval, iserror;
  int                 position, printed;
  int                 c, option_index;
  int                 item_index = -1;
  int                *ivalue;
  size_t              iz;
  long                ilong;
  long long           ilonglong;
  double              dbl;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  char                optstring[BUFSIZ];
  struct option      *longopts, *lo;

  /* build getopt string and long option structures */

  longopts = SC_ALLOC_ZERO (struct option, count + 1);

  lo = longopts;
  position = 0;
  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_char != '\0') {
      printed = snprintf (optstring + position, BUFSIZ - position,
                          "%c%s", item->opt_char, item->has_arg ?
                          item->has_arg == 2 ? "::" : ":" : "");
      SC_ASSERT (printed > 0);
      position += printed;
    }
    if (item->opt_name != NULL) {
      lo->name = item->opt_name;
      lo->has_arg = item->has_arg;
      lo->flag = &item_index;
      lo->val = (int) iz;
      ++lo;
    }
  }

  /* run getopt_long loop */

  retval = 0;
  opterr = 0;
  while (retval == 0) {
    c = getopt_long (argc, argv, optstring, longopts, &option_index);
    if (c == -1) {
      break;
    }
    if (c == '?') {             /* invalid option */
      if (optopt == '-' || !isprint (optopt)) {
        SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                    "Invalid long option or missing argument\n");
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid short option: -%c or missing argument\n",
                     optopt);
      }
      retval = -1;
      break;
    }

    item = NULL;
    if (c == 0) {               /* long option */
      SC_ASSERT (item_index >= 0);
      item = (sc_option_item_t *) sc_array_index (items, (size_t) item_index);
    }
    else {                      /* short option */
      for (iz = 0; iz < count; ++iz) {
        item = (sc_option_item_t *) sc_array_index (items, iz);
        if (item->opt_char == c) {
          break;
        }
      }
      if (iz == count) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Encountered invalid short option: -%c\n", c);
        retval = -1;
        break;
      }
    }
    SC_ASSERT (item != NULL);

    ++item->called;
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      ++*(int *) item->opt_var;
      break;
    case SC_OPTION_BOOL:
      if (optarg == NULL) {
        *(int *) item->opt_var = 1;
      }
      else if (strspn (optarg, "1tTyY") > 0) {
        *(int *) item->opt_var = 1;
      }
      else if (strspn (optarg, "0fFnN") > 0) {
        *(int *) item->opt_var = 0;
      }
      else {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error parsing boolean: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      break;
    case SC_OPTION_INT:
      ilong = strtol (optarg, NULL, 0);
      if (ilong < (long) INT_MIN || ilong > (long) INT_MAX || errno == ERANGE) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error parsing int: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      else {
        *(int *) item->opt_var = (int) ilong;
      }
      break;
    case SC_OPTION_SIZE_T:
#ifndef SC_HAVE_STRTOLL
      ilonglong = (long long) strtol (optarg, NULL, 0);
#else
      ilonglong = strtoll (optarg, NULL, 0);
#endif
      if (ilonglong < 0LL || errno == ERANGE) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error parsing size_t: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      else {
        *(size_t *) item->opt_var = (size_t) ilonglong;
      }
      break;
    case SC_OPTION_DOUBLE:
      dbl = strtod (optarg, NULL);
      if (errno == ERANGE) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error parsing double: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      else {
        *(double *) item->opt_var = dbl;
      }
      break;
    case SC_OPTION_STRING:
      sc_options_string_set ((sc_option_string_t *) item->opt_var, optarg);
      break;
    case SC_OPTION_INIFILE:
      if (sc_options_load_ini (package_id, err_priority, opt, optarg, NULL)) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error loading .ini file: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      break;
    case SC_OPTION_JSONFILE:
      if (sc_options_load_json (package_id, err_priority, opt, optarg, NULL)) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error loading JSON file: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      break;
    case SC_OPTION_CALLBACK:
      if (item->opt_fn (opt, optarg, item->user_data)) {
        if (optarg == NULL) {
          SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                      "Error by option callback\n");
        }
        else {
          SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                       "Error by option callback: %s\n", optarg);
        }
        retval = -1;            /* this ends option processing */
      }
      break;
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      iserror = *(ivalue = (int *) item->opt_var);
      *ivalue = sc_keyvalue_get_int_check ((sc_keyvalue_t *) item->user_data,
                                           optarg, &iserror);
      if (iserror) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error looking up: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      else {
        SC_FREE (item->string_value);
        item->string_value = SC_STRDUP (optarg);
      }
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
  }

  /* free memory, assign results and return */

  SC_FREE (longopts);
  sc_options_free_args (opt);

  opt->first_arg = (retval < 0 ? -1 : optind);
  opt->argc = argc;
  opt->argv = argv;

  return opt->first_arg;
}

int
sc_options_load_args (int package_id, int err_priority, sc_options_t * opt,
                      const char *inifile)
{
  int                 i, count;
  int                 iserror;
  dictionary         *dict;
  const char         *s;
  char                key[BUFSIZ];

  /* enable true collective loading inside the call */
  dict = sc_iniparser_load (inifile, opt->max_bytes,
                            sc_options_get_collective (opt));
  if (dict == NULL) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Could not load or parse .ini file\n");
    return -1;
  }

  count = sc_iniparser_getint (dict, "Arguments:count", -1, &iserror);
  if (count < 0 || iserror) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Invalid or missing argument count\n");
    iniparser_freedict (dict);
    return -1;
  }

  sc_options_free_args (opt);
  opt->args_alloced = 1;
  opt->first_arg = 0;
  opt->argc = count;
  opt->argv = SC_ALLOC (char *, count);
  memset (opt->argv, 0, count * sizeof (char *));

  for (i = 0; i < count; ++i) {
    snprintf (key, BUFSIZ, "Arguments:%d", i);
    s = iniparser_getstring (dict, key, NULL);
    if (s == NULL) {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                  "Invalid or missing argument count\n");
      iniparser_freedict (dict);
      return -1;
    }
    opt->argv[i] = SC_STRDUP (s);
  }

  iniparser_freedict (dict);
  return 0;
}
