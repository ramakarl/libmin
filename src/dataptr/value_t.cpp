
#include "value_t.h"
#include "string_helper.h"


static const Value_t nullval = Value_t();

void Value_t::CheckSizes()
{
  int expected = 8 + 16 + 40;
  int value_sz = sizeof(Value_t);

  /* if (value_sz != expected) {
    printf("--- WARNING: Size of Value_t. Expected: %d, Value_t Size: %d\n", expected, value_sz);        
  } */
}

const char* Value_t::getData ()
{
  return (dt == T_STR) ? str.c_str() : (char*) &c;     
}

int Value_t::getDataLen()
{
  switch (dt) {
  case T_CHAR:   return sizeof(char); break;
  case T_INT:    return sizeof(int); break;
  case T_FLOAT:  return sizeof(float); break;
  case T_VEC4:   return sizeof(Vec4F); break;
  case T_STR:    return str.length(); break;
  case T_REF:    return sizeof(xlong); break;
  case T_BUF:    return 16;    break;
  case T_TIME:   return sizeof(sjtime); break;  
  };
  return 0;
}

// Get type from literal char
uchar Value_t::getType( uchar lit )
{
  switch (lit) {
  case 'C': return T_CHAR; break;
  case 'I': return T_INT; break;
  case 'F': return T_FLOAT; break;
  case 'D': return T_TIME; break;
  case 'V': return T_VEC4; break;
  case 'S': return T_STR; break;
  case 'R': return T_REF; break;
  case 'P': return T_PAIR; break;
  }
  return T_NULL;
}

uchar Value_t::getTypeCh( uchar dt)
{
  switch (dt) {
  case T_CHAR:  return 'C'; break;
  case T_INT:   return 'I'; break;
  case T_FLOAT: return 'F'; break;
  case T_TIME:  return 'D'; break;
  case T_VEC4:  return 'V'; break;
  case T_STR:   return 'S'; break;
  case T_REF:   return 'R'; break;
  case T_PAIR:  return 'P'; break;
  }
  return '?';
}

void Value_t::MakePair(Value_t& v1, Value_t& v2)
{
  dt = T_PAIR;
  str = v1.WriteTyped() + "|" + v2.WriteTyped();
  
  memset(buf, T_NULL, 16);
  buf[0] = v1.dt;
  buf[1] = v2.dt;
}

void Value_t::FromStr(std::string s)
{
  if (s.find('|') != std::string::npos) {     
    dt = T_PAIR; str = s;                   // pair (copy verbatim)
  } else {
    dt = getType ( s.at(0) );
    std::string val = strParseOutDelim( s, "(", ")" );
    SetValue ( dt, val );
  }
}

void Value_t::getPair(Value_t& v1, Value_t& v2)
{
  uchar lt, rt;
  std::string left, rgt;
  strSplit ( str, "|", left, rgt );
  lt = getType( left.at(0) );
  rt = getType( rgt.at(0) );
  left = strParseOutDelim( left, "(", ")" );
  rgt = strParseOutDelim( rgt, "(", ")");
  v1.SetValue ( lt, left );
  v2.SetValue ( rt, rgt );
}

std::string Value_t::Write()
{  
  std::string s = "?";
  switch (dt) {
  case T_CHAR:   s = std::string(1, c); break;
  case T_INT:    s = iToStr(i); break;
  case T_FLOAT:  s = fToStr(f); break;
  case T_VEC4:   s = vecToStr(vec); break;
  case T_STR:    s = str; break;
  case T_PAIR:   s = str; break;
  case T_REF:    s = xlToStr(uid); break;
  case T_BUF:    s = std::string(buf);    break;
  case T_TIME:   TimeX t(tm); s = t.WriteDateTime();  break;
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
void Value_t::SetBufToValue(char* buf, int pos, int len, Value_t val)
{
  switch (val.dt) {
  case T_REF:		              *(uxlong*) &buf[pos] = val.uid;	    break;
  case T_INT:                 *(int*)   &buf[pos] = val.i;  break;
  case T_CHAR: case T_INTC:   *(char*)  &buf[pos] = val.c;  break;
  case T_FLOAT:	case T_TIME:  *(float*) &buf[pos] = val.f;  break;
  case T_VEC4:                *(Vec4F*) &buf[pos] = val.vec;  break;
  case T_STR:	case T_PAIR: {
    int sz = val.str.length(); if (sz >= len) sz = len-1;
    memcpy(&buf[pos], val.str.c_str(), sz);
    buf[pos + sz] = '\0';
    } break;   
  };
}

void Value_t::SetValue(uchar t, std::string s)
{
  dt = t;
  str.clear();
  switch (dt) {
  case T_REF:     uid = strToI64 ( s ); break;
  case T_CHAR:    c = strToC(s); break;
  case T_INT:     i = strToI(s); break;
  case T_INTC:    c = (uchar) strToI(s); break;
  case T_FLOAT:   f = strToF(s); break;
  case T_VEC4:    vec = strToVec4(s,','); break;
  case T_STR:     str = s; break;
  };
}


// typeless set operator
//
Value_t& Value_t::CastUpdate (Value_t& val)
{
  if (val.dt == dt) {    
    switch (dt) {
    case T_STR:   str = val.str;  break;
    case T_PAIR:  str = val.str;  break;
    case T_INT:   i = val.i;      break;
    case T_CHAR:  c = val.c;      break;
    case T_FLOAT: f = val.f;      break;
    case T_VEC4:  vec = val.vec;  break;
    case T_REF:   uid = val.uid;  break;
    case T_TIME:  tm = val.tm;    break;
    default:
      memcpy(buf, val.buf, 16);
      str = val.str;
    };  
  } else {
    switch (dt) {
    case T_REF:
      switch (val.dt) {      
      case T_INT:     uid = xlong(val.i); break;
      case T_FLOAT:   uid = xlong(val.f); break;
      case T_VEC4:    uid = xlong(val.vec.w); break;
      case T_STR:     uid = strToI64(val.str); break;
      };
      break;
    case T_STR:
      switch (val.dt) {
      case T_REF:     str = "?WORD?"; break;
      case T_INT:     str = iToStr(val.i); break;
      case T_FLOAT:   str = fToStr(val.f); break;
      case T_VEC4:    str = vecToStr(val.vec); break;
      };
      break;
    case T_CHAR:
      switch (val.dt) {            
      case T_INT:     c = char(val.i); break;
      case T_FLOAT:   c = char(val.f); break;
      case T_VEC4:    c = char(val.vec.x); break;
      };
      break;
    case T_INT:
      switch (val.dt) {
      case T_STR:     i = strToI(val.str);    break;      
      case T_FLOAT:   i = int(val.f);         break;
      case T_VEC4:    i = int(val.vec.x);     break;
      };
      break;
    case T_FLOAT:
      switch (val.dt) {
      case T_STR:     f = strToF(val.str);    break;
      case T_INT:     f = float(val.i);       break;      
      case T_VEC4:    f = float(val.vec.x);   break;
      };
      break;
    case T_VEC4:
      switch (val.dt) {
      case T_STR:     vec = strToVec4(val.str,',');  break;
      case T_INT:     vec.x = float(val.i);   break;
      case T_FLOAT:   vec.x = vec.z + float(val.f) * (vec.w-vec.z);   break;    // interpolation
      };
      break;
    };
  }
  return *this;
}

Value_t Value_t::Cast (Value_t& val, char dest_dt)
{
  if (val.dt == dest_dt) return val;			// same type, return it
  switch (dest_dt) {  
  case T_STR:
    if (val.dt == T_NULL)    return Value_t("");
    if (val.dt == T_REF)     return Value_t("?WORD?");        // must be resolved by DB, not by casting
    if (val.dt == T_INT)     return Value_t(iToStr(val.i));
    if (val.dt == T_FLOAT)   return Value_t(fToStr(val.f));
    if (val.dt == T_VEC4)    return Value_t(vecToStr(val.vec));
    break;
  case T_CHAR:
    if (val.dt == T_INT)     return Value_t(char(val.i));
    if (val.dt == T_FLOAT)   return Value_t(char(val.f));
    if (val.dt == T_VEC4)    return Value_t(char(val.vec.x));
    break;
  case T_INT:
    if (val.dt == T_REF)     return Value_t(int(val.uid));
    if (val.dt == T_NULL)    return Value_t(int(0));
    if (val.dt == T_CHAR)    return Value_t(int(val.c));
    if (val.dt == T_FLOAT)   return Value_t(int(val.f));
    if (val.dt == T_VEC4)    return Value_t(int(val.vec.x));
    break;
  case T_FLOAT:
    if (val.dt == T_REF)     return Value_t(float(val.uid));
    if (val.dt == T_NULL)    return Value_t(float(NAN));
    if (val.dt == T_CHAR)    return Value_t(float(val.c));
    if (val.dt == T_INT)     return Value_t(float(val.i));
    if (val.dt == T_VEC4)    return Value_t(float(val.vec.x));
    break;
  case T_VEC4:
    if (val.dt == T_REF)     return Value_t(Vec4F(0,0,0,val.uid));
    if (val.dt == T_NULL)    return Value_t(Vec4F(NAN, NAN, NAN, NAN));
    if (val.dt == T_CHAR)    return Value_t(Vec4F(val.c, 0, 0, 0));
    if (val.dt == T_INT)     return Value_t(Vec4F(val.i, 0, 0, 0));
    if (val.dt == T_FLOAT)   return Value_t(Vec4F(val.f, 0, 0, 0));
    break;
  };
  // dbgprintf( "ERROR: CastValue: Unable to cast from %s (%d) to %s (%d).\n", getDTStr(val.dt).c_str(), val.dt, getDTStr(dest_dt).c_str(), dest_dt);  
  return Value_t();
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
