//--------------------------------------------------------------------------------
// Copyright 2019-2022 (c) Quanta Sciences, Rama Hoetzlein, ramakarl.com
//
// 
// * Derivative works may append the above copyright notice but should not remove or modify earlier notices.
//
// MIT License:
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, including without 
// limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
#ifndef DEF_DATAPTR_H
	#define DEF_DATAPTR_H

	#include "common_defs.h"
	#include "common_cuda.h"
	#include <assert.h>
	#include <vector>
	#include <string>

	#ifdef USE_CUDA
		#include "cuda.h"	
		#define PUSH_CTX		cuCtxPushCurrent(cuCtx);
		#define POP_CTX			CUcontext pctx; cuCtxPopCurrent(&pctx);
	#else
		#define PUSH_CTX
		#define POP_CTX		
	#endif

	#define DT_MISC			0
	#define DT_NONE			0
	#define DT_UCHAR		1		// 8-bit
	#define DT_USHORT		2		// 16-bit,  1 chan @ 16-bit
	#define DT_UCHAR3		3		// 24-bit,  3 chan @  8-bit
	
	#define DT_UCHAR4		4		// 32-bit,  4 chan @  8-bit
	#define DT_INT			5		// 32-bit,  1 chan @ 32-bit
	#define DT_UINT			6		// 32-bit,  1 chan @ 32-bit 
	#define DT_FLOAT		7		// 32-bit,  1 chan @ 32-bit (float)

	#define DT_USHORT3	8		//  48-bit, 3 chan @ 16-bit
	#define DT_UINT64		9		//  64-bit, 1 chan @ 64-bit
	#define DT_FLOAT3		12		//  96-bit, 3 chan @ 32-bit (float)
	#define DT_FLOAT4		16	    // 128-bit, 4 chan @ 32-bit (float)

	
	#define DT_CPU			1		// use flags
	#define DT_CUMEM		2
	#define DT_CUARRAY		4
	#define DT_CUINTEROP	8
	#define DT_GLTEX		16	
	#define DT_GLVBO		32

	HELPAPI int getTypeSize(uchar dtype);

	class HELPAPI DataPtr {
	public:
		DataPtr() { mNum=0; mMax=0; mStride=0; mUseRX=0; mUseRY=0; mUseRZ=0; mUseType=DT_MISC; mUseFlags=DT_MISC; 
					mSize=0; mCpu=0; mGLID=-1;
					#ifdef USE_CUDA
						mGpu=0; mGrsc=0; mGarray=0; mGtex = -1; mGsurf = -1; 
					#endif			
					}
		~DataPtr();

		
		// Buffer operations
		void			Resize ( int stride, uint64_t cnt, char* dat=0x0, uchar dest_flags=DT_CPU );
		int				Append ( int stride, uint64_t cnt, char* dat=0x0, uchar dest_flags=DT_CPU );
		void			UseMax ()	{ mNum = mMax; }
		void			SetUsage ( uchar dt, uchar flags=DT_MISC, int rx=-1, int ry=-1, int rz=-1 );		// special usage (2D,3D,GLtex,GLvbo,etc.)
		void			UpdateUsage ( uchar flags );		
		void			ReallocateCPU ( uint64_t oldsz, uint64_t newsz );
		void			FillBuffer ( uchar v );
		void			CopyTo ( DataPtr* dest, uchar dest_flags );
		void			Commit ();		
		void			Retrieve ();	
		bool			Map();
		bool			Unmap();
		void			Clear ();

		// Data access
		int				getUsage ()		{ return mUseType; }				
		uint64_t		getDataSz ( int cnt, int stride )	{ return (uint64_t) cnt * stride; }
		int				getNum()	{ return mNum; }
		int				getMax()	{ return mMax; }
		char*			getData()	{ return mCpu; }
		#ifdef USE_CUDA
			CUdeviceptr		getGPU()	{ return mGpu; }		
		#endif
		void			SetElem(uint64_t n,  void* dat)	{ memcpy ( mCpu+n*mStride, dat, mStride); }
		char*			getPtr(uint64_t n)		{ return mCpu + n*mStride; }		

		// Multi-dimensional Get/Set
		char*			getPtr(uint64_t x, uint64_t y, uint64_t z) {
									return mCpu + ((z*mUseRY + y)*mUseRX + x) * mStride; }

		// Helper functions		
		void			SetElemInt(uint64_t n, int val)	{ * (int*) (mCpu+n*mStride) = val; }
		int				getElemInt(uint64_t n)			{ return * (int*) (mCpu+n*mStride); }

	public:
		uint64_t		mNum=0, mMax=0, mSize=0;
		int				mStride=0;
		uchar			mRefID=0, mUseType=0, mUseFlags=0;	// usage
		int				mUseRX=0, mUseRY=0, mUseRZ=0;
		bool			bCpu=false, bGpu=false;
		char*			mCpu=NULL;
		
		int				mGLID=-1;						// OpenGL

		#ifdef USE_CUDA
		  // CUDA data storage
			CUdeviceptr		mGpu;					// CUDA
			CUgraphicsResource	mGrsc;	// CUDA-GL interop
			CUarray				mGarray;	
			CUtexObject		mGtex;				// CUDA texture/surface interop
			CUsurfObject	mGsurf;
		#else
			// ensure DataPtr is same size even when CUDA disabled
			devdata_t			mGpu;
			devdata_t			mGrsc;
			devdata_t			mGarray;
			devdata_t			mGtex;
			devdata_t			mGsrf;
		#endif

		static int		mFBO;
	};

#endif




