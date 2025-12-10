
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
  set ( BCRYPT_INC "${BCRYPT_ROOT_DIR}/include" CACHE PATH "Path to include files" FORCE)
  if (WIN32)
    # Windows sub-path to libs
    set ( BCRYPT_BIN "${BCRYPT_ROOT_DIR}/win64" CACHE PATH "Path to libraries" FORCE)	
  else()
    # Linux sub-path to libs
    set ( BCRYPT_BIN "${BCRYPT_ROOT_DIR}/linux" CACHE PATH "Path to libraries" FORCE)	 
  endif()

  #-- Locate Header files
  set ( OK_H "0" )
  _FIND_FILE ( BCRYPT_FILES BCRYPT_INC "bcrypt/bcrypt_class.h" "bcrypt/bcrypt_class.h" OK_H )
  if ( OK_H EQUAL 1 ) 
    message ( STATUS "  Found. Bcrypt Header files. ${BCRYPT_INCLUDE_DIR}" )
  else()
    message ( "  NOT FOUND. Bcrypt Header files" )
    set ( BCRYPT_FOUND "NO" )
  endif ()

  #-------- Locate Static Library	  
  set ( OK_LIB 0 )	  
  unset(LIST_DEBUG_LIB)
  unset(LIST_RELEASE_LIB)  
  _FIND_FILE ( LIST_DEBUG_LIB BCRYPT_BIN "bcryptd.lib" "libbcrypt.so" OK_LIB )  	    
  _FIND_FILE ( LIST_RELEASE_LIB BCRYPT_BIN "bcrypt.lib" "libbcrypt.so" OK_LIB )  	    
    
  if ( (${OK_LIB} EQUAL 2) ) 
    message ( STATUS "  Found. Bcrypt Library. ${BCRYPT_LIB_DIR}" )	   
  else()
	  set ( BCRYPT_FOUND "NO" )	   
	  message ( "  NOT FOUND. Bcrypt Library." )	   
  endif()

endif()
 
if ( ${BCRYPT_FOUND} STREQUAL "NO" )
  message( FATAL_ERROR "
    Please set BCRYPT_ROOT_DIR to the root location 
    of installed Bcrypt library containing bcrypt.lib/dll
    Not found at BCRYPT_ROOT_DIR: ${BCRYPT_ROOT_DIR}\n"
  )
endif()

set ( BCRYPT_DEBUG_LIBS "${BCRYPT_BIN}/${LIST_DEBUG_LIB}" )
set ( BCRYPT_REL_LIBS "${BCRYPT_BIN}/${LIST_RELEASE_LIB}" )

#-- We do not want user to modified these vars, but helpful to show them
message ( STATUS "  BCRYPT_ROOT_DIR: ${BCRYPT_ROOT_DIR}" )
message ( STATUS "  BCRYPT_BIN:   ${BCRYPT_BIN}" )
message ( STATUS "  BCRYPT_DEBUG_LIBS:${BCRYPT_DEBUG_LIBS}" )
message ( STATUS "  BCRYPT_REL_LIBS:  ${BCRYPT_REL_LIBS}" )

mark_as_advanced(BCRYPT_FOUND)






