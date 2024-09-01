
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

#define DEBUG_TIF		false		// enable this for tif debug output


//**************************************
//
// TIFF Format
//
// NOTE: CURRENT CAPABILITIES
//		- Load supports:
//					1 bits/channel			BW
//					8,16,32 bit/channel		Grayscale (no alpha)
//					8,16,32 bit/channel		RGB (with or without alpha)
//		- Save supports:
//					8 bit					RGB (no alpha)
//					16 bit					RGB (no alpha)
//
// * NO LZW COMPRESSION *
//
//**************************************

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "imagex.h"
#include "imageformat_tiff.h"

#include "event_system.h"

CImageFormatTiff::CImageFormatTiff () : CImageFormat()
{
	m_DebugTif = DEBUG_TIF;
	m_eTiffStatus = TifOk;
	m_eTag = TifDocname;
	m_eCompression = TifCompNone;
	m_ePhoto = TifRgb;
	m_eMode = TifColor;
	m_xres = 0;
	m_xres = 0;
	m_bHasAlpha = false;
	m_NumChannels = 0;
	m_BitsPerChannelOffset = 0;
	m_NumStrips = 0;
	m_StripOffsets = 0;
	m_StripCounts = 0;
	m_RowsPerStrip = 0;
	m_bpp = 0;
	m_bpr = 0;
}

// Function: LoadTiff
//
// Input:
//		m_name				Set to TIFF_FORMAT 
//		m_filename			Name of TIFF file to load
// Output:
//		m_success			Set to TRUE of FALSE
//		m_mode				Format mode (BW, GRAY, RGB, INDEX)
//		m_num_chan			Number of channels present
//		m_bpc[n]			Bits per channel for channel [n]
//		m_compress			Compression mode (NONE, PACKBITS, LZW)
//		m_photo			Photometric interpretation
//		m_num_strips		Number of strips
//		m_rps				Rows per strip
//
bool CImageFormatTiff::Load (char *filename, ImageX* img )
{
	StartFormat ( filename, img, ImageOp::Loading );

	unsigned long pnt;

	if ( m_DebugTif )	dbgprintf ("----- TIFF LOADING: %s\n", filename );

	m_Tif = fopen ( filename, "rb" );	
	if ( m_Tif == 0x0 ) {
		m_eStatus = ImageOp::FileNotFound;		
		return false;
	}
	fseek ( m_Tif, 0, SEEK_END );
	uint64_t sz = ftell ( m_Tif );
	fseek ( m_Tif, 0, SEEK_SET );	

	new_event ( m_Buf, 16384, 'app ', 'tiff', 0, 0x0 );
	m_Buf.attachFromFile (m_Tif, sz);
	m_Buf.startRead ();

	m_bHasAlpha = false;
	m_xres = 0;
	m_yres = 0;		
	unsigned short byteorder = m_Buf.getUShort();
	if (byteorder  != (XBYTE2) TIFF_BYTEORDER) {
		//m_Buf.SetOrder (BUFFER_ORDER_MBF);				
	}
	unsigned short magic = m_Buf.getUShort ();
	if ( magic != TIFF_MAGIC) {
		m_eTiffStatus = TifNoMagic; m_eStatus = ImageOp::InvalidFile;
		fclose ( m_Tif );
		return false;
	}

	pnt = m_Buf.getULong ();
	
	m_Buf.setPos (pnt);

	if ( !LoadTiffDirectory () ) { 
		fclose ( m_Tif ); 
		return false; 
	}		
	
	// success
	m_eTiffStatus = TifOk;
	m_eStatus = ImageOp::Successs;
	return true;
}

void CImageFormatTiff::ComputeMinMaxData (unsigned long count, unsigned long offset, int row)
{
	char in[TIFF_BUFFER];
	int x, y, lastrow;	

	m_Buf.setPos (offset);						// Set file position to TIFF data
	lastrow = row + m_RowsPerStrip - 1;
	if (lastrow > m_yres-1) lastrow = m_yres-1;		// Determine last row to process
	
	switch (m_eMode) {
	case TifBw: {							// Black & White TIFF		
		m_Min = 0; m_Max = 1;
	} break;
	case TifGrayscale: {						// Grayscale TIFF
		switch (m_BitsPerChannel[TifGray]) {
		case 8: {									// 8 Bits per Channel			
			uchar* pIn;			
			for (y = row; y <= lastrow; y++) {
				m_Buf.getBuf ( in, m_bpr );
				pIn = (uchar*) in;
				for (x=0; x < m_xres; x++) {
					if ( *pIn < m_Min ) m_Min = *pIn;
					if ( *pIn > m_Max ) m_Max = *pIn;
					pIn++;
				}
			}
		} break;
		case 16: {									// 16 Bits per Channel			
			XBYTE2 *pIn;			
			for (y = row; y <= lastrow;	y++) {
				m_Buf.getBuf ( in, m_bpr );
				pIn = (XBYTE2 *) in;
				for (x=0; x < m_xres; x++) {
					if ( *pIn < m_Min ) m_Min = *pIn;
					if ( *pIn > m_Max ) m_Max = *pIn;
					pIn++;
				}
			}
		 } break;
		case 32: {									// 32 Bits per Channel
			XBYTE4 *pIn;
			for (y = row; y <= lastrow;	y++) {
				m_Buf.getBuf (in, m_bpr );
				pIn = (XBYTE4 *) in;
				for (x=0; x < m_xres; x++ ) {
					if ( *pIn < m_Min ) m_Min = *pIn;
					if ( *pIn > m_Max ) m_Max = *pIn;
					pIn++;
				}
			}
		} break;
		}
	} break;
	}
}

void CImageFormatTiff::ComputeMinMax ()
{
	unsigned long pos, row;	
	unsigned long pos_offsets, pos_counts;		// These are not pointers, must increment by 4!
	unsigned long count, offset, strip;
	
	// Reset min/max
	m_Min = 0xFFFFFFFF;
	m_Max = 0;

	pos = m_Buf.getPosInt ();
	pos_counts = m_StripCounts;
	pos_offsets = m_StripOffsets;
	if (m_NumStrips==1) {		
		count = pos_counts;
		offset = pos_offsets;
		row = 0;			
		ComputeMinMaxData (count, offset, row);
	} else {		
		row = 0;		
		for (strip=0; strip < m_NumStrips; strip++) {
			m_Buf.setPos (pos_counts);
			count = m_Buf.getUShort();
			m_Buf.setPos  (pos_offsets);
			offset = m_Buf.getULong();

			ComputeMinMaxData (count, offset, row);
			
			row += m_RowsPerStrip;
			pos_counts+=4;						// These are not pointers, must increment by 4!
			pos_offsets+=4;
		}
	}				
	m_Buf.setPos (pos);
}

bool CImageFormatTiff::LoadTiffDirectory ()
{
	unsigned int num;	
	unsigned long offset, pos;	
		
	// Read Number of TIFF Directory Entries
	num = m_Buf.getUShort();	

	// Reset BPC
	m_BitsPerChannel[TifGray] = 0;
	m_BitsPerChannel[TifRed] =	0;
	m_BitsPerChannel[TifGreen] = 0;
	m_BitsPerChannel[TifBlue] =	0;
	m_BitsPerChannel[TifAlpha] = 0;		

	// Read TIFF Directory Entries to fill ImageFormat Info
	for (unsigned int n = 0; n <= num; n++)	{
		if (!LoadTiffEntry ()) {
			// Error set by LoadTiffEntry
			return false;
		}
	}

	// Make sure we can support this TIF file
	if (m_eCompression == TifCompLzw) {		
		m_eTiffStatus = TifLzwNotSupported;
		m_eStatus = ImageOp::FeatureNotSupported;		
		return false;
	}

	// Determine new image specs
	ImageOp::Format eNewFormat;

	switch (m_eMode) {						
	case TifColor: {									// Color Image

		pos = m_Buf.getPosInt();
		m_Buf.setPos (m_BitsPerChannelOffset);			// Get bits per channel
		
		m_BitsPerChannel[TifRed] =		m_Buf.getUShort();
		m_BitsPerChannel[TifGreen] =	m_Buf.getUShort();
		m_BitsPerChannel[TifBlue] =		m_Buf.getUShort();
		m_BitsPerChannel[TifAlpha] =  m_bHasAlpha ? m_Buf.getUShort() : 0;
		
		// support color with or without alpha
		// *limitation*: stores into 8-bit RGB even if the data is 16 or 32-bit color		
		eNewFormat = (m_bHasAlpha) ? ImageOp::RGBA8 : ImageOp::RGB8;	
		
		m_Buf.setPos (pos);			// Calculate bits per pixel and bytes per row
		
	} break;

	case TifGrayscale: {	
		// support both 16-bit and 8-bit grayscale
		eNewFormat = (m_BitsPerChannel[TifGray] == 16) ? ImageOp::BW16 : ImageOp::BW8;	
	} break;	
	}
	// Compute bpp and bpr
	m_bpp = m_BitsPerChannel[TifGray] + m_BitsPerChannel[TifRed] + m_BitsPerChannel[TifGreen] + m_BitsPerChannel[TifBlue] + m_BitsPerChannel[TifAlpha];
	m_bpr = (int) floor ( (m_xres * m_bpp) / 8.0);

	if ( m_DebugTif ) {
			dbgprintf ( "BPC Grayscale:   %d\n", m_BitsPerChannel[TifGray] );		
			dbgprintf ( "BPC Red:   %d\n", m_BitsPerChannel[TifRed] );
			dbgprintf ( "BPC Green: %d\n", m_BitsPerChannel[TifGreen] );
			dbgprintf ( "BPC Blue:  %d\n", m_BitsPerChannel[TifBlue] );
			dbgprintf ( "BPC Alpha: %d\n", m_BitsPerChannel[TifAlpha] );
			dbgprintf ( "Resizing: %d x %d, bpp %d\n", m_xres, m_yres, m_bpp );
		}		

	// Resize image	
	m_pImg->Resize ( m_xres, m_yres, eNewFormat ); 
	
	// If no RowsPerStrip tag, assume full image in 1 strip
	//  (see libtiff 4.4.0, TIFFReadEncodedStripGetStripSize)
	if ( m_RowsPerStrip==0 ) {
		m_RowsPerStrip = m_yres;
		m_NumStrips  = 1;
	}

	// Load actual TIFF Image Data into Image
	LoadTiffStrips ();

	// Check if there are multiple images present in TIFF file	
	offset = m_Buf.getULong();
	if (offset!=0) {
		// Warning only.
		if ( m_DebugTif ) dbgprintf ( "ImageFormatTiff::LoadTiff: (Warning only) File contains multiple tiff images - reading only first.\n");
	}
	
	// Success
	m_eStatus = ImageOp::Successs;
	return true;
}

bool CImageFormatTiff::LoadTiffEntry ()
{
	unsigned int tag, typ;
	unsigned long int count, offset;	
		
	// Read Entry Tag (WIDTH, HEIGHT, EXTRASAMPLES, BITSPERSAMPLE, COMPRESSION, etc.)
	tag = m_Buf.getUShort();
	// Read Entry Type and Count (SHORT or LONG)
	typ = m_Buf.getUShort();
	count = m_Buf.getULong();
	if (typ == (int) TifShort && count==1) {
		offset = m_Buf.getUShort();
		m_Buf.getUShort();
	} else {
		offset = m_Buf.getULong();
	}
	// DEBUG OUTPUT
	// printf ("tag:%u type:%u count:%lu offset:%lu\n", tag, typ, count, offset);
	
	// Add information to ImageFormat info based on Entry Tag
	switch (tag) {	
	case TifImageWidth:		
		m_xres = offset;			
		if (m_DebugTif) dbgprintf ( "TifImgWidth: %d xres\n", m_xres );		
		break;
	case TifImageHeight:		
		m_yres = offset;
		if (m_DebugTif) dbgprintf ( "TifImgHeight: %d yres\n", m_yres );		
		break;
	case TifExtraSamples:		
		m_bHasAlpha = true;		
		if (m_DebugTif) dbgprintf ( "TifExtraSamples - has alpha\n" );		
		break;
	case TifSamplesPerPixel:
		m_SamplesPerPix = count;
		if (m_DebugTif) dbgprintf ( "TifSamplesPerPix: %d\n", count );		
		break;
	case TifBitsPerSample:
		// NOTE: The TIFF specification defines 'samples',
		// while the Image class defines 'channels'.
		// Thus, bits-per-sample (in TIFF) is the same as bits-per-channel
		// And, samples-per-pixel (in TIFF) is the same as channels-per-pixel
		
		// Detemine samples-per-pixel here 
		// (this is just "count" of how many bits-per-sample 
		//  entries are present)		
		m_NumChannels = count;
		switch (count) {		
		case 1: {			// One channel: B&W or GRAYSCALE
			m_BitsPerChannel[TifGray] = offset;
			m_BitsPerChannelOffset = 0;
			if (offset==1)	m_eMode = TifBw;
			else			m_eMode = TifGrayscale;
		} break;
		case 2: {			// Two channels: GRAYSCALE with ALPHA
			m_BitsPerChannel[TifGray] = offset;
			m_BitsPerChannelOffset = offset;
			m_eMode = TifGrayscale;
		} break;		
		case 3:				// Three channels: RGB
		case 4: {			// Four channels: RGB with ALPHA
			m_BitsPerChannelOffset = offset;	
			if (m_DebugTif) dbgprintf ( "BPC Offset: %d\n", m_BitsPerChannelOffset );			
			m_eMode = TifColor;
		} break;
		default: {
			m_eTiffStatus = TifUnknownTiffMode;
			m_eStatus = ImageOp::FeatureNotSupported;
			return false;
		} break;
		}
		break;
	case TifCompression:
		if (offset == (int) TifCompNone) {		// No compression
			m_eCompression = TifCompNone;
		} else {											// LZW compression
			m_eCompression = TifCompLzw;	
		}
		break;	
	case TifPhotometric:
		if (offset == (int) TifPalette)
			m_eMode = TifIndex;		
		break;
	case TifStripOffsets:
		m_StripOffsets = offset; 
		if (m_DebugTif) dbgprintf ( "Pos Offsets: %d\n", m_StripOffsets );		
		break;
	case TifRowsPerStrip:
		m_RowsPerStrip = offset;
		m_NumStrips = (unsigned long) int ((m_yres + m_RowsPerStrip - 1) / m_RowsPerStrip);		
		if (m_DebugTif) dbgprintf ( "Rows/Strip:  %d\n", m_RowsPerStrip );
		if (m_DebugTif) dbgprintf ( "Num Strips:  %d\n", m_NumStrips );		
		break;
	case TifStripByteCounts:
		m_StripCounts = offset; 		
		if (m_DebugTif) dbgprintf ( "Strip Counts:  %d\n", m_StripCounts );		
		break;
	case TifPlanarConfiguration:
		m_PlanarConfig = offset;
		break;
	}
	return true;
}

void CImageFormatTiff::LoadTiffStrips ()
{	
	unsigned long pos, row;	
	unsigned long pos_offsets, pos_counts;		// These are not pointers, must increment by 4!
	unsigned long count, offset, strip;
	
	pos = m_Buf.getPosInt();
	pos_counts = m_StripCounts;
	pos_offsets = m_StripOffsets;

	if (m_NumStrips==1) {		
		count = pos_counts;
		offset = pos_offsets;
		row = 0;			
		LoadTiffData (count, offset, row);
	} else {		
		row = 0;		

		for (strip=0; strip < m_NumStrips; strip++) {
			
			// get next count from count header
			m_Buf.setPos (pos_counts);		count =		m_Buf.getUShort();
			// get next offset from offset header
			m_Buf.setPos (pos_offsets);		offset =	m_Buf.getULong ();

			LoadTiffData (count, offset, row);
			
			row += m_RowsPerStrip;
			pos_counts += 4;						// Each entry is 4-bytes 
			pos_offsets += 4;
		}
	}				
	m_Buf.setPos (pos);
}

void CImageFormatTiff::LoadTiffData (unsigned long count, unsigned long offset, int row)
{
	char in[TIFF_BUFFER];
	char *pData;
	int x, y, lastrow;	

	pData = (char*) m_pImg->GetData() + row * m_pImg->GetBytesPerRow();
	
	if (m_DebugTif) dbgprintf ( "Data %d: pos %d, num %d\n", row, offset, count );	

	m_Buf.setPos (offset);						// Set file position to TIFF data
	lastrow = row + m_RowsPerStrip - 1;
	if (lastrow > m_yres-1) lastrow = m_yres-1;		// Determine last row to process
	
	switch (m_eMode) {
	case TifBw: {							// Black & White TIFF		
		char* pIn;
		
		dbgprintf ( "ERROR: B&W TIFF NOT SUPPORTED. See code.\n" );
		exit(-18);

		for (y = row; y <= lastrow; y++) {			// Assume 1 Bit Per Pixel
			m_Buf.getBuf (in, m_bpr);					// Read row in TIFF
			
			//code.UnstuffEachBit (in, out, m_BytesPerRow);	// Unstuff row from bits to bytes
			//code.Scale (out, m_BytesPerRow*8, 255);			// Rescale row from 0-1 to 0-255
			
			//if (m_ePhoto == TifWhiteZero)		// Invert if photometrically stored
			//				code.Invert (out, m_BytesPerRow*8);				
			
			pIn = in;
			for (x=0; x < m_xres; x++) {
				*pData++ = (XBYTE) *pIn;				// Copy Black/White byte data 
				*pData++ = (XBYTE) *pIn;				// into Red, Green and Blue bytes.
				*pData++ = (XBYTE) *pIn++;
			}
		}		
	} break;

	case TifGrayscale: {						// Grayscale TIFF
		switch (m_BitsPerChannel[TifGray]) {
		case 8: case 16: case 32: {									// 8 Bits per Channel				
			for (y = row; y <= lastrow; y++) {
				m_Buf.getBuf ( pData, m_bpr );				
				pData += m_bpr;
			}
		} break;
		}
	} break;

	case TifColor: {									// Full Color TIFF
		switch (m_BitsPerChannel[TifRed]) {					
		case 8: {											// 8 Bits per Channel
			
			// Supports 3 (RGB) and 4 components (RGBA, /w alpha)
			for (y = row; y <= lastrow; y++) {
				m_Buf.getBuf (pData, m_bpr);
				pData += m_bpr;
			}			
		} break;
			 
		case 16: {											// 16 Bits per Channel

			XBYTE2 *pIn;
			if (m_bHasAlpha) {						// Alpha included.										
				for (y = row; y <= lastrow;	y++) {
					m_Buf.getBuf (in, m_bpr);
					pIn = (XBYTE2 *) in;
					for (x=0; x < m_xres; x++) {
						*pData++ = (XBYTE) (*pIn++ >> 8); 
						*pData++ = (XBYTE) (*pIn++ >> 8);
						*pData++ = (XBYTE) (*pIn++ >> 8); 
						*pData++ = (XBYTE) (*pIn++ >> 8);
					}
				}
			} else {
				for (y = row; y <= lastrow; y++) {			// No Alpha.
					m_Buf.getBuf (in, m_bpr);
					pIn = (XBYTE2 *) in;
					for (x=0; x < m_xres; x++) {
						*pData++ = (XBYTE) (*pIn++ >> 8); 
						*pData++ = (XBYTE) (*pIn++ >> 8);
						*pData++ = (XBYTE) (*pIn++ >> 8);					
					}
				}
			}
		} break;

		case 32: {									// 32 Bits per Channel
 			XBYTE4 *pIn;
			if (m_bHasAlpha) {				
				for (y = row; y <= lastrow;	y++) {
					m_Buf.getBuf (in, m_bpr);
					pIn = (XBYTE4 *) in;
					for (x=0; x < m_xres; x++) {
						*pData++ = (XBYTE) (*pIn++ >> 24); 
						*pData++ = (XBYTE) (*pIn++ >> 24);
						*pData++ = (XBYTE) (*pIn++ >> 24); 
						*pData++ = (XBYTE) (*pIn++ >> 24);
					}
				}
			} else {
				
				for (y = row; y <= lastrow; y++) {
					m_Buf.getBuf (in, m_bpr);
					pIn = (XBYTE4 *) in;
					for (x=0; x < m_xres; x++) {
						*pData++ = (XBYTE) (*pIn++ >> 24); 
						*pData++ = (XBYTE) (*pIn++ >> 24);
						*pData++ = (XBYTE) (*pIn++ >> 24);					
					}
				}
			}
		} break;
		}
	} break;
	}

	bool stop=true;
}

bool CImageFormatTiff::SaveTiffData ()
{	
	uchar* pData;
	int y;

	pData = m_pImg->GetData();						// Get ImageX color data	

	if (m_DebugTif) dbgprintf ( "Data Start: pos: %ld\n", m_Buf.getPosInt () );	
	// DEBUG OUTPUT
	// printf ("Data pos: %lu\n", tiff.GetPosition ());
	// printf ("Est. pos: %lu\n", TIFF_SAVE_POSDATA);	

	switch (m_eMode) {
	case TifGrayscale: {
		switch (m_BitsPerChannel[TifGray]) {
		case 16: {									// 16 Bits per Channel
			XBYTE2 out[TIFF_BUFFER];						
			for (y=0; y < m_yres; y++) {				
				memcpy ( out, pData, m_bpr );				
				m_Buf.attachBuf ( (char*) out, m_bpr );
				pData += m_bpr;
			}			
		} break;		
		case 32: {									// 32 Bits per Channel
			XBYTE4 out[TIFF_BUFFER];						
			for (y=0; y < m_yres; y++) {				
				memcpy ( out, pData, m_bpr );
				m_Buf.attachBuf ( (char*) out, m_bpr );
				pData += m_bpr;				
			}			
		} break;		
		}
		} break;
	case TifColor: {								// Save Full Color TIFF		
		switch (m_BitsPerChannel[TifRed]) {
		case 8: {									// 8 Bits per Channel
			XBYTE out[TIFF_BUFFER];			
			for (y=0; y < m_yres; y++) {								
				memcpy ( out, pData, m_bpr );	
				m_Buf.attachBuf ( (char*) out, m_bpr );
				pData += m_bpr;
			}			
		} break;	
		case 16: {
			XBYTE2 out[TIFF_BUFFER];
			for (y=0; y < m_yres; y++) {								
				memcpy ( out, pData, m_bpr );	
				m_Buf.attachBuf ( (char*) out, m_bpr );
				pData += m_bpr;
			}
		} break;
		default: {		
			printf ("SaveTiff: Given pixel depth and color mode not supported.\n");
			exit(-8);		
		} break;
		}
		} break;
	}
	return true;
}

bool CImageFormatTiff::SaveTiffEntry (enum TiffTag eTag)
{
	enum TiffType eType;	
	unsigned long int iCount, iOffset;	
		
	switch (eTag) {	
	case TifNewSubType: {
		eType = TifLong;
		iCount = 1; 
		iOffset = 0;
	} break;
	case TifImageWidth: {
		eType = TifShort;
		iCount = 1;
		iOffset = m_xres;
		if (m_DebugTif) dbgprintf ( "XRes: %d\n", m_xres );
	} break;
	case TifImageHeight: {
		eType = TifShort;
		iCount = 1;
		iOffset = m_yres;
		if (m_DebugTif) dbgprintf ( "YRes: %d\n", m_yres );
	} break;		
	case TifBitsPerSample: {
		// NOTE: The TIFF specification defines 'samples',
		// while the Image class defines 'channels'.
		// Thus, bits-per-sample (in TIFF) is the same as bits-per-channel
		// And, samples-per-pixel (in TIFF) is the same as channels-per-pixel
		
		switch (m_eMode) {
		case TifBw: {
			eType = TifShort;
			iCount = 1; 
			iOffset = 1;
		} break;
		case TifGrayscale: {
			eType = TifShort;
			iCount = 1;
			iOffset = m_bpp;
		} break;
		case TifColor: {
			eType = TifShort;
			iCount = 3;
			iOffset = TIFF_SAVE_POSBPC;
		} break;
		default: {
			dbgprintf ("Save:Tiff: Unknown TIFF mode.\n");
			exit(-8);
		} break;
		}		
		m_NumChannels = iCount;
	} break;
	case TifCompression: {
		eType = TifShort;
		iCount = 1; 
		iOffset = (int) TifCompNone;
	} break;
	case TifPhotometric: {
		eType = TifShort;
		iCount = 1; 
		switch (m_eMode) {
		case TifBw: iOffset = (int) TifBlackZero; break;
		case TifGrayscale: iOffset = (int) TifBlackZero; break;
		case TifColor: iOffset = (int) TifRgb; break;
		case TifIndex: iOffset = (int) TifPalette; break;
		}
		m_ePhoto = (enum TiffPhotometric) iOffset;
	} break;
	case TifStripOffsets: {
		eType = TifLong;
		iCount = m_yres;							// how many in list
		iOffset = TIFF_SAVE_POSOFFSETS + m_yres*4;	// where is the list
		if (m_DebugTif) dbgprintf ( "Num Strips:  %d\n", iCount );
		if (m_DebugTif) dbgprintf ( "Pos Offsets: %d\n", iOffset );
	} break;		
	case TifSamplesPerPixel: {
		eType = TifShort;
		iCount = 1;
		iOffset = m_NumChannels;
	} break;
	case TifRowsPerStrip: {
		eType = TifShort;
		iCount = 1;
		iOffset = 1;
		m_RowsPerStrip = 1;
		m_NumStrips = m_yres;
	} break;
	case TifStripByteCounts: {
		eType = TifLong;
		iCount = m_yres;					// how many in list 
		iOffset = TIFF_SAVE_POSOFFSETS;		// where is the list
		switch (m_eMode) {
		case TifBw: m_bpr = (int) floor ((m_xres / 8.0) + 1.0); break;
		case TifGrayscale: m_bpr = (int) floor ((m_xres * m_bpp) / 8.0); break;
		case TifColor: m_bpr = (int) floor ((m_xres * m_bpp) / 8.0); break;
		}		
		if (m_DebugTif) dbgprintf ( "Pos Counts:   %d\n", iOffset );
		//m_StripCounts = m_BytesPerRow;
	} break;
	case TifXres: {
		eType = TifRational;
		iCount = 1;
		iOffset = 0;
	} break;
	case TifYres: {
		eType = TifRational; 
		iCount = 1; 
		iOffset = 0;
	} break;
	case TifResUnit: {
		eType = TifShort;
		iCount = 1;
		iOffset = 2;
	} break;
	case TifColorMap: {
		eType = TifShort;
		iCount = 0; 
		iOffset = 0;
	} break;
	case TifPlanarConfiguration: {
		eType = TifShort;
		iCount = 1;
		iOffset = 1;
	} break;
	}

	m_Buf.attachUShort( eTag );
	m_Buf.attachUShort( eType );	
	m_Buf.attachULong ( iCount );
	
	if (eType==TifShort && iCount==1) {		
		m_Buf.attachUShort ( iOffset );		
		m_Buf.attachUShort ( 0 );
	} else {
		m_Buf.attachULong ( iOffset );
	}
	// DEBUG OUTPUT
	// printf ("-- tag:%u type:%u count:%lu offset:%lu\n", tag, typ, count, offset);
	return true;
}

bool CImageFormatTiff::SaveTiffExtras (enum TiffTag eTag)
{
	switch (eTag) {
	case TifBitsPerSample: {
		// DEBUG OUTPUT
		// printf ("BPC pos: %u\n", tiff.GetPosition());
		// printf ("Est.BPC pos: %u\n", TIFF_SAVE_POSBPC);		
		int bpc = (m_bpp==24) ? 8 : 16;
		m_Buf.attachUShort ( bpc );
		m_Buf.attachUShort ( bpc );
		m_Buf.attachUShort ( bpc );
	} break;	
	case TifXres: {
		// DEBUG OUTPUT
		// printf ("Xres pos: %u\n", tiff.GetPosition());
		// printf ("Est.Xres pos: %u\n", TIFF_SAVE_POSXRES);		
		m_Buf.attachULong ( 1 );
		m_Buf.attachULong ( 1 );
	} break;
	case TifYres: {
		// DEBUG OUTPUT
		// printf ("Yres pos: %u\n", tiff.GetPosition());
		// printf ("Est.Yres pos: %u\n", TIFF_SAVE_POSYRES);		
		m_Buf.attachULong ( 1 );
		m_Buf.attachULong ( 1 );
	} break;
	}
	return true;
}

bool CImageFormatTiff::SaveTiffDirectory ()
{
	// *NOTE*: TIFF Spec 6.0 appears to be incorrect. 
	// The maximum entry number should be written (base 0). NOT the number of entries.
	//
	m_Buf.attachUShort ( TIFF_SAVE_ENTRIES-1 );

	switch (m_eMode) {
	case TifBw: 
		m_BitsPerChannel[TifGray] = 1; 
		break;
	case TifGrayscale: 
		m_BitsPerChannel[TifGray] = m_bpp; 
		m_bpr = (int) floor ((m_xres * m_bpp) / 8.0); 
		break;
	case TifColor: {
		int bpc = (m_bpp==24) ? 8 : 16;
		m_BitsPerChannel[TifRed] = bpc;
		m_BitsPerChannel[TifGreen] = bpc;
		m_BitsPerChannel[TifBlue] = bpc;
		m_bpr = (int) floor ((m_xres * m_bpp) / 8.0); 
	} break;
	}
	
	SaveTiffEntry (TifImageWidth);
	SaveTiffEntry (TifImageHeight);
	SaveTiffEntry (TifBitsPerSample);
	SaveTiffEntry (TifCompression);
	SaveTiffEntry (TifPhotometric);
	SaveTiffEntry (TifStripOffsets);
	SaveTiffEntry (TifSamplesPerPixel);	
	SaveTiffEntry (TifRowsPerStrip);
	SaveTiffEntry (TifStripByteCounts);	
	SaveTiffEntry (TifPlanarConfiguration);	

	m_Buf.attachULong ( 0 );

	SaveTiffExtras (TifBitsPerSample);

	// at this point, we should have written exactly TIFF_SAVE_POFOSSETS bytes	
	int offs = m_Buf.getPosInt();
	assert ( offs == TIFF_SAVE_POSOFFSETS );

	int strip_header_size = (m_yres*4) * 2;		// 2x = counts & offsets

	// Strip counts -- List of # of bytes in each strip. 
	// 4-byte list (2x ushort) per row	
	for (int n=0; n < m_yres; n++) {
		if (m_DebugTif) dbgprintf ( "Counts #%d: pos: %d, val: %d\n", n, m_Buf.getPos(), m_bpr );
		m_Buf.attachUShort ( m_bpr );		
		m_Buf.attachUShort ( 0 );
	}
	
	// Strip offsets -- List of strip offsets. 
	// 4-byte list (ulong) per row
	for (int n=0; n < m_yres; n++) {
		if (m_DebugTif) dbgprintf ( "Offset #%d: pos: %d, val: %d\n", n, m_Buf.getPos(), TIFF_SAVE_POSOFFSETS + m_yres*4*2 + n * m_bpr);
		m_Buf.attachULong ( TIFF_SAVE_POSOFFSETS + strip_header_size + n * m_bpr );
	}

	SaveTiffData ();

	return true;
}

// Function: SaveTiff
//
// Input:
//		m_filename			Name of file to save
//		m_mode				Format mode (BW, GRAY, RGB, INDEX)
//		m_compress			Compression mode
//		m_bpp				Bits per pixel
// Output:
//		m_status
//
// Example #1 - 160 x 120.. Saved /w Photoshop
//   TIF: XRes: 160
//   TIF: YRes: 120
//   TIF: BPC Offset: 242
//   TIF: Pos Offsets: 7178
//   TIF: Rows/Strip:  120
//   TIF: Num Strips:  1
//   TIF: Pos Counts:  57600
//   TIF: BPC Red:   8
//   TIF: BPC Green: 8
//   TIF: BPC Blue:  8
//   TIF: Data 0: pos 7178, num 57600
//
// Example #2 - 160 x 120.. Saved with this class
//
//   TIF: XRes: 160
//   TIF: YRes: 120
//   TIF: BPC Offset: 134
//   TIF: Pos Offsets: 620
//   TIF: Rows/Strip:  1
//   TIF: Num Strips:  120
//   TIF: Pos Counts:  140
//   TIF: BPC Red:   8
//   TIF: BPC Green: 8
//   TIF: BPC Blue:  8
//

bool CImageFormatTiff::Save (char *filename, ImageX* img )
{
	StartFormat ( filename, img, ImageOp::Saving );

	m_Tif = fopen ( filename, "wb" );
	if ( m_Tif == 0x0 ) { 
		dbgprintf ( "ERROR: Unable to create TIF file %s\n", filename );
		return false;
	}	
	
	switch ( m_pImg->GetFormat() ) {
	case ImageOp::RGB8: case ImageOp::RGBA8: case ImageOp::RGBA32F: case ImageOp::RGB16: 
		m_eMode = TifColor;
		break;
	case ImageOp::BW8: case ImageOp::BW16: case ImageOp::BW32: case ImageOp::F32:
		m_eMode = TifGrayscale;
		break;
	}
	m_xres = m_pImg->GetWidth();
	m_yres = m_pImg->GetHeight();
	m_bpp = m_pImg->GetBitsPerPix ();
	
	new_event ( m_Buf, 16384, 'app ', 'tiff', 0, 0x0 );

	m_Buf.startWrite ();
	m_Buf.attachUShort ( TIFF_BYTEORDER );
	m_Buf.attachUShort  ( TIFF_MAGIC );
	m_Buf.attachULong ( TIFF_SAVE_POSIFD );
	
	SaveTiffDirectory ();			// Write IFD
	
	// write buffer to file
	fwrite ( m_Buf.getData (), 1, m_Buf.getDataLength(), m_Tif );

	fflush ( m_Tif );

	fclose ( m_Tif );
	m_eTiffStatus = TifOk;

	return true;
}



