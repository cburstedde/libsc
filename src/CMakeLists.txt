target_sources(sc PRIVATE sc.c sc_mpi.c sc_containers.c sc_avl.c sc_string.c sc_unique_counter.c sc_functions.c sc_statistics.c sc_ranges.c sc_io.c sc_amr.c sc_search.c sc_sort.c sc_flops.c sc_random.c sc_polynom.c sc_keyvalue.c sc_refcount.c sc_shmem.c sc_allgather.c sc_reduce.c sc_notify.c sc_uint128.c)

if(SC_HAVE_GETOPT_H)
  target_sources(sc PRIVATE sc_options.c sc_getopt.c sc_getopt1.c)
endif()

if(SC_HAVE_WINSOCK2_H)
  target_sources(sc PRIVATE sc_builtin/gettimeofday.c)
  target_link_libraries(sc PRIVATE ${WINSOCK_LIBRARIES})
endif()
