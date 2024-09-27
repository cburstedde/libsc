include(CheckCSourceCompiles)
function(check_mpicommshared)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          int temp = MPI_COMM_TYPE_SHARED;
          return 0;
        }
    "
    SC_ENABLE_MPICOMMSHARED)

endfunction()

check_mpicommshared()
