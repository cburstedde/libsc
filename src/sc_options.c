/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
*/

/* sc.h comes first in every compilation unit */
#include <sc.h>
#include <sc_options.h>
#include <iniparser.h>

#ifdef SC_OPTIONS

sc_options_t       *
sc_options_new (const char *program_path)
{
  sc_options_t       *opt;

  opt = SC_ALLOC_ZERO (sc_options_t, 1);

  snprintf (opt->program_path, BUFSIZ, "%s", program_path);
  opt->program_name = basename (opt->program_path);
  opt->option_items = sc_array_new (sizeof (sc_option_item_t));

  return opt;
}

void
sc_options_destroy (sc_options_t * opt)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;

  for (iz = 0; iz < count; ++iz) {
    item = sc_array_index (items, iz);
    SC_FREE (item->string_value);
  }

  sc_array_destroy (opt->option_items);
  SC_FREE (opt);
}

void
sc_options_add_switch (sc_options_t * opt, int opt_char,
                       const char *opt_name,
                       int *variable, const char *help_string)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  iz = items->elem_count;
  sc_array_resize (items, iz + 1);
  item = sc_array_index (items, iz);

  item->opt_type = SC_OPTION_SWITCH;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->has_arg = 0;
  item->help_string = help_string;
  item->string_value = NULL;

  *variable = 0;
}

void
sc_options_add_int (sc_options_t * opt, int opt_char,
                    const char *opt_name,
                    int *variable, int init_value, const char *help_string)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  iz = items->elem_count;
  sc_array_resize (items, iz + 1);
  item = sc_array_index (items, iz);

  item->opt_type = SC_OPTION_INT;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->has_arg = 1;
  item->help_string = help_string;
  item->string_value = NULL;

  *variable = init_value;
}

void
sc_options_add_double (sc_options_t * opt, int opt_char,
                       const char *opt_name,
                       double *variable, double init_value,
                       const char *help_string)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  iz = items->elem_count;
  sc_array_resize (items, iz + 1);
  item = sc_array_index (items, iz);

  item->opt_type = SC_OPTION_DOUBLE;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->has_arg = 1;
  item->help_string = help_string;
  item->string_value = NULL;

  *variable = init_value;
}

void
sc_options_add_string (sc_options_t * opt,
                       int opt_char,
                       const char *opt_name,
                       const char **variable,
                       const char *init_value, const char *help_string)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  iz = items->elem_count;
  sc_array_resize (items, iz + 1);
  item = sc_array_index (items, iz);

  item->opt_type = SC_OPTION_STRING;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = variable;
  item->has_arg = 1;
  item->help_string = help_string;

  /* init_value may be NULL */
  *variable = item->string_value = SC_STRDUP (init_value);
}

void
sc_options_add_inifile (sc_options_t * opt,
                        int opt_char,
                        const char *opt_name, const char *help_string)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  sc_option_item_t   *item;

  SC_ASSERT (opt_char != '\0' || opt_name != NULL);
  SC_ASSERT (opt_name == NULL || opt_name[0] != '-');

  iz = items->elem_count;
  sc_array_resize (items, iz + 1);
  item = sc_array_index (items, iz);

  item->opt_type = SC_OPTION_INIFILE;
  item->opt_char = opt_char;
  item->opt_name = opt_name;
  item->opt_var = NULL;
  item->has_arg = 1;
  item->help_string = help_string;
  item->string_value = NULL;
}

void
sc_options_print_help (sc_options_t * opt, int include_args, FILE * nout)
{
  int                 printed;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  const char         *provide_args;
  const char         *provide_short;
  const char         *provide_long;

  if (nout == NULL)
    return;

  provide_args = include_args ? " <ARGUMENTS>" : "";
  if (count == 0) {
    fprintf (nout, "Usage: %s%s\n", opt->program_name, provide_args);
    return;
  }

  fprintf (nout, "Usage: %s <OPTIONS>%s\n", opt->program_name, provide_args);
  fprintf (nout, "   OPTIONS:\n");
  for (iz = 0; iz < count; ++iz) {
    item = sc_array_index (items, iz);
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      provide_short = "";
      provide_long = "";
      break;
    case SC_OPTION_INT:
      provide_short = " <INT>";
      provide_long = "=<INT>";
      break;
    case SC_OPTION_DOUBLE:
      provide_short = " <DOUBLE>";
      provide_long = "=<DOUBLE>";
      break;
    case SC_OPTION_STRING:
      provide_short = " <STRING>";
      provide_long = "=<STRING>";
      break;
    case SC_OPTION_INIFILE:
      provide_short = " <INIFILE>";
      provide_long = "=<INIFILE>";
      break;
    default:
      SC_CHECK_NOT_REACHED ();
    }
    if (item->opt_char != '\0' && item->opt_name != NULL) {
      printed = fprintf (nout, "      -%c%s | --%s%s",
                         item->opt_char, provide_short,
                         item->opt_name, provide_long);
    }
    else if (item->opt_char != '\0') {
      printed = fprintf (nout, "      -%c%s", item->opt_char, provide_short);
    }
    else if (item->opt_name != NULL) {
      printed = fprintf (nout, "      --%s%s", item->opt_name, provide_long);
    }
    else {
      SC_CHECK_NOT_REACHED ();
    }
    if (item->help_string != NULL) {
      fprintf (nout, "%*s%s\n", SC_MAX (1, 48 - printed), "",
               item->help_string);
    }
  }

#if 0
  if (include_args) {
    fprintf (nout, "   ARGUMENTS:\n");
  }
#endif
}

void
sc_options_print_summary (sc_options_t * opt, FILE * nout)
{
  int                 printed;
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  const char         *string_val;

  if (nout == NULL)
    return;

  if (count == 0) {
    fprintf (nout, "No command line options specified\n");
    return;
  }

  fprintf (nout, "Summary:\n   OPTIONS:\n");
  for (iz = 0; iz < count; ++iz) {
    item = sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE) {
      continue;
    }
    if (item->opt_name == NULL) {
      printed = fprintf (nout, "      -%c: ", item->opt_char);
    }
    else {
      printed = fprintf (nout, "      %s: ", item->opt_name);
    }
    switch (item->opt_type) {
    case SC_OPTION_SWITCH:     /* fall through no break */
    case SC_OPTION_INT:
      printed += fprintf (nout, "%d", *(int *) item->opt_var);
      break;
    case SC_OPTION_DOUBLE:
      printed += fprintf (nout, "%g", *(double *) item->opt_var);
      break;
    case SC_OPTION_STRING:
      string_val = *(const char **) item->opt_var;
      if (string_val == NULL) {
        string_val = "<unspecified>";
      }
      printed += fprintf (nout, "%s", string_val);
      break;
    default:
      SC_CHECK_NOT_REACHED ();
    }
    fprintf (nout, "\n");
  }
}

void
sc_options_print_arguments (sc_options_t * opt,
                            int first_arg, int argc, char **argv, FILE * nout)
{
  int                 i;

  if (nout == NULL) {
    return;
  }

  if (first_arg == 0) {
    fprintf (nout, "   ARGUMENTS: none\n");
  }
  else {
    fprintf (nout, "   ARGUMENTS:\n");
    for (i = first_arg; i < argc; ++i) {
      fprintf (nout, "      %d: %s\n", i - first_arg, argv[i]);
    }
  }
}

int
sc_options_load (sc_options_t * opt, const char *inifile, FILE * nerr)
{
  size_t              iz;
  sc_array_t         *items = opt->option_items;
  size_t              count = items->elem_count;
  sc_option_item_t   *item;
  dictionary         *dict;
  int                *ivalue;
  double             *dvalue;
  const char         *s, *key;
  char                skey[BUFSIZ], lkey[BUFSIZ];
  int                 found_short, found_long;

  dict = iniparser_load (inifile, nerr);
  if (dict == NULL) {
    return -1;
  }

  for (iz = 0; iz < count; ++iz) {
    item = sc_array_index (items, iz);
    if (item->opt_type == SC_OPTION_INIFILE) {
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
      snprintf (lkey, BUFSIZ, "Options:%s", item->opt_name);
      found_long = iniparser_find_entry (dict, lkey);
    }
    if (found_short && found_long) {
      if (nerr != NULL) {
        fprintf (nerr, "Duplicates %s %s in file: %s\n", skey, lkey, inifile);
      }
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

    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      *(int *) item->opt_var += iniparser_getboolean (dict, key, 0);
      break;
    case SC_OPTION_INT:
      ivalue = (int *) item->opt_var;
      *ivalue = iniparser_getint (dict, key, *ivalue);
      break;
    case SC_OPTION_DOUBLE:
      dvalue = (double *) item->opt_var;
      *dvalue = iniparser_getdouble (dict, key, *dvalue);
      break;
    case SC_OPTION_STRING:
      s = iniparser_getstring (dict, key, NULL);
      if (s != NULL) {
        SC_FREE (item->string_value);   /* deals with NULL */
        *(const char **) item->opt_var = item->string_value = SC_STRDUP (s);
      }
      break;
    default:
      SC_CHECK_NOT_REACHED ();
    }
  }

  iniparser_freedict (dict);
  return 0;
}

int
sc_options_parse (sc_options_t * opt, int argc, char **argv, FILE * nerr)
{
  int                 retval;
  int                 position, printed;
  int                 c, option_index;
  int                 item_index = -1;
  size_t              iz;
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
    item = sc_array_index (items, iz);
    if (item->opt_char != '\0') {
      printed = snprintf (optstring + position, BUFSIZ - position,
                          "%c%s", item->opt_char, item->has_arg ? ":" : "");
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

  /* run getopt loop */

  retval = 0;
  opterr = 0;
  while (retval == 0) {
    c = getopt_long (argc, argv, optstring, longopts, &option_index);
    if (c == -1) {
      break;
    }
    if (c == '?') {             /* invalid option */
      if (nerr != NULL) {
        fprintf (nerr, "Encountered invalid option\n");
      }
      retval = -1;
      break;
    }

    item = NULL;
    if (c == 0) {               /* long option */
      SC_ASSERT (item_index >= 0);
      item = sc_array_index (items, (size_t) item_index);
    }
    else {                      /* short option */
      for (iz = 0; iz < count; ++iz) {
        item = sc_array_index (items, iz);
        if (item->opt_char == c) {
          break;
        }
      }
      if (iz == count) {
        if (nerr != NULL) {
          fprintf (nerr, "Encountered invalid short option: %c\n", c);
        }
        retval = -1;
        break;
      }
    }
    SC_ASSERT (item != NULL);

    switch (item->opt_type) {
    case SC_OPTION_SWITCH:
      ++*(int *) item->opt_var;
      break;
    case SC_OPTION_INT:
      *(int *) item->opt_var = strtol (optarg, NULL, 0);
      break;
    case SC_OPTION_DOUBLE:
      *(double *) item->opt_var = strtod (optarg, NULL);
      break;
    case SC_OPTION_STRING:
      SC_FREE (item->string_value);     /* deals with NULL */
      *(const char **) item->opt_var = item->string_value =
        SC_STRDUP (optarg);
      break;
    case SC_OPTION_INIFILE:
      if (sc_options_load (opt, optarg, nerr)) {
        if (nerr != NULL) {
          fprintf (nerr, "Error loading file: %s\n", optarg);
        }
        retval = -1;            /* this ends option processing */
      }
      break;
    default:
      SC_CHECK_NOT_REACHED ();
    }
  }

  /* free memory and return */
  SC_FREE (longopts);
  return retval < 0 ? -1 : optind;
}

#endif

/* EOF sc_options.c */
