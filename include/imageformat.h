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

#ifndef DEF_IMAGEFORMAT
	#define	DEF_IMAGEFORMAT

	#include "imagex.h"	
	#include <stdio.h>
#ifdef WIN32
	#include <windows.h>
	#include <conio.h>
#endif

	#define FILE_NAMELEN	512

	class HELPAPI CImageFormat {
	public:
		CImageFormat ();
		~CImageFormat ();

		// Interface functions (called by Image)	
		virtual std::string UsesExt() { return ""; }
		virtual bool Load (char *filename, ImageX* img) {return false;}
		virtual bool Save (char *filename, ImageX* img) {return false;}		
		virtual bool CanLoadType ( unsigned char* magic, std::string ext ) { return false; }
		virtual bool CanSaveType ( std::string ext )		{ return false; }
		virtual void SetQuality (int q)									{m_quality= q;}
		virtual ImageOp::FormatStatus LoadIncremental () {return ImageOp::LoadNotReady;}		

		// Helper functions 
		void StartFormat ( char* filename, ImageX* img, ImageOp::FormatStatus status );		
		std::string GetStatusMsg ();
		ImageOp::FormatStatus GetStatus ()	{ return m_eStatus; }

	public:
		ImageX*				m_pImg;				// Image (ImageFormat does not own it)		
		
		char					m_Filename[FILE_NAMELEN];
		ImageOp::FormatStatus 	m_eStatus;		
		bool					m_incremental;
		int						m_quality;

		// General format data		
		int						m_xres, m_yres;		
		int						m_bpr;
		int						m_bpp;
	};


#endif


