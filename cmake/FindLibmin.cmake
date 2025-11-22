
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
  get_property ( LIBS_OPTIMIZED GLOBAL PROPERTY LIBS_OPTIMIZED)
  get_property ( LIBS_DEBUG GLOBAL PROPERTY LIBS_DEBUG)
  get_property ( LIBS_PLATFORM GLOBAL PROPERTY LIBS_PLATFORM)
  get_property ( PACKAGE_DLLS GLOBAL PROPERTY PACKAGE_DLLS )
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

    add_definitions(-DBUILD_MAIN)
    IF (BUILD_CONSOLE)
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

function ( _REQUIRE_CONSOLE )
  add_definitions(-DBUILD_MAIN)
  LIST( APPEND SRC_FILES "${LIBMIN_ROOT_MAINS}/main_console.cpp" )
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
          add_definitions(-DBUILD_OPENGL)  		
          if (WIN32)
	          set ( GL_LIBS "opengl32.lib" )	          
          else()
            set( GL_LIBS "gl glew x11" )
	        endif()
          set_property(GLOBAL APPEND PROPERTY LIBS_PLATFORM ${GL_LIBS} )
          message ( STATUS "  ---> Using GL" )
        endif()
    endif()
endfunction()

function ( _REQUIRE_GLEW)    
    OPTION (BUILD_GLEW "Build with GLEW" ON)
    if (BUILD_GLEW)
      add_definitions(-DGLEW_STATIC)	    
      LIST(APPEND GLEW_FILES "${LIBEXT_ROOT}/include/GL/glew.c" )       
      _ATTACH_FILES ( "GLEW" ${GLEW_FILES} )      
    endif()
endfunction()

###################################################################################
# Include OpenSSL
#
function ( _REQUIRE_OPENSSL _openssl_default)
    OPTION (BUILD_OPENSSL "Build with OpenSSL" ${_openssl_default} )
    if (BUILD_OPENSSL) 

	    find_package(OpenSSL)

	    if ( OPENSSL_FOUND )	
		    add_definitions(-DBUILD_OPENSSL)	
		    include_directories ( ${OPENSSL_INC} )
		    link_directories ( ${OPENSSL_BIN} )
        _ATTACH_LIB ( "OpenSSL" ${OPENSSL_INC} ${OPENSSL_BIN} ${OPENSSL_DEBUG_LIBS} ${OPENSSL_REL_LIBS} ${OPENSSL_DLLS} )
	    else()
		    message ( FATAL_ERROR "\n  Unable to find OpenSLL library. Link with a different /libext for third-party libs that include OpenSSL.\n")
	    endif()
    endif()
endfunction()

#####################################################################################
# Include Bcrypt
#
function ( _REQUIRE_BCRYPT _bcrypt_default)
    OPTION (BUILD_BCRYPT "Build with Bcrypt" ${_bcrypt_default} )
    if (BUILD_BCRYPT) 
        find_package(Bcrypt)
        if ( BCRYPT_FOUND )	
	        add_definitions(-DBUILD_BCRYPT)
          _ATTACH_LIB ( "BCrypt" ${BCRYPT_INC} ${BCRYPT_BIN} ${BCRYPT_DEBUG_LIBS} ${BCRYPT_REL_LIBS} ${BCRYPT_DLLS} )		        	        
        endif()
    endif()
endfunction()

#####################################################################################
# Include JPG    
#   
function ( _REQUIRE_JPG )   
  set ( OK_H "0" )    
  set ( _jpg_srch "${LIBEXT_ROOT}/win64" )
	_FIND_FILE ( JPG_LIBS _jpg_srch "libjpg_2019x64.lib" "" OK_H )		
	if ( OK_H EQUAL 1 )
      add_definitions(-DBUILD_JPG) 
      set ( JPG_OPT "${LIBEXT_ROOT}/win64/libjpg_2019x64.lib" )
      set ( JPG_DEBUG "${LIBEXT_ROOT}/win64/libjpg_2019x64d.lib" )
      set_property (GLOBAL APPEND PROPERTY LIBS_OPTIMIZED ${JPG_OPT} )
      set_property (GLOBAL APPEND PROPERTY LIBS_DEBUG ${JPG_DEBUG} )
      include_directories( "${LIBEXT_ROOT}/include" )
      message ( STATUS "  ---> Using libjpg" )
    else ()
      message ( FATAL_ERROR "
      JPG libraries not found. 
      Set LIBEXT_DIR to libmin/libext path for 3rd party libs.
      ")
    endif()
endfunction()


####################################################################################
# Include CUDA
#
macro ( _REQUIRE_CUDA BUILD_CUDA_default kernel_path) 
    
    OPTION (BUILD_CUDA "Build with CUDA" ${BUILD_CUDA_default})

    if (BUILD_CUDA) 
      message(STATUS "  CUDA support enabled")    

      # enable CUDA language (with cmake >3.18)
      enable_language(CUDA)        
      set(CMAKE_CUDA_STANDARD 17)
      set(CMAKE_CUDA_STANDARD_REQUIRED ON)      

      # attach cuda common helpers
      add_definitions(-DBUILD_CUDA)  
      LIST(APPEND CUDA_COMMON "${LIBMIN_ROOT}/src/cuda/common_cuda.cpp" "${LIBMIN_ROOT}/include/cuda/common_cuda.h")
      _ATTACH_FILES ( "CUDA" ${CUDA_COMMON} )   
      
      # attach all .cu and .cuh in the kernel_path folder given
      get_filename_component (_cuda_kernels_path "${CMAKE_CURRENT_SOURCE_DIR}/${kernel_path}" REALPATH)
      file(GLOB CUDA_FILES "${_cuda_kernels_path}/*.cu" )        # project scope          
      # "${_cuda_kernels_path}/*.cuh"

      # give access to libmin cuda kernels
      set ( LIBMIN_KERNELS_PATH "${LIBMIN_ROOT}/include/cuda")
      target_include_directories ( libmin::libmin INTERFACE ${LIBMIN_KERNELS_PATH} )
   

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
        if ( BUILD_DEBUG_PTX )
	        compile_ptx ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${CMAKE_CURRENT_SOURCE_DIR},${LIBMIN_ROOT_INC},${LIBMIN_KERNELS_PATH}" OPTIONS --ptx -dc -Xptxas -v -G -g --use_fast_math --maxrregcount=32 )
        else()
	        compile_ptx ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${CMAKE_CURRENT_SOURCE_DIR},${LIBMIN_ROOT_INC},${LIBMIN_KERNELS_PATH}" OPTIONS --ptx -dc -Xptxas -v -O3 --use_fast_math --maxrregcount=32 )
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
  
  if ( WIN32 ) 

    # Windows - PTX compile
     
	  file ( MAKE_DIRECTORY "${ARG_TARGET_PATH}/Debug" )
	  file ( MAKE_DIRECTORY "${ARG_TARGET_PATH}/Release" )	  	  	  
    
    # nvcc arch target (only 1 allowed when compiling to ptx)
    foreach(arch ${CMAKE_CUDA_ARCHITECTURES} )
      list(APPEND _ARCHS "--generate-code=arch=compute_${arch},code=[sm_${arch},compute_${arch}]")
    endforeach()

    # convey setup
    message ( STATUS "  NVCC Options: ${ARG_OPTIONS}" )  
	  message ( STATUS "  NVCC Include: ${ARG_INCLUDE}" )
    message ( STATUS "  NVCC Archs: ${_ARCHS}" )
    
	  # Custom build rule to generate ptx files from cuda files
	  FOREACH( kernel ${ARG_SOURCES} )
		  get_filename_component( kernel_ext ${kernel} EXT )						# Input extension
		  get_filename_component( kernel_base ${kernel} NAME_WE )				# Input base
		  if ( ${kernel_ext} STREQUAL ".cu" )			
				
		    # Set output names
		    set( _ptx_name "${kernel_base}.ptx" )							# Output name
		    set( _ptx_path "${ARG_TARGET_PATH}/$(Configuration)/${_ptx_name}" )	# Output with path		    
		    LIST( APPEND PTX_FILES ${_ptx_path} )		  # Output list with paths		    
		    add_custom_command(
			    OUTPUT ${_ptx_path}
			    MAIN_DEPENDENCY ${kernel}
			    COMMAND ${CMAKE_CUDA_COMPILER} --ptx ${ARG_OPTIONS} ${_ARCHS} ${kernel} -I\"${ARG_INCLUDE}\" -o \"${_ptx_path}\" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			    )			
		    endif()
	  ENDFOREACH( )

  else ()
    
    # Linux - PTX compile
    #    
    if (CMAKE_CUDA_COMPILER)

      message ( STATUS "  NVCC Include: ${ARG_INCLUDE}" )
      add_library ( PTX_FILES OBJECT ${ARG_SOURCES})
      set_property (TARGET PTX_FILES PROPERTY CUDA_PTX_COMPILATION ON )
      string (REPLACE "," ";" _COMPILEPTX_INCLUDE "${ARG_INCLUDE}")  
      FOREACH(input ${ARG_SOURCES} )
        get_filename_component( input_ext ${input} EXT )
        get_filename_component( input_without_ext ${input} NAME_WE )
        set( output "${input_without_ext}.ptx" )
        set( output_with_path "${ARG_TARGET_PATH}/CMakeFiles/PTX_FILES.dir/source/${input_without_ext}.ptx" )
        LIST(APPEND PTX_FILES ${output})
        LIST(APPEND PTX_FILES_PATH ${output_with_path})
      ENDFOREACH()
      FOREACH(input ${ARG_INCLUDE})
        message ( STATUS "  PTX INCLUDE: ${input}")
        target_include_directories ( PTX_FILES PRIVATE ${input} )        
      ENDFOREACH()

    else()      

      file ( MAKE_DIRECTORY "${ARG_TARGET_PATH}" )
      FOREACH(input ${ARG_SOURCES})
        get_filename_component( input_ext ${input} EXT )									# Input extension
        get_filename_component( input_without_ext ${input} NAME_WE )						# Input base
        if ( ${input_ext} STREQUAL ".cu" )			
          # Set output names
          set( output "${input_without_ext}.ptx" ) # Output name
          set( output_with_path "${ARG_TARGET_PATH}/${input_without_ext}.ptx" )	# Output with path

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

function ( _ATTACH_FILES NAME LIST)  
  message ( STATUS "  ${NAME}: ${LIST}")
  set_property (GLOBAL APPEND PROPERTY LIBMIN_FILES ${LIST} )
  message ( STATUS "  ---> Using ${NAME} (STATIC) " ) 
endfunction()

function ( _ATTACH_LIB NAME INC BINPATH DEBUG REL DLLS)
  
  set_property (GLOBAL APPEND PROPERTY LIBS_DEBUG "${DEBUG}" )
  set_property (GLOBAL APPEND PROPERTY LIBS_OPTIMIZED "${REL}" )
	_EXPANDLIST( OUTPUT PACKAGE_DLLS SOURCE ${BINPATH} FILES ${DLLS} )

  set_property (GLOBAL APPEND PROPERTY PACKAGE_DLLS ${DLLS} )
  message ( STATUS "  ---> Using ${NAME}, ${BINPATH}/${DLLS} ")
endfunction()


function( install_files PROJ FILES DESTINATION OUTPUT_VAR )   
  set (generated_files "")
  set (oneValueArgs PROJ DESTINATION OUTPUT_VAR )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT ARG_DESTINATION)
    set (ARG_DESTINATION "")
  endif()
    
  foreach(f ${ARG_FILES})
    get_filename_component(name ${f} NAME)
    set(out "${CMAKE_BINARY_DIR}/${ARG_DESTINATION}/${name}")

    add_custom_command(
        OUTPUT ${out}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${ARG_DESTINATION}"
        COMMAND ${CMAKE_COMMAND} -E copy ${f} ${out}
        DEPENDS ${f}
    )
    list(APPEND generated_files ${out})
  endforeach()
  
  add_custom_target( ${ARG_PROJ} ALL DEPENDS ${generated_files})
  
  install(FILES ${generated_files} DESTINATION ${ARG_DESTINATION})
  
  set( ${ARG_OUTPUT_VAR} ${generated_files} PARENT_SCOPE)   
endfunction()


#------------------------------------------------- CROSS-PLATFORM INSTALL PTX
#
macro( install_ptx )   
  set (options "")
  set (oneValueArgs DESTINATION)
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # copy ptx files to destination
  foreach ( _file ${ARG_FILES} )
	  get_filename_component ( _ptxname ${_file} NAME ) 	    
	  set ( _fixed "${ARG_DESTINATION}/${_ptxname}" ) 
    message ( STATUS "  Install ptx: ${_file} --> ${_fixed}")
  	add_custom_command ( TARGET ${PROJNAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy  ${_file} ${_fixed}
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

function(_LINK ) 
  set (options "")
  set (multiValueArgs PROJECT OPT DEBUG PLATFORM )
  CMAKE_PARSE_ARGUMENTS(_LINK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug PARENT_SCOPE ) 

	set (PROJ_NAME ${_LINK_PROJECT})

  target_link_libraries( ${PROJ_NAME} PRIVATE libmin::libmin libext::libext )	

	foreach (item IN ITEMS ${_LINK_PLATFORM} )
		target_link_libraries( ${PROJ_NAME} PRIVATE ${item} )	
		list (APPEND LIBLIST ${item})
	endforeach() 	

	foreach (item IN ITEMS ${_LINK_DEBUG} )
		target_link_libraries ( ${PROJ_NAME} PRIVATE $<$<CONFIG:Debug>:${item} )
		list (APPEND LIBLIST ${item})
	endforeach()
	
	foreach (item IN ITEMS ${_LINK_OPT} )   
		target_link_libraries ( ${PROJ_NAME} PRIVATE $<$<CONFIG:Release>:${item} )
	endforeach()

  if (BUILD_CUDA)
      # link to cuda libraries
      target_link_libraries( ${PROJNAME} PRIVATE cuda)
  endif()
  
  message ( STATUS "\n----- DONE" )	
	string (REPLACE ";" "\n   " OUTSTR "${LIBLIST}")
	message ( STATUS "  Libraries used:\n   ${OUTSTR}" )
endfunction()

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

