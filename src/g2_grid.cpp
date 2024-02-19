
#include "g2_grid.h"
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
    // update self first
    m_pos = p;

    // 2D grid 
    if (m_layout[0].active && m_layout[1].active) {        
        printf ( "ERROR: 2D grids not yet supported.\n");
        exit(-3);
    } 

    // 1D grid
    int L = (m_layout[G_LX].active ? G_LX : G_LY);
    g2Size spec;
    g2Obj* obj;
    float major_res = (L==G_LX) ? p.z-p.x : p.w-p.y;    
    float minor_res = (L==G_LX) ? p.w-p.y : p.z-p.x;
    float x, y, sz;
    float major_used;
    bool repeat = false;
    int n, fill = -1;
    Vec4F adv;
    Vec4F pos = p;    

    // Pre-layout to define '*' (expand to fill)
    major_used = 0;
    for (n=0; n < m_layout[L].sections.size(); n++ ) {
                
        obj = m_layout[L].sections[n];
        
        // get size spec
        if ( n < m_layout[L].sizes.size() )
            spec = m_layout[L].sizes[n];
    
        // evaluate by size type
        switch (spec.typ) {
        case '%': sz = major_res * spec.amt/100.0f; break;
        case 'x': sz = spec.amt; break;
        case '*': sz = 0; break;        
        };
        major_used += sz;  
    }

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
        case 'x': sz = spec.amt;                    break;
        case '*': sz = major_res - major_used;      break;
        };              
        getDimensions ( L, sz, pos, adv );   
        if (obj != 0x0) obj->UpdateLayout( pos );
        pos += adv;
    }
}


void g2Grid::drawChildren ( uchar what )
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
            if (obj !=0x0 ) obj->drawBackgrd();
          }
          break;
        case 'r':
          // draw children borders
          for (int n=0; n < m_layout[L].sections.size(); n++) {
            obj = m_layout[L].sections[n];
            if (obj !=0x0 ) obj->drawBorder();
          }
          break;
        case 'f':
          // draw children foregrounds
          for (int n=0; n < m_layout[L].sections.size(); n++) {
            obj = m_layout[L].sections[n];
            if (obj !=0x0 ) obj->drawForegrd();
          }
          break;
        };
      }
    }    
}

void g2Grid::drawBackgrd ()
{
    drawFill ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_backclr );
    drawChildren ( 'b' );    
}
void g2Grid::drawBorder ()
{
    drawRect ( Vec2F(m_pos.x,m_pos.y), Vec2F(m_pos.z, m_pos.w), m_borderclr );
    drawChildren ( 'r' );
}
void g2Grid::drawForegrd ()
{
    char msg[256];
    //strncpy (msg, m_name.c_str(), 256);
    //drawText ( Vec2F(m_pos.x, m_pos.y), msg, Vec4F(1,1,1,1));
    drawChildren ( 'f' );
}
