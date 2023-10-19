
#include <assert.h>
#include <time.h>
#include "timex.h"
#include "common_defs.h"
#include "string_helper.h"

#include "g2lib.h"
#include "g2_grid.h"
#include "g2_textbox.h"


using namespace glib;

// Global singleton
glib::g2Lib  glib::g2;


void g2Lib::LoadSpec ( std::string fname )
{
    std::string fpath;

    if ( !getFileLocation ( fname, fpath ) ) {
        printf ( "ERROR: Unable to find file %s\n", fname.c_str() );
        exit(-1);
    }
    char filepath[1024];
    strncpy ( filepath, fpath.c_str(), 1024 );
    
    FILE* fp = fopen ( filepath, "rt" );
    if ( fp==0x0 ) {
        printf ( "ERROR: Unable to open file %s\n", fname.c_str() );
        exit(-1);
    }
    char buf[2048];
    std::string lin, word;
    std::vector<std::string> words;

    while (fgets( buf, 2048, fp)) {
        
        lin = strTrim(buf) + "|";       // strip '\n' and add |

        // parse words into basic sentence
        words.clear ();
        for (int n=0; lin.length() > 0; n++) {
            word = strTrim( strSplitLeft ( lin, "|" ) );
            if ( word.length() > 0 )
                if ( n <= 3 )
                    words.push_back ( word );            
                else
                    words[3] = words[3] + "|"+word;
        }
        if (words.size() > 0 ) {
            // ensure there are 4
            while (words.size()<4)
                words.push_back ("X");
            // ensure vals have terminal |
            words[3] = words[3] + "|";

            // build object spec
            if (words[1].compare ( "is a")==0) {
                SetDef ( words[0], words[2] );
            } 
            if (words[1].compare ( "has")==0) {
                SetKeyVal ( words[0], words[2], words[3] );
            }
        }
    }    

    // print object specs
    /*printf ( "---- SPEC\n" );
    for (int n=0; n < m_objdefs.size(); n++) {
        printf ( "%d: %s [%s]\n", n, m_objdefs[n].obj.c_str(), m_objdefs[n].isa.c_str() );
        for (int j=0; j < m_objdefs[n].keys.size(); j++) {
            printf ("  %s: %s\n", m_objdefs[n].keys[j].c_str(), m_objdefs[n].vals[j].c_str() );
        }
    }
    printf ( "----\n" ); */

    fclose(fp);
}


void g2Lib::SetDef ( std::string name, std::string isa )
{
    g2Def* def = FindDef (name);
    if (!def) {
        g2Def newdef;
        newdef.obj = name;
        newdef.isa = isa;
        m_objdefs.push_back ( newdef );
    }
} 

void g2Lib::SetKeyVal ( std::string name, std::string key, std::string val )
{
    g2Def* def = FindDef (name);
    if (def) {
        def->keys.push_back ( key );
        def->vals.push_back ( val );
    } else {
        printf ("ERROR: %s not defined yet.\n", name.c_str() );
    }
}

std::string g2Lib::getVal ( std::string name, std::string key )
{
    g2Def* def = FindDef(name);
    if (def) {
        for (int j=0; j < def->keys.size(); j++) {
            if ( def->keys[j].compare( key ) == 0)
                return def->vals[j];
        }        
    }
    return "";  // not found
}

g2Def* g2Lib::FindDef ( std::string name )
{
    for (int n=0; n < m_objdefs.size(); n++) {
        if ( m_objdefs[n].obj.compare (name)==0) {
            return &m_objdefs[n];
        }
    }
    // not found
    return 0x0;
}

uchar g2Lib::FindType ( std::string isa )
{
    if ( isa.compare("grid")==0) return 'g';
    if ( isa.compare("textbox")==0) return 't';
    return '?';
}

g2Obj* g2Lib::FindObj ( std::string name )
{
    for (int n=0; n < m_objlist.size(); n++) {
        if ( m_objlist[n]->getName().compare (name)==0) {
            return m_objlist[n];
        }
    }
    // not found
    return 0x0;
}

g2Obj* g2Lib::AddObj ( std::string name, uchar typ )
{
    g2Obj* obj;
    // create object
    switch (typ) {
    case 'g': obj = new g2Grid; break;
    case 't': obj = new g2TextBox; break;
    default:
        return 0x0;
    };
    obj->m_name = name;        
    obj->m_backclr = Vec4F(0, 0, 0, 0);
    obj->m_borderclr = Vec4F(.8,.8,.8,1);

    // add to master list
    m_objlist.push_back ( obj );

    return obj;
}

void g2Lib::BuildAll ()
{
    std::string name, isa;
    uchar typ;
    g2Obj* obj;
    std::string cmd;

    // Pass 1 - build all objects first
    for (int n=0; n < m_objdefs.size(); n++) {

        // get name
        name = m_objdefs[n].obj;        

        // get type
        isa = m_objdefs[n].isa;
        typ = FindType ( isa );
        if ( typ=='?' ) {
            printf ( "ERROR: Unknown object type %s for %s\n", isa.c_str(), name.c_str() );
            exit(-2);
        }        
        // add object        
        obj = AddObj ( name, typ );

        printf ( "%d %p\n", n, obj );
    }

    // Pass 2 - build layouts, sections and obj references
    uchar L;
    for (int n=0; n < m_objlist.size(); n++) {

        // get object
        obj = m_objlist[n];

        // check if root
        cmd = getVal ( obj->getName(), "root" );
        if (cmd.size()>0) {
            if (cmd.at(0)=='X') m_root = n;
        }

        // apply layouts & sections for grids
        if ( obj->getType()=='g') {
           
            if ( BuildLayout ( obj, G_LX ) ) L = G_LX;
            if ( BuildLayout ( obj, G_LY ) ) L = G_LY;     

            BuildSections ( obj, L );            
        }
        
    }
}

bool g2Lib::BuildLayout ( g2Obj* obj, uchar ly )
{
    uchar typ;
    float amt;
    g2Size size;
    std::string key, val, sz;
    g2Grid* grid = dynamic_cast<g2Grid*>( obj );    
    if (grid==0x0) {
        printf ("ERROR: This is not a grid.\n");
        exit(-2);
    }

    // get layout on object
    switch ( ly ) {
    case G_LX: key = "X layout"; break;
    case G_LY: key = "Y layout"; break;
    };
    g2Layout* layout = grid->getLayout ( ly );
    if (layout==0x0) {
      printf ("ERROR: No layout found.\n");
      exit(-3);
    }

    // get layout spec from def
    val = getVal ( obj->getName(), key );   

    if ( val.size() == 0 ) 
        return false;

    layout->active = true;

    // a layout specifies the *size* of each section in a grid
    //    
    for (int n=0; val.length()>0; n++) {
        sz = strSplitLeft ( val, "|" );
        size.typ = '?';
        size.amt = 0;

        if ( sz.find('%') != std::string::npos ) {
            // percent found
            size.typ = '%';
            size.amt = strToF ( sz );
        } else if ( sz.find('p') != std::string::npos ) {
            // pixels found
            size.typ = 'x';
            size.amt = strToF ( sz );
        } else if ( sz.find('.') != std::string::npos ) {
            // repeat found
            size.typ = '.';
        } else if ( sz.find('*') != std::string::npos ) {
            // expand found            
            size.typ = '*';
        } else if ( sz.find('G') != std::string::npos ) {
            // grow found
            size.typ = 'G';
        }
        layout->sizes.push_back ( size );
    }
}


void g2Lib::BuildSections ( g2Obj* obj, uchar ly )
{
    g2Obj* obj_ref;
    std::string obj_name;
    std::string key, val, sz;
    g2Grid* grid = dynamic_cast<g2Grid*>( obj );   
    if (grid==0x0) {
        printf ("ERROR: This is not a grid.\n");
        exit(-2);
    }

    // get layout on object    
    g2Layout* layout = grid->getLayout ( ly );
     if (layout==0x0) {
      printf ("ERROR: No layout found.\n");
      exit(-3);
    }

    // get section spec from def
    val = getVal ( obj->getName(), "sections" );   

    if ( val.size() > 0 ) {

        // sections specify the *object* in each section of a grid
        //
        for (int n=0; val.length()>0; n++) {
            
            // find obj for this section
            obj_name = strSplitLeft ( val, "|" );            
            obj_ref = FindObj ( obj_name );

            if (obj_ref==0x0 && obj_name.compare(".") != 0) {
                // object not found, and is not a wildcard '.',
                // so lets create a placeholder for kindness
                obj_ref = AddObj ( obj_name, 't' );
                obj_ref->m_backclr = Vec4F(0.2,0.2,0.2,1);
            }          
            
            //printf ("%s (%p) ref by %s (%p)\n", obj_name.c_str(), obj_ref, obj->m_name.c_str(), obj );

            layout->sections.push_back ( obj_ref );
        }

    }
}

void g2Lib::LayoutAll (float xres, float yres)
{
    g2Obj* curr = m_objlist[ m_root ];
    
    curr->UpdateLayout ( Vec4F(0, 0, xres, yres) );
}


void g2Lib::Render (int w, int h)
{
    start2D( w, h );
    
    g2Obj* curr = m_objlist[ m_root ];

    setview2D ( curr->m_pos.z, curr->m_pos.w ); 

    curr->Render ( 'b' ); 
    curr->Render ( 'r' ); 
    curr->Render ( 'f' ); 

    end2D();
}
         