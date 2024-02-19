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
    
    #include "g2_obj.h"
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

        std::string   m_text;
        Vec4F         m_textclr;
        float         m_textsz;

        ImageX*       m_icon;

    };


    }

#endif
