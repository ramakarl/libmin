
#include "string_helper.h"
#include "imagex.h"
#include "g2_textbox.h"
#include "gxlib.h"

using namespace glib;

// icon-scale
#define ICON_FIT      100
#define ICON_AUTO     101
#define ICON_STRETCH  102

// icon-place
#define ICON_CENTER   210
#define ICON_LEFT     211
#define ICON_RIGHT    212
#define ICON_TOP      213
#define ICON_BOTTOM   214

g2TextBox::g2TextBox ()
{
  m_textclr.Set(1,1,1,1);
  m_text = "";
  m_textsz = 16;
  m_icon = 0;
  m_icon_scalex = ICON_FIT;
  m_icon_scaley = ICON_FIT;
  m_icon_placex = ICON_CENTER;
  m_icon_placey = ICON_CENTER;
}

void g2TextBox::LayoutIcon ()
{
  // Icon Layout rules:
  // icon_scale - 0-100   = % aspect-preserving fit to region
  // icon_scale - FIT     = same as 100%
  // icon_scale - AUTO    = preserve aspect ratio
  // icon_scale - STRETCH = use up region, ignore aspect
  //
  if (m_icon==0) return;

  // image specs
  float ix, iy, iasp, rx, ry, rasp;
  ix = m_icon->GetWidth();          // image width
  iy = m_icon->GetHeight();         // image height
  iasp = ix / iy;                   // image aspect ratio

  // region specs
  rx = (m_pos.z - m_pos.x);         // region width
  ry = (m_pos.w - m_pos.y);         // region height
  rasp = rx / ry;                   // region aspect ratio

  // icon scaling
  // fit horizontal
  if ( m_icon_scalex <= ICON_FIT ) {
    rx *= (m_icon_scalex/100.0f);   // fractional fit
    if ( (m_icon_scaley <= ICON_FIT && iasp > rasp) || m_icon_scaley==ICON_AUTO)  {
      ry = rx / iasp;               // perserve aspect
    }
  }
  // fit vertical
  if ( m_icon_scaley <= ICON_FIT )
    ry *= (m_icon_scaley/100.0f);   // fractional fit
    if ( (m_icon_scalex <= ICON_FIT && iasp < rasp) || m_icon_scalex==ICON_AUTO) {
      rx = ry * iasp;               // perserve aspect
    }
  
  // icon positioning
  float px, py;    
  switch ( m_icon_placex ) {
  case ICON_CENTER:   px = 0.5f;    break;
  case ICON_LEFT:     px = 0.0f;    break;
  case ICON_RIGHT:    px = 1.0f;    break;
  default:            px = m_icon_placex/100.0f;    break;
  };
  switch ( m_icon_placey ) {
  case ICON_CENTER:   py = 0.5f;    break;
  case ICON_TOP:      py = 0.0f;    break;
  case ICON_BOTTOM:   py = 1.0f;    break;
  default:            py = m_icon_placey/100.0f;    break;
  };
  // set top-left corner (positioning)
  m_icon_pos.x = m_pos.x + ( (m_pos.z-rx) - m_pos.x ) * px;
  m_icon_pos.y = m_pos.y + ( (m_pos.w-ry) - m_pos.y ) * py;

  // set bottom-right corner
  m_icon_pos.z = m_icon_pos.x + rx;
  m_icon_pos.w = m_icon_pos.y + ry;
}

void g2TextBox::UpdateLayout ( Vec4F p )
{
  // update self 
  m_pos = p;

  m_pos = SetMargins ( p, m_minx, m_maxx, m_miny, m_maxy );    

  LayoutIcon ();
}

void g2TextBox::SetProperty ( std::string key, std::string val )
{
  std::string horiz, vert;

  if ( key.compare("color")==0 ) {
    m_textclr = strToVec4 ( val, ',' );

  } else if ( key.compare("text")==0 ) {
    m_text = val;

  } else if ( key.compare("icon")==0 ) {

    g2Obj::LoadImg ( m_icon, val );  
    // printf ( "loaded: %s <- %s\n", m_name.c_str(), val.c_str() );

  } else if ( key.compare("icon-scale")==0 ) {
    
    vert = val; horiz = strSplitLeft ( vert, "|" );
    
    // parse horiz scaling
    if (horiz.compare("auto")==0)           { m_icon_scalex = ICON_AUTO; } 
    else if (horiz.compare("fit")==0)       { m_icon_scalex = ICON_FIT;  }
    else if (horiz.compare("stretch")==0)   { m_icon_scalex = ICON_STRETCH; }
    else                                    { m_icon_scalex = strToI ( strSplitLeft( horiz, "%" )); }    

    // parse vert scaling
    if (vert.compare("auto")==0)            { m_icon_scaley = ICON_AUTO; }
    else if (vert.compare("fit")==0)        { m_icon_scaley = ICON_FIT;  }
    else if (vert.compare("stretch")==0)    { m_icon_scaley = ICON_STRETCH; }
    else                                    { m_icon_scaley = strToI ( strSplitLeft( vert, "%" )); }    

  } else if ( key.compare("icon-place")==0 ) {
    
    vert = val; horiz = strSplitLeft ( vert, "|" );
    
    // parse horiz pos
    if (horiz.compare("center")==0)         { m_icon_placex = ICON_CENTER; }
    else if (horiz.compare("left")==0)      { m_icon_placex = ICON_LEFT; }
    else if (horiz.compare("right")==0)     { m_icon_placex = ICON_RIGHT; } 
    else                                    { m_icon_placex = strToI ( strSplitLeft( horiz, "%" )); }    

    // parse vert pos
    if (vert.compare("center")==0)          { m_icon_placey = ICON_CENTER; }
    else if (vert.compare("top")==0)        { m_icon_placey = ICON_TOP; }
    else if (vert.compare("bottom")==0)     { m_icon_placey = ICON_BOTTOM; } 
    else                                    { m_icon_placey = strToI ( strSplitLeft( vert, "%" )); }    
    
  } else {    
    g2Obj::SetProperty ( key, val );
  }
}

void g2TextBox::drawBackgrd (bool dbg)
{  
  drawFill ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_backclr );
}

void g2TextBox::drawBorder (bool dbg)
{
  if (dbg) {
    drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1,0.5,0,1) );
    return;
  }
  if ( m_borderclr.w > 0 ) {
    drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_borderclr );
  }
}

void g2TextBox::drawForegrd ( bool dbg)
{
  char msg[512];

  // icon
  if ( m_icon != 0x0 ) {
    drawImg ( m_icon, Vec2F(m_icon_pos.x, m_icon_pos.y), Vec2F(m_icon_pos.z, m_icon_pos.w), Vec4F(1,1,1,1) );    
  }

  // text
  if (dbg) {
    strncpy (msg, m_name.c_str(), 512);    
  } else {
    strncpy (msg, m_text.c_str(), 512);   // item text
  }
  setTextSz ( m_textsz, 0 );
  drawText ( Vec2F(m_pos.x, m_pos.y), msg, Vec4F(1,1,1,1));
}


