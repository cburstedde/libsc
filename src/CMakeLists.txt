target_sources(sc PRIVATE sc.c sc_mpi.c sc_containers.c sc_avl.c
sc_string.c sc_unique_counter.c
sc_functions.c sc_statistics.c
sc_ranges.c sc_io.c
sc_amr.c sc_search.c sc_sort.c
sc_flops.c sc_random.c
sc_polynom.c
sc_keyvalue.c sc_refcount.c sc_shmem.c
sc_allgather.c sc_reduce.c sc_notify.c
sc_uint128.c sc_v4l2.c
sc_puff.c
sc_options.c sc_getopt.c sc_getopt1.c
sc_scda.c
sc_camera.c
)

if(MSVC AND NOT SC_HAVE_GETTIMEOFDAY)
  target_sources(sc PRIVATE sc_builtin/gettimeofday.c)
endif()
