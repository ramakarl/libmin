
#include "string_helper.h"
#include "g2_textbox.h"
#include "gxlib.h"

using namespace glib;

g2TextBox::g2TextBox ()
{
  m_textclr.Set(1,1,1,1);
  m_text = "";
  m_textsz = 16;
  m_icon = 0;
}

void g2TextBox::UpdateLayout ( Vec4F p )
{
    // update self 
    m_pos = p;
}

void g2TextBox::SetProperty ( std::string key, std::string val )
{
  if ( key.compare("color")==0 ) {
    m_textclr = strToVec4 ( val, ',' );

  } else if ( key.compare("text")==0 ) {
    m_text = val;

  } else if ( key.compare("icon")==0 ) {

    g2Obj::LoadImg ( m_icon, val );  
    printf ( "loaded: %s <- %s\n", m_name.c_str(), val.c_str() );

  } else {    
    g2Obj::SetProperty ( key, val );
  }
}

void g2TextBox::drawBackgrd ()
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
    drawImg ( m_icon, Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1,1,1,1) );    
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


