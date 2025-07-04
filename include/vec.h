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


#ifndef HELPER_VEC
	#define HELPER_VEC

    #include "common_defs.h"
    #include <math.h>
    #include <stdlib.h>  // for rand

	class Vec2I;
	class Vec2F;
	class Vec3I;
	class Vec3F;
	class Vec4F;
	class Vec4D;
	class Matrix4F;
	class Quaternion;

	// Vec2I Declaration

	#define VNAME		2DI
	#define VTYPE		int

	class HELPAPI Vec2I {
	public:
		VTYPE x, y;

		// Constructors/Destructors
		Vec2I();							
		~Vec2I();			
		Vec2I (const VTYPE xa, const VTYPE ya);		
		Vec2I (const Vec2I &op);				// *** THESE SHOULD ALL BE const
		Vec2I (const Vec2F &op);						
		Vec2I (const Vec3I &op);				
		Vec2I (const Vec3F &op);				
		Vec2I (const Vec4F &op);

		// Member Functions		
		inline Vec2I &operator= (const Vec2I &op);
		inline Vec2I &operator= (const Vec2F &op);		
		inline Vec2I &operator= (const Vec3I &op);
		inline Vec2I &operator= (const Vec3F &op);
		inline Vec2I &operator= (const Vec4F &op);

		inline Vec2I &operator+= (const Vec2I &op);
		inline Vec2I &operator+= (const Vec2F &op);
		inline Vec2I &operator+= (const Vec3I &op);
		inline Vec2I &operator+= (const Vec3F &op);
		inline Vec2I &operator+= (const Vec4F &op);

		inline Vec2I &operator-= (const Vec2I &op);
		inline Vec2I &operator-= (const Vec2F &op);
		inline Vec2I &operator-= (const Vec3I &op);
		inline Vec2I &operator-= (const Vec3F &op);
		inline Vec2I &operator-= (const Vec4F &op);
	
		inline Vec2I &operator*= (const Vec2I &op);
		inline Vec2I &operator*= (const Vec2F &op);
		inline Vec2I &operator*= (const Vec3I &op);
		inline Vec2I &operator*= (const Vec3F &op);
		inline Vec2I &operator*= (const Vec4F &op);
		inline Vec2I &operator* (const Vec2F &op);

		inline Vec2I &operator/= (const Vec2I &op);
		inline Vec2I &operator/= (const Vec2F &op);
		inline Vec2I &operator/= (const Vec3I &op);
		inline Vec2I &operator/= (const Vec3F &op);
		inline Vec2I &operator/= (const Vec4F &op);		

		// Note: Cross product does not exist for 2D vectors (only 3D)
		inline double Dot (const Vec2I &v);
		inline double Dot (const Vec2F &v);

		inline double Dist (const Vec2I &v);
		inline double Dist (const Vec2F &v);
		inline double Dist (const Vec3I &v);
		inline double Dist (const Vec3F &v);
		inline double Dist (const Vec4F &v);

		inline double DistSq (const Vec2I &v);
		inline double DistSq (const Vec2F &v);
		inline double DistSq (const Vec3I &v);
		inline double DistSq (const Vec3F &v);
		inline double DistSq (const Vec4F &v);
		
		inline Vec2I &Normalize (void);
		inline double Length (void);
		inline VTYPE *Data (void);
	};
	
	#undef VNAME
	#undef VTYPE

	// Vec2F Declarations

	#define VNAME		2DF
	#define VTYPE		float

	class HELPAPI Vec2F {
	public:
		VTYPE x, y;

		// Constructors/Destructors
		 Vec2F ();
		 ~Vec2F ();
		 Vec2F (const VTYPE xa, const VTYPE ya);
		 Vec2F (const Vec2I &op);
		 Vec2F (const Vec2F &op);
		 Vec2F (const Vec3I &op);
		 Vec2F (const Vec3F &op);
		 Vec2F (const Vec4F &op);

		 Vec2F &Set (const float xa, const float ya);		 

		 Vec2F &operator= (const Vec2I &op);
		 Vec2F &operator= (const Vec2F &op);
		 Vec2F &operator= (const Vec3I &op);
		 Vec2F &operator= (const Vec3F &op);
		 Vec2F &operator= (const Vec4F &op);
		
		 Vec2F &operator+= (const Vec2I &op);
		 Vec2F &operator+= (const Vec2F &op);
		 Vec2F &operator+= (const Vec3I &op);
		 Vec2F &operator+= (const Vec3F &op);
		 Vec2F &operator+= (const Vec4F &op);		 

		 Vec2F &operator-= (const Vec2I &op);
		 Vec2F &operator-= (const Vec2F &op);
		 Vec2F &operator-= (const Vec3I &op);
		 Vec2F &operator-= (const Vec3F &op);
		 Vec2F &operator-= (const Vec4F &op);		 

		 Vec2F &operator*= (const Vec2I &op);
		 Vec2F &operator*= (const Vec2F &op);
		 Vec2F &operator*= (const Vec3I &op);
		 Vec2F &operator*= (const Vec3F &op);
		 Vec2F &operator*= (const Vec4F &op);
		 Vec2F &operator*= (const float op)		{ x *= op; y *= op; return *this; }		 

		 Vec2F &operator/= (const Vec2I &op);
		 Vec2F &operator/= (const Vec2F &op);
		 Vec2F &operator/= (const Vec3I &op);
		 Vec2F &operator/= (const Vec3F &op);
		 Vec2F &operator/= (const Vec4F &op);
		 Vec2F &operator/= (const double v)		{x /= (float) v; y /= (float) v; return *this;}

		 // Slower operations (makes temporary)
		 Vec2F operator- (const float op);
		 Vec2F operator- (const Vec2F &op);
		 Vec2F operator+ (const float op);
		 Vec2F operator+ (const Vec2F &op);
		 Vec2F operator* (const float op);
		 Vec2F operator* (const Vec2F &op);		 

		// Note: Cross product does not exist for 2D vectors (only 3D)		
		 double Dot(const Vec2I &v);
		 double Dot(const Vec2F &v);

		 double Dist (const Vec2I &v);
		 double Dist (const Vec2F &v);
		 double Dist (const Vec3I &v);
		 double Dist (const Vec3F &v);
		 double Dist (const Vec4F &v);

		 double DistSq (const Vec2I &v);
		 double DistSq (const Vec2F &v);
		 double DistSq (const Vec3I &v);
		 double DistSq (const Vec3F &v);
		 double DistSq (const Vec4F &v);

		 Vec2F &Normalize (void);
		 double Length (void);
		 VTYPE *Data (void);
	};
	
	#undef VNAME
	#undef VTYPE

	// Vec3I Declaration
	#define VNAME		3DI
	#define VTYPE		int

	class HELPAPI Vec3I {
	public:
		VTYPE x, y, z;
	
		// Constructors/Destructors
		Vec3I();
		Vec3I (const VTYPE xa, const VTYPE ya, const VTYPE za);
		Vec3I (const Vec3I &op);
		Vec3I (const Vec3F &op);
		Vec3I (const Vec4F &op);

		// Set Functions
		Vec3I &Set (const VTYPE xa, const VTYPE ya, const VTYPE za);

		// Member Functions		
		Vec3I &operator= (const Vec3I &op);
		Vec3I &operator= (const Vec3F &op);
		Vec3I &operator= (const Vec4F &op);
				
		Vec3I &operator+= (const Vec3I &op);
		Vec3I &operator+= (const Vec3F &op);
		Vec3I &operator+= (const Vec4F &op);

		Vec3I &operator-= (const Vec3I &op);
		Vec3I &operator-= (const Vec3F &op);
		Vec3I &operator-= (const Vec4F &op);
	
		Vec3I &operator*= (const Vec3I &op);
		Vec3I &operator*= (const Vec3F &op);
		Vec3I &operator*= (const Vec4F &op);

		Vec3I &operator/= (const Vec3I &op);
		Vec3I &operator/= (const Vec3F &op);
		Vec3I &operator/= (const Vec4F &op);

		Vec3I operator+ (const int op)			{ return Vec3I(x+op, y+op, z+op); }
		Vec3I operator+ (const float op)		{ return Vec3I(int(x+op), int(y+op), int( z+op)); }
		Vec3I operator+ (const Vec3I &op)	{ return Vec3I(x+op.x, y+op.y, z+op.z); }
		Vec3I operator- (const int op)			{ return Vec3I(x-op, y-op, z-op); }
		Vec3I operator- (const float op)		{ return Vec3I(int(x-op), int( y-op), int( z-op)); }
		Vec3I operator- (const Vec3I &op)	{ return Vec3I(x-op.x, y-op.y, z-op.z); }
		Vec3I operator* (const int op)			{ return Vec3I(x*op, y*op, z*op); }
		Vec3I operator* (const float op)		{ return Vec3I(int(x*op), int( y*op), int( z*op)); }
		Vec3I operator* (const Vec3I &op)	{ return Vec3I(x*op.x, y*op.y, z*op.z); }		
		Vec3I operator/ (const int op)			{ return Vec3I(x/op, y/op, z/op); }
		Vec3I operator/ (const float op)		{ return Vec3I(int(x/op), int(y/op), int(z/op)); }
		Vec3I operator/ (const Vec3I &op)	{ return Vec3I(x/op.x, y/op.y, z/op.z); }		

		Vec3I &Cross (const Vec3I &v);
		Vec3I &Cross (const Vec3F &v);	
		
		double Dot(const Vec3I &v);
		double Dot(const Vec3F &v);

		double Dist (const Vec3I &v);
		double Dist (const Vec3F &v);
		double Dist (const Vec4F &v);

		double DistSq (const Vec3I &v);
		double DistSq (const Vec3F &v);
		double DistSq (const Vec4F &v);

		Vec3I &Normalize (void);
		double Length (void);

		VTYPE &X(void)				{return x;}
		VTYPE &Y(void)				{return y;}
		VTYPE &Z(void)				{return z;}
		VTYPE W(void)					{return 0;}
		const VTYPE &X(void) const	{return x;}
		const VTYPE &Y(void) const	{return y;}
		const VTYPE &Z(void) const	{return z;}
		const VTYPE W(void) const		{return 0;}
		VTYPE *Data (void)			{return &x;}
	};	

	#undef VNAME
	#undef VTYPE

	// Vec3F Declarations

	#define VNAME		3DF
	#define VTYPE		float

	
	class HELPAPI Vec3F {
	public:
		VTYPE x, y, z;
	
		// Constructors/Destructors
		Vec3F() {x=0; y=0; z=0;}
		Vec3F (const VTYPE xa, const VTYPE ya, const VTYPE za);
		Vec3F (const Vec3I &op);
		Vec3F (const Vec3F &op);
		Vec3F (const Vec4F &op);
		Vec3F (const Vec4D &op);

		// Set Functions
		Vec3F &Set (const VTYPE xa, const VTYPE ya, const VTYPE za);
		
		// Member Functions
		Vec3F &operator= (const int op);
		Vec3F &operator= (const double op);
		Vec3F &operator= (const Vec2F &op);
		Vec3F &operator= (const Vec3I &op);
		Vec3F &operator= (const Vec3F &op);
		Vec3F &operator= (const Vec4F &op);
		Vec3F &operator= (const Vec4D &op);

		Vec3F &operator+= (const int op);
		Vec3F &operator+= (const double op);
		Vec3F &operator+= (const Vec3I &op);
		Vec3F &operator+= (const Vec3F &op);
		Vec3F &operator+= (const Vec4F &op);

		Vec3F &operator-= (const int op);
		Vec3F &operator-= (const double op);
		Vec3F &operator-= (const Vec3I &op);
		Vec3F &operator-= (const Vec3F &op);
		Vec3F &operator-= (const Vec4F &op);
	
		Vec3F &operator*= (const int op);
		Vec3F &operator*= (const double op);
		Vec3F &operator*= (const Vec3I &op);
		Vec3F &operator*= (const Vec3F &op);
		Vec3F &operator*= (const Vec4F &op);		
		Vec3F &operator*= (const Quaternion &op);

		Vec3F &operator/= (const int op);
		Vec3F &operator/= (const double op);
		Vec3F &operator/= (const Vec3I &op);
		Vec3F &operator/= (const Vec3F &op);
		Vec3F &operator/= (const Vec4F &op);

		// Slow operations - require temporary variables
		Vec3F operator+ (int op)				{ return Vec3F(x+float(op), y+float(op), z+float(op)); }
		Vec3F operator+ (float op)				{ return Vec3F(x+op, y+op, z+op); }
		Vec3F operator+ (const Vec3F &op)	{ return Vec3F(x+op.x, y+op.y, z+op.z); }
		Vec3F operator+ (const Vec3I &op)	{ return Vec3F(x+op.x, y+op.y, z+op.z); }
		Vec3F operator- (int op)				{ return Vec3F(x-float(op), y-float(op), z-float(op)); }
		Vec3F operator- (float op)		{		 return Vec3F(x-op, y-op, z-op); }
		Vec3F operator- (const Vec3F &op)	{ return Vec3F(x-op.x, y-op.y, z-op.z); }
		Vec3F operator- (const Vec3I &op)	{ return Vec3F(x-op.x, y-op.y, z-op.z); }
		Vec3F operator* (int op)				{ return Vec3F(x*float(op), y*float(op), z*float(op)); }
		Vec3F operator* (float op)				{ return Vec3F(x*op, y*op, z*op); }
		Vec3F operator* (const Vec3F &op)	{ return Vec3F(x*op.x, y*op.y, z*op.z); }		
		Vec3F operator* (const Vec3I &op)	{ return Vec3F(x*op.x, y*op.y, z*op.z); }				
		Vec3F operator* (const Matrix4F& op);
		Vec3F operator* (const Quaternion& op);

		Vec3F operator/ (int op)				{ return Vec3F(x/float(op), y/float(op), z/float(op)); }
		Vec3F operator/ (float op)				{ return Vec3F(x/op, y/op, z/op); }
		Vec3F operator/ (const Vec3F &op)	{ return Vec3F(x/op.x, y/op.y, z/op.z); }		
		Vec3F operator/ (const Vec3I &op)	{ return Vec3F(x/float(op.x), y/float(op.y), z/float(op.z)); }		
		// --

		bool operator< (const Vec3F& op)	{ return (x < op.x) || (y < op.y) || (z < op.z); }
		bool operator> (const Vec3F& op)  { return (x > op.x) || (y > op.y) || (z > op.z); }

		// Matrix4F Functions - requires class definition (cannot inline)
		Vec3F &operator*= (const Matrix4F &op);	

		bool Equal ( const Vec3F& op ) { return (x==op.x && y==op.y && z==op.z ); }
		bool NotEqual ( const Vec3F& op ) { return !(x==op.x && y==op.y && z==op.z ); }

		Vec3F Cross (const Vec3I &v);
		Vec3F Cross (const Vec3F &v);
		Vec3F Cross (const Vec3F& v1, const Vec3F& v2);

		// --
		static Vec3F CrossFunc (const Vec3I &v1, const Vec3I &v2) {
			Vec3F c;
			c.x = (VTYPE) (v1.y * v2.z - v1.z * v2.y); 
			c.y = (VTYPE) (-v1.x * v2.z + v1.z * v2.x); 
			c.z = (VTYPE) (v1.x * v2.y - v1.y * v2.x); 
			return c;
		}
		static Vec3F CrossFunc (const Vec3F &v1, const Vec3F &v2) {
			Vec3F c;				
			c.x = (VTYPE) (v1.y * v2.z - v1.z * v2.y); 
			c.y = (VTYPE) (-v1.x * v2.z + v1.z * v2.x); 
			c.z = (VTYPE) (v1.x * v2.y - v1.y * v2.x); 
			return c;
		}
		double Dot(const Vec3I &v);
		double Dot(const Vec3F &v);

		double Dist (const Vec3I &v);
		double Dist (const Vec3F &v);
		double Dist (const Vec4F &v);

		double DistSq (const Vec3I &v);
		double DistSq (const Vec3F &v);
		double DistSq (const Vec4F &v);
		
		Vec3F& Normalize(void);
		Vec3F& NormalizeFast(void);
		Vec3F getNormalizedFast();
		Vec3F& Clamp(float a, float b);
		
		double Length(void);
		float LengthFast ();		
		float LengthSq()			{ 	return x * x + y * y + z * z; }

		Vec3F &Random ()		{ x=float(rand())/RAND_MAX; y=float(rand())/RAND_MAX; z=float(rand())/RAND_MAX;  return *this;}
		Vec3F &Random (Vec3F a, Vec3F b)		{ x=a.x+float(rand()*(b.x-a.x))/RAND_MAX; y=a.y+float(rand()*(b.y-a.y))/RAND_MAX; z=a.z+float(rand()*(b.z-a.z))/RAND_MAX;  return *this;}
		Vec3F &Random (float x1,float x2, float y1, float y2, float z1, float z2)	{ x=x1+float(rand()*(x2-x1))/RAND_MAX; y=y1+float(rand()*(y2-y1))/RAND_MAX; z=z1+float(rand()*(z2-z1))/RAND_MAX;  return *this;}
		Vec3F RGBtoHSV ();
		Vec3F HSVtoRGB ();
		Vec3F PolarToCartesianX ( Vec3F angs );

		VTYPE &X()				{return x;}
		VTYPE &Y()				{return y;}
		VTYPE &Z()				{return z;}
		VTYPE W()					{return 0;}
		const VTYPE &X() const	{return x;}
		const VTYPE &Y() const	{return y;}
		const VTYPE &Z() const	{return z;}
		const VTYPE W() const		{return 0;}
		VTYPE *Data ()			{return &x;}
	};
	
	
	#undef VNAME
	#undef VTYPE


	// Vec4F Declarations

	#define VNAME		4DF
	#define VTYPE		float

	class HELPAPI Vec4F {
	public:
		VTYPE x, y, z, w;
	
		Vec4F &Set (const float xa, const float ya, const float za)	{ x =xa; y= ya; z=za; w=1; return *this;}
		Vec4F &Set (const float xa, const float ya, const float za, const float wa )	{ x =xa; y= ya; z=za; w=wa; return *this;}

		// Constructors/Destructors
		Vec4F() {x=0; y=0; z=0; w=0;}
		Vec4F (const VTYPE xa, const VTYPE ya, const VTYPE za, const VTYPE wa);
		Vec4F (const Vec3I &op);
		Vec4F (const Vec3F &op);
		Vec4F (const Vec3F &op, const float opw);
		Vec4F (const Vec4F &op);
		Vec4F (const Vec4D &op);
		Vec4F (const Quaternion& op);

		// Member Functions
		Vec4F &operator= (const int op);
		Vec4F &operator= (const double op);
		Vec4F &operator= (const Vec3I &op);
		Vec4F &operator= (const Vec3F &op);	
		Vec4F &operator= (const Vec4F &op);


		Vec4F &operator+= (const int op);
		Vec4F &operator+= (const float op);
		Vec4F &operator+= (const double op);
		Vec4F &operator+= (const Vec3I &op);
		Vec4F &operator+= (const Vec3F &op);
		Vec4F &operator+= (const Vec4F &op);

		Vec4F &operator-= (const int op);
		Vec4F &operator-= (const double op);
		Vec4F &operator-= (const Vec3I &op);
		Vec4F &operator-= (const Vec3F &op);
		Vec4F &operator-= (const Vec4F &op);

		Vec4F &operator*= (const int op);
		Vec4F &operator*= (const double op);
		Vec4F &operator*= (const Vec3I &op);
		Vec4F &operator*= (const Vec3F &op);
		Vec4F &operator*= (const Vec4F &op);
		Vec4F &operator*= (const float* op );
		Vec4F &operator*= (const Matrix4F &op);		

		Vec4F &operator/= (const int op);
		Vec4F &operator/= (const double op);
		Vec4F &operator/= (const Vec3I &op);
		Vec4F &operator/= (const Vec3F &op);
		Vec4F &operator/= (const Vec4F &op);

		// Slow operations - require temporary variables
		Vec4F operator+ (const int op)			{ return Vec4F(x+float(op), y+float(op), z+float(op), w+float(op)); }
		Vec4F operator+ (const float op)		{ return Vec4F(x+op, y+op, z+op, w*op); }
		Vec4F operator+ (const Vec4F &op)	{ return Vec4F(x+op.x, y+op.y, z+op.z, w+op.w); }
		Vec4F operator- (const int op)			{ return Vec4F(x-float(op), y-float(op), z-float(op), w-float(op)); }
		Vec4F operator- (const float op)		{ return Vec4F(x-op, y-op, z-op, w*op); }
		Vec4F operator- (const Vec4F &op)	{ return Vec4F(x-op.x, y-op.y, z-op.z, w-op.w); }
		Vec4F operator* (const int op)			{ return Vec4F(x*float(op), y*float(op), z*float(op), w*float(op)); }
		Vec4F operator* (const float op)		{ return Vec4F(x*op, y*op, z*op, w*op); }
		Vec4F operator* (const Vec4F &op)	{ return Vec4F(x*op.x, y*op.y, z*op.z, w*op.w); }		
		Vec4F operator* (const Matrix4F& op);
		Vec4F operator/ (const int op)			{ return Vec4F(x/float(op), y/float(op), z/float(op), w/float(op)); }
		Vec4F operator/ (const float op)		{ return Vec4F(x/op, y/op, z/op, w/op); }
		Vec4F operator/ (const Vec4F &op)	{ return Vec4F(x/op.x, y/op.y, z/op.z, w/op.w); }		
		// --

		Vec4F &Set ( CLRVAL clr )	{
			x = (float) RED(clr);		// (float( c      & 0xFF)/255.0)	
			y = (float) GRN(clr);		// (float((c>>8)  & 0xFF)/255.0)
			z = (float) BLUE(clr);		// (float((c>>16) & 0xFF)/255.0)
			w = (float) ALPH(clr);		// (float((c>>24) & 0xFF)/255.0)
			return *this;
		}
		Vec4F& fromClr ( CLRVAL clr ) { return Set (clr); }
		CLRVAL toClr () { return (CLRVAL) COLORA( x, y, z, w ); }

		Vec4F& Clamp ( float xc, float yc, float zc, float wc )
		{
			x = (x > xc) ? xc : x;
			y = (y > yc) ? yc : y;
			z = (z > zc) ? zc : z;
			w = (w > wc) ? wc : w;
			return *this;
		}

		Vec4F Cross (const Vec4F &v);	
		
		double Dot (const Vec4F &v);

		double Dist (const Vec4F &v);

		double DistSq (const Vec4F &v);

		Vec4F &Normalize (void);
		double Length (void);

		Vec4F &Random ()		{ x=float(rand())/RAND_MAX; y=float(rand())/RAND_MAX; z=float(rand())/RAND_MAX; w = 1;  return *this;}

		VTYPE C(int i)				{return ((VTYPE*) &x)[i]; }		// i-th component
		VTYPE &X(void)				{return x;}
		VTYPE &Y(void)				{return y;}
		VTYPE &Z(void)				{return z;}
		VTYPE &W(void)				{return w;}
		const VTYPE &X(void) const	{return x;}
		const VTYPE &Y(void) const	{return y;}
		const VTYPE &Z(void) const	{return z;}
		const VTYPE &W(void) const	{return w;}
		VTYPE *Data (void)			{return &x;}
	};
	
	#undef VNAME
	#undef VTYPE

	// Vec4D Declarations

	#define VNAME		4DD
	#define VTYPE		double

	class HELPAPI Vec4D {
	public:
		VTYPE x, y, z, w;
	
		Vec4D &Set (const float xa, const float ya, const float za)	{ x =xa; y= ya; z=za; w=1; return *this;}
		Vec4D &Set (const float xa, const float ya, const float za, const float wa )	{ x =xa; y= ya; z=za; w=wa; return *this;}

		// Constructors/Destructors
		Vec4D() {x=0; y=0; z=0; w=0;}
		Vec4D (const VTYPE xa, const VTYPE ya, const VTYPE za, const VTYPE wa);
		Vec4D (const Vec3I &op);
		Vec4D (const Vec3F &op);
		Vec4D (const Vec4F &op);
		Vec4D (const Vec4D &op);

		// Member Functions
		Vec4D &operator= (const int op);
		Vec4D &operator= (const double op);
		Vec4D &operator= (const Vec3I &op);
		Vec4D &operator= (const Vec3F &op);
		Vec4D &operator= (const Vec4F &op);
		Vec4D &operator= (const Vec4D &op);

		Vec4D &operator+= (const int op);
		Vec4D &operator+= (const float op);
		Vec4D &operator+= (const double op);
		Vec4D &operator+= (const Vec3I &op);
		Vec4D &operator+= (const Vec3F &op);
		Vec4D &operator+= (const Vec4F &op);
		Vec4D &operator+= (const Vec4D &op);

		Vec4D &operator-= (const int op);
		Vec4D &operator-= (const double op);
		Vec4D &operator-= (const Vec3I &op);
		Vec4D &operator-= (const Vec3F &op);
		Vec4D &operator-= (const Vec4F &op);
		Vec4D &operator-= (const Vec4D &op);

		Vec4D &operator*= (const int op);
		Vec4D &operator*= (const double op);
		Vec4D &operator*= (const Vec3I &op);
		Vec4D &operator*= (const Vec3F &op);
		Vec4D &operator*= (const Vec4F &op);
		Vec4D &operator*= (const Vec4D &op);

		Vec4D &operator*= (const float* op );
		Vec4D &operator*= (const Matrix4F &op);		

		Vec4D &operator/= (const int op);
		Vec4D &operator/= (const double op);
		Vec4D &operator/= (const Vec3I &op);
		Vec4D &operator/= (const Vec3F &op);
		Vec4D &operator/= (const Vec4F &op);
		Vec4D &operator/= (const Vec4D &op);

		// Slow operations - require temporary variables
		Vec4D operator+ (const int op)			{ return Vec4D(x+float(op), y+float(op), z+float(op), w+float(op)); }
		Vec4D operator+ (const float op)		{ return Vec4D(x+op, y+op, z+op, w*op); }
		Vec4D operator+ (const Vec4D &op)	{ return Vec4D(x+op.x, y+op.y, z+op.z, w+op.w); }
		Vec4D operator- (const int op)			{ return Vec4D(x-float(op), y-float(op), z-float(op), w-float(op)); }
		Vec4D operator- (const float op)		{ return Vec4D(x-op, y-op, z-op, w*op); }
		Vec4D operator- (const Vec4D &op)	{ return Vec4D(x-op.x, y-op.y, z-op.z, w-op.w); }
		Vec4D operator* (const int op)			{ return Vec4D(x*float(op), y*float(op), z*float(op), w*float(op)); }
		Vec4D operator* (const float op)		{ return Vec4D(x*op, y*op, z*op, w*op); }
		Vec4D operator* (const Vec4D &op)	{ return Vec4D(x*op.x, y*op.y, z*op.z, w*op.w); }		
		Vec4D operator/ (const int op)			{ return Vec4D(x/float(op), y/float(op), z/float(op), w/float(op)); }
		Vec4D operator/ (const float op)		{ return Vec4D(x/op, y/op, z/op, w/op); }
		Vec4D operator/ (const Vec4D &op)	{ return Vec4D(x/op.x, y/op.y, z/op.z, w/op.w); }		
		// --

		Vec4D &Set ( CLRVAL clr )	{
			x = (float) RED(clr);		// (float( c      & 0xFF)/255.0)	
			y = (float) GRN(clr);		// (float((c>>8)  & 0xFF)/255.0)
			z = (float) BLUE(clr);		// (float((c>>16) & 0xFF)/255.0)
			w = (float) ALPH(clr);		// (float((c>>24) & 0xFF)/255.0)
			return *this;
		}
		Vec4D& fromClr ( CLRVAL clr ) { return Set (clr); }
		CLRVAL toClr () { return (CLRVAL) COLORA( x, y, z, w ); }

		Vec4D& Clamp ( float xc, float yc, float zc, float wc )
		{
			x = (x > xc) ? xc : x;
			y = (y > yc) ? yc : y;
			z = (z > zc) ? zc : z;
			w = (w > wc) ? wc : w;
			return *this;
		}

		Vec4D Cross (const Vec4D &v);	
		
		double Dot (const Vec4D &v);

		double Dist (const Vec4D &v);

		double DistSq (const Vec4D &v);

		Vec4D &Normalize (void);
		double Length (void);

		Vec4D &Random ()		{ x=float(rand())/RAND_MAX; y=float(rand())/RAND_MAX; z=float(rand())/RAND_MAX; w = 1;  return *this;}

		VTYPE &X(void)				{return x;}
		VTYPE &Y(void)				{return y;}
		VTYPE &Z(void)				{return z;}
		VTYPE &W(void)				{return w;}
		const VTYPE &X(void) const	{return x;}
		const VTYPE &Y(void) const	{return y;}
		const VTYPE &Z(void) const	{return z;}
		const VTYPE &W(void) const	{return w;}
		VTYPE *Data (void)			{return &x;}
	};
	
	#undef VNAME
	#undef VTYPE

		// Vector8S Declaration - 8x short ints

	#define VNAME		8S
	#define VTYPE		unsigned short

	class HELPAPI Vec8S {
	public:
		VTYPE x,y,z,w,x2,y2,z2,w2;
	
		// Constructors/Destructors
		Vec8S()								{ x=0;y=0;z=0;w=0;x2=0;y2=0;z2=0;w2=0; }		
		Vec8S(VTYPE c0, VTYPE c1, VTYPE op)	{ x=c0; y=op;z=op;w=op; x2=c1; y2=op;z2=op;w2=op; }
		Vec8S(VTYPE op)						{  for (int i=0; i<8; i++) ((VTYPE*) &x)[i] = op; }
		Vec8S(const Vec4F& c, VTYPE op)		{ x=c.x; y=c.y; z=c.z; w=c.w; x2=op;y2=op;z2=op;w2=op; }	
		Vec8S(const Vec4F& c)							{ x = c.x; y = c.y; z = c.z; w = c.w; x2 = 0; y2 = 0; z2 = 0; w2 = 0; }

		Vec8S& Set (VTYPE op)				{ x=op;y=op;z=op;w=op;x2=op;y2=op;z2=op;w2=op; return *this;}
		Vec8S& Set (int i, VTYPE op)			{ ((VTYPE*) &x)[i] = op; return *this;}
		
		Vec8S& operator= (const Vec4F& op)  { x=op.x; y=op.y; z=op.z; w=op.w; x2=0;y2=0;z2=0;w2=0; return *this;}

		Vec8S &operator= (const int op)			{ for (int i=0; i<8; i++) ((VTYPE*) &x)[i] = op; return *this;}
		Vec8S &operator= (const Vec8S &op)	{ for (int i=0; i<8; i++) ((VTYPE*) &x)[i] = ((VTYPE*) &op.x)[i]; return *this;}
		Vec8S &operator+= (const Vec8S &op)	{ for (int i=0; i<8; i++) ((VTYPE*) &x)[i] += ((VTYPE*) &op.x)[i]; return *this;}
		Vec8S &operator-= (const Vec8S &op)	{ for (int i=0; i<8; i++) ((VTYPE*) &x)[i] -= ((VTYPE*) &op.x)[i]; return *this;}
		Vec8S &operator*= (const Vec8S &op)	{ for (int i=0; i<8; i++) ((VTYPE*) &x)[i] *= ((VTYPE*) &op.x)[i]; return *this;}
		Vec8S &operator/= (const Vec8S &op)	{ for (int i=0; i<8; i++) ((VTYPE*) &x)[i] /= ((VTYPE*) &op.x)[i]; return *this;}

		VTYPE get (int i)						{ return ((VTYPE*) &x)[i]; }
		VTYPE *getData ()						{ return &x;}		
	};
	
	#undef VNAME
	#undef VTYPE

#endif

#ifndef MATRIX_DEF
	#define MATRIX_DEF
		
	#include <stdio.h>
	#include <iostream>
	#include <memory.h>
	#include <math.h>
	#include <string>

	// Matrix4F Declaration	
	#define VNAME		F
	#define VTYPE		float

	class HELPAPI Matrix4F {
	public:	
		VTYPE	data[16];		

		// Constructors/Destructors
		Matrix4F ( const float* dat );
		Matrix4F () { for (int n=0; n < 16; n++) data[n] = 0.0; }
		Matrix4F ( float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11,	float f12, float f13, float f14, float f15 );

		// Member Functions
		VTYPE &operator () (const int n)					{ return data[n]; }
		VTYPE &operator () (const int c, const int r)	{ return data[ (r<<2)+c ]; }		
		Matrix4F &operator= (const unsigned char c);
		Matrix4F &operator= (const int c);
		Matrix4F &operator= (const double c);				
		Matrix4F &operator=  (const float* op);
		Matrix4F &operator=  (const Matrix4F& op);		// copy
		
		Matrix4F &operator+= (const unsigned char c);
		Matrix4F &operator+= (const int c);
		Matrix4F &operator+= (const double c);				
		Matrix4F &operator+= (const Vec3F& t);		// quick translate
		Matrix4F &operator-= (const unsigned char c);
		Matrix4F &operator-= (const int c);
		Matrix4F &operator-= (const double c);
		Matrix4F &operator*= (const unsigned char c);
		Matrix4F &operator*= (const int c);
		Matrix4F &operator*= (const double c);	
		Matrix4F &operator*= (const float* op);	
		Matrix4F &operator*= (const Vec3F& t);		// quick scale
		Matrix4F& operator/= (const unsigned char c);
		Matrix4F& operator/= (const int c);
		Matrix4F& operator/= (const double c);
		
		// matrix multiply		
		Matrix4F  operator* (const Matrix4F& op);
		Matrix4F& Multiply (const Matrix4F& a, const Matrix4F& b);	
		Matrix4F& operator*= (const Matrix4F& op);	
		
		// identity
		Matrix4F &Identity ();
		Matrix4F &Identity (const int order);

		Matrix4F &Transpose (void);

		// compose transforms
		Matrix4F& Translate(const Vec3F& vec);
		Matrix4F& Rotate(const Matrix4F& mtx);
		Matrix4F& Rotate(const Vec3F& vec);
		Matrix4F& Scale(const Vec3F& vec);
		Matrix4F& InvTranslate(const Vec3F& vec);
		Matrix4F& InvRotate(Matrix4F mtx);
		Matrix4F& InvScale(const Vec3F& vec);			

		Matrix4F &Scale (double sx, double sy, double sz);
		Matrix4F &RotateYZX (const Vec3F& angs);		// roll,pitch,yaw when Y-axis is up
		Matrix4F &RotateZYX ( const Vec3F& angs );
		Matrix4F &RotateZYXT (const Vec3F& angs, const Vec3F& t);
		Matrix4F &RotateTZYX (const Vec3F& angs, const Vec3F& t);
		Matrix4F &RotateTZYXS(const Vec3F& angs, const Vec3F& t, const Vec3F& s);
		Matrix4F &RotateX (const double ang);
		Matrix4F &RotateY (const double ang);
		Matrix4F &RotateZ (const double ang);
		Matrix4F &Ortho (double sx, double sy, double n, double f);		
		Matrix4F &Translate (double tx, double ty, double tz);
		Matrix4F &PreTranslate (const Vec3F& t);
		Matrix4F &PostTranslate (const Vec3F& t);
		Matrix4F &SetTranslate(const Vec3F& t);		
		
		Vec3F getTranslation ()		{ return Vec3F(data[12], data[13], data[14]); }
		
		void Print ();
		std::string WriteToStr ();

		Matrix4F operator* (const float &op);	
		Matrix4F operator* (const Vec3F &op);	
		Matrix4F operator+ (const Matrix4F &op);	
		Matrix4F &operator+= (const Matrix4F &op);
		
		// Basis transforms
		Matrix4F& normalizedBasis(const Vec3F& fwd);
		Matrix4F& toBasis(const Vec3F& c1, const Vec3F& c2, const Vec3F& c3);
		Matrix4F& toBasisInv(const Vec3F& c1, const Vec3F& c2, const Vec3F& c3);

		// Translate, Rotate, Scale (TRS) - applied right-to-left
		Matrix4F& TRST(Vec3F pos, Quaternion r, Vec3F s, Vec3F piv);				// scale -> rotate -> translate
		Matrix4F& ReverseTRS (Vec3F& pos, Quaternion& quat, Vec3F& scal);		// get back pos, quat, scal

		// Scale-Rotate-Translate (compound matrix)
		Matrix4F &TransSRT (const Vec3F &c1, const Vec3F &c2, const Vec3F &c3, const Vec3F& t, const Vec3F& s);
		Matrix4F& InvertTRS();
		Matrix4F &SRT (const Vec3F &c1, const Vec3F &c2, const Vec3F &c3, const Vec3F& t, const Vec3F& s);
		Matrix4F &SRT (const Vec3F &c1, const Vec3F &c2, const Vec3F &c3, const Vec3F& t, const float s);

		// invTranslate-invRotate-invScale (compound matrix)
		Matrix4F &InvTRS (const Vec3F &c1, const Vec3F &c2, const Vec3F &c3, const Vec3F& t, const Vec3F& s);
		Matrix4F &InvTRS (const Vec3F &c1, const Vec3F &c2, const Vec3F &c3, const Vec3F& t, const float s);

		Matrix4F &operator= ( float* mat);
		Matrix4F &InverseProj ( const float* mat );
		Matrix4F &InverseView ( const float* mat, const Vec3F& pos );
		Vec4F GetT ( float* mat );

		Matrix4F& makeLookAt ( Vec3F eye, Vec3F target, Vec3F up );
		Matrix4F& makeLookAt ( Vec3F eye, Vec3F target, Vec3F up, Vec3F& xaxis, Vec3F& yaxis, Vec3F& zaxis);
		Matrix4F& makeOrtho (float left, float right, float bottom, float top, float minz, float maxz);
		Matrix4F& makeOrthogonalInverse (Matrix4F& src);
		Matrix4F& makeInverse3x3 (Matrix4F& s );
		Matrix4F& makeRotationMtx (Matrix4F& s );
		Matrix4F& makeRotationInv (Matrix4F& s );
		
		Matrix4F Inverse(Matrix4F& m );				// inverse of the matrix
		Matrix4F Transpose(Matrix4F& m );				// transpose of the matrix
		Vec3F getTrans ()	{ return Vec3F( data[12], data[13], data[14] ); }

		int GetX()			{ return 4; }
		int GetY()			{ return 4; }
		int GetRows(void)	{ return 4; }
		int GetCols(void)	{ return 4; }	
		int GetLength(void)	{ return 16; }
		VTYPE *GetData(void)	{ return data; }
		Vec4F GetRowVec(int r);

		unsigned char *GetDataC (void) const	{return NULL;}
		int *GetDataI (void)	const			{return NULL;}
		float *GetDataF (void) const		{return (float*) data;}

		float GetF (const int r, const int c);
	};

	#undef VNAME
	#undef VTYPE


	// MatrixF Declaration	
	#define VNAME		F
	#define VTYPE		float

	class HELPAPI MatrixF {
	public:	
		VTYPE *data;
		int rows, cols, len;		

		// Constructors/Destructors		
		 MatrixF ();
		 ~MatrixF ();
		 MatrixF (const int r, const int c);

		// Member Functions
		 VTYPE GetVal ( int c, int r );
		 VTYPE &operator () (const int c, const int r);
		 MatrixF &operator= (const unsigned char c);
		 MatrixF &operator= (const int c);
		 MatrixF &operator= (const double c);		
		 MatrixF &operator= (const MatrixF &op);
		
		 MatrixF &operator+= (const unsigned char c);
		 MatrixF &operator+= (const int c);
		 MatrixF &operator+= (const double c);		
		 MatrixF &operator+= (const MatrixF &op);

		 MatrixF &operator-= (const unsigned char c);
		 MatrixF &operator-= (const int c);
		 MatrixF &operator-= (const double c);		
		 MatrixF &operator-= (const MatrixF &op);

		 MatrixF &operator*= (const unsigned char c);
		 MatrixF &operator*= (const int c);
		 MatrixF &operator*= (const double c);				
		 MatrixF &operator*= (const MatrixF &op);		

		 MatrixF &operator/= (const unsigned char c);
		 MatrixF &operator/= (const int c);
		 MatrixF &operator/= (const double c);				
		 MatrixF &operator/= (const MatrixF &op);

		 MatrixF &Multiply4x4 (const MatrixF &op);
		 MatrixF &Multiply (const MatrixF &op);
		 MatrixF &Resize (const int x, const int y);
		 MatrixF &ResizeSafe (const int x, const int y);
		 MatrixF &InsertRow (const int r);
		 MatrixF &InsertCol (const int c);
		 MatrixF &Transpose (void);
		 MatrixF &Identity (const int order);
		 MatrixF &RotateX (const double ang);
		 MatrixF &RotateY (const double ang);
		 MatrixF &RotateZ (const double ang);
		 MatrixF &Ortho (double sx, double sy, double n, double f);		
		 MatrixF &Translate (double tx, double ty, double tz);
		 MatrixF &Basis (const Vec3F &c1, const Vec3F &c2, const Vec3F &c3);
		 MatrixF &GaussJordan (MatrixF &b);
		 MatrixF &ConjugateGradient (MatrixF &b);
		 MatrixF &Submatrix ( MatrixF& b, int mx, int my);
		 MatrixF &MatrixVector5 (MatrixF& x, int mrows, MatrixF& b );
		 MatrixF &ConjugateGradient5 (MatrixF &b, int mrows );
		 double Dot ( MatrixF& b );

		 void Print ( char* fname );

		 int GetX();
		 int GetY();	
		 int GetRows(void);
		 int GetCols(void);
		 int GetLength(void);
		 VTYPE *GetData(void);
		 void GetRowVec (int r, Vec3F &v);

		 unsigned char *GetDataC (void) const	{return NULL;}
		 int *GetDataI (void)	const			{return NULL;}
		 float *GetDataF (void) const		{return data;}

		 float GetF (const int r, const int c);
	};
	#undef VNAME
	#undef VTYPE

#endif
