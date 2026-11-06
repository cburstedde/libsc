include(CheckCSourceCompiles)
function(check_mpideriveddatatypes)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  # check that MPI_Type_contigous, MPI_Type_commit and MPI_Type_free are
  # available all at once (MPI_Type_free needs a commited, custom datatype)
  set(CMAKE_REQUIRED_QUIET 1)
  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          MPI_Datatype custom;
          MPI_Init ((int *) 0, (char ***) 0);
          MPI_Type_contiguous (2, MPI_BYTE, &custom);
          MPI_Type_commit (&custom);
          MPI_Type_free (&custom);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_HAVE_TYPE_FREE)
    if (NOT SC_HAVE_TYPE_FREE)
      # we want consistent output messages for both test programs
      message (CHECK_START "Performing test SC_HAVE_DERIVED_DATATYPES")
      message (CHECK_FAIL "Failed")
      return ()
    endif()

    set(CMAKE_REQUIRED_QUIET 0)
    check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          int block_lens[1];
          MPI_Aint displacements[1];
          MPI_Datatype types[1], custom;
          block_lens[0] = 1;
          displacements[0] = 0;
          types[0] = MPI_BYTE;
          MPI_Init ((int *) 0, (char ***) 0);
          MPI_Type_create_struct (1, block_lens, displacements, types,
                                  &custom);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_HAVE_DERIVED_DATATYPES)

endfunction()

check_mpideriveddatatypes()
