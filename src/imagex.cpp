
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

#include <math.h>
#include <assert.h>

#include "file_tga.h"

#include "imagex.h"
#include "imageformat.h"
#include "imageformat_png.h"
#include "imageformat_tiff.h"
#include "imageformat_tga.h"

#ifdef BUILD_JPG	
	#include "imageformat_jpg.h"
#endif

std::vector<CImageFormat*> gImageFormats;
XBYTE ImageX::fillbuf[16384];

void addImageFormat ( CImageFormat* fmt )
{
	gImageFormats.push_back ( fmt );
}

#define BUF_INFO		0
#define BUF_PIX			1
#define BUF_ALPHA		2
#define BUF_DEPTH		3

//-----------------------------------------------------
//  Image Geometry Control
//-----------------------------------------------------

ImageX::ImageX ()
{
	mAutocommit = true;
	m_UseFlags = DT_CPU;
	m_Pix.Clear();	
	Resize ( 0, 0, ImageOp::RGB8 );
}
ImageX::ImageX ( int xr, int yr, ImageOp::Format fmt, uchar use_flags )
{
	mAutocommit = true;
	m_UseFlags = use_flags;
	m_Pix.Clear();	
	Resize ( xr, yr, fmt );
}

void ImageX::Create ( )
{
	return Create ( 0, 0, ImageOp::RGB8 );
}

void ImageX::Create ( int xr, int yr, ImageOp::Format eFormat, uchar use_flags)
{	
	Resize ( xr, yr, eFormat, use_flags );
}
ImageX::~ImageX (void)
{
	DeleteBuffers ();
}
void ImageX::Clear ()
{
	DeleteBuffers ();
}

void ImageX::DeleteBuffers ()
{
	m_Pix.Clear();	
}

void ImageX::SetUsage ( uchar use_flags )
{
	if (use_flags == 0 ) return;
	m_UseFlags = use_flags;
	m_Pix.SetUsage (  GetDataType ( mFmt ), use_flags, mXres, mYres, 1 );
	m_Pix.UpdateUsage ( m_UseFlags );
}

void ImageX::Commit ( uchar use_flags )
{	
	if (use_flags == 0) use_flags = m_UseFlags;
	if ( !(use_flags & DT_GLTEX) ) use_flags |= DT_GLTEX;
	SetUsage ( use_flags );			// reserves GLID - this already does commit. See m_Pix.UpdateUsage
	//m_Pix.Commit();							// assumes allocated on gpu
}
void ImageX::Map()
{
	m_Pix.Map();
}
void ImageX::Unmap()
{
	m_Pix.Unmap();
}

void ImageX::Retrieve()
{		
	m_Pix.Retrieve();
}

void ImageX::Resize ( int xr, int yr )
{
	Resize ( xr, yr, mFmt );
}

void ImageX::Resize ( int xr, int yr, ImageOp::Format eFormat, uchar use_flags)
{	
	ResizeChannel ( 0, xr, yr, eFormat, use_flags );
}

void ImageX::ResizeChannel ( int chan, int xr, int yr, ImageOp::Format fmt, uchar use_flags)
{

	if ( mXres != xr || mYres != yr || mFmt != fmt ) {

		if ( use_flags==0 ) 
			use_flags = m_UseFlags;		// use existing flags
		else				
			m_UseFlags = use_flags;		// write new flags

		// Set new pixel format parameters		
		uchar dt = GetDataType ( fmt );
		SetFormat ( xr, yr, fmt );		
		m_Pix.SetUsage ( dt, use_flags, xr, yr, 1 );
		m_Pix.Resize ( GetBytesPerPix(), xr*yr, 0x0, use_flags );		
		m_Pix.mNum = xr*yr;
				
		// Update formatting functions
		SetFormatFunc ();
	}
}


void ImageX::ChangeFormat ( ImageOp::Format fmt )
{
	if ( GetFormat() == fmt ) return;

	// save data in another image
	ImageX save;
	save.Copy ( this );			

	// Resize to new format
	Resize ( mXres, mYres, fmt );

	// Write existing to new
	//  (uses format-specific get/set functions)
	Vec4F c;
	for (int y=0; y < mYres; y++) {
		for (int x=0; x < mXres; x++) {
			c = save.GetPixel (x, y);
			SetPixel ( x, y, c );
		}
	}
	if (mAutocommit) Commit();
} 

void ImageX::Resample ( ImageX* src )
{
	Vec4F c;
	float dx = float(src->GetWidth()) / mXres;
	float dy = float(src->GetHeight()) / mYres;

	for (int y=0; y < mYres; y++) {
		for (int x=0; x < mXres; x++) {
			c = src->GetPixel (x * dx, y * dy);
			SetPixel ( x, y, c );
		}
	}

}


void ImageX::CopyToAlpha ()
{
	// convert to RGBA
	if ( mFmt != ImageOp::RGBA8 )
		ChangeFormat ( ImageOp::RGBA8 );

	// Transfer values in alpha
	uchar* pix = GetData();
	uchar v;
	for (int n=0; n < mXres*mYres; n++ ) {
		v = *pix;
		*pix++ = 255;
		*pix++ = 255;
		*pix++ = 255;
		*pix++ = v;		// save to alpha
	}
	if (mAutocommit) Commit();
}

void ImageX::FlipY ()
{
	int pitch = mBytesPerRow;
	unsigned char* data = GetData();
	unsigned char* buf = (unsigned char*) malloc ( mBytesPerRow );
	for (int y=0; y < mYres/2; y++ ) {
		memcpy ( buf, data + (y*pitch), pitch );		
		memcpy ( data + (y*pitch), data + ((mYres-y-1)*pitch), pitch );		
		memcpy ( data + ((mYres-y-1)*pitch), buf, pitch );
	}
	if (mAutocommit) Commit();
}

/* 
bool ImageX::LoadPng ( char* fname, bool bGrey )
{
	char fpath[1024];
	
	if ( ! getFileLocation ( fname, fpath ) ) {
		dbgprintf ( "ERROR: Unable to find png: %s.\n", fname );
		return false;
	}

	#ifdef BUILD_PNG	
		std::vector< unsigned char > out;
		unsigned int w, h; 

		unsigned error = lodepng::decode ( out, w, h, fpath, (bGrey ? LCT_GREY : LCT_RGBA), (bGrey ? 16 : 8) );
		if (error) {
			dbgprintf  ( "ERROR: Reading PNG: %s\n", lodepng_error_text(error) );
			return false;
		}	
		Create ( w, h, bGrey ? ImageOp::BW16 : ImageOp::RGBA8 );
		int stride = GetSize() / mYres;

		uchar* pix = GetData();

		for (int y=0; y < mYres; y++ ) 
			memcpy ( pix + y*stride, &out[ y*stride ], stride );

		//FlipY();

		dbgprintf ( "Decoded PNG: %d x %d, %s\n", mXres, mYres, fpath );

		if (mAutocommit) Commit();

		return true;
	#else
		return false;
	#endif
	
}

bool ImageX::SavePng ( char* fname )
{
	#ifdef BUILD_PNG
		dbgprintf  ( "Saving PNG: %s\n", fname );
		save_png ( fname, GetData(), mXres, mYres, 4 );
		return true;
	#else
		return false;
	#endif
}

bool ImageX::LoadTga ( char* fname )
{
	TGA* tga_file = new TGA;
	TGA::TGAError err = tga_file->load(fname);
	if (err != TGA::TGA_NO_ERROR) {
		delete tga_file;
		return false;  
	}	 
	int xres = tga_file->m_nImageWidth;
	int yres = tga_file->m_nImageHeight;	

	ImageOp::Format fmt;	 
	switch ( tga_file->m_texFormat ) {
	case TGA::RGB:		fmt = ImageOp::RGB8;	break;
	case TGA::RGBA:		fmt = ImageOp::RGBA8;	break;
	case TGA::ALPHA:	fmt = ImageOp::BW16;	break;
	case -1:
		delete tga_file;
		return false;
	}

	Resize ( xres, yres, fmt, DT_CPU | DT_GLTEX );
 
	memcpy ( GetData(), tga_file->m_nImageData, GetSize() );
    
	delete tga_file;

	if (mAutocommit) Commit();

	return true;
}
*/

// Bits per pixel - value depends on format
unsigned char ImageX::GetBitsPerPix (ImageOp::Format ef)
{
	switch (ef) {
	case ImageOp::RGBA32F:						return 128;		break;	// 4 chan,32-bit = 128
	case ImageOp::RGB16:						return 48;		break;	// 3 chan,16-bit =  24
	case ImageOp::RGB8: case ImageOp::BGR8:		return 24;		break;	// 3 chan, 8-bit =  24
	case ImageOp::RGBA8:						return 32;		break;	// 4 chan, 8-bit =  32
	case ImageOp::BW8:							return 8;		break;	// 1 chan, 8-bit =   8
	case ImageOp::BW16:							return 16;		break;
	case ImageOp::BW32:							return 32;		break;
	case ImageOp::F32:							return 32;		break;
	}
	return 0;
}

// Data type - value depends on format
unsigned char ImageX::GetDataType (ImageOp::Format ef)
{
	switch (ef) {
	case ImageOp::BW8:								return DT_UCHAR;	break;
	case ImageOp::BW16:								return DT_USHORT;	break;
	case ImageOp::BW32:								return DT_UINT;		break;	
	case ImageOp::RGB16:							return DT_USHORT3;	break;
	case ImageOp::RGB8: case ImageOp::BGR8:			return DT_UCHAR3;	break;
	case ImageOp::RGBA8:							return DT_UCHAR4;	break;		
	case ImageOp::F32:								return DT_FLOAT;	break;
	case ImageOp::RGBA32F:							return DT_FLOAT4;	break;	
	}
	return 0;
}

// Filtering Options
void ImageX::SetFilter ( ImageOp::Filter ef )
{
	switch (ef) {
	case ImageOp::NoFilter:			SetFlag ( ImageOp::FilterLo, false );	SetFlag ( ImageOp::FilterHi, false ); break;
	case ImageOp::Linear:			SetFlag ( ImageOp::FilterLo, true );	SetFlag ( ImageOp::FilterHi, false ); break;
	case ImageOp::MipNoFilter:		SetFlag ( ImageOp::FilterLo, false );	SetFlag ( ImageOp::FilterHi, true ); break;
	case ImageOp::MipLinear:		SetFlag ( ImageOp::FilterLo, true);		SetFlag ( ImageOp::FilterHi, true ); break;
	};
}

ImageOp::Filter ImageX::GetFilter ()
{
	if (HasFlag (ImageOp::FilterHi)) {
		// Mipmap
		if (HasFlag (ImageOp::FilterLo)) return ImageOp::MipLinear;
		else return ImageOp::MipNoFilter;
	} else {
		// Standard
		if (HasFlag (ImageOp::FilterLo)) return ImageOp::Linear;
		else return ImageOp::NoFilter;
	}
}


/*
void ImageX::AddChannel ( std::string name, int xr, int yr, ImageOp::Format eFormat )
{
	int chan = GetNumBuf()-1;

	char infoname[256];
	sprintf ( infoname, "i%02d", chan );	
	ImageInfo info ( xr, yr, eFormat );
	AddParam ( p, infoname, DT_DATA, 0x0 );
	SetParamData ( p, (uchar*) &info, sizeof(ImageInfo) );

	// Add new channel data
	bufPos b = AddBuffer ( 'E', p, name, info.GetBytesPerPix(eFormat), xr*yr );
	ReserveBuffer ( b, xr*yr );
}*/

/*bool ImageX::LoadChannel ( std::string name, std::string filename )
{
	ImageX		ctrl;
	ImageX		tmp_img ( 0, 0, ImageOp::RGBA8 );
	int			chan;

	ctrl.clear();
	ctrl.add_data ( BUF_UNDEF );

	// Load image file
	if ( ctrl.Load ( filename) ) {

		// Determine channel destination buffer
		chan = FindChannel ( name );
		if ( chan == BUF_UNDEF-1)	{
			AddChannel ( 0, name, tmp_img.GetWidth(), tmp_img.GetHeight(), tmp_img.GetFormat() );
			chan = FindChannel ( name );
		}
		ImageInfo* info = getInfo (chan);
		XBYTE* pixbuf = getData (chan);

		// Resize channel buffer
		ResizeChannel ( chan, tmp_img.GetWidth(), tmp_img.GetHeight(), tmp_img.GetFormat() );

		// Copy pixel data from temp image to channel
		ctrl.Paste ( 0, 0, tmp_img.GetWidth(), tmp_img.GetHeight(), 0, 0, pixbuf, tmp_img.GetFormat(), tmp_img.GetWidth(), tmp_img.GetHeight() );

		printf ( 'imgf', INFO, "Loaded channel: %s (%d), %dx%d, bpp:%d\n", name.c_str(), chan, tmp_img.GetWidth(), tmp_img.GetHeight(), tmp_img.GetBitsPerPixel() );

	}		
	return true;
}
*/

Vec4F ImageX::GetPixelFilteredUV (float x, float y)
{
	float u = x * (mXres - 1);
	float v = y * (mYres - 1);
	int xu = u;
	int yu = v;
	u -= xu;
	v -= yu;
	
	Vec4F c[7];

	(this->*m_getPixelFunc) ( xu,    yu, c[0] );
	(this->*m_getPixelFunc) ( xu+1,  yu, c[1] );
	(this->*m_getPixelFunc) ( xu,  yu+1, c[2] );
	(this->*m_getPixelFunc) ( xu+1,yu+1, c[3] );
	
	// bi-linear filtering
	c[4] = c[0] + (c[1]-c[0]) * u;
	c[5] = c[2] + (c[3]-c[2]) * u;
	c[6] = c[4] + (c[5]-c[4]) * v;	
	
	return Vec4F(c[6].x/255.0f,c[6].y/255.0f,c[6].z/255.0f,c[6].w/255.0f); 
}


float ImageX::GetPixelFilteredUV16 (float x, float y)
{
	float u = x * (mXres - 1);
	float v = y * (mYres - 1);
	int xu = u;
	int yu = v;	
	u -= xu;
	v -= yu;
	
	float c[7];
	unsigned short i ;

	if (xu < 0 || yu < 0 || xu >= mXres-1 || yu >= mYres-1 ) return 0;

	i = GetPixel16 ( xu,    yu );	c[0] = float(i) / 65535.0f;
	i = GetPixel16 ( xu+1,  yu );	c[1] = float(i) / 65535.0f;
	i = GetPixel16 ( xu,  yu+1 );	c[2] = float(i) / 65535.0f;
	i = GetPixel16 ( xu+1,yu+1 ); c[3] = float(i) / 65535.0f;
	
	// bi-linear filtering
	c[4] = c[0] + (c[1]-c[0]) * u;
	c[5] = c[2] + (c[3]-c[2]) * u;
	c[6] = c[4] + (c[5]-c[4]) * v;	
	
	return c[6];
}

float ImageX::GetPixelUV16 (float x, float y)
{
	int xu = x * (mXres - 1);
	int yu = y * (mYres - 1);	
	if (xu < 0 || yu < 0 || xu >= mXres-1 || yu >= mYres-1 ) return 0;
	return GetPixel16 ( xu, yu ) / 65535.0f;	
}

void ImageX::SetFormatFunc ( ImageOp::Format eFormat )
{	
	switch ( eFormat ) {
	case ImageOp::BW8:
		m_getPixelFunc = &ImageX::getPixelBW8;
		m_setPixelFunc = &ImageX::setPixelBW8;		
		break;
	case ImageOp::BW16:
		m_getPixelFunc = &ImageX::getPixelBW16;
		m_setPixelFunc = &ImageX::setPixelBW16;		
		break;
	case ImageOp::BW32:
		m_getPixelFunc = &ImageX::getPixelBW32;
		m_setPixelFunc = &ImageX::setPixelBW32;		
		break;
	case ImageOp::RGB8:		
		m_getPixelFunc = &ImageX::getPixelRGB8;
		m_setPixelFunc = &ImageX::setPixelRGB8;		
		break;
	case ImageOp::RGBA8:		
		m_getPixelFunc = &ImageX::getPixelRGBA8;
		m_setPixelFunc = &ImageX::setPixelRGBA8;		
		break;
	case ImageOp::RGB16:	
		break;
	case ImageOp::RGBA32F:
		m_getPixelFunc = &ImageX::getPixelRGBA32F;
		m_setPixelFunc = &ImageX::setPixelRGBA32F;		
		break;
	case ImageOp::BGR8:
		m_getPixelFunc = &ImageX::getPixelBGR8;
		m_setPixelFunc = &ImageX::setPixelBGR8;		
		break;
	case ImageOp::F32:
		m_getPixelFunc = &ImageX::getPixelF32;
		m_setPixelFunc = &ImageX::setPixelF32;		
		break;
	};

}

// Copy data from external source. Size & format must already be set.
void ImageX::TransferData ( char* buf ) 
{
	memcpy ( m_Pix.mCpu, buf, m_Pix.mSize );		// size valid after Resize

	m_Pix.Commit ();
}

void ImageX::TransferFrom ( ImageX* src_img )
{
	int xr = src_img->GetWidth();
	int yr = src_img->GetHeight();
	
	if ( xr > 0 && yr > 0 ) {

		// Determine new format
		// Note: Only resolution, format and data are transferred. 
		// All other flags (filtering, usage) are left alone.
		unsigned int orig_flags = mFlags;
		
		uchar dt = src_img->GetDataType( src_img->GetFormat() );
		SetFormat ( xr, yr, src_img->GetFormat() );				// Set new pixel format
		m_Pix.SetUsage( dt, m_UseFlags, xr,yr,1);
		m_Pix.mNum = xr * yr;

		SetFlagEqual ( ImageOp::Channels, orig_flags );
		SetFlagEqual ( ImageOp::FilterLo, orig_flags );
		SetFlagEqual ( ImageOp::FilterHi, orig_flags );
		SetFlagEqual ( ImageOp::UseLo, orig_flags );
		SetFlagEqual ( ImageOp::UseHi, orig_flags );

		// Deep copy pixel data		
		CopyIntoBuffer ( m_Pix, src_img->m_Pix, GetBytesPerPix(), xr, yr );
			
		SetFormatFunc ();
	}
}

void ImageX::CopyIntoBuffer ( DataPtr& dest, DataPtr& src, int bpp, int w, int h )
{
	dest.Clear();
	if ( src.mCpu == 0 ) return;
	dest.Resize ( bpp, w*h );
	src.CopyTo ( &dest, DT_CPU );
}

bool ImageX::LoadAlpha ( char *filename )
{
	ImageX ctrl;
	ImageX alpha_img ( 0, 0, ImageOp::RGBA8 );
	std::string errmsg;

	// Load the alpha image
	if ( ctrl.Load ( filename, errmsg ) ) {

		// Change format to RGBA8
		ChangeFormat ( ImageOp::RGBA8 );

		// Copy loaded image into alpha channel
        //CopyToAlpha ( &alpha_img );
	}		
	return true;
}

bool ImageX::LoadIncremental ( char *filename )
{

	/*
	CImageFormatJpg*	pJpgLoader;

	// create new jpg reader
	pJpgLoader = new CImageFormatJpg;
	pJpgLoader->m_incremental = true;		// set incremental

	// start incremental load
	if ( !pJpgLoader->Load (filename, this) ) {
		delete ( pJpgLoader ); mpImageLoader = 0x0;
		return false;
	}
	*/

	// loading..

	return true;	
}

ImageOp::FormatStatus ImageX::LoadNextRow ()
{
	/*
	if ( mpImageLoader == 0x0) return ImageOp::LoadNotReady;

	// still loading..
	ImageOp::FormatStatus eStatus = mpImageLoader->LoadIncremental ();	
	if ( eStatus == ImageOp::LoadDone ) {		
		// load complete. 
		// delete image loader.
		delete mpImageLoader;
		mpImageLoader = 0x0;				
	} 
	return eStatus; */
	return ImageOp::LoadDone;
}

bool ImageX::Load (char* filename, char* alphaname )
{
	std::string errmsg;
	Load ( filename, errmsg );
	bool result = LoadAlpha ( alphaname );		
	return result;
}
/*bool ImageX::LoadCubemap ( int n, char* filename )
{
	int xr, yr;
	ImageX temp;
	temp.Load ( filename );

	if ( n==0 ) {
		DeleteBuffers ();
		
		// Set new pixel format
		xr = temp.GetWidth ();
		yr = temp.GetHeight ();		
		m_Info.Size = GetBytesPerPixel() * xr * yr;	
		m_Info.mXres = xr; m_Info.mYres = yr;	
		m_Info.bDataOwner = true;		
		SetFormat ( ImageOp::RGB8 );
		SetBPP ( xr );

		// Add buffers for each face
		AddAttr ( IMGCUBE+0, -1, "cube0", GetBytesPerPixel(), xr * yr );
		AddAttr ( IMGCUBE+1, -1, "cube1", GetBytesPerPixel(), xr * yr );
		AddAttr ( IMGCUBE+2, -1, "cube2", GetBytesPerPixel(), xr * yr );
		AddAttr ( IMGCUBE+3, -1, "cube3", GetBytesPerPixel(), xr * yr );
		AddAttr ( IMGCUBE+4, -1, "cube4", GetBytesPerPixel(), xr * yr );
		AddAttr ( IMGCUBE+5, -1, "cube5", GetBytesPerPixel(), xr * yr );
		SetPivot (0, 0);								// Default pivot, top left.
		SetFormatFunc (0);
		
		m_Info.bCube = true;
	}
	CopyBuffer ( IMGCUBE + n, m_Pix.buf, &temp );
	
	updateTexture ();

	return true;
}*/

void ImageX::SetupFormats()
{
	if (gImageFormats.size() == 0) {
		addImageFormat(new CImageFormatPng);
		addImageFormat(new CImageFormatTiff);
		addImageFormat(new CImageFormatTga);
		#ifdef BUILD_JPG	
				addImageFormat(new CImageFormatJpg);
		#endif
	}
}

bool ImageX::Load ( std::string filename, std::string& errmsg)
{	
	char ffull[2048];
	strncpy ( ffull, filename.c_str(), 2048 );

	// Setup Formats
	SetupFormats();
	
	// Read magic bytes (first 4 bytes)
	// JPG: if ((magic[0] == 0xD8 && magic[1] == 0xFF) || (magic[1] == 0xD8 && magic[0] == 0xFF)) 
	// TIF: if (magic[0] == 0x49 && magic[1] == 0x49 && magic[2] == 0x2A ) {
	// BMP: if ((magic[0] == 0x4D && magic[1] == 0x42) || (magic[1] == 0x4D && magic[0] == 0x42)) 
  // PNG: if (magic[0] == 0x89 && magic[1] == 0x50 && magic[2] == 0x4E && magic[3] == 0x47) 
  // TGA: if( magic[1] == 0 && (magic[2] == 2 || magic[2] == 3) ) 
	//
	FILE* fp = fopen(ffull, "rb" );
	if ( !fp ) {		
		errmsg = std::string("ERROR: File not found: ") + filename;
		return false;
	}
	unsigned char magic[4];
	fread ((void*) magic, sizeof(char), 4, fp );
	fclose ( fp );

	// Try to load with each loader 	
	//
	std::string msg;
	bool extmatch = false;
	bool loaded = false;

	// Get file parts
	std::string fpath, fname, fext;
	getFileParts(filename, fpath, fname, fext);

	for (int n=0; n < gImageFormats.size() && !loaded; n++) {
		
		if (fext.compare(gImageFormats[n]->UsesExt()) == 0) {
			extmatch = true;
			if (gImageFormats[n]->CanLoadType(magic, ffull)) {
				if (gImageFormats[n]->Load(ffull, this)) {
					// success
					loaded = true;
				}
				else {
					// loader was unable to load
					errmsg = gImageFormats[n]->GetStatusMsg();
					return false;
				}
			}
		}
	}
	if ( !extmatch ) {		// extension not supported
		errmsg = "Unsupported image extension ." + fext;
		return false;
	}

	// Default filtering
	SetFilter( ImageOp::Filter::Linear );

	if (mAutocommit) Commit();

	errmsg = "";
	return true;
}


bool ImageX::Save (char *filename)
{
	std::string fname = filename;
	std::string fext = fname.substr ( fname.length()-3, 3 );

	// Setup Formats
	SetupFormats ();

	// Try to save with each format
	bool saved = false;
	for (int n=0; n < gImageFormats.size() && !saved; n++) {
		if ( gImageFormats[n]->CanSaveType ( fext ) ) {
			if ( gImageFormats[n]->Save ( filename, this ) ) {
				saved = true;
			} else {
				saved = false;
				// errmsg = gImageFormats[n]->GetStatusMsg ();
			}
		}
	}
	return saved;
}


// Scales 'this' image with filtering.
/*void ImageX::Scale (int nx, int ny)
{
	// Create new image data for rescaling
	Image* new_img = new Image ( nx, ny, GetFormat () );

	// Rescale current data into new data buffer
	{ (this->*m_ScaleFunc) ( new_img->GetData(), GetFormat(), nx, ny ); }

	// Make new data the current data (transfer ownership)
	TransferFrom ( new_img );
	delete ( new_img );
} */



// Copy another image to the current one
//   (retains the original format)
void ImageX::Copy (ImageX* src )
{
	// resize self
	Resize ( src->mXres, src->mYres, src->mFmt );

	// copy data from src to self
	memcpy ( GetData(), src->GetData(), src->GetSize() );	
}

// Copies 'this' image into a new image
ImageX* ImageX::CopyNew ()
{
	// Create new image and copy with same format
	ImageX* new_img = new ImageX;
	new_img->Copy ( this );
	return new_img;
}

/*
bool ImageX::UpdateGL ()
{
	Image* img = (Image*) b.dat;
	int flgs = img->getUpdate();
	GLuint* VBO = (GLuint*) img->getVBO();
	
	//debug.Printf ( "Updated Shape: %s, %p\n", img->getName().c_str(), mShape );

	if ( flgs & (objFlags) ObjectX::fBuild ) {			

		// Allocate VBO array
		#ifdef DEBUG_GLERR
			glGetError ();
		#endif
		if ( VBO != 0x0 ) {
			glDeleteTextures ( 1, VBO );
			//debug.Printf ( "Deleted GLID: %d\n", mShape->VBO[0] );
			setGLID ( -1 );
			DeleteVBO ();
		}
		VBO = (GLuint*) CreateVBO ( 1 );

		// Generate textures
		glGenTextures ( 1, VBO );
		#ifdef DEBUG_GLERR
			if ( glGetError() != GL_NO_ERROR ) return false;
		#endif
		printf ( 'data', INFO, "Data, GPU: %d, %s (GLID: %d) \n", img->getID(), img->getName().c_str(), VBO[0] );
		img->setGLID ( VBO[0] );					// fast access to GL buffer data
		img->clearUpdate ( ObjectX::fBuild );		// done getting new vbo
		flgs |= ObjectX::fUpdatePos;				// force texture update
	}	
	if ( flgs & (objFlags) ObjectX::fUpdatePos ) {

		int chan = img->GetParamInt ( b.pos, "channel" );
		ImageInfo* info = img->getInfo ( chan );
		XBYTE* pixbuf = img->getData ( chan );

		// How many images?
		int imgcnt = info->IsCube() ? 6 : 1;
		int imgbind = info->IsCube() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
		int imgfilter = info->MipFilter() ? (info->NoFilter() ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR) :
											(info->NoFilter() ? GL_NEAREST : GL_LINEAR );		
		// Bind texture
		glBindTexture ( imgbind, VBO[ 0 ] );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1);								// pixel alignment
		glTexParameterf ( imgbind, GL_TEXTURE_WRAP_S, GL_REPEAT );			// wrapping
		glTexParameterf ( imgbind, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexParameteri ( imgbind, GL_TEXTURE_MIN_FILTER, imgfilter );		// filtering       
		glTexParameteri ( imgbind, GL_TEXTURE_MAG_FILTER, (imgfilter==GL_LINEAR_MIPMAP_LINEAR) ? GL_LINEAR : imgfilter );		
		
		
		int ms = GL_MAX_TEXTURE_SIZE;

		// *NOTE* 
		// glewInit causes INVALID ENUM on some calls to glTexImage2D. But the call still succeeds.
		// The workaround is to temporarily disable ARB debug here.
		//rendGL->setARBDebug ( 0 );

		// Load textures
		for (int n=0; n < imgcnt; n++ ) {
			int target = info->IsCube() ? GL_TEXTURE_CUBE_MAP_POSITIVE_X+n : GL_TEXTURE_2D ;
			switch ( info->eFormat ) {				
			case ImageOp::BW8:		glTexImage2D ( target, 0, GL_LUMINANCE8, info->mXres, info->mYres,	0, GL_LUMINANCE,	GL_UNSIGNED_BYTE, pixbuf );	break;
			case ImageOp::BW16:		glTexImage2D ( target, 0, GL_LUMINANCE16, info->mXres, info->mYres, 0, GL_LUMINANCE,	GL_UNSIGNED_SHORT, pixbuf );	break;
			case ImageOp::BW32:		glTexImage2D ( target, 0, GL_LUMINANCE32, info->mXres, info->mYres, 0, GL_LUMINANCE,	GL_UNSIGNED_SHORT, pixbuf );	break;			
			case ImageOp::RGB8:		glTexImage2D ( target, 0, GL_RGB8, info->mXres, info->mYres,		0, GL_RGB,			GL_UNSIGNED_BYTE, pixbuf );		break;
			case ImageOp::BGR8:		glTexImage2D ( target, 0, GL_RGB8, info->mXres, info->mYres,		0, GL_BGR_EXT,		GL_UNSIGNED_BYTE, pixbuf );	break;
			case ImageOp::RGBA32F:	glTexImage2D ( target, 0, GL_RGBA32F, info->mXres, info->mYres,		0, GL_RGBA,			GL_FLOAT, pixbuf );				break;
			case ImageOp::F32:		glTexImage2D ( target, 0, GL_R32F, info->mXres, info->mYres,		0, GL_LUMINANCE,	GL_FLOAT, pixbuf );	break;
			};			
			// Generate mipmaps
			if ( info->MipFilter() )	glGenerateMipmap ( target );			
		}
		// Turn ARB debugging back on.
		//rendGL->setARBDebug ( 1 );		

		img->clearUpdate ( ObjectX::fAll );
	}
	return true;
}

void ImageX::render_data ( bufRef b )
{	
	 glPushAttrib ( GL_LIGHTING_BIT );
	glDisable ( GL_LIGHTING );		
	glEnable  ( GL_TEXTURE_2D );
	glEnable ( GL_BLEND );
	glColor3f ( 1, 1, 1 );

	GLuint* VBO = (GLuint*) getVBO();

	//glLoadIdentity ();
	//rendGL->getCurrCam()->setModelMatrix ();

	if ( getInfo()->IsCube() ) {		
		glEnable ( GL_TEXTURE_CUBE_MAP_EXT );
		glBindTexture ( GL_TEXTURE_CUBE_MAP, VBO[0] );		
		//glTranslatef ( 0, 0, 0 );
		glScalef ( 160.0, 160.0, 160.0 );
		//glCallList ( sph );
		glDisable ( GL_TEXTURE_CUBE_MAP_EXT );
	} else {
		//glMultMatrixf ( m_Obj->getTformData() );
		glBindTexture ( GL_TEXTURE_2D, VBO[0] );
		glBegin ( GL_QUADS );
		glTexCoord2f ( 0, 0 ); glVertex3f ( -10, 0.01, -10 );
		glTexCoord2f ( 1, 0 ); glVertex3f ( -10, 0.01, 10 );
		glTexCoord2f ( 1, 1 ); glVertex3f (  10, 0.01, 10 );
		glTexCoord2f ( 0, 1 ); glVertex3f (  10, 0.01, -10 );
		glEnd ();	
			
	}
	glDisable  ( GL_BLEND );
	glDisable  ( GL_TEXTURE_2D );
	glPopAttrib (); 
}

void ImageX::Draw ()	
{
	glEnable ( GL_TEXTURE_2D );
	glBindTexture ( GL_TEXTURE_2D, (GLuint) m_GLID );		
	glBegin ( GL_QUADS );
	glTexCoord2f ( 0, 0 );	glVertex2f ( 0, 0 );
	glTexCoord2f ( 1, 0 );	glVertex2f ( 1, 0 );
	glTexCoord2f ( 1, 1 );  glVertex2f ( 1, 1 );
	glTexCoord2f ( 0, 1 );  glVertex2f ( 0, 1 );
	glEnd (); 
}

void ImageX::createTexture ()
{
	glGenTextures ( 1, (GLuint*) &m_GLID );	
	glBindTexture ( GL_TEXTURE_2D, m_GLID );
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1);	
}

*/

void ImageX::Fill (float v)
{
	Fill (v,v,v,v);
}
void ImageX::Fill (float r, float g, float b, float a)
{
	Vec4F c (r,g,b,a);

	// using format overloads for set pixel
	// -- this is slow for now
	for (int y=0; y < mYres; y++)
		for (int x=0; x < mXres; x++) 
			SetPixel ( x, y, c);

	if (mAutocommit) Commit();
}

void ImageX::Dot(int x, int y, float r, Vec4F c)
{
	float dist, alpha;
	int ri = int(r);
	if (x < r || y < r || x > mXres-r || y > mYres-r) return;

	for (int i=-ri; i <= ri; i++ ) {
		for (int j = -ri; j <= ri; j++) {
			dist = sqrt(float(i*i)+float(j*j));
			if (dist > r+1) continue;
			alpha = (dist < r) ? 1.0 : r + 1 - dist;
			alpha = fmax(0.0f, fmin(1.0f, alpha));
			BlendPixel( x+i, y+j, c, alpha);
		}
	}
}

void swap(float* a, float* b)
{
	float temp = *a;
	*a = *b;
	*b = temp;
}
int roundn (float x) { 	return int(x + 0.5); }
float fracn(float x) 
{
	if (x > 0) return x - int(x);
	else return x - (int(x) + 1);
}

// Xiaolin Wu line algorithm - anti-aliased 
//
void ImageX::Line (float x0, float y0, float x1, float y1, Vec4F c)
{	
	int steep = fabs(y1 - y0) > fabs(x1 - x0);

	if (steep)		{ swap(&x0, &y0);	swap(&x1, &y1);	}
	if (x0 > x1)	{	swap(&x0, &x1);	swap(&y0, &y1);	}

	// compute the slope
	float dx = x1 - x0;
	float dy = y1 - y0;
	float gradient = (dx==0.0) ? 1 : dy / dx;
	int xpxl1 = x0;
	int xpxl2 = x1;
	float intersectY = y0;

	// main loop
	if (steep) {
		int x;
		for (x = xpxl1; x <= xpxl2; x++)	{				
			BlendPixel ( int(intersectY),   x, c, 1.0-fracn(intersectY) );
			BlendPixel ( int(intersectY)+1, x, c, fracn(intersectY) );
			intersectY += gradient;
		}
	}	else {
		int x;
		for (x = xpxl1; x <= xpxl2; x++) {
			BlendPixel (x, int(intersectY), c, 1.0-fracn(intersectY) );
			BlendPixel (x, int(intersectY)+1, c, fracn(intersectY) );
			intersectY += gradient;
		}
	}
}

void ImageX::BlendPixel(int x, int y, Vec4F c, float alpha)
{
	Vec4F px;
	(this->*m_getPixelFunc) (x, y, px);	
	(this->*m_setPixelFunc) (x, y, c * alpha + px * (1-alpha) );
}


//---------------------------------------- PIXEL ACCESSORS
//
void ImageX::setPixelRGB8 (int x, int y, Vec4F c)
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = (XBYTE*) (GetData() + (y * mXres + x) * 3);
		*pix++ = c.x;
		*pix++ = c.y;
		*pix++ = c.z;		
	}	
}
void ImageX::getPixelRGB8 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = (XBYTE*) (GetData() + (y * mXres + x) * 3);
    c.x = *pix++;
    c.y = *pix++;
    c.z = *pix++;
		c.w = 255;
	}
}
void ImageX::setPixelRGBA8 (int x, int y, Vec4F c)
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = (XBYTE*) (GetData() + (y * mXres + x) * 4);
		*pix++ = c.x;
		*pix++ = c.y;
		*pix++ = c.z;		
		*pix++ = c.w;
	}	
}

void ImageX::getPixelRGBA8 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = (XBYTE*) (GetData() + (y * mXres + x) * 4);
        c.x = *pix++;
        c.y = *pix++;
        c.z = *pix++;
		c.w = *pix++;
	}
}

void ImageX::setPixelRGBA32F (int x, int y, Vec4F c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		float* pix = (float*) (GetData() + (y * mXres + x) * GetBytesPerPix() );
		*pix++ = c.x;
		*pix++ = c.y;
		*pix++ = c.z;
		*pix++ = c.w;	
	}	
}
void ImageX::getPixelRGBA32F (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		float* pix = (float*) (GetData() + (y * mXres + x) * GetBytesPerPix() );
        c.x = *pix++;
        c.y = *pix++;
        c.z = *pix++;
		c.w = *pix++;
	}
}
void ImageX::setPixelBGR8 (int x, int y, Vec4F c)
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = (XBYTE*) (GetData() + (y * mXres + x) * 3 );
		*pix++ = c.z;
		*pix++ = c.y;
		*pix++ = c.x;		
	}	
}
void ImageX::getPixelBGR8 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = (XBYTE*) (GetData() + (y * mXres + x) * 3);
        c.z = *pix++;
        c.y = *pix++;
        c.z = *pix++;
		c.w = 255;
	}
}
void ImageX::setPixelRGB16 (int x, int y, Vec4F c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE2* pix = (XBYTE2*) (GetData() + (y * mXres + x)*GetBytesPerPix());
		*pix++ = c.x;
		*pix++ = c.y;
		*pix++ = c.z;
		//  setNotify ( 1 ); // *** NOTIFY(1)
	}	
}
void ImageX::getPixelRGB16 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE2* pix = (XBYTE2*) (GetData() + (y * mXres + x)*GetBytesPerPix());
        c.x = *pix++;
        c.y = *pix++;
        c.z = *pix++;
		c.w = 255;
	}
}

void ImageX::setPixelBW8 (int x, int y, Vec4F c)
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE* pix = ((XBYTE*) GetData()) + (y * mXres + x);			// XBYTE stride
		*pix = c.x;		
	}	
}
void ImageX::getPixelBW8 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x <mXres && y < mYres ) {
		XBYTE* pix = ((XBYTE*) GetData()) + (y * mXres + x);			// XBYTE stride
		c.x = *pix;
		c.y = 0; c.z = 0; c.w = 255;        
	}
}
void ImageX::setPixelBW16 (int x, int y, Vec4F c)
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE2* pix = ((XBYTE2*) GetData()) + (y * mXres + x);		// XBYTE2 stride
		*pix = c.x;		
	}	
}
void ImageX::getPixelBW16 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE2* pix = ((XBYTE2*) GetData()) + (y * mXres + x);		// XBYTE2 stride
        c.x = *pix;
        c.y = 0; c.z = 0; c.w = 1;
	}
}
void ImageX::setPixelBW32 (int x, int y, Vec4F c)
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE4* pix = ((XBYTE4*) GetData()) + (y * mXres + x);	// XBYTE4 stride
		*pix = c.x;	
	}	
}
void ImageX::getPixelBW32 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		XBYTE4* pix = ((XBYTE4*) GetData()) + (y * mXres + x);	// XBYTE4 stride
        c.x = *pix;
		c.y = 0; c.z = 0; c.w = 1;        
	}
}
void ImageX::setPixelF32 (int x, int y, Vec4F c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		float* pix = ((float*) GetData()) + (y * mXres + x);		// float stride
		*pix = c.x;		
	}	
}
void ImageX::getPixelF32 (int x, int y, Vec4F& c )
{
	if ( x>=0 && y>=0 && x < mXres && y < mYres ) {
		float* pix = ((float*) GetData())  + (y * mXres + x);		// float stride
    c.x = *pix;        
		c.y = 0; c.z = 0; c.w = 1;        
	}
}