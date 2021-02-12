# avoid extremely long error printouts as it's ususally the first error that's relevant
set(_maxerr 3)

if(CMAKE_C_COMPILER_ID STREQUAL GNU)
  add_compile_options(-fmax-errors=${_maxerr})
elseif(CMAKE_C_COMPILER_ID STREQUAL Intel)
  add_compile_options(-diag-error-limit=${_maxerr})
elseif(CMAKE_C_COMPILER_ID STREQUAL IntelLLVM)
  add_compile_options(-fmax-errors=${_maxerr})
endif()
