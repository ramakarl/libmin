
#include "string_helper.h"

#include "g2_obj.h"
using namespace glib;


void g2Obj::SetProperty ( std::string key, std::string val )
{
  if ( key.compare("backclr")==0 ) {
    m_backclr = strToVec4 ( val, ',' );

  } else if ( key.compare("borderclr")==0 ) {
    m_borderclr = strToVec4 ( val, ',' );
  }
}




