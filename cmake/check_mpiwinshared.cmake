include(CheckCSourceCompiles)
function(check_mpiwinshared)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          int* window_buffer;
          MPI_Win window;
          MPI_Win_allocate_shared(sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window_buffer, &window);
          return 0;
        }
    "
    SC_ENABLE_MPIWINSHARED)

endfunction()

check_mpiwinshared()
