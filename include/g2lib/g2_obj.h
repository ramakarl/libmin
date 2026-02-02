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
    #include "value_t.h"

    class ImageX;
  
    namespace glib {

    enum Event_t {      // Events - how user triggers action
      EStart = 0,
      EClick = 1,
      EMouse = 2,
      EMotion = 3,
      ESelect = 4,
      EAdjust = 5,
      EMax,
      EUndef
    };    
    enum Act_t {        // Action - type of response
      ANull = 0,
      ASet = 1,
      ANav = 2,
      AGoto = 3,
      ASel = 4,
      AMsg = 5,
      APop = 6,
      ACmd = 7,
      AScope = 8,
      AMax    
    };

    static inline std::string getActStr(Act_t a)
    {
      std::string s = "?";
      switch (a) {
      case ANull:  s = "ANull";  break;
      case ASet:   s = "ASet";   break;
      case ANav:   s = "ANav";   break;
      case AGoto:  s = "AGoto";  break;
      case ASel:   s = "ASel";   break;
      case AMsg:   s = "AMsg";   break;
      case APop:   s = "APop";   break;      
      case ACmd:   s = "ACmd";   break;       
      };
      return s;
    }

    class g2Action {
    public:
      g2Action() { gid = -1; event = EUndef; act = ANull; key="";  }
      g2Action( Event_t e, Act_t a) { event = e; act=a; }
      
      int         gid;          // gui object handle
      Event_t     event;        // event:  click, mouse, motion, ..
      Act_t       act;          // action: set, nav, goto, sel, msg, cmd

      std::string key;          // name of variable
      uint        var_handle;   // handle to variable
      uint        var_type;     // type:  T_FLOAT, T_VEC4, T_STR
      Vec4F       var_range;

      Value_t     value;        // value to set      
    };

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
        
        void    SetParent ( g2Obj* p )   {m_parent = p;}
        void    LoadImg ( ImageX*& img, std::string fname );
        g2Size  ParseSize ( std::string sz );
        Vec4F   SetRegion ( Vec4F p, Vec4F r, g2Size minx, g2Size maxx, g2Size miny, g2Size maxy );
        void    AddAction ( g2Action& a );
        bool    RunAction ( Event_t e, Value_t val = Value_t::nullval);

        std::string   getName()       { return m_name;}
        bool          isSelected();
        
    public:
        g2Obj*          m_parent;
        int             m_id;
        std::string     m_name;        

        Vec4F           m_pos;
        Vec4F           m_backclr;
        Vec4F           m_borderclr;
        Vec4F           m_region;
        g2Size          m_minx, m_maxx, m_miny, m_maxy;

        // style options        
        bool            m_rounded;
        bool            m_isModal;

        std::vector<g2Action>   m_actions;
    };

    }

#endif
