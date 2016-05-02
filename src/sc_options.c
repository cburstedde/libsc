/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

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

#include <sc_getopt.h>
#include <sc_options.h>
#include <iniparser.h>

#include <errno.h>

typedef enum
{
  SC_OPTION_SWITCH,
  SC_OPTION_BOOL,
  SC_OPTION_INT,
  SC_OPTION_SIZE_T,
  SC_OPTION_DOUBLE,
  SC_OPTION_STRING,
  SC_OPTION_INIFILE,
  SC_OPTION_CALLBACK,
  SC_OPTION_KEYVALUE
}
sc_option_type_t;

typedef struct
{
  sc_option_type_t    opt_type;
  int                 opt_char;
  const char         *opt_name;
  void               *opt_var;
  void                (*opt_fn) (void);
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

static              size_t
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
  opt->program_name = basename (opt->program_path);
  opt->option_items = sc_array_new (sizeof (sc_option_item_t));
  opt->subopt_names = sc_array_new (sizeof (char *));
  opt->args_alloced = 0;
  opt->first_arg = -1;
  opt->argc = 0;
  opt->argv = NULL;

  /* set default spacing for printing option summary */
  sc_options_set_spacing (opt, -1, -1);

  return opt;
}

static void
sc_options_destroy_internal (sc_options_t * opt)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  sc_array_t         *names = opt->subopt_names;
  char               *name;

  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    SC_FREE (item->string_value);
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
sc_options_destroy (sc_options_t * opt)
{
  sc_options_destroy_internal (opt);
}

void
sc_options_set_spacing (sc_options_t * opt, int space_type, int space_help)
{
  SC_ASSERT (opt != NULL);

  opt->space_type = space_type < 0 ? sc_options_space_type : space_type;
  opt->space_help = space_help < 0 ? sc_options_space_help : space_help;
}

void
sc_options_add_switch (sc_options_t * opt, int opt_char,
                       const char *opt_name,
                       int *variable, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_SWITCH;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 0;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
  item->user_data = NULL;

  *variable = 0;
}

void
sc_options_add_bool (sc_options_t * opt, int opt_char,
                     const char *opt_name,
                     int *variable, int init_value, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_BOOL;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 2;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
  item->user_data = NULL;

  *variable = init_value;
}

void
sc_options_add_int (sc_options_t * opt, int opt_char, const char *opt_name,
                    int *variable, int init_value, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_INT;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 1;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
  item->user_data = NULL;

  *variable = init_value;
}

void
sc_options_add_size_t (sc_options_t * opt, int opt_char, const char *opt_name,
                       size_t * variable, size_t init_value,
                       const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_SIZE_T;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 1;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
  item->user_data = NULL;

  *variable = init_value;
}

void
sc_options_add_double (sc_options_t * opt, int opt_char,
                       const char *opt_name,
                       double *variable, double init_value,
                       const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_DOUBLE;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 1;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
  item->user_data = NULL;

  *variable = init_value;
}

void
sc_options_add_string (sc_options_t * opt, int opt_char,
                       const char *opt_name, const char **variable,
                       const char *init_value, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_STRING;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 1;
  item->called = 0;
  item->help_string = help_string;
  item->user_data = NULL;

  /* init_value may be NULL */
  *variable = item->string_value = SC_STRDUP (init_value);
}

void
sc_options_add_inifile (sc_options_t * opt, int opt_char,
                        const char *opt_name, const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_INIFILE;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = NULL;
  item->opt_fn = NULL;
  item->has_arg = 1;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
  item->user_data = NULL;
}

void
sc_options_add_callback (sc_options_t * opt, int opt_char,
                         const char *opt_name, int has_arg,
                         sc_options_callback_t fn, void *data,
                         const char *help_string)
{
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_CALLBACK;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = NULL;
  item->opt_fn = (void (*)(void)) fn;
  item->has_arg = has_arg;
  item->called = 0;
  item->help_string = help_string;
  item->string_value = NULL;
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

  item = (sc_option_item_t *) sc_array_push (opt->option_items);

  item->opt_type = SC_OPTION_KEYVALUE;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->opt_fn = NULL;
  item->has_arg = 1;
  item->called = 0;
  item->help_string = help_string;
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
      sc_options_add_string (opt, '\0', *name, (const char **) item->opt_var,
                             item->string_value, item->help_string);
      break;
    case SC_OPTION_INIFILE:
      sc_options_add_inifile (opt, '\0', *name, item->help_string);
      break;
    case SC_OPTION_CALLBACK:
      sc_options_add_callback (opt, '\0', *name, item->has_arg,
                               (sc_options_callback_t) item->opt_fn,
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
  int                 printed;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  const char         *provide;
  const char         *separator;
  char                outbuf[BUFSIZ];
  char               *copy, *tok;

  SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority,
               "Usage: %s%s%s\n", opt->program_name,
               count == 0 ? "" : " <OPTIONS>",
               arg_usage == NULL ? "" : " <ARGUMENTS>");
  if (count > 0) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority, "Options:\n");
  }

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
      provide = "<FILE>";
      break;
    case SC_OPTION_CALLBACK:
      if (item->has_arg) {
        provide = "<ARG>";
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
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority, "%s\n", outbuf);
  }

  if (arg_usage != NULL && arg_usage[0] != '\0') {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority, "Arguments:\n");
    copy = SC_STRDUP (arg_usage);
    for (tok = strtok (copy, "\n\r"); tok != NULL;
         tok = strtok (NULL, "\n\r")) {
      SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority, "   %s\n", tok);
    }
    SC_FREE (copy);
  }
}

void
sc_options_print_summary (int package_id, int log_priority,
                          sc_options_t * opt)
{
  int                 i;
  int                 bvalue, printed;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  const char         *string_val;
  char                outbuf[BUFSIZ];

  SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority, "Options:\n");

  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE ||
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
      string_val = *(const char **) item->opt_var;
      if (string_val == NULL) {
        string_val = "<unspecified>";
      }
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%s", string_val);
      break;
#if 0
    case SC_OPTION_CALLBACK:
      if (item->called) {
        string_val = item->has_arg ? item->string_value : "true";
      }
      else {
        string_val = "<unspecified>";
      }
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%s", string_val);
      break;
#endif
    case SC_OPTION_KEYVALUE:
      SC_ASSERT (item->string_value != NULL);
      printed += snprintf (outbuf + printed, BUFSIZ - printed,
                           "%s", item->string_value);
      break;
    default:
      SC_ABORT_NOT_REACHED ();
    }
    SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority, "%s\n", outbuf);
  }

  if (opt->first_arg < 0) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority,
                "Arguments: not parsed\n");
  }
  else {
    if (opt->first_arg == opt->argc) {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority,
                  "Arguments: none\n");
    }
    else {
      SC_GEN_LOG (package_id, SC_LC_GLOBAL, log_priority, "Arguments:\n");
    }
    for (i = opt->first_arg; i < opt->argc; ++i) {
      SC_GEN_LOGF (package_id, SC_LC_GLOBAL, log_priority, "   %d: %s\n",
                   i - opt->first_arg, opt->argv[i]);
    }
  }
}

int
sc_options_load (int package_id, int err_priority,
                 sc_options_t * opt, const char *inifile)
{
  int                 found_short, found_long;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  dictionary         *dict;
  int                 iserror;
  int                 bvalue;
  int                *ivalue;
  double             *dvalue;
  size_t             *zvalue;
  const char         *s, *key;
  char                skey[BUFSIZ], lkey[BUFSIZ];

  dict = iniparser_load (inifile);
  if (dict == NULL) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Could not load or parse inifile\n");
    return -1;
  }

  for (iz = 0; iz < count; ++iz) {
    item = (sc_option_item_t *) sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE ||
        item->opt_type == SC_OPTION_CALLBACK) {
      continue;
    }

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
      SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
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

    ++item->called;
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      bvalue = iniparser_getboolean (dict, key, -1);
      if (bvalue == -1) {
        bvalue = sc_iniparser_getint (dict, key, 0, &iserror);
        if (bvalue <= 0 || iserror) {
          SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
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
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
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
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid int %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      break;
    case SC_OPTION_SIZE_T:
      zvalue = (size_t *) item->opt_var;
      *zvalue = sc_iniparser_getsizet (dict, key, *zvalue, &iserror);
      if (iserror) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid size_t %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      break;
    case SC_OPTION_DOUBLE:
      dvalue = (double *) item->opt_var;
      *dvalue = sc_iniparser_getdouble (dict, key, *dvalue, &iserror);
      if (iserror) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Invalid double %s in file: %s\n", key, inifile);
        iniparser_freedict (dict);
        return -1;
      }
      break;
    case SC_OPTION_STRING:
      s = iniparser_getstring (dict, key, NULL);
      if (s != NULL) {
        SC_FREE (item->string_value);   /* deals with NULL */
        *(const char **) item->opt_var = item->string_value = SC_STRDUP (s);
      }
      break;
#if 0
    case SC_OPTION_CALLBACK:
      if (item->has_arg) {
        s = iniparser_getstring (dict, key, NULL);
        if (s == NULL) {
          SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                       "Invalid string %s in file: %s\n", key, inifile);
          iniparser_freedict (dict);
          return -1;
        }
        SC_FREE (item->string_value);   /* deals with NULL */
        item->string_value = SC_STRDUP (s);
      }
      else {
        s = NULL;
      }
      fn = (sc_options_callback_t) item->opt_fn;
      if (fn (opt, s, item->user_data)) {
        iniparser_freedict (dict);
        return -1;
      }
      break;
#endif
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
          SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                       "Invalid key %s for option %s in file: %s\n",
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
    if (item->opt_type == SC_OPTION_STRING && item->string_value == NULL) {
      continue;
    }
    if (item->opt_type == SC_OPTION_INIFILE ||
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
      retval = fprintf (file, "%s\n", item->string_value);
      break;
#if 0
    case SC_OPTION_CALLBACK:
      if (item->has_arg) {
        SC_ASSERT (item->string_value != NULL);
        retval = fprintf (file, "%s\n", item->string_value);
      }
      else {
        retval = fprintf (file, "%s\n", "true");
      }
      break;
#endif
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
  sc_options_callback_t fn;

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
      SC_FREE (item->string_value);     /* deals with NULL */
      *(const char **) item->opt_var = item->string_value =
        SC_STRDUP (optarg);
      break;
    case SC_OPTION_INIFILE:
      if (sc_options_load (package_id, err_priority, opt, optarg)) {
        SC_GEN_LOGF (package_id, SC_LC_GLOBAL, err_priority,
                     "Error loading file: %s\n", optarg);
        retval = -1;            /* this ends option processing */
      }
      break;
    case SC_OPTION_CALLBACK:
      if (item->has_arg) {
        SC_FREE (item->string_value);   /* deals with NULL */
        item->string_value = SC_STRDUP (optarg);
      }
      fn = (sc_options_callback_t) item->opt_fn;
      if (fn (opt, item->has_arg ? optarg : NULL, item->user_data)) {
#if 0
        SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                    "Error in callback option\n");
#endif
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

  dict = iniparser_load (inifile);
  if (dict == NULL) {
    SC_GEN_LOG (package_id, SC_LC_GLOBAL, err_priority,
                "Could not load or parse inifile\n");
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

void
sc_options_broadcast (sc_options_t * opt, int root, sc_MPI_Comm mpicomm)
{
  int                 mpiret;
  int                 rank;
  int                 n_int, n_size_t, n_double, n_string;
  int                 length, length_sum;
  int                *int_data;
  size_t              countz, iz;
  long long          *size_t_data;
  double             *double_data;
  char               *char_data;
  sc_option_item_t   *item;

  /* setup some variables */
  mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
  SC_CHECK_MPI (mpiret);
  countz = opt->option_items->elem_count;

  /* gather information about the data to be broadcast */
  n_int = n_size_t = n_double = n_string = 0;
  length_sum = 0;
  for (iz = 0; iz < countz; ++iz) {
    item = (sc_option_item_t *) sc_array_index (opt->option_items, iz);
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
    case SC_OPTION_BOOL:
    case SC_OPTION_INT:
    case SC_OPTION_KEYVALUE:
      ++n_int;
      break;
    case SC_OPTION_SIZE_T:
      ++n_size_t;
      break;
    case SC_OPTION_DOUBLE:
      ++n_double;
      break;
    case SC_OPTION_STRING:
      if (rank == root) {
        if (item->string_value != NULL) {
          length_sum += strlen (item->string_value);
        }
      }
      ++n_string;
      break;
    case SC_OPTION_INIFILE:
    case SC_OPTION_CALLBACK:
      /* do nothing is sufficient for meta-options */
      break;
    }
  }

  /* allocate data buffers of known size for broadcast */
  int_data = SC_ALLOC_ZERO (int, 5 + n_int + n_string);
  size_t_data = SC_ALLOC_ZERO (long long, n_size_t);
  double_data = SC_ALLOC_ZERO (double, n_double);
  char_data = NULL;

  /* fill data into buffers by the sender */
  if (rank == root) {
    char_data = SC_ALLOC (char, length_sum);
    int_data[0] = n_int;
    int_data[1] = n_size_t;
    int_data[2] = n_double;
    int_data[3] = n_string;
    int_data[4] = length_sum;
    n_int = n_size_t = n_double = n_string = 0;
    length_sum = 0;
    for (iz = 0; iz < countz; ++iz) {
      item = (sc_option_item_t *) sc_array_index (opt->option_items, iz);
      switch (item->opt_type) {
      case SC_OPTION_SWITCH:
      case SC_OPTION_BOOL:
      case SC_OPTION_INT:
      case SC_OPTION_KEYVALUE:
        int_data[5 + n_int + n_string] = *(int *) item->opt_var;
        ++n_int;
        break;
      case SC_OPTION_SIZE_T:
        size_t_data[n_size_t] = (long long) *(size_t *) item->opt_var;
        ++n_size_t;
        break;
      case SC_OPTION_DOUBLE:
        double_data[n_double] = *(double *) item->opt_var;
        ++n_double;
        break;
      case SC_OPTION_STRING:
        if (item->string_value != NULL) {
          length = strlen (item->string_value);
          memcpy (char_data + length_sum, item->string_value, length);
          int_data[5 + n_int + n_string] = length;
          length_sum += length;
        }
        else {
          int_data[5 + n_int + n_string] = -1;
        }
        ++n_string;
        break;
      case SC_OPTION_INIFILE:
      case SC_OPTION_CALLBACK:
        /* do nothing is sufficient for meta-options */
        break;
      }
    }
  }

  /* broadcast the known-size buffers */
  mpiret = sc_MPI_Bcast (int_data, 5 + n_int + n_string, sc_MPI_INT,
                         root, opt->mpicomm);
  SC_CHECK_MPI (mpiret);
  SC_ASSERT (int_data[0] == n_int);
  SC_ASSERT (int_data[1] == n_size_t);
  SC_ASSERT (int_data[2] == n_double);
  SC_ASSERT (int_data[3] == n_string);
  mpiret = sc_MPI_Bcast (size_t_data, n_size_t, sc_MPI_LONG_LONG_INT,
                         root, opt->mpicomm);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Bcast (double_data, n_double, sc_MPI_DOUBLE,
                         root, opt->mpicomm);
  SC_CHECK_MPI (mpiret);

  /* allocate receiver data and broadcast now-known-size buffers */
  if (rank != root) {
    SC_ASSERT (char_data == NULL);
    SC_ASSERT (length_sum == 0);
    length_sum = int_data[4];
    char_data = SC_ALLOC (char, length_sum);
  }
  else {
    SC_ASSERT (char_data != NULL);
    SC_ASSERT (int_data[4] == length_sum);
  }
  mpiret = sc_MPI_Bcast (char_data, length_sum, sc_MPI_CHAR,
                         root, opt->mpicomm);
  SC_CHECK_MPI (mpiret);

  /* write received values into option values */

  /* free memory */
  SC_FREE (int_data);
  SC_FREE (size_t_data);
  SC_FREE (double_data);
  SC_FREE (char_data);
}
