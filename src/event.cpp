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
#include "event.h"

static char mbuf [ 16384 ];

extern char* new_event_data ( size_t size, int& max, EventPool* pool, eventStr_t name, const char* msg=0 );
extern void free_event_data ( char*& data, EventPool* pool, eventStr_t name, int cid, const char* msg=0 );
extern void free_event ( Event& e, const char* msg=0 );
extern void expand_event ( Event& e, size_t size );
extern int event_alloc;
#ifdef DEBUG_EVENT_MEM
	extern void emem_rename ( Event& e, eventStr_t oldname, eventStr_t newname, const char* msg );
#endif

eventStr_t strToName (std::string str )
{
	char buf[5];
	strcpy ( buf, str.c_str() );
	eventStr_t name;
	((char*) &name)[3] = buf[0];
	((char*) &name)[2] = buf[1];
	((char*) &name)[1] = buf[2];
	((char*) &name)[0] = buf[3];
	return name;
}

std::string nameToStr ( eventStr_t name )			// static function
{
	char buf[5];
	buf[0] = ((char*) &name)[3];
	buf[1] = ((char*) &name)[2];
	buf[2] = ((char*) &name)[1];
	buf[3] = ((char*) &name)[0];
	buf[4] = '\0';
	return std::string ( buf );
}

Event::Event () : Event ( 0, 0 )
{
}

Event::Event ( size_t size, eventStr_t targ, eventStr_t name, eventStr_t state, EventPool* pool, const char* msg  )
{
	mTimeStamp = 0;
	mRefs = 0;
	mTargetID = NULL_TARGET;
	mTarget = targ;
	mName = name;
	mDataLen = 0;
	
	mCID = event_alloc;			// creation ID	
	mData = new_event_data ( size, mMax, pool, name, msg );	  // payload allocation	
	memset ( mData, 'E', mMax );
	
	mOwner = pool;
	bOwn = true;					// event retains ownership
	bDestroy = true;			
	mPos = mData;
}

Event::Event ( eventStr_t target, eventStr_t name)
{	
	memcpy(mScope, "emem", 4); mScope[4]='\0';
	mTimeStamp = 0;
	mRefs = 0;
	mTargetID = NULL_TARGET;
	mTarget = target;
	mName = name;
	mDataLen = 0;
	mMax = 0;	
	mData = 0x0;
	mPos = 0x0;	
	mOwner = 0x0;
	mCID = -1;
	bOwn = true;
	bDestroy = true;

	// check member variable structure (important!)
	int headersz = (char*) &mData - (char*) &mDataLen;
	if ( headersz != Event::staticSerializedHeaderSize() ) {
		//dbgprintf ( "  ERROR: Event member header not %d on this platform.\n", Event::staticSerializedHeaderSize() );
		dbgprintf ( "  ERROR: Event member header not %d on this platform.\n", Event::staticSerializedHeaderSize() );
	}
}

void Event::copyEventVars ( Event* dst, const Event* src )
{
	// this is NOT a deep copy. the data pointed to by mData is NOT copied here.
	// only the member variables reference to data are transferred.	
	dst->mName = src->mName;
	dst->mTarget = src->mTarget;
	dst->mCID = src->mCID;
	dst->mTimeStamp = src->mTimeStamp;	
	dst->mRefs = src->mRefs;
	dst->mSrcSock = src->mSrcSock;
	dst->mTargetID = src->mTargetID;			
	dst->mMax = src->mMax;	
	dst->bOwn = src->bOwn;	
	dst->bDestroy = src->bDestroy;
	dst->mDataLen = src->mDataLen;
	
	// data transfer of ownership
	dst->mOwner = src->mOwner;				
	dst->mData = src->mData;
	dst->mPos = mData;	

	for (int n=0; n < 5; n++)
		dst->mScope[n] = src->mScope[n];	
}

#ifdef DEBUG_EVENT_MEM
	#include <assert.h>
#endif

// Event const copy constructor does a deep copy
//  (unable to acquire the source event data by modifying src ownership)
Event::Event ( const Event& src )
{
	#ifdef DEBUG_EVENT_MEM
		printf ("WARN: Event const copy cannot acquire source event.\n" );			
	#endif

	copy ( src );
}

Event::Event ( Event& src )
{
	acquire ( src );		// transfer ownership	
}

Event& Event::operator= (Event* src)
{
	acquire(*src);		// transfer ownership
	return *this;
}
// called on direct assignment. eg. Event e = new_event (..)
Event& Event::operator= ( Event& src )
{
	acquire ( src );		// transfer ownership	
	return *this;
}

void Event::expand ( int size)
{
	expand_event ( *this, (size_t) size );
}

void Event::clear ()
{
	
	if ( mData != 0x0 ) {
		// reuse data if possible
		memset ( mData, 'C', mMax );
	} else {
		// new data
		mCID = event_alloc;	
		mData = new_event_data ( mMax, mMax, mOwner, mName, "clear" );
	}
	mPos = 0;	
}

Event::~Event ()
{
	// only free if owned and destroyed
	if ( bOwn && bDestroy && mData != 0x0 ) {
		free_event_data ( mData, mOwner, mName, mCID, "~event" );		
	}
	// now out of scope
	mData = 0x0;
	mPos = 0;	
	bOwn = false;
	bDestroy = false;
}

// trigger free at next opportunity
void Event::consume()
{
	bDestroy = true;
}
// persist data across func boundaries
void Event::persist ()
{
	bDestroy = false;
}

void Event::copy ( const Event& src )
{
	// don't copy self
	if (this == &src) return;

	// any prior data on this event is discarded
	if ( bOwn && mData != 0x0 ) {
		free_event_data ( mData, mOwner, mName, mCID, "~acq" );
		mData = 0x0;
	}	
	// copy vars
	copyEventVars ( this, &src );	

	// alloc new mem (always on copy)
	mCID = event_alloc;	
	mData = new_event_data ( mMax, mMax, mOwner, mName, "copy" );

	// deep copy
	memcpy ( mData, src.mData, src.mDataLen );
	mDataLen = src.mDataLen;
	mPos = mData;

	bOwn = true;
	bDestroy = true;
}

// acquire 
// - there can be only one owner of event data at a time
// - this func efficiently transfers ownership to another event (not a deep copy)
void Event::acquire ( Event& src)
{
	// any prior data on dest event is discarded
	if ( bOwn && mData != 0x0 ) {
		free_event_data ( mData, mOwner, mName, mCID, "~acq" );
		mData = 0x0;
	}	
	// transfer memory ptrs
	copyEventVars ( this, &src );		

	// dest becomes new owner	(given the right to expand/reallocate)
	mData = src.mData;
	bOwn = true;		

	// src must be detached from data (to avoid bad pointers)
	src.mData = 0x0;
	src.mPos = 0; 
	src.mMax = 0;
	src.mDataLen = 0;
	src.bOwn = false;
	src.bDestroy = false;
}

void Event::setName ( eventStr_t new_name, const char* new_msg )
{	
	#ifdef DEBUG_EVENT_MEM
		emem_rename ( *this, mName, new_name, new_msg );
	#endif
	mName = new_name; 
}


void Event::attachInt ( int i)
{
	if ( mDataLen+sizeof(int) > mMax ) expand ( mDataLen*2 );
	* (int*) mPos = i;
	mPos += sizeof(int);
	mDataLen += sizeof(int);
}
void Event::attachUChar (unsigned char i)
{
	if ( mDataLen+sizeof(char) > mMax ) expand ( mDataLen*2 );
	* (unsigned char*) mPos = i;
	mPos += sizeof(char);
	mDataLen += sizeof(char);
}
void Event::attachShort (short i)
{
	if ( mDataLen+sizeof(signed short) > mMax ) expand ( mDataLen*2 );
	* (signed short*) mPos = i;
	mPos += sizeof(signed short);
	mDataLen += sizeof(signed short);
}

void Event::attachUShort ( unsigned short i)
{
	if ( mDataLen+sizeof(short) > mMax ) expand ( mDataLen*2 );
	* (unsigned short*) mPos = i;
	mPos += sizeof(signed short);
	mDataLen += sizeof(signed short);
}

void Event::attachUInt (unsigned int i)
{
	if ( mDataLen+sizeof(int) > mMax ) expand ( mDataLen*2 );
	* (unsigned int*) mPos = i;
	mPos += sizeof(int);
	mDataLen += sizeof(int);
}
void Event::attachULong (unsigned long i)
{
	if ( mDataLen+sizeof(unsigned long) > mMax ) expand ( mDataLen*2 );
	* (unsigned long*) mPos = i;
	mPos += sizeof(unsigned long);
	mDataLen += sizeof(unsigned long);
}

void Event::attachInt64 (xlong i)
{
	if ( mDataLen+sizeof(xlong) > mMax ) expand ( mDataLen*2 );
	* (xlong*) mPos = i;
	mPos += sizeof(xlong);
	mDataLen += sizeof(xlong);
}
void Event::attachFloat (float f)
{
	if ( mDataLen+sizeof(float) > mMax ) expand ( mDataLen*2 );
	* (float*) mPos = f;
	mPos += sizeof(float);
	mDataLen += sizeof(float);
}

void Event::attachDouble (double f)
{
	if ( mDataLen+sizeof(double) > mMax ) expand ( mDataLen*2 );
	* (double*) mPos = f;
	mPos += sizeof(double);
	mDataLen += sizeof(double);
}

void Event::attachBool ( bool b)
{
	if ( mDataLen+sizeof(bool) > mMax ) expand ( mDataLen*2 );
	* (bool*) mPos = b;
	mPos += sizeof(bool);
	mDataLen += sizeof(bool);
}
void Event::attachVec4 ( Vec4F v )
{
	attachFloat ( v.x );
	attachFloat ( v.y );
	attachFloat ( v.z );
	attachFloat ( v.w );
}


void Event::attachStr (std::string str )
{
	int len = str.length();
	attachInt ( len );
	if ( str.length() > 0 ) {
		if ( mDataLen + len > mMax ) expand ( mMax*2 + len );
		memcpy ( (char*) mPos, str.c_str(), len );
		mPos += len;
		mDataLen += len;
	}
}
void Event::attachPrintf (const char* format, ... )
{
	char buf[8192];
	va_list argptr;
    va_start(argptr, format);
    vsprintf( buf, format, argptr);
    va_end(argptr);
	attachStr ( buf );
}

void Event::attachMem (char* buf, int len )
{
	attachInt ( len );
	if ( mDataLen + len > mMax ) expand ( mMax*2 + len );
	memcpy ( mPos, buf, len );
	mPos += len;
	mDataLen += len;
}
void Event::attachBuf (char* buf, int len )
{
	if ( mDataLen + len > mMax ) expand ( mMax*2 + len );
	memcpy ( mPos, buf, len );
	mPos += len;
	mDataLen += len;
}
void Event::attachFromFile  (FILE* fp, int len )
{
	if ( mDataLen + len > mMax ) expand ( len );
	fread ( mPos, 1, len, fp );
	mPos += len;
	mDataLen += len;
}

// attach file to event
//
bool Event::attachFile (std::string fullpath)
{
	// Open file
	char fpath[2048];
	strncpy(fpath, fullpath.c_str(), 2048);
	FILE* fp = fopen(fpath, "rb");
	if (fp == 0x0) return false;

	// Get file size	
	fseek(fp, 0, SEEK_END);
	size_t sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);		// reset pos

	// Load file
	char* buf = (char*) malloc(sz);
	if (buf==0x0) {fclose(fp); return false;}
	fread (buf, sz, 1, fp);

	// Attach file to event as int & buffer
	attachInt ( sz );
	attachBuf ( buf, sz );

	fclose ( fp );
	free ( buf ); 
	return true;
}

// read file from event
//
int Event::getFile(std::string fullpath)
{
	char fpath[2048];
	strncpy(fpath, fullpath.c_str(), 2048);

	// Get file size from event
	int sz = getInt();

	// Allocate memory
	char* buf = (char*)malloc(sz);
	if (buf == 0x0) return false;
	getBuf ( buf, sz );

	// Write file
	FILE* fp = fopen ( fpath, "wb" );
	if ( fp==0x0 ) return 0;
	fwrite ( buf, sz, 1, fp );
	fclose ( fp );
	free ( buf );

	return sz;
}


void Event::attachBufAtPos (int pos, char* buf, int len )
{
	if ( pos+len > mMax  )
		expand ( imax(mMax*2, pos+len) );

	mPos = getData() + pos;
	memcpy ( mPos, buf, len );
	mPos += len;
	if ( pos+len > mDataLen ) mDataLen = pos+len;
}

Vec4F Event::getVec4 ()
{
	Vec4F v;
	v.x = getFloat ();
	v.y = getFloat ();
	v.z = getFloat ();
	v.w = getFloat ();
	return v;
}

void Event::writeUShort (int pos, unsigned short i)
{
	* (unsigned short*) (getData() + pos) = i;
}

int Event::getInt ()
{
	int i = * (int*) mPos;
	mPos += sizeof(int);
	return i;
}
unsigned char Event::getUChar ()
{
	unsigned char i = * (unsigned char*) mPos;
	mPos += sizeof(unsigned char);
	return i;
}
signed short Event::getShort ()
{
	signed short i = * (signed short*) mPos;
	mPos += sizeof(signed short);
	return i;
}
unsigned short Event::getUShort ()
{
	unsigned short i = * (unsigned short*) mPos;
	mPos += sizeof(unsigned short);
	return i;
}
unsigned int Event::getUInt ()
{
	unsigned int i = * (unsigned int*) mPos;
	mPos += sizeof(unsigned int);
	return i;
}
unsigned long Event::getULong ()
{
	unsigned long i = * (unsigned long*) mPos;
	mPos += sizeof(unsigned long);
	return i;
}

xlong Event::getInt64 ()
{
	xlong i = * (xlong*) mPos;
	mPos += sizeof(xlong);
	return i;
}
float Event::getFloat ()
{
	float f = * (float*) mPos;
	mPos += sizeof(float);
	return f;
}
double Event::getDouble ()
{
	double d = * (double*) mPos;
	mPos += sizeof(double);
	return d;
}
bool Event::getBool ()
{
	bool b = * (bool*) mPos;
	mPos += sizeof(bool);
	return b;
}

std::string Event::getStr ()
{
	if ( mPos-mData >= mDataLen ) return "EVENT READ OVERFLOW";
	int i = getInt ();
	if ( i > mDataLen ) {
		dbgprintf ("ERROR: Event string length is corrupt.\n");
		return "ERROR";
	}
	if ( i > 0 ) {
		// dbgprintf  ( "  len: %d npos: %d\n", i, (int) (mPos-getData()) );
		memcpy ( mbuf, mPos, i ); *(mbuf + i) = '\0';
		mPos += i;
		if ( mPos >= getData() + mDataLen ) mPos = getData() + mDataLen;
		return std::string ( mbuf );
	}
	return "";
}

void Event::getStr (char* str)
{
	if ( mPos-mData >= mDataLen ) {strcpy(str, "EVENT READ OVERFLOW"); return; }
	int i = getInt ();
	if ( i > 0 ) {
		strncpy ( str, mPos, i ); str[i] = '\0';
		mPos += i;
		if ( mPos >= getData() + mDataLen ) mPos = getData() + mDataLen;
	}
}

void Event::getMem ( char* buf, int maxlen )
{
	int len = getInt ();
	if ( mPos >= getData() + mDataLen ) return;
	memcpy ( buf, mPos, imin ( len, maxlen ) );
	mPos += len;
}
void Event::getBuf ( char* buf, int len )
{
	if ( mPos >= getData() + mDataLen ) return;
	memcpy ( buf, mPos, len );
	mPos += len;
}

void Event::getBufAtPos ( int pos, char* buf, int len )
{
	memcpy ( buf, getData() + pos, len ) ;
}

void Event::startRead ()
{
	mPos = mData;
}
void Event::startWrite ()
{
	mPos = mData;
	mDataLen = 0;
}

std::string	Event::getNameStr ()
{
	return nameToStr ( getName () );
}
std::string	Event::getSysStr ()
{
	return nameToStr ( mTarget );
}
char* Event::serialize ()
{
	// start of serialized header information (member vars) for this event
	char* header = (char*) this + Event::staticOffsetLenInfo();

	// memory location where data is being prepared with payload
	char* serial_data = mData - Event::staticSerializedHeaderSize();

	// transfer current serialized header values into payload area
	memcpy ( serial_data, header, Event::staticSerializedHeaderSize() );

	// data is now complete. attachments in payload are already serialized

	return serial_data;
}

void Event::deserialize (char* buf, int serial_len )
{
	int cid = mCID;		// save cid

	// NOTE: Incoming buffer includes serialized event header. Serial length is payload + header.	
	const int hsz = Event::staticSerializedHeaderSize();

	// Transfer payload into mData pointer
	// mData is allocated with extra header space for serialization. See: new_event_data
	memcpy ( mData - hsz, buf, serial_len );

	// Transfer serialized header into Event pointer (not the same as mData pointer)
	memcpy ( (char*) this + Event::staticOffsetLenInfo(), buf, hsz );

	// Update payload length and read/write pos
	mDataLen = serial_len - hsz;
	mPos =		mData + mDataLen;

	mCID = cid;			// restore cid
}

void Event::setTime ( unsigned long t )
{
	mTimeStamp.SetSJT ( t );
}

