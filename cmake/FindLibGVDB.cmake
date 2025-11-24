
#####################################################################################
# Find GVDB
#
unset(GVDB_FOUND CACHE)
unset(GVDB_INCLUDE_DIR CACHE)

# NOTE:
# GVDB_ROOT is the path to the source repository. eg. /libgvdb
# GVDB_INSTALL is the path to the final, compiled binaries. eg. /build/libgvdb

if ( NOT DEFINED GVDB_INSTALL )
  get_filename_component ( BASEDIR "${LIBMIN_ROOT}/../build/libgvdb" REALPATH )
  set ( GVDB_INSTALL ${BASEDIR} CACHE PATH "Location of GVDB library" FORCE)
endif()
if ( GVDB_INSTALL STREQUAL "NOTFOUND" )
  message ( FATAL_ERROR "
  GVDB Library is required.   
  Compile libgvdb first. Set GVDB_INSTALL to the *installed* location of libgvdb.
  ")
endif()
message ( STATUS "\n  Searching for compiled GVDB installation at.. ${GVDB_INSTALL}")

#-- Paths to installed GVDB Library
# *NOTE* Kernel headers .cuh will also be placed into installed GVDB_INCLUDE_DIR location
#
set ( GVDB_INC_DIR "${GVDB_INSTALL}/include" )
set ( GVDB_LIB_DIR "${GVDB_INSTALL}/lib" )

file( GLOB GVDB_PTX ${GVDB_INSTALL}/lib/*.ptx)
file( GLOB GVDB_GLSL ${GVDB_INSTALL}/lib/*.glsl)
file( GLOB GVDB_DEBUG_LIB "${GVDB_INSTALL}/Debug/libgvdbd.lib" )
file( GLOB GVDB_REL_LIB "${GVDB_INSTALL}/Release/libgvdb.lib" )

#-- only when building shared
# file( GLOB GVDB_DLLS "${GVDB_LIB_DIR}/libgvdb.dll" "${GVDB_LIB_DIR}/libgvdbd.dll")

# Mark found if the correct vars exist 
find_package_handle_standard_args(LibGVDB DEFAULT_MSG GVDB_INC_DIR GVDB_PTX GVDB_GLSL GVDB_DEBUG_LIB GVDB_REL_LIB )

if (LIBGVDB_FOUND)

  # Found. Show results.
  message ( STATUS "  Found library GVDB. ${GVDB_INSTALL} ")
  message ( STATUS "  GVDB_INSTALL: ${GVDB_INSTALL}" )
  message ( STATUS "  GVDB_INC_DIR: ${GVDB_INC_DIR}" )
  message ( STATUS "  GVDB_DEBUG: ${GVDB_DEBUG_LIB}" )
  message ( STATUS "  GVDB_REL:   ${GVDB_REL_LIB}" )
  message ( STATUS "  GVDB_DLLS:  ${GVDB_DLLS}" )
  message ( STATUS "  GVDB_PTX:   ${GVDB_PTX}" )
  message ( STATUS "  GVDB_GLSL:  ${GVDB_GLSL}" )  

endif ()
