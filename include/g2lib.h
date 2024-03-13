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
        
        // Layout Spec
        void LoadSpec ( std::string fname );
        void AddSpec ( std::string sentence );
        void ParseSpecToDef ( std::string lin ) ;
        void SetDef ( std::string name, std::string isa );
        void SetKeyVal ( std::string name, std::string key, std::string val );
        g2Def* FindDef ( std::string name );
        uchar FindType ( std::string isa );
        std::string getVal ( std::string name, std::string key );
        int getValList ( std::string name, std::string key, std::vector<std::string>& list );
        bool hasVal ( std::string name, std::string key, std::string val );
        void getWords ( std::string str, std::vector<std::string>& words, int maxw=1000 );

        // Build GUI
        void BuildAll ();
        bool BuildLayout ( g2Obj* obj, uchar ly );        
        void BuildSections ( g2Obj* obj, uchar ly );        
        g2Obj* AddObj ( std::string name, uchar typ );
        g2Obj* FindObj ( std::string name );

        // Pages
        void AddPage (int id);
        void OpenPage ( std::string name );
        void ClosePage ( std::string name );
        //int getCurrPage () { return (m_curr_page < m_pages.size()) ? m_pages [ m_curr_page ] : -1; }

        // Layout & Render
        void LayoutAll (float xres, float yres);
        void Render (int w, int h);

    public:
        std::vector< g2Def >        m_objdefs;      // object specs

        std::vector< g2Obj* >       m_objlist;      // object list
        std::vector< int >          m_pages;
        std::vector< int >          m_active_pages;

        std::vector<std::string>    m_spec;         // original spec (temporary)
    };

    // Global singleton
    extern GXAPI glib::g2Lib   g2;

    };

#endif
