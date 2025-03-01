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


#include "camera3d.h"

// Camera3D IMPLEMENTATION 
// 
// The Camera3D transformation of an arbitrary point is:
//
//		Q' = Q * T * R * P
//
// where Q  = 3D point
//		 Q' = Screen point
//		 T = Camera3D Translation (moves Camera3D to origin)
//		 R = Camera3D Rotation (rotates Camera3D to point 'up' along Z axis)
//       P = Projection (projects points onto XY plane)
// 
// T is a unit-coordinate system translated to origin from Camera3D:
//		[1	0  0  0]
//		[0	1  0  0]
//		[0	0  1  0]
//		[-cx -cy -cz 0]		where c is Camera3D location
// R is a basis matrix:	
//
// P is a projection matrix:
//

Camera3D::Camera3D ()
{	
	mProjType = Perspective;
	mWire = 0;
	mXres = 0; mYres = 0;

	up_dir.Set ( 0.0, 1.0, 0 );				// frustum params
	mAspect = (float) 800.0f/600.0f;
	mDolly = 5.0;
	mFov = 40.0;	
	mNear = (float) 0.1;
	mFar = (float) 400.0;
	mTile.Set ( 0, 0, 1, 1 );
	mDOF.Set ( 0.18, 35, 8 );		// DOF. f/4, 35m, 8 meters

	for (int n=0; n < 8; n++ ) mOps[n] = false;	
	mOps[0] = false;

//	mOps[0] = true;		
//	mOps[1] = true;

	SetOrbit ( 0, 45, 0, Vec3F(0,0,0), 120.0, 1.0 );
	LookAt ();
}

void Camera3D::getBounds (Vec2F cmin, Vec2F cmax, float dst, Vec3F& min, Vec3F& max)
{
	Vec3F a = getPos();
	Vec3F b = a + (inverseRay(cmin.x, cmin.y, 1, 1) * dst);
	Vec3F c = a + (inverseRay(cmax.x, cmax.y, 1, 1) * dst);

	min = a; max = a;
	if ( b.x < min.x ) min.x = b.x;
	if ( b.y < min.y ) min.y = b.y;
	if ( b.z < min.z)  min.z = b.z;
	if ( b.x > max.x)  max.x = b.x;
	if ( b.y > max.y)  max.y = b.y;
	if ( b.z > max.z)  max.z = b.z;
	
	if ( c.x < min.x) min.x = c.x;
	if ( c.y < min.y) min.y = c.y;
	if ( c.z < min.z)  min.z = c.z;
	if ( c.x > max.x)  max.x = c.x;
	if ( c.y > max.y)  max.y = c.y;
	if ( c.z > max.z)  max.z = c.z;
}


bool Camera3D::pointInFrustum ( float x, float y, float z )
{
	int p;
	for ( p = 0; p < 6; p++ )
		if( frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= 0 )
			return false;
	return true;
}

bool Camera3D::boxInFrustum ( Vec3F bmin, Vec3F bmax)
{
	Vec3F vmin, vmax;
	int p;
	bool ret = true;	
	for ( p = 0; p < 6; p++ ) {
		vmin.x = ( frustum[p][0] > 0 ) ? bmin.x : bmax.x;		// Determine nearest and farthest point to plane
		vmax.x = ( frustum[p][0] > 0 ) ? bmax.x : bmin.x;		
		vmin.y = ( frustum[p][1] > 0 ) ? bmin.y : bmax.y;
		vmax.y = ( frustum[p][1] > 0 ) ? bmax.y : bmin.y;
		vmin.z = ( frustum[p][2] > 0 ) ? bmin.z : bmax.z;
		vmax.z = ( frustum[p][2] > 0 ) ? bmax.z : bmin.z;
		if ( frustum[p][0]*vmax.x + frustum[p][1]*vmax.y + frustum[p][2]*vmax.z + frustum[p][3] <= 0 ) return false;		// If nearest point is outside, Box is outside
		else if ( frustum[p][0]*vmin.x + frustum[p][1]*vmin.y + frustum[p][2]*vmin.z + frustum[p][3] <= 0 ) ret = true;		// If nearest inside and farthest point is outside, Box intersects
	}
	return ret;			// No points found outside. Box must be inside.
	
	/* --- Original method - Slow yet simpler.
	int p;
	for ( p = 0; p < 6; p++ ) {
		if( frustum[p][0] * bmin.x + frustum[p][1] * bmin.y + frustum[p][2] * bmin.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmax.x + frustum[p][1] * bmin.y + frustum[p][2] * bmin.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmax.x + frustum[p][1] * bmin.y + frustum[p][2] * bmax.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmin.x + frustum[p][1] * bmin.y + frustum[p][2] * bmax.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmin.x + frustum[p][1] * bmax.y + frustum[p][2] * bmin.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmax.x + frustum[p][1] * bmax.y + frustum[p][2] * bmin.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmax.x + frustum[p][1] * bmax.y + frustum[p][2] * bmax.z + frustum[p][3] > 0 ) continue;
		if( frustum[p][0] * bmin.x + frustum[p][1] * bmax.y + frustum[p][2] * bmax.z + frustum[p][3] > 0 ) continue;
		return false;
	}
	return true;*/
}

void Camera3D::SetOrbit  ( Vec3F angs, Vec3F tp, float dist, float dolly )
{
	SetOrbit ( angs.x, angs.y, angs.z, tp, dist, dolly );
}

void Camera3D::SetOrbit ( float ax, float ay, float az, Vec3F tp, float dist, float dolly )
{
	up_dir = Vec3F(0,1,0);

	ang_euler.Set ( ax, ay, az );
	mOrbitDist = dist;
	mDolly = dolly;
	double dx, dy, dz;
	dx = cos ( ang_euler.y * DEGtoRAD ) * sin ( ang_euler.x * DEGtoRAD ) ;
	dy = sin ( ang_euler.y * DEGtoRAD );
	dz = cos ( ang_euler.y * DEGtoRAD ) * cos ( ang_euler.x * DEGtoRAD );
	from_pos.x = tp.x + (float) dx * mOrbitDist;
	from_pos.y = tp.y + (float) dy * mOrbitDist;
	from_pos.z = tp.z + (float) dz * mOrbitDist;
	to_pos = tp;
	//to_pos.x = from_pos.x - (float) dx * mDolly;
	//to_pos.y = from_pos.y - (float) dy * mDolly;
	//to_pos.z = from_pos.z - (float) dz * mDolly;	

	LookAt ();
}

// setAngles - this is only a yaw/pitch rotation
// NOTE: roll (az) is not used here!
// 
void Camera3D::setAngles ( float ax, float ay, float az )
{
	ang_euler = Vec3F(ax,ay,az);
	to_pos.x = from_pos.x - (float) (cos ( ay * DEGtoRAD ) * sin ( ax * DEGtoRAD ) * mOrbitDist);
	to_pos.y = from_pos.y - (float) (sin ( ay * DEGtoRAD ) * mOrbitDist);
	to_pos.z = from_pos.z - (float) (cos ( ay * DEGtoRAD ) * cos ( ax * DEGtoRAD ) * mOrbitDist);
	LookAt ();
}

void Camera3D::setDirection ( Vec3F from, Vec3F to, float roll )
{
	from_pos = from;
	to_pos = to;
	Vec3F del = from_pos - to_pos;	
	float r = sqrt(del.x*del.x + del.z*del.z);

	// orbit dist
	mOrbitDist = (to - from).Length();
	
	// new angles
	ang_euler.x = atan2 ( del.x, del.z ) * RADtoDEG;
	ang_euler.y = asin ( del.y / r ) * RADtoDEG;
	ang_euler.z = roll;	

	// up vector
	Matrix4F local; 	
	Vec3F xaxis, yaxis, zaxis;
	xaxis = from - to;						xaxis.Normalize();	
	zaxis = xaxis.Cross(Vec3F(0,1,0));	zaxis.Normalize();
	yaxis = zaxis.Cross(xaxis);				yaxis.Normalize();
	local.toBasis(xaxis, yaxis, zaxis);	

	up_dir.Set ( 0, cos(roll*DEGtoRAD), sin(roll*DEGtoRAD) );
	up_dir *= local; 	

	LookAt();
}

void Camera3D::moveRelative ( float dx, float dy, float dz )
{
	Vec3F vec ( dx, dy, dz );
	vec *= getRotateInv();
	to_pos += vec;
	from_pos += vec;
	LookAt ();
}

void Camera3D::moveOrbit ( float ax, float ay, float az, float dd )
{
	ang_euler += Vec3F(ax,ay,az);
	mOrbitDist += dd;
	
	double dx, dy, dz;
	dx = cos ( ang_euler.y * DEGtoRAD ) * sin ( ang_euler.x * DEGtoRAD ) ;
	dy = sin ( ang_euler.y * DEGtoRAD );
	dz = cos ( ang_euler.y * DEGtoRAD ) * cos ( ang_euler.x * DEGtoRAD );
	from_pos.x = to_pos.x + (float) dx * mOrbitDist;
	from_pos.y = to_pos.y + (float) dy * mOrbitDist;
	from_pos.z = to_pos.z + (float) dz * mOrbitDist;
	LookAt ();
}

void Camera3D::moveToPos ( float tx, float ty, float tz )
{
	to_pos += Vec3F(tx,ty,tz);

	double dx, dy, dz;
	dx = cos ( ang_euler.y * DEGtoRAD ) * sin ( ang_euler.x * DEGtoRAD ) ;
	dy = sin ( ang_euler.y * DEGtoRAD );
	dz = cos ( ang_euler.y * DEGtoRAD ) * cos ( ang_euler.x * DEGtoRAD );
	from_pos.x = to_pos.x + (float) dx * mOrbitDist;
	from_pos.y = to_pos.y + (float) dy * mOrbitDist;
	from_pos.z = to_pos.z + (float) dz * mOrbitDist;
	LookAt ();
}

void Camera3D::Copy ( Camera3D& op )
{
	mDolly = op.mDolly;
	mOrbitDist = op.mOrbitDist;
	from_pos = op.from_pos;
	to_pos = op.to_pos;
	ang_euler = op.ang_euler; 
	mProjType = op.mProjType;
	mAspect = op.mAspect;
	mFov = op.mFov;
	mNear = op.mNear;
	mFar = op.mFar;
	LookAt ();
}

void Camera3D::LookAt ()
{
	// look matrix (orientation w/o translation). Matches OpenGL's gluLookAt	
	view_matrix.makeLookAt ( from_pos, to_pos, up_dir, side_vec, up_vec, dir_vec );
	view_matrix.PreTranslate ( Vec3F(-from_pos.x, -from_pos.y, -from_pos.z ) );		// rotation /w view translate

	updateAll ();
}

void Camera3D::SetOrientation ( Vec3F angs, Vec3F pos )
{
	from_pos = pos;

	// roll, pitch, yaw;	
	view_matrix.RotateYZX ( angs );		// X=roll, Z=pitch, Y=yaw
	view_matrix.PreTranslate ( Vec3F(-from_pos.x, -from_pos.y, -from_pos.z ) );		// rotation /w view translate
	
	updateAll ();
}


void Camera3D::SetMatrices (const float* view_mtx, const float* proj_mtx, Vec3F model_pos )
{
	// Assign the matrices we have
	//  p = Tc V P M p		
	view_matrix = Matrix4F(view_mtx);
	proj_matrix = Matrix4F(proj_mtx);
	tileproj_matrix = proj_matrix;

	// From position
	from_pos = view_matrix.getTranslation();

	// Compute mFov, mAspect, mNear, mFar
	mNear = double(proj_matrix(2, 3)) / double(proj_matrix(2, 2) - 1.0f );
	mFar  = double(proj_matrix(2, 3)) / double(proj_matrix(2, 2) + 1.0f );
	double sx = 2.0f * mNear / proj_matrix(0, 0);
	double sy = 2.0f * mNear / proj_matrix(1, 1);
	mAspect = sx / sy;
	mFov = 2.0f * atan(sx / mNear) / DEGtoRAD;

	updateAll ();
}

void Camera3D::updateProj ()
{
	// construct projection matrix  --- MATCHES OpenGL's gluPerspective function (DO NOT MODIFY)
	float sy = (float) tan ( mFov * DEGtoRAD/2.0f );	
	proj_matrix = 0.0f;
	proj_matrix(0,0) = 1.0 / (sy * mAspect);		// matches OpenGL definition
	proj_matrix(1,1) = 1.0 / sy;
	proj_matrix(2,2) = -(mFar + mNear)/(mFar - mNear);			// C
	proj_matrix(2,3) = -(2.0f*mFar * mNear)/(mFar - mNear);		// D
	proj_matrix(3,2) = -1.0f;

	// construct tile projection matrix --- MATCHES OpenGL's glFrustum function (DO NOT MODIFY) 
	/*float l, r, t, b;
	l = -sx + 2.0f*sx*mTile.x;						// Tile is in range 0 <= x,y <= 1
	r = -sx + 2.0f*sx*mTile.z;
	t =  sy - 2.0f*sy*mTile.y;
	b =  sy - 2.0f*sy*mTile.w;
	tileproj_matrix = 0.0f;
	tileproj_matrix(0,0) = 2.0f*mNear / (r - l);
	tileproj_matrix(1,1) = 2.0f*mNear / (t - b);
	tileproj_matrix(0,2) = (r + l) / (r - l);		// A
	tileproj_matrix(1,2) = (t + b) / (t - b);		// B
	tileproj_matrix(2,2) = proj_matrix(2,2);		// C
	tileproj_matrix(2,3) = proj_matrix(2,3);		// D
	tileproj_matrix(3,2) = -1.0f; */
	tileproj_matrix = proj_matrix;	
}

void Camera3D::updateView ()
{	
	origRayWorld = from_pos;
}

void Camera3D::updateAll ()
{
	updateProj();
	updateView();
	updateFrustum ();
}

Matrix4F Camera3D::getViewProjInv ()
{
	Matrix4F iv, ip;
	iv.makeRotationInv ( view_matrix );
	ip.InverseProj ( tileproj_matrix.GetDataF() );
	iv *= ip;
	return iv;	
}

void Camera3D::setModelMatrix ( float* mtx )
{
	memcpy ( model_matrix.GetDataF(), mtx, sizeof(float)*16 );
}

void Camera3D::setViewMatrix ( float* mtx )
{
	view_matrix = Matrix4F(mtx);
	from_pos = view_matrix.getTranslation();
	
	updateView ();
	updateFrustum ();
}

void Camera3D::setProjMatrix ( float* mtx )
{
	// Compute mFov, mAspect, mNear, mFar
	proj_matrix = Matrix4F(mtx);
	mNear = double( proj_matrix(2, 3)) / double(proj_matrix(2, 2) - 1.0f );
	mFar  = double( proj_matrix(2, 3)) / double(proj_matrix(2, 2) + 1.0f );
	double sx = 2.0f * mNear / proj_matrix(0, 0);
	double sy = 2.0f * mNear / proj_matrix(1, 1);
	mAspect = sx / sy;
	mFov = 2.0f * atan(sx / mNear) / DEGtoRAD;

	updateProj ();
}

void Camera3D::updateFrustum ()
{
	Matrix4F mv;
	mv = tileproj_matrix;					// Compute the model-view-projection matrix
	mv *= view_matrix;
	float* mvm = mv.GetDataF();
	float t;

	// Right plane
   frustum[0][0] = mvm[ 3] - mvm[ 0];
   frustum[0][1] = mvm[ 7] - mvm[ 4];
   frustum[0][2] = mvm[11] - mvm[ 8];
   frustum[0][3] = mvm[15] - mvm[12];
   t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
   frustum[0][0] /= t; frustum[0][1] /= t; frustum[0][2] /= t; frustum[0][3] /= t;
	// Left plane
   frustum[1][0] = mvm[ 3] + mvm[ 0];
   frustum[1][1] = mvm[ 7] + mvm[ 4];
   frustum[1][2] = mvm[11] + mvm[ 8];
   frustum[1][3] = mvm[15] + mvm[12];
   t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2]    * frustum[1][2] );
   frustum[1][0] /= t; frustum[1][1] /= t; frustum[1][2] /= t; frustum[1][3] /= t;
	// Bottom plane
   frustum[2][0] = mvm[ 3] + mvm[ 1];
   frustum[2][1] = mvm[ 7] + mvm[ 5];
   frustum[2][2] = mvm[11] + mvm[ 9];
   frustum[2][3] = mvm[15] + mvm[13];
   t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2]    * frustum[2][2] );
   frustum[2][0] /= t; frustum[2][1] /= t; frustum[2][2] /= t; frustum[2][3] /= t;
	// Top plane
   frustum[3][0] = mvm[ 3] - mvm[ 1];
   frustum[3][1] = mvm[ 7] - mvm[ 5];
   frustum[3][2] = mvm[11] - mvm[ 9];
   frustum[3][3] = mvm[15] - mvm[13];
   t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2]    * frustum[3][2] );
   frustum[3][0] /= t; frustum[3][1] /= t; frustum[3][2] /= t; frustum[3][3] /= t;
	// Far plane
   frustum[4][0] = mvm[ 3] - mvm[ 2];
   frustum[4][1] = mvm[ 7] - mvm[ 6];
   frustum[4][2] = mvm[11] - mvm[10];
   frustum[4][3] = mvm[15] - mvm[14];
   t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2]    * frustum[4][2] );
   frustum[4][0] /= t; frustum[4][1] /= t; frustum[4][2] /= t; frustum[4][3] /= t;
	// Near plane
   frustum[5][0] = mvm[ 3] + mvm[ 2];
   frustum[5][1] = mvm[ 7] + mvm[ 6];
   frustum[5][2] = mvm[11] + mvm[10];
   frustum[5][3] = mvm[15] + mvm[14];
   t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2]    * frustum[5][2] );
   frustum[5][0] /= t; frustum[5][1] /= t; frustum[5][2] /= t; frustum[5][3] /= t;

   tlRayWorld = inverseRayProj(-1.0f,  1.0f, 1.0f);
   trRayWorld = inverseRayProj(1.0f, 1.0f, 1.0f);
   blRayWorld = inverseRayProj(-1.0f, -1.0f, 1.0f);
   brRayWorld = inverseRayProj(1.0f, -1.0f, 1.0f);
}

float Camera3D::calculateLOD ( Vec3F pnt, float minlod, float maxlod, float maxdist )
{
	Vec3F vec = pnt;
	vec -= from_pos;
	float lod = minlod + ((float) vec.Length() * (maxlod-minlod) / maxdist );	
	lod = (lod < minlod) ? minlod : lod;
	lod = (lod > maxlod) ? maxlod : lod;
	return lod;
}

float Camera3D::getDu ()
{
	return (float) tan ( mFov * DEGtoRAD/2.0f ) * mNear;
}
float Camera3D::getDv ()
{
	return (float) tan ( mFov * DEGtoRAD/2.0f ) * mNear / mAspect;
}

Vec3F Camera3D::getU ()
{
	return side_vec;
}
Vec3F Camera3D::getV ()
{
	return up_vec;
}
Vec3F Camera3D::getW ()
{
	return dir_vec;
}
Matrix4F Camera3D::getUVWMatrix()
{
	Matrix4F uvw;
	uvw.toBasis ( side_vec, up_vec, dir_vec);
	
	//toBasisInv(xaxis, yaxis, zaxis);

	return uvw;
}


/*void Camera3D::setModelMatrix ()
{
	glGetFloatv ( GL_MODELVIEW_MATRIX, model_matrix.GetDataF() );
}
void Camera3D::setModelMatrix ( Matrix4F& model )
{
	model_matrix = model;
	mv_matrix = model;
	mv_matrix *= view_matrix;
	#ifdef USE_DX

	#else
		glLoadMatrixf ( mv_matrix.GetDataF() );
	#endif
}
*/

Vec3F Camera3D::inverseRayProj(float x, float y, float z)
{
	Vec4F p(x, y, z, 1.0f);

	// inverse view-projection matrix
	Matrix4F ivp = getViewProjInv();

	Vec4F wp(0.0f, 0.0f, 0.0f, 0.0f);
	wp.x = ivp.data[0] * p.x + ivp.data[4] * p.y + ivp.data[8] * p.z + ivp.data[12];
	wp.y = ivp.data[1] * p.x + ivp.data[5] * p.y + ivp.data[9] * p.z + ivp.data[13];
	wp.z = ivp.data[2] * p.x + ivp.data[6] * p.y + ivp.data[10] * p.z + ivp.data[14];
	wp.w = ivp.data[3] * p.x + ivp.data[7] * p.y + ivp.data[11] * p.z + ivp.data[15];

	return Vec3F(wp.x / wp.w, wp.y / wp.w, wp.z / wp.w);
}

Vec3F Camera3D::inverseRay (float x, float y, float xres, float yres, float z)
{		
	float sy = (float) tan(mFov * DEGtoRAD / 2.0f);
	float sx = sy * mAspect;
	float tu, tv;
	tu = mTile.x + x * (mTile.z-mTile.x) / xres;		// *NOTE*. If mXres=0 you must call cam.setSize(w,h) with screen res.
	tv = mTile.y + y * (mTile.w-mTile.y) / yres;
	Vec4F pnt ( (tu-0.5f)*2.0*sx , (0.5f-tv)*2.0*sy, -z, 1 );
	// x = 2*near/sx
	// y = 2*near/sy;

	Matrix4F ir = getRotateInv ();
	pnt *= ir;
	pnt.Normalize ();

	return pnt;
}

Vec4F Camera3D::project ( Vec3F& p, Matrix4F& vm )
{
	Vec4F q = p;								// World coordinates
	q.w = 1.0;
	
	q *= vm;										// Eye coordinates
	
	q *= tileproj_matrix;								// Projection 

	q /= q.w;										// Normalized Device Coordinates (w-divide)
	
	q.x = (q.x*0.5f+0.5f) / mXres;
	q.y = (q.y*0.5f+0.5f) / mYres;
	q.z = q.z*0.5f + 0.5f;							// Stored depth buffer value
		
	return q;
}


Vec4F Camera3D::project ( Vec3F& p )
{
	Vec4F q = p;								// World coordinates
	q.w = 1.0;
	q *= view_matrix;								// Eye coordinates

	q *= tileproj_matrix;								// Clip coordinates
	
	q /= q.w;										// Normalized Device Coordinates (w-divide)

	q.x = (q.x*0.5f+0.5f)*mXres;
	q.y = (0.5f-q.y*0.5f)*mYres;
	q.z = q.z*0.5f + 0.5f;							// Stored depth buffer value
		
	return q;
}

void PivotX::setPivot ( float x, float y, float z, float rx, float ry, float rz )
{
	from_pos.Set ( x,y,z);
	ang_euler.Set ( rx,ry,rz );
}

void PivotX::updateTform ()
{
	trans.RotateZYXT ( ang_euler, from_pos );
}


/*void Camera3D::draw_gl ()
{
	Vec3F pnt; 
	int va, vb;
	
	if ( !mOps[0] ) return;

	// Box testing
	//
	// NOTES: This demonstrates AABB box testing against the frustum 
	// Boxes tested are 10x10x10 size, spaced apart from each other so we can see them.
	if ( mOps[5] ) {
		glPushMatrix ();
		glEnable ( GL_LIGHTING );
		glColor3f ( 1, 1, 1 );	
		Vec3F bmin, bmax, vmin, vmax;
		int lod;
		for (float y=0; y < 100; y += 10.0 ) {
		for (float z=-100; z < 100; z += 10.0 ) {
			for (float x=-100; x < 100; x += 10.0 ) {
				bmin.Set ( x, y, z );
				bmax.Set ( x+8, y+8, z+8 );
				if ( boxInFrustum ( bmin, bmax ) ) {				
					lod = (int) calculateLOD ( bmin, 1, 5, 300.0 );
					//rendGL->drawCube ( bmin, bmax, Vec3F(1,1,1) );
				}
			}
		}
		}
		glPopMatrix ();
	}

	glDisable ( GL_LIGHTING );	
	glLoadMatrixf ( getViewMatrix().GetDataF() );

	// Frustum planes (world space)
	//
	// NOTE: The frustum planes are drawn as discs because
	// they are boundless (infinite). The minimum information contained in the
	// plane equation is normal direction and distance from plane to origin.
	// This sufficiently defines infinite planes for inside/outside testing,
	// but cannot be used to draw the view frustum without more information.
	// Drawing is done as discs here to verify the frustum plane equations.
	if ( mOps[2] ) {
		glBegin ( GL_POINTS );
		glColor3f ( 1, 1, 0 );
		Vec3F norm;
		Vec3F side, up;
		for (int n=0; n < 6; n++ ) {
			norm.Set ( frustum[n][0], frustum[n][1], frustum[n][2] );
			glColor3f ( n/6.0, 1.0- (n/6.0), 0.5 );
			side = Vec3F(0,1,0); side.Cross ( norm ); side.Normalize ();	
			up = side; up.Cross ( norm ); up.Normalize();
			norm *= frustum[n][3];
			for (float y=-50; y < 50; y += 1.0 ) {
				for (float x=-50; x < 50; x += 1.0 ) {
					if ( x*x+y*y < 1000 ) {
						//pnt = side * x + up * y - norm; 
                        pnt = side;
                        Vec3F tv = up;

                        tv *= y;
                        pnt *= x;
                        pnt += tv;
                        pnt -= norm;

						glVertex3f ( pnt.x, pnt.y, pnt.z );
					}
				}
			}
		}
		glEnd (); 
	}

	// Inside/outside testing
	//
	// NOTES: This code demonstrates frustum clipping 
	// tests on individual points.
	if ( mOps[4] ) {
		glColor3f ( 1, 1, 1 );
		glBegin ( GL_POINTS );
		for (float z=-100; z < 100; z += 4.0 ) {
			for (float y=0; y < 100; y += 4.0 ) {
				for (float x=-100; x < 100; x += 4.0 ) {
					if ( pointInFrustum ( x, y, z) ) {
						glVertex3f ( x, y, z );
					}
				}
			}
		}
		glEnd ();
	}
	
	// Inverse rays (world space)
	//
	// NOTES: This code demonstrates drawing 
	// inverse camera rays, as might be needed for raytracing or hit testing.
	if ( mOps[3] ) {
		glBegin ( GL_LINES );
		glColor3f ( 0, 1, 0);
		for (float x = 0; x <= 1.0; x+= 0.5 ) {
			for (float y = 0; y <= 1.0; y+= 0.5 ) {
				pnt = inverseRay ( x, y, mFar );
				pnt += from_pos;
				glVertex3f ( from_pos.x, from_pos.y, from_pos.z );		// all inverse rays originate at the camera center
				glVertex3f ( pnt.x, pnt.y, pnt.z );
			}
		}
		glEnd ();
	}

	// Projection
	//
	// NOTES: This code demonstrates 
	// perspective projection _without_ using the OpenGL pipeline.
	// Projection is done by the camera class. A cube is drawn on the near plane.
	
	// Cube geometry
	Vec3F pnts[8];
	Vec3I edge[12];
	pnts[0].Set (  0,  0,  0 );	pnts[1].Set ( 10,  0,  0 ); pnts[2].Set ( 10,  0, 10 ); pnts[3].Set (  0,  0, 10 );		// lower points (y=0)
	pnts[4].Set (  0, 10,  0 );	pnts[5].Set ( 10, 10,  0 ); pnts[6].Set ( 10, 10, 10 ); pnts[7].Set (  0, 10, 10 );		// upper points (y=10)
	edge[0].Set ( 0, 1, 0 ); edge[1].Set ( 1, 2, 0 ); edge[2].Set ( 2, 3, 0 ); edge[3].Set ( 3, 0, 0 );					// 4 lower edges
	edge[4].Set ( 4, 5, 0 ); edge[5].Set ( 5, 6, 0 ); edge[6].Set ( 6, 7, 0 ); edge[7].Set ( 7, 4, 0 );					// 4 upper edges
	edge[8].Set ( 0, 4, 0 ); edge[9].Set ( 1, 5, 0 ); edge[10].Set ( 2, 6, 0 ); edge[11].Set ( 3, 7, 0 );				// 4 vertical edges
	
	// -- White cube is drawn using OpenGL projection
	if ( mOps[6] ) {
		glBegin ( GL_LINES );
		glColor3f ( 1, 1, 1);
		for (int e = 0; e < 12; e++ ) {
			va = edge[e].x;
			vb = edge[e].y;
			glVertex3f ( pnts[va].x, pnts[va].y, pnts[va].z );
			glVertex3f ( pnts[vb].x, pnts[vb].y, pnts[vb].z );
		}
		glEnd ();	
	}

	//---- Draw the following in camera space..
	// NOTES:
	// The remainder drawing steps are done in 
	// camera space. This is done by multiplying by the
	// inverse_rotation matrix, which transforms from camera to world space.
	// The camera axes, near, and far planes can now be drawn in camera space.
	glPushMatrix ();
	glLoadMatrixf ( getViewMatrix().GetDataF() );
	glTranslatef ( from_pos.x, from_pos.y, from_pos.z );
	glMultMatrixf ( invrot_matrix.GetDataF() );				// camera space --to--> world space

	// -- Red cube is drawn on the near plane using software projection pipeline. See Camera3D::project
	if ( mOps[6] ) {
		glBegin ( GL_LINES );
		glColor3f ( 1, 0, 0);
		Vec4F proja, projb;
		for (int e = 0; e < 12; e++ ) {
			va = edge[e].x;
			vb = edge[e].y;
			proja = project ( pnts[va] );
			projb = project ( pnts[vb] );
			if ( proja.w > 0 && projb.w > 0 && proja.w < 1 && projb.w < 1) {	// Very simple Z clipping  (try commenting this out and see what happens)
				glVertex3f ( proja.x, proja.y, proja.z );
				glVertex3f ( projb.x, projb.y, projb.z );
			}
		}
		glEnd ();
	}
	// Camera axes
	glBegin ( GL_LINES );
	float to_d = (from_pos - to_pos).Length();
	glColor3f ( .8,.8,.8); glVertex3f ( 0, 0, 0 );	glVertex3f ( 0, 0, -to_d );
	glColor3f ( 1,0,0); glVertex3f ( 0, 0, 0 );		glVertex3f ( 10, 0, 0 );
	glColor3f ( 0,1,0); glVertex3f ( 0, 0, 0 );		glVertex3f ( 0, 10, 0 );
	glColor3f ( 0,0,1); glVertex3f ( 0, 0, 0 );		glVertex3f ( 0, 0, 10 );
	glEnd ();

	if ( mOps[1] ) {
		// Near plane
		float sy = tan ( mFov * DEGtoRAD / 2.0);
		float sx = sy * mAspect;
		glColor3f ( 0.8, 0.8, 0.8 );
		glBegin ( GL_LINE_LOOP );
		glVertex3f ( -mNear*sx,  mNear*sy, -mNear );
		glVertex3f (  mNear*sx,  mNear*sy, -mNear );
		glVertex3f (  mNear*sx, -mNear*sy, -mNear );
		glVertex3f ( -mNear*sx, -mNear*sy, -mNear );
		glEnd ();
		// Far plane
		glBegin ( GL_LINE_LOOP );
		glVertex3f ( -mFar*sx,  mFar*sy, -mFar );
		glVertex3f (  mFar*sx,  mFar*sy, -mFar );
		glVertex3f (  mFar*sx, -mFar*sy, -mFar );
		glVertex3f ( -mFar*sx, -mFar*sy, -mFar );
		glEnd ();

		// Subview Near plane
		float l, r, t, b;
		l = -sx + 2.0*sx*mTile.x;						// Tile is in range 0 <= x,y <= 1
		r = -sx + 2.0*sx*mTile.z;
		t =  sy - 2.0*sy*mTile.y;
		b =  sy - 2.0*sy*mTile.w;
		glColor3f ( 0.8, 0.8, 0.0 );
		glBegin ( GL_LINE_LOOP );
		glVertex3f ( l * mNear, t * mNear, -mNear );
		glVertex3f ( r * mNear, t * mNear, -mNear );
		glVertex3f ( r * mNear, b * mNear, -mNear );
		glVertex3f ( l * mNear, b * mNear, -mNear );		
		glEnd ();
		// Subview Far plane
		glBegin ( GL_LINE_LOOP );
		glVertex3f ( l * mFar, t * mFar, -mFar );
		glVertex3f ( r * mFar, t * mFar, -mFar );
		glVertex3f ( r * mFar, b * mFar, -mFar );
		glVertex3f ( l * mFar, b * mFar, -mFar );		
		glEnd ();
	}

	glPopMatrix ();
}
*/

