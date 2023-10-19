cmake_minimum_required(VERSION 2.8)
set(PROJNAME libgx)
Project(${PROJNAME})
Message(STATUS "-------------------------------")
Message(STATUS "Processing Project ${PROJNAME}:")

#####################################################################################
# Bootstrap
#
set( BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
find_path ( CMAKE_HELPERS_PATH "HelpersBootstrap.cmake" HINTS ${BASE_DIRECTORY}/../lib_full/cmake/ )
if ( ${CMAKE_HELPERS_PATH} STREQUAL "HELPERS-NOTFOUND" )
    message ( FATAL_ERROR "\n Please set the CMAKE_HELPERS_PATH to location of HelpersBootstrap.cmake" )
endif()
include( ${CMAKE_HELPERS_PATH}/HelpersBootstrap.cmake )		# Cross-Platform functions
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/include")
set ( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )

####################################################################################
# Find LIBFULL
#
find_package(LibFull)

if (LIBFULL_FOUND)
  add_definitions(-DUSE_LIBFULL)  
  add_definitions(-DUSE_NETWORK)      
  include_directories(${LIBFULL_INC})
  include_directories(${LIBEXT_INC})
  LIST( APPEND LIBRARIES_OPTIMIZED "${LIBFULL_LIB}/${LIBFULL_REL}")
  LIST( APPEND LIBRARIES_DEBUG "${LIBFULL_LIB}/${LIBFULL_DEBUG}")	
  _EXPANDLIST( OUTPUT PACKAGE_DLLS SOURCE ${LIBFULL_LIB} FILES ${LIBFULL_DLLS} )
  message ( STATUS "--> Using LIBFULL")

  LIST( APPEND LIBRARIES_OPTIMIZED "shcore.lib")
  LIST( APPEND LIBRARIES_DEBUG "shcore.lib")	
  add_definitions(-DUSE_DPI_AWARE)  
  message ( STATUS "--> Using DPI AWARE")
endif()


#####################################################################################
# Options

# this is only for GX app testing. most gx_lib usage should not enable this
add_definitions ( -DGX_STATIC)

_REQUIRE_MAIN()

_REQUIRE_GL ()

_REQUIRE_JPG ()




#--- symbols in release mode
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF" CACHE STRING "" FORCE)

######################
# CMAKE_INSTALL_PREFIX -- path where library will be installed to

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   if ( WIN32 )
      get_filename_component ( _instpath "${CMAKE_CURRENT_BINARY_DIR}" REALPATH )
   else()
      get_filename_component ( _instpath "/usr/local/shapes" REALPATH )
   endif()
   set ( CMAKE_INSTALL_PREFIX ${_instpath} CACHE PATH "default install path" FORCE)   
endif()

#####################################################################################
# Assets
#
if ( NOT DEFINED ASSET_PATH ) 
  get_filename_component ( _assets "${BASE_DIRECTORY}/assets" REALPATH )
  set ( ASSET_PATH ${_assets} CACHE PATH "Full path to /assets" )
  add_definitions( -DASSET_PATH="${ASSET_PATH}/" )
endif()
file(GLOB ASSET_FILES ${ASSET_PATH}/*.* )

if ( NOT DEFINED SHADER_PATH ) 
  get_filename_component ( _shaders "${BASE_DIRECTORY}/shaders" REALPATH )
  set ( SHADER_PATH ${_shaders} CACHE PATH "Full path to /shaders" )
  add_definitions( -DSHADER_PATH="${SHADER_PATH}/" )
endif()
file(GLOB SHADER_FILES ${SHADER_PATH}/*.* )

#####################################################################################
# Executable
#
file(GLOB MAIN_FILES *.cpp *.c *.h )

list( APPEND ALL_SOURCE_FILES ${MAIN_FILES} )

#-- generic codes
list( APPEND ALL_SOURCE_FILES ${COMMON_SOURCE_FILES} )
list( APPEND ALL_SOURCE_FILES ${PACKAGE_SOURCE_FILES} )

if ( NOT DEFINED WIN32 )
  set(libdeps GL GLEW X11)
  LIST(APPEND LIBRARIES_OPTIMIZED ${libdeps})
  LIST(APPEND LIBRARIES_DEBUG ${libdeps})
ENDIF()

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}")    
add_definitions(-DASSET_PATH="${ASSET_PATH}/")
add_definitions(-DSHADER_PATH="${SHADER_PATH}/")

add_executable (${PROJNAME} ${ALL_SOURCE_FILES} ${ASSET_FILES} ${SHADER_FILES} ) 

#--- Debug and Release .EXE
set ( CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" )
set_target_properties( ${PROJNAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

#####################################################################################
# Additional Libraries
#
_LINK ( PROJECT ${PROJNAME} OPT ${LIBRARIES_OPTIMIZED} DEBUG ${LIBRARIES_DEBUG} PLATFORM ${PLATFORM_LIBRARIES} )

#####################################################################################
# Windows specific
#
_MSVC_PROPERTIES()
source_group(Shaders FILES ${SHADER_FILES})
source_group(Assets FILES ${ASSET_FILES})
source_group(Main FILES ${MAIN_FILES} ${PACKAGE_SOURCE_FILES})

#####################################################################################
# Install Binaries
#
#
_DEFAULT_INSTALL_PATH()

_COPY_AT_BUILD ( FOLDER "${ASSET_PATH}" DESTINATION "${EXECUTABLE_OUTPUT_PATH}/assets" )	# assets 
_COPY_AT_BUILD ( FOLDER "${SHADER_PATH}" DESTINATION "${EXECUTABLE_OUTPUT_PATH}/shaders" )	# shaders

_INSTALL ( FILES ${SHADERS} DESTINATION "${EXECUTABLE_OUTPUT_PATH}/assets" )			# shaders
_INSTALL ( FILES ${PACKAGE_DLLS} DESTINATION ${EXECUTABLE_OUTPUT_PATH} )			# DLLs
install ( FILES $<TARGET_PDB_FILE:${PROJNAME}> DESTINATION ${EXECUTABLE_OUTPUT_PATH} OPTIONAL )	# PDB

install ( FILES ${INSTALL_LIST} DESTINATION ${EXECUTABLE_OUTPUT_PATH} )				# exe, pdb

###########################
# Done
message ( STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}" )
message ( STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}" )
message ( STATUS "EXECUTABLE_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH}" )
message ( STATUS "ASSET_PATH: ${ASSET_PATH}")




