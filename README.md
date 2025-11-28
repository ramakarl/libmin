
<img width=60% src="https://github.com/ramakarl/libmin/blob/main/libmin_design.png" />

## Libmin: A minimal utility library for computer graphics

### by [Rama Karl](http://ramakarl.com) (c) 2023. MIT license

Libmin stands for *minimal utility library* for computer graphics. Libmin combines multiple useful functions into a single direct-compile framework with no external dependencies.
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

Libmin is not built directly. <br>
To build an application, place the app and Libmin repo as sibling folders.<br>
<pre>
\codes
 ├── \application
 └── \libmin
</pre>
The application has a Cmake that bootstraps and requests Libmin to help with direct-compiled code, third party libs, build and linkage.
Before 2024, Libmin was a static/shared library and can be reconfigured as such, yet the new design builds code files directly into your application.

For a simple, complete example see Flock2 cmake: https://github.com/ramakarl/Flock2/blob/main/CMakeLists.txt<br>
See also <a href="https://github.com/ramakarl/just_math">just_math</a>, as all samples there use Libmin.<br>

## Design

Libmin was designed as a versatile, non-intrusive library to provide cross-platform mains, support code, libraries and wrappers<br>
for applications that range from interactive OpenGL apps, to GPU-based CUDA apps, to console-based networking apps.

To support many application types non-intrusively, static/shared libraries were found to be insufficient as they make too 
many assumptions regarding what is necessary to include and leading to bloated shared libs and difficulty versioning across many apps.
The primary reasons for static/shared libraries is to provide a feature barrier and to support many applications 
without duplicating compilation. Libmin is open source and lightweight enough that both are unnecessary and add complexity. 
Direct code compilation is simpler, allows for selective inclusion, is faster to compile, and is easier to debug. 
Yet how to provide code modularity with direct inclusion? Optional convenience macros solve this. Macro functions take care
of code inclusions (vec, image, mesh, network), of additional libs (OpenGL, OpenSSL, CUDA, FFTW), of linkage (_LINK), and install steps (install_files, install_ptx).
Think of Libmin as Cmake the way you wanted it to work.

Applications can follow the typical Cmake workflow. If you include nothing, it is no different that a blank slate C/C++ project built with Cmake.
Libmin provides convenience macros that automatically compile & link in additional source code and third party libs.
When fully utilized, the result is a simplified project Cmake that simply requests what it needs, links and builds.

## Usage
An application that uses Libmin create a new CMakeLists.txt and then follows these steps:<br>
1. Bootstrap - Create a Cmake that loads the Libmin Bootstrap to find the Libmin packaged cmakes.<br>
2. Options - Convenience functions are called to request code include & library linkages [optional].<br>
3. Executable - The application executable/library is created as usual using add_executable or add_library.<br>
4. Link - The _LINK macro is invoked to simplify linkage [optional].<br>
5. Install - Install is done as usual. Additional helper macros install_ptx and install_files are provided to simplify multi-file installs [optional].<br>

These steps follow the usual Cmake workflow and all Libmin steps are optional. <br>
Full use of Libmin provides a convenient way to build in vectors, images, meshes, events, networking, OpenSSL, OpenGL, CUDA, cross-platform mains, etc. for a variety of applications types.<br>

For a simple, complete example see: https://github.com/ramakarl/Flock2/blob/main/CMakeLists.txt<br>

The following is a generalized example:<br>
```
cmake_minimum_required(VERSION 2.8...3.5)
set (CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "")
set(PROJNAME _your_project_name_)
Project(${PROJNAME})
################
# 1. LIBMIN Bootstrap - this section finds the libmin/cmakes
set ( LIBMIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../libmin" CACHE STRING "Location of Libmin")
find_package( Libmin QUIET )
..
################
# 2. Options - this section specifies the code & linkage that the application desires
_REQUIRE_DATAPTR()
_REQUIRE_IMAGE()
_REQUIRE_3D()
_REQUIRE_GXLIB()
_REQUIRE_MAIN()
_REQUIRE_GL()
_REQUIRE_GLEW()
..
################
# 3. Executable
file(GLOB MAIN_FILES *.cpp *.c *.h )
_GET_GLOBALS()
add_executable (${PROJNAME} ${MAIN_FILES} ${CUDA_FILES} ${PACKAGE_SOURCE_FILES} ${LIBMIN_FILES})
# 4. Linkage
_LINK ( PROJECT ${PROJNAME} OPT ${LIBS_OPTIMIZED} DEBUG ${LIBS_DEBUG} PLATFORM ${LIBS_PLATFORM} )
..
################
# 5. Install Binary
install ( FILES ${INSTALL_LIST} DESTINATION .)				# EXE
```

## Libmin Code
Libmin source code, found in this repo, is direct-compiled into an application code.
These are separated by folder in the Libmin repo, and requested using convenience macros:<br>
_REQUIRE_3D - request inclusion of /src/3d folder<br>
_REQUIRE_IMAGE - request inclusion of /src/image folder<br>
_REQUIRE_DATAPTR - request inclusion of /src/dataptr folder<br>
_REQUIRE_G2LIB - request inclusion of /src/g2lib folder<br>
_REQUIRE_GXLIB - request inclusion of /src/gxlib folder<br>
_REQUIRE_NETWORK - request inclusion of /src/network folder<br>
_REQUIRE_MAIN - request linkage to interactive cross-platform main<br>

## Mains

Libmin provides main wrappers for multiple platforms including Windows, Linux and Android.<br>
Main wrappers are optional. You may write your own main.<br>
A main wrapper is used by calling the _REQUIRE_MAIN() function in your project cmake.

## Third-party Libraries
Third party libraries are not included by default.<br>
They are provided on-demand with convenience macros:<br>
_REQUIRE_GL - request linkage to OpenGL lib<br>
_REQUIRE_GLEW - request linkage to GLEW lib<br>
_REQUIRE_JPG - request linkage to libjpg <br>
_REQUIRE_EXT - request linkage to additional libraries bundled in libext<br>
_REQUIRE_OPENSSL(default) - request linkage to libssl-dev<br>
_REQUIRE_BCRYPT(default) - request linkage to bcrypt<br>
_REQUIRE_CUDA(default) - request linkage to NV CUDA, with automatic .cu to PTX compilation<br>

## Direct Contributions
Thanks to:<br>
Mark Zifchock - for early versions of the Android cross-platform pathway<br>
Marcus Pieska - for the OpenSSL stack over the TCP/IP Network layer<br>

## License
Libmin is MIT Licensed with contributions from other BSD and MIT licensed sources.<br>
Individual portions of libmin are listed here with their original licensing.<br>
Copyright listing for Libmin:<br>
Copyright (c) 2007-2022, Quanta Sciences, Rama Hoetzlein. MIT License (image, dataptr, events, gxlib)<br>
Copyright (c) 2017 NVIDIA GVDB, by Rama Hoetzlein. BSD License (camera3d, tga, str_helper, vec, mains)<br>
Copyright (c) 2020 Yuji Hirose. MIT License (httplib.h)<br>
Copyright (c) 2005-2013 Lode Vandevenne. BSD License (LodePNG, file_png)<br>
Copyright (c) 2015-2017 Christian Stigen Larsen. BSD License (mersenne)<br>
Copyright (c) 2002-2012 Nikolaus Gebhardt, Irrlicht Engine. BSD License (quaternion)<br>

## Contact
Feel free to contact me if you have any questions, comments or suggestions:<br>
**Rama Hoetzlein** <br>
Website: <a href="https://ramakarl.com">ramakarl.com</a><br>
Email: ramahoetzlein@gmail.com<br>
