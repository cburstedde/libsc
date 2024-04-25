# --- generate pkg-config .pc
set(pc_req_private "")
if(SC_ENABLE_MPI)
  string(APPEND pc_req_private " ompi ompi-c zlib")
elseif(SC_HAVE_ZLIB)
  string(APPEND pc_req_private " zlib")
endif()


#
# macro to convert a boolean variable (ON/OFF) to 0/1.
#
macro(convert_01 varin varout)

  if(NOT DEFINED ${varin})
    message(FATAL_ERROR "variable ${varin} not defined")
  endif()

  if(${varin})
    set(${varout} 1)
  else()
    set(${varout} 0)
  endif()

endmacro()

#
# macro to convert a boolean variable (ON/OFF) to yes/no.
#
macro(convert_yn varin varout)

  if(NOT DEFINED ${varin})
    message(FATAL_ERROR "variable ${varin} not defined")
  endif()

  if(${varin})
    set(${varout} yes)
  else()
    set(${varout} no)
  endif()

endmacro()


convert_yn(SC_ENABLE_MPI mpi_pc)
convert_yn(SC_ENABLE_OPENMP openmp_pc)
convert_yn(SC_HAVE_JSON sc_have_json_pc)
convert_yn(SC_USE_INTERNAL_ZLIB zlib_pc)
convert_yn(SC_ENABLE_DEBUG debug_build_pc)

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
