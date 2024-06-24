//--------------------------------------------------------------------------------
// Copyright 2007 (c) Quanta Sciences, Rama Hoetzlein, ramakarl.com
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

	// TimeX here..
	#include "timex.h"

	#ifdef _WIN32
		#include <io.h>
		#include <fcntl.h>	
		#include <conio.h>
		#include <timeapi.h>
	#elif __ANDROID__
		#include <fcntl.h>
        #include <android/log.h>
	#elif __linux__
			#include <fcntl.h>
			#include <stdarg.h>    // for va_start, va_args
			#include <string.h>    // for strncpy
			#include <stdlib.h>    // for atoi  
	#endif




	#ifdef USE_NVTX
		#include <nvToolsExt.h>
		nvtxRangePushFunc	g_nvtxPush = 0x0;			// Pointer to nv-perfmarker func
		nvtxRangePopFunc	g_nvtxPop = 0x0;			// Pointer to nv-perfmarker func
	#else
		void (*g_nvtxPush)(char*) = 0x0;
		void (*g_nvtxPop)() = 0x0;
	#endif

	bool				g_perfInit = false;			// Is perf started? Checks for DLL
	bool				g_perfOn = false;			// Is perf on? DLL was found. Otherwise no perf.
	int					g_perfLevel = 0;			// Current level of push/pop
	sjtime				g_perfStack[1024];			// Stack of recorded CPU timings
	char*				g_perfMsg[1024][256];		// Stack of recorded messages
	FILE*				g_perfCons = 0x0;			// On-screen console to output CPU timing
	bool				g_perfConsOut = false;
	int					g_perfPrintLev = 2;			// Maximum level to print. Set with PERF_SET
	bool				g_perfCPU = false;			// Do CPU timing? Set with PERF_SET
	bool				g_perfGPU = false;			// Do GPU timing? Set with PERF_SET
	std::string			g_perfFName = "";			// File name for CPU output. Set with PERF_SET
	FILE*				g_perfFile = 0x0;			// File handle for output


	void PERF_PRINTF ( char* format, ... )
	{
		if ( g_perfCons == 0x0 ) {
				va_list  vlist;
			va_start(vlist, format);    
			#if defined(__ANDROID__)
				__android_log_vprint(ANDROID_LOG_DEBUG, "NAPP", format, vlist );
			#elif defined(__linux__)
				vprintf(format, vlist);
			#elif defined(_WIN32)
				vprintf(format, vlist);
			#endif	
		} else {
			va_list argptr;
			va_start (argptr, format);				
			vfprintf ( g_perfCons, format, argptr);			
			va_end (argptr);			
			fflush ( g_perfCons );
		} 
	}

	void PERF_PUSH ( const char* msg )
	{
		if ( !g_perfInit ) return;
		if ( g_perfOn ) {
			strncpy ( (char*) g_perfMsg[g_perfLevel], msg, 256 );			
			if ( g_perfGPU ) (*g_nvtxPush) ( (char*) g_perfMsg[g_perfLevel] );
			if ( g_perfCPU && g_perfLevel < g_perfPrintLev ) {			
				PERF_PRINTF ( "%*s%s\n", g_perfLevel <<1, "", g_perfMsg[g_perfLevel] );
				if ( g_perfFile != 0x0 ) fprintf ( g_perfFile, "%*s%s\n", g_perfLevel <<1, "", g_perfMsg[g_perfLevel]  );
				g_perfStack [ g_perfLevel ] = TimeX::GetSystemNSec ();
			}
			g_perfLevel++;
		} 
	}
	float PERF_POP ()
	{
		if ( g_perfOn ) {
			if ( g_perfGPU ) (*g_nvtxPop) ();
			g_perfLevel--;
			if ( g_perfCPU && g_perfLevel < g_perfPrintLev ) {
				sjtime curr = TimeX::GetSystemNSec ();
				curr -= g_perfStack [ g_perfLevel ];
				float msec = ((float) curr)/MSEC_SCALAR;
				PERF_PRINTF ( "%*s%s: %f ms\n", g_perfLevel <<1, "", g_perfMsg[g_perfLevel], msec );		
				//if ( g_perfFile != 0x0 ) fprintf ( g_perfFile, "%*s%s: %f ms\n", g_perfLevel <<1, "", g_perfMsg[g_perfLevel], msec );
				return msec;
			}
		} 
		return 0.0;
	}

	void PERF_START ()
	{
		g_perfStack [ g_perfLevel ] = TimeX::GetSystemNSec ();
		g_perfLevel++;
	}

	float PERF_STOP ()
	{
		g_perfLevel--;
		sjtime curr = TimeX::GetSystemNSec ();
		curr -= g_perfStack [ g_perfLevel ];
		return ((float) curr) / MSEC_SCALAR;
	}

	void PERF_SET ( bool cpu, int lev, bool gpu, char* fname )
	{
		g_perfCPU = cpu;
		if ( lev == 0 ) lev = 32767;
		g_perfPrintLev = lev;
		g_perfGPU = gpu;		
		g_perfFName = fname;	
		if ( g_perfFName.length() > 0 ) {
			if ( g_perfFile == 0x0 ) g_perfFile = fopen ( g_perfFName.c_str(), "wt" );
		}	
	}

	void PERF_INIT ( int buildbits, bool cpu, bool gpu, bool cons, int lev, const char* fname )
	{
		g_perfCPU = cpu;
		g_perfGPU = gpu;
		g_perfConsOut = cons;
		if ( lev == 0 ) lev = 32767;
		g_perfPrintLev = lev;
		g_perfInit = true;
		g_perfLevel = 0;	
		g_perfFile = 0x0;
		g_perfFName = fname;	
		if ( g_perfFName.length() > 0 ) {
			if ( g_perfFile == 0x0 ) g_perfFile = fopen ( g_perfFName.c_str(), "wt" );
		}
	
		#if defined(WIN32)
			// Address of NV Perfmarker functions	
			char libname[128];	
			if ( buildbits == 64 ) {
				strcpy ( libname, "nvToolsExt64_1.dll" );		
			} else {
				strcpy ( libname, "nvToolsExt32_1.dll" );
			}
			#ifdef USE_NVTX
				#ifdef UNICODE
					wchar_t libwc[128];
					MultiByteToWideChar(CP_ACP, 0, libname, -1, libwc, 8192);   		
					LoadLibrary ( libwc );  		
					HMODULE mod = GetModuleHandle( libwc );
				#else
					LoadLibrary ( libname );  		
					HMODULE mod = GetModuleHandle( libname );
				#endif	
				g_nvtxPush = (nvtxRangePushFunc) GetProcAddress( mod, "nvtxRangePushA");
				g_nvtxPop  = (nvtxRangePopFunc)  GetProcAddress( mod, "nvtxRangePop");
			#else
				PERF_PRINTF("WARNING: GPU markers not enabled for GVDB Library. Set cmake flag USE_NVTX.\n");
				g_perfGPU = false;
			#endif

			// Console window for CPU timings
			if ( g_perfCPU ) {
				/*AllocConsole ();
				long lStdHandle = (long) GetStdHandle( STD_OUTPUT_HANDLE );
				int hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
				g_perfCons = _fdopen( hConHandle, "w" );
				setvbuf(g_perfCons, NULL, _IONBF, 1);
				*stdout = *g_perfCons; */
				PERF_PRINTF ( "PERF_INIT: Enabling CPU markers.\n" );
			} else {
				PERF_PRINTF ( "PERF_INIT: No CPU markers.\n" );
			}
			if ( g_perfGPU ) {
				if ( g_nvtxPush != 0x0 && g_nvtxPop != 0x0 ) {
					PERF_PRINTF ( "PERF_INIT: Enabling GPU markers. Found %s.\n", libname );			
				} else {			
					PERF_PRINTF ( "PERF_INIT: Disabling GPU markers. Did not find %s.\n", libname );			
					g_perfGPU = false;
				}		
			} else {
				PERF_PRINTF ( "PERF_INIT: No GPU markers.\n" );
			}
			g_perfOn = g_perfCPU || g_perfGPU;
			if ( !g_perfOn ) {
				PERF_PRINTF ( "PERF_INIT: Disabling perf. No CPU or GPU markers.\n" );			
			}
		#else
			#ifdef USE_NVTX
				g_nvtxPush = nvtxRangePushA;
				g_nvtxPop = nvtxRangePop;
			#endif
			g_perfCons = 0x0;		
		#endif

		TimeX start;		// create Time obj to initialize system timer
	}

	//---------------- TIMING CLASS
	// R.Hoetzlein

	#ifdef _MSC_VER
		#include <windows.h>
	#else
		#include <sys/time.h>
	#endif 

	#include <stdio.h>
	#include <time.h>
	#include <math.h>

	#ifdef _MSC_VER
		#define VS2005
		#pragma comment ( lib, "winmm.lib" )
		LARGE_INTEGER	m_BaseCount;
		LARGE_INTEGER	m_BaseFreq;
	#endif

	const int TimeX::m_DaysInMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	bool TimeX::m_Started = false;
	sjtime			m_BaseTime;
	sjtime			m_BaseTicks;

	void start_basetime ( sjtime base )
	{	
		m_BaseTime = base;

		#ifdef _MSC_VER
			m_BaseTicks = timeGetTime();
			QueryPerformanceCounter ( &m_BaseCount );
			QueryPerformanceFrequency ( &m_BaseFreq );
		#else
			struct timeval tv;
			gettimeofday(&tv, NULL);
			m_BaseTicks = ((sjtime) tv.tv_sec * 1000000LL) + (sjtime) tv.tv_usec;		
        #endif
	}

	sjtime TimeX::GetSystemMSec ()
	{
		#ifdef _MSC_VER
			return m_BaseTime + sjtime(timeGetTime() - m_BaseTicks)*MSEC_SCALAR;
		#else
			struct timeval tv;
			gettimeofday(&tv, NULL);			
			sjtime t = ((sjtime) tv.tv_sec * 1000000LL) + (sjtime) tv.tv_usec;	
			return m_BaseTime + ( t - m_BaseTicks) * 1000LL;			// 1000LL - converts microseconds to milliseconds
		#endif
	}

	sjtime TimeX::GetSystemNSec ()
	{
		#ifdef _MSC_VER
      // nanosec accuracy (Windows)
			LARGE_INTEGER currCount;
			QueryPerformanceCounter ( &currCount );
			return m_BaseTime + sjtime( (double(currCount.QuadPart-m_BaseCount.QuadPart) / m_BaseFreq.QuadPart) * SEC_SCALAR);
    #else
      // nanosec accuracy (Linux)
		  /*timespec t;
			clock_gettime ( CLOCK_PROCESS_CPUTTIME_ID, &t ); // MP-TODO: Stackoverflow: Add -lrt to the end of g++ command line. This links in the librt.so "Real Time" shared library.    
			sjtime t_nsec = ((sjtime) t.tv_sec * 1000000LL) + (sjtime) tv.tv_nsec;
			return m_BaseTime + ( t_nsec - m_BaseTicks);*/
            struct timeval tv; // MP-TODO: Quick non-fix to get on with compiling process
            gettimeofday(&tv, NULL);
            sjtime t = ((sjtime) tv.tv_sec * 1000000LL) + (sjtime) tv.tv_usec;
            return m_BaseTime + ( t - m_BaseTicks) * 1000LL;			// 1000LL - converts microseconds to milliseconds
		#endif	

		/*-- old code for linux, not as accurate as clock_gettime
		struct timeval tv;
    gettimeofday(&tv, NULL);
    sjtime t = ((sjtime) tv.tv_sec * 1000000LL) + (sjtime) tv.tv_usec;
		*/
	}

#ifdef _MSC_VER
	
	// windows nanosleep
	BOOLEAN nanosleep(long long ns) {		
		HANDLE timer;	
		LARGE_INTEGER li;	
		// create timer
		if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
			return FALSE;
		// set timer properties
		li.QuadPart = -ns;
		if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
			CloseHandle(timer);
			return FALSE;
		}
		// start & wait for timer
		WaitForSingleObject(timer, INFINITE);		
		CloseHandle(timer);		
		return TRUE;
	}
#endif

	void TimeX::SleepNSec ( float nsec )
	{
		#ifdef _MSC_VER
			nanosleep ( (long long) nsec );
		#else
		  timespec t;
			t.tv_sec = int( nsec / 1000 ); // MP-TODO: looks like this should be nsec
			t.tv_nsec = ( nsec - ( t.tv_sec*1000 ) ) * 1000000L; // MP-TODO: same
			nanosleep ( &t, 0 ); // MP-TODO: should work
		#endif
	}
  

	void TimeX::SetTimeNSec ()
	{
		m_CurrTime = GetSystemNSec ();
	}


	TimeX::TimeX ()
	{	
		if ( !m_Started ) {
			m_Started = true;
            m_BaseTime = 0;
            SetSystemTime ();				        // Set m_CurrTime to the wall clock
			start_basetime ( m_CurrTime );	// Start base timer at wall clock
		}
		m_CurrTime = 0;
	}

	// Note regarding hours:
	//  0 <= hr <= 23
	//  hr = 0 is midnight (12 am)
	//  hr = 1 is 1 am
	//  hr = 12 is noon 
	//  hr = 13 is 1 pm (subtract 12)
	//  hr = 23 is 11 pm (subtact 12)

	// GetScaledJulianTime
	// Returns -1.0 if the time specified is invalid.
	sjtime TimeX::GetScaledJulianTime ( int hr, int min, int m, int d, int y, int s, int ms, int ns )
	{
		double MJD;				// Modified Julian Date (JD - 2400000.5)
		sjtime SJT;				// Scaled Julian Time SJT = MJD * 86400000 + UT

		// Check if date/time is valid
		if (m <=0 || m > 12) return (sjtime) -1;	
		if ( y % 4 == 0 && m == 2) {	// leap year in february
			if (d <=0 || d > m_DaysInMonth[m]+1) return (sjtime) -1;
		} else {
			if (d <=0 || d > m_DaysInMonth[m]) return (sjtime) -1;		
		}
		if (hr < 0 || hr > 23) return (sjtime) -1;
		if (min < 0 || min > 59) return  (sjtime) -1;

		// Compute Modified Julian Date
		MJD = 367 * y - int ( 7 * (y + int (( m + 9)/12)) / 4 );
		MJD -= int ( 3 * (int((y + (m - 9)/7)/100) + 1) / 4);
		MJD += int ( 275 * m / 9 ) + d + 1721028.5 - 1.0;
		MJD -= 2400000.5;
		// Compute Scaled Julian Time
		SJT = sjtime(MJD) * sjtime( DAY_SCALAR );	
		SJT += hr * HR_SCALAR + min * MIN_SCALAR + s * SEC_SCALAR + ms * MSEC_SCALAR + ns * NSEC_SCALAR;
		return SJT;
	}

	sjtime TimeX::GetScaledJulianTime ( int hr, int min, int m, int d, int y )
	{
		return GetScaledJulianTime ( hr, min, m, d, y, 0, 0, 0 );
	}

	void TimeX::GetTime ( sjtime SJT, int& hr, int& min, int& m, int& d, int& y)
	{
		int s = 0, ms = 0, ns = 0;
		GetTime ( SJT, hr, min, m, d, y, s, ms, ns );
	}

	void TimeX::GetTime ( sjtime SJT, int& hr, int& min, int& m, int& d, int& y, int& s, int &ms, int& ns)
	{	
		// Compute Universal Time from SJT
		sjtime UT = sjtime( SJT % sjtime( DAY_SCALAR ) );

		// Compute Modified Julian Date from SJT
		double MJD = double(SJT / DAY_SCALAR);

		// Use MJD to get Month, Day, Year
		double z = floor ( MJD + 1 + 2400000.5 - 1721118.5);
		double g = z - 0.25;
		double a = floor ( g / 36524.25 );
		double b = a - floor  ( a / 4.0 );
		y = int( floor (( b + g ) / 365.25 ) );
		double c = b + z - floor  ( 365.25 * y );
		m = int (( 5 * c + 456) / 153 );
		d = int( c - int (( 153 * m - 457) / 5) );
		if (m > 12) {
			y++;
			m -= 12;
		}
		// Use UT to get Hrs, Mins, Secs, Msecs
		hr = int( UT / HR_SCALAR );
		UT -= hr * HR_SCALAR;
		min = int( UT / MIN_SCALAR );
		UT -= min * MIN_SCALAR;
		s = int ( UT / SEC_SCALAR );
		UT -= s * SEC_SCALAR;
		ms = int ( UT / MSEC_SCALAR );	
		UT -= ms * MSEC_SCALAR;
		ns = int ( UT / NSEC_SCALAR );

		// UT Example:
		//      MSEC_SCALAR =         1
		//      SEC_SCALAR =      1,000
		//      MIN_SCALAR =     60,000
		//		HR_SCALAR =   3,600,000
		//      DAY_SCALAR = 86,400,000
		//
		//   7:14:03, 32 msec 	
		//   UT = 7*3,600,000 + 14*60,000 + 3*1,000 + 32 = 26,043,032
		//
		//   26,043,032 / 3,600,000 = 7			26,043,032 - (7 * 3,600,000) = 843,032
		//      843,032 /    60,000 = 14		   843,032 - (14 * 60,000) = 3,032
		//        3,032 /     1,000 = 3              3,032 - (3 * 1,000) = 32
		//           32 /         1 = 32	
	}

	void TimeX::GetTime (int& s, int& ms, int& ns )
	{
		int hr, min, m, d, y;
		GetTime ( m_CurrTime, hr, min, m, d, y, s, ms, ns );
	}


	void TimeX::GetTime (int& hr, int& min, int& m, int& d, int& y)
	{
		GetTime ( m_CurrTime, hr, min, m, d, y);
	}

	void TimeX::GetTime (int& hr, int& min, int& m, int& d, int& y, int& s, int& ms, int& ns)
	{
		GetTime ( m_CurrTime, hr, min, m, d, y, s, ms, ns);
	}

	bool TimeX::SetTime ( int sec )
	{
		int hr, min, m, d, y;
		GetTime ( m_CurrTime, hr, min, m, d, y );
		m_CurrTime = GetScaledJulianTime ( hr, min, m, d, y, sec, 0, 0 );
		return true;
	}

	bool TimeX::SetTime ( int sec, int msec )
	{
		int hr, min, m, d, y;
		GetTime ( m_CurrTime, hr, min, m, d, y );
		m_CurrTime = GetScaledJulianTime ( hr, min, m, d, y, sec, msec, 0 );
		return true;
	}

	bool TimeX::SetDateTime (int yr, int mo, int day, int hr, int min, int sec, int msec, int nsec )
	{
		m_CurrTime = GetScaledJulianTime ( hr, min, mo, day, yr, sec, msec, nsec );
		if (m_CurrTime == -1.0) return false;
		return true;
	}

	// Convert the given float storage (f) back to month/day/year
	void TimeX::GetDateF ( float f, int& m, int& d, int& y )
	{
		int hr, min, sec;

		y = int(f);	f = f - y;
		m = f*12;	f = f*12 - m;
		d = f*32;	f = f*32 - d;
		hr = f*24;	f = f*24 - hr;
		min = f*60;	f = f*60 - min;
		sec = int(f);
	}

	float TimeX::SetDateF ( int m, int d, int y )
	{
		int hr=0, min=0, sec=0;
		m_CurrTime = sec*F_SEC_MULT +  min*F_MIN_MULT + hr*F_HR_MULT + d*F_DAY_MULT + m*F_MONTH_MULT + y*F_YEAR_MULT;

		return float(m_CurrTime) / float(F_YEAR_MULT);
	}


	// Convert a date string to float storage (f)
	float TimeX::SetDateF ( std::string line, int mp, int mc, int dp, int dc, int yp, int yc )
	{   
		int hr=0, min=0, sec=0;
		int m, d, y;	
		m = atoi ( line.substr ( mp, mc).c_str () );
		d = atoi ( line.substr ( dp, dc).c_str () );
		y = atoi ( line.substr ( yp, yc).c_str () );

		m_CurrTime = sec*F_SEC_MULT +  min*F_MIN_MULT + hr*F_HR_MULT + d*F_DAY_MULT + m*F_MONTH_MULT + y*F_YEAR_MULT;

		return float(m_CurrTime) / float(F_YEAR_MULT);
	}


	std::string TimeX::GetDayOfWeekName ()
	{
		switch (GetDayOfWeek()) {
		case 1:		return "Sunday";	break;
		case 2:		return "Monday";	break;
		case 3:		return "Tuesday";	break;
		case 4:		return "Wednesday";	break;
		case 5:		return "Thursday";	break;
		case 6:		return "Friday";	break;
		case 7:		return "Saturday";	break;
		}
		return "day error";
	}

	int TimeX::GetDayOfWeek ()
	{
		// Compute Modified Julian Date
		double MJD = (double) m_CurrTime / sjtime( DAY_SCALAR );

		// Compute Julian Date
		double JD = floor ( MJD + 1 + 2400000.5 );
		int dow = (int(JD - 0.5) % 7) + 4;
		if (dow > 7) dow -= 7;

		// day of week (1 = sunday, 7 = saturday)
		return dow ;
	}

	int TimeX::GetWeekOfYear ()
	{
		int hr, min, m, d, y;
		GetTime ( hr, min, m, d, y );
		double mjd_start = (double) GetScaledJulianTime ( 0, 0, 1, 1, y ) / DAY_SCALAR; // mjt for jan 1st of year
		double mjd_curr = (double) GetScaledJulianTime ( 0, 0, m, d, y ) / DAY_SCALAR; // mjt for specified day in year
		double JD = floor ( mjd_start + 1 + 2400000.5 );
		int dow = (int ( JD - 0.5 ) % 7) + 4;  // day of week for jan 1st of year.
		if (dow > 7) dow -= 7;
	
		// week of year (first week in january = week 0)
		return int((mjd_curr - mjd_start + dow -1 ) / 7 );
	}

	float TimeX::GetElapsedSec ( TimeX& base )
	{
		return float( sjtime(m_CurrTime - base.GetSJT() ) ) / sjtime( SEC_SCALAR );
	}

	float TimeX::GetElapsedMSec ( TimeX& base )
	{
		return float( sjtime(m_CurrTime - base.GetSJT() ) ) / sjtime( MSEC_SCALAR );
	}
	
	float TimeX::GetElapsedDays ( TimeX& base )
	{
		return float( sjtime(m_CurrTime - base.GetSJT() ) ) / sjtime( DAY_SCALAR );
	}

	float TimeX::GetElapsedWeeks ( TimeX& base )
	{
		return GetElapsedDays(base) / 7.0f;
	}

	float TimeX::GetElapsedMonths ( TimeX& base)
	{
		return int ( double(GetElapsedDays(base)) / 30.416 );
	}

	float TimeX::GetElapsedYears ( TimeX& base )
	{
		// It is much easier to compute this in m/d/y format rather
		// than using julian dates.
		int bhr, bmin, bm, bd, by;
		int ehr, emin, em, ed, ey;
		GetTime ( base.GetSJT(), bhr, bmin, bm, bd, by );
		GetTime ( m_CurrTime, ehr, emin, em, ed, ey );
		if ( em < bm) {
			// earlier month
			return ey - by - 1;
		} else if ( em > bm) {
			// later month
			return ey - by;
		} else {
			// same month
			if ( ed < bd ) {
				// earlier day
				return ey - by - 1;
			} else if ( ed >= bd ) {
				// later or same day
				return ey - by;
			}
		}
		return -1;
	}
	#pragma warning(disable:4244)

	long TimeX::GetFracDay ( TimeX& base )
	{
		// Resolution = 5-mins
		return long( sjtime(m_CurrTime - base.GetSJT() ) % sjtime(DAY_SCALAR) ) / (MIN_SCALAR*5);
	}

	long TimeX::GetFracWeek ( TimeX& base )
	{
		// Resolution = 1 hr
		long day = int(GetElapsedDays(base)) % 7;		// day in week
		long hrs = long( sjtime(m_CurrTime - base.GetSJT() ) % sjtime(DAY_SCALAR) ) / (HR_SCALAR);
		return day * 24 + hrs;
	}

	long TimeX::GetFracMonth ( TimeX& base )
	{
		// Resolution = 4 hrs
		long day = (long) fmod ( double(GetElapsedDays(base)), 30.416 );	// day in month
		long hrs = long( sjtime(m_CurrTime - base.GetSJT() ) % sjtime(DAY_SCALAR) ) / (HR_SCALAR*4);
		return day * (24 / 4) + hrs;
	}

	long TimeX::GetFracYear ( TimeX& base )
	{
		// It is much easier to compute this in m/d/y format rather
		// than using julian dates.
		int bhr, bmin, bm, bd, by;
		int ehr, emin, em, ed, ey;
		sjtime LastFullYear;
		GetTime ( base.GetSJT() , bhr, bmin, bm, bd, by );
		GetTime ( m_CurrTime, ehr, emin, em, ed, ey );
		if ( em < bm) {
			// earlier month
			LastFullYear = GetScaledJulianTime ( ehr, emin, bm, bd, ey - 1);		
			return long( sjtime(m_CurrTime - LastFullYear) / sjtime(DAY_SCALAR) );		
		} else if ( em > bm) {
			// later month
			LastFullYear = GetScaledJulianTime ( ehr, emin, bm, bd, ey);
			return long( sjtime(m_CurrTime - LastFullYear) / sjtime(DAY_SCALAR) );				
		} else {
			// same month
			if ( ed < bd ) {
				// earlier day
				LastFullYear = GetScaledJulianTime ( ehr, emin, bm, bd, ey - 1);		
				return long( sjtime(m_CurrTime - LastFullYear) / sjtime(DAY_SCALAR) );		
			} else if ( ed > bd ) {
				// later day
				LastFullYear = GetScaledJulianTime ( ehr, emin, bm, bd, ey);
				return long( sjtime(m_CurrTime - LastFullYear) / sjtime(DAY_SCALAR) );
			} else {
				return 0;	// same day
			}
		}	
	}


	bool TimeX::SetTime ( std::string line )
	{
		int hr, min, m, d, y;
		std::string dat;
		if ( line.substr ( 0, 1 ) == " " ) 
			dat = line.substr ( 1, line.length()-1 ).c_str();
		else 
			dat = line;

		hr = atoi ( dat.substr ( 0, 2).c_str() );
		min = atoi ( dat.substr ( 3, 2).c_str() );
		m = atoi ( dat.substr ( 6, 2).c_str () );
		d = atoi ( dat.substr ( 9, 2).c_str () );
		y = atoi ( dat.substr ( 12, 4).c_str () );
		return SetDateTime ( y, m, d, hr, min, 0 );
	}
	
	// trim - avoiding depend on string_helper
	std::string trim (std::string str)
	{
		size_t lft = str.find_first_not_of ( " \t\r\n" );
		size_t rgt = str.find_last_not_of ( " \t\r\n" );
		if ( lft == std::string::npos || rgt == std::string::npos ) return "";
		return str.substr ( lft, rgt-lft+1 );
	}

	// Read formatted: YYYY-MM-DD HH:MM:SS
	bool TimeX::ReadDateTime ( std::string line )
	{
		int hr, min, sec, mo, day, yr;
		line = trim ( line );
		line = trim ( line );
		yr = atoi ( line.substr ( 0, 4).c_str () );
		mo = atoi ( line.substr ( 5, 2).c_str () );
		day = atoi ( line.substr ( 8, 2).c_str () );
		hr = atoi ( line.substr ( 11, 2).c_str() );
		min = atoi ( line.substr ( 14, 2).c_str() );
		sec = atoi ( line.substr ( 17, 2).c_str() );		
		return SetDateTime ( yr, mo, day, hr, min, sec);
	}

	// Write formatted: YYYY-MM-DD HH:MM:SS
	std::string TimeX::WriteDateTime ()
	{
		char buf[200];
		std::string line;
		int hr, min, m, d, y, s, ms, ns;

		GetTime ( hr, min, m, d, y, s, ms, ns );	
		sprintf ( buf, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, hr, min, s );
		return std::string ( buf );
	}

	std::string TimeX::GetReadableTime ()
	{
		char buf[200];
		std::string line;
		int hr, min, m, d, y, s, ms, ns;

		GetTime ( hr, min, m, d, y, s, ms, ns );	
		sprintf ( buf, "%02d:%02d,%03d.%06d", min, s, ms, ns);
		//sprintf ( buf, "%02d:%02d:%02d %03d.%06d %02d-%02d-%04d", hr, min, s, ms, ns, m, d, y);
		return std::string ( buf );
	}

	std::string TimeX::GetReadableSJT ()
	{
		char buf[200];	
		sprintf ( buf, "%I64d", m_CurrTime );
		return std::string ( buf );
	}

	std::string TimeX::GetReadableTime ( int fmt )
	{
		char buf[200];	
		int hr, min, m, d, y, s, ms, ns;
		GetTime ( hr, min, m, d, y, s, ms, ns );	

		switch (fmt) {
		case 0: sprintf ( buf, "%02d %03d.%06d", s, ms, ns);
		}
		return std::string ( buf );
	}

	void TimeX::SetSystemTime ()
	{
		int hr, mn, sec, m, d, y;
		char timebuf[100];
		char datebuf[100];
		std::string line;

		#ifdef _MSC_VER
			#ifdef VS2005
			_strtime_s ( timebuf, 100 );
			_strdate_s ( datebuf, 100 );
			#else
			_strtime ( timebuf );
			_strdate ( datebuf );
			#endif
		#endif
		#if (defined(__linux__) || defined(__CYGWIN__))
			time_t tt; 
			struct tm tim;
			tt = time(NULL);	
			localtime_r(&tt, &tim);	
			sprintf( timebuf, "%02i:%02i:%02i", tim.tm_hour, tim.tm_min, tim.tm_sec);
			sprintf( datebuf, "%02i:%02i:%02i", tim.tm_mon, tim.tm_mday, tim.tm_year % 100);
		#endif

		line = timebuf;
		hr = atoi ( line.substr ( 0, 2).c_str() );
		mn = atoi ( line.substr ( 3, 2).c_str() );
		sec = atoi ( line.substr ( 6, 2).c_str() );
		line = datebuf;
		m = atoi ( line.substr ( 0, 2).c_str() );
		d = atoi ( line.substr ( 3, 2).c_str() );
		y = atoi ( line.substr ( 6, 2).c_str() );
	
		// NOTE: This only works from 1930 to 2030
		if ( y > 30) y += 1900;
		else y += 2000;
	
		SetDateTime ( y, m, d, hr, mn, sec );
	}
   

	double TimeX::GetSec ()
	{
		return (double) (m_CurrTime / SEC_SCALAR);
	}

	double TimeX::GetMSec ()
	{
		return (double) (m_CurrTime / MSEC_SCALAR);

		//int s, ms, ns;
		//GetTime ( s, ms, ns );
		//return ms;
	}

	void TimeX::Advance ( TimeX& t )
	{
		m_CurrTime += t.GetSJT ();
	}

	void TimeX::AdvanceMinutes ( float n)
	{
		m_CurrTime += (sjtime) MIN_SCALAR * n;
	}

	void TimeX::AdvanceHours ( float n )
	{
		m_CurrTime += (sjtime) HR_SCALAR * n;	
	}

	void TimeX::AdvanceDays ( float n )
	{
		m_CurrTime += (sjtime) DAY_SCALAR * n;	
	}

	void TimeX::AdvanceSec ( float n )
	{
		m_CurrTime += (sjtime) SEC_SCALAR * n;	
	}

	void TimeX::AdvanceMins ( float n)
	{
		m_CurrTime += (sjtime) MIN_SCALAR * n;
	}	

	void TimeX::AdvanceMSec ( float n )
	{
		m_CurrTime += (sjtime) MSEC_SCALAR * n;	
	}

	TimeX& TimeX::operator= ( const TimeX& op )	{ m_CurrTime = op.m_CurrTime; return *this; }
	TimeX& TimeX::operator= ( TimeX& op )			{ m_CurrTime = op.m_CurrTime; return *this; }
	bool TimeX::operator< ( const TimeX& op )		{ return (m_CurrTime < op.m_CurrTime); }
	bool TimeX::operator> ( const TimeX& op )		{ return (m_CurrTime > op.m_CurrTime); }
	bool TimeX::operator< ( TimeX& op )			{ return (m_CurrTime < op.m_CurrTime); }
	bool TimeX::operator> ( TimeX& op )			{ return (m_CurrTime > op.m_CurrTime); }

	bool TimeX::operator<= ( const TimeX& op )		{ return (m_CurrTime <= op.m_CurrTime); }
	bool TimeX::operator>= ( const TimeX& op )		{ return (m_CurrTime >= op.m_CurrTime); }
	bool TimeX::operator<= ( TimeX& op )			{ return (m_CurrTime <= op.m_CurrTime); }
	bool TimeX::operator>= ( TimeX& op )			{ return (m_CurrTime >= op.m_CurrTime); }

	TimeX TimeX::operator- ( TimeX& op )
	{
		return TimeX( m_CurrTime - op.GetSJT() );
	}
	TimeX TimeX::operator+ ( TimeX& op )
	{
		return TimeX( m_CurrTime + op.GetSJT() );
	}

	bool TimeX::operator== ( const TimeX& op )
	{
		return (m_CurrTime == op.m_CurrTime);
	}
	bool TimeX::operator!= ( TimeX& op )
	{
		return (m_CurrTime != op.m_CurrTime);
	}
	
	void TimeX::RegressionTest ()
	{
		// This code verifies the Julian Date calculations are correct for all
		// minutes over a range of years. Useful to debug type issues when
		// compiling on different platforms.
		//
		int m, d, y, hr, min;
		int cm, cd, cy, chr, cmin;

		for (y=2000; y < 2080; y++) {
			for (m=1; m <= 12; m++) {
				for (d=1; d <= 31; d++) {
					for (hr=0; hr<=23; hr++) {
						for (min=0; min<=59; min++) {
							if ( SetDateTime ( y, m, d, hr, min ) ) {
								GetTime ( chr, cmin, cm, cd, cy );
								if ( hr!=chr || min!=cmin || m!=cm || d!=cd || y!=cy) {
	//								printf ( 'luna', INFO, "Error: %d, %d, %d, %d, %d = %I64d\n", hr, min, m, d, y, GetSJT());
	//								printf ( 'luna', INFO, "-----: %d, %d, %d, %d, %d\n", chr, cmin, cm, cd, cy);
								}
							}
						}
					}				
				}			
			}
	//		printf ( 'luna', INFO, "Verified: %d\n", y);
		}
	}

