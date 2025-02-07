# --- generate pkg-config .pc
set(pc_req_private "")
if(SC_ENABLE_MPI)
  string(APPEND pc_req_private " ompi ompi-c zlib")
elseif(SC_HAVE_ZLIB)
  string(APPEND pc_req_private " zlib")
endif()

include(cmake/utils.cmake)
convert_yn(SC_ENABLE_MPI mpi_pc)

# The OpenMP code has been moved to the attic
# convert_yn(SC_ENABLE_OPENMP openmp_pc)

convert_yn(SC_HAVE_JSON sc_have_json_pc)
convert_yn(SC_USE_INTERNAL_ZLIB zlib_pc)
convert_yn(SC_ENABLE_DEBUG debug_build_pc)

set(pc_filename libsc-${git_version}.pc)
configure_file(${CMAKE_CURRENT_LIST_DIR}/pkgconf.pc.in ${pc_filename} @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${pc_filename}
        DESTINATION lib/pkgconfig)

set(pc_target ${pc_filename})
set(pc_link lib/pkgconfig/libsc.pc)

install(CODE "
    file(CREATE_LINK ${pc_target} \${CMAKE_INSTALL_PREFIX}/${pc_link} SYMBOLIC)
")
