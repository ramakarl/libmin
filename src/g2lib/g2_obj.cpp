
#include "string_helper.h"
#include "imagex.h"

#include "g2_obj.h"
#include "g2_lib.h"
using namespace glib;


g2Obj::g2Obj ()
{
  m_backclr = Vec4F(0, 0, 0, 0);
  m_borderclr = Vec4F(0, 0, 0, 0);
  m_region = Vec4F(0, 0, 1, 1);
  m_rounded = false;
  m_debug = false;
  m_isModal = false;
}

void g2Obj::AddAction(g2Action& a)
{
  if (m_actions.size()==0) {
    // InitActions
    m_actions.push_back (g2Action(EClick, ANull ) );
    m_actions.push_back (g2Action(EMouse, ANull ) );
    m_actions.push_back (g2Action(EMotion, ANull ) );
  }

  m_actions[a.event] = a;
}

bool g2Obj::RunAction(Event_t e, Value_t val )
{
  if (m_actions.size()==0) return false;
  
  return g2.RunAction (&m_actions[e], val );
}

bool g2Obj::isSelected() 
{
  return g2.getSelected() == this; 
}

// Common properties
//  (all object types)
//
void g2Obj::SetProperty ( std::string key, std::string val )
{
  if (key.compare("opt") == 0) {
    if (val.compare("modal")==0) m_isModal = true;

  } else if ( key.compare("backclr")==0 ) {
    m_backclr = strToVec4 ( val, ',' );

  } else if ( key.compare("borderclr")==0 ) {
    m_borderclr = strToVec4 ( val, ',' );

  } else if ( key.compare("style")==0 ) {

    if (val.compare("rounded")==0) m_rounded = true;

  } else if ( key.compare("margins")==0 ) {

    std::string str;
    str = strSplitLeft ( val, "|" );    m_minx = ParseSize ( str );
    str = strSplitLeft ( val, "|" );    m_maxx = ParseSize ( str );
    str = strSplitLeft ( val, "|" );    m_miny = ParseSize ( str );
    str = strSplitLeft ( val, "|" );    m_maxy = ParseSize ( str );
    //dbgprintf ( "%f %f %f %f\n", m_minx.amt, m_maxx.amt, m_miny.amt, m_maxy.amt );

  } else if (key.compare("region") == 0) {

    m_region = strToVec4 ( val, ',');

  } else if ( key.compare("debug")==0 ) {
    m_debug = true;
  }
}


Vec4F g2Obj::SetRegion ( Vec4F p, Vec4F r, g2Size minx, g2Size maxx, g2Size miny, g2Size maxy )
{
  Vec4F adv;
  g2Size spec;

  // region inset
  Vec4F dp (p.z-p.x, p.w-p.y, p.z-p.x, p.w-p.y);
  p = Vec4F(p.x, p.y, p.x, p.y) + dp * r;

  // x-axis adjust
  spec = minx;
  switch (spec.typ) {
  case '%': adv.x = (p.z-p.x) * spec.amt/100.0f; break;
  case 'x': adv.x = spec.amt; break;
  }; 
  spec = maxx;
  switch (spec.typ) {
  case '%': adv.z = -(p.z-p.x) * spec.amt/100.0f; break;
  case 'x': adv.z = -spec.amt; break;      
  }; 

  // y-axis adjust
  spec = miny;
  switch (spec.typ) {
  case '%': adv.y = (p.w-p.y) * spec.amt/100.0f; break;
  case 'x': adv.y = spec.amt; break;      
  }; 
  spec = maxy;
  switch (spec.typ) {
  case '%': adv.w = -(p.w-p.y) * spec.amt/100.0f; break;
  case 'x': adv.w = -spec.amt; break;      
  }; 

  return p + adv;
}

g2Size g2Obj::ParseSize ( std::string sz ) 
{
  g2Size size;
  size.typ = '?';
  size.amt = 0;
  if ( sz.find('%') != std::string::npos ) {
      // percent found
      size.typ = '%';
      sz = strSplitLeft ( sz, "%" );
      size.amt = strToF ( sz );
  } else if ( sz.find('p') != std::string::npos ) {
      // pixels found
      size.typ = 'x';
      sz = strSplitLeft( sz, "px" );
      size.amt = strToF ( sz );
      if ( size.amt==0 ) {
          dbgprintf ( "WARNING: Size 0 for %s.\n", sz.c_str() );
      }
  } else if ( sz.find('.') != std::string::npos ) {
      // repeat found
      size.typ = '.';
  } else if ( sz.find('*') != std::string::npos ) {
      // expand found            
      size.typ = '*';
  } else if ( sz.find('G') != std::string::npos ) {
      // grow found
      size.typ = 'G';
  }
  return size;
}

void g2Obj::LoadImg ( ImageX*& img, std::string fname )
{
  std::string fpath; 

  if (img != 0x0 ) delete img;
  
  img = new ImageX; 

  // try file as given
  if ( getFileLocation ( fname, fpath ) )
    if ( img->Load ( fpath ) ) 
      return;  

  // try png
  std::string fbase = strSplitLeft ( fname, "." );  
  fname = fbase + ".png";
  if ( getFileLocation ( fname, fpath ) )
    if ( img->Load ( fpath ) ) 
      return;

  // try jpg
  fname = fbase + ".jpg";
  if (getFileLocation(fname, fpath)) {
    if (img->Load(fpath)) 
      return;   // success
  }

  delete img;
  img = 0x0;
  
  printf ( "WARNING: Unable to load image: %s\n", fbase.c_str() );  
}



