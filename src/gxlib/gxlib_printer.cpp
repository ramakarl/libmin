

#include <assert.h>

#include "gxlib_printer.h"

using namespace glib;


GXPrinter::GXPrinter()
{
  m_tif = 0x0;
}

GXPrinter::~GXPrinter()
{
  Destroy();
}

void GXPrinter::Destroy()
{
    if (m_depth) glDeleteRenderbuffers(1, &m_depth);
    if (m_color) glDeleteTextures(1, &m_color);
    if (m_fbo)   glDeleteFramebuffers(1, &m_fbo);

    m_depth = m_color = m_fbo = 0;
    m_target.tileres = Vec2I(0,0);
}

void GXPrinter::Setup ( float width, float height, int ppi, RenderFunc func, void* context )
{
  SetRenderFunc ( func, context );  

  m_target.outSz = Vec4F(0,0, width, height);
  m_target.ppi = ppi;
  m_target.tileres = Vec2I(4096, 4096);
  m_target.printing = true; 
}


bool GXPrinter::Create (int w, int h)
{
  Destroy();

  m_target.tileres.x = w;
  m_target.tileres.y = h;

  // Frame buffer object
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  checkGL ( "GXPrinter::frame_buffer", true );

  // Color target
  glGenTextures(1, &m_color);
  glBindTexture(GL_TEXTURE_2D, m_color);
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                GL_TEXTURE_2D, m_color, 0 );
  checkGL ( "GXPrinter::color_target", true );

  // Depth target
  glGenRenderbuffers(1, &m_depth);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
  glRenderbufferStorage( GL_RENDERBUFFER,  GL_DEPTH_COMPONENT24, w, h);
  glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER, m_depth );
  checkGL ( "GXPrinter::depth_target", true );

  GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, drawBuffers);
  checkGL ( "GXPrinter::draw_buffers", true );

  bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  if (!ok) {
    printf ( "ERROR: Creating frame buffers for printing.\n");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return ok;
}

// Tile 2D view
// view   - world space view to tile
// fw/fh  - 

Vec4F GXPrinter::TileView ( int x, int y ) 
{
  Vec4F& f = m_target.fullview;
  Vec2I cnt = m_target.tilecnt;
  
  float dx = float(f.z - f.x) / cnt.x;
  float dy = float(f.w - f.y) / cnt.y;

  m_target.subtile = Vec2I(x,y);  
  m_target.view = Vec4F( f.x + (x+0) * dx, f.w - (y+1) * dy, 
                         f.x + (x+1) * dx, f.w - (y+0) * dy);

  return m_target.view;
}

void GXPrinter::CreateTIF ( std::string fname, int xres, int yres )
{
  int pos;

  m_bpp = 24;                 // bits per pixel
  int bpr = (int) xres * 3;   // bytes per row
  m_fname = fname;

  // create a TIF file on disk
  m_tif = fopen ( m_fname.c_str(), "wb" );

  int num_entry = 10;
  int ifd_start = 8;                // directory
  int ifd_end = ifd_start + 2 + num_entry * 12 + 4;
  int so_start = ifd_end + 6;       // strip offsets
  int so_end = so_start + yres*4;
  m_pix_start = so_end + yres*4;

  // tiff header
  m_buf.clear();
	m_buf.write<uint16_t>( TIFF_BYTEORDER );
	m_buf.write<uint16_t>( TIFF_MAGIC );
	m_buf.write<uint32_t>( ifd_start );  

  // tiff directory (IFD)
  m_buf.write<uint16_t>( num_entry );    // # of entries
  m_buf.pack_quad ( TifImageWidth,      TifShort, 1, xres );
  m_buf.pack_quad ( TifImageHeight,     TifShort, 1, yres );
  m_buf.pack_quad ( TifBitsPerSample,   TifShort, 3, ifd_end );
  m_buf.pack_quad ( TifCompression,     TifShort, 1, TifCompNone );
  m_buf.pack_quad ( TifPhotometric,     TifShort, 1, TifColor );  
  m_buf.pack_quad ( TifStripOffsets,    TifLong,  yres, so_start );
  m_buf.pack_quad ( TifSamplesPerPixel, TifShort, 1, 3 );
  m_buf.pack_quad ( TifRowsPerStrip,    TifShort, 1, 1 );
	m_buf.pack_quad ( TifStripByteCounts, TifLong, yres, so_end );
	m_buf.pack_quad ( TifPlanarConfiguration, TifShort, 1, 1);
  m_buf.write<uint32_t>( 0 );
  
  // tiff extras - bits-per-channel (BPC)  
  m_buf.write<uint16_t>( 8 );
	m_buf.write<uint16_t>( 8 );
	m_buf.write<uint16_t>( 8 ); 

  pos = m_buf.size();  
	assert ( m_buf.size() == so_start );

  // tiff strip offsets  
  for (int n=0; n < yres; n++) {
    m_buf.write<uint32_t> ( m_pix_start + n * bpr );
  }
  // tiff strip counts 
  for (int n=0; n < yres; n++) {
    m_buf.write<uint32_t> ( bpr );    
  }

  assert ( m_buf.size() == m_pix_start );

  // write tiff info 
  fwrite ( m_buf.dataptr(), 1, m_buf.size(), m_tif );

  m_buf.clear ();
}

void GXPrinter::WriteToTIF ( ImageX& img, int x, int y )
{
  int bpp = 3;

  int xres = m_target.outRes.x;
  int yres = m_target.outRes.y;

  XBYTE out[TIFF_BUFFER];			
  int xsub = img.GetWidth();
  int ysub = img.GetHeight();
  int xstart = x * xsub;  
  int ystart = y * ysub;

  int dst_offs;  
  int src_stride = xsub * 3;

  // sub-image rows
  uchar* pix = img.GetData();
	for (y=0; y < ysub; y++) {
    dst_offs = m_pix_start + ( (ystart + y) * xres + xstart ) * bpp;
    _fseeki64 ( m_tif, dst_offs, SEEK_SET );
    fwrite ( pix, 1, src_stride, m_tif );

		pix += src_stride;
	}	

}

void GXPrinter::CloseTIF ()
{
  fclose ( m_tif );
  m_tif = 0x0;
}

bool GXPrinter::Print ( Vec4F fullview )
{
  Vec4F view;

  if (!m_renderFunc) return false;

  m_target.fullview = fullview;
  m_target.printing = true;  

  // Determine output resolution
  int W = int( m_target.outSz.z * m_target.ppi );
  int H = int( m_target.outSz.w * m_target.ppi );  
  
  // Tile size & count
  int TX = m_target.tileres.x;
  int TY = m_target.tileres.y;
  m_target.width = TX;
  m_target.height = TY;
  m_target.region = Vec4F(0, 0, TX, TY);      // viewport

  m_target.tilecnt = Vec2I( (W + TX - 1) / TX,  (H + TY - 1) / TY );  

  // Create render buffers
  if (!Create( m_target.tileres.x, m_target.tileres.y )) {
    return false;
  }
  // Create output buffer
  ImageX img;
  img.Resize ( TX, TY, ImageOp::RGB8 );

  // Stream to TIFF
  int RX = m_target.tilecnt.x * TX;
  int RY = m_target.tilecnt.y * TY;
  m_target.outRes = Vec2I(RX, RY);
  CreateTIF ( "out.tif", RX, RY );

  // Print tiles
  char fname[128];
  printf ( "Printing.. \n" );
  int cnt = m_target.tilecnt.x*m_target.tilecnt.y;

  gx.clearSets();

  for (int y=0; y < m_target.tilecnt.y; y++) {  
    for (int x=0; x < m_target.tilecnt.x; x++) {
      
      int i = (y*m_target.tilecnt.x)+x;

      printf ( " %d of %d, %4.2f%%\n", i, cnt, float(i)*100.0f / cnt );

      // set tile view
      view = TileView (x, y);
      
      // compute matrices
       m_target.Update ( view, m_target.region );      

      // render scene to fbo      
      glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);      

      m_renderFunc( m_context, m_target );

      // actual opengl render
      drawAll ();

      // readback pixels
      glReadBuffer ( GL_COLOR_ATTACHMENT0 );
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, m_target.width, m_target.height, GL_RGB, GL_UNSIGNED_BYTE, img.GetData() );
      img.FlipY();

      // stream to TIF
      WriteToTIF ( img, x, y );
      
      // optional save each tile
      // sprintf ( fname, "out_%dx%d.png", x, y );      
      // img.Save ( fname ); 
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0 );

  CloseTIF();

  printf ("Wrote TIF: %d x %d, %s\n", RX, RY, m_fname.c_str()) ;
}
