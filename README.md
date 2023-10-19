
## Libmin: A minimal utility library for computer graphics

### by Rama Karl (c) 2023. MIT license

Libmin stands for *minimal utility library* for computer graphics. Libmin combines multiple useful functions into a single library, but with no external dependencies.
Projects with BSD and MIT Licenses have been merged into libmin. The full library is MIT Licensed. Copyrights may be appended but should not be modified or removed.
The functionality in Libmin includes:
- Vectors, 4x4 Matrices, Cameras (vec.h, camera.h)
- Quaternions (quaternion.h)
- Stateful Mersenee Twister RNG (mersenne.h)
- Event system (event.h, event_system.h)
- Drawing in 2D/3D in OpenGL core profile (nv_gui.h)
- Smart memory pointers for CPU/GPU via OpenGL/CUDA (dataptr.h)
- Image class, with multiple image loaders (image.h, png/tga/jpg/tiff)
- Directory listings (directory.h)
- Http connections (httlib.h)
- Time library, with nanosecond accuracy over millenia (timex.h)
- Widget library, very basic GUI (widget.h)

## Third-party Libraries

Third party libs may be found in \libext.

These are not built into libmin.
They are provided for convenience to applications that use libmin.
For example, the _REQUIRED_JPG cmake directive allows an application to request linkage to libjpg dynamic libs. 



