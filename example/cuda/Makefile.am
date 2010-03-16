
# This file is part of the SC Library
# Makefile.am in cuda
# included non-recursively from toplevel directory

if SC_WITH_CUDA

bin_PROGRAMS += example/cuda/sc_prac5

example_cuda_sc_prac5_SOURCES = example/cuda/main.c \
        example/cuda/prac5.h example/cuda/prac5.cu

example_cuda_sc_prac5_LDADD = @NVCCLIBS@

endif
