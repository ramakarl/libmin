
#
# Find Libmin
#
message ( "\n--- LOADING LIBMIN" )
message ( STATUS "--- Running FindLibmin.cmake" )

unset(LIBMIN_FOUND CACHE)
unset(LIBMIN_INC_DIR CACHE)

message ( STATUS "----- Searching for LibminConfig.cmake" )

# Provide paths and functions from LibminConfig.cmake
# We need the libmin config cmake first
# The Libmin_DIR should be set during bootstrap prior to calling this.
#
find_package(Libmin CONFIG )
if ( NOT DEFINED Libmin_FOUND ) 
  message( FATAL_ERROR "
      Unable to find or load LibminConfig.cmake\n"
  )
endif()

message ( STATUS "----- Searching for Libmin installed binaries " )
message ( STATUS "LIBMIN Library:")
message ( STATUS "  Searching for libmin at.. ${LIBMIN_INSTALL}")

# Find Libmin
if ( LIBMIN_INSTALL STREQUAL "NOTFOUND" )
   message( FATAL_ERROR "
      Please set LIBMIN_INSTALL to the location
      of installed binaries containing libmin.lib and .dll
      Not found at: ${LIBMIN_INSTALL}\n"
   )
else ()

    #-- Paths
	set ( LIBMIN_INC_DIR "${LIBMIN_INSTALL}/include" CACHE PATH "Path to include files" FORCE)
        set ( LIBMIN_LIB_DIR "${LIBMIN_INSTALL}/bin" CACHE PATH "Path to libraries" FORCE)
	set ( LIBMIN_SRC_DIR "${LIBMIN_INSTALL}/src" CACHE PATH "Path to libraries" FORCE)
	set ( LIBMIN_GLEW_DIR "${LIBMIN_INSTALL}/GL" CACHE PATH "Path to glew.c" FORCE)
	

	#-------- Locate Header files
    set ( OK_H "0" )
	_FIND_FILE ( LIBMIN_FILES LIBMIN_INC_DIR "common_defs.h" "common_defs.h" OK_H )	
	_FIND_FILE ( LIBMIN_FILES LIBMIN_INC_DIR "vec.h" "vec.h" OK_H )
	_FIND_FILE ( LIBMIN_FILES LIBMIN_INC_DIR "timex.h" "timex.h" OK_H )
	if ( OK_H EQUAL 3 )
	    message ( STATUS "  Found. Libhelp header files. ${LIBMIN_INCLUDE_DIR}" )
	else()
	    message ( "  NOT FOUND. Libhelp Header files" )
  	    set ( LIBMIN_FOUND FALSE )
	endif ()

    #-------- Locate Library
	if (NOT DEFINED ${BUILD_LIBMIN_STATIC} )
		message ( STATUS "  Searching for libmin dll or so. (not building libmin static)")
		set ( LIST_DLL "" )
		set ( LIST_DEBUG "" )
		set ( LIST_REL "" )
		set ( OK_DLL 0 )
		set ( OK_LIB 0 )
		_FIND_FILE ( LIST_DEBUG LIBMIN_LIB_DIR "libmind.lib" "libmin.so" OK_LIB )
		_FIND_FILE ( LIST_DLL LIBMIN_LIB_DIR "libmind.dll" "libmin.so" OK_DLL )
  
		_FIND_FILE ( LIST_REL LIBMIN_LIB_DIR "libmin.lib" "libmin.so" OK_LIB )
		_FIND_FILE ( LIST_DLL LIBMIN_LIB_DIR "libmin.dll" "libmin.so" OK_DLL )

		if ( (${OK_DLL} GREATER_EQUAL 1) AND (${OK_LIB} GREATER_EQUAL 1) )
		   message ( STATUS "  Found. Libmin so/dlls in ${LIBMIN_LIB_DIR}" )
		   set ( LIBMIN_FOUND TRUE )
		else()
		   set ( LIBMIN_FOUND FALSE )
		   message ( "  NOT FOUND. Libmin so/dlls in ${LIBMIN_LIB_DIR}" )
		endif()
	else()
	    message ( STATUS "  Building libmin static. ")
	endif()
endif()

set ( LIBMIN_DLLS ${LIST_DLL} CACHE INTERNAL "" FORCE)
set ( LIBMIN_DEBUG ${LIST_DEBUG} CACHE INTERNAL "" FORCE)
set ( LIBMIN_REL ${LIST_REL} CACHE INTERNAL "" FORCE)

#-- We do not want user to modified these vars, but helpful to show them
message ( STATUS "LIBMIN STATUS" )
message ( STATUS "  LIBMIN_SRC_DIR:  ${LIBMIN_SRC_DIR}" )
message ( STATUS "  LIBMIN_INC_DIR:  ${LIBMIN_INC_DIR}" )
message ( STATUS "  LIBMIN_GLEW_DIR: ${LIBMIN_GLEW_DIR}" )
message ( STATUS "  LIBMIN_LIB_DIR:  ${LIBMIN_LIB_DIR}" )
message ( STATUS "  LIBMIN_DLLS:     ${LIBMIN_DLLS}" )
message ( STATUS "  LIBMIN_DEBUG:    ${LIBMIN_DEBUG}" )
message ( STATUS "  LIBMIN_REL:      ${LIBMIN_REL}" )

if ( LIBMIN_FOUND )
  message ( "--- LIBMIN LOAD COMPLETE\n" )
else()
  message ( FATAL_ERROR "\nERROR: --- UNABLE TO FIND OR LOAD LIBMIN LIBRARY\n" )
endif()

set ( LIBMIN_FOUND TRUE CACHE BOOL "" FORCE)

