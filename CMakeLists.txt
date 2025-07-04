
cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0175)
  cmake_policy(SET CMP0175 OLD) 
endif()

set (CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "")

if(WIN32)
  set(PROJNAME libmin)
else()
  # linux prefixes the lib automatically
  set(PROJNAME min)
  add_compile_options (-g)
  # set_source_files_properties (network_system.cpp PROPERTIES COMPILE_FLAGS -g)
  add_compile_options (-Wno-multichar)
endif()

# add_compile_options (-O3)

Project(${PROJNAME})
Message("-------------------------------")
Message("PROJECT NAME: ${PROJNAME}")

#####################################################################################
# Bootstrap
#
set ( LIBMIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" )
list (APPEND CMAKE_PREFIX_PATH "${LIBMIN_ROOT}/cmake" )
list (APPEND CMAKE_MODULE_PATH "${LIBMIN_ROOT}/cmake" )
set ( LIBMIN_INSTALL "SELF" CACHE PATH "Libmin binaries" )
find_package ( Libmin CONFIG )
unset ( Libmin_DIR CACHE )

#####################################################################################
# Options

add_definitions ( -DLIBMIN_EXPORTS)		# we are building the libmin.dll. create export funcs

add_definitions ( -DBUILD_PNG)			# always build PNG
add_definitions ( -DBUILD_TIF)			# always build TIF

_REQUIRE_GL ()

_REQUIRE_GLEW ()	       

_REQUIRE_OPENSSL (false)	            # default = false

OPTION ( BUILD_JPG "Build with JPG" OFF )
if (BUILD_JPG)
  _REQUIRE_JPG ()
endif()

OPTION ( USE_NVTX "Use NVTX markers" OFF )     #  performance markers available
if ( USE_NVTX )
  add_definitions ( -DUSE_NVTX )
endif()

OPTION ( USE_PROFILE_NET "Profile network system" OFF)	# profile network system
if ( USE_PROFILE_NET )
  add_definitions ( -DPROFILE_NET )
endif()

#####################################################################################
# Find CUDA

OPTION (BUILD_CUDA "Build with CUDA" OFF)
if (BUILD_CUDA) 

    # enable_lanague(CUDA)
	
	# NOTE: When running cmake, use -T cuda=10.2, for cmake-gui specify a Configure time (clear cache)
    if (NOT CUDA_TOOLKIT_VERSION) 
		set ( CUDA_TOOLKIT_VERSION "12.4" )
		message ( STATUS "*** No CUDA_TOOLKIT_VERSION specified. Assuming CUDA ${CUDA_TOOLKIT_VERSION}." )
	endif()
	
	find_package(CUDAToolkit "${CUDA_TOOLKIT_VERSION}" REQUIRED EXACT)
	if ( CUDAToolkit_FOUND )
		message( STATUS "--> Using package CUDA (ver ${CUDAToolkit_VERSION})") 
		add_definitions(-DUSE_CUDA)    
		include_directories(${CUDAToolkit_INCLUDE_DIRS})
		LIST(APPEND LIBRARIES_OPTIMIZED ${CUDAToolkit_CUDA_LIBRARY} ${CUDAToolkit_CUDART_LIBRARY} )
		LIST(APPEND LIBRARIES_DEBUG ${CUDAToolkit_CUDA_LIBRARY} ${CUDAToolkit_CUDART_LIBRARY} )
		LIST(APPEND PACKAGE_SOURCE_FILES ${CUDAToolkit_INCLUDE_DIRS} )
		source_group(CUDA FILES ${CUDAToolkit_INCLUDE_DIRS} ) 
		if (USE_NVTX)
		  add_definitions(-DUSE_NVTX)
		endif()
	    # set(CUDA_PTX_COMPILATION ON)		
	else()
 		message ( FATAL_ERROR "---> Unable to find package CUDA")
	endif()
endif()

#################################################################
# Source files for this project
#
file(GLOB SOURCE_FILES src/*.cpp src/*.c)
file(GLOB OPENGL_FILES include/GL/*.c)
file(GLOB INCLUDE_FILES include/*.hpp include/*.h)

file(GLOB INSTALL_CMAKES cmake/*.*)
file(GLOB INSTALL_MAINS mains/*.*)

include_directories(${LIBMIN_ROOT_SRC})
include_directories(${LIBMIN_ROOT_INC})

#####################################################################################
# Library output
#
unset ( ALL_SOURCE_FILES )
list( APPEND ALL_SOURCE_FILES ${SOURCE_FILES} )
list( APPEND ALL_SOURCE_FILES ${INCLUDE_FILES} )
list( APPEND ALL_SOURCE_FILES ${PACKAGE_SOURCE_FILES} )
if (BUILD_OPENGL)
	list( APPEND ALL_SOURCE_FILES ${OPENGL_FILES} )
endif()
# list( APPEND ALL_SOURCE_FILES ${UTIL_OPTIX_FILES} )
if (BUILD_CUDA)
	file (GLOB CUDA_FILES cuda/*.cuh cuda/*.cu)	
endif()

# Definitions
add_definitions(-DLIBHELP_EXPORTS)	# Export dll symbols

# Set the library type
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" ON)
set (LIB_TYPE STATIC)
if (BUILD_SHARED_LIBS)
  set (LIB_TYPE SHARED)
endif()

if( WIN32 AND NOT GLUT_FOUND)
  add_definitions(/wd4267) #remove size_t to int warning
  add_definitions(/wd4996) #remove printf warning
  add_definitions(/wd4244) #remove double to float conversion warning
  add_definitions(/wd4305) #remove double to float truncation warning
endif()

add_library (${PROJNAME} ${LIB_TYPE} ${ALL_SOURCE_FILES} ${CUDA_FILES} ${PTX_FILES} )

if (BUILD_CUDA) 
  target_link_libraries( ${PROJNAME} CUDA::cuda_driver)
endif()
  
# debug and relase libs
set ( CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" )
set_target_properties( ${PROJNAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

set_target_properties( ${PROJNAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX} )
set_target_properties( ${PROJNAME} PROPERTIES VS_INTERMEDIATE_DIRECTORY_DEBUG ${CMAKE_INSTALL_PREFIX}/Debug )
set_target_properties( ${PROJNAME} PROPERTIES VS_INTERMEDIATE_DIRECTORY_RELEASE ${CMAKE_INSTALL_PREFIX}/Release )
set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX} )
set_target_properties( ${PROJNAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

#####################################################################################
# Linkage
#
_LINK ( PROJECT ${PROJNAME} OPT ${LIBRARIES_OPTIMIZED} DEBUG ${LIBRARIES_DEBUG} PLATFORM ${PLATFORM_LIBRARIES} )

################################################################
# Windows specific
#
if ( WIN32 )    
	# instruct CMake to automatically build INSTALL project in Visual Studio 
	set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)
	# visual studio source groups
	source_group(Include FILES ${INCLUDE_FILES} )
	source_group(Source FILES ${SOURCE_FILES} )	
	source_group(OpenGL FILES ${OPENGL_FILES} )
endif()

################################################################
# Install Binaries
#
set (_inc_path "${CMAKE_INSTALL_PREFIX}/include/" )
set (_bin_path "${CMAKE_INSTALL_PREFIX}/bin/" )
file (MAKE_DIRECTORY "${_inc_path}" )
file (MAKE_DIRECTORY "${_bin_path}" )

message ( STATUS "LIBEXT Root:   ${LIBEXT_ROOT}" )

if (BUILD_OPENGL)
	include_directories( "${LIBEXT_ROOT}/include" )
	file (COPY "${LIBEXT_ROOT}/include/GL" DESTINATION ${_inc_path} )
	file (COPY "${LIBEXT_ROOT}/include/GL/glew.c" DESTINATION ${_inc_path} )
endif()
if (BUILD_CUDA)
   message ( STATUS "CUDA Files: ${CUDA_FILES}")
   install ( FILES ${CUDA_FILES} DESTINATION ${_inc_path} )
endif()

install ( FILES ${GLEW_FILES} DESTINATION ${_bin_path} )

# header files
message ( STATUS "Include files: ${LIBMIN_ROOT_INC} --> ${_inc_path}" )
_INSTALL ( FILES ${INCLUDE_FILES} DESTINATION ${_inc_path} )

# pdb & dlls (windows)
if ( WIN32 )
  install ( FILES $<TARGET_PDB_FILE:${PROJNAME}> DESTINATION ${_bin_path} OPTIONAL )
  _INSTALL ( FILES ${PACKAGE_DLLS} DESTINATION ${_bin_path} )
endif()


# output shared library
install ( TARGETS ${PROJNAME} LIBRARY DESTINATION ${_bin_path} )
install ( TARGETS ${PROJNAME} DESTINATION ${_bin_path} )

#################################
# Done
message ( STATUS "---------------------------------" )
message ( STATUS "Install Bin: ${_bin_path}" )
message ( STATUS "Install Inc: ${_inc_path}" )
message ( STATUS "Install Path: ${CMAKE_INSTALL_PREFIX}" )
message ( STATUS "---------------------------------" )



