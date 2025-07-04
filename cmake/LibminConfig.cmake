
####################################################################################
# LIBMIN CONFIG
#
# Provide the libmin repository path, install path, and libext path
# for access to mains, third-party libs and cmake helpers
#  LIBMIN_ROOT_PATH    = path to required source repository (eg. libmin/src)
#  LIBMIN_INSTALL_PATH = path to installed libmin binaries (eg. libmin.lib, .dll, .so)
#  LIBEXT_PATH         = path to third-party libs (eg. libjpg, libgvdb, liboptix)
#
function(_CONFIRM_PATH outvar targetPath targetWin targetLinux varName isFatal )
  set(oneValueArgs outVar targetPath targetWin targetLinux varName )  
  unset ( result CACHE )  
  if ( WIN32 )
    find_file ( result ${targetWin} PATHS ${targetPath} )  
  else()
    find_file ( result ${targetLinux} PATHS ${targetPath} )  
  endif()
  if ( ${result} STREQUAL "result-NOTFOUND" )
    set (${outvar} "NOTFOUND" CACHE PATH "" )    
    set (found "NOTFOUND")
  else()
    set (${outvar} ${result} CACHE PATH "" )    
    set (found "OK")
  endif()     
  if ( isFatal ) 
    message ( STATUS "  Confirming: ${targetFile} in ${targetPath} -> ${found}")
    if ( ${outvar} STREQUAL "NOTFOUND" )
      message ( FATAL_ERROR "\n  Cannot find ${varName} at ${targetPath}.
         Please set ${varName} to the correct path, which should contain
         files such as ${targetFile}.\n"   
      )
    endif()
  endif()
  unset ( result CACHE )
endfunction()

##############
# LIBMIN_ROOT - libmin src, should be set by caller during bootstrap
#
message ( STATUS "  LIBMIN_ROOT: ${LIBMIN_ROOT}")
get_filename_component ( LIBMIN_ROOT "${LIBMIN_ROOT}" REALPATH)
set ( LIBMIN_ROOT "${LIBMIN_ROOT}" CACHE PATH "Path to /libmin source" )
_CONFIRM_PATH ( LIBMIN_ROOT "${LIBMIN_ROOT}" "/src/dataptr.cpp" "/src/dataptr.cpp" "LIBMIN_ROOT" TRUE)

##############
# LIBEXT_ROOT - libext src, by default use the minimal /libext that comes with libmin
#
# There are two versions of libext.
#  libmin/libext - Local libext    - Includes only bcrypt, tinygltf, libjpg, openssl
#  libext        - Extended libext - Includes fftw3.3, laszip, portaudio, sqlite, optix, gvdb
#
# For smaller apps, this keeps down the repository/download size for libmin small.
#
if (NOT DEFINED BUILD_LIBEXT_EXTENDED )
   option ( BUILD_LIBEXT_EXTENDED "Use external libext" FALSE )
endif()
if ( NOT DEFINED LIBEXT_ROOT )
  if (BUILD_LIBEXT_EXTNEDED) 
     # check for /libext with extended third party libs - may return NOTFOUND
     CONFIRM_PATH ( LIBEXT_EXTENDED "${LIBMIN_ROOT}/../libext/" "cmake/FindFFTW.cmake" "cmake/FindFFTW.cmake" "LIBEXT_EXTENDED" FALSE)
  else()
     SET ( LIBEXT_EXTENDED "NOTFOUND" )
  endif()
  if ( LIBEXT_EXTENDED STREQUAL "NOTFOUND" ) 
    # Use Local libext (found in libmin/libext)
    get_filename_component ( LIBEXT_ROOT "${LIBMIN_ROOT}/libext" REALPATH)    
  else()
    # Use Extended libext (search at a sibling path of libmin)
    get_filename_component ( LIBEXT_ROOT "${LIBMIN_ROOT}/../libext" REALPATH)       
  endif()
else() 
  # use cached or user-set path for /libext
  get_filename_component ( LIBEXT_ROOT "${LIBEXT_ROOT}" REALPATH)
endif()
_CONFIRM_PATH ( LIBEXT_EXTENDED "${LIBEXT_ROOT}" "cmake/FindFFTW.cmake" "cmake/FindFFTW.cmake" "LIBEXT_EXTENDED" FALSE)
if ( LIBEXT_EXTENDED STREQUAL "NOTFOUND" ) 
  message ( "  LIBEXT using internal to LIBMIN at ${LIBEXT_ROOT}")    
else()
  message ( "  LIBEXT using *extended* third-parties at ${LIBEXT_ROOT}.")    
endif()  
set ( LIBEXT_ROOT ${LIBEXT_ROOT} CACHE PATH "Path to /libext source" )
_CONFIRM_PATH ( LIBEXT_ROOT "${LIBEXT_ROOT}" "/include/openssl/bio.h" "/include/openssl/bio.h" "LIBEXT_ROOT" TRUE)

################
# LIBMIN_INSTALL - libmin binaries, by default assume its in /build/libmin 
# if set to "SELF" it means we are building libmin itself, and dont need the install path
#
if ( NOT LIBMIN_INSTALL STREQUAL "SELF" )
  if ( NOT DEFINED LIBMIN_INSTALL )
    get_filename_component ( LIBMIN_INSTALL "${LIBMIN_ROOT}/../build/libmin" REALPATH)
  else()
    get_filename_component ( LIBMIN_INSTALL "${LIBMIN_INSTALL}" REALPATH)
  endif()
  set ( LIBMIN_INSTALL ${LIBMIN_INSTALL} CACHE PATH "Path to /libmin installed binaries" )
  _CONFIRM_PATH ( LIBMIN_INSTALL "${LIBMIN_INSTALL}" "/bin/libmind.lib" "/libmin.so" "LIBMIN_INSTALL" TRUE)
endif()

# Repository paths
set ( LIBMIN_ROOT_MAINS "${LIBMIN_ROOT}/mains" )
set ( LIBMIN_ROOT_SRC "${LIBMIN_ROOT}/src")
set ( LIBMIN_ROOT_INC "${LIBMIN_ROOT}/include")
set ( LIBEXT_ROOT ${LIBEXT_ROOT} )

#set ( DEBUG_HEAP false CACHE BOOL "Enable heap checking (debug or release).")
set ( DEBUG_HEAP true CACHE BOOL "Enable heap checking (debug or release).")
if ( ${DEBUG_HEAP} )
   add_definitions( -DDEBUG_HEAP)
   add_definitions( -D_CRTDBG_MAP_ALLOC)
endif()

message ( STATUS "----- Running LibminConfig.cmake" )
message ( STATUS "  CURRENT DIRECTORY: ${CMAKE_CURRENT_SOURCE_DIR}" )
message ( STATUS "  LIBMIN REPOSITORY: ${LIBMIN_ROOT}" )
message ( STATUS "  LIBEXT REPOSITORY: ${LIBEXT_ROOT}" )
message ( STATUS "  LIBMIN INSTALLED:  ${LIBMIN_INSTALL}" )

# BOOTSTRAP COMPLETE
###############################

#-------------------------------------- COPY CUDA BINS
# This macro copies all binaries for the CUDA library to the target executale location. 
#
macro(_copy_cuda_bins projname )	
	add_custom_command(
		TARGET ${projname} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CUDA_DLL} $<TARGET_FILE_DIR:${PROJNAME}>   
	)
endmacro()

####################################################################################
# Provide Libext - extended 3rd party libraries: 
#  OpenSSL, Laszip, OptiX, PortAudio, CUFFT, PixarUSD, LibGDVB, LibOptiX
#
function (_REQUIRE_LIBEXT)      
    message (STATUS "  Searching for LIBEXT... ${LIBEXT_ROOT}")
    if (EXISTS "${LIBEXT_ROOT}" AND IS_DIRECTORY "${LIBEXT_ROOT}")
        set( LIBEXT_FOUND TRUE )
        message (STATUS "  ---> Using LIBEXT: ${LIBEXT_ROOT}")
        list( APPEND CMAKE_MODULE_PATH "${LIBEXT_ROOT}/cmake" )
        list( APPEND CMAKE_PREFIX_PATH "${LIBEXT_ROOT}/cmake" )        
        set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
        set( CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
        set( LIBEXT_FOUND ${LIBEXT_FOUND} PARENT_SCOPE)
    else()
message (FATAL_ERROR "  LIBEXT Not Found at: ${LIBEXT_ROOT}. 
Set LIBEXT_ROOT to the location of libext.
")
    endif()
endfunction()

###################################################################################
# Provide a cross-platform main
#
function ( _REQUIRE_MAIN )
    # Add Main to build

    OPTION (BUILD_CONSOLE "Build console app" OFF)

    add_definitions(-DUSE_MAIN)
    IF(BUILD_CONSOLE)
    	LIST( APPEND SRC_FILES "${LIBMIN_ROOT_MAINS}/main_console.cpp" )
    ELSE()
	    IF(WIN32)
  		    LIST( APPEND SRC_FILES "${LIBMIN_ROOT_MAINS}/main_win.cpp" )
	    ELSEIF(ANDROID)
		    LIST( APPEND SRC_FILES "${LIBMIN_ROOT_MAINS}/main_android.cpp" )
	    ELSE()
	  	    LIST( APPEND SRC_FILES "${LIBMIN_ROOT_MAINS}/main_x11.cpp" )
	    ENDIF()
    ENDIF()
	LIST( APPEND SRC_FILES "${LIBMIN_ROOT_MAINS}/main.h" )
    include_directories( ${LIBMIN_ROOT_MAINS} )

    set ( PACKAGE_SOURCE_FILES ${SRC_FILES} PARENT_SCOPE )
endfunction()

###################################################################################
# Include OpenGL 
#
function ( _REQUIRE_GL )
    OPTION (BUILD_OPENGL "Build with OpenGL" ON)
    if (BUILD_OPENGL)            
        message ( STATUS "  Searching for GL.." )
        find_package(OpenGL)
        if (OPENGL_FOUND)          
          include_directories("${LIBEXT_PATH}/include")
          add_definitions(-DUSE_OPENGL)  		# Use OpenGL
          add_definitions(-DBUILD_OPENGL)  		
          IF (WIN32)
	       LIST(APPEND LIBRARIES_OPTIMIZED "opengl32.lib" )
	       LIST(APPEND LIBRARIES_DEBUG "opengl32.lib" )
           set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
           set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
          ENDIF()
          message ( STATUS "  ---> Using GL" )
        endif()
    endif()
endfunction()

function ( _REQUIRE_GLEW)    
    OPTION (BUILD_GLEW "Build with GLEW" ON)
    if (BUILD_GLEW)
      include_directories("${LIBEXT_ROOT}/include")
      if (WIN32)        
        LIST(APPEND LIBRARIES_OPTIMIZED "${LIBEXT_ROOT}/win64/glew64.lib" )
	    LIST(APPEND LIBRARIES_DEBUG "${LIBEXT_ROOT}/win64/glew64d.lib" )
        set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
        set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)        
        LIST(APPEND GLEW_FILES "${LIBEXT_ROOT}/win64/glew64.dll" )
        LIST(APPEND GLEW_FILES "${LIBEXT_ROOT}/win64/glew64d.dll" )
        message ( "GLEW: ${GLEW_FILES}")
        set(GLEW_FILES ${GLEW_FILES} PARENT_SCOPE)     

      endif()
      message ( STATUS "  ---> Using GLEW (dll)" )        
    endif()
endfunction()


###################################################################################
# Include OpenSSL
#
function ( _REQUIRE_OPENSSL use_openssl_default)
    OPTION (BUILD_OPENSSL "Build with OpenSSL" ${use_openssl_default} )
    if (BUILD_OPENSSL) 

	    find_package(OpenSSL)

	    if ( OPENSSL_FOUND )	
		    add_definitions(-DBUILD_OPENSSL)
	
		    include_directories ( ${OPENSSL_INCLUDE_DIR} )
		    link_directories ( ${OPENSSL_LIB_DIR} )
		    LIST( APPEND LIBRARIES_OPTIMIZED "${OPENSSL_LIB}")
		    LIST( APPEND LIBRARIES_DEBUG "${OPENSSL_LIB}")	
	        set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
        	set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
		    _EXPANDLIST( OUTPUT PACKAGE_DLLS SOURCE ${OPENSSL_LIB_DIR} FILES ${OPENSSL_DLL} )
            set(PACKAGE_DLLS ${PACKAGE_DLLS} PARENT_SCOPE)
		    message ( STATUS "--> Using OpenSSL, ${OPENSSL_LIB_DIR}/${OPENSSL_DLL} ")
	    else()
		    message ( FATAL_ERROR "\n  Unable to find OpenSLL library. Link with a different /libext for third-party libs that include OpenSSL.\n")
	    endif()
    endif()
endfunction()

#####################################################################################
# Include Bcrypt
#
function ( _REQUIRE_BCRYPT use_bcrypt_default)
    OPTION (BUILD_BCRYPT "Build with Bcrypt" ${use_bcrypt_default} )
    if (BUILD_BCRYPT) 
        find_package(Bcrypt)

        if ( BCRYPT_FOUND )	
	        if ( NOT DEFINED USE_BCRYPT )	       
		        SET(USE_BCRYPT ON CACHE BOOL "Use Bcrypt")
	        endif()
	        if ( USE_BCRYPT )
		        add_definitions(-DUSE_BCRYPT)		
		        message ( STATUS " Using Bcrypt")
		        include_directories ( ${BCRYPT_INCLUDE_DIR} )
		        link_directories ( ${BCRYPT_LIB_DIR} )
		        LIST( APPEND LIBRARIES_DEBUG "${BCRYPT_DEBUG_LIB}" )	
		        LIST( APPEND LIBRARIES_OPTIMIZED "${BCRYPT_RELEASE_LIB}" )   
                set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
                set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
		        _EXPANDLIST( OUTPUT PACKAGE_DLLS SOURCE ${BCRYPT_LIB_DIR} FILES ${BCRYPT_DLL})
	        endif()
        endif()
    endif()
endfunction()

#####################################################################################
# Include JPG    
#   
function ( _REQUIRE_JPG )  
 
     set ( OK_H "0" )    
     if (WIN32) 
       set ( _jpg_srch "${LIBEXT_ROOT}/win64" )
     else()
       set ( _jpg_srch "/usr/lib/x86_64-linux-gnu/" )
     endif()

     _FIND_FILE ( JPG_LIBS _jpg_srch "libjpg_2019x64.lib" "libjpeg.so" OK_H )		
       
    if ( OK_H EQUAL 1 )
      if (WIN32)
        list ( APPEND LIBRARIES_OPTIMIZED "${LIBEXT_ROOT}/win64/libjpg_2019x64.lib" )
        list ( APPEND LIBRARIES_DEBUG "${LIBEXT_ROOT}/win64/libjpg_2019x64d.lib" )
      else()
	find_package ( JPEG REQUIRED )
	list ( APPEND LIBRARIES_OPTIMIZED ${JPEG_LIBRARY} )
	list ( APPEND LIBRARIES_DEBUG ${JPEG_LIBRARY} )
	message ( STATUS "  Using JPEG_LIBRARY: ${JPEG_LIBRARY}" )
      endif()
      set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
      set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
      include_directories( "${LIBEXT_ROOT}/include" )
      add_definitions(-DBUILD_JPG) 
      message ( STATUS "  ---> Using libjpg" )
    else ()
      message ( FATAL_ERROR "
      JPG libraries not found. Please install libjpeg-dev or libmin/libext.
      ")
    endif()
endfunction()

#####################################################################################
# Include TinyGLTF
#   
function ( _REQUIRE_GLTF )   
     set ( OK_H "0" )    
     set ( _gltf_srch "${LIBEXT_ROOT}/include/tinygltf/" )
	_FIND_FILE ( GLTF_INC _gltf_srch "tiny_gltf.h" "" OK_H )		
	if ( OK_H EQUAL 1 )
      add_definitions(-DBUILD_GLTF) 
      include_directories( "${LIBEXT_ROOT}/include/tinygltf" )
      message ( STATUS "  ---> Using tinygltf" )
    else ()
      message ( FATAL_ERROR "
      TinyGLTF library not found at: ${_gltf_srch}
      Set LIBEXT_DIR to libmin/libext path for 3rd party libs.
      ")
    endif()
endfunction()


####################################################################################
# Include CUDA
#
function ( _REQUIRE_CUDA use_cuda_default kernel_path) 
    
    OPTION (BUILD_CUDA "Build with CUDA" ${use_cuda_default})

    if (BUILD_CUDA) 
        
        if (NOT CUDA_TOOLKIT_VERSION) 
	        set ( CUDA_TOOLKIT_VERSION "10.2" CACHE PATH "CUDA Toolkit version")
            message ( STATUS "*** No CUDA_TOOLKIT_VERSION specified. Assuming CUDA ${CUDA_TOOLKIT_VERSION}. ")
        endif()        
        message( STATUS "  Searching for CUDA..") 

        find_package(CUDAToolkit "${CUDA_TOOLKIT_VERSION}" REQUIRED EXACT QUIET)
        if ( CUDAToolkit_FOUND )
            ##########################################
            # Link CUDA
            #
	        message( STATUS "  ---> Using package CUDA (ver ${CUDAToolkit_VERSION})") 
            add_definitions(-DBUILD_CUDA)    
	        add_definitions(-DUSE_CUDA)    
	        include_directories(${CUDAToolkit_INCLUDE_DIRS})
	        LIST(APPEND LIBRARIES_OPTIMIZED ${CUDAToolkit_CUDA_LIBRARY} ${CUDAToolkit_CUDART_LIBRARY} )
	        LIST(APPEND LIBRARIES_DEBUG ${CUDAToolkit_CUDA_LIBRARY} ${CUDAToolkit_CUDART_LIBRARY} )
	        LIST(APPEND PACKAGE_SOURCE_FILES ${CUDAToolkit_INCLUDE_DIRS} )    
            set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
            set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
            set(PACKAGE_SOURCE_FILES ${PACKAGE_SOURCE_FILES} PARENT_SCOPE)
	        source_group(CUDA FILES ${CUDAToolkit_INCLUDE_DIRS} )

            ##########################################
            # Compile PTX Files
            #
            set( USE_DEBUG_PTX OFF CACHE BOOL "Enable CUDA debugging with NSight")  
            if ( USE_DEBUG_PTX )
	           set ( DEBUG_FLAGS ";-g;-G;-D_DEBUG;-DEBUG")
            else()
	           set ( DEBUG_FLAGS "")
            endif()
            if ( NOT DEFINED CUDA_ARCH )	        
	           set( CUDA_ARCH "compute_50" CACHE STRING "CUDA Architecture target" )
  	           set( CUDA_CODE "sm_50" CACHE STRING "CUDA Code target" )
               message( "WARNING: CUDA_ARCH not set by parent cmake. Using default. arch: ${CUDA_ARCH}, code: ${CUDA_CODE}")
               set( CUDA_ARCH ${CUDA_ARCH} PARENT_SCOPE)
               set( CUDA_CODE ${CUDA_CODE} PARENT_SCOPE)
            endif ()
            get_filename_component (_cuda_kernels "${CMAKE_CURRENT_SOURCE_DIR}/${kernel_path}" REALPATH)
            file(GLOB CUDA_FILES "${_cuda_kernels}/*.cu" "${_cuda_kernels}/*.cuh")              
            message ( STATUS "Searching for kernels.. ${_cuda_kernels}")
            message ( STATUS "Building CUDA kernels: ${CUDA_FILES}" )
            message ( STATUS "Requested CUDA arch: ${CUDA_ARCH}, code: ${CUDA_CODE}")             
            # parent cmake can specify: ADDITIONAL_CUDA_INCLUDES, CUDA_ARCH and CUDA_CODE
            if ( USE_DEBUG_PTX )
	            _COMPILEPTX ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${CMAKE_CURRENT_SOURCE_DIR},${LIBMIN_ROOT_INC},${ADDITIONAL_CUDA_INCLUDES}" OPTIONS --ptx -arch=${CUDA_ARCH} -code=${CUDA_CODE} -dc -Xptxas -v -G -g --use_fast_math --maxrregcount=32 )
            else()
	            _COMPILEPTX ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${CMAKE_CURRENT_SOURCE_DIR},${LIBMIN_ROOT_INC},${ADDITIONAL_CUDA_INCLUDES}" OPTIONS --ptx -arch=${CUDA_ARCH} -code=${CUDA_CODE} -dc -Xptxas -v -O3 --use_fast_math --maxrregcount=32 )
            endif()
            set (CUDA_FILES ${CUDA_FILES} PARENT_SCOPE)
            set (CUDA_PTX ${CUDA_PTX} PARENT_SCOPE)
            set (CUDA_PTX_PATHS ${CUDA_PTX_PATHS} PARENT_SCOPE)
        else()
	        message ( FATAL_ERROR "  ---> Unable to find package CUDA")
        endif()
    else()
        # check if cuda is at least installed
        find_package(CUDA)
        message ( "Warning: CUDA not found/enabled. Set BUILD_CUDA to enable GPU. May need to enable when building Libmin also.")
    endif()
    if (USE_NVTX)
        add_definitions(-DUSE_NVTX)
	endif()	
endfunction()



#------------------------------------ CROSS-PLATFORM PTX COMPILE 
#
# _COMPILEPTX( SOURCES file1.cu file2.cu TARGET_PATH <path where ptxs should be stored> GENERATED_FILES ptx_sources NVCC_OPTIONS -arch=sm_20)
# Generates ptx files for the given source files. ptx_sources will contain the list of generated files.
#
FUNCTION( _COMPILEPTX )
  set(options "")
  set(oneValueArgs TARGET_PATH GENERATED GENPATHS INCLUDE)  
  set(multiValueArgs OPTIONS SOURCES)
  CMAKE_PARSE_ARGUMENTS( _COMPILEPTX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  # Match the bitness of the ptx to the bitness of the application
  set( MACHINE "--machine=32" )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( MACHINE "--machine=64" )
  endif()
  unset ( PTX_FILES CACHE )
  unset ( PTX_FILES_PATH CACHE )  
  
  if ( WIN32 ) 

     # Windows - PTX compile
     #
	file ( MAKE_DIRECTORY "${_COMPILEPTX_TARGET_PATH}/Debug" )
	file ( MAKE_DIRECTORY "${_COMPILEPTX_TARGET_PATH}/Release" )
	string (REPLACE ";" " " _COMPILEPTX_OPTIONS "${_COMPILEPTX_OPTIONS}")  
	separate_arguments( _OPTS WINDOWS_COMMAND "${_COMPILEPTX_OPTIONS}" )
	message ( STATUS "NVCC Options: ${_COMPILEPTX_OPTIONS}" )  
	message ( STATUS "NVCC Include: ${_COMPILEPTX_INCLUDE}" )

        set ( INCL "-I\"${_COMPILEPTX_INCLUDE}\"" )

    if (NOT DEFINED CUDA_NVCC_EXECUTABLE ) 
        message ( "WARNING: CUDA_NVCC_EXECTUABLE is being set from CUDAToolkit." )
        set (CUDA_NVCC_EXECUTABLE "${CUDAToolkit_NVCC_EXECUTABLE}")
    endif()
    message( STATUS "NVCC Executable: ${CUDA_NVCC_EXECUTABLE}" )

	# Custom build rule to generate ptx files from cuda files
	FOREACH( input ${_COMPILEPTX_SOURCES} )
		get_filename_component( input_ext ${input} EXT )									# Input extension
		get_filename_component( input_without_ext ${input} NAME_WE )						# Input base
		if ( ${input_ext} STREQUAL ".cu" )			
				
		# Set output names
		set( output "${input_without_ext}.ptx" )							# Output name
		set( output_with_path "${_COMPILEPTX_TARGET_PATH}/$(Configuration)/${input_without_ext}.ptx" )	# Output with path
		set( output_with_quote "\"${output_with_path}\"" )
		LIST( APPEND PTX_FILES ${output} )		# Append to output list
		LIST( APPEND PTX_FILES_PATH ${output_with_path} )    

		message( STATUS "NVCC Compile: ${_COMPILEPTX_OPTIONS} ${input} ${INCL} -o ${output_with_path} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}")
  
		add_custom_command(
			OUTPUT  ${output_with_path}
			MAIN_DEPENDENCY ${input}
			COMMAND ${CUDA_NVCC_EXECUTABLE} --ptx ${_OPTS} ${input} ${INCL} -o ${output_with_quote} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			)			
		endif()
	ENDFOREACH( )

  else ()
    
    # Linux - PTX compile
    #    
    if (CMAKE_CUDA_COMPILER)

      message ( STATUS "NVCC INCLUDE: ${_COMPILEPTX_INCLUDE}" )
      add_library ( PTX_FILES OBJECT ${_COMPILEPTX_SOURCES})
      set_property (TARGET PTX_FILES PROPERTY CUDA_PTX_COMPILATION ON )
      string (REPLACE "," ";" _COMPILEPTX_INCLUDE "${_COMPILEPTX_INCLUDE}")  
      FOREACH(input ${_COMPILEPTX_SOURCES} )
        get_filename_component( input_ext ${input} EXT )
        get_filename_component( input_without_ext ${input} NAME_WE )
        set( output "${input_without_ext}.ptx" )
        set( output_with_path "${_COMPILEPTX_TARGET_PATH}/CMakeFiles/PTX_FILES.dir/source/${input_without_ext}.ptx" )
        LIST(APPEND PTX_FILES ${output})
        LIST(APPEND PTX_FILES_PATH ${output_with_path})
      ENDFOREACH()
      FOREACH(input ${_COMPILEPTX_INCLUDE})
        message ( STATUS "  PTX INCLUDE: ${input}")
        target_include_directories ( PTX_FILES PRIVATE ${input} )        
      ENDFOREACH()

    else()      

      file ( MAKE_DIRECTORY "${_COMPILEPTX_TARGET_PATH}" )
      FOREACH(input ${_COMPILEPTX_SOURCES})
        get_filename_component( input_ext ${input} EXT )									# Input extension
        get_filename_component( input_without_ext ${input} NAME_WE )						# Input base
        if ( ${input_ext} STREQUAL ".cu" )			
          # Set output names
          set( output "${input_without_ext}.ptx" ) # Output name
          set( output_with_path "${_COMPILEPTX_TARGET_PATH}/${input_without_ext}.ptx" )	# Output with path

          set( compile_target_ptx "${input_without_ext}_PTX")
          set( custom_command_var "${input_without_ext}_OUTPUT")
          # compile ptx
          cuda_compile_ptx(custom_command_var ${input} OPTIONS "${DEBUG_FLAGS}")
          # This will only configure file generation, we need to add a target to
          # generate a file cuda_generated_<counter>_${input_without_ext}.ptx
          # Add custom command to rename to simply ${input_without_ext}.ptx
          add_custom_command(OUTPUT ${output_with_path}
                          COMMAND ${CMAKE_COMMAND} -E rename ${custom_command_var} ${output_with_path}
                          DEPENDS ${custom_command_var})
          add_custom_target(${compile_target_ptx} ALL DEPENDS ${input} ${output_with_path} SOURCES ${input})

          # Add this output file to list of generated ptx files  
          LIST(APPEND PTX_FILES ${output})
          LIST(APPEND PTX_FILES_PATH ${output_with_path} )
        endif()
      ENDFOREACH()
    endif()

  endif()

  set( ${_COMPILEPTX_GENERATED} ${PTX_FILES} PARENT_SCOPE)
  set( ${_COMPILEPTX_GENPATHS} ${PTX_FILES_PATH} PARENT_SCOPE)

ENDFUNCTION()


function(_EXPANDLIST)
  set (options "")
  set (oneValueArgs SOURCE OUTPUT)
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS(_EXPANDLIST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  set (OUT_LIST "")
  if (_EXPANDLIST_SOURCE)      
     set ( _EXPANDLIST_SOURCE "${_EXPANDLIST_SOURCE}/" )	  
  endif()

  #---- append to existing list
  foreach (_file ${${_EXPANDLIST_OUTPUT}} )	
     list ( APPEND OUT_LIST ${_file})
  endforeach()
  #---- new items added
  foreach (_file ${_EXPANDLIST_FILES} )	
     set ( _fullpath ${_EXPANDLIST_SOURCE}${_file} )
     list ( APPEND OUT_LIST "${_fullpath}" )
  endforeach()    
  
  set ( ${_EXPANDLIST_OUTPUT} ${OUT_LIST} PARENT_SCOPE )

endfunction()

#------------------------------------ CROSS-PLATFORM, MULTI-FILE INSTALLS

# _INSTALL_PRE - install multiple files prior to build, at cmake time
#
function( _INSTALL_PRE )   
  set (options "")
  set (oneValueArgs DESTINATION SOURCE OUTPUT )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS(_INSTALL_PRE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if (_INSTALL_PRE_SOURCE)      
     set ( _INSTALL_PRE_SOURCE "${_INSTALL_PRE_SOURCE}/" )	  
  endif()
  set ( OUT_LIST ${${_INSTALL_PRE_OUTPUT}} )

  file ( MAKE_DIRECTORY "${_INSTALL_PRE_DESTINATION}/" )
       
  # collect files to pre-install
  foreach (_file ${_INSTALL_PRE_FILES} )	
    get_filename_component ( _path "${_file}" DIRECTORY )               
    if ( "${_path}" STREQUAL "" )		   
        set ( _fullpath "${_INSTALL_PRE_SOURCE}${_file}")            
    else ()
        set ( _fullpath "${_file}" )            
    endif()        
    list ( APPEND OUT_LIST "${_fullpath}" )
    # copy the file now (cmake time)
    file (COPY "${_fullpath}" DESTINATION "${_INSTALL_PRE_DESTINATION}/" )
  endforeach()        

endfunction()


function( _INSTALL )   
  set (options "")
  set (oneValueArgs DESTINATION SOURCE OUTPUT )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS(_INSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if (_INSTALL_SOURCE)      
     set ( _INSTALL_SOURCE "${_INSTALL_SOURCE}/" )	  
  endif()
  set ( OUT_LIST ${${_INSTALL_OUTPUT}} )

  file ( MAKE_DIRECTORY "${_INSTALL_DESTINATION}/" )

  #-- POST-BUILD FILE COPY
  #
  foreach (_file ${_INSTALL_FILES} )	
    get_filename_component ( _path "${_file}" DIRECTORY )               
    get_filename_component ( _name "${_file}" NAME )
    if ( "${_path}" STREQUAL "" )		   
      set ( _fullpath "${_INSTALL_SOURCE}${_file}")            
    else ()
      set ( _fullpath "${_file}" )            
    endif()
    message ( STATUS "Install: ${_fullpath} -> ${_INSTALL_DESTINATION}" )
    add_custom_command(
       TARGET ${PROJNAME} POST_BUILD
       DEPENDS ${_fullpath}
       COMMAND ${CMAKE_COMMAND} -E copy ${_fullpath} ${_INSTALL_DESTINATION}
    ) 
    #-- not working
    # add_custom_target(COPY_POSTBUILD${_name} ALL DEPENDS ${_fullpath} )
    # add_dependencies(${PROJNAME} COPY_${_name} )
    list ( APPEND OUT_LIST "${_fullpath}" )
  endforeach()    
  
  set ( ${_INSTALL_OUTPUT} ${OUT_LIST} PARENT_SCOPE )
   
endfunction()

#------------------------------------------------- CROSS-PLATFORM INSTALL PTX
#
function( _INSTALL_PTX )   
  set (options "")
  set (oneValueArgs DESTINATION OUTPUT )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS(_INSTALL_PTX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set ( OUT_LIST ${${_INSTALL_PTX_OUTPUT}} )

  unset ( PTX_FIXED )

  if ( WIN32 )   

    foreach ( _file IN ITEMS ${_INSTALL_PTX_FILES} )
	get_filename_component ( _ptxbase ${_file} NAME_WE )
 	get_filename_component ( _ptxpath ${_file} DIRECTORY )
 	get_filename_component ( _ptxparent ${_ptxpath} DIRECTORY )   # parent directory
	set ( _fixed "${_ptxparent}/${_ptxbase}.ptx" )                # copy to parent to remove compile time $(Configuration) path
  	add_custom_command ( TARGET ${PROJNAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy  ${_file} ${_fixed}
        )
	list ( APPEND PTX_FIXED ${_file} )     # NOTE: Input of FILES must be list of ptx *with paths*	
	list ( APPEND OUT_LIST ${_fixed} )
    endforeach()

  else()

    foreach ( _file IN ITEMS ${_INSTALL_PTX_FILES} )
      get_filename_component ( _ptxpath ${_file} DIRECTORY )
      get_filename_component ( _ptxbase ${_file} NAME_WE )
      string ( SUBSTRING ${_ptxbase} 27 -1 _ptxname )
      set ( _fixed "${_ptxpath}/${_ptxname}.ptx" )
      add_custom_command ( TARGET ${PROJNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy  ${_file} ${_fixed}
        )      
      list ( APPEND PTX_FIXED ${_fixed} )
      list ( APPEND OUT_LIST ${_fixed} )
    endforeach()
  endif()
  
  # Install PTX
  message ( STATUS "PTX files: ${PTX_FIXED}" )
  _INSTALL ( FILES ${PTX_FIXED} DESTINATION ${_INSTALL_PTX_DESTINATION} )

  set ( ${_INSTALL_PTX_OUTPUT} ${OUT_LIST} PARENT_SCOPE )

endfunction()

#----------- COPY_AT_BUILD
# Copies files at post-build time rather than cmake time.
# Useful for assets, shaders and other dynamic content files.
#
function( _COPY_AT_BUILD )
  set (options "")
  set (oneValueArgs FOLDER DESTINATION )
  CMAKE_PARSE_ARGUMENTS(_COPY_AT_BUILD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  file(GLOB fileList "${_COPY_AT_BUILD_FOLDER}/*.*")

  file(MAKE_DIRECTORY ${_COPY_AT_BUILD_DESTINATION} )

  foreach(file ${fileList})
  
    get_filename_component ( fileext ${file} EXT )
    get_filename_component ( filebase ${file} NAME )
    set( newFile "${_COPY_AT_BUILD_DESTINATION}/${filebase}")
    message ( "ASSET: ${file} -> ${newFile}" )

    add_custom_command (
        OUTPUT ${newFile}
        MAIN_DEPENDENCY ${file}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file} ${newFile}
     )    
     if ( ${fileext} STREQUAL ".obj" )
        set_source_files_properties ( ${file} PROPERTIES HEADER_FILE_ONLY TRUE)
     endif()
  endforeach()

  # add_custom_target ( export_files ALL DEPENDS ${fileList} )

endfunction()


#----------------------------------------------- CROSS-PLATFORM FIND FILES
# Find one or more of a specific file in the given folder
# Returns the file name w/o path
macro(_FIND_FILE targetVar searchDir nameWin64 nameLnx cnt)
  unset ( fileList )  
  unset ( nameFind )
  if ( WIN32 ) 
     SET ( nameFind ${nameWin64} )
  else()     
     SET ( nameFind ${nameLnx} )
  endif()  
  if ( "${nameFind}" STREQUAL ""  )
    MATH (EXPR ${cnt} "${${cnt}}+1" )	
  else()
    set ( resolvedDir "${${searchDir}}" )
    file(GLOB fileList "${resolvedDir}/${nameFind}")  
    list(LENGTH fileList NUMLIST)  
    if (NUMLIST GREATER 0)	
       MATH (EXPR ${cnt} "${${cnt}}+1" )	
       set(${targetVar} "${${targetVar}};${fileList}" )
    endif() 
  endif()
endmacro()

#----------------------------------------------- CROSS-PLATFORM FIND MULTIPLE
# Find all files in specified folder with the given extension.
# This creates a file list, where each entry is only the filename w/o path
# Return the count of files
macro(_FIND_MULTIPLE targetVar searchDir extWin64 extLnx )    
  unset ( fileList )    
  unset ( targetVar ) 
    if ( WIN32 )   
     SET ( extFind ${extWin64} )     
  else()
     SET ( extFind ${extLnx} )
  endif()  
  file( GLOB fileList "${searchDir}/*.${extFind}")  
  list( APPEND ${targetVar} ${fileList} )  
endmacro()

#------------------------------------------------ FIND GLOBAL
macro(_FIND_GLOBAL outvar targetPath startDir targetFile )
  set ( _target "${targetPath}${targetFile}" FORCE )
  set ( up1 "${startDir}/.." )
  set ( up2 "${startDir}/../.." )
  set ( up3 "${startDir}/../.." )    
  set ( up4 "${startDir}/../../.." )    
  unset ( result CACHE )
  find_file ( result ${_target} PATHS ${startDir} ${up1} ${up2} ${up3} ${up4} )
  string (REPLACE ${targetFile} "" result ${result} )
  if ( ${result} STREQUAL "result-NOTFOUND" )
    set (${outvar} "NOTFOUND" CACHE PATH "" )
  else()
    set (${outvar} ${result} CACHE PATH "" )
  endif()   
  unset ( result CACHE )
endmacro()

#----------------------------------------------- LIST ALL source
function(_LIST_ALL_SOURCE )   
  set (options "")
  set (oneValueArgs "" )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS(_LIST_ALL_SOURCE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  unset ( SOURCE_LIST )
  foreach ( _file IN ITEMS ${_LIST_ALL_SOURCE_FILES} )
     message ( STATUS "Source: ${_file}")			# uncomment to check source files
     list ( APPEND SOURCE_LIST ${_file} )
  endforeach()

  set ( ALL_SOURCE_FILES ${SOURCE_LIST} PARENT_SCOPE )
endfunction()

function(_LINK ) 
    set (options "")
    set (multiValueArgs PROJECT OPT DEBUG PLATFORM )
    CMAKE_PARSE_ARGUMENTS(_LINK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug PARENT_SCOPE ) 

	set (PROJ_NAME ${_LINK_PROJECT})

	foreach (_item IN ITEMS ${_LINK_PLATFORM} )
		target_link_libraries( ${PROJ_NAME} general -Wl,--no-as-needed ${_item} )	
		list (APPEND LIBLIST ${_item})
	endforeach() 	

	foreach (_item IN ITEMS ${_LINK_DEBUG} )
		target_link_libraries ( ${PROJ_NAME} debug -Wl,--no-as-needed ${_item} )
		list (APPEND LIBLIST ${_item})
	endforeach()
	
	foreach (_item IN ITEMS ${_LINK_OPT} )   
		target_link_libraries ( ${PROJ_NAME} optimized -Wl,--no-as-needed ${_item} )
	endforeach()
	
	string (REPLACE ";" "\n   " OUTSTR "${LIBLIST}")
	message ( STATUS "Libraries used:\n   ${OUTSTR}" )
endfunction()

macro(_MSVC_PROPERTIES)
    if (WIN32) 
	    # Instruct CMake to automatically build INSTALL project in Visual Studio 
	    set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)

	    set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
        set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR} )
        set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR} )

	    # Set startup PROJECT	
	    if ( (${CMAKE_MAJOR_VERSION} EQUAL 3 AND ${CMAKE_MINOR_VERSION} GREATER 5) OR (${CMAKE_MAJOR_VERSION} GREATER 3) )
		    message ( STATUS "VS Startup Project: ${CMAKE_CURRENT_BINARY_DIR}, ${PROJNAME}")
		    set_property ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJNAME} )
	    endif()		

    endif()
endmacro ()

macro(_DEFAULT_INSTALL_PATH)
	if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )	   
	   set ( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Install path" FORCE)   
	endif()	
endmacro()
