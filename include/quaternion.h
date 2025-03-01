// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h 

/* 
 This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Please note that the Irrlicht Engine is based in part on the work of the 
  Independent JPEG Group, the zlib and the libPng. This means that if you use
  the Irrlicht Engine in your product, you must acknowledge somewhere in your
  documentation that you've used the IJG code. It would also be nice to mention
  that you use the Irrlicht Engine, the zlib and libPng. See the README files 
  in the jpeglib, the zlib and libPng for further informations.
  
*NOTES*:
  2021 (C) Rama Hoetzlein - Significant changes made to this file
  - This includes corrections to slerp, conjugate and inverse
  - New functions for log, exp and basis
  - Dual quaternions
  - Distribution terms are same as original and must include this notice

*/


#ifndef QUATERNION_H
	#define QUATERNION_H

	#include "common_defs.h"
	#include "vec.h"
	//#include "matrix.h"


	#define Q_EPS		0.00001

	// Quaternion
	// - concisely express an orientation
	// - or a rotation transformation
	class HELPAPI Quaternion {
	public:
			//! Default Constructor
			Quaternion() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f) {}

			//! Constructor
			inline Quaternion(f32 x, f32 y, f32 z, f32 w)		{ set(x, y, z, w); }
			inline Quaternion( Vec4F v )					{ set(v.x, v.y, v.z, v.w); }	// copy from vec4

			//! Constructor which converts euler angles (radians) to a Quaternion
			inline Quaternion(f32 x, f32 y, f32 z)				{ set(x, y, z); }		
			inline void fromEuler (Vec3F angs)				    { set( angs ); }		// DEGtoRAD included

			//! Constructor which converts euler angles (radians) to a Quaternion
			inline Quaternion(const Vec3F& angs)			{ set(angs.x, angs.y, angs.z); }
			inline Quaternion(const Vec3F& vec, float w )	{ set(vec.x, vec.y, vec.z, w); }

			//! Constructor which converts a matrix to a Quaternion
			inline Quaternion(const Matrix4F& mat)				{ (*this) = mat; }

			//! Equalilty operator
			inline bool operator==(const Quaternion& other) const	{ return ((X == other.X) && (Y == other.Y) && (Z == other.Z) && (W == other.W)); }
			bool equals(const Quaternion& other, const f32 tolerance = ROUNDING_ERROR_f32) const;

			//! inequality operator
			inline bool operator!=(const Quaternion& other) const	{ return !(*this == other); }			

			//! Assignment operator
			inline Quaternion& operator=(const Quaternion& other)	{ X = other.X; Y = other.Y; Z = other.Z; W = other.W; return *this; }

			// Mathematical operators
			// Rotation: A * B >> apply global rotation A to B
			//           B * A >> apply local rotation A to B
			Quaternion& operator=(const Matrix4F& other);				// Matrix assignment operator
			Quaternion operator+(const Quaternion& other) const;		// Add operator
			Quaternion operator*(const Quaternion& other) const;		// Multiplication operator
			Quaternion& operator*=(const Quaternion& other);			// Multiplication. 
			Quaternion operator*(f32 s) const;							// Multiplication operator with scalar
			Quaternion& operator*=(f32 s);								// Multiplication operator with scalar
			Vec3F operator*(const Vec3F& v) const;				// Multiplication with vector, giving rotated vector			
			f32 dotProduct(const Quaternion& other) const;				// Dot product of two quaternions

			// Set operators
			inline Quaternion& set(f32 x, f32 y, f32 z, f32 w)	{ X = x; Y = y; Z = z; W = w; return *this; }
			inline Quaternion& set(const Quaternion& quat)		{ return (*this = quat); }
			Quaternion& set(f32 x, f32 y, f32 z);						// Sets new Quaternion based on euler angles (radians)
			Quaternion& set(const Vec3F& vec);						// Sets new Quaternion based on euler angles (radians)
			inline Quaternion& Identity() { W = 1.f; X = 0.f; Y = 0.f; Z = 0.f; return *this; } // Set identity
			Quaternion identity() { return Quaternion(0,0,0,1.f); }

			// Fix Quaternion within rounding tolerance
			void fixround();

			//! Normalizes the Quaternion
			Quaternion& normalize();


			// Get operators
			Matrix4F getMatrix() const;									// Creates a matrix from this Quaternion
			void getMatrix( Matrix4F &dest, const Vec3F &translation=Vec3F() ) const;	// Creates a matrix from this Quaternion
			void getMatrixCenter( Matrix4F &dest, const Vec3F &center, const Vec3F &translation ) const;			
			void getMatrix_transposed( Matrix4F &dest ) const;			// Creates a matrix from this Quaternion

			// Inverse operators 
			Quaternion inverse ();										// q^-1 = inverse = <-qxyz, qw>/||q||
			Quaternion conjugate ();									// q* = conjugate = <-qxyz, qw>
			Quaternion negative() { return Quaternion(X, Y, Z, -W); }	// -q = negative quaternion (-W)
			Quaternion dual(Vec3F pos);							// dual portion of a dual quaternion
			
			// Construct a quaternion..
			Quaternion& fromBasis (Vec3F c1, Vec3F c2, Vec3F c3 );		// From orthonormal basis
			Quaternion& fromAngleAxis (f32 angle, const Vec3F& axis);			// From rotation angle and axis			
			Quaternion& fromRotationFromTo(Vec3F from, Vec3F to, float frac=1.0);	// From a rotation from one vector to another
			Quaternion& fromDirectionAndUp ( Vec3F fwd, Vec3F up );			// From a direction and up vector
			Quaternion& fromDirectionAndRoll ( Vec3F fwd, float roll );			// From a direction and roll angle
			Quaternion& changeAngle (f32 angle);
			
			// Linear interpolation - Set this Quaternion to the linear interpolation 
			// q1 = First Quaternion, q2 = Second quaternion, time = parametric value from 0 to 1
			Quaternion& lerp(Quaternion q1, Quaternion q2, f32 time);

			// Spherical interpolation - Set this Quaternion to the result of the spherical interpolation 
			// q1 = First Quaternion, q2 = Second quaternion, time = parametric value from 0 to 1
			// threshold = avoid inaccuracies at the end (time=1) the interpolation switches to linear interpolation 
			// at some point. This value defines how much of the remaining interpolation will be calculated with lerp. 
			// Everything from 1-threshold up will be linear interpolation.
			Quaternion slerp(Quaternion q1, Quaternion q2, f32 time, f32 threshold=.05f);

			// Fills an angle (radians) around an axis (unit vector)
			void toAngleAxis (f32 &angle, Vec3F& axis) const;
			inline f64 getAngle ()	{ return (W>1.0) ? 0 : 2.0f * acosf(W) * RADtoDEG; }			
			Vec3F getAxis()		{ return Vec3F(X,Y,Z); }

			// Output this Quaternion to an euler angle (radians)
			void toEuler(Vec3F& euler);

			// Rotate a vector by this Quaternion
			Vec3F rotateVec(Vec3F v);

			// Set Quaternion to represent a rotation from one vector to another.
			void rotationFromTo(Vec3F from, Vec3F to);
			
			// Logarithm & Exp of quaternion
			Quaternion log ();
			Quaternion unit_log ();		// faster
			Quaternion exp ();
			Quaternion unit_exp ();		// faster

			// Quaternion elements.
			f32 X;	// vectorial (imaginary) part
			f32 Y;
			f32 Z;
			f32 W;	// real part
	};

	// Dual Quaternion
	// - whereas quaternions express orientation/rotation
	// - dual quats can express position and orientation, the..
	// - complete affine transform of an object, but easily separated into pos & rotation
	class Dualquat {
	public:
		inline Dualquat() { m_real.set(0, 0, 0, 1); m_dual.set(0, 0, 0, 0); }
		inline Dualquat(Quaternion real, Quaternion dual) {			// set to another Dualquat
			m_real = real;
			m_dual = dual;
		}
		inline void zero()		{ m_real.set(0, 0, 0, 0); m_dual.set(0, 0, 0, 0); }
		inline void identity()	{ m_real.set(0, 0, 0, 1); m_dual.set(0, 0, 0, 0); }
		
		Dualquat(Vec3F pos) {				// set from position
			m_real.set(0, 0, 0, 1);
			m_dual = m_real.dual(pos);			// already normalized		
		}
		Dualquat(Quaternion rot, Vec3F pos) { // set from a rotation and position
			m_real = rot.normalize();
			m_dual = m_real.dual(pos);			// already normalized		
		}
		Dualquat operator* (float f) {
			return Dualquat(m_real * f, m_dual * f);
		}
		Dualquat operator* (Dualquat& op) {
			return Dualquat(op.m_real * m_real, op.m_dual * m_real + op.m_real * m_dual);
		}
		Dualquat operator+ (Dualquat& op) {
			return Dualquat(m_real + op.m_real, m_dual + op.m_dual);
		}
		Dualquat inverse() {									// invert the dualquat
			return Dualquat(m_real.inverse(), m_dual.inverse());
		}
		Dualquat& normalize() {
			float mag = m_real.dotProduct(m_real);
			if (mag < 0.0000001f) return *this;
			m_real *= 1.0f / mag;
			m_dual *= 1.0f / mag;
			return *this;
		}
		// return the orientation (as a quaternion)
		Quaternion getRotate() { return m_real; }

		// return the translation (as a vector)
		Vec3F getTranslate() { Quaternion t = (m_dual * 2.0f) * m_real.inverse(); return Vec3F(t.X, t.Y, t.Z); }

		// Dualquat elements.
		Quaternion m_real;
		Quaternion m_dual;
	};

#endif
