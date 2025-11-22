
## Libmin: A minimal utility library for computer graphics

### by [Rama Karl](http://ramakarl.com) (c) 2023. MIT license

Libmin stands for *minimal utility library* for computer graphics. Libmin combines multiple useful functions into a single library but with no external dependencies.
Projects with BSD and MIT Licenses have been merged into libmin. The full library is MIT Licensed.<br>

The functionality in Libmin includes:
- Vectors, 4x4 Matrices, Cameras (vec.h, camera.h)
- Quaternions (quaternion.h)
- Stateful Mersenee Twister RNG (mersenne.h)
- Event system (event.h, event_system.h)
- Network system, real-time non-blocking, event-based (network_system.h)
- Drawing in 2D/3D in OpenGL core profile (gxlib)
- 2D GUI Interface library (g2lib)
- Smart memory pointers for CPU/GPU via OpenGL/CUDA (dataptr.h)
- Image class. Multiple image loaders for png, tga, tif. (imagex.h)
- Mesh class. Handles large meshes, loads obj, also suppots .mtl files. (meshx.h)
- Font rendering, using a texture atlas.
- Directory listings (directory.h)
- Http connections (httlib.h)
- Time library, with nanosecond accuracy over millenia (timex.h)

## Projects using Libmin

- <a href="https://github.com/ramakarl/just_math">just_math</a> - Collection of pure math demos for computer graphics<br>
- <a href="https://github.com/ramakarl/flightsim">flightsim</a> - A simple, Single-Body Flight Simulator (SBFM)<br>
- <a href="https://github.com/ramakarl/flock2">flock2</a> - A Model for Orientation-based Social Flocking<br>
- <a href="https://github.com/quantasci/logrip">logrip</a> - Defend against AI crawlers & botgs with server log analysis<br>
- <a href="https://github.com/quantasci/ProjectiveDisplacement">ProjectiveDisplacement</a> - Projective Displacement Mapping for Raytraced Editable Surfaes<br>
- <a href="https://github.com/quantasci/shapes">SHAPES</a> - A lightweight, node-based renderer for dynamic and natural systems<br>
- <a href="https://github.com/ramakarl/invk">invk</a> - An Inverse Kinematics Library using Quaternions<br>

## Build & Examples

Libmin is not built directly.<br>
It was previously a static/shared library before 2024, and can be reconfigured as such,<br>
yet the new usage builds code files directly into other projects. This saves compile time,<br>
eliminates multiple build projects, and makes interactive debugging much easier.<br>

Look at <a href="https://github.com/ramakarl/just_math">just_math</a> for example cmake project builds. <br>
All samples there make use of Libmin.<br>

## Mains

Libmin provides main wrappers for multiple platforms including Windows, Linux and Android.<br>
Main wrappers are optional. You may write your own main.<br>
A main wrapper is used by calling the _REQUIRE_MAIN() function in your project cmake.

## Third-party Libraries

Third party libs may be found in \libext.

These are not built into libmin.<br>
They are provided for convenience to applications that use libmin.<br>
<Br>
Convenience functions (called in application CMake):
- _REQUIRE_3D - request inclusion of /src/3d folder
- _REQUIRE_IMAGE - request inclusion of /src/image folder
- _REQUIRE_DATAPTR - request inclusion of /src/dataptr folder
- _REQUIRE_G2LIB - request inclusion of /src/g2lib folder
- _REQUIRE_GXLIB - request inclusion of /src/g2lib folder
- _REQUIRE_NETWORK - request inclusion of /src/network folder
- _REQUIRE_MAIN - request linkage to interactive cross-platform main
- _REQUIRE_GL - request linkage to OpenGL lib
- _REQUIRE_GLEW - request linkage to GLEW lib
- _REQUIRE_LIBEXT - request linkage to third-party libs, enables LIBEXT
- _REQUIRE_JPG - request linkage to libjpg 
- _REQUIRE_OPENSSL(default) - request linkage to libssl-dev
- _REQUIRE_BCRYPT(default) - request linkage to bcrypt
- _REQUIRE_CUDA(default) - request linkage to NV CUDA, with automatic .cu to PTX compilation

## Application Cmakes 
Libmin was designed as a versatile library, providing application wrappers that range from<br>
interactive OpenGL apps, to GPU-based CUDA apps, to console-based networking apps.<br>

An application that uses libmin creates a CMakeLists.txt.<br>
For a detailed example see: https://github.com/ramakarl/Flock2/blob/main/CMakeLists.txt<br>

The following is a simplified, general example:<br>
```
cmake_minimum_required(VERSION 2.8...3.5)
set (CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "")
set(PROJNAME _your_project_name_)
Project(${PROJNAME})

################
# LIBMIN Bootstrap - this section finds the libmin/cmakes
set ( LIBMIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../libmin" CACHE STRING "Location of Libmin")
find_package( Libmin QUIET )
..
################
# Options - this section specifies the code & linkage that the application desires
_REQUIRE_DATAPTR()
_REQUIRE_IMAGE()
_REQUIRE_3D()
_REQUIRE_GXLIB()
_REQUIRE_MAIN()
_REQUIRE_GL()
_REQUIRE_GLEW()
..
################
# Executable
file(GLOB MAIN_FILES *.cpp *.c *.h )
_GET_GLOBALS()
add_executable (${PROJNAME} ${MAIN_FILES} ${CUDA_FILES} ${PACKAGE_SOURCE_FILES} ${LIBMIN_FILES})
_LINK ( PROJECT ${PROJNAME} OPT ${LIBS_OPTIMIZED} DEBUG ${LIBS_DEBUG} PLATFORM ${LIBS_PLATFORM} )
..
################
# Install Binary
install ( FILES ${INSTALL_LIST} DESTINATION .)				# EXE
```

## License

Libmin is MIT Licensed with contributions from other BSD and MIT licensed sources.<br>
Individual portions of libmin are listed here with their original licensing.<br>
<br>
Copyright listing for Libmin:<br>
Copyright (c) 2007-2022, Quanta Sciences, Rama Hoetzlein. MIT License (image, dataptr, events, gxlib)<br>
Copyright (c) 2017 NVIDIA GVDB, by Rama Hoetzlein. BSD License (camera3d, tga, str_helper, vec, mains)<br>
Copyright (c) 2020 Yuji Hirose. MIT License (httplib.h)<br>
Copyright (c) 2005-2013 Lode Vandevenne. BSD License (LodePNG, file_png)<br>
Copyright (c) 2015-2017 Christian Stigen Larsen. BSD License (mersenne)<br>
Copyright (c) 2002-2012 Nikolaus Gebhardt, Irrlicht Engine. BSD License (quaternion)<br>
<br>
Derivative changes to Libmin may append this copyright listing but should not modify it.<br>

Contact: Rama Hoetzlein at ramahoetzlein@gmail.com
