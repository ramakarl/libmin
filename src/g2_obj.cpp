
#include "string_helper.h"
#include "imagex.h"

#include "g2_obj.h"
using namespace glib;


g2Obj::g2Obj ()
{
  m_backclr = Vec4F(0, 0, 0, 0);
  m_borderclr = Vec4F(0, 0, 0, 0);
  m_debug = false;
}

void g2Obj::SetProperty ( std::string key, std::string val )
{
  if ( key.compare("backclr")==0 ) {
    m_backclr = strToVec4 ( val, ',' );

  } else if ( key.compare("borderclr")==0 ) {
    m_borderclr = strToVec4 ( val, ',' );
  
  } else if ( key.compare("debug")==0 ) {
    m_debug = true;
  }

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
  if ( getFileLocation ( fname, fpath ) )
    if ( img->Load ( fpath ) ) return;
  
  printf ( "WARNING: Unable to load image: %s\n", fbase.c_str() );  
}



