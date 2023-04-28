# Define simple macro to ease interoperability with autotools.
# These macro are used to write variable values in pgk-config files.

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
