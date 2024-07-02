
#####################################################################################
# Find Bcrypt

unset(BCRYPT_FOUND CACHE)
unset(BCRYPT_INCLUDE_DIR CACHE)
unset(BCRYPT_DLL)
unset(BCRYPT_LIB)

if ( NOT DEFINED BCRYPT_ROOT_DIR )  
  set ( BCRYPT_ROOT_DIR ${LIBEXT_ROOT} CACHE PATH "Location of bcrypt" FORCE)    
endif()

message ( STATUS "Searching for Bcrypt at.. ${BCRYPT_ROOT_DIR}")
set( BCRYPT_FOUND "YES" )

if ( BCRYPT_ROOT_DIR )

    #-- Paths 
	set ( BCRYPT_INCLUDE_DIR "${BCRYPT_ROOT_DIR}/include" CACHE PATH "Path to include files" FORCE)
    if (WIN32)
       # Windows sub-path to libs
	   set ( BCRYPT_LIB_DIR "${BCRYPT_ROOT_DIR}/win64" CACHE PATH "Path to libraries" FORCE)	
    else()
       # Linux sub-path to libs
       set ( BCRYPT_LIB_DIR "${BCRYPT_ROOT_DIR}/linux" CACHE PATH "Path to libraries" FORCE)	
    endif()

	#-------- Locate Header files
    set ( OK_H "0" )
	_FIND_FILE ( BCRYPT_FILES BCRYPT_INCLUDE_DIR "bcrypt/bcrypt_class.h" "bcrypt/bcrypt_class.h" OK_H )
	if ( OK_H EQUAL 1 ) 
	    message ( STATUS "  Found. Bcrypt Header files. ${BCRYPT_INCLUDE_DIR}" )
	else()
	    message ( "  NOT FOUND. Bcrypt Header files" )
  	    set ( BCRYPT_FOUND "NO" )
	endif ()

    #-------- Locate Library	
    set ( OK_DLL 0 )	
    set ( OK_LIB 0 )	
    unset(LIST_DLL)
    unset(LIST_DEBUG_LIB)
    unset(LIST_RELEASE_LIB)
    _FIND_FILE ( LIST_DLL BCRYPT_LIB_DIR "bcrypt.dll" "libbcrypt.so" OK_DLL )		
    _FIND_FILE ( LIST_DLL BCRYPT_LIB_DIR "bcryptd.dll" "libbcrypt.so" OK_DLL )		
    _FIND_FILE ( LIST_DEBUG_LIB BCRYPT_LIB_DIR "bcryptd.lib" "libbcrypt.so" OK_LIB )  	    
    _FIND_FILE ( LIST_RELEASE_LIB BCRYPT_LIB_DIR "bcrypt.lib" "libbcrypt.so" OK_LIB )  	    
    
	if ( (${OK_DLL} EQUAL 2) AND (${OK_LIB} EQUAL 2) ) 
	   message ( STATUS "  Found. Bcrypt Library. ${BCRYPT_LIB_DIR}" )	   
	else()
	   set ( BCRYPT_FOUND "NO" )	   
	   message ( "  NOT FOUND. Bcrypt Library. (so/dll or lib missing)" )	   
	endif()

endif()
 
if ( ${BCRYPT_FOUND} STREQUAL "NO" )
   message( FATAL_ERROR "
      Please set BCRYPT_ROOT_DIR to the root location 
      of installed Bcrypt library containing bcrypt.lib/dll
      Not found at BCRYPT_ROOT_DIR: ${BCRYPT_ROOT_DIR}\n"
   )
endif()

set ( BCRYPT_DLL ${LIST_DLL} CACHE INTERNAL "" FORCE)
set ( BCRYPT_DEBUG_LIB ${LIST_DEBUG_LIB} CACHE INTERNAL "" FORCE)
set ( BCRYPT_RELEASE_LIB ${LIST_RELEASE_LIB} CACHE INTERNAL "" FORCE)

#-- We do not want user to modified these vars, but helpful to show them
message ( STATUS "  BCRYPT_ROOT_DIR: ${BCRYPT_ROOT_DIR}" )
message ( STATUS "  BCRYPT_DLL:  ${BCRYPT_DLL}" )
message ( STATUS "  BCRYPT_DEBUG_LIB:  ${BCRYPT_DEBUG_LIB}" )
message ( STATUS "  BCRYPT_RELEASE_LIB:  ${BCRYPT_RELEASE_LIB}" )

mark_as_advanced(BCRYPT_FOUND)






