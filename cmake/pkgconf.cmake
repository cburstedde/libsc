# --- generate pkg-config .pc
set(pc_libs_private "-llibb64")
set(pc_req_private "ompi ompi-c orte zlib")

set(pc_filename libsc-${git_version}.pc)
configure_file(${CMAKE_CURRENT_LIST_DIR}/pkgconf.pc.in ${pc_filename} @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${pc_filename} DESTINATION lib/pkgconfig)
