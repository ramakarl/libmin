//--------------------------------------------------
//
// GX: Low-level Rendering Library
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//
//--------------------------------------------------

#ifndef DEF_GX_PRINTER
    #define DEF_GX_PRINTER
    
    #include "gxlib.h"    
    #include "camera3d.h"
    #include "imageformat_tiff.h"

    using RenderFunc = void (*) (void*, RenderTarget&);

    namespace glib {

    struct GXPrinter : public CImageFormatTiff {
    public:
      GXPrinter();
      ~GXPrinter();

      void  Setup ( float width, float height, int ppi, RenderFunc func, void* context );
      void  SetRenderFunc(RenderFunc func, void* context) { m_renderFunc = func; m_context = context; }
      bool  Create ( int w, int h );
      Vec4F TileView ( int x, int y );

      void  CreateTIF ( std::string fname, int xres, int yres );
      void  WriteToTIF ( ImageX& img, int x, int y );
      void  CloseTIF ();

      bool  Print ( Vec4F fullview );

      void  Destroy ();

     
      RenderTarget  m_target;

      RenderFunc    m_renderFunc = nullptr;

      void*         m_context;

      GLuint        m_fbo;
      GLuint        m_color;
      GLuint        m_depth;

      std::string   m_fname;
      FILE*         m_tif;
      int           m_bpp;
      int           m_pix_start;
      ByteBuf	      m_buf;

    };

    };

#endif
