//--------------------------------------------------
//
// GX: Low-level Rendering Library
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//
// ver 1.0. 9/9/2023
// GX is a greatly improved 2D drawing library for OpenGL 3.0 core profile.
// - supports OpenGL ES, GL3.0 core profile for mobile & desktop
// - correct drawing overlap regardless of call order (not tied to sets)
// - stream oriented. draw functions in any order.
// - call optimized. multiple similar calls are automatically batched into 1 draw call.
// - single buffer. only one GPU buffer transfer for all draw calls.
// - memory efficient transfers for dynamic draw. vertices are compact 24 bytes.
// - shader efficient. 2D fragment shader is just: outclr = imgclr * vertclr
// - supports drawing of many images.
// - profiling. debug mode shows # draw calls, vert count, and memory usage.
// - easy to develop. draw functions declared only once (unlike nv_gui).
// - useful draw functions. lines, boxes, fills, text, cirlces, etc.
//
//--------------------------------------------------


#ifndef DEF_GX
    #define DEF_GX
    
    #include "gxlib_types.h"
    #include "imagex.h"
    #include "camera3d.h"

    // GX Library
    namespace glib {

    // 2D Interface Functions
    GXAPI bool init2D ( const char* fontName );    
    GXAPI void debug2D ( bool tf );    
    GXAPI void clear2D ();
    GXAPI void start2D ( int w, int h, bool bStatic = false );
    GXAPI void setview2D ( int w, int h );
    GXAPI void setMatrices2D ( int grp, int xr, int yr );    
    GXAPI void setTextSz ( float hgt, float kern );
    GXAPI void end2D ();    
    GXAPI void drawLine ( Vec2F a, Vec2F b, Vec4F clr );
    GXAPI void drawRect ( Vec2F a, Vec2F b, Vec4F clr );
    GXAPI void drawFill ( Vec2F a, Vec2F b, Vec4F clr );
    GXAPI void drawGradient ( Vec2F a, Vec2F b, Vec4F c0, Vec4F c1, Vec4F c2, Vec4F c3 );
    GXAPI void drawCircle ( Vec2F a, float r, Vec4F clr  );
    GXAPI void drawText ( Vec2F a, char* msg, Vec4F clr );    
    GXAPI void drawImg ( ImageX* img, Vec2F a, Vec2F b, Vec4F clr );
    
    // 3D Interface
    GXAPI void start3D ( Camera3D* cam, bool bStatic = false );
    GXAPI void setMaterial ( Vec3F Ka, Vec3F Kd, Vec3F Ks, float Ns, float Tf);
    GXAPI void setLight3D ( Vec3F pos, Vec4F clr );
    GXAPI void setView3D ( Camera3D* cam );
    GXAPI void setEnvmap3D ( ImageX* img );
    GXAPI void end3D ();    
    GXAPI void drawLine3D ( Vec3F a, Vec3F b, Vec4F clr );
    GXAPI void drawLineDotted3D ( Vec3F a, Vec3F b, Vec4F clr, int segs=10 );
    GXAPI void drawCircle3D ( Vec3F p1, Vec3F p2, float r, Vec4F clr  );
    GXAPI void drawBox3D (Vec3F p, Vec3F q, Vec4F clr );
    GXAPI void drawBox3D (Vec3F p, Vec3F q, Vec4F clr, Matrix4F& xform );
    GXAPI void drawBoxDotted3D (Vec3F p, Vec3F q, Vec4F clr, int segs=10 );
    GXAPI void drawTri3D( Vec3F a, Vec3F b, Vec3F c, Vec3F n, Vec4F clr, bool solid=false);
    GXAPI void drawFace3D( Vec3F a, Vec3F b, Vec3F c, Vec3F d, Vec3F n, Vec4F clr, bool solid=false);
    GXAPI void drawSphere3D (Vec3F p, float r, Vec4F clr, bool solid=true);    
    GXAPI void selfDraw3D ( Camera3D* cam );
    GXAPI void selfSetLight3D ( Vec3F pos, Vec4F clr );
    GXAPI void selfSetTexture ( int glid=-1 );
    GXAPI void selfSetMaterial ( Vec3F Ka, Vec3F Kd, Vec3F Ks, float Ns, float Tf);
    GXAPI void selfSetModelMtx ( Matrix4F& mtx );
    GXAPI void selfEndDraw3D ();
    
    GXAPI void drawAll ();  



    class GXAPI gxLib {
    public:
        gxLib ();

        // initialization
        void        createShader2D ();
        void        createShader3D ();

        // building geometry
        gxSet*      getCurrSet ()         {return &m_set[m_curr_grp];}   
        gxSet*      getSet (int grp)      {return &m_set[grp];}
        void        clearSet (int grp);
        void        expandSet ( gxSet* s, uchar typ, uchar prim, int64_t add_bytes );
        void        attachPrim ( gxSet* s, uchar typ, uchar prim );
        void        finishPrim ( gxSet* s );
        gxVert*     allocGeom2D (int vert_cnt, uchar prim );
        gxVert*     allocImg2D (int vert_cnt, uchar prim, ImageX* font_img );
        gxVert3D*   allocGeom3D (int vert_cnt, uchar prim );        
        void        updateVBO ( gxSet* s );

        // fonts
        bool        loadFont ( const char * fontName );
        gxFont&     getCurrFont ()      { return m_font; }

        // rendering 
        void        drawSet ( int g );
        
        // member vars
        int         m_Xres, m_Yres;
        Vec4F   m_view2D;
        bool        m_debug;

        // primitive sets
        gxSet       m_set[GRP_MAX];
        int         m_curr_grp;                 // group: S_STAT2, S_DYN2, S_STAT3, S_DYN3
        uchar       m_curr_type;                // types: 'I' (Image), 'i' (gxImg/font), 'x' (none)        
        uchar       m_curr_prim;                // prims: PRIM_POINTS, PRIM_LINES, PRIM_TRI, PRIM_IMGS
        uint64_t    m_curr_img;                 
        int         m_curr_num;

        // text drawing
        float		m_text_hgt, m_text_kern;
        gxFont      m_font;
        ImageX      m_font_img, m_white_img;

        // opengl
        GLuint		mSH[S_MAX];					// shaders
		GLuint		mPARAM[S_MAX][SP_MAX];      // shader params
        GLuint      mVAO;                       // VAO

        // sin/cos tables
        float       cos_table[36000];
        float       sin_table[36000];

    };
    
    // Global singleton
    extern GXAPI glib::gxLib   gx;

    };
    


#endif
