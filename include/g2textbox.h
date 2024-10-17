//--------------------------------------------------
//
// giTextBox
// 
// Quanta Sciences (c) 2023
// Rama Hoetzlein
//
//--------------------------------------------------


#ifndef DEF_GI_TEXTBOX
    #define DEF_GI_TEXTBOX
    
    #include "g2obj.h"
    #include "imagex.h"
  
    namespace glib {

    class GXAPI g2TextBox : public g2Obj {
    public:   
        g2TextBox ();

        virtual uchar getType()     { return 't'; }
        virtual void UpdateLayout ( Vec4F p ); 
        virtual void SetProperty ( std::string key, std::string val );
        virtual void drawBackgrd (bool dbg);
        virtual void drawBorder (bool dbg);
        virtual void drawForegrd (bool dbg);

        std::string getPrintedText(Vec4F& clr);

        void LayoutIcon ();
        void LayoutText ();

        std::string   m_text;
        std::string   m_text_empty;
        Vec4F         m_text_clr;
        Vec4F         m_text_margin;
        float         m_text_size;
        uchar         m_text_placex, m_text_placey;
        Vec4F         m_text_pos;

        ImageX*       m_icon;        
        uchar         m_icon_scalex, m_icon_scaley;   // 0-100 => %
        uchar         m_icon_placex, m_icon_placey;
        Vec4F         m_icon_pos; 

    };


    }

#endif
