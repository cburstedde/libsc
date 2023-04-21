# --- generate pkg-config .pc
set(pc_req_private "")
if(mpi)
  string(APPEND pc_req_private " ompi ompi-c zlib")
elseif(zlib)
  string(APPEND pc_req_private " zlib")
endif()

set(pc_filename libsc-${git_version}.pc)
configure_file(${CMAKE_CURRENT_LIST_DIR}/pkgconf.pc.in ${pc_filename} @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${pc_filename}
        DESTINATION lib/pkgconfig)
