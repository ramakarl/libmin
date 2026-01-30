

#ifndef DEF_VALUE
	#define DEF_VALUE

	#include "common_defs.h"	
	#include "timex.h"
	#include "vec.h"
	#include "string_helper.h"
	#include <string>	
	#include <unordered_map>

	// Primary value types
	#define T_NULL		0
	#define T_REF			1
	#define T_CHAR		2
	#define T_INT			3
	#define T_INTC		4			// Integer in one byte	
	#define T_FLOAT		5
	#define T_DOUBLE	6
	#define T_VEC3		7
	#define T_VEC4		8
	#define T_STR			9
	#define T_BUF			10
	#define	T_TIME		11
	#define	T_FILE		12
	#define T_VAL			13		// Typeless value. Value_t
	#define T_TYPE		14		// Type indicator (1 byte)
	#define T_PAIR		15
	#define T_BASIC		20		// End of basic types

	#define T_UNKNOWN		254
	#define T_ERR				255

	inline static bool useStr (uchar dt)		{return (dt==T_STR || dt==T_PAIR);}

	// Value
  // - this class is intended for typeless simplicity & convenience  
	//
	class Value_t {
	public:
		Value_t ()		{ dt = T_NULL; memset(&v, 0, sizeof(v)); }
		~Value_t()		{ Clear(); }

		Value_t (const Value_t& op)							{ Copy ( op ); }
		Value_t& operator= (const Value_t& op)	{ Copy ( op ); return *this; }

		// null value
		static const Value_t nullval;
		bool isNullType() { return (dt == T_NULL); }
		bool isNull()			{ return (dt == T_NULL) || (dt==T_VEC4 && isnan(v.v4.x)) || (dt==T_FLOAT && isnan(v.f)); }

		// constructors
		Value_t (char src)					{ setC(src); }
		Value_t (int src)						{ setI(src); }
		Value_t (float src)					{ setF(src); }
		Value_t (Vec4F src)					{ setV4(src); }
		Value_t (xlong src)					{ setRef(src); }
		Value_t (char* src, int l)	{ setBuf(src,l); }
		Value_t (TimeX src)					{ setTime(src); }
		Value_t (std::string src )  { SetStr(src); }

		// packed formats
		void				Pack (char* buf, int maxlen);
		void				Unpack (char* v);		
		void				MakePair ( Value_t& v1, Value_t& v2 );
		void				getPair ( Value_t& v1, Value_t& v2 );

		// typeless functions	  
		void				Clear ();
		void				FromStr ( uchar dt, std::string val );
		void				FromTypedStr ( std::string str );
		static void	SetBufToValue(char* buf, int pos, int len, Value_t val);
		static Value_t Cast (Value_t& val, char dest_dt);
		Value_t&		Cast ( char dest_dt );
		Value_t&		Cast ( const Value_t& val );
		Value_t&		Copy ( const Value_t& val );

		// typed get/set
		void				setC (uchar src)					{ v.c = src;		dt = T_CHAR; }
		void				setI (int src)						{ v.i = src;		dt = T_INT; }
		void				setF (float src)					{ v.f = src;		dt = T_FLOAT; }
		void				setV4 (Vec4F src)					{ v.v4 = src;	dt = T_VEC4; }		
		void				setRef (xlong src)				{ v.uid = src;	dt = T_REF; }
		void				setBuf (char* src, int l) { memcpy(v.buf, src, l); dt = T_BUF; }
		void				setTime (TimeX src)				{ v.tm = src.GetSJT(); dt = T_TIME; }
		void				SetStr (std::string src);
		std::string getStr();
		uchar				getC();
		int					getI();
		float				getF();
		xlong				getXL();
		Vec4F				getV4();

		const char* getData ();
		int					getDataLen ();		
		TimeX				getTime () { return TimeX(v.tm); }		

		// printing	
		std::string Write();
		std::string WriteTyped();		

		static void CheckSizes();

	public:		
		// 8 bytes 
		uint64_t dt;						// 1-byte type (padded to 8)

		// 16 bytes
		union Payload {
			Payload()  { memset(buf,0,16); }		// required for non-trivial union (Vec4F has constuctors & operators)

			uchar		c;						// 1 byte
			int			i;						// 4 bytes
			float		f;						// 4 bytes
			xlong		uid;					// 8 bytes
			sjtime	tm;						// 8 bytes			
			std::string* str;			// 8 bytes + 40 bytes (debug) / 32 bytes (release) + heap
			Vec4F		v4;						// 16 bytes
			uint128_t u128;				// 16 bytes
			char		buf[16];			// 16 bytes			
		} v;			// <-- member name of union

		// total: 24 bytes
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

	//----------- Type system

	typedef void (*ConvFn) (const char* s, void* d, int len);	
	extern ConvFn gConvTable[16][16];

	extern uchar				lookupType[256];
	static uchar				lookupTypeCh[16] = {'?', 'R', 'C', 'I', 'J', 'F', 'B', '3', 'V', 'S', 'U', 'D', 'X', 'L', 'T', 'P'};
	static uchar				lookupTypeSz[16] = {0, sizeof(xlong), sizeof(char), sizeof(int), sizeof(char), sizeof(float), sizeof(double), sizeof(Vec3F), sizeof(Vec4F), sizeof(void*), 16, sizeof(sjtime), sizeof(void*), sizeof(Value_t), sizeof(char), sizeof(void*) };
	static std::string	lookupTypeStr[16] = { "NULL", "REF", "CHAR", "INT", "INTC", "FLOAT", "DOUBLE", "VEC3", "VEC4", "STR", "BUF", "TIME", "FILE", "VAL", "TYPE", "PAIR" };

	// type funcs
	void BuildTypeLookups ();
	inline uchar				getType (uchar ch)   { return lookupType[ch]; }
	inline uchar				getTypeCh (uchar dt) { return lookupTypeCh[dt]; }
	inline std::string	getTypeStr(uchar dt) { return lookupTypeStr[dt]; }
	inline int					getTypeSz (uchar dt) { return lookupTypeSz[dt]; }
	
	inline void convNo 		  (const char* s, void* d, int len)		{  }
	inline void convCopy		(const char* s, void* d, int len)		{ memcpy(d, s, len); }
	// to_ref
	inline void convIToR		(const char* s, void* d, int len)		{ (*(xlong*)d) = (xlong) *(int*) s; }
	inline void convFToR		(const char* s, void* d, int len)		{ (*(xlong*)d) = (xlong) *(float*)s; }
	inline void convV4ToR		(const char* s, void* d, int len)		{ (*(xlong*)d) = (xlong) ((Vec4F*)s)->x; }
	inline void convSToR		(const char* s, void* d, int len)		{ (*(xlong*)d) = strToI64(*(std::string*)s); }
	// to_char
	inline void convIToC		(const char* s, void* d, int len)		{ (*(uchar*)d) = (uchar) *(int*) s; }
	inline void convFToC		(const char* s, void* d, int len)		{ (*(uchar*)d) = (uchar) *(float*)s; }
	inline void convV4ToC		(const char* s, void* d, int len)		{ (*(uchar*)d) = (uchar) ((Vec4F*)s)->x; }
	inline void convSToC		(const char* s, void* d, int len)		{ (*(uchar*)d) = !((std::string*)s)->empty() ? ((std::string*)s)->at(0) : ' '; }
	// to_int
	inline void convRToI		(const char* s, void* d, int len)		{ (*(int*)d) = (int) *(xlong*)s; }
	inline void convCToI		(const char* s, void* d, int len)		{ (*(int*)d) = (int) *(uchar*)s; }
	inline void convFToI		(const char* s, void* d, int len)		{ (*(int*)d) = (int) *(float*)s; }
	inline void convV4ToI		(const char* s, void* d, int len)		{ (*(int*)d) = (int) ((Vec4F*)s)->x; }	
	inline void convSToI		(const char* s, void* d, int len)		{ (*(int*)d) = strToI(*(std::string*)s); }
	// to_float
	inline void convNToF		(const char* s, void* d, int len)		{ (*(float*)d) = NAN; }
	inline void convRToF		(const char* s, void* d, int len)		{ (*(float*)d) = (float) *(xlong*) s; }	
	inline void convCToF		(const char* s, void* d, int len)		{ (*(float*)d) = (float) *(uchar*) s; }
	inline void convIToF		(const char* s, void* d, int len)		{ (*(float*)d) = (float) *(int*)s; }
	inline void convV4ToF		(const char* s, void* d, int len)		{ (*(float*)d) = (float) ((Vec4F*)s)->x; }
	inline void convSToF		(const char* s, void* d, int len)		{ (*(float*)d) = strToF(*(std::string*)s); }
	// to_str	
	inline void convNToS		(const char* s, void* d, int len)		{ (*(std::string*)d) = ""; }
	inline void convRToS		(const char* s, void* d, int len)		{ (*(std::string*)d) = xlToStr(*(xlong*)s); }
	inline void convIToS		(const char* s, void* d, int len)		{ (*(std::string*)d) = iToStr(*(int*)s); }
	inline void convFToS		(const char* s, void* d, int len)		{ (*(std::string*)d) = fToStr(*(int*)s); }
	inline void convV4ToS		(const char* s, void* d, int len)		{ (*(std::string*)d) = vecToStr(*(Vec4F*)s); }
	inline void convCToS		(const char* s, void* d, int len)		{ (*(std::string*)d) = std::string(1, *(uchar*)s); }
	inline void convBfToS   (const char* s, void* d, int len)		{ (*(std::string*)d) = std::string(s); }
	// to_vec4
	inline void convNToV4		(const char* s, void* d, int len)		{ ((Vec4F*)d)->Set(NAN, NAN, NAN, NAN); }
	inline void convRToV4		(const char* s, void* d, int len)		{ float v = *(xlong*) s; ((Vec4F*)d)->Set(v, v, v, v);}
	inline void convCtoV4		(const char* s, void* d, int len)		{ float v = *(uchar*) s; ((Vec4F*)d)->Set(v, v, v, v);}
	inline void convItoV4		(const char* s, void* d, int len)		{ float v = *(int*)   s; ((Vec4F*)d)->Set(v, v, v, v);}
	inline void convFtoV4		(const char* s, void* d, int len)		{ float v = *(float*) s; ((Vec4F*)d)->Set(v, v, v, v);}
	// to_buf
	inline void convRToBuf	(const char* s, void* d, int len)		{ std::string m = xlToStr(*(xlong*) s); int l = imin(len-1,m.length()); memcpy( d, m.c_str(), l ); ((char*) d)[l]='\0'; }	
	inline void convCToBuf	(const char* s, void* d, int len)		{ * (char*) d = *s; ((char*)d)[1]='\0'; }
	inline void convIToBuf	(const char* s, void* d, int len)		{ std::string m = iToStr(*(int*) s);    int l = imin(len-1,m.length()); memcpy( d, m.c_str(), l ); ((char*) d)[l]='\0'; }	
	inline void convFToBuf	(const char* s, void* d, int len)		{ std::string m = fToStr(*(float*) s);  int l = imin(len-1,m.length()); memcpy( d, m.c_str(), l ); ((char*) d)[l]='\0'; }	
	inline void convV4ToBuf	(const char* s, void* d, int len)		{ std::string m = vecToStr(*(Vec4F*) s);int l = imin(len-1,m.length()); memcpy( d, m.c_str(), l ); ((char*) d)[l]='\0'; }	
	inline void convSToBuf	(const char* s, void* d, int len)		{ int l = imin(len-1, ((std::string*)s)->length()); memcpy( d, ((std::string*)s)->c_str(), l); ((char*) d)[l]='\0'; }
	inline void convBfToBf  (const char* s, void* d, int len)		{ int l = imin(len-1, strlen(s)); memcpy( d, s, l ); ((char*) d)[l]='\0'; }

#endif