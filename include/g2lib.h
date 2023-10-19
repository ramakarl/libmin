//--------------------------------------------------
//
// GI: GUI Library
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//
//
//--------------------------------------------------


#ifndef DEF_GI
    #define DEF_GI
    
    #include "gxlib.h"
    #include "gxlib_types.h"
    #include "g2_obj.h"
  
    namespace glib {

    class GXAPI g2Lib {
    public:
        g2Lib () {};
        
        // load/import layout spec
        void LoadSpec ( std::string fname );
        void SetDef ( std::string name, std::string isa );
        void SetKeyVal ( std::string name, std::string key, std::string val );
        g2Def* FindDef ( std::string name );
        uchar FindType ( std::string isa );
        std::string getVal ( std::string name, std::string key );

        // build GUI
        void BuildAll ();
        bool BuildLayout ( g2Obj* obj, uchar ly );        
        void BuildSections ( g2Obj* obj, uchar ly );
        g2Obj* AddObj ( std::string name, uchar typ );
        g2Obj* FindObj ( std::string name );

        // refresh layout
        void LayoutAll (float xres, float yres);

        // render GUI
        void Render (int w, int h);

    public:
        std::vector< g2Def >        m_objdefs;      // object specs

        std::vector< g2Obj* >       m_objlist;      // object list
        int                         m_root;
    };

    // Global singleton
    extern GXAPI glib::g2Lib   g2;

    };

#endif
