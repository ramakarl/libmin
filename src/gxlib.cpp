

#include <assert.h>
#include <time.h>
#include "timex.h"

#include "gxlib.h"

 #define EXTRA_CHECKS			// debug with GL checks

using namespace glib;

// Global singleton
glib::gxLib    glib::gx;

TimeX basetime;

// 2D Interface
bool glib::init2D ( const char* fontName )
{
	gx.m_curr_set = 0;
	gx.m_curr_type = 'x';
	gx.m_text_hgt = 24;
  gx.m_Xres = -1;
  gx.m_Yres = -1;
	gx.m_debug = false;

	basetime.SetTimeNSec ();

	// confirm GL context to start
	#ifdef BUILD_OPENGL
    #if defined(_WIN32)
        HGLRC ctx = wglGetCurrentContext ();
        if (ctx==0x0) { dbgprintf ( "ERROR: need valid context before calling gxLib::init2D.\n"); exit(-1); }
    #elif defined(__ANDROID__)
        EGLContext ctx = eglGetCurrentContext();
        if (ctx==0x0) { dbgprintf ( "ERROR: need valid context before calling gxLib::init2D.\n"); exit(-1); }
    #endif
				
	#endif	

	// create shaders 
	// (must have GL context at this point)
	gx.createShader2D();
	gx.createShader3D();

	// sin/cos tables 
	for (int a=0; a <= 36000; a++) {
		gx.cos_table[a] = cos ( (a/100.0f)*DEGtoRAD );
		gx.sin_table[a] = sin ( (a/100.0f)*DEGtoRAD );
	}

	// generate VAO
	#ifdef BUILD_OPENGL
		glGenVertexArrays ( 1, (GLuint*) &gx.mVAO );
		glBindVertexArray ( gx.mVAO );
	#endif

	// load white img
	gx.m_white_img.Create ( 8, 8, ImageOp::RGBA8, DT_CPU | DT_GLTEX );
	gx.m_white_img.Fill ( 255,255,255,255 );

	// load default font
	if (!gx.loadFont ( fontName )) {
		printf ("ERROR: Unable to load font. Aborting.\n");
		exit(-7);
		return false;
	}

    return true;
}
void glib::debug2D (bool tf)
{
	gx.m_debug = tf;
}

void glib::clear2D () 
{
	gx.clearSet ( gx.m_curr_set );
}

void glib::destroy2D ()
{
	gx.destroy();
}

// setViewRegion
// - used to determine font size, etc.
// - can occur outside of start2D/end2D
void glib::setViewRegion(Vec4F v, Vec4F r) 
{ 
	gx.m_View = v; gx.m_Region = r; 
}

void glib::start2D ( int w, int h, bool bStatic )
{
	gxSet s;
	gx.m_curr_prim = PRIM_NONE;
	gx.m_curr_num = 0;
	
	gxSet* set = gx.addSet ( '2', bStatic );	
	set->region.Set ( 0, 0, w, h );		// default view & region
	set->view.Set( 0, 0, w, h );
	set->text_aspect = 1.0;						// default text aspect

	setview2D ( w, h );
}

void glib::setTextAspect (float a)
{
	gxSet* s = gx.getCurrSet();
	if (s==0x0) return;
	s->text_aspect = a;
}

void glib::end2D ()
{
	// finish any remaining prims
	gx.finishPrim ( gx.getCurrSet() );	

	// start next set
	gx.m_curr_set++;	
}


// set view matrices for full screen 2D window
void glib::setview2D (int w, int h)
{
  gx.m_Xres = w;
  gx.m_Yres = h;

	gxSet* s = gx.getCurrSet();
	if (s == 0x0) return;
	s->view.Set(0, 0, w, h);				// default, screen-space view
	s->region.Set (0, 0, w, h);

	Matrix4F proj, view, model;
	view.Scale ( 2.0/w, -2.0/h, 1.0 );	
  model.Translate ( -w/2.0, -h/2.0, 0 );
  view *= model;
  model.Identity();
  proj.Identity();
	setMatrices ( model, view, proj );   
}

// set view matrices explicitly
void glib::setview2D ( int w, int h, Vec4F view, Vec4F region, Matrix4F& model, Matrix4F& viewmtx, Matrix4F& projmtx )
{
	gx.m_Xres = w;
	gx.m_Yres = h;

	gxSet* s = gx.getCurrSet();
	if (s == 0x0) return;
	s->view = view;							// custom view & region
	s->region = region;

	setMatrices ( model, viewmtx, projmtx );		// custom matrices
}

void glib::setMatrices ( Matrix4F& model, Matrix4F& view, Matrix4F& proj, int s )
{	
	// assign 2D view matrices to a cmd set
  if (s==-1) s = gx.m_curr_set;
  gxSet* set = gx.getSet( s);
	if (set==0x0) return;

  memcpy ( set->model_mtx, model.GetDataF(), 16 * sizeof(float) );
	memcpy ( set->view_mtx,  view.GetDataF(), 16 * sizeof(float) );
	memcpy ( set->proj_mtx,  proj.GetDataF(), 16 * sizeof(float) );
}

inline void vclr ( gxVert* v, Vec4F clr, float a=1)
{
	clr.w *= a;
	v->clr = VECCLR(clr);
}

void glib::drawLine ( Vec2F a, Vec2F b, Vec4F clr )
{
    gxVert* v = gx.allocGeom2D ( 2, PRIM_LINES );
    v->x = a.x; v->y = a.y; v->z = 0; vclr(v,clr); v++;
	v->x = b.x; v->y = b.y; v->z = 0; vclr(v,clr); v++;
}

void glib::drawRect  ( Vec2F a, Vec2F b, Vec4F clr )
{
	glib::drawLine ( a, Vec2F(b.x,a.y), clr );
	glib::drawLine ( Vec2F(b.x,a.y), b, clr );
	glib::drawLine ( b, Vec2F(a.x,b.y), clr );
	glib::drawLine ( Vec2F(a.x,b.y), a, clr );
}

void glib::drawFill ( Vec2F a, Vec2F b, Vec4F clr )
{
	gxVert* v = gx.allocGeom2D ( 6, PRIM_TRI );

	// repeat first for jump
	v->x = b.x; v->y = a.y; v->z = 0; vclr(v,clr); v++;
		
	// two triangles (as strip, must start top right)
	v->x = b.x; v->y = a.y; v->z = 0; vclr(v,clr); v++;
	v->x = b.x; v->y = b.y; v->z = 0; vclr(v,clr); v++;
	v->x = a.x; v->y = a.y; v->z = 0; vclr(v,clr); v++;
	v->x = a.x; v->y = b.y; v->z = 0; vclr(v,clr); v++;
		
	// repeat last for jump
	v->x = a.x; v->y = b.y; v->z = 0; vclr(v,clr); v++;
}

void glib::drawRoundedRect (Vec2F a, Vec2F b, Vec4F clr, float r )
{
	int du = 15;
	int sec = (90/du)+1;
  // # pnts = 4 arcs * 2 pnts/sec * # sec 
	gxVert* v = gx.allocGeom2D( 4*2*sec + 2, PRIM_LINES);

	int n=0;
	Vec2F pl, p, c;
	pl = a + Vec2F(1 * r, 0);  
	for (int u = 0; u <= 90; u += du) {					// top left arc
		p = Vec2F(a.x+r,a.y+r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = pl.x; v->y = pl.y; v->z = 0; vclr(v, clr); v++; n++;
		v->x = p.x; v->y = p.y; v->z = 0; vclr(v, clr); v++; n++;
		pl = p;
	}	
	for (int u = 90; u <= 180; u += du) {				// bottom left arc
		p = Vec2F(a.x+r, b.y-r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = pl.x; v->y = pl.y; v->z = 0; vclr(v, clr); v++; n++;
		v->x = p.x; v->y = p.y; v->z = 0; vclr(v, clr); v++; n++;
		pl = p;
	}
	for (int u = 180; u <= 270; u += du) {
		p = Vec2F(b.x-r, b.y-r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = pl.x; v->y = pl.y; v->z = 0; vclr(v, clr); v++; n++;
		v->x = p.x; v->y = p.y; v->z = 0; vclr(v, clr); v++; n++;
		pl = p;
	}
	for (int u = 270; u <= 360; u += du) {
		p = Vec2F(b.x-r, a.y+r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = pl.x; v->y = pl.y; v->z = 0; vclr(v, clr); v++; n++; 
		v->x = p.x; v->y = p.y; v->z = 0; vclr(v, clr); v++; n++;
		pl = p;
	}
	// last edge
	v->x = p.x; v->y = p.y; v->z = 0; vclr(v, clr); v++; n++;
	v->x = a.x+r; v->y = a.y; v->z = 0; vclr(v, clr); v++; n++;		// terminal point

	assert ( n == 4*2*sec + 2 );
}

void glib::drawRoundedFill(Vec2F a, Vec2F b, Vec4F clr, float r)
{
	int n = 0;
	int du = 15;
	int sec = (90/du)+1;		// sections per arc
  // # pnts = 4 arcs * 3 pnts/sec * # sec/arc + 4 jump pnts
	gxVert* v = gx.allocGeom2D( 4*3*sec+ 4, PRIM_TRI);			

	Vec2F p;
	Vec2F c (a.x+r, a.y);				// fan pnt (all tris touch this pnt)
	
  // repeat first jump
	v->x = a.x + r;	v->y = a.y + r; v->z = 0;		vclr(v, clr); v++; n++;		
	// first edge
	v->x = a.x + r;	v->y = a.y + r; v->z = 0;	vclr(v, clr); v++;	n++;
	v->x = a.x + r; v->y = a.y + 0; v->z = 0; vclr(v, clr); v++;	n++;

	for (int u = 0; u <= 90; u += du) {	
		p = Vec2F(a.x+r, a.y+r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);		
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;		// 2x fan = rim pnt -> center -> out		
		v->x = c.x; v->y = c.y; v->z = 0;		vclr(v, clr); v++;
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++; n += 3;
	}
	for (int u = 90; u <= 180; u += du) {	
		p = Vec2F(a.x+r, b.y-r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;		// 2x fan = rim pnt -> center -> out		
		v->x = c.x; v->y = c.y; v->z = 0;		vclr(v, clr); v++;
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++; n += 3; 
	}
	for (int u = 180; u <= 270; u += du) {		// segs
		p = Vec2F(b.x-r, b.y-r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;		// 2x fan = rim pnt -> center -> out		
		v->x = c.x; v->y = c.y; v->z = 0;		vclr(v, clr); v++;
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++; n += 3; 
	}
	for (int u = 270; u <= 360; u += du) {		// segs
		p = Vec2F(b.x-r, a.y+r) + Vec2F(-gx.sin_table[u * 100] * r, -gx.cos_table[u * 100] * r);
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;		// 2x fan = rim pnt -> center -> out		
		v->x = c.x; v->y = c.y; v->z = 0;		vclr(v, clr); v++;
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++; n += 3; 
	}
	// repeat last jump
	v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++; n++;
	
	assert( n == 4*3*sec+4 );

}



void glib::drawGradient ( Vec2F a, Vec2F b, Vec4F c0, Vec4F c1, Vec4F c2, Vec4F c3 )
{
	gxVert* v = gx.allocGeom2D ( 6, PRIM_TRI );

	// repeat first for jump
	v->x = a.x; v->y = a.y; v->z = 0; vclr(v,Vec4F(0,0,0,0)); v++;
		
	// two triangles (as strip, must start top right)
	v->x = a.x; v->y = a.y; v->z = 0; vclr(v,c0); v++;
	v->x = b.x; v->y = a.y; v->z = 0; vclr(v,c1); v++;		
	v->x = a.x; v->y = b.y; v->z = 0; vclr(v,c2); v++;
	v->x = b.x; v->y = b.y; v->z = 0; vclr(v,c3); v++;
		
	// repeat last for jump
	v->x = b.x; v->y = b.y; v->z = 0; vclr(v,Vec4F(0,0,0,0)); v++;
}

void glib::drawCircle ( Vec2F a, float r, Vec4F clr  )
{	
	int du = 15;	
	gxVert* v = gx.allocGeom2D ( 2*((360/du)+1), PRIM_LINES );	

	// draw circle
	Vec2F pl, p, c;
	pl = a + Vec2F(1 * r, 0);
	for (int u=0; u <= 360; u += du ) {
		p = a + Vec2F( gx.cos_table[u*100] * r, gx.sin_table[u*100] * r / gx.getTextAspect() );		
		v->x = pl.x; v->y = pl.y; v->z = 0; vclr (v,clr); v++;
		v->x = p.x ; v->y = p.y ; v->z = 0; vclr (v,clr); v++;
		pl = p;
	}		
}

void glib::drawCircleFill (Vec2F a, float r, Vec4F clr)
{
	int n = 0;
	int du = 15;
	int segs = (360/du)+1;
	gxVert* v = gx.allocGeom2D(3 * segs + 4, PRIM_TRI);

	// draw circle
	Vec2F pl, p;	
	pl = a + Vec2F(1 * r, 0);

	// repeat first jump
	v->x = a.x;		v->y = a.y; v->z = 0;		vclr(v, clr); v++;		n++;
	
	// first edge
	v->x = a.x;		v->y = a.y; v->z = 0;		vclr(v, clr); v++;		n++;
	v->x = pl.x; v->y = pl.y; v->z = 0; vclr(v, clr); v++;		n++;

	for (int u = 0; u <= 360; u += du) {		// segs
		p = a + Vec2F(gx.cos_table[u * 100] * r, gx.sin_table[u * 100] * r / gx.getTextAspect() );
		
		// 2x fan = rim pnt -> center -> out		
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;
		v->x = a.x; v->y = a.y; v->z = 0;		vclr(v, clr); v++;
		v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;
			n+=3;
		pl = p;
	}
	// repeat last jump
	v->x = p.x; v->y = p.y; v->z = 0;		vclr(v, clr); v++;

	int cnt = 3*segs+2;
}


void glib::drawImg ( ImageX* img, Vec2F a, Vec2F b, Vec4F clr )
{
	gxVert* v = gx.allocImg2D ( 6, PRIM_TRI, img );

	// repeat first for jump
	v->x = a.x; v->y = a.y; v->z = 0; vclr(v,clr,0); v++;
		
	// two triangles (as strip, must start top right)
	v->x = a.x; v->y = a.y; v->z = 0; vclr(v,clr,1); v->tx = 0; v->ty = 0;	v++;
	v->x = b.x; v->y = a.y; v->z = 0; vclr(v,clr,1); v->tx = 1; v->ty = 0;	v++;		
	v->x = a.x; v->y = b.y; v->z = 0; vclr(v,clr,1); v->tx = 0; v->ty = 1;	v++;
	v->x = b.x; v->y = b.y; v->z = 0; vclr(v,clr,1); v->tx = 1; v->ty = 1;	v++;
		
	// repeat last for jump
	v->x = b.x; v->y = b.y; v->z = 0; vclr(v,clr,0); v++;
}


//------------------------------ FONT RENDERING

// gxLib FONTS:
// 
//       world hgt = points --> point_factor --> pixels --> world
// 
// Eqn:  world hgt = points * point_factor * pix_per_pnt * pnt_to_world
// Defs:
// - world         = Desired size of font in world units. See: setTextSz()
// - point_factor  = Device points adjustment (in pnts/10 pnt). eg. val 7 means render 10 pnt at 7 pnt actual.
// - pix_per_pnt   = Converts points to pixels (in pix/pnt). See: setTextDevice()
//                   Device dependent. Set by device-specific initialization.
// - pnt_to_world  = Converts pixels to world unit (in units/pnt). See: setTextPnts(), getPntToWorld
//                   World scale dependent. Set when establishing view.
// Notes:
// - If you want to render at world size, use setTextSz ( hgt_world ) -- no conversion needed
// - If you want to render at point size, use setTextPnts ( hgt_pnt ) -- does above conversion
// - When rendering to screen, there is no world. The last term in eqn (pnt_per_world) is 1.
// 
// Common devices:
//   Mobile (Galaxy S10)      = 7.154 pix_per_pnt
//   Desktop (Philips 4K 27") = 1.792 pix_per_pnt
//

float glib::getPntToWorld ()
{
	gxSet* s = gx.getCurrSet(); if (s==0) return 1;

  // pnt_to_world = point factor --> pixels --> world
  //  (actual measure of pnt_to_world is in: units/pixels)
	return (gx.m_BasePnt/10.0) * gx.m_PixPerPnt * fabs((s->view.w - s->view.y) / (s->region.w - s->region.y));
}
Vec4F glib::getView()			{ return gx.m_sets[gx.m_curr_set].view; }
Vec4F glib::getRegion()		{ return gx.m_sets[gx.m_curr_set].region; }

// set device pixels per point
// - PixPerPnt = pixels per point for given device. eg. mobile = 7.154 pix/pnt
// - BasePnt   = point adjust factor. eg. 7 pnt means render 10 pnt at 7 pnt actual
void glib::setTextDevice (float pix_per_pnt, float base_pnt)
{
	gx.m_PixPerPnt = pix_per_pnt;
	gx.m_BasePnt = base_pnt;
}

// set desired font hgt in world units
//
void glib::setTextSz (float hgt_world, float kern)
{
	gx.m_text_hgt = hgt_world;
	gx.m_text_kern = kern;
}

void glib::setTextPnts (float hgt_pnts, float kern )
{
	gx.m_text_hgt = hgt_pnts * getPntToWorld();	
	gx.m_text_kern = kern * getPntToWorld();
}

void glib::drawText ( Vec2F a, std::string msg, Vec4F clr )
{
	int len = (int) msg.size();
	if ( len == 0 ) 
		return;

	gxVert* v = gx.allocImg2D ( len*6, PRIM_TRI, &gx.m_font_img );

	if (gx.m_font_img.getGLID()==-1) {
		printf ( "ERROR: Font texture is not loaded or not on GPU.\n");
		exit(-7);
	}

	// get current font
	gxFont& font = gx.getCurrFont ();		
	float lX = a.x;
	float lY = a.y;
	float lLinePosX = a.x;
	float lLinePosY = a.y;			
	char ch;
	
	// Font rendering:
  // - font glyph contains type dimension in bitmap pixel units
  // - fontPixToWorld = converts from font bitmap pixel to world units = text_hgt / font.ascent;
  // - text_hgt    = already contains point -> pixel -> world conversions
  // - text_aspect = text aspect adjustment (y only)
  
	float fontPixToWorld = gx.m_text_hgt / font.ascent;			
	float text_aspect = gx.getTextAspect();
	int cused = 0;

	for (int c=0; c < len; c++ ) {
		ch = msg.at(c);
		if ( ch == '\n' ) {
			// line return
			lX = lLinePosX;
			lLinePosY += gx.m_text_hgt;
			lY = lLinePosY;
		
		} else if ( ch >= 0 && ch <= 128 ) {
			// printable character
			gxGlyph& gly = font.glyphs[ ch ];
			float pX = lX + gly.offX * fontPixToWorld;
			float pY = lY + (gly.offY * fontPixToWorld / text_aspect);
			float pW = gly.width * fontPixToWorld;
			float pH = gly.height * fontPixToWorld * -1.0 / text_aspect;
	
			// GRP_TRITEX is a triangle strip!
			// repeat first point (jump), zero alpha
			v->x = pX;		v->y = pY;	v->z = 0;		vclr(v,clr, 0);		v->tx = gly.u; v->ty = gly.v;	v++;

			// four corners of glyph
			v->x = pX;		v->y = pY;		v->z = 0; 	vclr(v,clr, 1);		v->tx = gly.u; v->ty = gly.v + gly.dv;	v++;
			v->x = pX;		v->y = pY-pH;	v->z = 0;		vclr(v,clr, 1);		v->tx = gly.u; v->ty = gly.v;	v++;			
			v->x = pX+pW;	v->y = pY;		v->z = 0;		vclr(v,clr, 1);		v->tx = gly.u + gly.du; v->ty = gly.v + gly.dv;	v++;
			v->x = pX+pW;	v->y = pY-pH;	v->z = 0; 	vclr(v,clr, 1);		v->tx = gly.u + gly.du; v->ty = gly.v ;	v++;

			// repeat last point (jump), zero alpha
			v->x = pX+pW;	v->y = pY-pH;	v->z = 0;			vclr(v,clr, 0);		v->tx = gly.u + gly.du; v->ty = gly.v; v++;
	
			lX += (gly.advance + gx.m_text_kern) * fontPixToWorld;
			lY += 0;			
			cused++;

		}	else if ( ch=='\0' ) {
			// end of line
			break;
		}
		// note: unicode/extended chars will not be used
	}


	// clear remainder of VBO entries
	for (int n = cused; n < len; n++ ) {
		memset ( v, 0, 6*sizeof(gxVert) ); v += 6;				
	}
}

Vec2F glib::getTextDim ( char mode, float hgt, std::string msg )
{
	int len = (int) msg.size();
	if (len == 0)	return Vec2F(0, 0);						// no text

	float text_hgt = hgt;
	if (mode == 'p') {
		text_hgt = hgt * (gx.m_BasePnt / 10.0) * gx.m_PixPerPnt * fabs((gx.m_View.w - gx.m_View.y) / (gx.m_Region.w - gx.m_Region.y));
	}	
	// get current font
	gxFont& font = gx.getCurrFont();
	if (font.ascent == 0)	return Vec2F(0, 0);					// we don't have a font yet
	float fontPixToWorld = text_hgt / font.ascent;		// font to world scale
	
	// world size width of text
	float lX = 0, lMax = 0;	
	char ch;
	for (int c=0; c < len; c++) {
		ch = msg.at(c);
		if (ch == '\n') {
			lX = 0;
		}	else if (ch >= 0 && ch <= 128) {			
			lX += (font.glyphs[ch].advance + gx.m_text_kern) * fontPixToWorld;
			if (lX > lMax) lMax = lX;
		}
	}
	Vec2F sz;
	sz.x = lMax;	
	sz.y = font.ascent * fontPixToWorld;

	// world space to pixels
	// px = sz * Vec2F( float(s->region.z - s->region.x) / (view.z - view.x), float(region.w - region.y) / (view.w - view.y) );

	return sz;
}

void glib::start3D ( Camera3D* cam, bool bStatic )
{
  gx.m_curr_prim = PRIM_NONE;	
	gx.m_curr_num = 0;	

	gxSet* set = gx.addSet ( '3', bStatic );
	//set->clip_region.Set ( 0, 0, w, h );

	setView3D ( cam );	
	
	// default env map & material
	setEnvmap3D ( &gx.m_white_img );
	setMaterial ( Vec3F(0,0,0), Vec3F(1,1,1), Vec3F(.5,.5,.5), 20, 1.0 );
}

void glib::setEnvmap3D ( ImageX* img )
{
	gxSet* set = gx.getCurrSet();
	set->envmap = img->getGLID();
}

void glib::setView3D ( Camera3D* cam )
{
	gxSet* s = gx.getCurrSet ();
	Matrix4F ident; 
	ident.Identity ();
	s->cam_from = cam->getPos();
	s->cam_to = cam->getToPos();
	memcpy ( s->model_mtx, ident.GetDataF(), 16 * sizeof(float) );
	memcpy ( s->view_mtx, cam->getViewMatrix().GetDataF(), 16 * sizeof(float) );
	memcpy ( s->proj_mtx, cam->getProjMatrix().GetDataF(), 16 * sizeof(float) );
}

void glib::setMaterial ( Vec3F Ka, Vec3F Kd, Vec3F Ks, float Ns, float Tf)
{
	gxSet* s = gx.getCurrSet ();
	s->ambclr = Ka;
	s->diffclr = Kd;
	s->specclr = Ks;
	s->specpow = Ns;
}

void glib::setLight3D ( Vec3F pos, Vec4F clr )
{
	gxSet* s = gx.getCurrSet ();
	s->lightpos = pos;
	s->lightclr = clr;
}

void glib::end3D ()
{
	// finish any remaining prims
	gx.finishPrim ( gx.getCurrSet() );
	
	// start next set
	gx.m_curr_set++;
}

inline void vclr3d ( gxVert3D* v, Vec4F clr, float a=1)
{
	clr.w *= a;
	v->clr = VECCLR(clr);
}
inline void vno ( gxVert3D* v )
{
	v->nx = NO_NORM; v->tx = 0; v->ty = 0;
}
inline void vnorm ( gxVert3D* v, Vec3F n, bool solid )
{
	v->nx = solid ? NO_NORM : n.x; 
	v->ny = n.y; 
	v->nz = n.z;
}

void glib::drawLine3D ( Vec3F a, Vec3F b, Vec4F clr )
{
    gxVert3D* v = gx.allocGeom3D ( 2, PRIM_LINES );
    v->x = a.x; v->y = a.y; v->z = a.z; vclr3d (v,clr); vno(v); v++;
	v->x = b.x; v->y = b.y; v->z = b.z; vclr3d (v,clr); vno(v); v++;
}
void glib::drawLineDotted3D ( Vec3F a, Vec3F b, Vec4F clr, int segs )
{
	gxVert3D* v = gx.allocGeom3D ( segs*2, PRIM_LINES );

	Vec3F p = a;
	Vec3F dp = (b-a)/(segs*2);

	for (int s=0; s < segs; s++) {
		v->x = p.x; v->y = p.y; v->z = p.z; vclr3d (v,clr); vno(v); v++;
		v->x = p.x+dp.x; v->y = p.y+dp.y; v->z = p.z+dp.z; vclr3d (v,clr); vno(v); v++;
		p += dp*2;
	}
}

void glib::drawBox3D (Vec3F p, Vec3F q, Vec4F clr )
{	
	drawLine3D ( Vec3F(p.x, q.y, p.z), Vec3F(q.x, q.y, p.z), clr );		// y++ face
	drawLine3D ( Vec3F(q.x, q.y, p.z), Vec3F(q.x, q.y, q.z), clr );
	drawLine3D ( Vec3F(q.x, q.y, q.z), Vec3F(p.x, q.y, q.z), clr );
	drawLine3D ( Vec3F(p.x, q.y, q.z), Vec3F(p.x, q.y, p.z), clr );

	drawLine3D ( Vec3F(p.x, p.y, p.z), Vec3F(q.x, p.y, p.z), clr );		// y-- face
	drawLine3D ( Vec3F(q.x, p.y, p.z), Vec3F(q.x, p.y, q.z), clr );
	drawLine3D ( Vec3F(q.x, p.y, q.z), Vec3F(p.x, p.y, q.z), clr );
	drawLine3D ( Vec3F(p.x, p.y, q.z), Vec3F(p.x, p.y, p.z), clr );

	drawLine3D( Vec3F(p.x, p.y, p.z), Vec3F(p.x, q.y, p.z), clr );		// verticals
	drawLine3D( Vec3F(p.x, p.y, q.z), Vec3F(p.x, q.y, q.z), clr );
	drawLine3D( Vec3F(q.x, p.y, q.z), Vec3F(q.x, q.y, q.z), clr );
	drawLine3D( Vec3F(q.x, p.y, p.z), Vec3F(q.x, q.y, p.z), clr );
}

void glib::drawBox3D (Vec3F b1, Vec3F b2, Vec4F clr, Matrix4F& xform )
{
	Vec3F p[8];
	p[0].Set ( b1.x, b1.y, b1.z );	p[0] *= xform;
	p[1].Set ( b2.x, b1.y, b1.z );  p[1] *= xform;
	p[2].Set ( b2.x, b1.y, b2.z );  p[2] *= xform;
	p[3].Set ( b1.x, b1.y, b2.z );  p[3] *= xform;

	p[4].Set ( b1.x, b2.y, b1.z );	p[4] *= xform;
	p[5].Set ( b2.x, b2.y, b1.z );  p[5] *= xform;
	p[6].Set ( b2.x, b2.y, b2.z );  p[6] *= xform;
	p[7].Set ( b1.x, b2.y, b2.z );  p[7] *= xform;

	drawLine3D ( p[0], p[1], clr );
	drawLine3D ( p[1], p[2], clr );
	drawLine3D ( p[2], p[3], clr );
	drawLine3D ( p[3], p[0], clr );

	drawLine3D ( p[4], p[5], clr );
	drawLine3D ( p[5], p[6], clr );
	drawLine3D ( p[6], p[7], clr );
	drawLine3D ( p[7], p[4], clr );

	drawLine3D ( p[0], p[4], clr );
	drawLine3D ( p[1], p[5], clr );
	drawLine3D ( p[2], p[6], clr );
	drawLine3D ( p[3], p[7], clr );

}

void glib::drawBoxDotted3D (Vec3F p, Vec3F q, Vec4F clr, int segs )
{	
	drawLineDotted3D ( Vec3F(p.x, q.y, p.z), Vec3F(q.x, q.y, p.z), clr, segs );		// y++ face
	drawLineDotted3D ( Vec3F(q.x, q.y, p.z), Vec3F(q.x, q.y, q.z), clr, segs );
	drawLineDotted3D ( Vec3F(q.x, q.y, q.z), Vec3F(p.x, q.y, q.z), clr, segs );
	drawLineDotted3D ( Vec3F(p.x, q.y, q.z), Vec3F(p.x, q.y, p.z), clr, segs );

	drawLineDotted3D ( Vec3F(p.x, p.y, p.z), Vec3F(q.x, p.y, p.z), clr, segs );		// y-- face
	drawLineDotted3D ( Vec3F(q.x, p.y, p.z), Vec3F(q.x, p.y, q.z), clr, segs );
	drawLineDotted3D ( Vec3F(q.x, p.y, q.z), Vec3F(p.x, p.y, q.z), clr, segs );
	drawLineDotted3D ( Vec3F(p.x, p.y, q.z), Vec3F(p.x, p.y, p.z), clr, segs );

	drawLineDotted3D( Vec3F(p.x, p.y, p.z), Vec3F(p.x, q.y, p.z), clr, segs );		// verticals
	drawLineDotted3D( Vec3F(p.x, p.y, q.z), Vec3F(p.x, q.y, q.z), clr, segs );
	drawLineDotted3D( Vec3F(q.x, p.y, q.z), Vec3F(q.x, q.y, q.z), clr, segs );
	drawLineDotted3D( Vec3F(q.x, p.y, p.z), Vec3F(q.x, q.y, p.z), clr, segs );
}

void glib::drawCircle3D ( Vec3F p1, float r, Vec4F clr  )
{
	gxSet* s = gx.getCurrSet ();	
	Vec3F n = p1 - s->cam_from; n.Normalize();

	drawCircle3D ( p1, n, r, clr );
}


void glib::drawCircle3D ( Vec3F p1, Vec3F n, float r, Vec4F clr  )
{	
	int du = 15;
	gxVert3D* v = gx.allocGeom3D ( 2*((360/du)+1), PRIM_LINES );	

	// create a basis space oriented toward p2 (usually the camera)
	Vec3F up = (fabs(n.y)==1) ? Vec3F(0,0,1) : Vec3F(0,1,0);
	Vec3F bu = n.Cross (up);  bu.Normalize();
	Vec3F bv = n.Cross (bu);  bv.Normalize();

	// draw circle
	Vec3F pl, p, a;
	a = Vec3F(1 * r, 0, 0); 	
	pl = p1 + bu*a.x + bv*a.y;
	for (int u=0; u <= 360; u += du ) {
		a = Vec3F( gx.cos_table[u*100] * r, gx.sin_table[u*100] * r, 0 );
		p = p1 + bu*a.x + bv*a.y;
		v->x = pl.x; v->y = pl.y; v->z = pl.z; vclr3d (v,clr); vno(v); v++;
		v->x = p.x; v->y = p.y; v->z = p.z; vclr3d (v,clr); vno(v); v++;
		pl = p;
	}		

}


void glib::drawSphere3D (Vec3F p, float r, Vec4F clr, bool solid)
{
	float cu, su, cu1, su1;
	float cv, sv, cv1, sv1;
	int us, vs;
	Vec3F a, b, c, d, n;
	float s = 15;

	for (int v=0; v <= 180; v+= s ) {
		for (int u=0; u < 360; u += s ) {
			cu = gx.cos_table[ u*100]; cv = gx.cos_table[v*100]; su = gx.sin_table[u*100]; sv = gx.sin_table[v*100];
			us = int(u+s) % 360;
			vs = int(v+s) % 360;
			cu1 = gx.cos_table[us*100]; cv1 = gx.cos_table[vs*100]; su1 = gx.sin_table[us*100]; sv1 = gx.sin_table[vs*100];
			a.Set (cu *sv,  cv,  su * sv);		n=a;	a = a * r + p;			// u,v
			b.Set (cu1*sv,  cv,  su1* sv);				b = b * r + p;			// u+1,v
			c.Set (cu1*sv1, cv1, su1* sv1);				c = c * r + p;			// u+1,v+1
			d.Set (cu *sv1, cv1, su * sv1);				d = d * r + p;			// u,v+1			
			drawFace3D (a, b, c, d, n, clr, solid );
		}
	}
}

void glib::drawTri3D( Vec3F a, Vec3F b, Vec3F c, Vec3F n, Vec4F clr, bool solid )
{	
	gxVert3D* v = gx.allocGeom3D (5, PRIM_TRI);
	
	// repeat first for jump
	v->x = a.x; v->y = a.y; v->z = a.z; vclr3d(v,clr); vnorm(v,n,solid); v++;

	v->x = a.x; v->y = a.y; v->z = a.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
	v->x = b.x; v->y = b.y; v->z = b.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
	v->x = c.x; v->y = c.y; v->z = c.z; vclr3d(v,clr); vnorm(v,n,solid); v++;

	// repeat last for jump
	v->x = c.x; v->y = c.y; v->z = c.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
}

void glib::drawFace3D( Vec3F a, Vec3F b, Vec3F c, Vec3F d, Vec3F n, Vec4F clr, bool solid )
{	
	gxVert3D* v = gx.allocGeom3D (8, PRIM_TRI);
	
	// repeat first for jump
	v->x = a.x; v->y = a.y; v->z = a.z; vclr3d(v,clr); vnorm(v,n,solid); v++;

	v->x = a.x; v->y = a.y; v->z = a.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
	v->x = b.x; v->y = b.y; v->z = b.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
	v->x = c.x; v->y = c.y; v->z = c.z; vclr3d(v,clr); vnorm(v,n,solid); v++;

	v->x = a.x; v->y = a.y; v->z = a.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
	v->x = c.x; v->y = c.y; v->z = c.z; vclr3d(v,clr); vnorm(v,n,solid); v++;
	v->x = d.x; v->y = d.y; v->z = d.z; vclr3d(v,clr); vnorm(v,n,solid); v++;

	// repeat last for jump
	v->x = d.x; v->y = d.y; v->z = d.z; vclr3d(v,clr); vnorm(v,n,solid); v++;

}

void glib::drawCube3D (Vec3F p, Vec3F q, Vec4F clr )
{
	drawFace3D( Vec3F(p.x, q.y, p.z), Vec3F(q.x, q.y, p.z), Vec3F(q.x, q.y, q.z), Vec3F(p.x, q.y, q.z), Vec3F(0, 1, 0), clr );
	drawFace3D( Vec3F(p.x, p.y, p.z), Vec3F(q.x, p.y, p.z), Vec3F(q.x, p.y, q.z), Vec3F(p.x, p.y, q.z), Vec3F(0, -1, 0), clr );

	drawFace3D( Vec3F(p.x, p.y, q.z), Vec3F(q.x, p.y, q.z), Vec3F(q.x, q.y, q.z), Vec3F(p.x, q.y, q.z), Vec3F(0, 0,  1), clr );
	drawFace3D( Vec3F(p.x, p.y, p.z), Vec3F(q.x, p.y, p.z), Vec3F(q.x, q.y, p.z), Vec3F(p.x, q.y, p.z), Vec3F(0, 0, -1), clr );

	drawFace3D( Vec3F(q.x, p.y, p.z), Vec3F(q.x, q.y, p.z), Vec3F(q.x, q.y, q.z), Vec3F(q.x, p.y, q.z), Vec3F(1, 0, 0), clr );
	drawFace3D( Vec3F(p.x, p.y, p.z), Vec3F(p.x, q.y, p.z), Vec3F(p.x, q.y, q.z), Vec3F(p.x, p.y, q.z), Vec3F(-1, 0, 0), clr );

}

void glib::drawText3D ( Vec3F a, float sz, char* msg, Vec4F clr )
{
	int len = (int) strlen ( msg );
	if ( len == 0 ) 
		return;

	gxVert3D* v = gx.allocGeom3D ( len*6, PRIM_TRI, &gx.m_font_img );

	// create a basis space oriented toward the camera 
	gxSet* s = gx.getCurrSet ();	
	Vec3F n = a - s->cam_from; n.Normalize();
	Vec3F up = (fabs(n.y)==1) ? Vec3F(0,0,1) : Vec3F(0,1,0);
	Vec3F bu = n.Cross (up);  bu.Normalize();
	Vec3F bv = n.Cross (bu);  bv.Normalize();	
	bu *= sz;
	bv *= sz;

	// get current font
	gxFont& font = gx.getCurrFont ();	
	int glyphHeight = font.ascent + font.descent + font.linegap;
	float lX = 0;
	float lY = 0;
	float lLinePosX = 0;
	float lLinePosY = 0;
	const char* c = msg;
	int cnt = 0;

	Vec3F p;

	// text_hgt = desired height of font in pixels
	// text_kern = spacing between letters

	float textSz = 1.0 / glyphHeight;	// glyph scale
	float textStartPy = textSz;					// start location in pixels
	
	while (*c != '\0' && cnt < len ) {
		if ( *c == '\n' ) {
			lX = lLinePosX;
			lLinePosY += gx.m_text_hgt;
			lY = lLinePosY;
		} else if ( *c >=0 && *c <= 128 ) {
			gxGlyph& gly = font.glyphs[*c];
			float pX = lX + gly.offX * textSz;
			float pY = lY + gly.offY * textSz; 
			float pW = gly.width * textSz;
			float pH = gly.height * textSz;
	
			// GRP_TRITEX is a triangle strip!
			// repeat first point (jump), zero alpha
			p = a + bu * pX + bv * (pY+pH);				v->x = p.x;	v->y = p.y;	v->z = p.z;		vclr3d(v, clr, 0); vno(v);		v->tx = gly.u; v->ty = gly.v;	v++;

			// four corners of glyph
			p = a + bu * pX + bv * (pY+pH);				v->x = p.x;	v->y = p.y;	v->z = p.z;		vclr3d(v, clr, 1); vno(v);		v->tx = gly.u; v->ty = gly.v;	v++;
			p = a + bu * pX + bv * pY;						v->x = p.x;	v->y = p.y;	v->z = p.z;		vclr3d(v, clr, 1); vno(v);		v->tx = gly.u; v->ty = gly.v + gly.dv;	v++;
			p = a + bu * (pX+pW) + bv * (pY+pH);	v->x = p.x;	v->y = p.y;	v->z = p.z;		vclr3d(v, clr, 1); vno(v);		v->tx = gly.u + gly.du; v->ty = gly.v;	v++;
			p = a + bu * (pX+pW) + bv * (pY);			v->x = p.x;	v->y = p.y;	v->z = p.z;		vclr3d(v, clr, 1); vno(v);		v->tx = gly.u + gly.du; v->ty = gly.v + gly.dv;v++;			

			// repeat last point (jump), zero alpha
			p = a + bu * (pX+pW) + bv * (pY);			v->x = p.x;	v->y = p.y;	v->z = p.z;		vclr3d(v, clr, 0); vno(v);	  v->tx = gly.u + gly.du; v->ty = gly.v + gly.dv;v++;					
	
			lX += (gly.advance + gx.m_text_kern) * textSz;
			lY += 0;
			cnt++;
		}
		c++;
	}

	// clear remainder of VBO entries
	for (int n=cnt; n < len; n++ ) {
		memset ( v, 0, 6*sizeof(gxVert3D) ); v += 6;				
	}
}



void glib::selfStartDraw3D ( Camera3D* cam ) 
{		
	#ifdef BUILD_OPENGL
		glEnable ( GL_DEPTH_TEST );
		glEnable ( GL_BLEND );
		glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	
		#ifdef EXTRA_CHECKS
			checkGL ( "selfdraw3D enable cmds");
		#endif

		glBindVertexArray(gx.mVAO);	
		glUseProgram(gx.mSH[S3D]);
		#ifdef EXTRA_CHECKS
			checkGL ( "selfdraw3D bind vao");
		#endif

		Matrix4F ident;
		ident.Identity();	
		glUniformMatrix4fv ( gx.mPARAM[S3D][SP_MODEL], 1, GL_FALSE, ident.GetDataF() );
		glUniformMatrix4fv ( gx.mPARAM[S3D][SP_VIEW],  1, GL_FALSE, cam->getViewMatrix().GetDataF() );
		glUniformMatrix4fv ( gx.mPARAM[S3D][SP_PROJ],  1, GL_FALSE, cam->getProjMatrix().GetDataF() );
		#ifdef EXTRA_CHECKS
			checkGL ( "selfdraw3D matrices");
		#endif

		// send camera pos to shader
		Vec3F c = cam->getPos();
		glUniform3f ( gx.mPARAM[S3D][SP_CAMPOS], c.x, c.y, c.z );

		// default env map
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, gx.m_white_img.getGLID() );
		glUniform1i( gx.mPARAM[S3D][SP_ENVMAP], 1);
		
		// assume no texture
		selfSetTexture ();	
	#endif
	
}

void glib::selfSetMaterial ( Vec3F Ka, Vec3F Kd, Vec3F Ks, float Ns, float Tf)
{
	#ifdef BUILD_OPENGL
		glUniform3f ( gx.mPARAM[S3D][SP_AMBCLR],  Ka.x, Ka.y, Ka.z );
		glUniform3f ( gx.mPARAM[S3D][SP_DIFFCLR], Kd.x, Kd.y, Kd.z );
		glUniform3f ( gx.mPARAM[S3D][SP_SPECCLR], Ks.x, Ks.y, Ks.z );
		glUniform1f ( gx.mPARAM[S3D][SP_SPECPOW], Ns );			
	#endif
}

void glib::selfSetLight3D ( Vec3F pos, Vec4F clr )
{
	#ifdef BUILD_OPENGL
		glUniform3f ( gx.mPARAM[S3D][SP_LIGHTPOS], pos.x, pos.y, pos.z );		
		glUniform3f ( gx.mPARAM[S3D][SP_LIGHTCLR], clr.x, clr.y, clr.z );
	#endif
}

void glib::selfSetTexture ( int glid ) 
{
	#ifdef BUILD_OPENGL
		glActiveTexture ( GL_TEXTURE0 );				
		glBindTexture ( GL_TEXTURE_2D, (glid==-1) ? gx.m_white_img.getGLID() : glid );
		#ifdef EXTRA_CHECKS
			checkGL ( "selfSetTexture");
		#endif
	#endif	
}
void glib::selfSetModelMtx ( Matrix4F& mtx )
{
	#ifdef BUILD_OPENGL
		glUniformMatrix4fv ( gx.mPARAM[S3D][SP_MODEL], 1, GL_FALSE, mtx.GetDataF() );
		#ifdef EXTRA_CHECKS
			checkGL ( "selfSetModelMtx");
		#endif
	#endif	
}
void glib::selfSetModel3D(Matrix4F& model)
{
	glUniformMatrix4fv(gx.mPARAM[S3D][SP_MODEL], 1, GL_FALSE, model.GetDataF());
}


void glib::selfEndDraw3D ()
{
	#ifdef BUILD_OPENGL
		glUseProgram ( 0 );
		glBindVertexArray ( 0 );
		#ifdef EXTRA_CHECKS
			checkGL ( "selfSetModelMtx");
		#endif
	#endif
}



void glib::drawAll ()
{
	#ifdef BUILD_OPENGL
		checkGL ( "drawAll start");
	
		// draw each set
		gx.drawSets();

		checkGL ( "drawAll end");

		// restart sets
		gx.m_curr_set = 0;
	#endif
}

	
// -------------------------------------------------------------- GX Library
//
gxLib::gxLib ()
{
		mVS[S2D] = 0; mFS[S2D] = 0;
		mVS[S3D] = 0; mFS[S3D] = 0;
		mSH[S2D] = 0;
		mSH[S3D] = 0;
		mVAO = 0;

    int start_sz = 256;

		// default device (Desktop)
		setTextDevice ( 1.8, 10.0 );			// 1.8 pix/pnt, 10 pnt as 10 pnt

    // clear sets
		m_sets.clear();    
}

gxSet* gxLib::addSet ( char st, bool bStatic )
{	
	int n = m_sets.size();	
	if ( m_curr_set >= n ) {
		// allocate new set
		gxSet newset;
		newset.geom = 0;
		newset.size = 0;
		newset.bufsize = 0;
		newset.max = 256;		
		newset.geom = (char*) malloc ( 256 );
		newset.lastpos = -1;
		newset.vbo = 0;
		newset.view.Set (0, 0, 0, 0);
		newset.region.Set ( 0, 0, 0, 0 );
		m_sets.push_back ( newset );
		m_curr_set = n;
	} 
	// get set
  gxSet* s = getSet( m_curr_set );
	s->stype = st;
	s->sstatic = bStatic;
	if ( !bStatic ) {		
		// reset dynamic sets
		s->size = 0;		
		s->lastpos = -1;		
	}
	return s;
}

void gxLib::clearSet ( int s )
{
	gxSet* set = getSet ( s );
	set->size = 0;
	set->lastpos = -1;
}

void gxLib::clearSets ()
{
	for (int n=0; n < m_sets.size(); n++) {
		if (!m_sets[n].sstatic) {
			gx.clearSet (n);
		}
	}
}

void gxLib::destroySets()
{
	gxSet* s;
	for (int n = 0; n < m_sets.size(); n++) {
		s = &m_sets[n];
		s->size = 0;
		if (s->geom != 0x0) {
			free ( s->geom );
			s->max = 256; 
			s->geom = (char*) malloc(256);		// reset to small size
		}
		if (s->vbo != 0) {
			glDeleteBuffers( 1, (GLuint*) &s->vbo);
			s->vbo = 0;
		}
	}
}

void gxLib::finishPrim ( gxSet* s )
{
	// write prim count
	if (s->lastpos != -1) {
		gxPrim* lastp = (gxPrim*) (s->geom + s->lastpos);
		lastp->cnt = m_curr_num;
	}
	s->lastpos = -1;
	m_curr_num = 0;
}

void gxLib::expandSet ( gxSet* s, uchar typ, uchar prim, int64_t add_bytes )
{
	// add marker group (draw call) when:
	// - current type has changed (eg. x -> i)
	// - current prim has changed (eg. PRIM_TRI, PRIM_LINES)
	// - always on image, assumed image is different (NOTE: should use img GLID)
	//
	bool need_marker = (typ != m_curr_type || prim != m_curr_prim || typ == 'i');

  add_bytes += (need_marker ? sizeof(gxPrim) : 0);

	assert ( s->size <= s->max );

    // allocate expanded mem
    if ( s->size + add_bytes > s->max ) {
        int64_t new_max = s->max * 2 + add_bytes;        
        char* new_data = (char*) malloc ( new_max );
        memcpy ( new_data, s->geom, s->size );
        free ( s->geom );
        s->geom = new_data;
		assert ( s->size + add_bytes < new_max );
        s->max = new_max;
        s->mem = float(new_max) / (1024.0f*1024);
    }
    // attach a prim start marker if needed
    if ( need_marker ) {
		  // write prim count to last prim
		  finishPrim ( s );
		  // attach new prim
      attachPrim ( s, typ, prim );
		  // reset prim count
		  m_curr_type = typ;
      m_curr_prim = prim;		
		  m_curr_num = 0;
	  }
}

void gxLib::attachPrim ( gxSet* s, uchar typ, uchar prim )
{
  gxPrim* p = (gxPrim*) (s->geom + s->size);
  p->typ = typ;
  p->prim = prim;
	p->img_ptr = m_curr_img;
  p->cnt = 0;
	s->lastpos = s->size;	// save position of prim
  s->size += sizeof(gxPrim);	
}

gxVert* gxLib::allocGeom2D ( int cnt, uchar prim )
{
  gxSet* s = gx.getCurrSet();	
  expandSet ( s, 'x', prim, cnt * sizeof(gxVert) );    
	m_curr_num += cnt;
	gxVert* start = (gxVert*) (s->geom + s->size);
	s->size += cnt*sizeof(gxVert);	
  return start;
}
gxVert* gxLib::allocImg2D (int cnt, uchar prim, ImageX* img )
{
	m_curr_img = (uint64_t) img;			// assign image to prims
  gxSet* s = gx.getCurrSet();	
  expandSet ( s, 'i', prim, cnt * sizeof(gxVert) );    
	m_curr_num += cnt;
	gxVert* start = (gxVert*) (s->geom + s->size);
	s->size += cnt*sizeof(gxVert);
  return start;
}

gxVert3D* gxLib::allocGeom3D ( int cnt, uchar prim )
{
  gxSet* s = gx.getCurrSet();	
  expandSet ( s, 'x', prim, cnt * sizeof(gxVert3D) );    
	m_curr_num += cnt;
	gxVert3D* start = (gxVert3D*) (s->geom + s->size);
	s->size += cnt*sizeof(gxVert3D);	
  return start;
}
gxVert3D* gxLib::allocGeom3D (int cnt, uchar prim, ImageX* img )
{
	m_curr_img = (uint64_t) img;			// assign image to prims
  gxSet* s = gx.getCurrSet();	
  expandSet ( s, 'i', prim, cnt * sizeof(gxVert3D) );    
	m_curr_num += cnt;
	gxVert3D* start = (gxVert3D*) (s->geom + s->size);
	s->size += cnt*sizeof(gxVert3D);
  return start;
}



void gxLib::createShader2D ()
{
	#ifdef USE_OPENGL
		// OpenGL - Create shaders
		char buf[16384];
		int len = 0;

		// OpenGL 4.2 Core
		// -- Cannot use hardware lighting pipeline (e.g. glLightfv, glMaterialfv)
		mVS[S2D] = glCreateShader(GL_VERTEX_SHADER);
		GLchar const * vss =
			"#version 300 es\n"
			"#extension GL_ARB_explicit_attrib_location : enable\n"
			"\n"			
			"layout(location = 0) in vec3 inVertex;\n"
			"layout(location = 1) in uint inColor;\n"
			"layout(location = 2) in vec2 inTexCoord;\n"
			"out vec3 position;\n"		
			"out vec4 color;\n"				
			"out vec2 texcoord;\n"
			"uniform mat4 modelMatrix;\n"
			"uniform mat4 viewMatrix;\n"
			"uniform mat4 projMatrix;\n"
			"\n"
			"vec4 CLRtoVEC(uint c) { return vec4( float(c & uint(0xFF))/255.0, float((c>>8) & uint(0xFF))/255.0, float((c>>16) & uint(0xFF))/255.0, float((c>>24) & uint(0xFF))/255.0 ); }\n"
			"\n"
			"void main()\n"
			"{\n"		
			"	 position = (modelMatrix * vec4(inVertex,1)).xyz;\n"
			"    color = CLRtoVEC( inColor );\n"
			"    texcoord = inTexCoord;\n"
			"    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(inVertex,1);\n"
			"}\n"
		;
		glShaderSource(mVS[S2D], 1, &vss, 0);
		glCompileShader(mVS[S2D]);
		glGetShaderInfoLog (mVS[S2D], 16384, (GLsizei*) &len, buf );
		if ( len > 0 ) dbgprintf  ( "ERROR S_2D vertex shader: %s\n", buf );
		checkGL( "Compile S_2D vertex shader" );

		mFS[S2D] = glCreateShader(GL_FRAGMENT_SHADER);
		GLchar const * fss =
			"#version 300 es\n"
			"\n"					
			"  precision mediump float;\n"
			"  precision mediump int;\n"			
			"uniform sampler2D imgTex;\n"
			"in vec3 position;\n"
			"in vec4 color;\n"
			"in vec2 texcoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"\n"
			"void main()\n"
			"{\n"					
			"    vec4 imgclr = texture(imgTex, texcoord);\n"									
			"    outColor = color * imgclr;\n"
			"}\n"
		;
		

		glShaderSource(mFS[S2D], 1, &fss, 0);
		glCompileShader(mFS[S2D]);
		glGetShaderInfoLog ( mFS[S2D], 16384, (GLsizei*) &len, buf );
		if ( len > 0 ) dbgprintf  ( "ERROR S_2D fragment shader: %s\n", buf );
		checkGL( "Compile S_2D fragment shader" );

		mSH[ S2D ] = glCreateProgram();
		glAttachShader( mSH[ S2D ], mVS[S2D]);
		glAttachShader( mSH[ S2D ], mFS[S2D]);
		checkGL( "Attach program" );
		glLinkProgram( mSH[ S2D ] );
		checkGL( "Link program" );
		glUseProgram( mSH[ S2D ] );
		checkGL( "Use program" );

		mPARAM[S2D][SP_PROJ] =		glGetUniformLocation ( mSH[S2D], "projMatrix" );
		mPARAM[S2D][SP_MODEL] =	glGetUniformLocation ( mSH[S2D], "modelMatrix" );
		mPARAM[S2D][SP_VIEW] =		glGetUniformLocation ( mSH[S2D], "viewMatrix" );
		mPARAM[S2D][SP_TEX] =		glGetUniformLocation ( mSH[S2D], "imgTex" );		

		checkGL( "Get S_2D uniforms" );
	#endif
}

void gxLib::createShader3D ()
{
	#ifdef BUILD_OPENGL
	
	// OpenGL - Create shaders
	char buf[16384];
	int len = 0;
	checkGL( "Start shaders" );

	// OpenGL 4.2 Core
	// -- Cannot use hardware lighting pipeline (e.g. glLightfv, glMaterialfv)
	mVS[S3D] = glCreateShader(GL_VERTEX_SHADER);
	GLchar const * vss =
			"#version 300 es\n"
			"#extension GL_ARB_explicit_attrib_location : enable\n"
			"\n"
			"layout(location = 0) in vec3 inPosition;\n"
			"layout(location = 1) in uint inColor;\n"
			"layout(location = 2) in vec2 inTexCoord;\n"
			"layout(location = 3) in vec3 inNorm;\n"
			"out vec3 vpos;\n"		
			"out vec4 vcolor;\n"				
			"out vec2 vtexcoord;\n"
			"out vec3 vnorm;\n"
			"uniform mat4 modelMatrix;\n"
			"uniform mat4 viewMatrix;\n"
			"uniform mat4 projMatrix;\n"
		"\n"
		"mat4 getInvTranspose( mat4 m ) {\n"	
		"   m[3][0] = 0.f;\n"
		"   m[3][1] = 0.f;\n"
		"   m[3][2] = 0.f;\n"	
		"	float a2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];\n"
		"	float a1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];\n"
		"	float a1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];\n"
		"	float a0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];\n"
		"	float a0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];\n"
		"	float a0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];\n"
		"	float a2313 = m[1][2] * m[3][3] - m[1][3] * m[3][2];\n"
		"	float a1313 = m[1][1] * m[3][3] - m[1][3] * m[3][1];\n"
		"	float a1213 = m[1][1] * m[3][2] - m[1][2] * m[3][1];\n"
		"	float a2312 = m[1][2] * m[2][3] - m[1][3] * m[2][2];\n"
		"	float a1312 = m[1][1] * m[2][3] - m[1][3] * m[2][1];\n"
		"	float a1212 = m[1][1] * m[2][2] - m[1][2] * m[2][1];\n"
		"	float a0313 = m[1][0] * m[3][3] - m[1][3] * m[3][0];\n"
		"	float a0213 = m[1][0] * m[3][2] - m[1][2] * m[3][0];\n"
		"	float a0312 = m[1][0] * m[2][3] - m[1][3] * m[2][0];\n"
		"	float a0212 = m[1][0] * m[2][2] - m[1][2] * m[2][0];\n"
		"	float a0113 = m[1][0] * m[3][1] - m[1][1] * m[3][0];\n"
		"	float a0112 = m[1][0] * m[2][1] - m[1][1] * m[2][0];\n"
		"\n"
		"float det = m[0][0] * ( m[1][1] * a2323 - m[1][2] * a1323 + m[1][3] * a1223 )"
		"	 - m[0][1] * ( m[1][0] * a2323 - m[1][2] * a0323 + m[1][3] * a0223 )"
		"	 + m[0][2] * ( m[1][0] * a1323 - m[1][1] * a0323 + m[1][3] * a0123 )"
		"	 - m[0][3] * ( m[1][0] * a1223 - m[1][1] * a0223 + m[1][2] * a0123 );\n"
	    "det = 1.f / det;\n"
		"mat4 im;\n"
		"im[0][0] = det *   ( m[1][1] * a2323 - m[1][2] * a1323 + m[1][3] * a1223 );\n"
		"im[0][1] = det * - ( m[0][1] * a2323 - m[0][2] * a1323 + m[0][3] * a1223 );\n"
		"im[0][2] = det *   ( m[0][1] * a2313 - m[0][2] * a1313 + m[0][3] * a1213 );\n"
		"im[0][3] = det * - ( m[0][1] * a2312 - m[0][2] * a1312 + m[0][3] * a1212 );\n"
		"im[1][0] = det * - ( m[1][0] * a2323 - m[1][2] * a0323 + m[1][3] * a0223 );\n"
		"im[1][1] = det *   ( m[0][0] * a2323 - m[0][2] * a0323 + m[0][3] * a0223 );\n"
		"im[1][2] = det * - ( m[0][0] * a2313 - m[0][2] * a0313 + m[0][3] * a0213 );\n"
		"im[1][3] = det *   ( m[0][0] * a2312 - m[0][2] * a0312 + m[0][3] * a0212 );\n"
		"im[2][0] = det *   ( m[1][0] * a1323 - m[1][1] * a0323 + m[1][3] * a0123 );\n"
		"im[2][1] = det * - ( m[0][0] * a1323 - m[0][1] * a0323 + m[0][3] * a0123 );\n"
		"im[2][2] = det *   ( m[0][0] * a1313 - m[0][1] * a0313 + m[0][3] * a0113 );\n"
		"im[2][3] = det * - ( m[0][0] * a1312 - m[0][1] * a0312 + m[0][3] * a0112 );\n"
		"im[3][0] = det * - ( m[1][0] * a1223 - m[1][1] * a0223 + m[1][2] * a0123 );\n"
		"im[3][1] = det *   ( m[0][0] * a1223 - m[0][1] * a0223 + m[0][2] * a0123 );\n"
		"im[3][2] = det * - ( m[0][0] * a1213 - m[0][1] * a0213 + m[0][2] * a0113 );\n"
		"im[3][3] = det *   ( m[0][0] * a1212 - m[0][1] * a0212 + m[0][2] * a0112 );\n"
		"return im; } \n"
		/*"mat4 getInvTranspose( mat4 a ) {\n"		
		"   a[3][0] = 0.0;\n"
		"   a[3][1] = 0.0;\n"
		"   a[3][2] = 0.0;\n"
		"   float s0 = a[0][0] * a[1][1] - a[1][0] * a[0][1];\n"
		"	float s1 = a[0][0] * a[1][2] - a[1][0] * a[0][2];\n"
		"	float s2 = a[0][0] * a[1][3] - a[1][0] * a[0][3];\n"
		"	float s3 = a[0][1] * a[1][2] - a[1][1] * a[0][2];\n"
		"	float s4 = a[0][1] * a[1][3] - a[1][1] * a[0][3];\n"
		"	float s5 = a[0][2] * a[1][3] - a[1][2] * a[0][3];\n"
		"	float c5 = a[2][2] * a[3][3] - a[3][2] * a[2][3];\n"
		"	float c4 = a[2][1] * a[3][3] - a[3][1] * a[2][3];\n"
		"	float c3 = a[2][1] * a[3][2] - a[3][1] * a[2][2];\n"
		"	float c2 = a[2][0] * a[3][3] - a[3][0] * a[2][3];\n"
		"	float c1 = a[2][0] * a[3][2] - a[3][0] * a[2][2];\n"
		"	float c0 = a[2][0] * a[3][1] - a[3][0] * a[2][1];\n"
		"	float invdet = 1.0 / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);\n"
		"   mat4 m;\n"
		"	m[0][0] = ( a[1][1] * c5 - a[1][2] * c4 + a[1][3] * c3) * invdet;\n"
		"	m[0][1] = (-a[0][1] * c5 + a[0][2] * c4 - a[0][3] * c3) * invdet;\n"
		"	m[0][2] = ( a[3][1] * s5 - a[3][2] * s4 + a[3][3] * s3) * invdet;\n"
		"	m[0][3] = (-a[2][1] * s5 + a[2][2] * s4 - a[2][3] * s3) * invdet;\n"
		"	m[1][0] = (-a[1][0] * c5 + a[1][2] * c2 - a[1][3] * c1) * invdet;\n"
		"	m[1][1] = ( a[0][0] * c5 - a[0][2] * c2 + a[0][3] * c1) * invdet;\n"
		"	m[1][2] = (-a[3][0] * s5 + a[3][2] * s2 - a[3][3] * s1) * invdet;\n"
		"	m[1][3] = ( a[2][0] * s5 - a[2][2] * s2 + a[2][3] * s1) * invdet;\n"
		"	m[2][0] = ( a[1][0] * c4 - a[1][1] * c2 + a[1][3] * c0) * invdet;\n"
		"	m[2][1] = (-a[0][0] * c4 + a[0][1] * c2 - a[0][3] * c0) * invdet;\n"
		"	m[2][2] = ( a[3][0] * s4 - a[3][1] * s2 + a[3][3] * s0) * invdet;\n"
		"	m[2][3] = (-a[2][0] * s4 + a[2][1] * s2 - a[2][3] * s0) * invdet;\n"
		"   m[3][0] = (-a[1][0] * c3 + a[1][1] * c1 - a[1][2] * c0) * invdet;\n"
		"	m[3][1] = ( a[0][0] * c3 - a[0][1] * c1 + a[0][2] * c0) * invdet;\n"
		"	m[3][2] = (-a[3][0] * s3 + a[3][1] * s1 - a[3][2] * s0) * invdet;\n"
		"	m[3][3] = ( a[2][0] * s3 - a[2][1] * s1 + a[2][2] * s0) * invdet;\n"
		"    return m; }\n"*/
			"\n"
			"vec4 CLRtoVEC(uint c) { return vec4( float(c & uint(0xFF))/255.0, float((c>>8) & uint(0xFF))/255.0, float((c>>16) & uint(0xFF))/255.0, float((c>>24) & uint(0xFF))/255.0 ); }\n"			
			"\n"
			"void main()\n"
			"{\n"		
			"	 vpos = (modelMatrix * vec4(inPosition, 1.f)).xyz;\n"
			"  vcolor = CLRtoVEC ( inColor );\n"
			"  vtexcoord = inTexCoord;\n"
			"	 vnorm = (inNorm.x < -1.f) ? inNorm : normalize ( (vec4(inNorm,1) * getInvTranspose(modelMatrix)).xyz );\n"
			"  gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(inPosition, 1.f);\n"
			"}\n"
		;

	glShaderSource(mVS[S3D], 1, &vss, 0);
	glCompileShader(mVS[S3D]);
	glGetShaderInfoLog (mVS[S3D], 16384, (GLsizei*) &len, buf );
	if ( len > 0 ) dbgprintf  ( "ERROR CreateS3D vert: %s\n", buf );
	checkGL( "createShader3D Compile vertex shader" );

	mFS[S3D] = glCreateShader(GL_FRAGMENT_SHADER);
	GLchar const * fss =
		"#version 300 es\n"
		"  precision mediump float;\n"
		"  precision mediump int;\n"
		"uniform sampler2D imgTex;\n"
		"uniform sampler2D envTex;\n"
		"in vec3		vpos; \n"
		"in vec4		vcolor; \n"		
		"in vec2		vtexcoord; \n"
		"in vec3		vnorm; \n"
		"uniform vec3   amb_clr; \n"		
		"uniform vec3   diff_clr; \n"		
		"uniform vec3   spec_clr; \n"
		"uniform float  spec_power; \n"
		"uniform vec3   lightclr; \n"
		"uniform vec3	lightpos; \n"
		"uniform vec3	campos; \n"
		"out vec4		outColor;\n"
		"#define PI		3.141592f\n"
		"void main () {\n"		
		"	  vec3 lgtdir;\n"
		"		// environment probe lighting\n"
		"		vec2 ec;\n"
		"		vec3 dir, eclr;\n"
		"		eclr = vec3(0,0,0);\n"
		"		for (ec.x = 0.f; ec.x < 1.f; ec.x += 0.2f) {\n"
		"			for (ec.y = 0.f; ec.y < 1.f; ec.y += 0.2f) {\n"
		"				dir = vec3( cos(ec.x*2.f*PI)*sin(ec.y*PI), cos(ec.y*PI), sin(ec.x*2.f*PI)*sin(ec.y*PI) ) * 100.0f;\n"
		"				lgtdir = normalize ( dir - vpos );\n"
		"				eclr += 0.040f * texture( envTex, ec ).xyz * max(0.0f, dot( vnorm, lgtdir ));\n"
		"			}\n"
		"		}\n" 
		"    vec4 imgclr = vcolor * texture(imgTex, vtexcoord );\n"		
		"    lgtdir = normalize(lightpos - vpos);\n"
		"    vec3 V = normalize( campos - vpos );\n"
		"    vec3 R = normalize( vnorm * (2.0f*dot ( vnorm, lgtdir)) - lgtdir );\n"
		"    if (vnorm.x<-1.f) {\n"
		"       outColor = imgclr;\n"
		"    } else {\n"
		"       vec3 sclr = spec_clr * pow( max(0.0f, dot(R, V)), spec_power );\n"
		"       vec3 dclr = diff_clr * max( 0.0f, dot ( vnorm, lgtdir )); \n"
		"       outColor = vec4( eclr + lightclr * (amb_clr + dclr * imgclr.xyz + sclr), imgclr.w );\n"				
		"    }\n"		
		//"    outColor = vec4( 1,0,0,1 );\n"
		//"    outColor = vec4( eclr, imgclr.w );\n"
		//"    outColor = vec4( lightclr * (amb_clr + dclr * imgclr.xyz + sclr), imgclr.w );\n"		
		//"    outColor = vec4( diff_clr, imgclr.w );\n"		
		//"    outColor = vec4( vnorm, imgclr.w );\n"		
		"}\n"
	;

	/* "void main () {\n"
		"  outColor = vec4(1, 0, 0, 1); \n"
		"}\n"; */

	//"   float d = 0.1f + 0.9f * clamp( dot ( vnorm, normalize(lightpos-vpos) ), 0.f, 1.f); \n"
	// "   outColor = vec4(d, d, d, 1) * vcolor;\n"
	// "   outColor = vec4(vnorm.x, vnorm.y, vnorm.z, 1) * vcolor;\n"

	glShaderSource(mFS[S3D], 1, &fss, 0);
	glCompileShader(mFS[S3D]);
	glGetShaderInfoLog (mFS[S3D], 16384, (GLsizei*) &len, buf );
	if ( len > 0 ) dbgprintf  ( "ERROR CreateS3D frag: %s\n", buf );
	checkGL( "createShader3D Compile fragment shader" );

	mSH[S3D] = glCreateProgram();
	glAttachShader( mSH[S3D], mVS[S3D]);
	glAttachShader( mSH[S3D], mFS[S3D]);
	checkGL( "createShader3D attach program" );
	glLinkProgram( mSH[S3D] );
	checkGL( "createShader3D link program" );
	glUseProgram( mSH[S3D] );
	checkGL( "createShader3D use program" );

	mPARAM[S3D][SP_PROJ] =		glGetUniformLocation ( mSH[S3D], "projMatrix" );
	mPARAM[S3D][SP_MODEL] =		glGetUniformLocation ( mSH[S3D], "modelMatrix" );
	mPARAM[S3D][SP_VIEW] =		glGetUniformLocation ( mSH[S3D], "viewMatrix" );
	mPARAM[S3D][SP_CAMPOS] =	glGetUniformLocation ( mSH[S3D], "campos" );
	mPARAM[S3D][SP_LIGHTPOS] =	glGetUniformLocation ( mSH[S3D], "lightpos" );
	mPARAM[S3D][SP_LIGHTCLR] =	glGetUniformLocation ( mSH[S3D], "lightclr" );	
	mPARAM[S3D][SP_TEX] =		glGetUniformLocation ( mSH[S3D], "imgTex" );	
	mPARAM[S3D][SP_AMBCLR] =	glGetUniformLocation ( mSH[S3D], "amb_clr" );
	mPARAM[S3D][SP_DIFFCLR] =	glGetUniformLocation ( mSH[S3D], "diff_clr" );
	mPARAM[S3D][SP_SPECPOW] =	glGetUniformLocation ( mSH[S3D], "spec_power" );
	mPARAM[S3D][SP_SPECCLR] =	glGetUniformLocation ( mSH[S3D], "spec_clr" );
	mPARAM[S3D][SP_ENVMAP] =	glGetUniformLocation ( mSH[S3D], "envTex" );

	checkGL( "Get Shader Matrices" );	

	#endif
}

bool gxLib::loadFont ( const char * fontName )
{
	if (!fontName) return false;

	// Tristan's font file format
	struct GlyphInfo {
		struct Pix {			// pixel oriented data       
			int u, v;
			int width, height;
			int advance;
			int offX, offY;
		};
		struct Norm {		// normalized data       
			float u, v;		// position in the map in normalized coords
			float width, height;
			float advance;
			float offX, offY;
		};
		Pix  pix;
		Norm norm;
	};
	struct FileHeader {
		int texwidth, texheight;
		struct Pix {
			int ascent;
			int descent;
			int linegap;
		};
		struct Norm {
			float ascent;
			float descent;
			float linegap;
		};
		Pix  pix;
		Norm norm;
		GlyphInfo glyphs[256];
	};

	// find file
	char fname[200], fpath[1024];
	sprintf (fname, "%s.tga", fontName);
	if ( !getFileLocation ( fname, fpath ) ) {
		dbgprintf ( "**** ERROR: Cannot find font file: %s\n", fname );
		return false;
	}
	
	// load TGA file
	if (!m_font_img.Load (fpath) ) {
		dbgprintf( "ERROR: Must build with TGA support for fonts.\n" );
		return false;	
	}

	// make all pixels white. only alpha is glyph.
	// since shader multiplies by pixel clr: out = vert clr * pix(r,g,b,a)
	m_font_img.CopyToAlpha ();

	// change extension from .tga to .bin
	int l = (int) strlen(fpath);
	fpath[l-3] = 'b';	fpath[l-2] = 'i'; fpath[l-1] = 'n';	fpath[l-0] = '\0';
	FILE *fd = fopen( fpath, "rb" );
	if ( !fd ) {
		dbgprintf("ERROR: Cannot find %s.bin\n", fname);
		return false;
	}	

	// read entire .bin file (Tristan's format)
	FileHeader glyphInfos;
	int r = (int) fread ( &glyphInfos, 1, sizeof(FileHeader), fd);
	fclose(fd);

	// fix glyph offsets (make all positive)
	float ymin = 10000000, ymax = 0;
	for (int c=0; c < 256; c++ ) {
		GlyphInfo& gly = glyphInfos.glyphs[c];
		if ( gly.pix.offY < ymin) ymin = gly.pix.offY;		
	}
	for (int c=0; c < 256; c++ ) {
		GlyphInfo& gly = glyphInfos.glyphs[c];
		gly.pix.offY -= ymin;
	}		
	
	// put into GX Font structure
	m_font.ascent = glyphInfos.pix.ascent;
	m_font.descent = glyphInfos.pix.descent;
	m_font.linegap = glyphInfos.pix.linegap;
	for (int c=0; c < 256; c++) {		
		m_font.glyphs[c].width = glyphInfos.glyphs[c].pix.width;
		m_font.glyphs[c].height = glyphInfos.glyphs[c].pix.height;
		m_font.glyphs[c].advance = glyphInfos.glyphs[c].pix.advance;
		m_font.glyphs[c].offX = glyphInfos.glyphs[c].pix.offX;
		m_font.glyphs[c].offY = glyphInfos.glyphs[c].pix.offY;
		m_font.glyphs[c].u = glyphInfos.glyphs[c].norm.u;
		m_font.glyphs[c].du = glyphInfos.glyphs[c].norm.width;

		m_font.glyphs[c].v = 1 - glyphInfos.glyphs[c].norm.v;			// NOTE: Fonts are rendered from bottom up in .tga by bakeFonts.exe		
		m_font.glyphs[c].dv = - glyphInfos.glyphs[c].norm.height;
	}

	return true;
}

void gxLib::destroy()
{
	// free all sets from memory
	dbgprintf("destroy2D. Sets.\n");
	destroySets();

	// clear font from memory
	dbgprintf("destroy2D. Fonts.\n");
	m_font_img.Clear();

	dbgprintf("destroy2D. Shaders.\n");
	// remove 2D shaders
	glDetachShader(mSH[S2D], mVS[S2D]);
	glDetachShader(mSH[S2D], mFS[S2D]);
	glDeleteShader(mVS[S2D]); 
	glDeleteShader(mFS[S2D]);
	// remove 3D shaders
	glDetachShader(mSH[S3D], mVS[S3D]);
	glDetachShader(mSH[S3D], mFS[S3D]);	
	glDeleteShader(mVS[S3D]); 
	glDeleteShader(mFS[S3D]);
	mVS[S2D] = 0; mFS[S2D] = 0;
	mVS[S3D] = 0; mFS[S3D] = 0;

	// delete programs
	dbgprintf("destroy2D. SH2: %d, SH3: %d, VAO: %d\n", mSH[S2D], mSH[S3D], mVAO);
	glDeleteProgram(mSH[S3D]);
	glDeleteProgram(mSH[S2D]);
	mSH[S3D] = 0;
	mSH[S2D] = 0;
	
	// delete VAO
	glDeleteVertexArrays(1, (GLuint*)&mVAO);
	mVAO = 0;

}

void gxLib::drawSets ()
{
	for (int n=0; n < m_sets.size(); n++)
		drawSet ( n );
}

void gxLib::drawSet ( int g )
{
	gxSet* s = gx.getSet( g );
	if (s->size==0) return;

	PERF_PUSH ("drawSet");

	#ifdef _WIN32
		TimeX tm;
		float t1, t2;
		tm.SetTimeNSec ();
		t1 = tm.GetElapsedMSec (basetime);
	#else
		clock_t t1, t2;
		t1 = clock();
	#endif	
	// select shader
	int sh = (s->stype=='3') ? S3D : S2D;

	// opengl render setup
	#ifdef BUILD_OPENGL
		// viewport clipping
		if ( s->region.z != 0 ) {	
			glViewport ( s->region.x, s->region.y, s->region.z, s->region.w );
		}
		// bind shader	
		glUseProgram ( mSH[ sh ] );	
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet use program");
		#endif
	
		// bind vao
		glBindVertexArray (gx.mVAO);
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet bind vao");
		#endif

		// opengl enable prep	
		if (sh==S3D) {		
			// 3D setup
			glEnable(GL_DEPTH_TEST);
			#ifdef EXTRA_CHECKS
				checkGL ( "drawSet enable depth");
			#endif
		} else {
			// 2D setup
			glDisable ( GL_DEPTH_TEST );
			glDepthFunc ( GL_LEQUAL );
			#ifdef EXTRA_CHECKS
				checkGL ( "drawSet disable depth");
			#endif
		}
		glEnable ( GL_BLEND );		
		glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet enable blend");
		#endif

		// create & update vbo
		updateVBO ( s );
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet update vbo");
		#endif
	
		// update view matrices
		glUniformMatrix4fv ( mPARAM[sh][SP_PROJ],  1, GL_FALSE, s->proj_mtx );
		glUniformMatrix4fv ( mPARAM[sh][SP_MODEL], 1, GL_FALSE, s->model_mtx );
		glUniformMatrix4fv ( mPARAM[sh][SP_VIEW],  1, GL_FALSE, s->view_mtx );
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet matrices");
		#endif

		if (sh==S3D) {
			// update material
			glUniform3f ( gx.mPARAM[sh][SP_AMBCLR],  s->ambclr.x, s->ambclr.y, s->ambclr.z );
			glUniform3f ( gx.mPARAM[sh][SP_DIFFCLR], s->diffclr.x, s->diffclr.y, s->diffclr.z );
			glUniform3f ( gx.mPARAM[sh][SP_SPECCLR], s->specclr.x, s->specclr.y, s->specclr.z );
			glUniform1f ( gx.mPARAM[sh][SP_SPECPOW], s->specpow );			
			#ifdef EXTRA_CHECKS
				checkGL ( "drawSet material");
			#endif
	
			// update light
			glUniform3f ( gx.mPARAM[sh][SP_LIGHTPOS], s->lightpos.x, s->lightpos.y, s->lightpos.z );		
			glUniform3f ( gx.mPARAM[sh][SP_LIGHTCLR], s->lightclr.x, s->lightclr.y, s->lightclr.z );
			#ifdef EXTRA_CHECKS
				checkGL ( "drawSet light");
			#endif

			// update envmap		
			glActiveTexture ( GL_TEXTURE0 + 1 );		
			glBindTexture ( GL_TEXTURE_2D, s->envmap );
			glUniform1i ( mPARAM[sh][SP_ENVMAP], 1 );
		}

		// enable attrib arrays
		glEnableVertexAttribArray( slotPos );
		glEnableVertexAttribArray( slotClr );
		glEnableVertexAttribArray( slotUVs );
		if (sh==S3D) 
			glEnableVertexAttribArray(slotNorm);	
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet enable slots");
		#endif	

		// bind vbo
		glBindBuffer ( GL_ARRAY_BUFFER, s->vbo );	
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet bind vbo");
		#endif

		// bind texture slot	
		glActiveTexture ( GL_TEXTURE0 );
		glUniform1i ( mPARAM[sh][SP_TEX], 0 );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );	
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet active tex");
		#endif
	#endif

	gxPrim* p;
	int elem_sz;
	int cnt;    
	signed long long pos = 0;
	#ifdef BUILD_OPENGL
		GLuint imgGL;	
	#endif

	// parse and render command stream
	//
	if ( sh==S3D ) {
		//------------------------------ 3D draw calls
		s->calls = 0;
		s->primcnt = 0;
		while ( pos < s->size ) {

			// get prim marker
			p = (gxPrim*) (s->geom + pos);		
			cnt = p->cnt;				
			elem_sz = sizeof(gxVert3D);
			pos += sizeof(gxPrim);		// goto start of geometry

			// opengl render
			#ifdef BUILD_OPENGL
				// point to section of vbo				
				glVertexAttribPointer( slotPos, 3, GL_FLOAT, GL_FALSE, sizeof(gxVert3D), (void*) (pos + 0) );
				checkGL ( "glVertAttrPtr pos 3d");
				glVertexAttribIPointer(slotClr, 1, GL_UNSIGNED_INT,    sizeof(gxVert3D), (void*) (pos + 12) );
				checkGL ( "glVertAttrPtr clr 3d");
				glVertexAttribPointer( slotUVs, 2, GL_FLOAT, GL_FALSE, sizeof(gxVert3D), (void*) (pos + 16) );
				checkGL ( "glVertAttrPtr UV 3d");
				glVertexAttribPointer( slotNorm,3, GL_FLOAT, GL_FALSE, sizeof(gxVert3D), (void*) (pos + 24) );
				checkGL ( "glVertAttrPtr norm 3d");

				// bind image
				imgGL = (p->typ=='x') ? m_white_img.getGLID() : (p->typ=='i') ? ((ImageX*) p->img_ptr)->getGLID() : 0;
				glBindTexture ( GL_TEXTURE_2D, imgGL );
		
				// render elements	
				switch (p->prim) {
				case PRIM_LINES:	glDrawArrays(GL_LINES, 0, cnt);				break;
				case PRIM_TRI:		glDrawArrays(GL_TRIANGLE_STRIP, 0, cnt);	break;
				};
			#endif
			s->calls++;		
			s->primcnt += cnt;

			// next prim marker
			pos += cnt * elem_sz;	
		}

	} else {
		//------------------------------ 2D draw calls
		s->calls = 0;
		s->primcnt = 0;
		while ( pos < s->size ) {

			// get prim marker
			p = (gxPrim*) (s->geom + pos);		
			cnt = p->cnt;				
			elem_sz = sizeof(gxVert);
			pos += sizeof(gxPrim);		// goto start of geometry

			// opengl render
			#ifdef BUILD_OPENGL

				// point to section of vbo
				glVertexAttribPointer( slotPos, 3, GL_FLOAT, GL_FALSE, sizeof(gxVert), (void*) (pos + 0) );
				checkGL ( "glVertAttrPtr pos 2d");
				glVertexAttribIPointer(slotClr, 1, GL_UNSIGNED_INT,    sizeof(gxVert), (void*) (pos + 12) );
				checkGL ( "glVertAttrPtr clr 2d");
				glVertexAttribPointer( slotUVs, 2, GL_FLOAT, GL_FALSE, sizeof(gxVert), (void*) (pos + 16) );
				checkGL ( "glVertAttrPtr uv 2d");
			
				// bind image
				imgGL = (p->typ=='x') ? m_white_img.getGLID() : (p->typ=='i') ? ((ImageX*) p->img_ptr)->getGLID() : 0;			
				glBindTexture ( GL_TEXTURE_2D, imgGL );
		
				// render elements	
				switch (p->prim) {
				case PRIM_LINES:	glDrawArrays(GL_LINES, 0, cnt);				break;
				case PRIM_TRI:		glDrawArrays(GL_TRIANGLE_STRIP, 0, cnt);	break;		
				};
			#endif
		
			//dbgprintf ( "%c", (pt==PRIM_TRI) ? '/' : '-' );
			s->calls++;		
			s->primcnt += cnt;

			// next prim marker
			pos += cnt * elem_sz;	
		}
	}
	#ifdef EXTRA_CHECKS
		checkGL ( "drawSet stream");
	#endif

	// stop using shader
	#ifdef BUILD_OPENGL
		glUseProgram ( 0 );
		#ifdef EXTRA_CHECKS
			checkGL ( "drawSet stop shader");
		#endif
	#endif

	#ifdef _WIN32
		tm.SetTimeNSec ();
		t2 = tm.GetElapsedMSec (basetime);
		float msec = t2 - t1;
	#else
		t2 = clock();
		float msec = double(t2 - t1)*1000.0 / double(CLOCKS_PER_SEC);
	#endif	

	if ( m_debug ) {
		dbgprintf ( "gx: %s %s, vtx %d, draw calls %d (%4.2fmb), %3.3f msec\n",  
			(s->stype=='2') ? "2D" : "3D",
			(s->sstatic==true) ? "static" : "dyn",
			s->primcnt,
			s->calls, 
			s->mem, 
			msec );
	}
	PERF_POP ();

}

void gxLib::updateVBO ( gxSet* s )
{
	#ifdef BUILD_OPENGL
		// generate new vbo	
		if ( s->vbo == 0 ) {
			glGenBuffers ( 1, (GLuint*) &s->vbo );
			checkGL ( "updateVBO:glGenBuffers" );
		}
		// bind buffer
		glBindBuffer ( GL_ARRAY_BUFFER, s->vbo );			

		if (s->bufsize != s->max ) {
			// reallocate buffer on GPU
			s->bufsize = s->max;
			glBufferData ( GL_ARRAY_BUFFER, s->bufsize, s->geom, GL_DYNAMIC_DRAW );
		} else {
			// avoid the reallocation if size is equal or smaller
			glBufferSubData ( GL_ARRAY_BUFFER, 0, s->size, s->geom );
		}
		#ifdef EXTRA_CHECKS
			checkGL ( "gxLib updateVBO" );		
		#endif
	#endif
}

