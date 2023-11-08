cmake_minimum_required(VERSION 2.8)

set(PROJNAME libmin)
Project(${PROJNAME})
Message("-------------------------------")
Message("PROJECT NAME: ${PROJNAME}")

#####################################################################################
# Bootstrap
#
list (APPEND CMAKE_PREFIX_PATH "cmake" )
list (APPEND CMAKE_PREFIX_PATH "../libmin/cmake" )
set( BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "")

find_package ( Libmin CONFIG )
include_directories(${LIBMIN_SRC})
include_directories(${LIBMIN_SRC_INC})

#####################################################################################
# Options

add_definitions ( -DLIBMIN_EXPORTS)		# we are building the libmin.dll. create export funcs

add_definitions ( -DGLEW_STATIC)	    	# static build GLEW into libmin

add_definitions ( -DBUILD_PNG)			# always build PNG
add_definitions ( -DBUILD_TIF)			# always build TIF

set( USE_NVTX true CACHE BOOL "")		# performance markers available

#####################################################################################
# Include OpenGL
#
OPTION (BUILD_OPENGL "Build with OpenGL" ON)
if (BUILD_OPENGL)
   add_definitions(-DUSE_OPENGL)  		# Use OpenGL

   IF (WIN32)
     LIST(APPEND LIBRARIES_OPTIMIZED "opengl32.lib" )
     LIST(APPEND LIBRARIES_DEBUG "opengl32.lib" )
   ENDIF()
endif()

OPTION (BUILD_CUDA "Build with CUDA" OFF)
if (BUILD_CUDA) 

	#####################################################################################
	# Find CUDA

	if (NOT CUDA_TOOLKIT_ROOT_DIR) 
		set ( CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.2" CACHE PATH "CUDA Toolkit path")
	endif()

	find_package(CUDA)
	if ( CUDA_FOUND )
		message( STATUS "--> Using package CUDA (ver ${CUDA_VERSION})") 
		add_definitions(-DUSE_CUDA)    
		include_directories(${CUDA_TOOLKIT_INCLUDE})
		LIST(APPEND LIBRARIES_OPTIMIZED ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} )
		LIST(APPEND LIBRARIES_DEBUG ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} )
		LIST(APPEND PACKAGE_SOURCE_FILES ${CUDA_TOOLKIT_INCLUDE} )
		source_group(CUDA FILES ${CUDA_TOOLKIT_INCLUDE} ) 
		if (USE_NVTX)
		  add_definitions(-DUSE_NVTX)
		endif()
	else()
 		message ( FATAL_ERROR "---> Unable to find package CUDA")
	endif()
endif()

# Source files for this project
#
file(GLOB SOURCE_FILES src/*.cpp src/*.c)
file(GLOB OPENGL_FILES include/GL/*.c)
file(GLOB INCLUDE_FILES include/*.hpp include/*.h)

file(GLOB INSTALL_CMAKES cmake/*.*)
file(GLOB INSTALL_MAINS mains/*.*)

#####################################################################################
# Library paths
#
# CMAKE_INSTALL_PREFIX -- path where library will be installed to

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   if ( WIN32 )
      get_filename_component ( _instpath "${CMAKE_CURRENT_BINARY_DIR}" REALPATH )
   else()
      get_filename_component ( _instpath "/usr/local/libmin" REALPATH )
   endif()
   set ( CMAKE_INSTALL_PREFIX ${_instpath} CACHE PATH "default install path" FORCE)   
endif()

get_filename_component( LIB_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} REALPATH)
get_filename_component( INSTALL_BIN_PATH ${CMAKE_INSTALL_PREFIX}/bin REALPATH)
get_filename_component( INSTALL_INC_PATH ${CMAKE_INSTALL_PREFIX}/include REALPATH)
get_filename_component( INSTALL_LIB_PATH ${CMAKE_INSTALL_PREFIX}/lib REALPATH)

set ( EXECUTABLE_OUTPUT_PATH ${INSTALL_BIN_PATH} CACHE PATH "" FORCE )


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

# debug and relase libs
set ( CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" )
set_target_properties( ${PROJNAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

set_target_properties( ${PROJNAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${LIB_OUTPUT_PATH} )
set_target_properties( ${PROJNAME} PROPERTIES VS_INTERMEDIATE_DIRECTORY_DEBUG ${LIB_OUTPUT_PATH}/Debug )
set_target_properties( ${PROJNAME} PROPERTIES VS_INTERMEDIATE_DIRECTORY_RELEASE ${LIB_OUTPUT_PATH}/Release )
set_target_properties( ${PROJNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${LIB_OUTPUT_PATH} )
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
if (BUILD_OPENGL)
	file (COPY "${CMAKE_CURRENT_SOURCE_DIR}/include/GL" DESTINATION ${INSTALL_INC_PATH} )
	file (COPY "${CMAKE_CURRENT_SOURCE_DIR}/include/GL/glew.c" DESTINATION ${INSTALL_INC_PATH} )
endif()
if (BUILD_CUDA)
	install ( FILES ${CUDA_FILES} DESTINATION ${INSTALL_INC_PATH} )
endif()

install ( FILES ${INCLUDE_FILES} DESTINATION ${INSTALL_INC_PATH} )

install ( FILES ${INSTALL_LIST} DESTINATION ${INSTALL_BIN_PATH} )

install ( FILES ${INSTALL_CMAKES} DESTINATION "${LIB_OUTPUT_PATH}/cmake" )
install ( FILES ${INSTALL_MAINS} DESTINATION "${LIB_OUTPUT_PATH}/mains" )

if ( LINUX )
  install ( FILES $<TARGET_PDB_FILE:${PROJNAME}> DESTINATION ${INSTALL_BIN_PATH} OPTIONAL )
endif()
install ( FILES $<TARGET_LINKER_FILE:${PROJNAME}> DESTINATION ${INSTALL_BIN_PATH} )
_INSTALL ( FILES ${PACKAGE_DLLS} DESTINATION ${INSTALL_BIN_PATH} )
install ( TARGETS ${PROJNAME} DESTINATION ${INSTALL_BIN_PATH} )

#################################
# Done
message ( STATUS "---------------------------------" )
message ( STATUS "Output:   	 ${LIB_OUTPUT_PATH}" )
message ( STATUS "Installed Bin: ${INSTALL_BIN_PATH}" )
message ( STATUS "Installed Inc: ${INSTALL_INC_PATH}" )
message ( "\nLIBMIN INSTALLED PATH: ${CMAKE_INSTALL_PREFIX}" )
message ( STATUS "---------------------------------" )



