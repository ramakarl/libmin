
#include "value_t.h"
#include "string_helper.h"

static const Value_t nullval = Value_t();

// efficient conversion between any two types

ConvFn gConvTable[16][16] =
{
//   dst:   NULL    REF       CHAR      INT       INTC     FLOAT     DOUBLE   VEC3     VEC4      STR       BUF     TIME     FILE     VAL     TYPE     PAIR
//src
/*NULL */ { convNo, convNo,   convNo,   convNo,   convNo,  convNToF, convNo,  convNo,  convNToV4,convNToS, convNo,     convNo,  convNo,  convNo, convNo,  convNo },
/*REF  */ { convNo, convCopy, convNo,   convRToI, convNo,  convRToF, convNo,  convNo,  convRToV4,convRToS, convRToBuf, convNo,  convNo,  convNo, convNo,  convNo },
/*CHAR */ { convNo, convNo,   convCopy, convCToI, convCopy,convCToF, convNo,  convNo,  convCtoV4,convCToS, convCToBuf, convNo,  convNo,  convNo, convNo,  convNo },
/*INT  */ { convNo, convIToR, convIToC, convCopy, convNo,  convIToF, convNo,  convNo,  convItoV4,convIToS, convIToBuf, convNo,  convNo,  convNo, convNo,  convNo },
/*INTC */ { convNo, convNo,   convNo,   convCToI, convCopy,convCToF, convNo,  convNo,  convItoV4,convIToS, convCToBuf, convNo,  convNo,  convNo, convNo,  convNo },
/*FLOAT*/ { convNo, convFToR, convFToC, convFToI, convNo,  convCopy, convNo,  convNo,  convFtoV4,convFToS, convFToBuf, convNo,  convNo,  convNo, convNo,  convNo },
/*DBLE */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convCopy,convNo,  convNo,   convNo,   convNo,     convNo,  convNo,  convNo, convNo,  convNo },
/*VEC3 */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convCopy,convNo,   convNo,   convNo,     convNo,  convNo,  convNo, convNo,  convNo },
/*VEC4 */ { convNo, convV4ToR,convV4ToC,convV4ToI,convNo,  convV4ToF,convNo,  convNo,  convCopy, convV4ToS,convV4ToBuf,convNo,  convNo,  convNo, convNo,  convNo },
/*STR  */ { convNo, convSToR, convSToC, convSToI, convNo,  convSToF, convNo,  convNo,  convNo,   convCopy, convSToBuf, convNo,  convNo,  convNo, convNo,  convNo },
/*BUF  */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convNo,  convNo,   convBfToS,convBfToBf, convNo,  convNo,  convNo, convNo,  convNo },
/*TIME */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convNo,  convNo,   convNo,   convNo,     convCopy,convNo,  convNo, convNo,  convNo },
/*FILE */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convNo,  convNo,   convNo,   convNo,     convNo,  convCopy,convNo, convNo,  convNo },
/*VAL  */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convNo,  convNo,   convNo,   convNo,     convNo,  convNo,  convCopy,convNo, convNo },
/*TYPE */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convNo,  convNo,   convNo,   convNo,     convNo,  convNo,  convNo, convCopy,convNo },
/*PAIR */ { convNo, convNo,   convNo,   convNo,   convNo,  convNo,   convNo,  convNo,  convNo,   convNo,   convNo,     convNo,  convNo,  convNo, convNo,  convCopy }
};

uchar  lookupType[256];

void BuildTypeLookups ()
{
  memset(lookupType, 0, 256 );
  lookupType['?'] = T_NULL;
  lookupType['R'] = T_REF;
  lookupType['C'] = T_CHAR;
  lookupType['I'] = T_INT;
  lookupType['J'] = T_INTC;
  lookupType['F'] = T_FLOAT;
  lookupType['B'] = T_DOUBLE;
  lookupType['3'] = T_VEC3;
  lookupType['V'] = T_VEC4;
  lookupType['S'] = T_STR;
  lookupType['U'] = T_BUF;
  lookupType['D'] = T_TIME;
  lookupType['F'] = T_FILE;
  lookupType['L'] = T_VAL;
  lookupType['T'] = T_TYPE;
  lookupType['P'] = T_PAIR;
}



void Value_t::Clear ()
{ 
  if (useStr(dt)) {
    delete v.str; v.str=0x0;
  } else {
    memset(&v, 0, sizeof(v));
  }
  dt = T_NULL;
}

Value_t& Value_t::Copy (const Value_t& op)
{
  Clear ();
  if (useStr(op.dt)) {
    SetStr ( *op.v.str );
  } else {
    memcpy ( &v, &op.v, sizeof(v));
  } 
  dt = op.dt;
  return *this;
}

const char* Value_t::getData ()
{
  return useStr(dt) ? (v.str)->c_str() : (char*) &v.c;
}

void Value_t::CheckSizes()
{
  int expected = 8 + 16 + 40;
  int value_sz = sizeof(Value_t);

  /* if (value_sz != expected) {
    printf("--- WARNING: Size of Value_t. Expected: %d, Value_t Size: %d\n", expected, value_sz);        
  } */
}

int Value_t::getDataLen()
{
  switch (dt) {
  case T_CHAR:   return sizeof(char); break;
  case T_INT:    return sizeof(int); break;
  case T_FLOAT:  return sizeof(float); break;
  case T_VEC4:   return sizeof(Vec4F); break;
  case T_STR:    return (v.str)->length(); break;
  case T_REF:    return sizeof(xlong); break;
  case T_BUF:    return 16;    break;
  case T_TIME:   return sizeof(sjtime); break;  
  };
  return 0;
}

void Value_t::MakePair(Value_t& v1, Value_t& v2)
{
  SetStr ( v1.WriteTyped() + "|" + v2.WriteTyped() );
  dt = T_PAIR;
}

void Value_t::getPair(Value_t& v1, Value_t& v2)
{
  uchar lt, rt;
  std::string left, rgt;
  strSplit ( *v.str, "|", left, rgt );
  lt = getType( left.at(0) );
  rt = getType( rgt.at(0) );
  left = strParseOutDelim( left, "(", ")" );
  rgt = strParseOutDelim( rgt, "(", ")");
  v1.FromStr ( lt, left );
  v2.FromStr ( rt, rgt );
}

void Value_t::FromTypedStr (std::string s)
{
  if (s.find('|') != std::string::npos) {     
    SetStr ( s );           // pair (copy verbatim)
    dt = T_PAIR;        
  } else {
    dt = getType ( s.at(0) );
    std::string val = strParseOutDelim( s, "(", ")" );
    FromStr ( dt, val );
  }
}

void Value_t::FromStr (uchar t, std::string s)
{
  Clear();
  if (useStr(t)) {
    SetStr ( s ); 
  } else {
    dt = t;
    (gConvTable[T_STR][dt]) ( (const char*) &v, (void*) &s, getTypeSz(t) );
  }
}

std::string Value_t::Write()
{  
  std::string s = "?";
  switch (dt) {
  case T_CHAR:   s = std::string(1, v.c); break;
  case T_INT:    s = iToStr(v.i); break;
  case T_FLOAT:  s = fToStr(v.f); break;
  case T_VEC4:   s = vecToStr(v.v4); break;
  case T_STR:    s = *v.str; break;
  case T_PAIR:   s = *v.str; break;
  case T_REF:    s = xlToStr(v.uid); break;
  case T_BUF:    s = std::string(v.buf);    break;
  case T_TIME:   TimeX t(v.tm); s = t.WriteDateTime();  break;
  };
  return s;
}

std::string Value_t::WriteTyped()
{
  std::string s = Write();
  uchar ch = getTypeCh( dt );

  if (dt==T_PAIR) return s;
  return std::string(1, ch) + "("+s+")";      // enclose in type char  
}

// typeless set 
void Value_t::SetBufToValue(char* buf, int pos, int len, Value_t src)
{
  switch (src.dt) {
  case T_REF:		              *(uxlong*) &buf[pos] = src.v.uid; break;
  case T_INT:                 *(int*)   &buf[pos] = src.v.i;   break;
  case T_CHAR: case T_INTC:   *(char*)  &buf[pos] = src.v.c;   break;
  case T_FLOAT:	case T_TIME:  *(float*) &buf[pos] = src.v.f;   break;
  case T_VEC4:                *(Vec4F*) &buf[pos] = src.v.v4;  break;
  case T_STR:	case T_PAIR: {
    int sz = (src.v.str)->length(); if (sz >= len) sz = len-1;
    memcpy(&buf[pos], (src.v.str)->c_str(), sz);
    buf[pos + sz] = '\0';
    } break;   
  };
}

void Value_t::Pack (char* buf, int maxlen)
{
  *buf = dt;

  if (useStr(dt)) {
    memcpy ( buf+8, (v.str)->c_str(), imin(maxlen, (v.str)->length()) );    
  } else {
    memcpy ( buf+8, &v, imin(maxlen, sizeof(v) ) );	
  }
}	

void Value_t::Unpack (char* buf) 
{
  dt = *buf; 
  if (useStr(dt)) {
    SetStr (buf + 8);
  } else {
    memcpy (v.buf, buf + 8, 16);
  }
}   

void Value_t::SetStr (std::string src)
{
  if ( !useStr(dt) || v.str==0x0 ) {
    if (v.str !=0 ) delete v.str;
    v.str = new std::string;
  }
  *(v.str) = src;
  dt = T_STR;
}

std::string Value_t::getStr()
{
  // if already string, return it
  if (useStr(dt)) return *(v.str);  

  // i am not a string..
  Value_t as_str ( *this);          // create copy to preserve self
  as_str.Cast ( T_STR );            // cast self to string, with separate string storage
  return *(as_str.v.str);
}
uchar Value_t::getC()
{
  uchar c;
  (gConvTable[dt][T_CHAR]) ( (const char*) &v, (void*) &c, sizeof(uchar) );  
  return c;
}
int Value_t::getI()
{
  int c;
  (gConvTable[dt][T_INT]) ( (const char*) &v, (void*) &c, sizeof(int) );  
  return c;
}
float Value_t::getF()
{
  float c;
  (gConvTable[dt][T_FLOAT]) ( (const char*) &v, (void*) &c, sizeof(float) );  
  return c;
}
Vec4F Value_t::getV4()
{
  Vec4F c;
  (gConvTable[dt][T_VEC4]) ( (const char*) &v, (void*) &c, sizeof(Vec4F) );  
  return c;
}
xlong Value_t::getXL()
{
  xlong c;
  (gConvTable[dt][T_REF]) ( (const char*) &v, (void*) &c, sizeof(xlong) );  
  return c;
}


// casting
//

// cast self to type
Value_t& Value_t::Cast (char dest_dt)
{
  Value_t src = *this;
  if (useStr(dest_dt)) SetStr("");

  // function table lookup, to cast from src to dest
  if (useStr(src.dt)) {
    (gConvTable[src.dt][dest_dt]) ( (const char*) src.v.str, (void*) &v, sizeof(v) );   // Value_t string indirect
  } else {
    (gConvTable[src.dt][dest_dt]) ( (const char*) &src.v, (void*) &v, sizeof(v) );
  }

  return *this;
}

// cast from other to self
Value_t& Value_t::Cast (const Value_t& src)
{
  // function table lookup, to cast from src to dest
  if (useStr(src.dt)) {
    // *NOTE* we can assume if useStr(src.dt) then src.v.str must be valid
    (gConvTable[src.dt][dt]) ( (const char*) src.v.str, (void*) &v, sizeof(v) );   // Value_t string indirect
  } else {
    (gConvTable[src.dt][dt]) ( (const char*) &src.v, (void*) &v, sizeof(v) );
  }

  return *this;
}

// cast from one value to anoother 
Value_t Value_t::Cast (Value_t& src, char dest_dt)
{
  Value_t dst{};
  dst.dt = dest_dt;
  if (useStr(dest_dt)) dst.SetStr("");

  // function table lookup, to cast from src to dest
  if (useStr(src.dt)) {
    (gConvTable[src.dt][dest_dt]) ( (const char*) src.v.str, (void*) &dst.v, sizeof(v) );   // Value_t string indirect
  } else {
    (gConvTable[src.dt][dest_dt]) ( (const char*) &src.v, (void*) &dst.v, sizeof(v) );
  } 

  return dst;
}


//------------------------------------------------- KeyValues
//

void KeyValues::Clear()
{
  entries.clear ();
  index.clear ();
}

size_t KeyValues::Add(const std::string& name, uchar dt)
{
  size_t idx = entries.size();
  
  KeyVal_t e;  
  e.key = name;
  e.value.dt = dt;

  entries.push_back(e);
  index[name] = idx;
  return idx;
}

size_t KeyValues::Add (const std::string& name, Value_t& val)
{
  size_t idx = entries.size();

  KeyVal_t e;
  e.key = name;
  e.value = val;

  entries.push_back(e);
  index[name] = idx;
  return idx;
}

Value_t* KeyValues::Find (const std::string& name)
{
  auto it = index.find(name);
  if (it == index.end()) return nullptr;
  return &entries[it->second].value;
}

size_t KeyValues::FindNdx(const std::string& name)
{
  auto it = index.find(name);
  return (it == index.end()) ? nullndx : it->second;
}
