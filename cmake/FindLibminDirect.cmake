
####################################################################################
# LIBMIN DIRECT
#
# This cmake provides Libmin directly as compiled files.
# Does not search for libmin binaries or libext.
# Application cmake can use this as:
#      find_package ( LibminDirect )
#

function(_CONFIRM_PATH outvar targetPath targetFile varName )
  set(oneValueArgs outVar targetPath targetFile varName )  
  unset ( result CACHE )  
  find_file ( result ${targetFile} PATHS ${targetPath} )  
  if ( ${result} STREQUAL "result-NOTFOUND" )
    set (${outvar} "NOTFOUND" CACHE PATH "" )
    set (found "NOTFOUND")
  else()
    set (${outvar} ${result} CACHE PATH "" )
    set (found "OK")
  endif()   
  message ( STATUS "  Confirming: ${targetFile} in ${targetPath} -> ${found}")
  if ( ${found} STREQUAL "NOTFOUND" )
    message ( FATAL_ERROR "\n  Cannot find ${varName} at ${targetPath}.
       Please set ${varName} to the correct path, which should contain
       files such as ${targetFile}.\n"   
    )
  endif()
  unset ( result CACHE )
endfunction()

# LIBMIN_ROOT - should be set by caller during bootstrap
#
message ( STATUS "  LIBMIN_ROOT: ${LIBMIN_ROOT}")
if ( NOT DEFINED LIBMIN_ROOT )
  set ( LIBMIN_ROOT "${LIBMIN_ROOT}" CACHE PATH "Path to /libmin source" )
endif()
_CONFIRM_PATH ( LIBMIN_ROOT "${LIBMIN_ROOT}" "/src/dataptr.cpp" "LIBMIN_ROOT" )

# Repository paths
set ( LIBMIN_ROOT_MAINS "${LIBMIN_ROOT}/mains" )
set ( LIBMIN_ROOT_SRC "${LIBMIN_ROOT}/src")
set ( LIBMIN_ROOT_INC "${LIBMIN_ROOT}/include")
set ( LIBEXT_ROOT ${LIBEXT_ROOT} )
message ( STATUS "  Found paths..")
message ( STATUS "  LIBMIN_ROOT_MAINS: ${LIBMIN_ROOT_MAINS}")
message ( STATUS "  LIBMIN_ROOT_SRC: ${LIBMIN_ROOT_SRC}")
message ( STATUS "  LIBMIN_ROOT_INC: ${LIBMIN_ROOT_INC}")

#set ( DEBUG_HEAP false CACHE BOOL "Enable heap checking (debug or release).")
set ( DEBUG_HEAP true CACHE BOOL "Enable heap checking (debug or release).")
if ( ${DEBUG_HEAP} )
   add_definitions( -DDEBUG_HEAP)
   add_definitions( -D_CRTDBG_MAP_ALLOC)
endif()

message ( STATUS "----- Completed LibminDirect.cmake" )
message ( STATUS "  CURRENT DIRECTORY:  ${CMAKE_CURRENT_SOURCE_DIR}" )
message ( STATUS "  LIBMIN SOURCE CODE: ${LIBMIN_ROOT}" )

set ( LIBMIN_FOUND TRUE CACHE BOOL "" FORCE)

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

###################################################################################
# Include GLEW
#
function ( _REQUIRE_GLEW)    
    OPTION (BUILD_GLEW "Build with GLEW" ON)
    if (BUILD_GLEW)
      include_directories("${LIBEXT_PATH}/include")
      if (WIN32)        
	    LIST(APPEND LIBRARIES_OPTIMIZED "${LIBEXT_PATH}/win64/glew64.lib" )
	    LIST(APPEND LIBRARIES_DEBUG "${LIBEXT_PATH}/win64/glew64d.lib" )
        set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
        set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
        LIST(APPEND GLEW_FILES "${LIBEXT_PATH}/win64/glew64.dll" )
        LIST(APPEND GLEW_FILES "${LIBEXT_PATH}/win64/glew64d.dll" )
        set(GLEW_FILES ${GLEW_FILES} PARENT_SCOPE)
      endif()
      message ( STATUS "  ---> Using GLEW (dll)" )        
    endif()
endfunction()

###################################################################################
# Include OpenGL ES 3.0
#
function ( _REQUIRE_GLES )
    OPTION (BUILD_OPENGL "Build with OpenGL ES" ON)    
    if (BUILD_OPENGL)                
        message ( STATUS "  Linking with GLESv3.." )
        add_definitions (-DUSE_OPENGL)
        set(GLES_VER "GLESv3" PARENT_SCOPE)    
    endif()
endfunction()

#####################################################################################
# Include JPG    
#   
function ( _REQUIRE_JPG )   
     set ( OK_H "0" )    
     set ( _jpg_srch "${LIBEXT_PATH}/win64" )
	_FIND_FILE ( JPG_LIBS _jpg_srch "libjpg_2019x64.lib" "" OK_H )		
	if ( OK_H EQUAL 1 )
      add_definitions(-DBUILD_JPG) 
      list ( APPEND LIBRARIES_OPTIMIZED "${LIBEXT_PATH}/win64/libjpg_2019x64.lib" )
      list ( APPEND LIBRARIES_DEBUG "${LIBEXT_PATH}/win64/libjpg_2019x64d.lib" )
      set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
      set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
      include_directories( "${LIBEXT_PATH}/include" )
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
function ( _REQUIRE_CUDA use_cuda_default kernel_path) 
    
    OPTION (BUILD_CUDA "Build with CUDA" ${use_cuda_default})

    if (BUILD_CUDA) 
        if (NOT CUDA_TOOLKIT_ROOT_DIR) 
	        set ( CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.2" CACHE PATH "CUDA Toolkit path")
        endif()        
        message( STATUS "  Searching for CUDA..") 
        find_package(CUDA)
        if ( CUDA_FOUND )
            ##########################################
            # Link CUDA
            #
	        message( STATUS "  ---> Using package CUDA (ver ${CUDA_VERSION})") 
            add_definitions(-DBUILD_CUDA)    
	        add_definitions(-DUSE_CUDA)    
	        include_directories(${CUDA_TOOLKIT_INCLUDE})
	        LIST(APPEND LIBRARIES_OPTIMIZED ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} )
	        LIST(APPEND LIBRARIES_DEBUG ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} )
	        LIST(APPEND PACKAGE_SOURCE_FILES ${CUDA_TOOLKIT_INCLUDE} )    
            set(LIBRARIES_OPTIMIZED ${LIBRARIES_OPTIMIZED} PARENT_SCOPE)
            set(LIBRARIES_DEBUG ${LIBRARIES_DEBUG} PARENT_SCOPE)
            set(PACKAGE_SOURCE_FILES ${PACKAGE_SOURCE_FILES} PARENT_SCOPE)
	        source_group(CUDA FILES ${CUDA_TOOLKIT_INCLUDE} ) 	        

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
	            _COMPILEPTX ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${CMAKE_CURRENT_SOURCE_DIR},${LIBMIN_INC_DIR},${ADDITIONAL_CUDA_INCLUDES}" OPTIONS --ptx -arch=${CUDA_ARCH} -code=${CUDA_CODE} -dc -Xptxas -v -G -g --use_fast_math --maxrregcount=32 )
            else()
	            _COMPILEPTX ( SOURCES ${CUDA_FILES} TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR} GENERATED CUDA_PTX GENPATHS CUDA_PTX_PATHS INCLUDE "${CMAKE_CURRENT_SOURCE_DIR},${LIBMIN_INC_DIR},${ADDITIONAL_CUDA_INCLUDES}" OPTIONS --ptx -arch=${CUDA_ARCH} -code=${CUDA_CODE} -dc -Xptxas -v -O3 --use_fast_math --maxrregcount=32 )
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
        message ( "  NOTE: Set BUILD_CUDA to enable GPU. May need to set for libmin also.")
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
		file ( MAKE_DIRECTORY "${_COMPILEPTX_TARGET_PATH}/Debug" )
		file ( MAKE_DIRECTORY "${_COMPILEPTX_TARGET_PATH}/Release" )
		string (REPLACE ";" " " _COMPILEPTX_OPTIONS "${_COMPILEPTX_OPTIONS}")  
		separate_arguments( _OPTS WINDOWS_COMMAND "${_COMPILEPTX_OPTIONS}" )
		message ( STATUS "NVCC Options: ${_COMPILEPTX_OPTIONS}" )  
		message ( STATUS "NVCC Include: ${_COMPILEPTX_INCLUDE}" )

        set ( INCL "-I\"${_COMPILEPTX_INCLUDE}\"" )

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
    
				message( STATUS "NVCC Compile: ${CUDA_NVCC_EXECUTABLE} ${MACHINE} --ptx ${_COMPILEPTX_OPTIONS} ${input} ${INCL} -o ${output_with_path} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}")
    
				add_custom_command(
					OUTPUT  ${output_with_path}
					MAIN_DEPENDENCY ${input}
					COMMAND ${CUDA_NVCC_EXECUTABLE} ${MACHINE} --ptx ${_OPTS} ${input} ${INCL} -o ${output_with_quote} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
				)			
			endif()
		ENDFOREACH( )
  else ()
		# Linux - PTX compile
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

#------------------------------------ CROSS-PLATFORM INSTALL
function( _INSTALL )   
  set (options "")
  set (oneValueArgs DESTINATION SOURCE OUTPUT )
  set (multiValueArgs FILES )
  CMAKE_PARSE_ARGUMENTS(_INSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if (_INSTALL_SOURCE)      
     set ( _INSTALL_SOURCE "${_INSTALL_SOURCE}/" )	  
  endif()
  set ( OUT_LIST ${${_INSTALL_OUTPUT}} )

  if ( WIN32 )      
      # Windows - copy to desintation at post-build
      file ( MAKE_DIRECTORY "${_INSTALL_DESTINATION}/" )
      foreach (_file ${_INSTALL_FILES} )	
          get_filename_component ( _path "${_file}" DIRECTORY )               
          if ( "${_path}" STREQUAL "" )
		   
            set ( _fullpath "${_INSTALL_SOURCE}${_file}")            
          else ()
            set ( _fullpath "${_file}" )            
          endif()
          message ( STATUS "Install: ${_fullpath} -> ${_INSTALL_DESTINATION}" )
          add_custom_command(
             TARGET ${PROJNAME} POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy ${_fullpath} ${_INSTALL_DESTINATION}
          )                   
 	      list ( APPEND OUT_LIST "${_fullpath}" )
      endforeach()    
  else ()
      # Linux 
      if ( _INSTALL_SOURCE )	   
	    foreach ( _file ${_INSTALL_FILES} )
             list ( APPEND OUT_LIST "${_INSTALL_SOURCE}${_file}" )
        endforeach()
      else()
	     list ( APPEND OUT_LIST ${_INSTALL_FILES} )
      endif() 
  endif( )
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
      add_custom_command ( TARGET ${PROJNAME} PRE_LINK
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

	foreach (loop_var IN ITEMS ${_LINK_PLATFORM} )
		target_link_libraries( ${PROJ_NAME} general ${loop_var} )	
		list (APPEND LIBLIST ${loop_var})
	endforeach() 	

	foreach (loop_var IN ITEMS ${_LINK_DEBUG} )
		target_link_libraries ( ${PROJ_NAME} debug ${loop_var} )
		list (APPEND LIBLIST ${loop_var})
	endforeach()
	
	foreach (loop_var IN ITEMS ${_LINK_OPT} )   
		target_link_libraries ( ${PROJ_NAME} optimized ${loop_var} )
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
