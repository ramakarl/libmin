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
#ifdef BUILD_JPG

#ifndef DEF_IMAGEFORMAT_JPG
	#define	DEF_IMAGEFORMAT_JPG

	#include "imageformat.h"	

	#include "libjpg/jpeglib.h"
	#include "libjpg/jerror.h"
	#include <setjmp.h>

	struct extended_error_mgr {
		struct jpeg_error_mgr pub;
		jmp_buf setjmp_buffer;
	};

	typedef struct extended_error_mgr * extended_error_ptr;

	class HELPAPI CImageFormatJpg : public CImageFormat {
	public:		
		CImageFormatJpg ();

		virtual bool Load (char *filename, ImageX* img);
		virtual bool Save (char *filename, ImageX* img);		

		virtual std::string UsesExt() { return "jpg"; }

		virtual bool CanLoadType ( unsigned char* magic, std::string ext ) 
		{
			if ((magic[0] == 0xD8 && magic[1] == 0xFF) || (magic[1] == 0xD8 && magic[0] == 0xFF)) return true;
			if (ext.compare("jpg")==0) return true;
			return false;
		}
		virtual bool CanSaveType ( std::string ext )
		{
			if (ext.compare("jpg")==0) return true;
			return false;
		}

		virtual ImageOp::FormatStatus LoadIncremental ();

	private:
		jpeg_compress_struct			m_jpeg_cinfo;
		jpeg_decompress_struct		m_jpeg_dinfo;
		extended_error_mgr				m_jerr;
		FILE*											m_jpeg_file;
		XBYTE*										m_pDest;	
	};

#endif
	
#endif