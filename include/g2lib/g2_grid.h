//--------------------------------------------------
//
// giGrid
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//--------------------------------------------------


#ifndef DEF_GI_GRID
    #define DEF_GI_GRID
    
    #include "g2_obj.h"

    #define G_LX    0
    #define G_LY    1
  
    namespace glib {

    class GXAPI g2Grid : public g2Obj {
    public:
        g2Grid ();

        virtual uchar getType()     { return 'g'; }
        virtual void UpdateLayout( Vec4F p );        
        virtual void DrawBackgrd (bool dbg);
        virtual void DrawBorder (bool dbg);
        virtual void DrawForegrd (bool dbg);
        virtual void DrawChildren ( uchar what, bool dbg=false );
        virtual void DrawOverlays (bool dbg);
        virtual bool OnMouse(AppEnum button, AppEnum state, int mods, int x, int y);
        virtual bool OnMotion(AppEnum button, int x, int y, int dx, int dy);
        virtual bool FindParent(g2Obj* obj, g2Obj*& parent, Vec3I& id );
        virtual int  Traverse(std::vector<g2Obj*>& list);  
        virtual bool HandleExclusive();
        
        g2Layout* getLayout(uchar ly)   {return &m_layout[ly];}
        void      getDimensions ( uchar L, float sz, Vec4F& pos, Vec4F& adv );
        int       getChildren ( std::vector<g2Obj*>& list );
        g2Obj*    getChild( Vec3I id );        

        g2Layout        m_layout[2];       

        std::vector< g2Obj* >   m_overlays;
    };


    }

#endif
