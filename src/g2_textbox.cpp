
#include "string_helper.h"
#include "g2_textbox.h"
#include "gxlib.h"

using namespace glib;

g2TextBox::g2TextBox ()
{
  m_textclr.Set(1,1,1,1);
  m_text = "";
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

  } else {    
    g2Obj::SetProperty ( key, val );
  }
}

void g2TextBox::drawBackgrd ()
{
    drawFill ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_backclr );
}
void g2TextBox::drawBorder ()
{
    drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_borderclr );
}
void g2TextBox::drawForegrd ()
{
    char msg[512];
    strncpy (msg, m_text.c_str(), 512);
    drawText ( Vec2F(m_pos.x, m_pos.y), msg, Vec4F(1,1,1,1));
}


