
####################################################################################
# FIND LIBMIN 
#
# *** THIS CMAKE PROVIDES LIBMIN USING STATIC LINKAGE **
#
# Provide the libmin repository path, install path, and libext path
# for access to mains, third-party libs and cmake helpers
#  LIBMIN_ROOT_PATH    = path to libmin source repository (eg. libmin/src)
#  LIBEXT_ROOT         = path to third-party libs (eg. libjpg, libgvdb, liboptix)
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
    if ( ${found} STREQUAL "NOTFOUND" )
      message ( FATAL_ERROR "\n
Cannot find ${varName} at ${targetPath}.
Please set ${varName} to the correct path, which should contain
files such as ${targetFile}.\n"
      )
    endif()
  endif()
  unset ( result CACHE )
endfunction()

message ( STATUS "\n----- RUNNING FindLibmin.cmake " )

if (DEFINED ENV{LINUX_DEBUG})
  message (STATUS "LINUX DEBUGGING enabled")
  set(CMAKE_BUILD_TYPE Debug)           # sets -g and disables optimizations
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")  # ensure no optimization
  set(CMAKE_CUDA_FLAGS_DEBUG "-G")     # CUDA: generate debug info for kernels
endif()

# *NOTE** 
# LIBMIN_ prefix forces the variable to be namespaced to Libmin package,
# and allows it to be initialized & updated here but visible to project
set (LIBS_DEBUG "" )
set (LIBS_OPTIMIZED "" )	
set (LIBS_PLATFORM "" )	
set (LIBS_PACKAGE_DLLS "" )	

##############
# LIBMIN_ROOT - libmin src, should be set by caller during bootstrap
#
message ( STATUS "  LIBMIN_ROOT: ${LIBMIN_ROOT}")
get_filename_component ( LIBMIN_ROOT "${LIBMIN_ROOT}" REALPATH)
set ( LIBMIN_ROOT "${LIBMIN_ROOT}" CACHE PATH "Path to /libmin source" )
_CONFIRM_PATH ( LIBMIN_ROOT "${LIBMIN_ROOT}" "/src/common_defs.cpp" "/src/common_defs.cpp" "LIBMIN_ROOT" TRUE)

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )	   
  set ( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Install path" FORCE)   
endif()	

##############
# LIBEXT_ROOT - libext src, by default use the minimal /libext that comes with libmin
#
if ( NOT DEFINED LIBEXT_ROOT )
  # check for /libext with extended third party libs
  _CONFIRM_PATH ( LIBEXT_EXTENDED "${LIBMIN_ROOT}/../libext/" "cmake/FindFFTW.cmake" "cmake/FindFFTW.cmake" "LIBEXT_EXTENDED" FALSE)
  if ( LIBEXT_EXTENDED STREQUAL "NOTFOUND" ) 
    get_filename_component ( LIBEXT_ROOT "${LIBMIN_ROOT}/libext" REALPATH)    
  else()
    get_filename_component ( LIBEXT_ROOT "${LIBMIN_ROOT}/../libext" REALPATH)       
  endif()
else() 
  # use cached or user-set path for /libext
  get_filename_component ( LIBEXT_ROOT "${LIBEXT_ROOT}" REALPATH)
endif()
_CONFIRM_PATH ( LIBEXT_CHK "${LIBEXT_ROOT}" "cmake/FindFFTW.cmake" "cmake/FindFFTW.cmake" "LIBEXT_EXTENDED" FALSE)
if ( LIBEXT_CHK STREQUAL "NOTFOUND" ) 
  message ( "  LIBEXT using internal to LIBMIN at ${LIBEXT_ROOT}")    
else()
  message ( "  LIBEXT using *extended* third-parties at ${LIBEXT_ROOT}.")    
endif()  
set ( LIBEXT_ROOT ${LIBEXT_ROOT} CACHE PATH "Path to /libext source" )
_CONFIRM_PATH ( LIBEXT_ROOT "${LIBEXT_ROOT}" "/include/openssl/bio.h" "/include/openssl/bio.h" "LIBEXT_ROOT" TRUE)

# Repository paths
set ( LIBMIN_ROOT_MAINS "${LIBMIN_ROOT}/mains" )
set ( LIBMIN_ROOT_SRC "${LIBMIN_ROOT}/src")
set ( LIBMIN_ROOT_INC "${LIBMIN_ROOT}/include")
set ( LIBEXT_ROOT ${LIBEXT_ROOT} )

# Include libmin & libext 
add_library (libmin::libmin INTERFACE IMPORTED )
target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include")

add_library (libext::libext INTERFACE IMPORTED)
target_include_directories(libext::libext INTERFACE "${LIBEXT_ROOT}/include")

# Static libmin source (not a separate target)
file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/*.cpp" "${LIBMIN_ROOT}/src/*.c" "${LIBMIN_ROOT}/src/GL/*.c" "${LIBMIN_ROOT}/include/*.h")
set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )

# Symbols in release mode
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF" CACHE STRING "" FORCE)
set ( CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" )
# set_target_properties( ${PROJNAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )

# Libmin source code
add_definitions(-DLIBMIN_STATIC)    # Libmin Static
add_definitions(-DBUILD_LIBMIN)  

# Asset Path
if ( NOT DEFINED ASSET_PATH ) 
   get_filename_component ( _assets "${CMAKE_CURRENT_SOURCE_DIR}/assets" REALPATH )
   set ( ASSET_PATH ${_assets} CACHE PATH "Full path to /assets" )
endif()
file(GLOB GLSL_FILES ${ASSET_PATH}/*.glsl )
add_definitions(-DASSET_PATH="${ASSET_PATH}/")

# Debug heap
#set ( DEBUG_HEAP false CACHE BOOL "Enable heap checking (debug or release).")
set ( DEBUG_HEAP true CACHE BOOL "Enable heap checking (debug or release).")
if ( ${DEBUG_HEAP} )
   add_definitions( -DDEBUG_HEAP)
   add_definitions( -D_CRTDBG_MAP_ALLOC)
endif()

message ( STATUS "  SOURCE Directory: ${CMAKE_CURRENT_SOURCE_DIR}" )
message ( STATUS "  LIBMIN Root: ${LIBMIN_ROOT}" )
message ( STATUS "  LIBEXT Root: ${LIBEXT_ROOT}" )
message ( STATUS "  ASSET PATH:  ${ASSET_PATH}" )
message ( STATUS "----- READY Libmin\n" )

# BOOTSTRAP COMPLETE
###############################

####################################################################################
# COPY CUDA BINS
# This macro copies all binaries for the CUDA library to the target executable location. 
#
macro(_copy_cuda_bins projname )	
	add_custom_command(
		TARGET ${projname} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CUDA_DLL} $<TARGET_FILE_DIR:${PROJNAME}>   
	)
endmacro()

####################################################################################
# GET GLOBALS
# Macros run at global scope
macro(_GET_GLOBALS)
  get_property ( LIBMIN_FILES GLOBAL PROPERTY LIBMIN_FILES)  
endmacro()

####################################################################################
# Provide Libmin modules [optional]
#  /dataptr, /g2lib, /gxlib, /image, /network
#
macro(_REQUIRE_3D)
  file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/3d/*.cpp" "${LIBMIN_ROOT}/include/3d/*.h")
  set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )
  target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include/3d")
  add_definitions(-DBUILD_QUATERNION)   
endmacro()
macro(_REQUIRE_DATAPTR)
  file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/dataptr/*.cpp" "${LIBMIN_ROOT}/include/dataptr/*.h")
  set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )
  target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include/dataptr")
  add_definitions(-DBUILD_DATAPTR)   
endmacro()
macro(_REQUIRE_G2LIB)
  file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/g2lib/*.cpp" "${LIBMIN_ROOT}/include/g2lib/*.h")
  set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )
  target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include/g2lib")
  add_definitions(-DBUILD_G2LIB)   
endmacro()
macro(_REQUIRE_GXLIB)
  file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/gxlib/*.cpp" "${LIBMIN_ROOT}/include/gxlib/*.h")
  set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )
  target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include/gxlib")
  add_definitions(-DBUILD_GXLIB)   
endmacro()
macro(_REQUIRE_IMAGE)
  file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/image/*.cpp" "${LIBMIN_ROOT}/include/image/*.h")
  set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )
  target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include/image")
  add_definitions(-DBUILD_IMAGE)
  add_definitions(-DBUILD_PNG)        # Always build PNG
  add_definitions(-DBUILD_TIF)	      # Always build TIF
endmacro()
macro(_REQUIRE_NETWORK)
  file(GLOB LIBMIN_SRC "${LIBMIN_ROOT}/src/network/*.cpp" "${LIBMIN_ROOT}/include/network/*.h")
  set_property ( GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIBMIN_SRC} )
  target_include_directories(libmin::libmin INTERFACE "${LIBMIN_ROOT}/include/network")
  add_definitions(-DBUILD_NETWORK)
endmacro()

####################################################################################
# Provide Libext - extended 3rd party libraries: 
#  OpenSSL, Laszip, OptiX, PortAudio, CUFFT, PixarUSD, LibGDVB, LibOptiX
#
macro (_REQUIRE_LIBEXT)    
    message (STATUS "  Searching for LIBEXT... ${LIBEXT_ROOT}")
    if (EXISTS "${LIBEXT_ROOT}" AND IS_DIRECTORY "${LIBEXT_ROOT}")
        message (STATUS "  ---> Using LIBEXT: ${LIBEXT_ROOT}")
        list( APPEND CMAKE_MODULE_PATH "${LIBEXT_ROOT}/cmake" )
        list( APPEND CMAKE_PREFIX_PATH "${LIBEXT_ROOT}/cmake" )        
        set( LIBEXT_FOUND TRUE )
    else()
message (FATAL_ERROR "  LIBEXT Not Found at: ${LIBEXT_ROOT}. 
Set LIBEXT_ROOT to the location of libext.
")
    endif()
endmacro()

###################################################################################
# Provide a cross-platform main
#
macro ( _REQUIRE_MAIN )
    # Add Main to build

    OPTION (BUILD_CONSOLE "Build console app" OFF)

    add_definitions(-DBUILD_MAIN)

    IF (BUILD_CONSOLE)
    	set (APP_FILES "${LIBMIN_ROOT_MAINS}/main_console.cpp" )
    ELSE()
	    IF(WIN32)
  		    set( APP_FILES "${LIBMIN_ROOT_MAINS}/main_win.cpp" )
	    ELSEIF(ANDROID)
		      set( APP_FILES "${LIBMIN_ROOT_MAINS}/main_android.cpp" )
	    ELSE()
	  	    set( APP_FILES "${LIBMIN_ROOT_MAINS}/main_x11.cpp" )
	    ENDIF()
    ENDIF()
	  LIST( APPEND APP_FILES "${LIBMIN_ROOT_MAINS}/main.h" )
    include_directories( ${LIBMIN_ROOT_MAINS} )

    list( APPEND PACKAGE_SOURCE_FILES ${APP_FILES} )
endmacro()

macro ( _REQUIRE_CONSOLE )
  add_definitions(-DBUILD_MAIN)
  set( APP_FILES "${LIBMIN_ROOT_MAINS}/main_console.cpp" )
  include_directories( ${LIBMIN_ROOT_MAINS} )
  list( APPEND PACKAGE_SOURCE_FILES ${APP_FILES} )
endmacro()

###################################################################################
# Include OpenGL 
#
macro ( _REQUIRE_GL )
    OPTION (BUILD_OPENGL "Build with OpenGL" ON)
    if (BUILD_OPENGL)            
        message ( STATUS "  Searching for GL.." )
        find_package(OpenGL)
        if (OPENGL_FOUND)                    
          add_definitions(-DBUILD_OPENGL)  		
          _ATTACH_PLATFORM_LIB ( NAME "GL" WIN "opengl32.lib" LINUX "GL -lGLEW -lX11") 
        endif()
    endif()
endmacro()

macro ( _REQUIRE_GLEW)    
    OPTION (BUILD_GLEW "Build with GLEW" ON)
    if (BUILD_GLEW)
      add_definitions(-DGLEW_STATIC)	    
      LIST(APPEND GLEW_FILES "${LIBEXT_ROOT}/include/GL/glew.c" )       
      _ATTACH_FILES ( "GLEW" ${GLEW_FILES} )      
    endif()
endmacro()

###################################################################################
# Include OpenSSL
#
macro ( _REQUIRE_OPENSSL _openssl_default)
  OPTION (BUILD_OPENSSL "Build with OpenSSL" ${_openssl_default} )
  if (BUILD_OPENSSL) 
    find_package(OpenSSL)

    if ( OPENSSL_FOUND )	
      add_definitions(-DBUILD_OPENSSL)	
      include_directories ( ${OPENSSL_INC} )
      link_directories ( ${OPENSSL_BIN} )
      _ATTACH_LIB ( NAME "OpenSSL" INC ${OPENSSL_INC} BIN ${OPENSSL_BIN} DEBUG_LIBS ${OPENSSL_DEBUG_LIBS} REL_LIBS ${OPENSSL_REL_LIBS} DLLS ${OPENSSL_DLLS} )
    else()
      message ( FATAL_ERROR "\n  Unable to find OpenSLL library. Link with a different /libext for third-party libs that include OpenSSL.\n")
    endif()
  endif()
endmacro()

#####################################################################################
# Include Bcrypt
#
macro ( _REQUIRE_BCRYPT _bcrypt_default )
  OPTION (BUILD_BCRYPT "Build with Bcrypt" ${_bcrypt_default} )
  if (BUILD_BCRYPT)
    find_package(Bcrypt)
    if ( BCRYPT_FOUND )	      
      add_definitions(-DBUILD_BCRYPT)
      add_definitions(-DBCRYPT_STATIC)
      _ATTACH_LIB ( NAME "BCrypt" INC ${BCRYPT_INC} BIN ${BCRYPT_BIN} DEBUG_LIBS ${BCRYPT_DEBUG_LIBS} REL_LIBS ${BCRYPT_REL_LIBS} DLLS "" )
    endif()
  endif()
endmacro()

#####################################################################################
# Include GVDB
#
macro ( _REQUIRE_GVDB )
 
  find_package( LibGVDB )

  if ( LIBGVDB_FOUND )	
	  add_definitions(-DBUILD_GVDB)
    _ATTACH_LIB ( NAME "GVDB" INC ${GVDB_INC_DIR} BIN ${GVDB_LIB_DIR} DEBUG_LIBS ${GVDB_DEBUG_LIB} REL_LIBS ${GVDB_REL_LIB} DLLS ${GVDB_DLLS} )
    set_property (GLOBAL APPEND PROPERTY CUDA_INC_PATHS ${GVDB_INC_DIR} )

    OPTION (BUILD_USING_GVDB_SHARED "Use GVDB Shared Libs" OFF)
    if (NOT BUILD_USING_GVDB_SHARED)
      add_definitions(-DGVDB_STATIC)  
    endif()
  endif()
endmacro()

#####################################################################################
# Include JPG    
#   
macro ( _REQUIRE_JPG )   
  set ( OK_H "0" )    
  set ( _jpg_srch "${LIBEXT_ROOT}/win64" )
	_FIND_FILE ( JPG_LIBS _jpg_srch "libjpegt.lib" "" OK_H )		
	if ( OK_H EQUAL 1 )
      
      add_definitions(-DBUILD_JPG)
      set ( JPG_REL "${LIBEXT_ROOT}/win64/libjpegt.lib" )
      set ( JPG_DEBUG "${LIBEXT_ROOT}/win64/libjpegt_d.lib" )

      _ATTACH_LIB ( NAME "JPG" INC "${LIBEXT_ROOT}/include/libjpegt" BIN "${LIBEXT_ROOT}/win64" DEBUG_LIBS ${JPG_DEBUG} REL_LIBS ${JPG_REL} DLLS "" )
      
      message ( STATUS "  ---> Using libjpegt (turbo)" )
    else ()
      message ( FATAL_ERROR "
      JPG libraries not found. 
      Set LIBEXT_DIR to libmin/libext path for 3rd party libs.
      ")
    endif()
endmacro()


####################################################################################
# Include CUDA
#
macro ( _REQUIRE_CUDA BUILD_CUDA_default kernel_path) 
    
    OPTION (BUILD_CUDA "Build with CUDA" ${BUILD_CUDA_default})

    if (BUILD_CUDA) 

      # specify cuda arch
      #   60=Pascal, 70=Volta, 75=Turing, 80=Ampere, 86=RTX 3x, 89=RTX 4x, 120=RTX 5x
      set(CMAKE_CUDA_ARCHITECTURES $ENV{CUDA_ARCH})   

      # find CUDA
      # *NOTE*: On CMake 3.18+, enable_language(CUDA) replaces find_package(CUDA),
      #         or to provide ${CUDA_INCLUDE_DIRS} as target_include_directories
      # find_package(CUDA REQUIRED)
      
      message(STATUS "  CUDA Toolkit search path: $ENV{CUDA_PATH} ")

      # enable CUDA language (with cmake >3.18)
      enable_language(CUDA)        
      set(CMAKE_CUDA_STANDARD 17)
      set(CMAKE_CUDA_STANDARD_REQUIRED ON)     

      # confirm version
      if(NOT CMAKE_CUDA_VERSION)
        execute_process(
            COMMAND ${CMAKE_CUDA_COMPILER} --version
            OUTPUT_VARIABLE NVCC_OUTPUT
            ERROR_VARIABLE NVCC_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(STATUS "${NVCC_OUTPUT}")
        # Parse the major/minor version
        string(REGEX MATCH "release ([0-9]+)\\.([0-9]+)" _match "${NVCC_OUTPUT}")
        set(CUDA_VERSION_MAJOR "${CMAKE_MATCH_1}")
        set(CUDA_VERSION_MINOR "${CMAKE_MATCH_2}")
        set(CMAKE_CUDA_VERSION "${CUDA_VERSION_MAJOR}.${CUDA_VERSION_MINOR}")
      endif()
      
      message(STATUS "  CUDA Support enabled: ver ${CMAKE_CUDA_VERSION} at ${CMAKE_CUDA_COMPILER}")       
      
      # attach cuda common helpers
      add_definitions(-DBUILD_CUDA)  
      LIST(APPEND CUDA_COMMON "${LIBMIN_ROOT}/src/cuda/common_cuda.cpp" "${LIBMIN_ROOT}/include/cuda/common_cuda.h")
      _ATTACH_FILES ( "CUDA" ${CUDA_COMMON} )   
      
      # attach all .cu and .cuh in the kernel_path folder given
      get_filename_component (_cuda_kernels_path "${CMAKE_CURRENT_SOURCE_DIR}/${kernel_path}" REALPATH)
      file(GLOB CUDA_FILES "${_cuda_kernels_path}/*.cu" )        # project scope          
      # "${_cuda_kernels_path}/*.cuh"

      # set cuda/nvcc include paths, to add app and libmin kernel headers
      set ( LIBMIN_KERNEL_PATH "${LIBMIN_ROOT}/include/cuda" )
      set_property (GLOBAL APPEND PROPERTY CUDA_INC_PATHS ${CMAKE_CURRENT_SOURCE_DIR} )
      set_property (GLOBAL APPEND PROPERTY CUDA_INC_PATHS ${LIBMIN_ROOT_INC} )
      set_property (GLOBAL APPEND PROPERTY CUDA_INC_PATHS ${LIBMIN_KERNEL_PATH} )
      target_include_directories ( libmin::libmin INTERFACE ${LIBMIN_KERNEL_PATH} )   


      # cuda settings
      set( BUILD_DEBUG_PTX OFF CACHE BOOL "Enable CUDA debugging with NSight")  
      if ( BUILD_DEBUG_PTX )
	      set ( DEBUG_FLAGS ";-g;-G;-D_DEBUG;-DEBUG")
      else()
	      set ( DEBUG_FLAGS "")
      endif()
      if ( BUILD_NVTX)
          add_definitions(-DBUILD_NVTX)
	    endif()

      # compile kernels
      if ( INTERNAL_CUDA )
        
        foreach(f ${CUDA_FILES})
          get_filename_component (ext ${f} EXT)        
          if (ext STREQUAL ".cu")          
            set_source_files_properties( ${f} PROPERTIES CUDA_PTX_COMPILATION ON)
          endif()
        endforeach()

      else()
        # get all include paths for nvcc, as comma-separated (we are compiling ptx directly here)
        get_property (NVCC_INC GLOBAL PROPERTY CUDA_INC_PATHS )
        string (REPLACE ";" "," NVCC_INC "${NVCC_INC}")
        message ( STATUS "  NVCC Include Paths: ${NVCC_INC}")

        if ( BUILD_DEBUG_PTX )
	        compile_ptx ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${NVCC_INC}" OPTIONS --ptx -dc -Xptxas -v -G -g --use_fast_math --maxrregcount=32 )
        else()
	        compile_ptx ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${NVCC_INC}" OPTIONS --ptx -dc -Xptxas -v -O3 --use_fast_math --maxrregcount=32 )
        endif()
      endif()
      message ( STATUS "  CUDA Kernels: ${CUDA_FILES} " )
      message ( STATUS "  ---> Using CUDA")

    

    else ()
      message(STATUS "  CUDA support disabled")
    endif()
    
endmacro()



#------------------------------------ CROSS-PLATFORM PTX COMPILE 
#
# _COMPILEPTX( SOURCES file1.cu file2.cu TARGET_PATH <path where ptxs should be stored> GENERATED_FILES ptx_sources NVCC_OPTIONS -arch=sm_20)
# Generates ptx files for the given source files. ptx_sources will contain the list of generated files.
#
macro ( compile_ptx )
  set(options "")
  set(oneValueArgs TARGET_PATH GENERATED GENPATHS INCLUDE)  
  set(multiValueArgs OPTIONS SOURCES)
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  # Match the bitness of the ptx to the bitness of the application
  set( MACHINE "--machine=32" )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( MACHINE "--machine=64" )
  endif()
  unset ( PTX_FILES CACHE )
  unset ( PTX_FILES_PATH CACHE )  

  # nvcc arch target (only 1 allowed when compiling to ptx)
  foreach(arch ${CMAKE_CUDA_ARCHITECTURES} )
    list(APPEND _ARCHS "--generate-code=arch=compute_${arch},code=[sm_${arch},compute_${arch}]")
  endforeach()

  # convey setup
  message ( STATUS "  NVCC Options: ${ARG_OPTIONS}" )  
  message ( STATUS "  NVCC Include: ${ARG_INCLUDE}" )
  message ( STATUS "  NVCC Archs: ${_ARCHS}" )
  
  if ( WIN32 ) 

    # Windows - PTX compile     
	  file ( MAKE_DIRECTORY "${ARG_TARGET_PATH}/Debug" )
	  file ( MAKE_DIRECTORY "${ARG_TARGET_PATH}/Release" )
    
	  # Custom build rule to generate ptx files from cuda files
	  foreach( kernel ${ARG_SOURCES} )
		  get_filename_component( kernel_ext ${kernel} EXT )						# Input extension
		  get_filename_component( kernel_base ${kernel} NAME_WE )				# Input base
		  if ( ${kernel_ext} STREQUAL ".cu" )			
				
		    # Set output names
		    set( _ptx_name "${kernel_base}.ptx" )							# Output name
		    set( _ptx_path "${ARG_TARGET_PATH}/$(Configuration)/${_ptx_name}" )	# Output with path		    
		    LIST( APPEND PTX_FILES ${_ptx_path} )		  # Output list with paths		    
		    add_custom_command (
			    OUTPUT ${_ptx_path}
			    MAIN_DEPENDENCY ${kernel}
			    COMMAND ${CMAKE_CUDA_COMPILER} --ptx ${ARG_OPTIONS} ${_ARCHS} ${kernel} -I\"${ARG_INCLUDE}\" -o \"${_ptx_path}\" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			  )
		  endif()

	  endforeach( )

  else ()
    
    # Linux  - PTX compile  
    
    foreach( kernel ${ARG_SOURCES} )
      get_filename_component( kernel_ext ${kernel} EXT )
      get_filename_component( kernel_base ${kernel} NAME_WE )
      if ( ${kernel_ext} STREQUAL ".cu" )		
        # Set output names
        set( _ptx_name "${kernel_base}.ptx" )							# Output name
		    set( _ptx_path "${ARG_TARGET_PATH}/${_ptx_name}" )	# Output with path
        LIST(APPEND PTX_FILES ${output})
        add_custom_command (
			    OUTPUT ${_ptx_path}
			    MAIN_DEPENDENCY ${kernel}
			    COMMAND ${CMAKE_CUDA_COMPILER} --ptx ${ARG_OPTIONS} ${_ARCHS} ${kernel} -I\"${ARG_INCLUDE}\" -o \"${_ptx_path}\" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			  )
      endif()

    endforeach()

  endif()

endmacro()


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

macro ( _ATTACH_FILES NAME LIST)  
  message ( STATUS "  ${NAME}: ${LIST}")
  set_property (GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIST} )
  message ( STATUS "  ---> Using ${NAME} (STATIC) " ) 
endmacro()

macro (_ATTACH_PLATFORM_LIB)
  set (options "")
  set (oneValueArgs NAME WIN LINUX)
  set (multiValueArgs "" )
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (WIN32)
	  set ( ADD_LIBS ${ARG_WIN} )	          
  else()
    set ( ADD_LIBS ${ARG_LINUX} )
	endif()
  list ( APPEND LIBS_PLATFORM ${ADD_LIBS} )

  message ( STATUS "  ---> Using ${ARG_NAME}" )
endmacro ()
          
macro ( _ATTACH_LIB )
  set (options "")
  set (oneValueArgs NAME INC BIN)
  set (multiValueArgs DEBUG_LIBS REL_LIBS DLLS )
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # lib includes  
  include_directories ( ${ARG_INC} )
  
  # lib export/static libs (append to list)
  
  list ( APPEND LIBS_DEBUG ${ARG_DEBUG_LIBS} )
  list ( APPEND LIBS_OPTIMIZED ${ARG_REL_LIBS} )
  # set(LIBS_DEBUG "${LIBS_DEBUG}" PARENT_SCOPE)            #-- necessary when called from functions
  # set(LIBS_OPTMIZED_LIBS "${LIBS_DEBUG}" PARENT_SCOPE)
  
  # lib shared/binaries - ARG_DLLS should be a list of fully pathed files
  list ( APPEND LIBS_PACKAGE_DLLS ${ARG_DLLS} )
  
  # message ( " LIBS_DEBUG: ${ARG_DLLS}" )  

  message ( STATUS "  ---> Using ${ARG_NAME} ")
endmacro()


macro( install_files )   
  set (generated_files "")
  set (oneValueArgs TARGET DESTINATION  )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT ARG_DESTINATION)
    set (ARG_DESTINATION "")
  endif()

  # ensure directory exists
  add_custom_command(
    TARGET ${PROJNAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ARG_DESTINATION}"
  )

  foreach(f ${ARG_FILES})
    get_filename_component(name ${f} NAME)
    set(out "${ARG_DESTINATION}/${name}")
    add_custom_command(
        TARGET ${PROJNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${f} ${out}        
    )    
  endforeach()
  
endmacro()


#------------------------------------------------- CROSS-PLATFORM INSTALL PTX
#
macro( install_ptx )   
  set (options "")
  set (oneValueArgs DESTINATION)
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # copy ptx files to destination
  foreach ( f ${ARG_FILES} )
	  get_filename_component ( name ${f} NAME ) 	    
	  set ( out "${ARG_DESTINATION}/${name}" ) 
    message ( STATUS "  Install ptx: ${f} --> ${out}")
  	add_custom_command ( TARGET ${PROJNAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy  ${f} ${out}
    )	
  endforeach()
  
endmacro()

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
  unset ( targetVar )  
  if ( WIN32 ) 
     SET ( nameFind ${nameWin64} )
  else()     
     SET ( nameFind ${nameLnx} )
  endif()  
  if ( "${nameFind}" STREQUAL ""  )
    MATH (EXPR ${cnt} "${${cnt}}+1" )	
  else()
    file(GLOB fileList "${${searchDir}}/${nameFind}")  
    list(LENGTH fileList NUMLIST)  
    if (NUMLIST GREATER 0)	
       MATH (EXPR ${cnt} "${${cnt}}+1" )	
       list(APPEND ${targetVar} ${nameFind} )
    endif() 
  endif()
endmacro()

#----------------------------------------------- CROSS-PLATFORM FIND MULTIPLE
# Find all files in specified folder with the given extension.
# This creates a file list, where each entry is only the filename w/o path
# Return the count of files
macro(_FIND_MULTIPLE targetVar searchDir preFix extWin64 extLnx )    
  unset ( fileList )    
  unset ( targetVar ) 
    if ( WIN32 )   
     SET ( extFind ${extWin64} )     
  else()
     SET ( extFind ${extLnx} )
  endif()  
  file( GLOB fileList "${${searchDir}}/${preFix}*.${extFind}")    
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

macro(_LINK ) 
  set (options "")
  set (multiValueArgs PROJECT OPT DEBUG PLATFORM )
  CMAKE_PARSE_ARGUMENTS(_LINK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug PARENT_SCOPE ) 

	set (PROJ_NAME ${_LINK_PROJECT})
  
  target_link_libraries(${PROJ_NAME} PRIVATE 
    libmin::libmin 
    libext::libext
    ${_LINK_PLATFORM}
    $<$<CONFIG:Debug>:${_LINK_DEBUG}>
    $<$<CONFIG:Release>:${_LINK_OPT}>  
  )

  #--- NOTE: does not work separately. VS does not respect 'debug' and 'optimized'
  # target_link_libraries( ${PROJ_NAME} PRIVATE libmin::libmin libext::libext )	
  # target_link_libraries( ${PROJ_NAME} PRIVATE ${_LINK_PLATFORM} )
  # target_link_libraries( ${PROJ_NAME} PRIVATE debug ${_LINK_DEBUG} )
  # target_link_libraries( ${PROJ_NAME} PRIVATE optimized ${_LINK_OPT} )
  
  #--- NOTE: do not construct in this way! CMake does not handle $<$<CONFIG:Debug> properly when invoked many times.
	#foreach (item IN ITEMS ${_LINK_PLATFORM} )
  #	target_link_libraries( ${PROJ_NAME} PRIVATE ${item} )	
	#	list (APPEND LIBLIST ${item})
	#endforeach() 	
	#foreach (item IN ITEMS ${_LINK_DEBUG} )
	#	target_link_libraries ( ${PROJ_NAME} PRIVATE $<$<CONFIG:Debug>:${item} )
	#	list (APPEND LIBLIST ${item})
	#endforeach()	
	#foreach (item IN ITEMS ${_LINK_OPT} )   
  #	target_link_libraries ( ${PROJ_NAME} PRIVATE $<$<CONFIG:Release>:${item} )
  #endforeach()

  if (BUILD_CUDA)
      # link to cuda libraries
      target_link_libraries( ${PROJNAME} PRIVATE cuda)

      # force include dirs
      target_include_directories(${PROJ_NAME} PRIVATE "$ENV{CUDA_PATH}/include")
  endif()


  
  message ( STATUS "\n----- DONE" )	
	string (REPLACE ";" "\n   " OUTSTR "${LIBLIST}")
	message ( STATUS "  Libraries used:\n   ${OUTSTR}" )
endmacro()

macro(_STARTUP_PROJECT )

    if (WIN32) 
	    # Instruct CMake to automatically build INSTALL project in Visual Studio 
	    # set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)

	    set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
      set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR} )
      set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR} )

	    # Set startup PROJECT	
	    if ( (${CMAKE_MAJOR_VERSION} EQUAL 3 AND ${CMAKE_MINOR_VERSION} GREATER 5) OR (${CMAKE_MAJOR_VERSION} GREATER 3) )
		    message ( STATUS "  MSVC Startup: ${PROJNAME}")
		    set_property ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJNAME} )
	    endif()		

      source_group("Libmin" FILES ${LIBMIN_FILES} )
    endif()

endmacro ()

