//--------------------------------------------------
//
// g2Item
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//
//--------------------------------------------------


#ifndef DEF_GI_ITEM
    #define DEF_GI_ITEM
    
    #include "g2_obj.h"
    #include "imagex.h"
  
    namespace glib {

    class GXAPI g2Item : public g2Obj {
    public:   
        g2Item ();

        virtual uchar getType()     { return 't'; }
        virtual void UpdateLayout ( Vec4F p ); 
        virtual void SetProperty ( std::string key, std::string val );
        virtual void drawBackgrd (bool dbg);
        virtual void drawBorder (bool dbg);
        virtual void drawForegrd (bool dbg);
        virtual void drawSelected (bool dbg);
        virtual void OnSelect(int x, int y);
        virtual bool OnMouse(AppEnum button, AppEnum state, int mods, int x, int y);
        virtual bool OnMotion(AppEnum button, int x, int y, int dx, int dy);
        virtual bool OnKeyboard (int key, AppEnum action, int mods, int x, int y);
        virtual bool isEditable() { return m_isEditable; }

        std::string   getPrintedText(Vec4F& clr);        
        void          UpdateCursor();           

        void Populate ( KeyValues* list );
        void SetCombo ( int ndx );
        void SetCombo ( std::string item );

        void LayoutIcon ();
        void LayoutText ();

        // button properties
        bool          m_isButton;
        bool          m_isSlider;
        char          m_button_state;
        bool          m_button_toggleable;

        // text properties        
        std::string   m_text;
        std::string   m_text_empty;
        Vec4F         m_text_clr;
        Vec4F         m_text_margin;
        float         m_text_size;
        uchar         m_text_placex, m_text_placey;
        Vec4F         m_text_pos;
        Vec4F         m_selection;

        // slider properties
        float         m_slider_val;

        // edit properties
        bool          m_isEditable;           
        Vec4F         m_edit_pos;     // x=char, y=start char, z=pixel offset

        // combo properties
        bool          m_isCombo;
        KeyValues     m_combo_list;
        int           m_combo_ndx;

        // icon properties
        ImageX*       m_icon;        
        ImageX*       m_icon_on;
        uchar         m_icon_scalex, m_icon_scaley;   // 0-100 => %
        uchar         m_icon_placex, m_icon_placey;
        Vec4F         m_icon_pos; 

    };


    }

#endif
