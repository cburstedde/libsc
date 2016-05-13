
# This file is part of the SC Library
# Makefile.am in example/pthread
# included non-recursively from toplevel directory

if SC_ENABLE_PTHREAD

bin_PROGRAMS += example/pthread/sc_pthread example/pthread/sc_condvar
example_pthread_sc_pthread_SOURCES = example/pthread/pthread.c
example_pthread_sc_condvar_SOURCES = example/pthread/condvar.c

LINT_CSOURCES += $(example_pthread_sc_pthread_SOURCES) \
                 $(example_pthread_sc_condvar_SOURCES)

endif
