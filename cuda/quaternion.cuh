//
// Quaternions for GPU
// Copyright (c) 2023. Rama Hoetzlein
// MIT license
//

#ifndef QUAT_GPU_H
	#define QUAT_GPU_H

	#include "cutil_math.h"	

	typedef float4		quat4;

	#define feql(a, b, eps)		  (fabs(a - b) < eps)
	#define fsign(x)						(x >= 0 ? 1 : -1)
	#define fmodulus(x,y)       (x - trunc(x/y) * y)

	//#define PI									3.14159265358979f
	#define DEGtoRAD						(3.14159265358979f/180.0f)
  #define RADtoDEG						(180.0f/3.14159265358979f)

	

  inline __device__ __host__  float3 quat_mult ( float3 v, quat4 op )
	{
		float3 u, q;
		u = make_float3( op.x, op.y, op.z );
		q = u * float(2.0f * dot(u,v) );			// q = u * 2*(u . v) + v * (W*W - u . u)
		q += v * (op.w * op.w - dot(u,u) );	
		q += cross(u, v) * 2.0f * op.w;
		return q;
	}

	inline __device__ __host__ quat4 quat_mult ( quat4 b, quat4 a )
	{
		quat4 q;
		q.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z);
		q.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y);
		q.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z);
		q.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x);
		return q;
	}

	inline __device__ __host__ quat4 quat_normalize ( quat4 a )
	{
		float n = a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
		//if ( fabs(n) < 0.00001 ) return a;
		a *= 1.0f / sqrt(n);		
		return a;
	}


	inline __device__ __host__ quat4 quat_from_basis (float3 a, float3 b, float3 c)
	{
		quat4 q;
		float T = a.x + b.y + c.z;
		float s;
		if (T > 0) {
			float s = sqrt(T + 1) * 2.f;
			q.x = (b.z - c.y) / s;
			q.y = (c.x - a.z) / s;
			q.z = (a.y - b.x) / s;
			q.w = 0.25f * s;
		} else if ( a.x > b.y && a.x > c.z) {
			s = sqrt(1 + a.x - b.y - c.z) * 2;
			q.x = 0.25f * s;
			q.y = (a.y + b.x) / s;
			q.z = (c.x + a.z) / s;
			q.w = (b.z - c.y) / s;
		} else if (b.y > c.z) {
			s = sqrt(1 + b.y - a.x - c.z) * 2;
			q.x = (a.y + b.x) / s;
			q.y = 0.25f * s;
			q.z = (b.z + c.y) / s;
			q.w = (c.x - a.z) / s;
		} else {
			s = sqrt(1 + c.z - a.x - b.y) * 2;
			q.x = (c.x + a.z) / s;
			q.y = (b.z + c.y) / s;
			q.z = 0.25f * s;
			q.w = (a.y - b.x) / s;
		}
		quat_normalize( q );
		return q;
	}

	inline __device__ __host__  float3 quat_to_euler ( quat4 op )
	{
		float3 v;
		const float test = (op.x * op.y + op.z * op.w);

	  if ( feql(test, 0.5, 0.00001) ) {
			v.x = 0;							// roll = rotation about x-axis (bank)
			v.y = (PI / 2.0);			// pitch = rotation about y-axis (attitude)
			v.z = (-2.0 * atan2(op.x, op.w));		// yaw = rotation about z-axis (heading)		
		
		}	else if ( feql(test, -0.5, 0.00001)) {

			// heading = rotation about z-axis
			v.z = (2.0 * atan2(op.x, op.w));
			// bank = rotation about x-axis
			v.x = 0;
			// attitude = rotation about y-axis
			v.y = (PI / -2.0);

		} else	{

			v.x = atan2 (2.0 * (op.x * op.w - op.y * op.z), 1.0 - 2.0f * (op.x * op.x + op.z * op.z));	// roll = rotation about x-axis (bank)
			v.y = asin  (2.0 * test);								// pitch = rotation about y-axis (attitude)
			v.z = atan2 (2.0 * (op.x * op.z - op.y * op.w), 1.0 - 2.0f * (op.y * op.y + op.z * op.z));	// yaw = rotation about z-axis (heading)
	  }

		v.x *= RADtoDEG;
		v.y *= RADtoDEG;
		v.z *= RADtoDEG;

		return v;
	}

	inline __device__ __host__ quat4 quat_from_angleaxis ( float ang, float3 axis )
	{
		quat4 q;
		ang *= 0.5f;
		float fSin = sinf(ang);
		q.w = cosf(ang);
		q.x = fSin * axis.x;
		q.y = fSin * axis.y;
		q.z = fSin * axis.z;
		return q;
	}
	

	inline __device__ __host__ quat4 quat_rotation_fromto (float3 from, float3 to, float frac)
  {
	  float3 axis = to;	
	  axis = normalize ( cross(from, to ) );										// axis of rotation between vectors (from and to unmodified)	  
		return normalize ( quat_from_angleaxis ( acos( dot(from,to) )*frac, axis) );		// dot product = angle of rotation between 'from' and 'to' vectors	
  }

	inline __device__ __host__ quat4 quat_from_directionup (float3 fwd, float3 up)
  {
	  float3 side;
		fwd = normalize(fwd);
		side = normalize ( cross( fwd, up ) );
		up = normalize ( cross ( side, fwd ) );
		return quat_from_basis ( fwd, up, side );
  }


	inline __device__ __host__ quat4 quat_inverse ( quat4 op )
	{
		quat4 q = op;
		q *= -1.0 / sqrt(op.x*op.x + op.y*op.y + op.z*op.z + op.w*op.w);
		q.w *= -1.0;
		return q;
	}	

#endif
