
#####################################################################################
# Find OpenSSL

unset(OPENSSL_FOUND CACHE)
unset(OPENSSL_INC CACHE)

if ( NOT DEFINED OPENSSL_ROOT )
  if (WIN32)
    get_filename_component ( BASEDIR "${LIBEXT_ROOT}" REALPATH )
  else()
    get_filename_component ( BASEDIR "/usr/" REALPATH )
  endif()
  set ( OPENSSL_ROOT ${BASEDIR} CACHE PATH "Location of OpenSSL library" FORCE)
endif()
message ( STATUS "Searching for OpenSSL at.. ${OPENSSL_ROOT}")
set( OPENSSL_FOUND "YES" )

if ( OPENSSL_ROOT )

    #-- Paths to library (cached so user can modify)
    set ( OPENSSL_INC "${OPENSSL_ROOT}/include" CACHE PATH "Path to include files" FORCE)
    if (WIN32)
       # Windows sub-path to libs
       set ( OPENSSL_BIN "${OPENSSL_ROOT}/win64" CACHE PATH "Path to libraries" FORCE)	
    else()
       # Linux sub-path to libs
       set ( OPENSSL_BIN "${OPENSSL_ROOT}/lib/x86_64-linux-gnu" CACHE PATH "Path to libraries" FORCE)	
    endif()

	#-------- Locate Header files
    set ( OK_H "0" )
	_FIND_FILE ( OPENSSL_FILES OPENSSL_INC "openssl/ssl.h" "openssl/ssl.h" OK_H )
	if ( OK_H EQUAL 1 ) 
	    message ( STATUS "  Found. OpenSSL Header files. ${OPENSSL_INCLUDE_DIR}" )
	else()
	    message ( "  NOT FOUND. OpenSSL Header files" )
  	    set ( OPENSSL_FOUND "NO" )
	endif ()

   #-------- Locate Link/static Libs
   set ( OK_LIB 0 )	 
   unset ( LIST_LIB )	
	_FIND_FILE ( LIST_LIB OPENSSL_BIN "libssl.lib" "libssl.a" OK_LIB )
  _FIND_FILE ( LIST_LIB OPENSSL_BIN "libcrypto.lib" "libcrypto.a" OK_LIB )
  
  #-------- Locate Shared dll/so
  set ( OK_DLL 0 )	
  unset ( LIST_DLL )	
  _FIND_FILE ( LIST_DLL OPENSSL_BIN "libssl-3-x64.dll" "libssl.so" OK_DLL )	
	_FIND_FILE ( LIST_DLL OPENSSL_BIN "libcrypto-3-x64.dll" "libcrypto.so" OK_DLL )	
    
	if ( (${OK_DLL} EQUAL 2) AND (${OK_LIB} EQUAL 2) ) 
	   message ( STATUS "  Found. OpenSSL Library. ${OPENSSL_BIN}" )
	else()
	   set ( OPENSSL_FOUND "NO" )	   
	   message ( "  NOT FOUND. OpenSSL Library. (so/dll or lib missing)" )	   
	endif()

endif()
 
if ( ${OPENSSL_FOUND} STREQUAL "NO" )
   message( FATAL_ERROR "
      Please set OPENSSL_ROOT to the root location 
      of installed OpenSSL library containing libssl.lib and libcrypto.lib
      Not found at OPENSSL_ROOT: ${OPENSSL_ROOT}\n"
   )
endif()

set ( OPENSSL_DLLS ${LIST_DLL} CACHE INTERNAL "" FORCE)
set ( OPENSSL_DEBUG_LIBS ${LIST_LIB} CACHE INTERNAL "" FORCE)
set ( OPENSSL_REL_LIBS ${LIST_LIB} CACHE INTERNAL "" FORCE)

#-- We do not want user to modified these vars, but helpful to show them
message ( STATUS "  OPENSSL_ROOT: ${OPENSSL_ROOT}" )
message ( STATUS "  OPENSSL_DLL:  ${OPENSSL_DLLS}" )
message ( STATUS "  OPENSSL_LIB:  ${OPENSSL_DEBUG_LIBS}" )

mark_as_advanced(OPENSSL_FOUND)






