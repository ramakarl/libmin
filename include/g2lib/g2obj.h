//--------------------------------------------------
//
// giObj
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//
//--------------------------------------------------


#ifndef DEF_GI_OBJ
    #define DEF_GI_OBJ
    
    #include <string>
    #include "gxlib_types.h"
    #include "main_includes.h"

    class ImageX;
  
    namespace glib {

    class g2Obj;

    // object definition 
    struct g2Def {
        std::string   name;
        std::string   isa;
        std::vector<std::string> keys;        
        std::vector<std::string> vals;
    };

    // GUI size specifier
    struct g2Size {
        uchar typ;      // '%' (percent), 'x' (pixels), '*' (expand), 'G' (grow), '.' (repeat)
        float amt;      // number (in units ot type)
    };

    // GUI layout specifier
    struct g2Layout {
        bool                        active;        
        std::vector< g2Size >       sizes;        
        std::vector< g2Obj* >       sections;
    };

    class GXAPI g2Obj {
    public:   
        g2Obj();

        virtual uchar getType()     { return 'o'; }              
        virtual void UpdateLayout ( Vec4F region )  {};
        virtual void SetProperty ( std::string key, std::string val );
        virtual void drawBackgrd (bool dbg) {};
        virtual void drawBorder (bool dbg)  {};
        virtual void drawForegrd (bool dbg) {};      
        virtual void drawSelected (bool dbg) {};
        virtual void OnSelect (int x, int y) {};
        virtual bool OnMouse(AppEnum button, AppEnum state, int mods, int x, int y) { return false; }
        virtual bool OnMotion(AppEnum button, int x, int y, int dx, int dy)         { return false; }
        virtual bool OnKeyboard(int  key, AppEnum action, int mods, int x, int y)   { return false; }
        virtual bool FindParent( g2Obj* obj, g2Obj*& parent, Vec3I& id )            { return false; }                
        virtual int  Traverse( std::vector<g2Obj*>& list )   { list.push_back(this); return list.size(); }
        virtual bool isEditable() { return false; }
        
        
        void SetParent ( g2Obj* p )   {m_parent = p;}
        void LoadImg ( ImageX*& img, std::string fname );
        g2Size ParseSize ( std::string sz );
        Vec4F SetMargins ( Vec4F p, g2Size minx, g2Size maxx, g2Size miny, g2Size maxy );

        std::string getName()       { return m_name;}
        
    public:
        g2Obj*          m_parent;
        int             m_id;
        std::string     m_name;
        bool            m_debug;
        
        
        Vec4F           m_pos;
        Vec4F           m_backclr;
        Vec4F           m_borderclr;
        g2Size          m_minx, m_maxx, m_miny, m_maxy;

        // style options        
        bool            m_rounded;
        bool            m_isModal;
    };

    }

#endif
