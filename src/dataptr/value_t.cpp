
#include "value_t.h"
#include "string_helper.h"


static const Value_t nullval = Value_t();

void Value_t::CheckSizes()
{
  int expected = 8 + 16 + 40;
  int value_sz = sizeof(Value_t);

  if (value_sz != expected) {
    printf("--- Size of Value_t is wrong:\n");
    printf("  Expected: %d,  Value_t Size: %d\n", expected, value_sz);
    printf("---\n");
    exit(-77);
  }

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
char Value_t::getType( char lit )
{
  switch (lit) {
  case 'C': return T_CHAR; break;
  case 'I': return T_INT; break;
  case 'F': return T_FLOAT; break;
  case 'D': return T_TIME; break;
  case 'V': return T_VEC4; break;
  case 'S': return T_STR; break;
  case 'R': return T_REF; break;
  }
  return T_NULL;
}

std::string Value_t::Write()
{  
  switch (dt) {
  case T_CHAR:   return std::string(1, c); break;
  case T_INT:    return iToStr(i); break;
  case T_FLOAT:  return fToStr(f); break;
  case T_VEC4:   return vecToStr(vec); break;
  case T_STR:    return str; break;
  case T_REF:    return xlToStr(uid); break;
  case T_BUF:    return std::string(buf);    break;
  case T_TIME:   
    TimeX t(tm);
    return t.WriteDateTime(); 
    break;
  };
  return "?";
}

// typeless set operator
//
Value_t& Value_t::CastUpdate (Value_t& val)
{
  if (val.dt == dt) {    
    switch (dt) {
    case T_STR:   str = val.str;  break;
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
    if (val.dt == T_NULL)    return Value_t(int(0));
    if (val.dt == T_CHAR)    return Value_t(int(val.c));
    if (val.dt == T_FLOAT)   return Value_t(int(val.f));
    if (val.dt == T_VEC4)    return Value_t(int(val.vec.x));
    break;
  case T_FLOAT:
    if (val.dt == T_NULL)    return Value_t(float(NAN));
    if (val.dt == T_CHAR)    return Value_t(float(val.c));
    if (val.dt == T_INT)     return Value_t(float(val.i));
    if (val.dt == T_VEC4)    return Value_t(float(val.vec.x));
    break;
  case T_VEC4:
    if (val.dt == T_NULL)    return Value_t(Vec4F(NAN, NAN, NAN, NAN));
    if (val.dt == T_CHAR)    return Value_t(Vec4F(val.c, 0, 0, 0));
    if (val.dt == T_INT)     return Value_t(Vec4F(val.i, 0, 0, 0));
    if (val.dt == T_FLOAT)   return Value_t(Vec4F(val.f, 0, 0, 0));
    break;  
  };
  // dbgprintf( "ERROR: CastValue: Unable to cast from %s (%d) to %s (%d).\n", getDTStr(val.dt).c_str(), val.dt, getDTStr(dest_dt).c_str(), dest_dt);  
  return Value_t();
}

//------------------------------------------------- KeyValues_t
//

void KeyValues_t::Clear()
{
  entries.clear ();
  index.clear ();
}

size_t KeyValues_t::Add(const std::string& name, uchar dt)
{
  size_t idx = entries.size();
  
  KeyVal_t e;  
  e.key = name;
  e.value.dt = dt;

  entries.push_back(e);
  index[name] = idx;
  return idx;
}

size_t KeyValues_t::Add (const std::string& name, Value_t& val)
{
  size_t idx = entries.size();

  KeyVal_t e;
  e.key = name;
  e.value = val;

  entries.push_back(e);
  index[name] = idx;
  return idx;
}

Value_t* KeyValues_t::Find (const std::string& name)
{
  auto it = index.find(name);
  if (it == index.end()) return nullptr;
  return &entries[it->second].value;
}

size_t KeyValues_t::FindNdx(const std::string& name)
{
  auto it = index.find(name);
  return (it == index.end()) ? nullndx : it->second;
}
