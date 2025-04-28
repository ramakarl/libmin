//--------------------------------------------------------------------------------
// NVIDIA(R) GVDB VOXELS
// Copyright 2017, NVIDIA Corporation.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
//    in the documentation and/or  other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Version 1.0: Rama Hoetzlein, 5/1/2017
//----------------------------------------------------------------------------------

#include "vec.h"
#include "string_helper.h"
#ifdef USE_TIMEX
  #include "timex.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <assert.h>

#ifdef _WIN32
  #include <windows.h>    // needed for WideCharToMultiByte
  #include <assert.h>
#else
    #include <string.h>        // for strlcpy

    #ifndef strlcpy
         #define strlcpy  strncpy
    #endif
    #ifndef fopen_s
    int fopen_s(FILE **f, const char *name, const char *mode) {
      int ret = 0;
      assert(f);
      *f = fopen(name, mode);
      if (!*f)
        ret = errno;
      return ret;
    }
  #endif

#endif

int strToI (std::string s) {
  //return ::atoi ( s.c_str() );
  std::istringstream str_stream ( s );
  int x;
  if (str_stream >> x) return x;    // this is the correct way to convert std::string to int, do not use atoi
  return 0;
};

int strToI (std::string s, int &x) {
  //return ::atoi ( s.c_str() );
  std::istringstream str_stream ( s );

   // this is the correct way to convert std::string to int, do not use atoi
   //
  if (str_stream >> x) { return 0; }
  return -1;
};

xlong strToI64 (std::string s) 
{
	#if defined(_WIN32)
		return _strtoui64( s.c_str(), NULL, 0 );
    #elif defined(__ANDROID__)
        return std::strtoull( s.c_str(), NULL, 0 );
    #else
		return strtoull( s.c_str(), NULL, 0 );
	#endif
}

bool isFloat (std::string s) {

  int st = 0;
  char ch = s.at(st);
  if (ch == 43 || ch==45 ) st++;          // check +, -

  for (int n = st; n < s.length(); n++) {
    ch = s.at(n);
    if ( ch < 48 || ch > 57 || ch != 46 )    // fast check for non-numerical
      return false;
  }
  return true;
}

float strToF (std::string s) {
  //return ::atof ( s.c_str() );
  if (s.at(0)=='n') return std::numeric_limits<float>::quiet_NaN();
  std::istringstream str_stream ( s );
  float x;
  if (str_stream >> x) return x;    // this is the correct way to convert std::string to float, do not use atof
  return std::numeric_limits<float>::quiet_NaN();
};

int strToF (std::string s, float &val) {
  std::istringstream str_stream ( s );

  // this is the correct way to convert std::string to float, do not use atof
  //
  if (str_stream >> val) { return 0; }
  return -1;
};

double strToD (std::string s) {
  //return ::atof ( s.c_str() );
  std::istringstream str_stream ( s );
  double x;
  if (str_stream >> x) return x;    // this is the correct way to convert std::string to float, do not use atof
  return std::numeric_limits<double>::quiet_NaN();
};
unsigned long strToUL ( std::string s )
{
  std::istringstream str_stream ( s );
  unsigned long x;
  if ( str_stream >> x) return x;
  return 0;
}
unsigned char strToC ( std::string s ) {
  char c;
  memcpy ( &c, s.c_str(), 1 );    // cannot use atoi here. atoi only returns numbers for strings containing ascii numbers.
  return c;
};
// Return 4-byte long int whose bytes
// match the first 4 ASCII chars of string given.
unsigned long strToID ( std::string str )
{
  str = str + "    ";
  return (static_cast<unsigned long>(str.at(0)) << 24) |
       (static_cast<unsigned long>(str.at(1)) << 16) |
       (static_cast<unsigned long>(str.at(2)) << 8) |
       (static_cast<unsigned long>(str.at(3)) );
}

/*Vec4F strToV4 ( std::string val )
{
  Vec4F v(0,0,0,0);
  size_t pos1 = val.find (',');
  size_t pos2 = val.find (',', pos1+1);
  size_t pos3 = val.find (',', pos2+1);

  if ( pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos ) {
    v.x = atof ( val.substr ( 0, pos1 ).c_str() );
    v.y = atof ( val.substr ( pos1+1, pos2-pos1 ).c_str() );
    v.z = atof ( val.substr ( pos2+1, pos3-pos2 ).c_str() );
    v.w = atof ( val.substr( pos3+1 ).c_str() );
  }
  return v;
}*/

objType strToType (  std::string str )
{
  char buf[5];
  strcpy ( buf, str.c_str() );
  objType name;
  ((char*) &name)[3] = buf[0];
  ((char*) &name)[2] = buf[1];
  ((char*) &name)[1] = buf[2];
  ((char*) &name)[0] = buf[3];
  return name;
}

std::string typeToStr ( objType name )      // static function
{
  char buf[5];
  buf[0] = ((char*) &name)[3];
  buf[1] = ((char*) &name)[2];
  buf[2] = ((char*) &name)[1];
  buf[3] = ((char*) &name)[0];
  buf[4] = '\0';
  return std::string ( buf );
}


std::string cToStr ( char c )
{
  char buf[2];
  buf[0] = c;
  buf[1] = '\0';
  return std::string ( buf );
}

std::string iToStr ( int i )
{
  std::ostringstream ss;
  ss << i;
  return ss.str();
}
//-- note: may be incorrect. check on floats /w many digits
std::string fToStr ( float f )
{
  std::ostringstream ss;
  ss << f;
  return ss.str();
}
std::string xlToStr ( uint64_t f )
{
  std::ostringstream ss;
  ss << f;
  return ss.str();
}

std::string strFilebase ( std::string str )
{
  size_t pos = str.find_last_of ( '.' );
  if ( pos != std::string::npos )
    return str.substr ( 0, pos );
  return str;
}
std::string strFilepath ( std::string str )
{
  size_t pos = str.find_last_of ( '\\' );
  if ( pos != std::string::npos )
    return str.substr ( 0, pos+1 );
  return str;
}

// Parse string inside two separators.
//   e.g. "object<car>,other".. str='other', key='object', val='car'
// Parse string inside two separators.
//   e.g. "object<car>,other".. str='other', key='object', val='car'
bool strParseKeyVal ( std::string& str, uchar lsep, uchar rsep, std::string& key, std::string& val )
{
  std::string result;
  size_t lfound, rfound;

  if (lsep=='*' || rsep=='*' ) {
    key = "";
    val = str;
    return true;
  }
  lfound = str.find_first_of ( lsep );
  if ( lfound != std::string::npos) {
    rfound = str.find_first_of ( rsep, lfound+1 );
    if ( rfound != std::string::npos ) {
      val = str.substr ( lfound+1, rfound-lfound-1 );          // return string strickly between lsep and rsep
      key = str.substr ( 0, lfound );
      str = str.substr ( rfound+1 );
      return true;
    }
  }
  return false;
}



// Get string from inside two separators. Input is unchanged.
//  e.g. object<car> --> result: car
bool strGet ( std::string str, std::string& result, std::string lsep, std::string rsep )
{
  size_t lfound, rfound;

  lfound = str.find ( lsep );
  if ( lfound != std::string::npos) {
    rfound = str.find ( rsep, lfound+1 );
    if ( rfound != std::string::npos ) {
      result = str.substr ( lfound+1, rfound-lfound-1 );          // return string strictly between lsep and rsep
      return true;
    }
  }
  return false;
}

// strGet - return a substring between two separator *strings*, without modifying input string.
// //       does NOT treat lsep/rsep as a list of possible single-char separators.
// input:   strGet ( "THIS(world) THAT(one)", "THIS(", ")", result, pos )
// output:  result = "world", pos=6
bool strGet ( const std::string& str, std::string lsep, std::string rsep, std::string& result, size_t& pos )
{
  size_t lfound, rfound;
  lfound = str.find ( lsep );
  if ( lfound != std::string::npos ) {
    lfound += lsep.length();
    rfound = str.find ( rsep, lfound );
    if ( rfound != std::string::npos ) {
      result = str.substr ( lfound, rfound - lfound );
      pos = lfound;
      return true;
    }
  }
  return false;
}



// SplitLeft
//
// WARNING, this skips over separators at the front of the string
//
std::string strSplitLeft(std::string& str, std::string sep)
{
  std::string key, val;
  strSplitLeft ( str, sep, key, val );
  str = val;
  return key;
}
// SplitLeft - eg. "left,right" sep=, --> key="left", val="right", str=unchanged
bool strSplitLeft ( std::string str, std::string sep, std::string& key, std::string& val )
{
  size_t f1;
  f1 = str.find_first_of ( sep );
  if ( f1 == std::string::npos) {
    key = str; val = "";
    return false;
  }
  key = str.substr ( 0, f1 );
  val = str.substr( f1+1 );
  return true;
}
// Split string on separator. Return right.
std::string strSplitRight ( std::string& str, std::string sep )
{
  std::string result = "";
  size_t f1;
  f1 = str.find_first_of ( sep );
  if ( f1 != std::string::npos) {
    result = str.substr ( f1+1 );
    str = str.substr( 0, f1 );    // left side is continued in str
  }
  return result;
}

// Split string on separator
//   e.g. "object:car, other".. left='object:car', right='other'
bool strSplit ( std::string str, std::string sep, std::string& left, std::string& right )
{
  std::string result;
  size_t f1, f2;

  f1 = str.find_first_not_of ( sep );
  if ( f1 == std::string::npos ) f1 = 0;
  f2 = str.find_first_of ( sep, f1 );
  if ( f2 != std::string::npos) {
    left = str.substr ( f1, f2-f1 );
    right = str.substr ( f2+1 );
    return true;
  }
  left = "";
  right = str;
  return false;
}

// Split a string into multiple words delimited by sep
 int strSplitMultiple ( std::string str, std::string sep, std::vector<std::string>& list )
{
  list.clear ();
  size_t f1;
  f1 = str.find_first_of ( sep );

  while ( f1 != std::string::npos  ) {
    list.push_back ( strTrim(str.substr(0,f1)) );
    str = str.substr ( f1+1 );
    f1 = str.find_first_of ( sep );
  }
  list.push_back ( strTrim(str) );
  return (int) list.size();
}

bool strFileSplit ( std::string str, std::string& path, std::string& name, std::string& ext )
{
  size_t slash = str.find_last_of ( "/\\" );
  if ( slash != std::string::npos ) {
    path = str.substr ( 0, slash );
    str = str.substr ( slash+1 );
  } else {
    path = "";
  }
  size_t dot = str.find_last_of ( '.' );
  if ( dot != std::string::npos ) {
    name = str.substr ( 0, dot );
    ext = str.substr ( dot+1 );
  } else {
    name = str;
    ext = "";
  }
  return true;
}


// get cmd line argument
bool strArg(int argc, char** argv, const char* chk_arg, std::string& val)
{
  std::string value = "";
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], chk_arg) == 0) {
      if (i + 1 < argc) val = argv[i + 1];
      return true;
    }
  }
  return false;
}

// Parse out
// - parse a value out of a string, keeping remainder (left & right) intact
// - this func CONSUMES (removes) both the left and right separators
// e.g. "[MATERIAL] Basic" --> value "MATERIAL", remain="Basic"

std::string strParseOutDelim ( std::string& str, std::string lsep, std::string rsep )
{
  std::string value, remain;
  strParseOutDelim ( str, lsep, rsep, value, remain);
  str = remain;
  return value;
}

bool strParseOutDelim ( std::string str, std::string lseps, std::string rseps, std::string& value, std::string& remain)
{
  size_t f1, fL, fR;
  value = "";
  remain = str;

  f1 = str.find_first_of ( lseps );              // find separators
  if ( f1 == std::string::npos) return false;
  fR = str.find_first_of ( rseps, f1+1 );
  if ( fR == std::string::npos ) return false;

  fL = f1 + 1;                                    // separators are single chars
  value = str.substr ( fL, fR-fL );
  remain = str.substr ( 0, f1 ) + str.substr ( fR+1 );  // parse away the separators
  return true;
}

bool strParseOutStr (std::string str, std::string lstr, std::string rstr, std::string& value, std::string& remain)
{
  size_t f1, fL, fR;
  value = "";
  remain = str;

  f1 = str.find (lstr);              // find separators
  if (f1 == std::string::npos) return false;
  fR = str.find (rstr, f1 + lstr.length() );
  if (fR == std::string::npos) return false;

  fL = f1 + lstr.length();
  value = str.substr(fL, fR - fL);
  remain = str.substr(0, f1) + str.substr(fR + 1);  // parse away the separators
  return true;
}


// Parse chars
// - parse any number of alpha-numeric chars starting at lsep. 
// - this func CONSUMES the left-separator. there is no rsep.
// e.g. "EVAL(date=VEC4) | more" --> value "VEC4", remain="EVAL(date) | more"
//
bool strParseChars(std::string str, std::string lsep, std::string& value, std::string& remain)
{
  size_t f1, fL, fR;
  value = "";
  remain = str;

  f1 = str.find_first_of(lsep);              // find separators
  if (f1 == std::string::npos) return false;
  fL = f1 + lsep.length();
  fR = str.find_first_not_of ( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789", f1+1 );
  if (fR == std::string::npos) {
    // nothing to the right
    value = str.substr(fL);             
    remain = str.substr(0, f1);    
  } else {
    // keep stuff to the left & right
    value = str.substr(fL, fR - fL);
    remain = str.substr(0, f1) + str.substr(fR);    // parse away left sep
  }  
  return true;
}

// Parse a list as a set of key-value arguments. Input becomes output right side. Return is arg value. Tag is arg tag.
//   e.g. object:cat, name:felix -> str=name:felix, return=cat, tag=object
std::string strParseArg ( std::string& tag, std::string argsep, std::string sep, std::string& str )
{
  std::string result;
  size_t f1, f2, f3;
  f1 = str.find_first_not_of ( " \t" );
  if ( f1 == std::string::npos ) f1=0;
  f2 = str.find_first_of ( argsep, f1 );
  if ( f2 == std::string::npos ) f2=f1;
  f3 = str.find_first_of ( sep, f2 );
  if ( f3 != std::string::npos ) {
    tag = strTrim ( str.substr ( f1, f2-f1 ) );
    result = strTrim ( str.substr ( f2+1, f3-f2-1 ) );
    str = str.substr ( f3+1 );
  } else {
    tag = "";
    str = "";
    result = "";
  }
  return result;
}


std::string strParseFirst ( std::string& str, std::string sep, std::string others, char& ch )
{
  std::string result;
  size_t lfound, ofound;

  lfound = str.find_first_of ( sep );
  ofound = str.find_first_of ( others );
  lfound = ( lfound==std::string::npos || ( ofound!=std::string::npos && ofound<lfound) ) ? ofound : lfound;

  if ( lfound != std::string::npos ) {
    ch = str.at( lfound );
    result = str.substr ( 0, lfound );
    str = str.substr ( lfound+1 );
    return result;
  } else {
    ch = '\n';
    result = str;
    str = "";
    return result;
  }
}

std::string strReplace ( std::string str, std::string target, std::string replace )
{
  size_t found = str.find ( target );
  while ( found != std::string::npos ) {
    str = str.substr ( 0, found ) + replace + str.substr ( found + target.length() );
    found = str.find ( target );
  }
  return str;
}

bool strReplace ( std::string& str, std::string src, std::string dest, int& cnt )
{
  cnt = 0;
  size_t pos = 0;

  pos = str.find ( src, pos );  
  while ( pos != std::string::npos ) {
    str.replace( pos, src.length(), dest );    
    pos += dest.length() - src.length() + 1;    
    pos = str.find ( src, pos );
    cnt++;
  }
  return true;
}

bool strSub ( std::string str, int first, int cnt, std::string cmp )
{
  if ( str.substr ( first, cnt ).compare ( cmp ) == 0 ) return true;
  return false;
}

int strCount ( std::string& str, char ch )
{
  size_t pos = 0;
  int cnt = 0;
  while ( (pos=str.find_first_of ( ch, pos )) != std::string::npos ) {
    cnt++; pos++;
  }
  return cnt;
}

bool strEmpty ( const std::string& s)
{
  if ( s.empty() ) return true;
  if ( s.length()==0 ) return true;
  size_t pos = s.find_first_not_of ( " \n\b\t" );
  if (  pos == std::string::npos ) return true;
  return false;
}



int strFindFromList ( std::string str, std::vector<std::string>& list, int& pos )
{
  size_t posL;
  for (int n=0; n < list.size(); n++ ) {
    posL = str.find ( list[n] );
    if ( posL != std::string::npos ) {
      pos = (int) posL;
      return n;
    }
  }
  return -1;    // not found
}

// THIS IS BUGGY CODE
//
bool strIsNum ( std::string str, float& f )
{
  if (str.empty()) return false;
  std::string::iterator it;
  char ch;
  for (it = str.begin(); it != str.end(); it++ ) {
    ch = (*it);
    if ( ch!='.' && ch!='-' && ch!='0' && ch!='1' && ch!='2' && ch!='3' && ch!='4' && ch!='5' && ch!='6' && ch!='7' && ch!='8' &&  ch!='9'  )
       break;
  }
  if ( it==str.end() ) {
    f = atof ( str.c_str() );
    return true;
  }
  return false;
}

float strToNum ( std::string str )
{
  return (float) atof ( str.c_str() );
}

bool strToVec ( std::string& str, uchar lsep, uchar insep, uchar rsep, float* vec, int cpt )
{
  size_t l, r, p;
  std::string key, vstr;

  if ( !strParseKeyVal( str, lsep, rsep, key, vstr ) )
    return false;

  vstr += insep;
  p = 0;

  for (int i=0; i < cpt; i++ ) {
    l = vstr.find_first_not_of ( insep, p );  if ( l == std::string::npos ) return false;
    r = vstr.find_first_of ( insep, l );    if ( r == std::string::npos ) return false;
    vec[i] = atof ( vstr.substr ( l, r-l ).c_str() );
    p = r;
  }
  return true;
}

bool strToVec3 ( std::string& str, uchar lsep, uchar insep, uchar rsep, float* vec )
{
  return strToVec ( str, lsep, insep, rsep, vec, 3 );
}
bool strToVec4 ( std::string& str, uchar lsep, uchar insep, uchar rsep, float* vec )
{
  return strToVec ( str, lsep, insep, rsep, vec, 4 );
}
Vec3F strToVec3(std::string str, uchar sep)
{
  Vec3F v;
  float* vec = &v.x;
  strToVec ( str, '<', sep, '>', vec, 3);
  return v;
}
Vec4F strToVec4(std::string str, uchar sep)
{
  Vec4F v;
  float* vec = &v.x;
  strToVec(str, '<', sep, '>', vec, 4);
  return v;
}
std::string vecToStr ( Vec4F v )
{
  char buf[1024];  
  sprintf ( buf, "<%f,%f,%f,%f>", v.x, v.y, v.z, v.w );
  return std::string(buf);
}


std::string wsToStr ( const std::wstring& str )
{
#ifdef _MSC_VER
  int len = WideCharToMultiByte ( CP_ACP, 0, str.c_str(), (int) str.length(), 0, 0, 0, 0);
  char* buf = new char[ len+1 ];
  memset ( buf, '\0', len+1 );
  WideCharToMultiByte ( CP_ACP, 0, str.c_str(), (int) str.length(), buf, len+1, 0, 0);
#else
    int len = wcstombs( NULL, str.c_str(), 0 );
  char* buf = new char[ len ];
  wcstombs( buf, str.c_str(), len );
#endif
  std::string r(buf);
  delete[] buf;
  return r;
}

std::wstring strToWs (const std::string& s)
{
  wchar_t* buf = new wchar_t[ s.length()+1 ];

#ifdef _MSC_VER
  MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, buf, (int) s.length()+1 );
#else
    mbstowcs( buf, s.c_str(), s.length() + 1 );
#endif
  std::wstring r(buf);
  delete[] buf;
  return r;
}


// trim from start
#include <algorithm>
#include <cctype>

// trim left
std::string strLTrim(std::string str)
{
  size_t lft = str.find_first_not_of(" \t\r\n");
  if ( lft==std::string::npos ) return "";
  return str.substr(lft);
}

// trim right
std::string strRTrim(std::string str)
{
  size_t rgt = str.find_last_not_of(" \t\r\n");
  if ( rgt == std::string::npos) return "";
    return str.substr( 0, rgt);
}

// trim from both ends
std::string strTrim(std::string str)
{
  size_t lft = str.find_first_not_of ( " \t\r\n" );
  size_t rgt = str.find_last_not_of ( " \t\r\n" );
  if ( lft == std::string::npos || rgt == std::string::npos ) return "";
  return str.substr ( lft, rgt-lft+1 );
}

std::string strTrim ( std::string str, std::string charlist )
{
  size_t lft = str.find_first_not_of ( charlist );
  size_t rgt = str.find_last_not_of ( charlist );
  if ( lft == std::string::npos || rgt == std::string::npos) return "";
  return str.substr(lft, rgt - lft + 1);
}


std::string strLeft ( std::string str, int n )
{
  return str.substr ( 0, n );
}
std::string strRight ( std::string str, int n )
{
  if ( str.length() < n ) return "";
  return str.substr ( str.length()-n, n );
}

std::string strLeftOf ( std::string str, uchar sep )
{
  size_t f0;
  f0 = str.find ( sep );
  if ( f0 == std::string::npos) return str;
  return str.substr(0, f0);  
}
std::string strMidOf ( std::string str, uchar sep )
{
  size_t f0 = str.find (sep);
  size_t f1 = str.rfind (sep);
  if ( f0 == std::string::npos || f1==std::string::npos) return "";
  return str.substr(f0+1, f1-f0-1);
}
std::string strRightOf ( std::string str, uchar sep )
{
  size_t f1;
  f1 = str.rfind ( sep );
  if ( f1 == std::string::npos) return str;
  return str.substr(f1+1);  
}

int strExtract ( std::string& str, std::vector<std::string>& list )
{
  size_t found ;
  for (int n=0; n < list.size(); n++) {
    found = str.find ( list[n] );
    if ( found != std::string::npos ) {
      str = str.substr ( 0, found ) + str.substr ( found + list[n].length() );
      return n;
    }
  }
  return -1;
}


std::string readword ( char* line, char delim )
{
  char word_buf[8192];

  if ( readword ( line, delim, word_buf, 8192 ) ) return word_buf;

  return "";
}


bool readword ( char *line, char delim, char *word, int max_size )
{
  char *buf_pos;
  char *start_pos;

  // read past spaces/tabs, or until end of line/string
  for (buf_pos=line; (*buf_pos==' ' || *buf_pos=='\t') && *buf_pos!='\n' && *buf_pos!='\0';)
    buf_pos++;

  // if end of line/string found, then no words found, return null
  if (*buf_pos=='\n' || *buf_pos=='\0') {*word = '\0'; return false;}

  // mark beginning of word, read until end of word
  for (start_pos = buf_pos; *buf_pos != delim && *buf_pos!='\t' && *buf_pos!='\n' && *buf_pos!='\0';)
    buf_pos++;

  if (*buf_pos=='\n' || *buf_pos=='\0') {  // buf_pos now points to the end of buffer
        strncpy_sc (word, max_size, start_pos, max_size);  // copy word to output string
    if ( *buf_pos=='\n') *(word + strlen(word)-1) = '\0';
    *line = '\0';            // clear input buffer
  } else {
                      // buf_pos now points to the delimiter after word
    *buf_pos++ = '\0';          // replace delimiter with end-of-word marker
        strncpy_sc ( word, max_size, start_pos, (long long) (buf_pos-line) );  // copy word(s) string to output string
                      // move start_pos to beginning of entire buffer
    strcpy ( start_pos, buf_pos );    // copy remainder of buffer to beginning of buffer
  }
  return true;            // return word(s) copied
}




