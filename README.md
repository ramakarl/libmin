
## Libmin: A minimal utility library for computer graphics

### by [Rama Karl](http://ramakarl.com) (c) 2023. MIT license

Libmin stands for *minimal utility library* for computer graphics. Libmin combines multiple useful functions into a single library but with no external dependencies.
Projects with BSD and MIT Licenses have been merged into libmin. The full library is MIT Licensed.<br>

The functionality in Libmin includes:
- Vectors, 4x4 Matrices, Cameras (vec.h, camera.h)
- Quaternions (quaternion.h)
- Stateful Mersenee Twister RNG (mersenne.h)
- Event system (event.h, event_system.h)
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

## Mains

Libmin provides main wrappers for multiple platforms including Windows, Linux and Android.<br>
Main wrappers are optional. You may write your own main.<br>
A main wrapper is used by calling the _REQUIRE_MAIN() function in your cmake.

## Third-party Libraries

Third party libs may be found in \libext.

These are not built into libmin.<br>
They are provided for convenience to applications that use libmin.<br>
For example, the _REQUIRED_JPG() cmake function allows an application to request linkage to libjpg dynamic libs. <br>

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
