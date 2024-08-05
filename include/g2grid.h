//--------------------------------------------------
//
// giGrid
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//--------------------------------------------------


#ifndef DEF_GI_GRID
    #define DEF_GI_GRID
    
    #include "g2obj.h"

    #define G_LX    0
    #define G_LY    1
  
    namespace glib {

    class GXAPI g2Grid : public g2Obj {
    public:
        g2Grid ();

        virtual uchar getType()     { return 'g'; }
        virtual void UpdateLayout( Vec4F p );        
        virtual void drawBackgrd (bool dbg);
        virtual void drawBorder (bool dbg);
        virtual void drawForegrd (bool dbg);
        virtual void drawChildren ( uchar what, bool dbg=false );

        g2Layout* getLayout(uchar ly)   {return &m_layout[ly];}
        void getDimensions ( uchar L, float sz, Vec4F& pos, Vec4F& adv );

        g2Layout        m_layout[2];       
    };


    }

#endif
