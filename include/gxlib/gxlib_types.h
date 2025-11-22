//--------------------------------------------------
//
// GX: 2D Interface Library
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//--------------------------------------------------


#ifndef DEF_GX_DEFS
    #define DEF_GX_DEFS
    
    #include "vec.h"
    #include "common_defs.h"
    
    // Primitive types
    #define PRIM_NONE       0
    #define PRIM_POINTS     1
    #define PRIM_LINES      2    
    #define PRIM_TRI        3
    #define PRIM_IMGS       4

    // Shader IDs
	  #define S2D 	        2		// Color shader (2D)
    #define S3D 	        3		// 3D shader (Phong)
	  #define SINST	        4		// Instance shader
	  #define SPNT	        5		// Point shader (for compact point VBOs)
	  #define S_MAX	        6

    // Shader Params
    #define SP_MODEL    0
    #define SP_PROJ     1
    #define SP_VIEW     2
    #define SP_TEX      3
    #define SP_CAMPOS   4
    #define SP_LIGHTPOS 5
    #define SP_LIGHTCLR 6
    #define SP_AMBCLR   7
    #define SP_DIFFCLR  8
    #define SP_SPECPOW  9
    #define SP_SPECCLR  10    
    #define SP_ENVMAP   11
    #define SP_MAX      12

    // GLSL slots
    #define slotPos 	0
	  #define	slotClr	    1 
    #define slotUVs		2
	  #define slotNorm	3

    // GX img types
    #define IMG_NULL	    0xFFFFFFFF
	  #define IMG_RGB			0
	  #define IMG_RGBA		1
	  #define IMG_GREY16		2	

    #define NO_NORM		-111

    // GX Types
    //
    typedef	int					BUF;
    typedef	int					TEX;    

    struct gxVert {             // total: 24       		
        float x, y, z;			// 12 
        uint32_t clr;           // 4
        float tx, ty;           // 8
    };
    struct gxVert3D {           // total: 36  		      
        float x, y, z;			// 12		
        uint32_t clr;           // 4
        float tx, ty;           // 8
		float nx, ny, nz;       // 12        
    };
    struct gxPrim {             // total: 14
        uchar       typ;        // 1
        uchar       prim;       // 1        
        uint32_t    cnt;        // 4
        uint64_t    img_ptr;    // 8 bytes
    };

    struct gxSet {
        char        stype;          // set type, '2'=2D, '3'=3D
        bool        sstatic;        // is set static
        Vec4F       region;         // window region (clipping)
        Vec4F       view;           // 2D world view
        int64_t     lastpos;        // marker pos
        int64_t     size, max;      // buffer size        
        char*       geom;           // CPU buffer
        BUF         vbo;            // GPU buffer
        int64_t     bufsize;
        int         calls, primcnt;
        float       mem;

        float       model_mtx[16];
        float       view_mtx[16];
        float       proj_mtx[16];
        // float       aspect_correct;
        float       text_aspect;
        Vec3F       lightpos, lightclr;
        Vec3F       diffclr, ambclr, specclr;
        Vec3F       cam_from, cam_to;
        float       specpow;
        int         envmap;
    };
    
    // GX Fonts
    // See: https://freetype.org/freetype2/docs/glyphs/glyphs-3.html
    //
    struct gxGlyph {            //-- Glyph definition, single letter/symbol
		int width, height;      // glyph width & hgt (pixels)
		int advance;            // glyph x-advance (pixels)
		int offX, offY;         // glyph starting pixel offset		
        float u, v;		        // glyph position in icon sheet (normalized coords)
		float du, dv;           // glyph size in icon sheet
    };

    struct gxFont {             //-- Font definition
		int ascent;             // glyph ascent hgt (pixels)
		int descent;            // glyph descent hgt (pixels)
		int linegap;            // glyph line gap (pixels)		
		gxGlyph glyphs[256];    // set of glyphs
	};

    // GXLIB Linkage
    #if !defined ( LIBMIN_STATIC )
        #if defined ( LIBMIN_EXPORTS )				// inside DLL
            #if defined(_WIN32) || defined(__CYGWIN__)
                #define GXAPI		__declspec(dllexport)
            #else
                #define GXAPI		__attribute__((visibility("default")))
            #endif
        #else										// outside DLL
            #if defined(_WIN32) || defined(__CYGWIN__)
                #define GXAPI		__declspec(dllimport)
            #else
                #define GXAPI		//https://stackoverflow.com/questions/2164827/explicitly-exporting-shared-library-functions-in-linux
            #endif
        #endif          
    #else
        #define GXAPI
    #endif       
   
#endif
