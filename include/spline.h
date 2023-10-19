
#ifndef DEF_SPLINE
	#define DEF_SPLINE

	#include "datax.h" 
	#include "quaternion.h"	

	#define SP_TIME		0
	#define SP_KNOTS	1

	struct SplineChan {
		char		m_buf;
		char		m_eval;		// evaluation: '0' off, 'p' point, 'l' linear, 'b' bspline, 'c' catmull-rom, 'z' bezier
		char		m_type;		//  data type: '3' vec3, '4' vec4, 'q' quaternion		
		Vec4F	m_result;
	};


	class HELPAPI Spline : public DataX {
	public:
		Spline ();

		// Construct Spline
		void		CreateSpline ();
		void		AddChannel ( int ch, char eval, char dtype );
		void		AddKeyToSpline (float t);
		void		SetKey (int ch, Vec4F val, int i=-1);
		void		AlignSpline ();
		void		UpdateSpline ();
		int			FindKey( float t, float& u);
		
		// Evaluate Spline
		void		EvaluateSpline ( float t );							
		Vec4F	EvaluatePoint		( int b, int k, float u );			// vec3
		Vec4F	EvaluateLinear		( int b, int k, float u );			
		Vec4F	EvaluateBSpline		( int b, int k, float u );
		Vec4F	EvaluateCatmullRom	( int b, int k, float u );
		Vec4F	EvaluateBezier		( int b, int k, float u );
		
		Vec4F	EvaluateQSlerp		( int b, int k, float u );			// quaternion (spherical-linear)

		Vec3F	getSplineV3 ( int ch )	{ return (Vec3F) m_Channels[ch].m_result; }			// get evaluation results
		Vec4F	getSplineV4 ( int ch )	{ return (Vec4F) m_Channels[ch].m_result; }
		Quaternion	getSplineQ ( int ch )	{ return (Quaternion) m_Channels[ch].m_result; }

		int			getNumKeys();
		float		getKeyTime(int i)	{ return GetElemFloat(SP_TIME, i); }
		//Vec3F	getKeyPos(int i)	{ return *GetElemVec3(SP_POS, i); }
		//Quaternion  getKeyRot(int i)	{ return *GetElemQuat(SP_ROT, i); }

	public:		
		
		bool			m_rebuild;
		int				m_num;
		int				m_k;				// current key index

		int				m_degree;
		float			m_tension;
		float			m_control_amt;
		float			m_normal_amt;

		std::vector<SplineChan>	m_Channels;
	};

#endif
	
