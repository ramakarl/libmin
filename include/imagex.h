//--------------------------------------------------------------------------------
// Copyright 2007-2022 (c) Quanta Sciences, Rama Hoetzlein, ramakarl.com
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
#ifndef DEF_IMAGE
	#define DEF_IMAGE

	#include <string>	
	#include "common_defs.h"					
	#include "vec.h"
	#include "dataptr.h"

	class CImageFormat;

	HELPAPI void addImageFormat ( CImageFormat* fmt );

	class HELPAPI ImageOp {
	public:
		enum Format {			
			FmtNone = 0,
			BW1 = 1,
			BW8,
			BW16,
			BW32,
			RGB8,
			RGBA8,
			RGB12,
			RGB16,			
			RGBA16,			
			RGBA32F,
			BGR8,
			I420,
			IYUV,
			F32,		// full float
			F16,		// half
			Custom
		};
		enum Filter {
			NoFilter = 0,
			Linear = 1,
			MipNoFilter = 2,
			MipLinear = 3
		};
		enum Usage {
			Light = 0,
			Medium = 1,
			Heavy = 2
		};
		enum Flags {
			// Format flags
			Color = 1,			// BW or color?
			Masked = 2,			// 0-value is mask?
			Alpha = 4,			// Alpha channel?
			AlphaMerged = 8,	// Alpha channel is merged?
			Depth = 16,			// Depth channel?
			DepthMerged = 32,	// Depth channel is merged?
			Indexed = 64,		// Indexed pixels?
			Lowclr = 128,			// Low-color pixels (12-bit)?
			Highclr = 256,			// High-color pixels (24-bit)?
			Trueclr = 512,			// True-color pixels (32-bit)?
			// Extended usage flags
			Channels = 1024,	// Other channels present?
			FilterLo = 2048,	// Linear filter?
			FilterHi = 4096,	// Mipmap filter?
			UseLo = 8192,		// Usage method
			UseHi = 16384,
			Cube = 32768		// Cube map?
		};
		enum ImageStatus {
			Ready = 0,
			NeedsUpdate = 1,
			NeedsRebuild = 2,
			NotReady = 3
		};
		enum FormatStatus {
			Idle = 0,
			Loading = 1,
			Saving = 2,
			Successs = 3,			// Yesss - the extra 's' is necesssary because gcc uses the 'Success' keyword.
			DepthNotSupported = 4,
			FeatureNotSupported = 5,
			InvalidFile = 6,
			LibVersion = 7,
			FileNotFound = 8,
			NotImplemented = 9,			
			LoadOk = 10,
			LoadDone = 11, 
			LoadNotReady = 12
		};		
	};

	// Image Format Params
	struct ImageFormatParams {							
		int		iQuality;	// JPEG Quality. 0 to 100
		float	fScaling;	// 1/2,1/4,1/8,1
	};

	class HELPAPI ImageX {
	public:
		ImageX ();
		ImageX ( int xr, int yr, ImageOp::Format fmt, uchar use_flags=DT_CPU );
		ImageX ( std::string name, int xr, int yr, ImageOp::Format fmt );
		~ImageX ();
		void Clear ();
		
		virtual bool Load(std::string fname) {
			std::string errmsg;
			bool result = Load(fname, errmsg);
			if (!result) dbgprintf("ERROR ImageX::Load: %s\n", errmsg.c_str());
			return result;
		}

		// Image Loading & Saving
		bool Load(char* filename, char* alphaname);
		bool Load(std::string filename, std::string& errmsg);		
		bool LoadAlpha(char* filename);
		bool LoadIncremental(char* filename);
		ImageOp::FormatStatus LoadNextRow();
		bool Save(char* filename);								// Save Image
		void SetupFormats();

		//--- format-specific load/save (not supported)
		//bool LoadPng ( char* fname, bool bGrey=false );
		//bool LoadTga ( char* fname );
		//bool SavePng ( char* fname );

		// Image Creation, Resizing & Reformatting				
		void Create ();
		void Create ( int xr, int yr, ImageOp::Format eFormat, uchar use_flags=0 );		// Create image		
		void Resize ( int xr, int yr );
		void Resize ( int xr, int yr, ImageOp::Format eFormat, uchar use_flags=0 );
		void ResizeChannel ( int chan, int xr, int yr, ImageOp::Format eFormat, uchar use_flags=0 );
		void AddChannel ( std::string name, int xr, int yr, ImageOp::Format eFormat );		
		// void ChangeFormat (ImageOp::Format eFormat);
		void DeleteBuffers ();
		void CopyIntoBuffer ( DataPtr& dest, DataPtr& src, int bpp, int w, int h );
		void FlipY ();
		void CopyToAlpha ();

		// GPU
		void SetUsage ( uchar use_flags );		
		void Commit (uchar use_flags = 0);
		void Map();
		void Unmap();
		void Retrieve();
		int getGLID() { 
			if ( !(m_Pix.mUseFlags & DT_GLTEX) ) {
				printf ( "ERROR: Image not allocated as DT_GLTEX.\n");		// <-- breakpoint here to debug
				return -1;
			}
			if ( m_Pix.mGLID==-1) {
				printf ( "ERROR: Image not committed to GPU.\n");			// <-- breakpoint here to debug	
				return -1;
			}
			return m_Pix.mGLID; 		
		}
		#ifdef USE_CUDA
			CUdeviceptr getGPU()	{ return m_Pix.mGpu;  }		
			CUarray getArray( CUtexObject& in, CUsurfObject& out )	{ in = m_Pix.mGtex; out = m_Pix.mGsurf; return m_Pix.mGarray; }
		#endif
		
		// Image Copying
		void Copy (ImageX* src );				// Copy another image to current one
        ImageX* CopyNew ();						// Create new ImageX by direct copy of this one.
				
		// Pixel Operations
		inline Vec4F   GetPixel ( int x, int y )								{ Vec4F c;  (this->*m_getPixelFunc) (x,y, c); return c; }
		inline Vec4F   GetPixelUV ( float u, float v )					{ Vec4F c;  (this->*m_getPixelFunc) ( int(u*(mXres-1)), int(v*(mYres-1)), c); return c; }			
		Vec4F					 GetPixelFilteredUV (float x, float y);		
		void					 Dot (int x, int y, float r, Vec4F c );
		void					 Line (float x0, float y0, float x1, float y1, Vec4F c );
		void		       BlendPixel  ( int x, int y, Vec4F c, float alpha);
		void		SetPixel  ( int x, int y, Vec4F c )			{ (this->*m_setPixelFunc) (x,y, c); }
		void		SetPixelF ( int x, int y, float v )			{ *			(((float*) m_Pix.mCpu) + (y*mXres+x)) = v; }
		float		GetPixelF ( int x, int y )								{ return *	(((float*) m_Pix.mCpu) + (y*mXres+x)); }

		// Pixel Ops - 16-bit grayscale only
		uint16_t	GetPixel16 ( int x, int y )							{ return *	(((uint16_t*) m_Pix.mCpu) + (y*mXres+x)); }
		float	  	GetPixelUV16 ( float u, float v );
		float		GetPixelFilteredUV16 (float x, float y);
		void		SetPixel16 ( int x, int y, uint16_t v )				{ *			(((uint16_t*) m_Pix.mCpu) + (y*mXres+x)) = v; }
		
		// Image Operations
		void ChangeFormat ( ImageOp::Format fmt );
		void Resample ( ImageX* src );
		void Fill (float v);
		void Fill (float r, float g, float b, float a);

		void Scale ( int nx, int ny );

		// Image Information 
		int GetWidth ()							{ return mXres; }
		int GetHeight ()						{ return mYres; }
		XBYTE* GetData ()						{ return (XBYTE*) m_Pix.mCpu; }		// fast access (for pixel functions)
	
		// Image formatting
		void SetFormat ( int x, int y, ImageOp::Format ef) {
			mXres = x; mYres = y;
			mFlags = 0;
			mFmt = ef; 
			mBitsPerPix = GetBitsPerPix ( ef );
			mBytesPerRow = GetBytesPerRow ( x, ef );
		}		

		void SetFilter ( ImageOp::Filter ef );
		void SetUsage ( ImageOp::Usage ef );			
		ImageOp::Filter GetFilter ();		
		ImageOp::Usage	GetUsage ();			
		ImageOp::Format GetFormat ()			{ return mFmt; }
		unsigned int GetFlags ()				{ return mFlags; }								
		unsigned char GetDataType (ImageOp::Format ef);
		void SetFlag ( ImageOp::Flags f, bool val)	{ if (val) mFlags |= (unsigned int) f; else mFlags &= ~(unsigned int) f; }
		void SetFlagEqual ( ImageOp::Flags f, unsigned int src_flags )		{ SetFlag ( f, ( (src_flags & ((unsigned int) f)) == (unsigned int) f ) );}
		bool HasFlag ( ImageOp::Flags f )			{ return (mFlags & (unsigned int) f)==0 ? false : true; }
		bool NoFilter()							{ return !HasFlag ( ImageOp::FilterLo ); }
		bool MipFilter()						{ return HasFlag ( ImageOp::FilterHi ); }
		bool IsTransparent ()					{ return HasFlag ( ImageOp::Alpha); }
		bool IsCube ()							{ return HasFlag ( ImageOp::Cube ); }
		bool HasPowerSize ();
		unsigned char GetBitsPerPix (ImageOp::Format ef);
		inline unsigned char GetBitsPerPix ()	{ return mBitsPerPix; }			
		inline int GetBytesPerPix ()			{ return mBitsPerPix >> 3; }
		inline unsigned long GetBytesPerRow ()	{ return mBytesPerRow; }
		inline unsigned long GetBytesPerRow (int x, ImageOp::Format ef )	{ return GetBitsPerPix(ef)*x >> 3; }
		inline unsigned long GetSize ()			{ return (unsigned long) mBitsPerPix * (unsigned long) mXres * mYres >> 3; }

		// Essential Helper Functions		
		void TransferFrom ( ImageX* new_img);				// Transfer data ownership from another image
		void TransferData ( char* buf );					// Copy data from external source. Size & format must already be set.
		void SetFormatFunc ()					{ SetFormatFunc ( mFmt ); }
		void SetFormatFunc ( ImageOp::Format eFormat );
		void SetDirtyRegion ( Vec4F dr )		{ 
			if (dr.x<0) dr.x=0;
			if (dr.y<0) dr.y=0;
			if (dr.x >= mXres) dr.x = mXres - 1;
			if (dr.y >= mYres) dr.y = mYres - 1;
			if (dr.z>=mXres) dr.z = mXres-1;
			if (dr.w>=mYres) dr.w = mYres-1;
			mDirtyRegion = dr; 
		}
		Vec4F getDirtyRegion ()							{ return mDirtyRegion; }

	private:
		// Pixel accessors
		void (ImageX::*m_getPixelFunc) (int x, int y, Vec4F& c);
		void (ImageX::*m_setPixelFunc) (int x, int y, Vec4F c);			
		
		void getPixelBW8 ( int x, int y, Vec4F& c );
		void setPixelBW8 ( int x, int y, Vec4F c );		
		void getPixelBW16 ( int x, int y, Vec4F& c );
		void setPixelBW16 ( int x, int y, Vec4F c );		
		void getPixelBW32 ( int x, int y, Vec4F& c );
		void setPixelBW32 ( int x, int y, Vec4F c );
		void getPixelRGB8 ( int x, int y, Vec4F& c  );
		void setPixelRGB8 ( int x, int y, Vec4F c );		
		void getPixelRGBA8 ( int x, int y, Vec4F& c  );
		void setPixelRGBA8 ( int x, int y, Vec4F c );		
		void getPixelRGB16 ( int x, int y, Vec4F& c  );
		void setPixelRGB16 ( int x, int y, Vec4F c );		
		void getPixelBGR8 ( int x, int y, Vec4F& c  );
		void setPixelBGR8 ( int x, int y, Vec4F c );		
		void getPixelRGBA32F ( int x, int y, Vec4F& c  );
		void setPixelRGBA32F ( int x, int y, Vec4F c );
		void getPixelF32 ( int x, int y, Vec4F& c  );
		void setPixelF32 ( int x, int y, Vec4F c );
		
	public:
		ImageOp::Format	mFmt;					// Image Format
		unsigned int		mFlags;					// Image Flags
		unsigned char		mBitsPerPix;			// BPP = Bits-per-Pixel
		unsigned long		mBytesPerRow;			// Bytes per Row = BPP*Xres >> 3. Size = mBPP*Xres*Yres >> 3
		bool						mAutocommit;			// Automatically update on GPU after each func

		Vec4F						mDirtyRegion;
	
		int							mXres, mYres;			// Image Resolution

		DataPtr					m_Pix;

		uchar						m_UseFlags;

		int							m_CurrLoader;
		static XBYTE		fillbuf[];
	};
	
	// image format loaders
	extern std::vector<CImageFormat*>  gImageFormats;

#endif


