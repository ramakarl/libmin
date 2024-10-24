
//-------- MAIN ANDROID - We are on android platform here

#include <jni.h>

#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "gxlib.h"

#include "main.h"

// Global abstract pointer to the application
Application* pApp = 0x0;

// OSWindow
// This structure contains Hardware/Platform specific variables
// that would normally go into main.h, but we want to be OS specific.
// Therefore to keep main.h cross-platform, we have a struct for OS-specific variables

struct OSWindow
{
    OSWindow (Application* app) : m_app(app), m_screen(0), m_visible(true), _awindow(0), _javavm(0), _javaGlobalObject(0), _javaGlobalClass(0),
                                 _display(0), _surface(0), _context(0) {}    // empty constructor

    Application*    m_app;              // handle to the owner application
    int             m_screen;
    bool            m_visible;
    ANativeWindow*  _awindow;
    JavaVM*         _javavm;
    jobject         _javaGlobalObject;
    jclass          _javaGlobalClass;
    EGLDisplay      _display;
    EGLSurface      _surface;
    EGLContext      _context;
    EGLint          _width;
    EGLint          _height;
};

//------------------------------------------------ ANDROID ENTRY POINTS

// These functions have interfaces to the Java/JNI
// Any function which is visible to Java/JNI will have the 'native' prefix. (NOT all functions)

std::string getStrFromJString (JNIEnv* env, jstring str)
{
    if ( !str ) std::string();

    const jsize len = env->GetStringUTFLength(str);
    const char* strChars = env->GetStringUTFChars(str, (jboolean *)0);
    std::string result(strChars, len);
    env->ReleaseStringUTFChars(str, strChars);
    return result;
}
jstring getJStringFromStr (JNIEnv* env, std::string str)
{
    char cstr[1024];
    strcpy ( cstr, str.c_str() );
    return env->NewStringUTF( cstr );
}

extern "C"
{
    #include <android/native_window.h>
    #include <android/native_window_jni.h>
    #include <android/surface_control.h>

    bool nativeDestroyEGL ( )
    {
      dbgprintf ( "nativeDestroyGL()\n");

      dbgprintf ( "  nativeDestroyGL: appStopWindow.\n");
      pApp->appStopWindow();      // sets active=false, stops eglSwapBuffers

      OSWindow* win = pApp->m_win;  
      if (win==0x0) return false;

      if (win->_display == EGL_NO_DISPLAY) return true;

      dbgprintf( "  nativeDestroyGL: Clear context. eglMakeCurrent.\n");
      eglMakeCurrent(win->_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

      if (win->_context != EGL_NO_CONTEXT) {
        dbgprintf("  nativeDestroyGL: Destroy context. eglDestroyContext.\n");
        eglDestroyContext(win->_display, win->_context);
      }
      if (win->_surface != EGL_NO_SURFACE) {
        dbgprintf("  nativeDestroyGL: Destroy surface. eglDestroySurface.\n");
        eglDestroySurface(win->_display, win->_surface);
      }
      dbgprintf("  nativeDestroyGL: Terminate. eglTerminate.\n");
      eglTerminate(win->_display);

      win->_display = EGL_NO_DISPLAY;
      win->_surface = EGL_NO_SURFACE;
      win->_context = EGL_NO_CONTEXT;

      return true;
    }

    bool nativeRebuildEGL ( ANativeWindow* window )
    {
      dbgprintf("nativeRebuildEGL()\n");     

      // Assign new window
      pApp->m_win->_awindow = window;     

      //-- Start OpenGL GLES 3.0. Rebuild surface & context.
      int wid, hgt;
      dbgprintf( "  appCreateGL.\n");
      if (!pApp->appCreateGL(&pApp->m_cflags, wid, hgt)) {
        dbgprintf("ERROR: appCreateGL failed.\n");
        return false;
      }
      pApp->m_winSz[0] = wid;
      pApp->m_winSz[1] = hgt;      

      //-- App initialization (ENSURE ONE TIME)
      if (pApp->m_startup) {      // Call user init() only ONCE per application
        pApp->m_startup = false;
        dbgprintf("  appInitGL().\n");
        pApp->appInitGL();
        dbgprintf("  init()");  // Calls init2D
        if (!pApp->init()) { dbgprintf("ERROR: Unable to init() app.\n"); return false; }
      }

      //-- Start the app      
      pApp->appStartWindow();
    
      return true;
    }

    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeSurfaceCreated ( JNIEnv* env, jclass cself, jobject surface, jobject self)
    {
      ANativeWindow* window = ANativeWindow_fromSurface (env, surface);
      nativeRebuildEGL ( window );
    }

    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeSurfaceChanged (JNIEnv* env, jclass cself, jobject surface, jobject self)
    {
      glViewport ( 0, 0, pApp->m_winSz[0], pApp->m_winSz[1] );
    }
    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeSurfaceDestroyed (JNIEnv* env, jclass cself, jobject surface, jobject self)
    { 
      nativeDestroyEGL();
      if (pApp->m_win->_awindow) {
        ANativeWindow_release(pApp->m_win->_awindow);
        pApp->m_win->_awindow = 0;
      };
    }
    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeOnPause ( jclass cself )
    {
      nativeDestroyEGL();
      if (pApp->m_win->_awindow) {
        ANativeWindow_release(pApp->m_win->_awindow);
        pApp->m_win->_awindow = 0;
      };
    }    
    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeOnResume ( JNIEnv* env, jclass cself, jobject surf, jobject self )
    {          
    }
    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeStartup ( JNIEnv *env, jclass cself, jobject self)
    {

      // Create a new OSwindow container
      if (pApp->m_win == 0x0) {
        dbgprintf("  nativeRebuildGL: New OS Window containter.\n");
        pApp->m_win = new OSWindow(pApp);  // create os-specific variables first time
      }
      OSWindow* win = pApp->m_win;         // get OSWindow container

        // Get the Java VM from environment
      JavaVM* javaVm;
      env->GetJavaVM(&javaVm);

      // Get global objects
      jobject jo = reinterpret_cast<jobject>(env->NewGlobalRef(self));
      jclass jc = reinterpret_cast<jclass>(env->NewGlobalRef(cself));

      // Assign Android window & objects
      dbgprintf("  nativeRebuildGL: Assigning Java objects.\n");      
      win->_javavm = (JavaVM*)javaVm;
      win->_javaGlobalObject = jo;
      win->_javaGlobalClass = jc;
      win->_awindow = 0x0;

      // App startup()
      pApp->startup (); 
    }

    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeRun ( JNIEnv *env, jclass cself)
    {
      pApp->appRun();
    }

    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativePassEvent ( JNIEnv* env, jclass cself, jint type, jfloatArray vals )
    {
        guiEvent g;
        //jsize len = env->GetArrayLength(vals);
        jfloat *pvals = env->GetFloatArrayElements(vals, 0);

        g.typeOrdinal = type;
        g.xtarget = pvals[EVT_XTARGET];
        g.ytarget = pvals[EVT_YTARGET];
        g.xfocus = pvals[EVT_XFOCUS];
        g.yfocus = pvals[EVT_YFOCUS];
        g.xspan = pvals[EVT_XSPAN];
        g.yspan = pvals[EVT_YSPAN];

        pApp->appHandleEvent( g );
    }

    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeLoadAssets ( JNIEnv* env, jclass cself, jobject assetManager, jstring sdpath )
    {
        //--- This function unpacks the .apk asset folder to a set of files on the Android device
        dbgprintf ( "Unpacking assets.\n");
        AAssetManager* mgr = AAssetManager_fromJava( env, assetManager );
        AAssetDir* assetDir = AAssetManager_openDir( mgr, "");
        const char* filename = (const char*)NULL;
        char filepath[1024];

        std::string path = getStrFromJString ( env, sdpath );
        strcpy ( filepath, path.c_str() );
        addSearchPath ( filepath );         // add asset path to searchable paths

        while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
            sprintf ( filepath, "%s/%s", path.c_str(), filename );
            dbgprintf ( "  File: %s --> %s\n", filename, filepath );

            AAsset* asset = AAssetManager_open( mgr, filename, AASSET_MODE_STREAMING);
            char buf[BUFSIZ];
            int nb_read = 0;

            FILE* out = fopen( filepath, "w" );
            if ( out == NULL ) {
                dbgprintf ( "ERROR: Unable to write to asset file %s.\n", filepath );
                exit(-1);
            }
            while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
                fwrite(buf, nb_read, 1, out);
            fclose(out);

            AAsset_close(asset);
        }
        AAssetDir_close(assetDir);
    }

    JNIEXPORT void JNICALL
    Java_com_quantasciences_qtvc_MainActivity_nativeCleanup(JNIEnv *env, jclass cself)
    {
        dbgprintf ( "nativeCleanup\n");

        // shutdown native stuff, also calls user shutdown()
        dbgprintf(  "  appShutdown\n");
        pApp->appShutdown ();

        // destroy global references        
        env->DeleteGlobalRef( pApp->m_win->_javaGlobalClass );
        env->DeleteGlobalRef( pApp->m_win->_javaGlobalObject );
    }

}

//------------------------------------------------ Application

Application::Application() : m_renderCnt(1), m_win(0), m_debugFilter(0)
{
    dbgprintf ( "Application (constructor)\n" );
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

// appStart - called from the user function startup() to indicate desired application config
bool Application::appStart ( const std::string& title, const std::string& shortname, int width, int height, int Major, int Minor, int MSAA, bool GLDebug  )
{
    dbgprintf("appStart");

    bool vsyncstate = true;

    m_winSz[0] = width;             // desired width & height, may not be actual/final
    m_winSz[1] = height;

    m_cflags.major = Major;         // desired OpenGL version
    m_cflags.minor = Minor;

    m_active = false;                // not yet active

    return true;
}

bool Application::appStartWindow (void* arg1, void* arg2, void* arg3, void* arg4)
{
    dbgprintf("  activate()\n");  
    if ( !activate(pApp->m_winSz[0], pApp->m_winSz[1]) ) { dbgprintf ( "ERROR: Activate failed.\n"); return false; }

    m_active = true;                // yes, now active.

    return true;
}

bool Application::appCreateGL (const Application::ContextFlags *cflags, int& width, int& height)
{  
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

    EGLDisplay display;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    GLfloat ratio;

    if (m_win->_awindow == 0x0) {
        dbgprintf("ERROR: Window is null. Cannot start app.");
        return false;
    }
    dbgprintf("    Initializing context");

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        dbgprintf("ERROR: eglGetDisplay() returned error %d", eglGetError());
        return false;
    }
    if (!eglInitialize(display, 0, 0)) {
        dbgprintf("ERROR: eglInitialize() returned error %d", eglGetError());
        return false;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        dbgprintf("ERROR: eglChooseConfig() returned error %d", eglGetError());
        return false;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        dbgprintf("ERROR: eglGetConfigAttrib() returned error %d", eglGetError());
        return false;
    }

    dbgprintf("    Creating GL Surface");
    ANativeWindow_setBuffersGeometry(m_win->_awindow, 0, 0, format);

    if (!(surface = eglCreateWindowSurface(display, config, m_win->_awindow, 0))) {
        dbgprintf("ERROR: eglCreateWindowSurface() returned error %d", eglGetError());
        return false;
    }

    dbgprintf("    Creating GL Context");

    if (!(context = eglCreateContext(display, config, EGL_NO_CONTEXT, attributes ))) {
        dbgprintf("ERROR: eglCreateContext() returned error %d", eglGetError());
        return false;
    }

    dbgprintf("    Making Context Current");
    if (!eglMakeCurrent(display, surface, surface, context)) {
        dbgprintf("ERROR: eglMakeCurrent() returned error %d", eglGetError());
        return false;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &m_win->_width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &m_win->_height)) {
        dbgprintf("ERROR: eglQuerySurface() returned error %d", eglGetError());
        return false;
    }

    m_win->_display = display;
    m_win->_surface = surface;
    m_win->_context = context;

    dbgprintf("    Set Viewport");
    glViewport(0, 0, m_win->_width, m_win->_height);

    // tell parent about new width & height of android device
    width = m_win->_width;
    height = m_win->_height;

    return true;
}

bool Application::appInitGL()
{
    dbgprintf ( "appInitGL\n" );
    // additional opengl initialization
    //  (primary init of opengl occurs in WINinteral::initBase)
    //  initScreenQuadGL ();
    return true;
}

// appStopWindow
// - this function is called when the application loses focus or activity
// for example, on android when the app is backgrounded but not closed
bool Application::appStopWindow ()
{
    dbgprintf ( "  appStopWindow\n" );

    m_active = false;                 // No longer active

    dbgprintf("  deactivate()");        // Call user deactivate() each time window/surface is destroyed
    if (!deactivate()) { dbgprintf("ERROR: Deactivate failed.\n"); return false; }
    
    return true;
}

void Application::appShutdown ()
{
    // perform user shutdown() first
    shutdown();

    // destroy OpenGL surfaces
    appStopWindow();
}

void Application::appRun ()
{
    if ( pApp != 0x0 ) {
        if ( pApp->m_active && pApp->m_renderCnt > 0) {
            pApp->m_renderCnt--;

            // mouse glide
            if ( m_mouseButton == AppEnum::BUTTON_LEFT && m_mouseState == AppEnum::BUTTON_RELEASE ) {
//              appHandleEvent ( guiEvent(AppEnum::BUTTON_LEFT, AppEnum::BUTTON_RELEASE, m_mouseX, m_mouseY, m_dX, m_dY, 0, 0) );
                m_dX *= 0.95; m_dY *= 0.95;
                motion( m_mouseButton, m_mouseX, m_mouseY, m_dX, m_dY );
                if ( fabs(m_dX) < 1 && fabs(m_dY) < 1 ) m_mouseButton = AppEnum::BUTTON_NONE;
                //dbgprintf("Event: GLD, b:%d,%d x:%f, y:%f, dx:%f, dy:%f\n", m_mouseButton, m_mouseState, m_mouseX, m_mouseY, m_dX, m_dY);
            }

            pApp->display();            // Render user display() function
            appSwapBuffers();           // Swap buffers
        }
    }
}

void Application::appSwapBuffers()
{
    if (!eglSwapBuffers( m_win->_display, m_win->_surface) ) {
        dbgprintf( "ERROR: eglSwapBuffers() returned error %d", eglGetError() );
    }
}

bool Application::isActive()
{
    return m_active;
}
void Application::appSwapInterval(int i)
{
}

// update mouse - position only
void Application::appUpdateMouse ( float mx, float my )
{
    m_lastX = m_mouseX;
    m_lastY = m_mouseY;
    m_dX = (m_dX==-1) ? 0 : m_mouseX - mx;
    m_dY = (m_dY==-1) ? 0 : m_mouseY - my;
    m_mouseX = mx;
    m_mouseY = my;
}
// update mouse - with button or state change
void Application::appUpdateMouse ( float mx, float my, AppEnum button, AppEnum state )
{
    appUpdateMouse ( mx, my );
    if ( button != AppEnum::UNDEF ) {             // button changed
        m_mouseButton = button;
        m_startX = mx; m_startY = my;
    }
    if ( state != AppEnum::UNDEF ) {              // state changed
        m_mouseState = state;
        m_startX = mx; m_startY = my;
    }
    if ( state == AppEnum::BUTTON_PRESS || (state==AppEnum::BUTTON_RELEASE && button==AppEnum::BUTTON_RIGHT) ) {
        m_dX = -1; m_dY = -1;          // indicator to ignore the first motion update
    }
}

void Application::appSetKeyPress(int key, bool state)
{
    // not currently getting keyup for soft keyboard events
    // this will have to be changed to deal with hard keys

    // m_keyToggled[key] = (m_keyPressed[key] != state);
    // m_keyPressed[key] = state;
}

void Application::appHandleEvent (guiEvent g)
{
    switch ( g.typeOrdinal ){
        case AppEnum::ACTION_DOWN:
            if ( m_mouseButton == AppEnum::BUTTON_RIGHT ) return;
            appUpdateMouse( g.xtarget, g.ytarget, AppEnum::BUTTON_LEFT, AppEnum::BUTTON_PRESS );
            mouse( m_mouseButton, m_mouseState, 0, m_mouseX, m_mouseY );
            break;
        case AppEnum::ACTION_MOVE: {
            if ( m_mouseButton == AppEnum::BUTTON_RIGHT ) return;
            appUpdateMouse( g.xtarget, g.ytarget );
            motion( m_mouseButton, m_mouseX, m_mouseY, m_dX, m_dY );
            } break;
        case AppEnum::ACTION_UP: {
            if ( m_mouseButton == AppEnum::BUTTON_RIGHT ) return;
            float dx = m_dX, dy = m_dY;     // start glide
            appUpdateMouse( g.xtarget, g.ytarget , AppEnum::BUTTON_LEFT, AppEnum::BUTTON_RELEASE );
            mouse ( m_mouseButton, m_mouseState, 0, m_mouseX, m_mouseY );
            m_dX = dx; m_dY = dy;
     //     m_mouseButton = AppEnum::BUTTON_NONE;    //-- glide will turn it off
            } break;
        case AppEnum::ACTION_CANCEL:
            if ( m_mouseButton == AppEnum::BUTTON_RIGHT ) return;
            appUpdateMouse( g.xtarget, g.ytarget , AppEnum::BUTTON_LEFT, AppEnum::BUTTON_RELEASE );
            mouse ( m_mouseButton, m_mouseState, 0, m_mouseX, m_mouseY );
            m_mouseButton = AppEnum::BUTTON_NONE;
            break;
        case AppEnum::GESTURE_SCALE_BEGIN:          // start zoom
            appUpdateMouse( g.xtarget, g.ytarget, AppEnum::BUTTON_RIGHT, AppEnum::BUTTON_PRESS );
            m_spanX = g.xspan;
            m_spanY = g.yspan;
            mouse( m_mouseButton, m_mouseState, 0, m_mouseX, m_mouseY );
            break;
        case AppEnum::GESTURE_SCALE: {       // handle zoom
            bool ignore_first = (m_dX==-1 && m_dY==-1);
            appUpdateMouse( g.xtarget, g.ytarget );
            m_dX = ignore_first ? 0 : g.xspan - m_spanX;
            m_dY = ignore_first ? 0 :g.yspan - m_spanY;
            m_spanX = g.xspan;
            m_spanY = g.yspan;
            motion( m_mouseButton, m_mouseX, m_mouseY, m_dX, m_dY );
            } break;
        case AppEnum::GESTURE_SCALE_END:        // end zoom
            appUpdateMouse(g.xtarget, g.ytarget, AppEnum::BUTTON_RIGHT, AppEnum::BUTTON_RELEASE);
            mouse( m_mouseButton, m_mouseState, 0, m_mouseX, m_mouseY );
            m_mouseButton = AppEnum::BUTTON_NONE;
            break;
        case AppEnum::SOFT_KEY_PRESS:
            appSetKeyPress( (int) g.xtarget, true);
            keyboard ( (uchar) g.xtarget, AppEnum::BUTTON_PRESS, 0, m_mouseX, m_mouseY );
            break;
    }
  //  dbgprintf("Event: %d, b:%d,%d x:%f, y:%f, dx:%f, dy:%f\n", g.typeOrdinal, m_mouseButton, m_mouseState, m_mouseX, m_mouseY, m_dX, m_dY);
}

#ifdef USE_NETWORK
    bool Application::appSendEventToApp ( Event* e )
    {
        return pApp->on_event ( e );
    }
#endif

void Application::appOpenBrowser ( std::string app, std::string query )
{
    dbgprintf( "appOpenLink");

    JNIEnv *env;
    jint rs = m_win->_javavm->AttachCurrentThread(&env, NULL);
    assert (rs == JNI_OK);
    jmethodID method = env->GetMethodID( m_win->_javaGlobalClass, "openLink", "(Ljava/lang/String;)V");

    jstring jstr = getJStringFromStr ( env, query );
    env->CallVoidMethod( m_win->_javaGlobalObject, method, jstr );
}


void Application::appOpenKeyboard()
{
    JNIEnv *env;
    jint rs = m_win->_javavm->AttachCurrentThread(&env, NULL);
    assert (rs == JNI_OK);
    jmethodID method = env->GetMethodID( m_win->_javaGlobalClass, "openSoftKeyboard", "()V");
    env->CallVoidMethod( m_win->_javaGlobalObject, method);
}

void Application::appForegroundWindow ()
{
}
void Application::appCloseKeyboard()
{
    JNIEnv *env;
    jint rs = m_win->_javavm->AttachCurrentThread(&env, NULL);
    assert (rs == JNI_OK);

    jmethodID method = env->GetMethodID( m_win->_javaGlobalClass, "closeSoftKeyboard", "()V");

    env->CallVoidMethod( m_win->_javaGlobalObject, method);
}

int main(int argc, char **argv)
{
    dbgprintf ("Starting here\n");
    return 0;
}


