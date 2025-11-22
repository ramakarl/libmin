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

#include "event_system.h"

#include <assert.h>
#include <cmath>
#include <stdio.h>

int event_alloc = 0;
int event_free = 0;
#ifdef DEBUG_EVENT_MEM
	vecTrack_t event_tracks;
#endif

//	void assign( eventPtr src, bool v )	{ e = src.e; memcpy(scope,src.scope,4); bOwn = v; }
//	void rescope( char* s )				{ memcpy(scope,s,4); }

char* new_event_data ( size_t size, int& max, EventPool* pool, eventStr_t name, const char* msg )
{
	char* data;

	max = size;										// maximum *payload* size

	size += Event::staticSerializedHeaderSize();	// additional memory for serial header

	// allocate payload
	if ( size > MAX_POOL_SIZE || pool==0x0 ) {
		
		// standard allocation
		data = (char*) malloc ( size );
		if ( data==0 ) {
			dbgprintf ("ERROR: Unable to allocate large event.\n");
			exit(-2);
		}

		// event memory debugging
		#ifdef DEBUG_EVENT_MEM
			emem_track_alloc ( data, event_alloc, name, msg );
		#endif
		event_alloc++;		
		
	} else {
		#ifdef BUILD_EVENT_POOLING
			// optional Event Pooling
			data = (char*) pool->allocItem ( (int) size );
			max = pool->getItemMaxSize ( (int) size );
			max -= Event::staticSerializedHeaderSize();
		#endif
	}
	// serial header is automatically set aside (precedes the data start pos)
	// return the start of payload area
	return data + Event::staticSerializedHeaderSize();
}

void new_event ( Event& p, size_t size, eventStr_t targ, eventStr_t name, eventStr_t state, EventPool* pool, const char* msg )
{
	// close out old mem tag
	#ifdef DEBUG_EVENT_MEM
		if (p.mData != 0x0 ) {
			emem_track_free ( p.mData, p.mCID, p.mName, msg );
		}
	#endif
	
	// reuse event p
	p.mTimeStamp = 0;
	p.mRefs = 0;
	p.mTargetID = NULL_TARGET;
	p.mTarget = targ;
	p.mName = name;
	p.mDataLen = 0;
	p.mCID = event_alloc;			// creation ID
	
	// reuse payload
	if (p.mData == 0x0 || size > p.mMax ) {
		p.mData = new_event_data ( size, p.mMax, pool, name, msg );	  // payload allocation			
	}
	// memset ( p.mData, '0', p.mMax );			//--- debugging

	p.bOwn = true;					// event retains ownership
	p.bDestroy = true;			// default to kill on local func out of scope
	p.mPos = p.mData;
}

void free_event ( Event& p, const char* msg )
{
	if ( p.bOwn && p.mData != 0x0 ) {
		free_event_data ( p.mData, p.mOwner, p.mName, p.mCID, msg );
		p.mData = 0x0;
	}
	p.bOwn = false;
	p.bDestroy = false;
}

void expand_event (Event& p, size_t new_size)
{
	EventPool* pool = p.mOwner;	
	int old_pos = p.getPos() - p.getData();
	char* new_data = p.mData;
	int new_max = p.mMax;

	if ( p.mData == 0x0 ) {
		// new buffer needed
		new_data = new_event_data ( new_size, new_max, pool, p.mName, "exp" );

	} else if ( new_size > p.mMax ) {
		// copy to new buffer
		new_data = new_event_data ( new_size, new_max, pool, p.mName, "exp" );
		memcpy ( new_data, p.mData, p.mDataLen );		// will overwrite the event pool var
		// free old payload memory
		free_event_data ( p.mData, pool, p.mName, p.mCID, "exp" );
	}

	// update event
	p.mMax = new_max;
	p.mData = new_data;
	p.mPos = new_data + old_pos;
}

void free_event_data ( char*& data, EventPool* pool, eventStr_t name, int cid, const char* msg )
{
	if ( data == 0x0 ) return;		// nothing to free

	// adjust back to the original allocation pointer
	data -= Event::staticSerializedHeaderSize();

	if ( pool == 0x0 ) {

		// event memory debugging
		event_free++;		
		#ifdef DEBUG_EVENT_MEM	
			emem_track_free ( data, cid, name, msg );
		#endif
		
		// free event data
		free ( data );

	} else {
		#ifdef BUILD_EVENT_POOLING
			pool->freeItem ( data );
		#endif
	}	

	data = 0x0;
}

//------------------ event memory debugging
//
#ifdef DEBUG_EVENT_MEM
	void emem_track_alloc ( char* data, int cid, eventStr_t name, const char* msg ) 
	{
		if (msg==0) {
			bool stop1=true;		// breakpoint here to see call stack for empty msg (calling func)
		}
		if (nameToStr(name).empty()) {
			bool stop2=true;		// breakpoint here to see call stak for unnamed events
		}
		std::string tag = std::to_string(cid) + ":" + nameToStr(name);
		event_tracks.push_back ( eventTrack(tag,msg) );
		printf ( "%p: +%s, %d/%d +%d, %s\n", data, tag.c_str(), event_alloc, event_free, event_alloc-event_free, msg );			
	}

	void emem_track_free ( char* data, int cid, eventStr_t name, const char* msg ) 
	{
		std::string tag = std::to_string( cid ) + ":" + nameToStr(name);
		int found = -1;
		for (int i=0; i < event_tracks.size(); i++) {
			if (event_tracks[i].tag == tag ) {found = i; break;}		// only match tag part		
		}
		if ( found >= 0 ) {
			event_tracks.erase ( event_tracks.begin() + found );		// remove from list
		}
		printf ( "%p: -%s, %d/%d +%d, %s\n", data, tag.c_str(), event_alloc, event_free, event_alloc-event_free, msg );					
	}

	void emem_rename ( Event& e, eventStr_t oldname, eventStr_t newname, const char* msg )
	{
		if (e.mData == 0x0 ) return;
		emem_track_free ( e.mData, e.mCID, oldname, msg );
		emem_track_alloc ( e.mData, e.mCID, newname, msg );
	}

	void emem_check ()
	{
		#ifdef DEBUG_EVENT_MEM
			printf ( "----- unfreed events\n" );	
			for (int i=0; i < event_tracks.size(); i++) {
				printf ( " %s, %s\n", event_tracks[i].tag.c_str(), event_tracks[i].msg.c_str() );
			}
			printf ( "-----\n" );
		#endif
	}
#endif


//---------------------------------------------- Event Queue

EventQueue::EventQueue ()
{
	bool e = mList.empty ();
	mTraceFile = 0x0;
}

void EventQueue::startTrace ( char* fn )
{
	mTraceFile = fopen ( fn, "w+t" );
}
// safe pull of front event 
// (event acquire, does not call const copy)
void EventQueue::PopFront ( Event*& dest )
{
	dest = mList.front ();			// get first element
	mList.pop ();								// pop from list
}

void EventQueue::Push ( Event* e )
{
	e->incRefs ();				// Added to queue. Increment ref count.
	mList.push ( e );
}

void EventQueue::Push_back ( Event* e )
{
	std::queue < Event* >	q;

	while ( mList.size() > 0 ) {
		q.push ( mList.front() );
		mList.pop ();
	}
	mList.push ( e );
	while ( q.size() > 0 ) {
		mList.push ( q.front() );
		q.pop ();
	}
}

EventQueue& EventQueue::operator= ( EventQueue &op )
{
	Event e;
	while ( op.getSize() > 0 ) {
		e = op.mList.front();
		mList.push ( &e );
		op.mList.pop();
	}
	return *this;
}

void EventQueue::Clear ()
{
	while ( mList.size() > 0 )
		mList.pop ();
}


void EventQueue::trace ()
{
	Event e;
	std::queue<Event> temp;

	if ( mTraceFile == 0x0 ) return;

	fflush ( mTraceFile );

	while (!mList.empty()) {
		e = mList.front ();
		fprintf ( mTraceFile, "%s ", e.getNameStr().c_str() );
		fflush ( mTraceFile );
		temp.push ( e );
		mList.pop ();
	}
	fprintf ( mTraceFile, "\n");
	fflush ( mTraceFile );
	while (!temp.empty() ) {
		e = temp.front ();
		mList.push ( &e );
		temp.pop ();
	}
}


#ifdef BUILD_EVENT_POOLING

//------------------------------------------- EVENT POOLING [optional]

EventPool :: EventPool()
{
	// Preallocate empty bins.
	for (int n=0; n < BIN_CNT; n++) {
		mFullBins[n] = 0x0;
		mEmptyBins[n] = 0x0;
	}
	for (int n=0; n < BIN_CNT; n++)
		addBlock ( n );
}
EventPool :: ~EventPool()
{
	clear ();
}

void EventPool::clear ()
{
	blockPtr block;
	blockPtr next;

	for (int n=0; n < BIN_CNT; n++) {
		block = mFullBins[n];
		while (block != 0x0) {			// traverse linked list
			next = block->mNext;
			free ( block );
			block = next;
		}
		block = mEmptyBins[n];
		while (block != 0x0) {			// traverse linked list
			next = block->mNext;
			free ( block );
			block = next;
		}
		mFullBins[n] = 0x0;
		mEmptyBins[n] = 0x0;
	}
}

// Allocate.
// * Compiles down to 11 asm ops *
void* EventPool::allocItem ( int size )
{
	#ifdef DEBUG_MEMPOOL
		integrity();
	#endif

	size += sizeof(uint32_t);

	if ( size > MAX_POOL_SIZE ) {
		dbgprintf ( "  ERROR: Alloc size, %d, greater than MAX_POOL_SIZE (%d).\n", size, MAX_POOL_SIZE );
	}
	// include space for freebyte
	register blockPtr block = mEmptyBins[ getBin ( size ) ];	// Determine bin & block (given alloc size)

	if ( block->mPos == block->mEnd )		// Check if position is beyond end of block
		block = makeFull ( block );			//    ( if so, make full and make new block )

	block->mPos += block->mWidth;			// Get next item position
	block->mUsed++;							// Increment used items in block

	#ifdef MEM_CHECK
		if ( block->mUsed > block->mCount ) {
			dbgprintf ( "Bad end position. %p %p (%d/%d)\n", block->mPos, block->mEnd, block->mUsed, block->mCount );
			debug.Exit ( -2 );
		}
		dbgprintf ( 'memp', INFO, "  alloc: %p %d/%d, pos %d, item %p\n", block, block->mUsed, block->mCount, (char*) block->mPos - (char*) block, block->mPos );
		checkBlock ( block );
	#endif
	return block->mPos - block->mWidth;
}

// Free
void EventPool::freeItem ( void* item )
{
	#ifdef DEBUG_MEMPOOL
		integrity();
	#endif
	uint32_t* freeword = (uint32_t*) ((char*) item - sizeof(uint32_t));
	if ( *freeword == 0xFFFF ) { dbgprintf ( "Already freed, %p\n", item );  }
	register blockPtr block = (blockPtr) ((char*) item - *freeword);	// find the start of block

	*freeword = 0xFFFF;							// mark item as freed

	block->mUsed--;								// Decrement used items in block
	if ( block->mUsed == 0 ) {					// Check if used items goes to zero
		block = makeFree ( block );				//    (if so, free the block)
	}
	#ifdef MEM_CHECK
		printE ( 'memp', INFO, "   free: %p %d/%d, pos %d, item %p\n", block, block->mUsed, block->mCount, (char*) item - (char*) block, item );
		checkBlock ( block );
	#endif
}

// Add Block
blockPtr EventPool::addBlock ( int bin )
{
	// block = add block to front of EmptyBin list
	blockPtr block = (blockPtr) malloc ( getHeaderSize() + BLOCK_SIZE );	// Malloc block
	#ifdef MEM_CHECK
		dbgprintf ( 'memp', INFO,"  BLOCK addBlock: %p\n", block );
	#endif

	if ( block == 0x0 ) {
		dbgprintf ( "ERROR: Out of memory.\n" );
	}
	int w = getBlockWidth ( bin );				// Determine block bin, width & count
	int c = getItemCount ( bin );
	if ( mEmptyBins[bin] == 0x0 ) {				// Setup header
		setBlockHeader ( block, w, c, bin );
		mEmptyBins[bin] = block;
	} else {
		blockPtr next = mEmptyBins[bin];
		setBlockHeader ( block, w, c, bin);
		next->mPrev = block;
		block->mNext = next;
		mEmptyBins[bin] = block;
	}
	// Set freewords on all items
	// - Header item does not have a freeword
	// - Freeword is 4 bytes *before* the item, invading the end of previous item
	// - Freeword stores the negative offset to find the block (during freeItem)
	uint32_t* freeword = (uint32_t*) ((char*) block + getBlockWidth(bin) - sizeof(uint32_t));
	for (int n=0; n < c; n++) {
		*freeword = (char*) (freeword+1) - (char*) block;	// freeword+1 = next 4 bytes
		freeword += w / sizeof(uint32_t);				// next freeword (in units of uint32_t)
	}
	#ifdef MEM_CHECK
		integrity ();
	#endif
	return block;
}

// Make Block a Full block
blockPtr EventPool::makeFull ( blockPtr block )
{
	// block = guaranteed to be the front (head) of the EmptyBin linked list
	#ifdef MEM_CHECK
		dbgprintf ( "  BLOCK makeFull: %p\n", block );
	#endif

	int bin = block->mBin;
	blockPtr next_empty = block->mNext;

	// Make current block full
	//   (linked list add to front of Full)
	block->mbFull = true;					// Mark as full
	if ( mFullBins[bin] == 0x0 ) {
		block->mNext = 0x0;
	} else {
		blockPtr next = mFullBins[bin];		// Linked list insert into full bin
		next->mPrev = block;
		block->mNext = next;
	}
	block->mPrev = 0x0;
	mFullBins[bin] = block;

	// Make sure we still have empty blocks
	//   (linked list remove from front of Empty)
	if ( next_empty == 0x0 ) {
		mEmptyBins[bin] = 0x0;
		next_empty = addBlock ( bin );
	} else {
		next_empty->mPrev = 0x0;
		mEmptyBins[bin] = next_empty;
	}
	#ifdef MEM_CHECK
		integrity ();
	#endif
	return mEmptyBins[bin];
}


// Make Block a Free block
blockPtr EventPool::makeFree ( blockPtr block )
{
	// block = any block, in either Full or Empty list
	#ifdef MEM_CHECK
		dbgprintf ( "  BLOCK makeFree: %p\n", block );
	#endif

	int bin = block->mBin;
	blockPtr prev = block->mPrev;
	blockPtr next = block->mNext;

	if ( next != 0x0 ) {		// Linked list remove from free or full bin (block may be in either)
		next->mPrev = prev;
	}
	if ( prev != 0x0 ) {
		prev->mNext = next;
	} else {
		// First block
		if ( block->mbFull ) {
			mFullBins[bin] = 0x0;
		} else {
			mEmptyBins[bin] = 0x0;
		}
	}
	block->mPrev = 0x0;					// (just to be safe)
	block->mNext = 0x0;
	free ( block );						// Free the block

	#ifdef MEM_CHECK
		integrity ();
	#endif

	if ( mEmptyBins[bin] == 0x0 )
		addBlock ( bin );			// Allocate new empty bin if necessary
	return mEmptyBins[bin];
}

// Setup block header
void EventPool::setBlockHeader ( blockPtr block, int wid, int count, int bin )
{
	block->mMagic = 'LUNA';

	block->mPos = (itemPtr) block + wid;			// first item position skips the header item
	block->mEnd = (itemPtr) block + (count-1)*wid;
	block->mUsed = 0;
	block->mbFull = false;
	block->mBin = bin;
	block->mWidth = wid;
	block->mCount = count;
	block->mPrev = 0x0;
	block->mNext = 0x0;
}

int EventPool::checkBlock ( blockPtr block )
{
	int w = block->mWidth;
	int offset;

	uint32_t* freeword = (uint32_t*) ((char*) block + getBlockWidth(block->mBin) - sizeof(uint32_t));
	char* item = (char*) block + w;
	for (int n=0; n < block->mCount; n++) {
		if ( *freeword == 0xFFFF) continue;		// skip this one
		blockPtr block_chk = (blockPtr) (item - (char*) *freeword);	 // find the start of block
		if ( block_chk != block ) {
			dbgprintf ( "ERROR: Block %d, Item %d, Block (correct) %p, Freeword (wrong) %p\n", n, item, block, block_chk );
			return 0;
		}
		freeword += w / sizeof(uint32_t);		// next freeword (in units of uint32_t)
		item += w;								// next item
	}
	return block->mCount;
}


void EventPool::integrity ()
{
	char msg[50];
	blockPtr block;

	dbgprintf ( "MEMPOOL INTEGRITY\n" );
	int iOk;
	for (int n=0; n < BIN_CNT; n++) {
		block = mEmptyBins[n];
		dbgprintf ( "  Bin: %d, wid %d, cnt %d. ", n, getItemWidth(n), block->mUsed );
		iOk = 0;
		while (block != 0x0) {			// traverse linked list
            /*#ifdef _MSC_VER
			sprintf_s ( msg, 50, " %p (%d), ", block, block->mUsed );
            #else
			snprintf ( msg, 50, "%p (%d), ", block, block->mUsed );
            #endif
			debug.Print ( msg );			*/
			iOk += checkBlock ( block );
			block = block->mNext;
		}
		dbgprintf ( "%s. %d\n", iOk>0 ? "OK" : "BAD", iOk );

		/*printf ( "  Empty (bin %d, %05d): ", n, getItemWidth(n) );
		block = mEmptyBins[n];
		while (block != 0x0) {			// traverse linked list
            #ifdef _MSC_VER
			sprintf_s ( msg, 50, "%p (%d), ", block, block->mUsed );
            #else
			snprintf ( msg, 50, "%p (%d), ", block, block->mUsed );
            #endif
			debug.Print ( msg );
			checkBlock ( block );
			block = block->mNext;
		}
		debug.Print ( "\n" );*/
	}
}

// Compute number of allocated items in a bin
// (Add all items in both full and in use lists)
int EventPool::getAllocated ( int bin )
{
	blockPtr block;
	int bin_cnt=0;

	block = mFullBins[bin];
	while (block != 0x0) {
		bin_cnt += block->mUsed;
		block = block->mNext;
	}
	block = mEmptyBins[bin];
	while (block != 0x0) {
		bin_cnt += block->mUsed;
		block = block->mNext;
	}
	return bin_cnt;
}



//-- Standard log table. Matches width to bin
const char EventPool::logtable512[] = {
 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
};

unsigned char EventPool::getBin ( unsigned int v )
{
	// Find integer log base 2 of given width (v).
	return logtable512[ ((v-1) >> MIN_WIDTH_BITS) ];	// max ( 0, ceil[ log2(v / 64) ] )
}


#endif
