
#include "string_helper.h"
#include "imagex.h"
#include "g2textbox.h"
#include "g2lib.h"
#include "gxlib.h"
#include "event_system.h"

using namespace glib;

// Scaling
#define SCALE_FIT      100
#define SCALE_AUTO     101
#define SCALE_STRETCH  102

// Placement
#define PLACE_PERC     100      // 0-100, place by %
#define PLACE_CENTER   210
#define PLACE_LEFT     211
#define PLACE_RIGHT    212
#define PLACE_TOP      213
#define PLACE_BOTTOM   214

g2TextBox::g2TextBox ()
{
  m_isButton = false;  
  m_button_state = 0;
  m_button_toggleable = false;
  
  m_text_clr.Set(1,1,1,1);
  m_text = "";
  m_text_size = 10;      // default 10 pnt
  m_text_placex = PLACE_LEFT;
  m_text_placey = PLACE_CENTER;
  m_text_margin.Set(2,2,2,2);   // 2% margin

  m_isEditable = false;  
  m_edit_pos.Set(0,0,0,0);
  m_selection.Set(-1,-1,0,0);

  m_icon = 0;
  m_icon_on = 0;
  m_icon_scalex = SCALE_FIT;
  m_icon_scaley = SCALE_FIT;
  m_icon_placex = PLACE_CENTER;
  m_icon_placey = PLACE_CENTER;
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
  if ( m_icon_scalex <= SCALE_FIT ) {
    rx *= (m_icon_scalex/100.0f);   // fractional fit
    if ( (m_icon_scaley <= SCALE_FIT && iasp > rasp) || m_icon_scaley==SCALE_AUTO)  {
      ry = rx / iasp;               // perserve aspect
    }
  }
  // fit vertical
  if ( m_icon_scaley <= SCALE_FIT )
    ry *= (m_icon_scaley/100.0f);   // fractional fit
    if ( (m_icon_scalex <= SCALE_FIT && iasp < rasp) || m_icon_scalex==SCALE_AUTO) {
      rx = ry * iasp;               // perserve aspect
    }
  
  // icon positioning
  float px, py;    
  switch ( m_icon_placex ) {
  case PLACE_CENTER:   px = 0.5f;    break;
  case PLACE_LEFT:     px = 0.0f;    break;
  case PLACE_RIGHT:    px = 1.0f;    break;
  default:             px = m_icon_placex/100.0f;    break;
  };
  switch ( m_icon_placey ) {
  case PLACE_CENTER:   py = 0.5f;    break;
  case PLACE_TOP:      py = 0.0f;    break;
  case PLACE_BOTTOM:   py = 1.0f;    break;
  default:            py = m_icon_placey/100.0f;    break;
  };
  // set top-left corner (positioning)
  m_icon_pos.x = m_pos.x + ( (m_pos.z-rx) - m_pos.x ) * px;
  m_icon_pos.y = m_pos.y + ( (m_pos.w-ry) - m_pos.y ) * py;

  // set bottom-right corner
  m_icon_pos.z = m_icon_pos.x + rx;
  m_icon_pos.w = m_icon_pos.y + ry;
}


std::string g2TextBox::getPrintedText(Vec4F& clr)
{
  if (!m_text.empty()) {
    clr = m_text_clr;
    return m_text;
  } else {
    clr = Vec4F(.4,.4,.4, 1);
    return m_text_empty;
  }
}


void g2TextBox::LayoutText ()
{
  // Text Layout   
  
  // item specs
  Vec4F area = m_pos;                                       // text area defaults to item region
  area.x += m_text_margin.x * (m_pos.z-m_pos.x) / 100.0f;   // adjust area by text margin
  area.z -= m_text_margin.z * (m_pos.z-m_pos.x) / 100.0f;
  area.y += m_text_margin.y * (m_pos.w-m_pos.y) / 100.0f;
  area.w -= m_text_margin.w * (m_pos.w-m_pos.y) / 100.0f;
    
  // text positioning
  float px, py;
  switch (m_text_placex) {
  case PLACE_LEFT:     px = 0.0f;    break;
  case PLACE_CENTER:   px = 0.5f;    break;  
  case PLACE_RIGHT:    px = 1.0f;    break;
  default:             px = m_icon_placex / 100.0f;    break;
  };
  switch (m_text_placey) {  
  case PLACE_TOP:      py = 0.0f;    break;
  case PLACE_CENTER:   py = 0.5f;    break;
  case PLACE_BOTTOM:   py = 1.0f;    break;
  default:             py = m_icon_placey / 100.0f;    break;
  };  
  Vec4F clr;
  Vec2F sz = getTextDim( 'p', m_text_size, getPrintedText(clr) );

  // set top-left corner of text
  m_text_pos.x = area.x + (area.z-area.x) * px - sz.x*px;
  m_text_pos.y = area.y + (area.w-area.y) * py - sz.y*py;
}



void g2TextBox::UpdateLayout ( Vec4F p )
{
  // update self 
  m_pos = p;

  m_pos = SetMargins ( p, m_minx, m_maxx, m_miny, m_maxy );

  LayoutIcon ();

  LayoutText ();
}

// Item properties
//
void g2TextBox::SetProperty ( std::string key, std::string val )
{
  std::string horiz, vert;

  if (key.compare("opt") == 0) {
    if (val.compare("editable")==0)  m_isEditable = true;
    if (val.compare("button")==0)    m_isButton = true;
    if (val.compare("toggleable") == 0) m_button_toggleable = true;

  } else if ( key.compare("textclr")==0 || key.compare("text color")==0 ) {
    m_text_clr = strToVec4 ( val, ',' );

  } else if (key.compare("text empty") == 0 ) {
    m_text_empty = val;

  } else if (key.compare( "textsz") == 0 || key.compare("text size") == 0) {
    m_text_size = strToF(val);

  } else if (key.compare( "text align") == 0) {

    vert = val; horiz = strSplitLeft(vert, "|");

    // parse horiz pos
    if (horiz.compare("center") == 0)     { m_text_placex = PLACE_CENTER; }
    else if (horiz.compare("left") == 0)  { m_text_placex = PLACE_LEFT; }
    else if (horiz.compare("right") == 0) { m_text_placex = PLACE_RIGHT; }
    else { m_text_placex = strToI(strSplitLeft(horiz, "%")); }
    // parse vert pos
    if (vert.compare("center") == 0)      { m_text_placey = PLACE_CENTER; }
    else if (vert.compare("top") == 0)    { m_text_placey = PLACE_TOP; }
    else if (vert.compare("bottom") == 0) { m_text_placey = PLACE_BOTTOM; }
    else { m_text_placey = strToI(strSplitLeft(vert, "%")); }

  } else if ( key.compare("text")==0 ) {
    m_text = val;

  } else if ( key.compare("icon")==0 ) {

    g2Obj::LoadImg ( m_icon, val );      

  } else if (key.compare("icon on") == 0) {

    g2Obj::LoadImg( m_icon_on, val);

  } else if ( key.compare("icon-scale")==0 ) {
    
    vert = val; horiz = strSplitLeft ( vert, "|" );
    
    // parse horiz scaling
    if (horiz.compare("auto")==0)           { m_icon_scalex = SCALE_AUTO; } 
    else if (horiz.compare("fit")==0)       { m_icon_scalex = SCALE_FIT;  }
    else if (horiz.compare("stretch")==0)   { m_icon_scalex = SCALE_STRETCH; }
    else                                    { m_icon_scalex = strToI ( strSplitLeft( horiz, "%" )); }    

    // parse vert scaling
    if (vert.compare("auto")==0)            { m_icon_scaley = SCALE_AUTO; }
    else if (vert.compare("fit")==0)        { m_icon_scaley = SCALE_FIT;  }
    else if (vert.compare("stretch")==0)    { m_icon_scaley = SCALE_STRETCH; }
    else                                    { m_icon_scaley = strToI ( strSplitLeft( vert, "%" )); }    

  } else if ( key.compare("icon-place")==0 ) {
    
    vert = val; horiz = strSplitLeft ( vert, "|" );
    
    // parse horiz pos
    if (horiz.compare("center")==0)         { m_icon_placex = PLACE_CENTER; }
    else if (horiz.compare("left")==0)      { m_icon_placex = PLACE_LEFT; }
    else if (horiz.compare("right")==0)     { m_icon_placex = PLACE_RIGHT; } 
    else                                    { m_icon_placex = strToI ( strSplitLeft( horiz, "%" )); }    

    // parse vert pos
    if (vert.compare("center")==0)          { m_icon_placey = PLACE_CENTER; }
    else if (vert.compare("top")==0)        { m_icon_placey = PLACE_TOP; }
    else if (vert.compare("bottom")==0)     { m_icon_placey = PLACE_BOTTOM; } 
    else                                    { m_icon_placey = strToI ( strSplitLeft( vert, "%" )); }    
    
  } else {    
    g2Obj::SetProperty ( key, val );
  }
}

void g2TextBox::drawBackgrd (bool dbg)
{  
  if (m_rounded) {
    drawRoundedFill ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_backclr );
  } else {
    drawFill (Vec2F(m_pos.x, m_pos.y), Vec2F(m_pos.z, m_pos.w), m_backclr);
  }
}

void g2TextBox::drawBorder (bool dbg)
{
  if (dbg) {
    drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1,0.5,0,1) );
    return;
  }  
  if ( m_borderclr.w > 0 ) {
    if (m_rounded) {
      drawRoundedRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_borderclr );
    } else {
      drawRect (Vec2F(m_pos.x, m_pos.y), Vec2F(m_pos.z, m_pos.w), m_borderclr);
    }
  }
}

void g2TextBox::drawForegrd ( bool dbg)
{
  // button icon  
  if (m_button_state == 0) {    
    if (m_icon != 0x0) {      // down/off icon
      drawImg(m_icon, Vec2F(m_icon_pos.x, m_icon_pos.y), Vec2F(m_icon_pos.z, m_icon_pos.w), Vec4F(1, 1, 1, 1));
    }
  } else {    
    if (m_icon_on != 0x0) {   // up/on icon
      drawImg( m_icon_on, Vec2F(m_icon_pos.x, m_icon_pos.y), Vec2F(m_icon_pos.z, m_icon_pos.w), Vec4F(1, 1, 1, 1));
    } else {
      if (m_icon!=0x0) drawImg(m_icon, Vec2F(m_icon_pos.x, m_icon_pos.y), Vec2F(m_icon_pos.z, m_icon_pos.w), Vec4F(1, 1, 1, 1));
      drawFill( Vec2F(m_pos.x, m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1, 1, 1, .6));
    }
    if (!m_button_toggleable) m_button_state--;   // countdown non-toggleable buttons
  }

  // text
  //
  Vec4F clr;  
  std::string txt = getPrintedText (clr);  
  setTextPnts ( m_text_size, 0 );  
  drawText( Vec2F(m_text_pos.x, m_text_pos.y), txt, clr);  

  //-- debugging text area
  /* Vec4F area = m_pos;                                       // text area defaults to item region
  area.x += m_text_margin.x * (m_pos.z - m_pos.x) / 100.0f;   // adjust area by text margin
  area.z -= m_text_margin.z * (m_pos.z - m_pos.x) / 100.0f;
  area.y += m_text_margin.y * (m_pos.w - m_pos.y) / 100.0f;
  area.w -= m_text_margin.w * (m_pos.w - m_pos.y) / 100.0f;
  drawRect ( Vec2F(area.x, area.y), Vec2F(area.z,area.w), Vec4F(1,1,0,1) );
  drawLine ( Vec2F(( area.x+area.z)*0.5, area.y), Vec2F((area.x + area.z) * 0.5, area.w), Vec4F(1,0.5,0,1)); 
  drawLine ( Vec2F(area.x, (area.y+area.w)*0.5),  Vec2F(area.z, (area.y + area.w) * 0.5), Vec4F(1, 0.5, 0, 1));
  */
  
  if (m_isEditable && g2.getSelected() == this) {
    // selected text
    if (m_selection.y >= 0) {
      drawFill(Vec2F(int(m_text_pos.x + m_selection.z), m_pos.y + 5), Vec2F(int(m_text_pos.x + m_selection.w) + 2, m_pos.w - 5), Vec4F(0, 0, 0.4, 1));
      txt = m_text.substr ( m_selection.x, m_selection.y - m_selection.x );
      drawText(Vec2F(int(m_text_pos.x + m_selection.z), m_text_pos.y), txt, Vec4F(1,1,1,1) );
    }
    // cursor  
    drawFill ( Vec2F( int(m_text_pos.x+m_edit_pos.z)-1, m_pos.y+5), Vec2F( int(m_text_pos.x + m_edit_pos.z)+2, m_pos.w-5), Vec4F(1,1,1,1));
  }
 
}

void g2TextBox::drawSelected(bool dbg)
{
  if (m_isEditable) {        
    if (m_rounded) {
      drawRoundedRect(Vec2F(m_pos.x, m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1, 1, 1, 1));
    } else {
      drawRect(Vec2F(m_pos.x, m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1, 1, 1, 1));
    }
  }
}

void g2TextBox::OnSelect (int x, int y)
{
  // editable
  if (m_isEditable) {

    // set cursor in text
    if (m_text.length() > 0) {
      Vec2F sz(0, 0);
      int p = 0;
      // identify char in text matching cursor x
      std::string teststr = std::string(1, m_text.at(p));
      for (; m_text_pos.x + sz.x < x && p < m_text.length(); p++) {
        sz = getTextDim('p', m_text_size, teststr);        // check width
        if (p < m_text.length() - 1) teststr += std::string(1, m_text.at(p + 1));  // add next char
      }
      m_edit_pos.x = p;
      if (p >= m_text.length()) m_edit_pos.x = m_text.length();
      UpdateCursor();
    }
    else {
      m_edit_pos.x = 0;
    }
    // open keyboard
    if (g2.m_selected != this) g2.m_keyboard = 'o';
  }  

  // button action
  if (m_isButton) {
    if (m_button_toggleable) {
      m_button_state = 1 - m_button_state;
    }
    else {
      m_button_state = 120;      // deactivation countdown
    }
    // Find action associated with button
    std::string action = g2.getVal( m_name, "action" );    
    g2.SetAction ( action );
  }
}


bool g2TextBox::OnMouse(AppEnum button, AppEnum state, int mods, int x, int y)
{
  g2Obj* obj;

  if ( m_isEditable || m_isButton ) {
    if (button==AppEnum::BUTTON_LEFT && state==AppEnum::BUTTON_PRESS) {
      // select textbox
      if ( x > m_pos.x && y > m_pos.y && x < m_pos.z && y < m_pos.w ) {

        g2.OnSelect ( this, x, y );

        return true;
      }
    }
  }
  return false;
}

void g2TextBox::UpdateCursor ()
{
  Vec2F sz; 

  // enable selection
  if (m_selection.y >= 0 && m_selection.x >= 0) {
    if (m_selection.x > m_selection.y) { int tmp = m_selection.y; m_selection.y = m_selection.x; m_selection.x = tmp; }
    if (m_selection.x < 0) m_selection.x = 0;
    if (m_selection.y > m_text.length()) m_selection.y = m_text.length();
    sz = getTextDim('p', m_text_size, m_text.substr(0, m_selection.x));
    m_selection.z = sz.x;
    sz = getTextDim('p', m_text_size, m_text.substr(0, m_selection.y));
    m_selection.w = sz.x;
  }

  // recalculate width & cursor
  if (m_edit_pos.x < 0) m_edit_pos.x = 0;
  if (m_edit_pos.x > m_text.length()) m_edit_pos.x = m_text.length();
  sz = getTextDim('p', m_text_size, m_text.substr(0, m_edit_pos.x));
  m_edit_pos.z = sz.x;      // cursor pos (in pixels)
}

bool g2TextBox::OnKeyboard (int key, AppEnum action, int mods, int x, int y)
{
  if ( !m_isEditable ) return false;
  if ( action==AppEnum::BUTTON_RELEASE ) return false;
  
  if ( key > 400 ) return false;      // ignore explicit shift/ctrl/alt

  Vec2F sz;
  int dir;
  bool caught = false;
  // dbgprintf ( "key: %d, act: %d, mods: %d\n", (int) key, action, mods );

  // handle key
  switch (key) {
  case 263: case 262:         // left/right
    dir = (key == 263) ? -1 : 1;
    if (mods == 1) {
      if (m_selection.y==-1) {
        m_edit_pos.w = m_edit_pos.x;      // save selection start
        m_selection.Set(m_edit_pos.x, m_edit_pos.x, 0, 0);
      }      
      m_edit_pos.x += dir;      
      if (m_edit_pos.x < m_edit_pos.w)  m_selection.x = m_edit_pos.x;
      else                              m_selection.y = m_edit_pos.x;      
    } else {
      m_selection.Set(-1,-1,0,0);
      m_edit_pos.x += dir;      
    }    
    caught = true;
    break;
  case 265: case 264:         // up/down
    dir = (key == 265) ? -1 : 1;
    caught = true;
    break;
  case 268:                   // home
    if (mods==1) {
      m_selection.x = 0;      // select to home
      if (m_selection.y < 0) m_selection.y = m_edit_pos.x;
    } else {
      m_selection.Set(-1, -1, 0, 0);
    }
    m_edit_pos.y = 0;
    m_edit_pos.x = 0;         
    caught = true;
    break;
  case 269:                   // end
    if (mods == 1) {
      if (m_selection.x < 0) m_selection.x = m_edit_pos.x;
      m_selection.y = m_text.length();
    } else {
      m_selection.Set(-1, -1, 0, 0);
    }
    m_edit_pos.x = m_text.length();
    caught = true;
    break;  
  case 8:                     // backspace
    if (m_selection.y >= 0) {
      m_text = m_text.substr(0, m_selection.x) + m_text.substr(m_selection.y);   // delete selection
      m_edit_pos.x = m_selection.x;
      m_selection.Set(-1, -1, 0, 0);
    } else if (m_edit_pos.x > 0) {
      m_text = m_text.substr(0, m_edit_pos.x-1) + m_text.substr(m_edit_pos.x);    
      m_edit_pos.x--;
    }
    caught = true;
    break;
  case 127:                   // delete
    if (m_selection.y >= 0) {
      m_text = m_text.substr(0, m_selection.x ) + m_text.substr(m_selection.y);   // delete selection
      m_edit_pos.x = m_selection.x;
      m_selection.Set(-1, -1, 0, 0);
    } else if (m_edit_pos.x < m_text.length() ) {
      m_text = m_text.substr(0, m_edit_pos.x) + m_text.substr(m_edit_pos.x + 1);  // delete char
    }
    break;
  default:
    if (m_selection.y >= 0) {
      m_text = m_text.substr(0, m_selection.x) + m_text.substr(m_selection.y);   // delete selection
      m_edit_pos.x = m_selection.x;
      m_selection.Set(-1, -1, 0, 0);
    }
    if (m_edit_pos.x >= m_text.length() ) {
      m_text += key;
    } else {
      m_text = m_text.substr(0, m_edit_pos.x) + std::string(1, key) + m_text.substr(m_edit_pos.x);
    } 
    m_edit_pos.x++;
    caught = true;
    break;
  };

  UpdateCursor();
 
  return caught;
}


