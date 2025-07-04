//--------------------------------------------------------------------------------
// NVIDIA(R) GVDB VOXELS
// Copyright 2017, NVIDIA Corporation. 
//
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
//    in the documentation and/or  other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
//    from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Version 1.0: Rama Hoetzlein, 5/1/2017
//----------------------------------------------------------------------------------

#define GLX_GLXEXT_PROTOTYPES

#ifdef _WIN32
  #define GLEW_NO_GLU
  #include<GL/glew.h>
#else
  #define GLEW_NO_GLU
  #include<GL/glew.h>     // DO NOT USE GL/glxew.h (causes segfault)
  #include<GL/glx.h>
  #include<GL/glxext.h>

  #include<X11/Xlib.h>
  #include<X11/Xatom.h>
  #include<X11/keysym.h>
  #include<X11/extensions/xf86vmode.h>
#endif

#include"main.h"
#include<stdio.h>
#include<fcntl.h>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<string>
#include<stdarg.h>
#include<unistd.h>
#include<sys/timeb.h>

Display *g_dpy = 0;

//std::vector<Application*> g_windows;

Application* pApp = 0x0;
int gArgc;
char** gArgv;

XEvent uMsg;

// OSWindow
// This structure contains Hardware/Platform specific variables
// that would normally go into main.h, but we want to be OS specific.
// Therefore to keep main.h cross-platform, we have a struct for OS-specific variables

struct OSWindow
{
    OSWindow(Application* app) : m_app(app), _dpy(NULL), _visual(NULL) { }
    Application*    m_app;
    int             m_screen;
    bool            m_visible; 
    
    GLXContext      _glxctx;
    GLXFBConfig     _glxfbc;
    Display*	    _dpy;
    Window          _window;
    XVisualInfo*    _visual;
    XF86VidModeModeInfo _mode;
    XSetWindowAttributes _xwinattr;
};



typedef GLXContext(*glXCreateContextAttribsARBProc)(Display *,GLXFBConfig, GLXContext,Bool, const int*);

static int attrListDbl[] = {
    GLX_RGBA,GLX_DOUBLEBUFFER,
    GLX_RED_SIZE,4,
    GLX_GREEN_SIZE,4,
    GLX_BLUE_SIZE,4,
    GLX_DEPTH_SIZE,16,0
};

 
static bool ctxErrorOccurred;
static int ctxErrorHandler(Display *dpy, XErrorEvent *evt){
    ctxErrorOccurred = true;
    return 0;
}

//------------------------------------------------------------------------------
// Debug Callback
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#ifdef _DEBUG
static void APIENTRY myOpenGLCallback(  GLenum source,
                        GLenum type,
                        GLuint id,
                        GLenum severity,
                        GLsizei length,
                        const GLchar* message,
                        const GLvoid* userParam)
{

  Application* window = (Application*)userParam;

  GLenum filter = window->m_debugFilter;
  GLenum severitycmp = severity;
  // minor fixup for filtering so notification becomes lowest priority
  if (GL_DEBUG_SEVERITY_NOTIFICATION == filter){
    filter = GL_DEBUG_SEVERITY_LOW_ARB+1;
  }
  if (GL_DEBUG_SEVERITY_NOTIFICATION == severitycmp){
    severitycmp = GL_DEBUG_SEVERITY_LOW_ARB+1;
  }

  if (!filter|| severitycmp <= filter )
  {
  
    //static std::map<GLuint, bool> ignoreMap;
    //if(ignoreMap[id] == true)
    //    return;
    char *strSource = "0";
    char *strType = strSource;
    switch(source)
    {
    case GL_DEBUG_SOURCE_API_ARB:
        strSource = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        strSource = "WINDOWS";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        strSource = "SHADER COMP.";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        strSource = "3RD PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        strSource = "APP";
        break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        strSource = "OTHER";
        break;
    }
    switch(type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB:
        strType = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        strType = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        strType = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        strType = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        strType = "Performance";
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        strType = "Other";
        break;
    }
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        LOGE("ARB_debug : %s High - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        LOGW("ARB_debug : %s Medium - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        LOGI("ARB_debug : %s Low - %s - %s : %s\n", window->m_debugTitle.c_str(), strSource, strType, message);
        break;
    default:
        //LOGI("ARB_debug : comment - %s - %s : %s\n", strSource, strType, message);
        break;
    }
  }
}

//------------------------------------------------------------------------------
void checkGL( char* msg )
{
    GLenum errCode;
    //const GLubyte* errString;
    errCode = glGetError();
    if (errCode != GL_NO_ERROR) {
        //printf ( "%s, ERROR: %s\n", msg, gluErrorString(errCode) );
        LOGE("%s, ERROR: 0x%x\n", msg, errCode );
    }
}

#else
//------------------------------------------------------------------------------
void checkGL( char* msg ) {}
#endif

static int sysGetKeyMods(XEvent &evt, unsigned int key) {

    // X windows is different..
    //   A mod key is detected either as an actual 'key' or as a mod 'event'
        
    int mods = 0;

    if( (evt.xkey.state & ShiftMask) || key==KEY_LEFT_SHIFT || key==KEY_RIGHT_SHIFT){
        mods |= KMOD_SHIFT;
    }
    if( (evt.xkey.state & ControlMask) || key==KEY_LEFT_CONTROL || key==KEY_RIGHT_CONTROL ){
        mods |= KMOD_CONTROL;
    }
    if( (evt.xkey.state & Mod1Mask) || key==KEY_LEFT_ALT || key==KEY_RIGHT_ALT ){
        mods |= KMOD_ALT;
    }
    return mods;
}

static int translateKey(XEvent &evt, bool& printableKey)
{
    printableKey = false;
    if(evt.type != KeyPress && evt.type != KeyRelease) return 0;

    unsigned int key = evt.xkey.keycode;

    KeySym ksym = XLookupKeysym(&evt.xkey,0);

    switch(ksym){
   //  XLib has separate key symbols for left and right  verions of mod keys. 
        case XK_Shift_L:      return KEY_LEFT_SHIFT;
        case XK_Shift_R:      return KEY_RIGHT_SHIFT;
        case XK_Control_L:    return KEY_LEFT_CONTROL;
        case XK_Control_R:    return KEY_RIGHT_CONTROL;
        case XK_Alt_L:        return KEY_LEFT_ALT;
        case XK_Alt_R:        return KEY_RIGHT_ALT;
        case XK_Return:       return KEY_ENTER;
        case XK_Escape:       return KEY_ESCAPE;
        case XK_Tab:          return KEY_TAB;
        case XK_BackSpace:    return KEY_BACKSPACE;
        case XK_Home:         return KEY_HOME;
        case XK_End:          return KEY_END;
        case XK_Prior:        return KEY_PAGE_UP;
        case XK_Next:         return KEY_PAGE_DOWN;
        case XK_Insert:       return KEY_INSERT;
        case XK_Delete:       return KEY_DELETE;
        case XK_Left:         return KEY_LEFT;
        case XK_Right:        return KEY_RIGHT;
        case XK_Up:           return KEY_UP;
        case XK_Down:         return KEY_DOWN;
        case XK_F1:           return KEY_F1;
        case XK_F2:           return KEY_F2;
        case XK_F3:           return KEY_F3;
        case XK_F4:           return KEY_F4;
        case XK_F5:           return KEY_F5;
        case XK_F6:           return KEY_F6;
        case XK_F7:           return KEY_F7;
        case XK_F8:           return KEY_F8;
        case XK_F9:           return KEY_F9;
        case XK_F10:          return KEY_F10;
        case XK_F11:          return KEY_F11;
        case XK_F12:          return KEY_F12;
        case XK_F13:          return KEY_F13;
        case XK_F14:          return KEY_F14;
        case XK_F15:          return KEY_F15;
        case XK_F16:          return KEY_F16;
        case XK_F17:          return KEY_F17;
        case XK_F18:          return KEY_F18;
        case XK_F19:          return KEY_F19;
        case XK_F20:          return KEY_F20;
        case XK_Num_Lock:     return KEY_NUM_LOCK;
        case XK_Caps_Lock:    return KEY_CAPS_LOCK;
        case XK_Scroll_Lock:  return KEY_SCROLL_LOCK;
        case XK_Pause:        return KEY_PAUSE;
        // Numeric Keypad
        case XK_KP_0:         return KEY_KP_0;
        case XK_KP_1:         return KEY_KP_1;
        case XK_KP_2:         return KEY_KP_2;
        case XK_KP_3:         return KEY_KP_3;
        case XK_KP_4:         return KEY_KP_4;
        case XK_KP_5:         return KEY_KP_5;
        case XK_KP_6:         return KEY_KP_6;
        case XK_KP_7:         return KEY_KP_7;
        case XK_KP_8:         return KEY_KP_8;
        case XK_KP_9:         return KEY_KP_9;
        case XK_KP_Divide:    return '/';
        case XK_KP_Multiply:  return '*';
        case XK_KP_Subtract:  return '-';
        case XK_KP_Add:       return '+';
        case XK_KP_Decimal:   return '.';
        case XK_space:        return ' ';

        default:
        break;
    }
//
    // Now processing printable keys
    //
    printableKey = true;
    switch(ksym)
    {
        case XK_0:            return '0';
        case XK_1:            return '1';
        case XK_2:            return '2';
        case XK_3:            return '3';
        case XK_4:            return '4';
        case XK_5:            return '5';
        case XK_6:            return '6';
        case XK_7:            return '7';
        case XK_8:            return '8';
        case XK_9:            return '9';
        case XK_a:            return 'a';
        case XK_b:            return 'b';
        case XK_c:            return 'c';
        case XK_d:            return 'd';
        case XK_e:            return 'e';
        case XK_f:            return 'f';
        case XK_g:            return 'g';
        case XK_h:            return 'h';
        case XK_i:            return 'i';
        case XK_j:            return 'j';
        case XK_k:            return 'k';
        case XK_l:            return 'l';
        case XK_m:            return 'm';
        case XK_n:            return 'n';
        case XK_o:            return 'o';
        case XK_p:            return 'p';
        case XK_q:            return 'q';
        case XK_r:            return 'r';
        case XK_s:            return 's';
        case XK_t:            return 't';
        case XK_u:            return 'u';
        case XK_v:            return 'v';
        case XK_w:            return 'w';
        case XK_x:            return 'x';
        case XK_y:            return 'y';
        case XK_z:            return 'z';
        case XK_minus:        return '-';
        case XK_equal:        return '=';
        case XK_bracketleft:  return '{';
        case XK_bracketright: return '}';
        case XK_backslash:    return '/';
        case XK_semicolon:    return ';';
        case XK_comma:        return ',';
        case XK_period:       return '.';
    }
    return KEY_UNKNOWN;
}

Application::Application() : m_renderCnt(1), m_win(0), m_debugFilter(0)
{
    dbgprintf("Application (constructor)\n");
    pApp = this;        // global handle
    m_mouseX = -1;
    m_mouseY = -1;
    m_mods = 0;
    m_fullscreen = false;
    m_startup = true;
    m_active = false;
    memset(m_keyPressed, 0, sizeof(m_keyPressed));
    memset(m_keyToggled, 0, sizeof(m_keyToggled));
}

bool Application::appStart (const std::string& title, const std::string& shortname, int width, int height, int Major, int Minor, int MSAA, bool GLDebug )
{
    m_winSz[0] = width;
    m_winSz[1] = height;

    m_cflags.major = Major;         // desired OpenGL settings
    m_cflags.minor = Minor;
    m_cflags.MSAA = MSAA;
    m_cflags.debug = GLDebug;

    m_active = false;                // not yet active
    m_title = title;

    return true;
}

bool Application::appStartWindow ( void* arg1, void* arg2, void* arg3, void* arg4 )
{
    if (m_win == 0x0 ) {
       m_win = new OSWindow(this);      // OS-Specific Win Handles
    }
    
    //-- Open Display
    m_win->_dpy = XOpenDisplay(0);
    char *dpyName = XDisplayName(NULL);

    static int visual_attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE,8,
        GLX_GREEN_SIZE,8,
        GLX_BLUE_SIZE,8,
        GLX_ALPHA_SIZE,8,
        GLX_DEPTH_SIZE,24,
        //GLX_STENCIL_SIZE,8,
	//        GLX_SAMPLE_BUFFERS,1,
        //GLX_SAMPLES,8,
        GLX_DOUBLEBUFFER, True,
        None
    };
    int e1, e2;
    bool ret = glXQueryExtension( m_win->_dpy, &e1, &e2);
    if ( !ret ) { dbgprintf ( "No GLX available.\n" ); return false; } 
    int glxMajor,glxMinor;    
    dbgprintf( "DPY : %p \n", m_win->_dpy);
    int fbCount = 0; 
    GLXFBConfig *fbc = glXChooseFBConfig ( m_win->_dpy, DefaultScreen(m_win->_dpy), visual_attribs, &fbCount);
    if(!fbc) { dbgprintf("Could not get FB config.\n"); return false; }

    //-- Find display config
    int bestfbc = -1,worstfbc = -1, best_num_sample = -1, worst_num_sample = 999;
    int i;
    for(i=0;i<fbCount;++i){
        XVisualInfo *vi = glXGetVisualFromFBConfig(m_win->_dpy,fbc[i]);
        if(vi){
                int sampleBuf, samples;
                glXGetFBConfigAttrib(m_win->_dpy,fbc[i],GLX_SAMPLE_BUFFERS, &sampleBuf);
                glXGetFBConfigAttrib(m_win->_dpy,fbc[i],GLX_SAMPLES,&samples);
                if(bestfbc < 0 || sampleBuf && samples > best_num_sample) bestfbc = i,best_num_sample = samples;
                if(worstfbc < 0 || !sampleBuf || samples < worst_num_sample) worstfbc = i, worst_num_sample = samples;
         }
        XFree(vi);
    }
    m_win->_glxfbc = fbc[bestfbc];
    XFree(fbc);
    XVisualInfo* vi = glXGetVisualFromFBConfig(m_win->_dpy, m_win->_glxfbc);

    XSetWindowAttributes swa;
    Colormap cmap;
    swa.colormap = cmap = XCreateColormap(m_win->_dpy,RootWindow(m_win->_dpy,vi->screen),vi->visual,AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
    swa.override_redirect = False;

    int width = m_winSz[0];
    int height = m_winSz[1];
    dbgprintf( "Creating Window. Width: %d Height: %d.\n", width, height );
    m_win->_window = XCreateWindow(m_win->_dpy, RootWindow(m_win->_dpy, vi->screen),0,0,width,height,0,vi->depth,InputOutput,vi->visual, CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,&swa);
    sleep(1);
    
    // select events to catch
    XSelectInput( m_win->_dpy, m_win->_window, ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask);

    
    XFree(vi);
    char title[1024];
    strncpy(title, m_title.c_str(), 1024);
    XSetStandardProperties(m_win->_dpy, m_win->_window, title, "", None, NULL, 0, NULL);
    Atom wmDelete = XInternAtom(m_win->_dpy,"WM_DELETE_WINDOW",True);
    XSetWMProtocols(m_win->_dpy, m_win->_window,&wmDelete,1);

    XMapRaised (m_win->_dpy, m_win->_window);
    XFlush (m_win->_dpy);
    
    
    #ifdef USE_OPENGL

    //-- Create glXContext 
    const char *glxExts = glXQueryExtensionsString(m_win->_dpy,DefaultScreen(m_win->_dpy));
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB  = (glXCreateContextAttribsARBProc) glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB"); 
    ctxErrorOccurred = false;
    int (*oldHandler)(Display *, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);
    int contextattribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, m_cflags.major,
        GLX_CONTEXT_MINOR_VERSION_ARB, m_cflags.minor,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };    
    //  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB   -- not
    
    m_win->_glxctx = glXCreateContextAttribsARB(m_win->_dpy, m_win->_glxfbc, 0, True, contextattribs);

    XSetErrorHandler(oldHandler);
    if(!glXMakeCurrent(m_win->_dpy, m_win->_window, m_win->_glxctx)){
        printf("Error making glx context current.\n");
    }

    //-- Initialize Glew
    GLenum glewErr = glewInit();

    if (GLEW_OK != glewErr){
      printf("Error initialising glew: %s.\n",glewGetErrorString(glewErr));
    }
           
    #endif
    
    appInitGL();

    //-- App init (ONCE)
    if (m_startup) {                // Call user init() only ONCE per application
        m_startup = false;
        dbgprintf("  init()\n");
        if (!init()) { dbgprintf("ERROR: Unable to init() app.\n"); return false; }
    }       

    // Activate (starts glSwapBuffers)
    if (!activate(pApp->m_winSz[0], pApp->m_winSz[1])) { dbgprintf("ERROR: Activate failed.\n"); return false; }
    m_active = true;                // yes, now active.
    return true;
}

void Application::appQuit ()
{
    Atom wmDelete = XInternAtom( m_win->_dpy, "WM_DELETE_WINDOW", False);

    XEvent event = {};
    event.xclient.type = ClientMessage;
    event.xclient.message_type = XInternAtom( m_win->_dpy, "WM_PROTOCOLS", True);
    event.xclient.display = m_win->_dpy;
    event.xclient.window = m_win->_window;
    event.xclient.format = 32;
    event.xclient.data.l[0] = wmDelete;
    event.xclient.data.l[1] = CurrentTime;

    XSendEvent( m_win->_dpy,  m_win->_window, False, NoEventMask, &event);
    XFlush( m_win->_dpy );

}

void Application::appResizeWindow(int w, int h)
{
    XResizeWindow ( m_win->_dpy, m_win->_window, w, h );
    XFlush ( m_win->_dpy );
}

bool Application::appInitGL()
{    
    #ifdef USE_OPENGL
        // additional opengl initialization
        //  (primary init of opengl occurs in WINinteral::initBase)
        initBasicGL();

        glFinish();
    #endif

    return true;
}


bool Application::appPollEvents ()
{
    AppEnum btn;
    bool done = false;
    bool printableKey = true;
    XEvent event;
    static short mouseWheelScale = 5;
   

    while( XPending(m_win->_dpy) > 0) {

	XNextEvent( m_win->_dpy, &event);
	
	

	switch(event.type)
        {
		case Expose:            break;
		case GraphicsExpose:    break;
		case ConfigureNotify: {
		    int sw = event.xconfigure.width;
		    int sh = event.xconfigure.height;
		    pApp->reshape ( sw, sh );
		    pApp->setWinSz ( sw, sh );
		    } break;
		case ButtonPress: {
		    switch ( event.xbutton.button ) {
		    case Button1: btn = AppEnum::BUTTON_LEFT; break;
		    case Button2: btn = AppEnum::BUTTON_MIDDLE; break;
		    case Button3: btn = AppEnum::BUTTON_RIGHT; break;		    
    		    case Button4: pApp->mousewheel( 100 ); return true;
    		    case Button5: pApp->mousewheel( -100 ) ; return true;
    		    };
		    pApp->appUpdateMouse ( event.xbutton.x, event.xbutton.y, btn, AppEnum::BUTTON_PRESS );
		    pApp->mouse ( btn, AppEnum::BUTTON_PRESS, pApp->getMods(), pApp->getX(), pApp->getY() );
		    } break;
		case ButtonRelease:{
		    switch ( event.xbutton.button ) {
		    case Button1: btn = AppEnum::BUTTON_LEFT; break;
		    case Button2: btn = AppEnum::BUTTON_MIDDLE; break;
		    case Button3: btn = AppEnum::BUTTON_RIGHT; break;
		    };
		    pApp->appUpdateMouse ( event.xbutton.x,event.xbutton.y, btn, AppEnum::BUTTON_RELEASE );
		    pApp->mouse ( btn, AppEnum::BUTTON_RELEASE, pApp->getMods(), pApp->getX(), pApp->getY() );
		    pApp->m_mouseButton = AppEnum::BUTTON_NONE;
		    } break;
	       case MotionNotify: {
		    float dx = pApp->getX() - event.xmotion.x;
		    float dy = pApp->getY() - event.xmotion.y;
		    pApp->appUpdateMouse ( event.xmotion.x,event.xmotion.y );
		    pApp->motion ( pApp->m_mouseButton, pApp->getX(), pApp->getY(), pApp->getDX(), pApp->getDY() );
		      } break;
	       case KeyPress:{
		    int key = translateKey(event, printableKey);
		    pApp->setMods( sysGetKeyMods(event, key) );
		    pApp->appSetKeyPress( key, true );
		    pApp->keyboard ( key, AppEnum::BUTTON_PRESS, pApp->getMods(), pApp->getX(), pApp->getY() );		    
		    } break;
		case KeyRelease:{		    
		    int key = translateKey(event, printableKey);
		    pApp->setMods( sysGetKeyMods(event, key) );
		    pApp->appSetKeyPress( key, false );		    
		    pApp->keyboard ( key, AppEnum::BUTTON_RELEASE, pApp->getMods(), pApp->getX(), pApp->getY() );
		    } break;
		case ClientMessage:
		    if( strcmp(XGetAtomName( m_win->_dpy, event.xclient.message_type ),"WM_PROTOCOLS") == 0){
		        pApp->m_running = false;
		    }
		 break;            
	}
   }
   XSync(m_win->_dpy, True);
   
   return true;
}


void Application::appSetKeyPress(int key, bool state)
{
    m_keyToggled[key] = (m_keyPressed[key] != state);
    m_keyPressed[key] = state;
}

void Application::appRun()
{
   if (pApp != 0x0) {
   
       appPollEvents();         // UI input
   
       if (pApp->m_active && pApp->m_renderCnt > 0) {
           pApp->m_renderCnt--;
           pApp->display();     // render
           appSwapBuffers();    // gl swap
       }
   }
}

// update mouse - position only
void Application::appUpdateMouse(float mx, float my){
    m_lastX = m_mouseX;
    m_lastY = m_mouseY;
    m_dX = (m_dX == -1) ? 0 : m_mouseX - mx;
    m_dY = (m_dY == -1) ? 0 : m_mouseY - my;
    m_mouseX = mx;
    m_mouseY = my;
}
// update mouse - with button or state change
void Application::appUpdateMouse(float mx, float my, AppEnum button, AppEnum state)
{
    appUpdateMouse(mx, my);
    if (button != AppEnum::UNDEF) {             // button changed
        m_mouseButton = button;
        m_startX = mx; m_startY = my;
        m_dX = -1; m_dY = -1;        // indicator to ignore the first motion update
    }
    if (state != AppEnum::UNDEF) {              // state changed
        m_mouseState = state;
        m_startX = mx; m_startY = my;
        m_dX = -1; m_dY = -1;
    }
}

bool Application::appStopWindow()
{
    //**** TO DO
    
    return true;

}
void Application::appShutdown()
{
    // perform user shutdown() first
    shutdown();

    // destroy Windows & OpenGL surfaces
    appStopWindow();
}



void Application::appSwapBuffers()
{
   glXSwapBuffers (m_win->_dpy, m_win->_window );
}

void Application::appHandleArgs(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {                // valued argument> app -i input.txt
            on_arg(i, argv[i], argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-vsync") == 0 && i + 1 < argc) {        // builtin> app -vsync 1
            bool vsync = atoi(argv[i + 1]) ? true : false;
            appSetVSync(vsync);
            i++;
        } else {
            on_arg(i, argv[i], "");                // non-valued arg> app input.txt
        }        
    }
}

void Application::appSetVSync(bool vsync)
{

}


int main(int argc, char **argv)
{
  dbgprintf ("Starting.\n");
  
  Display *dpy = XOpenDisplay(0); 
  int nelements;
  GLXFBConfig *fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), 0, &nelements);
  std::string exe = std::string(argv[0]);
  std::replace(exe.begin(),exe.end(),'\\','/');
  size_t last = exe.rfind('/');

  pApp->startup();                        //-- App startup
    
  pApp->appHandleArgs( argc, argv );    //-- App handles args
  
  pApp->appStartWindow ( 0, 0, 0, 0 );
  
  pApp->m_running = true;   
  pApp->m_active = true;

  dbgprintf ("Running.\n" );

  while (pApp->m_running) {

      pApp->appRun();     //-- Run app

      if (pApp->getKeyPress(KEY_ESCAPE)) {            //-- ESC key
          // Post close message for proper shutdown
          dbgprintf("ESC pressed.\n");
          pApp->m_running = false;
      }
  }    

  pApp->appShutdown();
  
  return 0;
}



