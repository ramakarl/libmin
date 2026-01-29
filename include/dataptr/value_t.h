

#ifndef DEF_VALUE
	#define DEF_VALUE

	#include "common_defs.h"	
	#include "timex.h"
	#include "vec.h"
	#include <string>	
	#include <unordered_map>

	// Primary value types	
	#define T_REF				0			// Date types (3-bit value)
	#define T_INT				1	
	#define T_FLOAT			2
	#define T_DOUBLE		3
	#define	T_TIME			4
	#define	T_FILE			5
	#define T_VEC3			6
	#define T_VEC4			7
	#define T_STR				8
	#define T_CHAR			9
	#define T_BUF				10
	#define T_VAL				11		// Typeless value. See: DataValue class	
	#define T_TYPE			12		// Type indicator (1 byte)
	#define T_INTC			13		// Integer in one byte	
	#define T_LIST			14		// Packed lists	
	#define T_NULL			16
	#define T_BASIC			20		// End of basic types

	#define T_UNKNOWN		254
	#define T_ERR				255
	
	// type funcs
	static inline std::string getTypeStr(uchar dt) 
	{
		std::string t = "?";		// ? = T_UNKNOWN = 254
		switch (dt) {
		case T_REF:		t = "REF"; break;
		case T_INT:		t = "INT"; break;
		case T_INTC:	t = "INTC"; break;
		case T_FLOAT:	t = "FLOAT"; break;
		case T_DOUBLE:t = "DOUBLE"; break;
		case T_TIME:	t = "TIME"; break;
		case T_FILE:	t = "FILE"; break;
		case T_VEC3:	t = "VEC3"; break;
		case T_VEC4:	t = "VEC4"; break;
		case T_STR:		t = "STR"; break;
		case T_CHAR:	t = "CHAR"; break;
		case T_BUF:		t = "BUF"; break;
		case T_VAL:		t = "VALUE"; break;	
		case T_TYPE:	t = "TYPE"; break;		
		case T_LIST:  t = "LIST"; break;
		case T_NULL:	t = "NULL"; break;
		};
		return t;
	}

	// Value
  // - this class is intended for typeless simplicity & convenience  
	//
	class Value_t {
	public:
		Value_t ()		{ dt = T_NULL; str.clear(); }
		Value_t (const Value_t& op) { dt=op.dt; memcpy (buf, op.buf, 16); str=op.str; }
		Value_t& operator= (const Value_t& op) { dt=op.dt; memcpy (buf, op.buf, 16); str=op.str; return *this; }
		~Value_t ()		{};

		// null value
		static const Value_t nullval;
		bool isNullType() { return (dt == T_NULL); }
		bool isNull()			{ return (dt == T_NULL) || (dt==T_VEC4 && isnan(vec.x)) || (dt==T_FLOAT && isnan(f)); }

		// typed constructors
		Value_t (char v)	{ setC(v); }
		Value_t (int v)   { setI(v); }
		Value_t (float v) { setF(v); }
		Value_t (Vec4F v) { setV4(v); }
		Value_t (xlong v) { setRef(v); }
		Value_t (char* v, int l) { setBuf(v,l); }
		Value_t (TimeX v) { setTime(v); }
		Value_t (std::string v ) { setStr(v); }
		void FromBuf(char* v) { dt = *v; memcpy(buf, v + 8, 16); str = std::string(buf, 16); }
		void MakePair ( Value_t& v1, Value_t& v2 );
		static Value_t MakeList ( Value_t& v );
		void AppendList ( Value_t& v );

		// typeless functions	  
		void Clear ()  { dt = T_NULL; str.clear(); }
		static Value_t Cast(Value_t& val, char dt);
		Value_t& CastUpdate ( Value_t& val );

		// typed get/set
		void				setC (uchar v)					{ c = v;		dt = T_CHAR; }
		void				setI (int v)						{ i = v;		dt = T_INT; }
		void				setF (float v)					{ f = v;		dt = T_FLOAT; }
		void				setV4 (Vec4F v)					{ vec = v;	dt = T_VEC4; }
		void				setStr (std::string v)	{ str = v;	dt = T_STR; }
		void				setRef (xlong v)				{ uid = v;	dt = T_REF; }
		void				setBuf (char* v, int l) { memcpy(buf, v, l); dt = T_BUF; }
		void				setTime (TimeX v)				{ tm = v.GetSJT(); dt = T_TIME; }
		const char* getData ();
		int					getDataLen ();		
		TimeX				getTime () { return TimeX(tm); }		

		// printing	
		std::string Write();
		std::string WriteTyped();
		uchar				getType(uchar str);
		uchar				getTypeCh(uchar dt);

		static void CheckSizes();


	public:		
		// 8 bytes 
		uint64_t dt;						// 1-byte type (padded to 8)

		// 16 bytes
		union {									// anonymous union (C++)
			uchar		c;						// 1 byte
			int			i;						// 4 bytes
			float		f;						// 4 bytes
			xlong		uid;					// 8 bytes
			sjtime	tm;						// 8 bytes
			Vec4F		vec;					// 16 bytes
			char		buf[16];			// 16 bytes						
		};
		
		std::string str;				// 40 bytes (debug) / 32 bytes (release) + heap

	};

	// VecValues
	//
	typedef std::vector<Value_t>		VecValues;

	// KeyValues
	//
	struct KeyVal_t {
		std::string key;
		Value_t			value;
	};

	class KeyValues {
	public:

		void Clear ();
		size_t Add(const std::string& name, uchar dt );
		size_t Add(const std::string& name, Value_t& val );
		Value_t* Find(const std::string& name);
		size_t FindNdx(const std::string& name);
		Value_t& Get(size_t i)								{ return entries[i].value; }
		const Value_t& Get(size_t i) const		{ return entries[i].value; }
		size_t Size() const { return entries.size(); }
		
		static constexpr size_t nullndx = size_t(-1);

	private:
			std::vector < KeyVal_t > entries;
			std::unordered_map <std::string, size_t> index;

	
	};


#endif