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

/** \file sc3_options.h \ingroup sc3
 *
 * We provide an object to collect command line options.
 * Multiple instances may be used flexibly to parse command lines.
 *
 * Specific options can be added to an options object during setup.
 * They accept pointers to outside memory for name strings and the
 * options variable that will be assigned a new value on parsing.
 * This maximizes convenience but requires these outside variables
 * to stay in scope while the options object is in use.
 *
 * One way of being relatively safe is to point to static constant
 * strings for long option names and help messages and to long-lived
 * option variables of global duration, and to destroy the options
 * object after parsing and before the main program goes to work.
 */

#ifndef SC3_OPTIONS_H
#define SC3_OPTIONS_H

#include <sc3_alloc.h>

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

/** The options object is an opaque struct. */
typedef struct sc3_options sc3_options_t;

/** Query whether options object is not NULL and internally consistent.
 * The object may be valid in both its setup and usage phases.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer is not NULL and object consistent.
 */
int                 sc3_options_is_valid (const sc3_options_t * yy,
                                          char *reason);

/** Query whether options object is not NULL, consistent and not setup.
 * This means that the object is not (yet) in its usage phase.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object consistent, not setup.
 */
int                 sc3_options_is_new (const sc3_options_t * yy,
                                        char *reason);

/** Query whether options object is not NULL, internally consistent and setup.
 * This means that the object's parameterization is final and it is usable.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object consistent and setup.
 */
int                 sc3_options_is_setup (const sc3_options_t * yy,
                                          char *reason);

#if 0

/** Query whether an object is setup and has the dummy parameter set.
 * \param [in] yy       Any pointer.
 * \param [out] reason  If not NULL, existing string of length SC3_BUFSIZE
 *                      is set to "" if answer is yes or reason if no.
 * \return              True if pointer not NULL, object setup and resizable.
 */
int                 sc3_options_is_dummy (const sc3_options_t * yy,
                                          char *reason);

#endif

/** Create a new options object in its setup phase.
 * It begins with default parameters that can be overridden explicitly.
 * Setting and modifying parameters is only allowed in the setup phase.
 * Call \ref sc3_options_setup to change the object into its usage phase.
 * The defaults are documented in the sc3_options_set_* calls.
 * \param [in,out] alloc    An allocator that is setup, or NULL.
 *                          If NULL, we call \ref sc3_allocator_new_static.
 *                          The allocator is refd and remembered internally
 *                          and will be unrefd on object destruction.
 * \param [out] yyp     Pointer must not be NULL.
 *                      If the function returns an error, value set to NULL.
 *                      Otherwise, value set to an object with default values.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_new (sc3_allocator_t * alloc,
                                     sc3_options_t ** yyp);

/** Enable or disable recognition of '--' argument to stop processing.
 * \param [in,out] yy       The object must not be setup.
 * \param [in] var_stop     Pointer to existing integer variable or NULL.
 *                          When NULL, the stop feature is disabled.
 *                          Otherwise, its value is initialized to 0 and
 *                          set to 1 on parsing when '--' is encountered.
 *                          We leave it to the user to stop processing
 *                          based on the value assigned to this variable.
 *                          The variable must stay in scope/memory
 *                          while the options object is alive.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_set_stop (sc3_options_t * yy, int *var_stop);

/** Add a flag option.
 * It is initialized to 0 (false) and incremented on each occurrence.
 * \param [in,out] yy   The object must not be setup.
 * \param [in] opt_short    Short option character.
 *                          May be Nul for no short option.
 * \param [in] opt_long     Long option string, or NULL for none.
 *                          Should not include leading '--' and
 *                          must not contain white space.
 *                          We make a shallow (pointer) copy.
 *                          It is legal for both short and long option
 *                          to be Nul/NULL.  We still assign the default.
 * \param [in] opt_help     Help string to display, or NULL for none.
 *                          We make a shallow (pointer) copy.
 * \param [in] opt_variable Pointer to existing integer variable.
 *                          The variable must stay in scope/memory
 *                          while the options object is alive.
 *                          Value is initialized to 0.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_add_flag (sc3_options_t * yy,
                                          char opt_short,
                                          const char *opt_long,
                                          const char *opt_help,
                                          int *opt_variable);

/** Add an integer argument to options object.
 * \param [in,out] yy   The object must not be setup.
 * \param [in] opt_short    Short option character.
 *                          May be Nul for no short option.
 * \param [in] opt_long     Long option string, or NULL for none.
 *                          Should not include leading '--' and
 *                          must not contain white space.
 *                          We make a shallow (pointer) copy.
 *                          It is legal for both short and long option
 *                          to be Nul/NULL.  We still assign \a opt_value.
 * \param [in] opt_help     Help string to display, or NULL for none.
 *                          We make a shallow (pointer) copy.
 * \param [in] opt_variable Pointer to existing integer variable.
 *                          The variable must stay in scope/memory
 *                          while the options object is alive.
 * \param [in] opt_value    Assigned to \a opt_variable in this function.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_add_int (sc3_options_t * yy,
                                         char opt_short,
                                         const char *opt_long,
                                         const char *opt_help,
                                         int *opt_variable, int opt_value);

/** Add a string argument to options object.
 * \param [in,out] yy   The object must not be setup.
 * \param [in] opt_short    Short option character.
 *                          May be Nul for no short option.
 * \param [in] opt_long     Long option string, or NULL for none.
 *                          Should not include leading '--' or
 *                          white space (the latter will fail to match).
 *                          We make a shallow (pointer) copy.
 *                          It is legal for both short and long option
 *                          to be Nul/NULL.  We still assign \a opt_value.
 * \param [in] opt_help     Help string to display, or NULL for none.
 *                          We make a shallow (pointer) copy.
 * \param [in] opt_variable Pointer to existing string variable.
 *                          The variable must stay in scope/memory
 *                          while the options object is alive.
 * \param [in] opt_value    Assigned to \a opt_variable in this function.
 *                          This value and matched values are deep copied.
 *                          It is legal to pass NULL here.
 * \return                  NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_add_string (sc3_options_t * yy,
                                            char opt_short,
                                            const char *opt_long,
                                            const char *opt_help,
                                            const char **opt_variable,
                                            const char *opt_value);

/** Setup an object and change it into its usable phase.
 * \param [in,out] yy   This object must not yet be setup.  Setup on output.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_setup (sc3_options_t * yy);

/** Increase the reference count on options object by 1.
 * This is only allowed after the object has been setup.
 * \param [in,out] yy   Object must be setup.  Its refcount is increased.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_ref (sc3_options_t * yy);

/** Decrease the reference count on options object by one.
 * If the reference count drops to zero, the object is deallocated.
 * \param [in,out] yyp  The pointer must not be NULL and the object valid.
 *                      Its refcount is decreased.  If it reaches zero,
 *                      the object is destroyed and the value set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_unref (sc3_options_t ** yyp);

/** Destroy options object with a reference count of one.
 * It is a fatal error to destroy an object that is multiply referenced. We
 * unref the allocator that has been passed and refd in \ref sc3_options_new.
 * \param [in,out] yyp  This object must be valid and have a refcount of 1.
 *                      On output, value is set to NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_destroy (sc3_options_t ** yyp);

/** Parse the next matching entries of a C command-line style variable.
 * The function stops at the first argument it cannot match and returns.
 * Set result variable to differentiate non-matches from invalid arguments.
 * \param [in] yy           Options object must be setup.
 * \param [in] argc         Number of entries in the \a argv array.
 * \param [in] argv         Array of strings of length \a argc.
 *                          We do not modify this array at all.
 *                          Neither do we expect it to stay around.
 * \param [in,out] arg_pos  On input, valid index in [0, \a argc).
 *                          On output, index advanced to the next
 *                          unparsed position.  This may be the same
 *                          as its input value if no option has matched.
 *                          This may also be equal to \a argc, indicating
 *                          that no arguments are left to process.
 * \param [out] result      This is -1 for an invalid argument and otherwise
 *                          the number of matches identified.
 * \return              NULL on valid parameters and consistent operation,
 *                      options matched/invalid or not.  Return error object
 *                      when this function is called with invalid parameters.
 */
sc3_error_t        *sc3_options_parse (sc3_options_t * yy,
                                       int argc, char **argv,
                                       int *argp, int *result);

#if 0

/** Return dummy parameter of an object that is setup.
 * \param [in] yy       Object must be setup.
 * \param [out] dummy   Dummy parameter.  Pointer must not be NULL.
 * \return              NULL on success, error object otherwise.
 */
sc3_error_t        *sc3_options_get_dummy (const sc3_options_t * yy,
                                           int *dummy);

#endif

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* !SC3_OPTIONS_H */
