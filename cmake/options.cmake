option(mpi "use MPI library" off)
option(openmp "use OpenMP" off)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# --- default install directory under build/local
# users can specify like "cmake -B build --install-prefix=$HOME/mydir"
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # will not take effect without FORCE
  set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/local" CACHE PATH "Install top-level directory" FORCE)
endif()

# --- auto-ignore build directory
if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
  file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
endif()
