
#include "g2grid.h"
#include "gxlib.h"

using namespace glib;

g2Grid::g2Grid ()
{
    m_layout[0].active = false; 
    m_layout[1].active = false;    
}

void g2Grid::getDimensions ( uchar L, float sz, Vec4F& pos, Vec4F& adv )
{
    if ( L==G_LX) { 
        pos.z = pos.x + sz;
        adv = Vec4F(sz, 0, sz, 0);
    } else { 
        pos.w = pos.y + sz;
        adv = Vec4F(0, sz, 0, sz);
    }
}

void g2Grid::UpdateLayout ( Vec4F p )
{
    g2Size spec;
    g2Obj* obj;    
    float sz;
    float major_used;
    bool repeat = false;
    int n, fill = -1;
    Vec4F adv;

    // update self first
    m_pos = p;

    // margins (horiz & vert) 
    m_pos = SetMargins ( p, m_minx, m_maxx, m_miny, m_maxy );    

    // child regions
    Vec4F pos = m_pos;    

    // 2D grid 
    if (m_layout[0].active && m_layout[1].active) {        
        printf ( "ERROR: 2D grids not yet supported.\n");
        exit(-3);
    }
      
    // 1D grid layout
    int L = (m_layout[G_LX].active ? G_LX : G_LY);      
    float major_res = (L==G_LX) ? pos.z-pos.x : pos.w-pos.y;    
    float minor_res = (L==G_LX) ? pos.w-pos.y : pos.z-pos.x;    

    // Pre-layout to define '*' (expand to fill)
    major_used = 0;
    int star_cnt = 0;
    float star_sz = 0;
    for (n=0; n < m_layout[L].sections.size(); n++ ) {
                
        obj = m_layout[L].sections[n];
        
        // get size spec
        if ( n < m_layout[L].sizes.size() )
            spec = m_layout[L].sizes[n];
    
        // evaluate by size type        
        sz = 0;
        switch (spec.typ) {
        case '%': sz = major_res * spec.amt/100.0f; break;
        case 'x': sz = spec.amt; break;
        case '*': sz = 0; star_cnt++; break;        
        };
        major_used += sz;  
    }    
    // divide any remaining space equally among the '*'
    star_sz = (star_cnt==0) ? 0 : (major_res - major_used) / star_cnt;    
    
    // Layout each section
    for (n=0; n < m_layout[L].sections.size(); n++ ) {

        // get object
        obj = m_layout[L].sections[n];

        // get size spec
        if ( n < m_layout[L].sizes.size() )
            spec = m_layout[L].sizes[n];  

        // evaluate by size type
        switch (spec.typ) {
        case '%': sz = major_res * spec.amt/100.0f; break;
        case 'x': sz = spec.amt;     break;
        case '*': sz = star_sz;      break;
        };       

        getDimensions ( L, sz, pos, adv );

        // recursive call
        if (obj != 0x0) {
          obj->UpdateLayout( pos );
        }

        pos += adv;
    }
}



void g2Grid::drawChildren ( uchar what, bool dbg )
{
    g2Obj* obj;
    // L = horizontal & vertical = {G_LX, G_LY}    
    for (int L=0; L <=1; L++) {
      if ( m_layout[L].active ) {
        switch ( what ) {
        case 'b':
          // draw children backgrounds
          for (int n=0; n < m_layout[L].sections.size(); n++) {
            obj = m_layout[L].sections[n];
            if (obj !=0x0 ) obj->drawBackgrd( dbg );
          }
          break;
        case 'r':
          // draw children borders
          for (int n=0; n < m_layout[L].sections.size(); n++) {
            obj = m_layout[L].sections[n];
            if (obj !=0x0 ) obj->drawBorder( dbg );
          }
          break;
        case 'f':
          // draw children foregrounds
          for (int n=0; n < m_layout[L].sections.size(); n++) {
            obj = m_layout[L].sections[n];
            if (obj !=0x0 ) obj->drawForegrd( dbg );
          }
          break;
        };
      }
    }    
}

void g2Grid::drawBackgrd (bool dbg)
{
    /*if (dbg) {
      drawFill ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(0.1,0,0,1) );      
    }*/
    if ( m_backclr.w > 0 ) {
      drawFill ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_backclr );      
    }
    drawChildren ( 'b' );    
}
void g2Grid::drawBorder (bool dbg)
{
    if (dbg) {
      drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), Vec4F(1,1,0,1) );
    } else {
      drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_borderclr );
    }
    drawChildren ( 'r', dbg );
}
void g2Grid::drawForegrd (bool dbg)
{
    if ( dbg ) {
      char msg[256];
      strncpy (msg, m_name.c_str(), 256);
      drawText ( Vec2F(m_pos.x, m_pos.y), msg, Vec4F(1,1,1,1));
    }

    drawChildren ( 'f', dbg );
}
