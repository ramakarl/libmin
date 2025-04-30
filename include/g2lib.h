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
    #include "g2obj.h"
    #include "main_includes.h"      // AppEnum
  
    namespace glib {

    class GXAPI g2Lib {
    public:
        g2Lib () { m_selected = 0x0; m_keyboard = 0; }

        // Interaction (top-level)
        bool OnMouse ( AppEnum button, AppEnum state, int mods, int x, int y);
        bool OnMotion ( AppEnum button, int x, int y, int dx, int dy ); 
        bool OnKeyboard ( int key, AppEnum action, int mods, int x, int y);
        void OnSelect ( g2Obj* obj, int x, int y );
        
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
        g2Obj* getSelected () { return m_selected; }
        int Traverse(std::vector<g2Obj*>& list);        

        // Build GUI
        void BuildAll ();
        bool BuildLayout ( g2Obj* obj, uchar ly );        
        void BuildSections ( g2Obj* obj, uchar ly );        
        g2Obj* AddObj ( std::string name, uchar typ );
        g2Obj* FindObj ( std::string name );    

        // Pages
        bool AddPage (int id);
        bool OpenPage ( std::string name );
        void ClosePage ( std::string name );
        //int getCurrPage () { return (m_curr_page < m_pages.size()) ? m_pages [ m_curr_page ] : -1; }

        // Layout & Render
        void LayoutAll ( Vec4F view, Vec4F region );
        void Render (int w, int h);
        char getKeyboardRequest()   { return m_keyboard; }
        void setKeyboard(char k)    { m_keyboard = k; }
        void SetAction(std::string a) {m_action = a;}
        std::string getAction()     {return m_action;}
      

    public:
        std::vector< g2Def >        m_objdefs;      // object specs

        std::vector< g2Obj* >       m_objlist;      // object list
        std::vector< int >          m_pages;
        std::vector< int >          m_active_pages;

        std::vector<std::string>    m_spec;         // original spec (temporary)
                
        g2Obj*                      m_selected;     // selected object
        char                        m_keyboard;     // keyboard request
      
        std::string                 m_action;
    };

    // Global singleton
    extern GXAPI glib::g2Lib   g2;

    };

#endif
