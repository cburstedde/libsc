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

#ifndef SC_OPTIONS_H
#define SC_OPTIONS_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

#include <sc_containers.h>

SC_EXTERN_C_BEGIN;

typedef enum
{
  SC_OPTION_SWITCH,
  SC_OPTION_INT,
  SC_OPTION_DOUBLE,
  SC_OPTION_STRING,
  SC_OPTION_INIFILE,
}
sc_option_type_t;

typedef struct
{
  sc_option_type_t    opt_type;
  int                 opt_char;
  const char         *opt_name;
  void               *opt_var;
  int                 has_arg;
  const char         *help_string;
  char               *string_value;
}
sc_option_item_t;

typedef struct
{
  char                program_path[BUFSIZ];
  const char         *program_name;
  sc_array_t         *option_items;
  int                 first_arg;
  int                 argc;
  char              **argv;
}
sc_options_t;

/**
 * Create an empty options structure.
 * \param [in] program_path   Name or path name of the program.
 */
sc_options_t       *sc_options_new (const char *program_path);

/**
 * Destroy the options structure.
 */
void                sc_options_destroy (sc_options_t * opt);

/**
 * Add a switch option. This option is used without option arguments.
 * Every use increments the variable by one.  Its initial value is 0.
 * Either opt_char or opt_name must be valid, that is, not '\0'/NULL.
 * \param [in] opt_char      Short option character, may be '\0'.
 * \param [in] opt_name      Option name without initial dashes, may be NULL.
 * \param [in] variable      Address of the variable to store the option value.
 * \param [in] help_string   Help string for usage message, may be NULL.
 */
void                sc_options_add_switch (sc_options_t * opt,
                                           int opt_char,
                                           const char *opt_name,
                                           int *variable,
                                           const char *help_string);

/**
 * Add an option that takes an integer argument.
 * \param [in] init_value   The initial value of the variable.
 */
void                sc_options_add_int (sc_options_t * opt,
                                        int opt_char,
                                        const char *opt_name,
                                        int *variable, int init_value,
                                        const char *help_string);

/**
 * Add an option that takes a double argument.
 */
void                sc_options_add_double (sc_options_t * opt,
                                           int opt_char,
                                           const char *opt_name,
                                           double *variable,
                                           double init_value,
                                           const char *help_string);

/**
 * Add a string option.
 * \param [in] init_value  The default value of the option may be NULL.
 *                         If not NULL, the value is copied internally.
 * \param [out] variable   Will point to an internal string value.
 */
void                sc_options_add_string (sc_options_t * opt,
                                           int opt_char,
                                           const char *opt_name,
                                           const char **variable,
                                           const char *init_value,
                                           const char *help_string);

/**
 * Add an option to read in a file in .ini format.
 */
void                sc_options_add_inifile (sc_options_t * opt,
                                            int opt_char,
                                            const char *opt_name,
                                            const char *help_string);

/**
 * Print a usage message.
 * This function uses the SC_LC_GLOBAL log category.
 * That means the default action is to print only on rank 0.
 * Applications can change that by providing a user-defined log handler.
 * \param [in] package_id       Registered package id or -1.
 * \param [in] log_priority     Log priority for output according to sc.h.
 * \param [in] opt              The option structure.
 * \param [in] arg_usage        If not NULL, an <ARGUMENTS> string is appended
 *                              to the usage line.  If the string is non-empty,
 *                              it will be printed after the option summary
 *                              and an "ARGUMENTS:\n" title line.  Line breaks
 *                              are identified by strtok(3) and honored.
 */
void                sc_options_print_usage (int package_id, int log_priority,
                                            sc_options_t * opt,
                                            const char *arg_usage);

/**
 * Print a summary of all option values.
 * Prints the title "Options:" and a line for every option,
 * then the title "Arguments:" and a line for every argument.
 * This function uses the SC_LC_GLOBAL log category.
 * That means the default action is to print only on rank 0.
 * Applications can change that by providing a user-defined log handler.
 * \param [in] package_id       Registered package id or -1.
 * \param [in] log_priority     Log priority for output according to sc.h.
 * \param [in] opt              The option structure.
 */
void                sc_options_print_summary (int package_id,
                                              int log_priority,
                                              sc_options_t * opt);

/**
 * Loads a file in .ini format and updates entries found under [Options].
 * \param [in] package_id       Registered package id or -1.
 * \param [in] err_priority     Error log priority according to sc.h.
 * \param [in] opt              The option structure.
 * \param [in] inifile          Filename if the ini file to load.
 * \return                      Returns 0 on success, -1 on failure.
 */
int                 sc_options_load (int package_id, int err_priority,
                                     sc_options_t * opt, const char *inifile);

/**
 * Parse command line options.
 * \param [in] package_id       Registered package id or -1.
 * \param [in] err_priority     Error log priority according to sc.h.
 * \param [in] opt              The option structure.
 * \param [in] argc             Length of argument list.
 * \param [in,out] argv         Argument list may be permuted.
 * \return                      Returns -1 on an invalid option, otherwise
 *                              the position of the first non-option argument.
 */
int                 sc_options_parse (int package_id, int err_priority,
                                      sc_options_t * opt, int argc,
                                      char **argv);

SC_EXTERN_C_END;

#endif /* !SC_OPTIONS_H */
