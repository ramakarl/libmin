

#include <math.h>
#include <assert.h>

#include "meshx.h"
#include "meshx_info.h"
#include "string_helper.h"
#include "geom_helper.h"

//-----------------------------------------------------
//  Mesh 
//-----------------------------------------------------
MeshX::MeshX () 
{
	m_AddVertFunc = 0;
	m_AddFaceFast3Func = 0;
	m_AddFaceFast4Func = 0;	
	m_Format = MF_UNDEF;

}


MeshX::~MeshX ()
{
	DeleteAllBuffers();
}

bool MeshX::Load (std::string fname, float scal )
{	
	char fpath[1024];
	strncpy(fpath, fname.c_str(), 1024);
	std::string ext = strSplitRight (fname, ".");
	if (ext.compare("obj") == 0) return LoadObj(fpath, scal);
	if (ext.compare("ply") == 0) return LoadPly(fpath, scal);
	return false;
}

Vec4F MeshX::GetStats()
{
	// stats. x=memused (bytes), y=elements, z=resolution
	float mem=0;
	for (int i=0; i < GetNumBuf(); i++)
		mem += (GetNumElem(i)*GetBufStride(i));

	return Vec4F( mem, GetNumFace3(), GetNumVert(), 0);
}


void MeshX::SetFormatFunc ()
{
	switch ( m_Format ) {
	case MF_FV:		SetFuncFV();		break;
	case MF_FVF:	SetFuncFVF();		break;
	case MF_CM:		SetFuncCM();		break;
	}
}

void MeshX::SetUVRes ( int u, int v )
{
	m_Ures = u;
	m_Vres = v;
}


void MeshX::Clear ()
{
	EmptyAllBuffers ();
}

void MeshX::Debug ()
{
	switch ( m_Format ) {
	case MF_FV:		DebugFV (); break;
	case MF_CM:		DebugCM (); break;
	case MF_FVF:	DebugFVF (); break;
	}
}


//------------------------------------------ GENERIC FUNCTIONS
//
void MeshX::ComputeNormals (bool flat)
{
	Vec3F norm, side;
	AttrV3 *f;
	Vec3F v1, v2, v3;
	Vec3F* vn;

	if ( !isActive(BVERTNORM) ) {
		// add normal buffer
		int64_t cnt = GetNumVert ();
		AddBuffer ( BVERTNORM, "norm", sizeof(Vec3F), cnt );
		ReserveBuffer ( BVERTNORM, cnt );
	}

	// Clear vertex normals	
	for (vn = (Vec3F*) GetStart(BVERTNORM); vn <= (Vec3F*) GetEnd(BVERTNORM); vn++) {
		vn->x = 0; vn->y = 0; vn->z = 0;
	}

	// Compute normals of all faces
	if (flat) {
		// Flat normals
		for (f = (AttrV3*) GetStart(BFACEV3); f <= (AttrV3*) GetEnd(BFACEV3); f++ ) {
			v1 = *GetVertPos( f->v1 );	v2 = *GetVertPos( f->v2 );	v3 = *GetVertPos( f->v3 );			
			norm = norm.Cross ( v2-v1, v3-v1 );
			norm.NormalizeFast ();
			vn = GetVertNorm( f->v1 );	*vn = norm;
			vn = GetVertNorm( f->v2 );	*vn = norm;
			vn = GetVertNorm( f->v3 );	*vn = norm;
		}
	} else {
		// Smoothed normals

		int n=0;		
		Vec3F* vertbuf  = (Vec3F*) GetBufData(BVERTPOS);	// efficiency, stride not needed
		Vec3F* vnormbuf = (Vec3F*) GetBufData(BVERTNORM);	// efficiency, stride not needed

		// overall slow function due to scattered reads & writes 
		//--- faster code
		/*for (f = (AttrV3*) GetStart(BFACEV3); f <= (AttrV3*) GetEnd(BFACEV3); f++ ) {
			v1 = *(vertbuf + f->v1);			// was GetVertPos, also same as GetElemFast
			v2 = *(vertbuf + f->v2);
			v3 = *(vertbuf + f->v3);
			if (v1.x==0 && v1.y==0 && v1.z==0 && v2.x==0 && v2.y==0 && v2.z==0) continue;		// degenerate faces
			norm = (v2-v1).Cross ( v3-v1 );
			norm.NormalizeFast ();				// fast inverse square root
			*(vnormbuf + f->v1) += norm;		// was GetVertNorm
			*(vnormbuf + f->v2) += norm;
			*(vnormbuf + f->v3) += norm;
		}*/
		
		//--- slower code
		for (f = (AttrV3*)GetStart(BFACEV3); f <= (AttrV3*)GetEnd(BFACEV3); f++) {
			v1 = *GetVertPos( f->v1 );	v2 = *GetVertPos( f->v2 );	v3 = *GetVertPos( f->v3 );
			if (v1.x==0 && v1.y==0 && v1.z==0 && v2.x==0 && v2.y==0 && v2.z==0) continue;		// degenerate faces
			norm = (v2 - v1).Cross(v3 - v1);
			norm.Normalize();
			vn = GetVertNorm ( f->v1 );		*vn += norm;
			vn = GetVertNorm ( f->v2 );		*vn += norm;
			vn = GetVertNorm ( f->v3 );		*vn += norm;
		}

		// Normalize vertex normals
		Vec3F vec;
		for (vn = (Vec3F*) GetStart(BVERTNORM); vn <= (Vec3F*) GetEnd(BVERTNORM); vn++) 
			vn->Normalize ();
	}	
}

void MeshX::FlipNormals ()
{
	Vec3F* vn;	
	for (vn = (Vec3F*) GetStart(BVERTNORM); vn <= (Vec3F*) GetEnd(BVERTNORM); vn++) 
		*vn *= -1.0f;
}

void MeshX::ComputeBounds (Vec3F& bmin, Vec3F& bmax, int vmin, int vmax)
{
	if (vmax == 0) vmax = GetNumVert() - 1;

	Vec3F* v1 = GetVertPos ( vmin );
	bmin = *v1; bmax = *v1;
	
	for (int n=vmin+1; n <= vmax; n++) {
		v1 = GetVertPos ( n );
		if ( v1->x < bmin.x ) bmin.x = v1->x;
		if ( v1->y < bmin.y ) bmin.y = v1->y;
		if ( v1->z < bmin.z ) bmin.z = v1->z;
		if ( v1->x > bmax.x ) bmax.x = v1->x;
		if ( v1->y > bmax.y ) bmax.y = v1->y;
		if ( v1->z > bmax.z ) bmax.z = v1->z;		
	}
}

Vec3F MeshX::NormalizeMesh ( float sz, Vec3F& ctr, int vmin, int vmax )
{
	if (vmax == 0) vmax = GetNumVert() - 1;

	// Get bounding box
	Vec3F bmin, bmax;
	ComputeBounds ( bmin, bmax, vmin, vmax );
	bmin *= sz;	bmax *= sz;							// sz = resize object

	// Retrieve pivot	
	ctr = (bmin + bmax) * Vec3F(0.5f,0.5f,0.5f);
	
	// Compute new vertex positions		
	Vec3F* v = GetVertPos(vmin);
	for (int n=vmin; n <= vmax; n++) {
																// v = range [bmin,bmax] - input mesh 
		*v = ((*v)*sz - bmin) / (bmax-bmin);					// v'= range [0, 1] - normalize mesh
		*v = *v * Vec3F(2,2,2) + Vec3F(-1,-1,-1);		// o = range [-1,1] - output range, all shapes have this range
		v++;
	}
	return (bmax - bmin)*0.5f;
}

// Raytrace mesh
// Returns:
//  vndx  - index of the hit face and nearest vertex
//  vhit  - position of hit in face
//  vnear - position of nearest vertex
//
bool MeshX::Raytrace ( Vec3F orig, Vec3F dir, Matrix4F& xform, Vec3I& vndx, Vec3F& vnear, Vec3F& vhit, Vec3F& vnorm )
{
	AttrV3* f;	
	int fbest;
	float t, tbest, d, dbest;
	Vec3F v[3], hit;
	float alpha, beta;
	bool front;
	
	// find nearest hit triangle	
	fbest = -1;
	t = 1.0e10;
	tbest = 1.0e10;
	f = (AttrV3*) GetStart(BFACEV3);
	for ( int fi=0; fi < GetNumElem(BFACEV3); fi++) {

		v[0] = *GetVertPos( f->v1 );	v[0] *= xform;							// mesh transform
		v[1] = *GetVertPos( f->v2 );	v[1] *= xform;
		v[2] = *GetVertPos( f->v3 );	v[2] *= xform;	

		if ( intersectRayTriangle ( orig, dir, v[0], v[1], v[2], t, alpha, beta, front ) ) {	// check for triangle hit
			if ( t < tbest ) {													// find nearest hit
				fbest = fi;
				tbest = t;
			}			
		}
		f++;
	}
	if ( fbest==-1 ) return false;		// no hit

	// find nearest vertex in triangle
	dbest = 1.0e20;
	f = (AttrV3*) GetElem(BFACEV3, fbest );
	v[0] = *GetVertPos( f->v1 );	v[0] *= xform;
	v[1] = *GetVertPos( f->v2 );	v[1] *= xform;
	v[2] = *GetVertPos( f->v3 );	v[2] *= xform;	
	
	for (int i=0; i < 3; i++ ) {
		d = sqrt( (v[i].x-hit.x)*(v[i].x-hit.x) + (v[i].y-hit.y)*(v[i].y-hit.y) + (v[i].z-hit.z)*(v[i].z-hit.z) );		// distance from hit to each vertex
		if ( d < dbest ) {													// find nearest vertex
			vndx.x = fbest;													// face id
			vndx.y = (i==0) ? f->v1 : ((i==1) ? f->v2 : f->v3);				// vertex id					
			vndx.z = i;
			dbest = d;
		}
	}
	vhit = orig + dir * tbest;							// return best hit
	vnear = v[ vndx.z ];								// and nearest vertex
	
	xform.SetTranslate( Vec3F(0, 0, 0) );						// remove translation from transform
	vnorm = *GetVertNorm(vndx.y);	vnorm *= xform;		// get oriented normal
	return true;
}

void MeshX::Smooth ( int iter )
{
	Vec3F norm, side;	
	AttrV3 *f;
	Vec3F *v1, *v2, *v3;
	hList* flist;		// neighbor face list
	Vec3F *face_pos;
	face_pos = (Vec3F*) malloc ( sizeof(Vec3F) * GetNumFace3() );

	if ( !isActive(BVERTFLIST) ) {
		printf ( "ERROR: Mesh:Smooth. Mesh format does not support smoothing. Use FVF or CM.\n" );
		return;
	}

	int n;
	Vec3F vec;
	
	for (int j=0; j < iter; j++) {
		// Compute centroid of all faces	
		n = 0;
		for (f = (AttrV3*) GetStart(BFACEV3); f <= (AttrV3*) GetEnd(BFACEV3); f++) {
			v1 = GetVertPos ( f->v1 ); v2 = GetVertPos ( f->v2 ); v3 = GetVertPos ( f->v3 );
			face_pos[n].x = ( v1->x + v2->x + v3->x ) / 3.0;
			face_pos[n].y = ( v1->y + v2->y + v3->y ) / 3.0;
			face_pos[n].z = ( v1->z + v2->z + v3->z ) / 3.0;
			n++;
		}
		// Compute new vertex positions		
		for (n=0; n < GetNumVert(); n++) {
			v1 = GetVertPos ( n );
			flist = GetVertFList ( n );
			vec.Set (0,0,0);
			hval* fptr = GetHeap() + flist->pos;
			for (int j=0; j < flist->cnt; j++) {	// loop over neighboring faces of vertex
				vec.x += face_pos[ (*fptr) ].x;
				vec.y += face_pos[ (*fptr) ].y;
				vec.z += face_pos[ (*fptr) ].z;
				fptr++;
			}
			v1->x = vec.x / (float) flist->cnt;		// average position of neighboring faces
			v1->y = vec.y / (float) flist->cnt;
			v1->z = vec.z / (float) flist->cnt;
		}
	}
	free ( face_pos );
	
	ComputeNormals ();	//	-- No normal smoothing = preserves visual detail (interesting)
}


void MeshX::AppendMesh ( MeshX* src, int maxf, int maxv )
{
	// assumes FV mesh!

	//isActive(BVERTNORM)

	int src_numv = src->GetNumVert();
	int src_numf = src->GetNumFace3();
	int dest_numv = GetNumElem ( BVERTNORM );
	int dest_numf = GetNumElem ( BFACEV3 );

	if ( maxv==0 ) maxv = dest_numv + src_numv;
	if ( maxf==0 ) maxf = dest_numf + src_numf;

	// confirm same active buffers
	//  (note: we could check src->isActive, but generate these regardless because of lack of flexible array binding in RenderGL)
	if ( !isActive(BVERTNORM))	AddBuffer ( BVERTNORM, "norm", sizeof(Vec3F), 0 );
	if ( !isActive(BVERTTEX))   AddBuffer ( BVERTTEX, "tex", sizeof(Vec2F), 0 );
	if ( !isActive(BVERTCLR))	AddBuffer ( BVERTCLR, "clr", sizeof(uint), 0 );

	// expand buffers to hold src	
	ExpandBuffer ( BFACEV3,		maxf );	
	ExpandBuffer ( BVERTPOS,	maxv );	
	ExpandBuffer ( BVERTNORM,	maxv );
	ExpandBuffer ( BVERTTEX,	maxv );
	ExpandBuffer ( BVERTCLR,	maxv );		
	
	// copy vertices
	Vec3F* src_vpos =	src->GetElemVec3 ( BVERTPOS,	0 );
	Vec3F* src_vnorm =	src->GetElemVec3 ( BVERTNORM,	0 );
	Vec2F* src_vtex =	src->GetElemVec2 ( BVERTTEX,	0 );	
	uint*	   src_vclr =	(uint*) src->GetElem (BVERTCLR,	0 );
	Vec3F* dest_vpos =	GetElemVec3 ( BVERTPOS,		dest_numv );
	Vec3F* dest_vnorm =	GetElemVec3 ( BVERTNORM,	dest_numv );
	Vec2F* dest_vtex =	GetElemVec2 ( BVERTTEX,		dest_numv );
	uint*	   dest_vclr =	(uint*) GetElem ( BVERTCLR,	dest_numv );
	memcpy ( dest_vpos,		src_vpos,	sizeof(Vec3F) * src_numv );
	if (src->isActive(BVERTNORM)) 		memcpy ( dest_vnorm,	src_vnorm,	sizeof(Vec3F) * src_numv );	
	if (src->isActive(BVERTTEX)) 		memcpy ( dest_vtex,		src_vtex,	sizeof(Vec2F) * src_numv );
	if (src->isActive(BVERTCLR))
		memcpy ( dest_vclr,		src_vclr,	sizeof(uint) * src_numv );
	else
		memset ( dest_vclr,		0xFF, 		sizeof(uint) * src_numv );

	// copy faces (with indexing adjustment)
	AttrV3* src_f =		(AttrV3*) src->GetElem( BFACEV3, 0 );
	AttrV3* dest_f =	(AttrV3*) GetElem( BFACEV3, dest_numf );
	AttrV3 f;
	for (int i=0; i < src_numf; i++) {
		f = *src_f++;
		f.v1 += dest_numv;
		f.v2 += dest_numv;
		f.v3 += dest_numv;
		*dest_f++ = f;		
	}

	// set usage to max count
	ReserveBuffer ( BFACEV3,		dest_numf + src_numf );
	ReserveBuffer ( BVERTPOS,		dest_numv + src_numv );
	ReserveBuffer ( BVERTNORM,	dest_numv + src_numv );
	ReserveBuffer ( BVERTTEX,		dest_numv + src_numv );
	ReserveBuffer ( BVERTCLR,		dest_numv + src_numv );	
}



//------------------------------------------------------------------ FV - Face-Vertex Mesh
void MeshX::CreateFV () 
{	
	m_Format = MF_FV;
	m_Ures = 0;
	m_Vres = 0;

	AddBuffer ( BVERTPOS, "pos", sizeof(Vec3F), 0 );
	AddBuffer ( BFACEV3, "v3", sizeof(AttrV3), 0 );

	SetFormatFunc();
}

void MeshX::SetFuncFV ()
{
	m_AddVertFunc = &MeshX::AddVertFV;
	m_AddFaceFast3Func = &MeshX::AddFaceFast3FV;
	m_AddFaceFast4Func = &MeshX::AddFaceFast4FV;
}
xref MeshX::AddFaceFast3FV ( xref v1, xref v2, xref v3 )
{
	xref n = AddElem ( BFACEV3 );
	SetFace3 ( n, v1, v2, v3 );
	return n;
}
xref MeshX::AddFaceFast4FV ( xref v1, xref v2, xref v3, xref v4 )
{
	xref n = AddElem ( BFACEV4 );			// !!!! -------- NOT CORRECT
	SetFace4 ( n, v1, v2, v3, v4 );
	return n;
}
xref MeshX::AddVertFV ( float x, float y, float z )
{
	xref n = AddElem ( BVERTPOS );
	SetVertPos ( n, Vec3F(x,y,z) );
	return n;
}
xref MeshX::AddVertClr(Vec4F vec)
{
	if (!isActive(BVERTCLR)) AddBuffer(BVERTCLR, "clr", sizeof( CLRVAL ), 1);   // CLRVAL = uint = uint32_t
	xref n = AddElem(BVERTCLR);
	SetVertClr(n, COLORA(vec.x, vec.y, vec.z, vec.w) );
	return n;
}
xref MeshX::AddVertNorm ( Vec3F vec )
{
	if ( !isActive(BVERTNORM) ) AddBuffer ( BVERTNORM, "norm", sizeof(Vec3F), 1 );
	xref n = AddElem ( BVERTNORM );
	SetVertNorm ( n, vec );
	return n;
}
xref MeshX::AddVertTex ( Vec2F vec )
{
	if ( !isActive(BVERTTEX) ) AddBuffer ( BVERTTEX, "tex", sizeof(Vec2F), 1 );
	xref n = AddElem ( BVERTTEX );
	SetVertTex ( n, vec.x, vec.y );
	return n;
}
xref MeshX::AddMtlGroup ()
{
	if ( !isActive(BMTL) ) AddBuffer ( BMTL, "mtl", sizeof(AttrV3), 1 );
	xref n = AddElem ( BMTL );
	return n;	
}
xref MeshX::SetMtlGroup (int n, xref fa, xref fb)
{
	// pack face range (fa,fb) of the 
	// material group into an attrv3 struct
	AttrV3 f; 
	f.v1 = fa; 
	f.v2 = fb; 
	f.v3 = fb-fa+1;
	SetElem ( BMTL, n, &f); 
	return n;
}

void MeshX::DebugFV ()
{
	int n;
	Vec3F* v;
	AttrV3* f;
	printf ( "-- FV MESH --\n");	
	printf ( "-- verts\n" );
	for (n=0; n < GetNumVert(); n++) {
		v = GetVertPos ( n );
		printf ( "%d: (%2.1f,%2.1f,%2.1f)\n", n, v->x, v->y, v->z);		
	}
	printf ( "-- faces\n" );	
	for (n=0; n < GetNumFace3(); n++) {
		f = GetFace3 ( n );
		printf ( "%d: v:%d %d %d\n", n, f->v1, f->v2, f->v3);
	}	
}

/*void MeshX::UVSphere ()
{
	bufPos p = FindBuffer ( "vertex" );
	AddVertAttr ( "tex", sizeof(Vec2F) );
	
	refresh_data ();			// must be done because adding texture coords may change buffer locations in memory

	Vec2F* tex;
	Vec3F v;	
	double r;
	for (int n=0; n < GetNumVert(); n++) {
		tex = GetVertTex(n);
		v = *GetVertPos(n);		
		//tex->x = modf ( (double) (v.z/10.0f), &r );
		//tex->y = modf ( (double) (v.y/10.0f), &r );
		v.Normalize ();
		tex->x = atan2 ( (float) v.z, (float) v.x )/(2.0*3.141592);
		r = sqrt ( v.z*v.z + v.x*v.x );
		tex->y = atan2 ( (float) v.y, (float) r )/(2.0*3.141592);
	}	
}*/


//------------------------------------------------------------------ FVF - Face-vertex-face Mesh
void MeshX::CreateFVF () 
{	
	m_Format = MF_FVF;
	m_Ures = 0;
	m_Vres = 0;

	// vertices
	AddBuffer ( BVERTPOS, "pos", sizeof(Vec3F), 0 );		
	AddBuffer ( BVERTCLR, "clr", sizeof(CLRVAL), 0 );
	AddBuffer ( BVERTNORM, "norm", sizeof(Vec3F), 0 );
//	AddBuffer ( BVERTEX, "tex", sizeof(Vec2F), 0  );
	AddBuffer ( BVERTFLIST, "flist", sizeof(hList), 0 );		// vertex -> N faces
	
	// faces
	AddBuffer ( BFACEV3, "v3", sizeof(AttrV3), 0 );			// face -> 3x vertices

	AddHeap ( 64 );

	SetFormatFunc();
}

void MeshX::ClearFVF ()
{
	EmptyAllBuffers();
	ResetHeap ();
}
void MeshX::SetFuncFVF ()
{
	m_AddVertFunc = &MeshX::AddVertFVF;
	m_AddFaceFast3Func = &MeshX::AddFaceFast3FVF;
	m_AddFaceFast4Func = &MeshX::AddFaceFast4FVF;
}
xref MeshX::AddFaceFast3FVF ( xref v1, xref v2, xref v3 )
{
	int n = AddElem ( BFACEV3 );
	SetFace3 ( n, v1, v2, v3 );
	AddRef ( n, GetVertFList(v1), FACE_DELTA );	
	AddRef ( n, GetVertFList(v2), FACE_DELTA );
	AddRef ( n, GetVertFList(v3), FACE_DELTA );
	return n;
}
xref MeshX::AddFaceFast4FVF ( xref v1, xref v2, xref v3, xref v4 )
{
	int n = AddElem ( BFACEV4 );
	SetFace4 ( n, v1, v2, v3, v4 );
	AddRef ( n, GetVertFList(v1), FACE_DELTA );	
	AddRef ( n, GetVertFList(v2), FACE_DELTA );
	AddRef ( n, GetVertFList(v3), FACE_DELTA );
	AddRef ( n, GetVertFList(v4), FACE_DELTA );
	return n;
}
xref MeshX::AddVertFVF ( float x, float y, float z )
{
	int n = AddElem ( BVERTPOS );
	SetVertPos ( n, Vec3F(x,y,z) );
	ClearRefs ( GetVertFList(n) );
	return n;
}
void MeshX::DebugFVF ()
{
	int n;
	int j;	
	Vec3F* v;
	AttrV3* f;
	hList* flist;
	printf ( "-- FVF MESH --\n");	
	printf ( "-- verts\n" );
	for (n=0; n < GetNumVert(); n++) {
		v = GetVertPos ( n );
		flist = GetVertFList ( n );		
		printf ( "%d: (%2.1f,%2.1f,%2.1f) f:%d {", n, v->x, v->y, v->z, flist->pos );
		if ( flist->cnt > 0 ) {
			for (j=0; j < flist->cnt; j++) 
				printf ( "%d ", *(GetHeap() + flist->pos+j) - FACE_DELTA );
		}
		printf ( "}\n" );
	}
	printf ( "-- faces\n" );	
	for (n=0; n < GetNumFace3(); n++) {
		f = GetFace3 ( n );
		printf ( "%d: v:%d %d %d\n", n, f->v1, f->v2, f->v3);
	}
	DebugHeap ();		
}

//------------------------------------------------------------------ CM - Connected Mesh
// Create Connected Mesh (CM)

void MeshX::CreateCM () 
{	
	m_Format = MF_CM;
	m_Ures = 0;
	m_Vres = 0;

	// vertices
	AddBuffer ( BVERTPOS, "pos", sizeof(Vec3F), 0 );		
	AddBuffer ( BVERTCLR, "clr", sizeof(CLRVAL), 0 );
	AddBuffer ( BVERTNORM, "norm", sizeof(Vec3F), 0 );
	//AddBuffer ( "tex", sizeof(Vec2F), 0  );
	AddBuffer ( BVERTFLIST, "flist", sizeof(hList), 0 );		// vertex -> # faces
	AddBuffer ( BVERTELIST, "elist", sizeof(hList), 0 );		// vertex -> # edges
	
	// faces 
	AddBuffer ( BFACEV3, "v3", sizeof(AttrV3), 1 );			// face -> 3x vertices
	AddBuffer ( BFACEE3, "e3", sizeof(AttrE3), 1 );			// face -> 3x edges
	
	// edges
	AddBuffer ( BEDGES, "edge", sizeof(AttrEdge), 0 );		// edge -> vertices & faces

	SetFormatFunc();
}

void MeshX::ClearCM ()
{
	EmptyAllBuffers ();
	ResetHeap ();
}

void MeshX::SetFuncCM ()
{
	m_AddVertFunc = &MeshX::AddVertCM;
	m_AddFaceFast3Func = &MeshX::AddFaceFast3CM;
	m_AddFaceFast4Func = &MeshX::AddFaceFast4CM;
}
xref MeshX::AddFaceFast3CM ( xref v1, xref v2, xref v3 )
{
	int n = AddElem ( BFACEV3 );	
	AddElem ( BFACEE3 );

	AttrV3* f = (AttrV3*) GetElem ( BFACEV3, n );
	AttrE3* fe = (AttrE3*) GetElem ( BFACEE3, n );
	f->v1 = v1;	f->v2 = v2;	f->v3 = v3;	
	xref eNdx;
	eNdx = AddEdgeCM ( f->v1, f->v2, n );	fe->e1 = eNdx;
	eNdx = AddEdgeCM ( f->v2, f->v3, n );	fe->e2 = eNdx;
	eNdx = AddEdgeCM ( f->v3, f->v1, n );	fe->e3 = eNdx;
	AddRef ( n, GetVertFList(v1), FACE_DELTA );	
	AddRef ( n, GetVertFList(v2), FACE_DELTA );
	AddRef ( n, GetVertFList(v3), FACE_DELTA );
	return n;
}
xref MeshX::AddFaceFast4CM ( xref v1, xref v2, xref v3, xref v4 )
{
	int n = AddElem ( BFACEV4 );	
	AddElem ( BFACEE4 );	

	AttrV4* f = (AttrV4*) GetElem ( BFACEV4, n );
	AttrE4* fe = (AttrE4*) GetElem ( BFACEE4, n );
	f->v1 = v1;	f->v2 = v2;	f->v3 = v3;	f->v4 = v4;			// faces point to verts
	xref eNdx;
	eNdx = AddEdgeCM ( f->v1, f->v2, n );	fe->e1 = eNdx;		// edges to verts, faces to edges
	eNdx = AddEdgeCM ( f->v2, f->v3, n );	fe->e2 = eNdx;
	eNdx = AddEdgeCM ( f->v3, f->v4, n );	fe->e3 = eNdx;
	eNdx = AddEdgeCM ( f->v4, f->v1, n );	fe->e4 = eNdx;
	AddRef ( n, GetVertFList(v1), FACE_DELTA );		// verts to faces
	AddRef ( n, GetVertFList(v2), FACE_DELTA );
	AddRef ( n, GetVertFList(v3), FACE_DELTA );
	AddRef ( n, GetVertFList(v4), FACE_DELTA );
	return n;
}
xref MeshX::AddVertCM ( float x, float y, float z )
{
	int n = AddElem ( BVERTPOS );
	SetVertPos ( n, Vec3F(x,y,z) );
	ClearRefs ( GetVertFList(n) );
	ClearRefs ( GetVertEList(n) );
	return n;
}
xref MeshX::FindEdgeCM ( xref v1, xref v2 )
{	
	AttrEdge* pE;
	int n = 0;
	for (pE = (AttrEdge*) GetStart ( BEDGES ); pE <= (AttrEdge*) GetEnd ( BEDGES ); pE++) {
		if ( pE->v1 == v2 || pE->v2 == v2 ) return n;
		n++;
	}
	return -1;
}

xref MeshX::AddEdgeCM ( xref v1, xref v2, xref face )
{	
	xref eNdx = FindEdgeCM ( v1, v2 );	
	if ( eNdx == -1 ) {
		int n = AddElem ( BEDGES );
		SetEdge ( n, face, -1, v1, v2 );
		AddRef ( eNdx, GetVertEList(v1), EDGE_DELTA );
		AddRef ( eNdx, GetVertEList(v2), EDGE_DELTA );		
	} else {
		((AttrEdge*) GetElem ( BEDGES, eNdx ))->f2 = face;
	}
	return eNdx;
}

void MeshX::DebugCM ()
{
	int n;
	int j;	
	Vec3F* v; 
	hList *elist, *flist;
	AttrEdge* e; 
	AttrV3* f;
	AttrE3* fe;
	printf ( "-- MESH --\n");
	
	printf ( "-- verts\n" );
	for (n=0; n < GetNumVert(); n++) {
		v = GetVertPos ( n );
		elist = GetVertEList ( n );
		flist = GetVertFList ( n );
		printf ( "%d: (%2.1f,%2.1f,%2.1f) e:%d {", n, v->x, v->y, v->z, elist->pos );
		if ( elist->cnt > 0 ) { 
			for (j=0; j < elist->cnt; j++) 
				printf ( "%d ", *(GetHeap()+ elist->pos+j) - EDGE_DELTA );
		}
		printf ( "}, f:%d {", flist->pos );
		if ( flist->cnt > 0 ) {
			for (j=0; j < flist->cnt; j++) 
				printf ( "%d ", *(GetHeap()+ flist->pos+j) - FACE_DELTA );
		}
		printf ( "}\n" );	
	}

	printf ( "-- edges\n" );	
	for (n=0; n < GetNumEdge(); n++) {
		e = GetEdge (n);
		printf ( "%d: v:%d %d, f:%d %d\n", n, e->v1, e->v2, e->f1, e->f2 );		
	}

	printf ( "-- faces\n" );	
	for (n=0; n < GetNumFace3(); n++) {
		f = GetFace3 ( n );
		fe = GetFace3Edge ( n );
		printf ( "%d: v:%d %d %d, e:%d %d %d\n", n, f->v1, f->v2, f->v3, fe->e1, fe->e2, fe->e3 );		
	}
	DebugHeap ();	
	//_getch();
}

void MeshX::DebugHeap ()
{
	int hc = 0;
	hval* pVal = GetHeap();
//	debug.Printf ( "-- heap (size: %d, max: %d, free: %04d)\n", mHeapNum, mHeapMax, mHeapFree );
/*	for (int n=0; n < mHeapNum; n++) {
		if ( (n % 8) == 0 ) debug.Printf ( "\n[%04d] ", n );
		#ifdef MESH_DEBUG
			hc--;
			if ( *pVal == 0 ) {
				debug.Printf ( "..... ");								// allocated, but unused data
			} else if ( (*pVal == (hval) 0xFFFF) || hc ==1 || hc == 3) {
				debug.Printf ( "----- ");								// heap free space
			} else if ( *pVal >= VERT_DELTA && *pVal < EDGE_DELTA ) {
				debug.Printf ( "v%04d ", *pVal - VERT_DELTA );			// vertex ref
			} else if ( *pVal >= EDGE_DELTA && *pVal < FACE_DELTA ) {
				debug.Printf ( "e%04d ", *pVal - EDGE_DELTA );			// edge ref
			} else if ( *pVal >= FACE_DELTA ) {
				debug.Printf ( "f%04d ", *pVal - FACE_DELTA );			// face ref
			} else {
				if ( hc == 2 ) {
					debug.Printf ( "n%04d ", (int) *pVal );				// heap free: next pointer.
				} else {
					debug.Printf ( "H%04d ", (int) *pVal );				// heap free: count.
					hc = 4;
				}
			}			
		#else
			debug.Printf ( "%05d ", (int) *pVal );
		#endif
		pVal++;
	}*/
}

void MeshX::Measure ()
{	
	hval* pHeap = GetHeap ();
	hval* pCurr = GetHeap() + GetHeapFree();	
	int vs, es, fs, hs, hm, as, frees = 0;
	vs = GetNumVert(); if ( vs != 0 ) vs *= GetBufStride( BVERTPOS) * GetNumElem(BVERTPOS);
	es = GetNumEdge(); if ( es != 0 ) es *= GetBufStride( BEDGES ) * GetNumElem(BEDGES);
	fs = GetNumFace3(); if ( fs != 0 ) fs *= GetBufStride( BFACEV3 ) * GetNumElem(BFACEV3);
	hs = GetHeapNum() * sizeof(hval);
	hm = GetHeapMax() * sizeof(hval);
	
	if ( pHeap != 0x0 ) {
		while ( pCurr != pHeap-1 ) {
			frees += *(pCurr);
			pCurr = pHeap + * (hpos*) (pCurr + HEAP_POS);
		}
		frees *= sizeof(hval);
	} else {
		frees = 0;
		hm = 1;
	}
	as = 0;
	as += GetMaxElem(BVERTPOS) * GetBufStride ( BVERTPOS );
	as += GetMaxElem(BFACEV3) * GetBufStride ( BFACEV3 );
	as += GetMaxElem(BEDGES) * GetBufStride ( BEDGES );
	as += hm;

	printf ( "NumVert:     %07.1fk (%d)\n", vs/1024.0, GetNumVert() );
	printf ( "NumFace:     %07.1fk (%d)\n", fs/1024.0, GetNumFace3() );
	printf ( "NumEdge:     %07.1fk (%d)\n", es/1024.0, GetNumEdge() );
	printf ( "Heap Size:   %07.1fk (%d)\n", hs/1024.0, GetHeapNum() );
	printf ( "Free Size:   %07.1fk\n", frees/1024.0 );
	printf ( "Heap Used:   %07.1fk (%5.1f%%)\n", (hs-frees)/1024.0, (hs-frees)*100.0/(vs+es+fs+hs-frees) );
	printf ( "Heap Max:    %07.1fk\n", hm/1024.0 );	
	printf ( "Total Used:  %07.1fk\n", (vs+es+fs+hs-frees)/1024.0 );
	printf ( "Total Alloc: %07.1fk\n", as/1024.0 );
	printf ( "Fragmentation: %f%%\n", (hm-(hs-frees))*100.0 / hm );
}

/*int MeshX::GetIndex ( int b, void* v )
{
	if ( v == 0x0 ) return -1;
	return ((char*) v - (char*) mBuf[b].data) / mBuf[b].stride;
}*/

int MeshX::FindPlyElem ( char typ )
{
	for (int n=0; n < (int) m_Ply.size(); n++) {
		if ( m_Ply[n]->type == typ ) return n;
	}
	return -1;
}

int MeshX::FindPlyProp ( int elem, std::string name )
{
	for (int n=0; n < (int) m_Ply[elem]->prop_list.size(); n++) {
		if ( m_Ply[elem]->prop_list[n].name.compare ( name)==0 )
			return n;
	}
	return -1;
}

bool MeshX::LoadPly ( const char* fname, float scal )
{
	FILE* fp;

	int m_PlyCnt;
	float m_PlyData[40];
	char buf[1000];
	char bword[1000];
	std::string word;	
	int vnum, fnum, elem, cnt;
	char typ;

	if ( m_Format == MF_UNDEF ) {
		DeleteAllBuffers ();
		CreateFV ();
	}

	char* chp;

	Clear ();

	fp = fopen ( fname, "rt" );
	if ( fp == 0x0 ) { printf (  "Could not find file: %s\n", fname ); }	
	
	// Read header	
	chp = fgets ( buf, 1000, fp );
	readword ( buf, ' ', bword, 1000 ); word = bword;
	if ( word.compare("ply" )!=0 ) {
		printf ( "Not a ply file. %s\n", fname );
	}
	
	m_Ply.clear ();

	printf ( "Reading PLY.\n" ); 
	while ( feof( fp ) == 0 ) {
		chp = fgets ( buf, 1000, fp );
		readword ( buf, ' ', bword, 1000 );
		word = bword;
		if ( word.compare("comment" )!=0 ) {
			if ( word.compare("end_header")==0 ) break;
			if ( word.compare("property")==0 ) {
				readword ( buf, ' ', bword, 1000 );
				word = bword;
				if ( word.compare("float")==0 ) typ = PLY_FLOAT;
				if ( word.compare("float16")==0 ) typ = PLY_FLOAT;
				if ( word.compare("float32")==0 ) typ = PLY_FLOAT;
				if ( word.compare("int8")==0 ) typ = PLY_INT;
				if ( word.compare("uint8")==0 ) typ = PLY_UINT;
				if ( word.compare("list")==0) {
					typ = PLY_LIST;
					readword ( buf, ' ' , bword, 1000 );
					readword ( buf, ' ', bword, 1000 );
				}
				readword ( buf, ' ', bword, 1000 );
				word = bword;
				AddPlyProperty ( typ, word );
			}
			if ( word.compare("element" )==0 ) {
				readword ( buf, ' ', bword, 1000 );	word = bword;
				if ( word.compare("vertex")==0 ) {
					readword ( buf, ' ', bword, 1000 );
					vnum = atoi ( bword );
					printf ( "  Verts: %d\n", vnum );
					AddPlyElement ( PLY_VERTS, vnum );
				}
				if ( word.compare("face")==0 ) {
					readword ( buf, ' ', bword, 1000 );
					fnum = atoi ( bword );
					printf ( "  Faces: %d\n", fnum );
					AddPlyElement ( PLY_FACES, fnum );
				}
			}
		}		
	}

	// Read data
	int xi, yi, zi, ui, vi;
	printf ( "  Reading verts..\n" );	
	elem = FindPlyElem ( PLY_VERTS );
	xi = FindPlyProp ( elem, "x" );
	yi = FindPlyProp ( elem, "y" );
	zi = FindPlyProp ( elem, "z" );
	ui = FindPlyProp ( elem, "s" );
	vi = FindPlyProp ( elem, "t" );
	if ( elem == -1 || xi == -1 || yi == -1 || zi == -1 ) {
		printf (  "ERROR: Vertex data not found.\n" );
	}
	if ( ui != -1 && vi != -1 ) {
//		AddBuffer ( BVERTTEX, "tex", sizeof(Vec2F), 0 );
	}
	
	xref vert;
	for (int n=0; n < m_Ply[elem]->num; n++) {
		chp = fgets ( buf, 1000, fp );
		for (int j=0; j < (int) m_Ply[elem]->prop_list.size(); j++) {
			readword ( buf, ' ', bword, 1000 );
			m_PlyData[ j ] = atof ( bword );
		}		
		vert = AddVert ( m_PlyData[xi]*scal, m_PlyData[yi]*scal, m_PlyData[zi]*scal );
		if ( ui != -1 && vi != -1 && isActive(BVERTTEX) ) {
			//SetVertTex ( vert, m_PlyData[ui], m_PlyData[vi] );	
		}
	}

	printf ( "  Reading faces..\n" );
	elem = FindPlyElem ( PLY_FACES );
	xi = FindPlyProp ( elem, "vertex_indices" );
	if ( elem == -1 || xi == -1 ) {
		printf (  "Face data not found.\n" );
	}
	for (int n=0; n < m_Ply[elem]->num; n++) {
		chp = fgets ( buf, 1000, fp );
		m_PlyCnt = 0;
		for (int j=0; j < (int) m_Ply[elem]->prop_list.size(); j++) {
			if ( m_Ply[elem]->prop_list[j].type == PLY_LIST ) {
				readword ( buf, ' ', bword, 1000 );
				cnt = atoi ( bword );	
				m_PlyData[ m_PlyCnt++ ] = cnt;
				for (int c =0; c < cnt; c++) {
					readword ( buf, ' ', bword, 1000 );
					m_PlyData[ m_PlyCnt++ ] = atof ( bword );
				}
			} else {
				readword ( buf, ' ', bword, 1000 );
				m_PlyData[ m_PlyCnt++ ] = atof ( bword );
			}
		}
		if ( m_PlyData[xi] == 3 ) {
			//printf ( 'objs', INFO, "    Face: %d, %d, %d\n", (int) m_PlyData[xi+1], (int) m_PlyData[xi+2], (int) m_PlyData[xi+3] );
			AddFaceFast ( (int) m_PlyData[xi+1], (int) m_PlyData[xi+2], (int) m_PlyData[xi+3] );
		}
		
		if ( m_PlyData[xi] == 4 ) {
			//printf ( 'objs', INFO,  "    Face: %d, %d, %d, %d\n", (int) m_PlyData[xi+1], (int) m_PlyData[xi+2], (int) m_PlyData[xi+3], (int) m_PlyData[xi+4]);
			AddFaceFast ( (int) m_PlyData[xi+1], (int) m_PlyData[xi+2], (int) m_PlyData[xi+3], (int) m_PlyData[xi+4] );
		}
	}
	for (int n=0; n < (int) m_Ply.size(); n++) {
		delete ( m_Ply[n] );
	}
	m_Ply.clear ();
	m_PlyCurrElem = 0;

	Measure ();
	ComputeNormals ();

	return true;
}

void MeshX::AddPlyElement ( char typ, int n )
{
	printf ( "  Element: %d, %d\n", typ, n );
	SHPlyElement* p = new SHPlyElement;
	if ( p == 0x0 ) { printf ( "Unable to allocate PLY element.\n" ); }
	p->num = n;
	p->type = typ;
	p->prop_list.clear ();
	m_PlyCurrElem = (int) m_Ply.size();
	m_Ply.push_back ( p );
}

void MeshX::AddPlyProperty ( char typ, std::string name )
{
	printf ( "  Property: %d, %s\n", typ, name.c_str() );
	SHPlyProperty p;
	p.name = name;
	p.type = typ;
	m_Ply [ m_PlyCurrElem ]->prop_list.push_back ( p );
}

//----- define tuple to use as a unique key for vertex, norm and tex IDs
//
struct triple_t {
	triple_t(int a0, int b0, int c0 ) {a=a0; b=b0; c=c0;}
	xref a;
	xref b;
	xref c;

	bool operator==(const triple_t& other) const
	{ return (a==other.a && b==other.b && c==other.c); }

	bool operator< (const triple_t& other) const
	{ return (a < other.a && b < other.b && c < other.c); }
};

//-- required hash func for custom keys
namespace std {
	template<>
	struct hash<triple_t>
    {
		std::size_t operator()(const triple_t& k) const
		{
		  using std::size_t;
		  using std::hash;		  

		  // Compute individual hash values for first, second and third and combine them using XOR and bit shifting:
		  return (   (hash<xref>()(k.a)
				   ^ (hash<xref>()(k.b) << 1)) >> 1)
				   ^ (hash<xref>()(k.c) << 1);
		}
    };
}

// unordered map is faster since we only need uniqueness, ordering is not required
#include <unordered_map>

bool MeshX::LoadObj ( const char* fname, float scal )
{	
	std::vector<std::string>	fargs;
	std::unordered_map< triple_t, xref >	vntmap;		// encoding of vert,norm,tex IDs using tripleFunc.. map vert/norm/tex -> unique id
	std::vector<Vec3F>		nlist;
	std::vector<Vec3F>		tlist;
	std::vector<Vec3F>		vlist;

	FILE* fp;
	char* chp;
	char buf[4096];
	Vec3F vec;
	Vec3F norm, fnorm;
	std::string strline, word;	
	xref v[3], n[3], t[3], fv[3];
	xref idx_max = 0;
	int tmp;
	bool bNeedNormals = true;

	t[0] = -1;

	if ( m_Format == MF_UNDEF ) {
		DeleteAllBuffers ();
		CreateFV ();
	}
	Clear ();

	fp = fopen ( fname, "rt" );	
	if ( fp == 0 ) {
		printf ( "ERROR: Cannot find file %s\n", fname );
		return false;
	}
	Vec4F palette[8];
	palette[0] = Vec4F(0.5, 0.5, 0.5, 1);
	palette[1] = Vec4F(1,0,0,1);
	palette[2] = Vec4F(0,1,0,1);
	palette[3] = Vec4F(1,1,0,1);
	palette[4] = Vec4F(0,0,1,1);
	palette[5] = Vec4F(1,0,1,1);
	palette[6] = Vec4F(0,1,1,1);
	palette[7] = Vec4F(1,1,1,1);
	
	int curr_mtl = -1;
	int64_t fid = 0;
	int64_t grp_start, grp_end;
	
	while ( feof( fp ) == 0 ) {
		fgets ( buf, 16384, fp );
		strline = buf;
		word = strSplitLeft ( strline, " " );

		switch ( word[0] ) {
		
		case 'm':
			// material library. eg. materials.mtl
			m_MtlLib = strTrim(strline);
			break;

		case 'u':												
			// use material line. eg. usemtl Stone			

			// complete the previous group
			if (curr_mtl != -1 ) {
				grp_end = fid;		// last face id recorded
				SetMtlGroup ( curr_mtl, grp_start, grp_end );
			}

			// start new group
			curr_mtl = AddMtlGroup ();

			m_MtlList.push_back ( strTrim( strline) );

			grp_start = fid;

			break;

		case 'v':												// vertex line. v {x} {y} {z}			
			if ( word[1]=='n' ) {		
				strToVec3(strline, '*', ' ', '*', &vec.x );		
				nlist.push_back ( vec );						// normals
			} else if (word[1]=='t' ) {
				strToVec3(strline, '*', ' ', '*', &vec.x );
				tlist.push_back ( vec );						// texcoords
			} else if (word[1]==0 ) {
				strToVec3(strline, '*', ' ', '*', &vec.x );
				vec *= scal;
				vlist.push_back ( vec );						// position
			}
			break;


		case 'f':
			if (strline.find("//")!=std::string::npos) {		// face line: f v//n v//n v//n
				
				bNeedNormals = false;
				fargs.clear ();
				strSplitMultiple ( strline, " \n", fargs);	
				v[0] = strToI ( strLeftOf ( fargs[0], "/") )-1;
				n[0] = strToI ( strRightOf( fargs[0], "/") )-1;
				v[1] = strToI ( strLeftOf ( fargs[1], "/") )-1;
				n[1] = strToI ( strRightOf( fargs[1], "/") )-1;
				v[2] = strToI ( strLeftOf ( fargs[2], "/") )-1;
				n[2] = strToI ( strRightOf( fargs[2], "/") )-1;
				t[0] = -1; t[1] = -1; t[2] = -1;
				
			} else if (strline.find("/")!=std::string::npos) {	  // face line: f v/t/n v/t/n v/t/n
				bNeedNormals = false;
				fargs.clear ();
				strSplitMultiple ( strline, " \n", fargs);	
				v[0] = strToI ( strLeftOf ( fargs[0], "/") )-1;
				t[0] = strToI ( strMidOf  ( fargs[0], "/") )-1;
				n[0] = strToI ( strRightOf( fargs[0], "/") )-1;
				v[1] = strToI ( strLeftOf ( fargs[1], "/") )-1;
				t[1] = strToI ( strMidOf  ( fargs[1], "/") )-1;
				n[1] = strToI ( strRightOf( fargs[1], "/") )-1;
				v[2] = strToI ( strLeftOf ( fargs[2], "/") )-1;
				t[2] = strToI ( strMidOf  ( fargs[2], "/") )-1;
				n[2] = strToI ( strRightOf( fargs[2], "/") )-1;	

			} else {												// input line: f v v v

				bNeedNormals = true;
				fargs.clear ();
				strSplitMultiple ( strline, " \n", fargs);	
				v[0] = strToI ( fargs[0] )-1;
				v[1] = strToI ( fargs[1] )-1;
				v[2] = strToI ( fargs[2] )-1;
			}

			
			// check winding order - only if we have normals
			/* if ( !bNeedNormals ) {				
				norm = vlist[v[1]] - vlist[v[0]] ;
				norm = norm.Cross ( vlist[v[2]] - vlist[v[0]] );
				norm.Normalize();
				fnorm = nlist[n[0]];
				float flip = (norm.Dot(fnorm) > 0 ? 1 : -1);
			
				if ( flip==-1 ) {				// fix winding order
					tmp = v[1]; v[1] = v[2]; v[2] = tmp;
					tmp = n[1]; n[1] = n[2]; n[2] = tmp;
				}
			} */

			// convert face-normals pairs into independent vertex groups			
			// - both the normal and texture coords are included in the mapping
			// - normals: allow two verts with same pos to have diff normals -> creases
			// - texuvs:  allow two verts with same pos to have diff uvs -> seams			
			// vntmap = map from vert/norm/tex to fv, vert index list (unique verts)
			// vlist  = master vert list
			// nlist  = master norm list
			// tlist  = master uv list
			// fv     = vert index list, this face
			// v      = current verts, this face
			// n      = current norms, this face
			// t      = current uvs, this face
			
			xref tf;

			// for each vertex in the currently added face 'f'..
			//
			for (int j=0; j < 3; j++ ) {				
						
				// find if it already exists..
				if ( vntmap.find ( triple_t( v[j], n[j], t[j] ) ) == vntmap.end() ) {
					
					// if not. create a new vertex:
					fv[j] = AddVert ( vlist[v[j]] );			// vertex position

					if (!bNeedNormals)
						AddVertNorm( nlist[n[j]] );				// vertex normal

					if (curr_mtl >= 0 )
						AddVertClr( palette[ curr_mtl % 8 ] );	// vertex color (optional)

					if ( t[0] >= 0 && tlist.size()>0 ) 
						AddVertTex ( tlist[t[j]] );				// vertex texcoord (optional)
					
					// store large mapping so we can find it later
					vntmap[ triple_t( v[j], n[j], t[j] ) ] = fv[j];
				
				}  else {

					// found. use recorded vertex:
					fv[j] = vntmap[ triple_t( v[j], n[j], t[j] ) ];	

					Vec4F c = palette[ curr_mtl % 8 ];
					SetVertClr ( fv[j], COLORA(c.x,c.y,c.z,c.w) );
				}

				// record max idx
				if (v[j] > idx_max) idx_max = v[j];
				if (n[j] > idx_max) idx_max = n[j];
				if (t[j] > idx_max) idx_max = t[j];
				if (fv[j] > idx_max) idx_max = fv[j];

				/*if (fv[j] > 170000 && j==0) {					
					printf ( "%lld %lld %lld %lld\n", fv[j], v[j], n[j], t[j] );
				}*/
			}

			// add current face				
			//			
			fid = AddFaceFast ( fv[0], fv[1], fv[2] );

			break;
		}
	}	
	
	// finish the last group
	if ( curr_mtl != -1 ) {
		grp_end = fid;		// last face id recorded		
		SetMtlGroup ( curr_mtl, grp_start, grp_end );
	}

	printf ( " Mesh. idx_max: %lld\n",  (unsigned long long) idx_max );

	//Measure ();

	if ( bNeedNormals ) 
		ComputeNormals (false);

	return true;
}


#ifdef USE_NVGUI
	void MeshX::DrawNormals (float ln, Matrix4F& xform)
	{
		// Draw normals
		Vec3F* vp = (Vec3F*) GetStart(BVERTPOS);
		Vec3F* vn = (Vec3F*) GetStart(BVERTNORM);
	
		Vec3F a, b;

		for (; vp <= (Vec3F*) GetEnd(BVERTPOS); ) {		

			a = *vp;				a *= xform;	
			b = *vp + (*vn * ln);	b *= xform;

			drawLine3D( a, b, Vec4F(1,1,0,1) );

			vn++;
			vp++;
		}
	} 

	void MeshX::Sketch (int w, int h, Camera3D * cam)
	{
		// Draw triangle edges
		for (f = (AttrV3*) GetStart(BFACEV3); f <= (AttrV3*) GetEnd(BFACEV3); f++ ) {
			v1 = *GetVertPos( f->v1 ) + v0;	v2 = *GetVertPos( f->v2 ) + v0;	v3 = *GetVertPos( f->v3 ) + v0;	
			drawLine3D ( v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, 1,1,1,1 );
			drawLine3D ( v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, 1,1,1,1 );
			drawLine3D ( v3.x, v3.y, v3.z, v1.x, v1.y, v1.z, 1,1,1,1 );
		} 
	}

	
	/*bool MeshX::update_data ( bufRef b )
	{
		PERF_PUSH ( 'ren3', "MeshX::update" );

		DataX* dat = b.dat;
		GLuint* VBO = (GLuint*) getVBO ();
		int flgs = dat->getUpdate ();

		if ( flgs & (objFlags) ObjectX::fBuild ) {			
			// Allocate VBO array
			#ifdef DEBUG_GLERR
				glGetError ();
			#endif
			if ( VBO != 0x0 ) {
				glDeleteBuffersARB ( 5, VBO );
				dat->DeleteVBO ();
			}
			VBO = (GLuint*) dat->CreateVBO ( 10 );

			// Generate textures
			glGenBuffersARB ( 5, VBO );
			#ifdef DEBUG_GLERR
				if ( glGetError() != GL_NO_ERROR ) { dat->setUpdate(ObjectX::fBuild); return false; }
			#endif
			dat->clearUpdate ( ObjectX::fBuild );																				// clear build
			dat->setUpdate ( ObjectX::fUpdatePos );
		}
	
		if ( flgs & (objFlags) ObjectX::fUpdatePos ) {
			VBO[ 5 ] = dat->GetBufStride ( m_PosBuf );
			glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 0 ] );
			glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_PosBuf), NULL, GL_STATIC_DRAW_ARB);
			glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_PosBuf), dat->GetBufData(m_PosBuf), GL_STATIC_DRAW_ARB);
		}
		if ( flgs & (objFlags) ObjectX::fUpdateClr ) {
			VBO[ 6 ] = 0;
			if ( m_ClrBuf != BUF_UNDEF  ) {
				VBO[ 6 ] = dat->GetBufStride ( m_ClrBuf );
				glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 1 ] );
				glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_ClrBuf), NULL, GL_STATIC_DRAW_ARB);			
				glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_ClrBuf), dat->GetBufData(m_ClrBuf), GL_STATIC_DRAW_ARB);
			}
		}
		if ( flgs & (objFlags) ObjectX::fUpdateNorm ) {
			VBO[ 7 ] = 0;
			if ( m_NormBuf != BUF_UNDEF  ) {
				VBO[ 7 ] = dat->GetBufStride ( m_NormBuf );
				glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 2 ] );
				glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_NormBuf), NULL, GL_STATIC_DRAW_ARB);
				glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_NormBuf), dat->GetBufData(m_NormBuf), GL_STATIC_DRAW_ARB);
			}
		}
		if ( flgs & (objFlags) ObjectX::fUpdateTex ) {
			VBO[ 8 ] = 0;
			if ( m_TexBuf != BUF_UNDEF ) {
				VBO[ 8 ] = dat->GetBufStride ( m_TexBuf );
				glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 3 ] );
				glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_TexBuf), NULL, GL_STATIC_DRAW_ARB);
				glBufferDataARB ( GL_ARRAY_BUFFER_ARB, dat->GetBufSize(m_TexBuf), dat->GetBufData(m_TexBuf), GL_STATIC_DRAW_ARB);
			}
		}
		if ( flgs & (objFlags) ObjectX::fUpdateElems ) {		
			glBindBufferARB ( GL_ELEMENT_ARRAY_BUFFER_ARB, VBO[ 4 ] );
			glBufferDataARB ( GL_ELEMENT_ARRAY_BUFFER_ARB, dat->GetBufSize( m_FaceV3Buf ), NULL, GL_STATIC_DRAW_ARB);
			glBufferDataARB ( GL_ELEMENT_ARRAY_BUFFER_ARB, dat->GetBufSize( m_FaceV3Buf ), dat->GetBufData( m_FaceV3Buf ), GL_STATIC_DRAW_ARB);
		}
		dat->clearUpdate ( ObjectX::fGeom );			// Scene object has been updated

		PERF_POP ( 'ren3' );
		return true;
	}*/


	/*void MeshX::render_data ( bufRef b )
	{
		if ( rendGL->RenderTransparent() ) return;

		PERF_PUSH ( 'ren3', "MeshX::render" );

		rendGL->runShader ( this, 0 );	

		GLuint* VBO = (GLuint*) b.dat->getVBO();

		glDisable ( GL_TEXTURE_2D );
		glBindBuffer ( GL_ARRAY_BUFFER, VBO[ 0 ] );	
		glVertexAttribPointer( localPos, 3, GL_FLOAT, GL_FALSE, 0x0, 0 );
		glBindBuffer ( GL_ARRAY_BUFFER, VBO[ 2 ] );	
		glVertexAttribPointer( localNorm, 3, GL_FLOAT, GL_FALSE, 0x0, 0 );
		glBindBuffer ( GL_ARRAY_BUFFER, VBO[ 3 ] );	
		glVertexAttribPointer( localTex,  2, GL_FLOAT, GL_FALSE, 0x0, 0 );
	
		glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER_ARB, VBO[ 4 ] );	

		glDrawRangeElements ( GL_TRIANGLES, 0, GetNumVert (), GetNumFace3()*3, GL_UNSIGNED_INT, 0x0 );	

		 glEnableClientState ( GL_VERTEX_ARRAY );
		glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 0 ] );	
		glVertexPointer ( 3, GL_FLOAT, VBO[ 5 ], 0x0 );
		glBindBufferARB ( GL_ELEMENT_ARRAY_BUFFER_ARB, VBO[ 4 ] );	
		#ifndef BUILD_NOCOLORBUF
			if ( VBO[ 6 ] != 0 ) { 
				glEnable ( GL_COLOR_MATERIAL );
				glColorMaterial ( GL_FRONT_AND_BACK, GL_DIFFUSE );
				glEnableClientState ( GL_COLOR_ARRAY ); glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 1 ] ); glColorPointer ( GL_BGRA, GL_UNSIGNED_BYTE, VBO[ 6 ], 0x0 );
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			}
		#endif	
		if ( VBO[ 7 ] != 0 ) { glEnableClientState ( GL_NORMAL_ARRAY );	glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 2 ] ); glNormalPointer ( GL_FLOAT, VBO[ 7 ], 0x0 );}  
		if ( VBO[ 8 ] != 0 ) { glEnableClientState ( GL_TEXTURE_COORD_ARRAY ); glBindBufferARB ( GL_ARRAY_BUFFER_ARB, VBO[ 3 ] ); glTexCoordPointer ( VBO[8]>>2, GL_FLOAT, VBO[8], 0x0 );}  
		
		 #ifndef BUILD_NOCOLORBUF
			glDisableClientState ( GL_COLOR_ARRAY );
		#endif
		glDisableClientState ( GL_VERTEX_ARRAY );		
		glDisableClientState ( GL_NORMAL_ARRAY );
		glDisableClientState ( GL_TEXTURE_COORD_ARRAY ); 

		PERF_POP ( 'ren3' );
	}
	*/
#endif


// ***** NOTE ********** THIS FUNCTION should be moved outside of Mesh. 
// It is more an applied use of Mesh.
//   #include "geom_helper.h"   // needed for intersectRayTriangle
// Raytrace mesh
// Returns:
//  vndx  - index of the hit face and nearest vertex
//  vhit  - position of hit in face
//  vnear - position of nearest vertex
//
/* bool MeshX::Raytrace ( Vec3F orig, Vec3F dir, Matrix4F& xform, Vec3I& vndx, Vec3F& vnear, Vec3F& vhit, Vec3F& vnorm )
{
	AttrV3* f;	
	int fbest;
	float t, tbest, d, dbest;
	Vec3F v[3], hit;
	float alpha, beta;
	bool front;
	
	// find nearest hit triangle	
	fbest = -1;
	t = 1.0e10;
	tbest = 1.0e10;
	f = (AttrV3*) GetStart(BFACEV3);
	for ( int fi=0; fi < GetNumElem(BFACEV3); fi++) {

		v[0] = *GetVertPos( f->v1 );	v[0] *= xform;							// mesh transform
		v[1] = *GetVertPos( f->v2 );	v[1] *= xform;
		v[2] = *GetVertPos( f->v3 );	v[2] *= xform;	

		if ( intersectRayTriangle ( orig, dir, v[0], v[1], v[2], t, alpha, beta, front ) ) {	// check for triangle hit
			if ( t < tbest ) {													// find nearest hit
				fbest = fi;
				tbest = t;
			}			
		}
		f++;
	}
	if ( fbest==-1 ) return false;		// no hit

	// find nearest vertex in triangle
	dbest = 1.0e20;
	f = (AttrV3*) GetElem(BFACEV3, fbest );
	v[0] = *GetVertPos( f->v1 );	v[0] *= xform;
	v[1] = *GetVertPos( f->v2 );	v[1] *= xform;
	v[2] = *GetVertPos( f->v3 );	v[2] *= xform;	
	
	for (int i=0; i < 3; i++ ) {
		d = sqrt( (v[i].x-hit.x)*(v[i].x-hit.x) + (v[i].y-hit.y)*(v[i].y-hit.y) + (v[i].z-hit.z)*(v[i].z-hit.z) );		// distance from hit to each vertex
		if ( d < dbest ) {													// find nearest vertex
			vndx.x = fbest;													// face id
			vndx.y = (i==0) ? f->v1 : ((i==1) ? f->v2 : f->v3);				// vertex id					
			vndx.z = i;
			dbest = d;
		}
	}
	vhit = orig + dir * tbest;							// return best hit
	vnear = v[ vndx.z ];								// and nearest vertex
	
	xform.SetTranslate( Vec3F(0, 0, 0) );						// remove translation from transform
	vnorm = *GetVertNorm(vndx.y);	vnorm *= xform;		// get oriented normal
	return true;
} */
