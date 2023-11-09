
#include "g2_textbox.h"
#include "gxlib.h"

using namespace glib;

void g2TextBox::UpdateLayout ( Vec4F p )
{
    // update self 
    m_pos = p;
}

void g2TextBox::Render ( uchar what )
{
    switch (what ) {
    case 'b': drawBackgrd(); break;
    case 'r': drawBorder(); break;
    case 'f': drawForegrd(); break;
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
    char msg[256];
    strncpy (msg, m_name.c_str(), 256);
    drawText ( Vec2F(m_pos.x, m_pos.y), msg, Vec4F(1,1,1,1));
}


