include(CheckCSourceCompiles)
function(check_mpithread)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(int argc, char** argv) {
          int thread_support_provided;
          MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &thread_support_provided);
          return 0;
        }
    "
    SC_ENABLE_MPITHREAD)

endfunction()

check_mpithread()
