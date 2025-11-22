
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

## Example Usage

To see some example uses of Libmin, check out the Just Math repository:<br>
Just math - <a href="https://github.com/ramakarl/just_math">https://github.com/ramakarl/just_math</a><br>
All samples there make use of Libmin.<br>

## Building

Steps:
1) git clone https://github.com/ramakarl/libmin.git
2) Run ./build.sh
3) For more details/options, see <a href="https://github.com/ramakarl/just_math?tab=readme-ov-file#how-to-build">just_math, How to Build</a>

## Mains

Libmin provides main wrappers for multiple platforms including Windows, Linux and Android.<br>
Main wrappers are optional. You may write your own main.<br>
A main wrapper is used by calling the _REQUIRE_MAIN() function in your cmake.

## Third-party Libraries

Third party libs may be found in \libext.

These are not built into libmin.<br>
They are provided for convenience to applications that use libmin.<br>
<Br>
Convenience functions (called in application CMake):
- _REQUIRE_MAIN - request linkage to interactive cross-platform main
- _REQUIRE_GL - request linkage to OpenGL lib
- _REQUIRE_GLEW - request linkage to GLEW lib
- _REQUIRE_LIBEXT - request linkage to third-party libs, enables LIBEXT
- _REQUIRE_JPG - request linkage to libjpg 
- _REQUIRE_OPENSSL(default) - request linkage to libssl-dev
- _REQUIRE_BCRYPT(default) - request linkage to bcrypt
- _REQUIRE_CUDA(default) - request linkage to NV CUDA

## Application Cmakes 
Libmin was designed to be a very versatile library, providing application wrappers from interactive OpenGL apps, to GPU-based CUDA apps, to console-based networking apps.<br>
To achieve this, an application that uses libmin creates a CMakeLists.txt following this pseudo-code.<br>
For a detailed example see: https://github.com/ramakarl/Flock2/blob/main/CMakeLists.txt<br>

```
cmake_minimum_required(VERSION 2.8)
set (CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "")
set(PROJNAME _your_project_name_)
Project(${PROJNAME})

# LIBMIN Bootstrap - this section finds the libmin/cmakes
#
..
# Include LIBMIN - this section loads and links with libmin
#
find_package( Libmin QUIET )
..
# Options - this section specifies the linkage that the application desires
#
_REQUIRE_MAIN()  - optional (leave out for console apps)
_REQUIRE_GL()    - optional
_REQUIRE_GLEW()  - optional
_REQUIRE_CUDA(bool, ".")  - optional, for GPU-based apps
_REQUIRE_LIBEXT() - optional, load and link with additional 3rd party libs
_REQUIRE_OPENSSL(bool) - optional, links with openssl (from libext)
_REQUIRE_BCRYPT(bool) - optional, links with bcrypt (from libext)
..
# Asset Path - this section provides an ASSET_PATH variable to the 'assets' folder during compile
#
..
# App Code & Executable - this section finds the user-application cpp/h files
#
file( GLOB MAIN_FILES *.cpp *.c *.h )
..add_executable (..)
..
# Link Additional Libraries - this section links with all required & optional libs
#
_LINK ( PROJECT ${PROJNAME} OPT ${LIBRARIES_OPTIMIZED} DEBUG ${LIBRARIES_DEBUG} PLATFORM ${PLATFORM_LIBRARIES} )
..
# Install Binaries - this section builds the install, with dlls, shaders, ptx, exe & include as needed
#
file (COPY ..) - for assets folder
_INSTALL (FILES ${SHADERS} .. ) - for shaders
_INSTALL (FILES ${PACKAGE_DLLS} .. ) - for libext/libmin dlls
install (FILES $<TARGET_PDB_FILE:$PROJNAME}) .. ) -for pdb debug symbols
install (FIELS ${INSTALL_LIST} .. ) - for executable
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
