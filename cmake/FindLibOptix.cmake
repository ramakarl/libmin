
#
# Find LibOptix

unset(LIBOPTIX_FOUND CACHE)
unset(LIBOPTIX_INC CACHE)

if ( NOT DEFINED LIBOPTIX_ROOT )    
  get_filename_component ( BASEDIR "${LIBMIN_INSTALL}/../liboptix" REALPATH )  
  set ( LIBOPTIX_ROOT ${BASEDIR} CACHE PATH "Location of liboptix" FORCE)
endif()
message ( STATUS "Searching for liboptix at.. ${LIBOPTIX_ROOT_DIR}")
set( LIBOPTIX_FOUND "YES" )

if ( LIBOPTIX_ROOT )

    #-- Paths 
	set ( LIBOPTIX_INC "${LIBOPTIX_ROOT}/include" CACHE PATH "Path to include files" FORCE)
    set ( LIBOPTIX_LIB "${LIBOPTIX_ROOT}/lib" CACHE PATH "Path to libraries" FORCE)	

	#-------- Locate Header files
    set ( OK_H "0" )
	_FIND_FILE ( LIBOPTIX_FILES LIBOPTIX_INC "optix_scene.h" "optix_scene.h" OK_H )
	_FIND_FILE ( LIBOPTIX_FILES LIBOPTIX_INC "optix_trace.h" "optix_trace.h" OK_H )	
	if ( OK_H EQUAL 2 ) 
	    message ( STATUS "  Found. liboptix header files. ${LIBOPTIX_INCLUDE_DIR}" )
	else()
	    message ( "  NOT FOUND. liboptix Header files" )
  	    set ( LIBOPTIX_FOUND "NO" )
	endif ()

    #-------- Locate Library	
    set ( LIST_DLL "" )
	set ( LIST_DEBUG "" )
	set ( LIST_REL "" )
    set ( OK_DLL 0 )	
    set ( OK_LIB 0 )
	_FIND_FILE ( LIST_DEBUG LIBOPTIX_LIB "liboptixd.lib" "liboptixd.so" OK_LIB )  	
	_FIND_FILE ( LIST_REL LIBOPTIX_LIB "liboptix.lib" "liboptix.so" OK_LIB )  	
	
	#-- libraries required by libfull (even if app doesn't need them)
	_FIND_FILE ( LIST_DLL LIBOPTIX_LIB "liboptixd.dll" "" OK_DLL )			
	_FIND_FILE ( LIST_DLL LIBOPTIX_LIB "liboptix.dll" "" OK_DLL )		
	
	if ( (${OK_DLL} GREATER_EQUAL 1) AND (${OK_LIB} GREATER_EQUAL 1) ) 
	   message ( STATUS "  Found. liboptix Library. ${LIBOPTIX_LIB_DIR}" )	   
	else()
	   set ( LIBOPTIX_FOUND "NO" )	   
	   message ( "  NOT FOUND. liboptix Library. (so/dll or lib missing)" )	   
	endif()
	
	#-- optix ptx
	_FIND_MULTIPLE( LIST_PTX "${LIBOPTIX_LIB}" "ptx" "ptx")

endif()
 
if ( ${LIBOPTIX_FOUND} STREQUAL "NO" )
   message( FATAL_ERROR "
      Please set LIBOPTIX_ROOT_DIR to the root location 
      of installed Libhelp library containing libhelp_full.lib/dll
      Not found at LIBOPTIX_ROOT_DIR: ${LIBOPTIX_ROOT_DIR}\n"
   )
endif()

set ( LIBOPTIX_DLLS ${LIST_DLL} CACHE INTERNAL "" FORCE)
set ( LIBOPTIX_DEBUG ${LIST_DEBUG} CACHE INTERNAL "" FORCE)
set ( LIBOPTIX_REL ${LIST_REL} CACHE INTERNAL "" FORCE)
set ( LIBOPTIX_PTX ${LIST_PTX} CACHE INTERNAL "" FORCE)

#-- We do not want user to modified these vars, but helpful to show them
message ( STATUS "  LIBOPTIX_ROOT: ${LIBOPTIX_ROOT}" )
message ( STATUS "  LIBOPTIX_INC:  ${LIBOPTIX_INC}" )
message ( STATUS "  LIBOPTIX_LIB:  ${LIBOPTIX_LIB}" )
message ( STATUS "  LIBOPTIX_DLL:  ${LIBOPTIX_DLLS}" )
message ( STATUS "  LIBOPTIX_DEBUG:${LIBOPTIX_DEBUG}" )
message ( STATUS "  LIBOPTIX_REL:  ${LIBOPTIX_REL}" )
message ( STATUS "  LIBOPTIX_PTX:  ${LIBOPTIX_PTX}" )

mark_as_advanced(LIBOPTIX_FOUND)


