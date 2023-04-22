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

set(pc_target ${pc_filename})
set(pc_link ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/libsc.pc)

install(CODE "execute_process( \
    COMMAND ${CMAKE_COMMAND} -E create_symlink \
    ${pc_target} \
    ${pc_link}   \
    )"
  )
