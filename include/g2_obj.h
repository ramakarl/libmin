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
  
    namespace glib {

    class g2Obj;

    // object definition 
    struct g2Def {
        std::string obj;
        std::string isa;
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
        virtual uchar getType()     { return 'o'; }              
        virtual void UpdateLayout ( Vec4F p )  {};
        virtual void SetProperty ( std::string key, std::string val );
        virtual void drawBackgrd () {};
        virtual void drawBorder ()  {};
        virtual void drawForegrd () {};        

        std::string getName()       { return m_name;}
        
    public:
        std::string     m_name;
        Vec4F           m_pos;
        Vec4F           m_backclr;
        Vec4F           m_borderclr;
    };

    }

#endif
