
#ifndef GEOM_HELPER

	#define GEOM_HELPER

	#include "common_defs.h"
	#include "vec.h"
	class Camera3D;
	

	// Geometry utility functions	
	HELPAPI bool		intersectLineLine(Vec3F p1, Vec3F p2, Vec3F p3, Vec3F p4, Vec3F& pa, Vec3F& pb, double& mua, double& mub);
	HELPAPI Vec3F	intersectLineLine(Vec3F p1, Vec3F p2, Vec3F p3, Vec3F p4);	
	HELPAPI Vec3F	intersectLinePlane(Vec3F p1, Vec3F p2, Vec3F p0, Vec3F pnorm);
	HELPAPI bool		intersectLineBox(Vec3F p1, Vec3F p2, Vec3F bmin, Vec3F bmax, float& t);
	HELPAPI bool		intersectRayTriangle ( Vec3F orig, Vec3F dir, Vec3F& v0, Vec3F& v1, Vec3F& v2, float& t, float& alpha, float& beta, bool& front );
	HELPAPI Vec3F	projectPointLine(Vec3F p, Vec3F p0, Vec3F p1 );
	HELPAPI Vec3F	projectPointLine(Vec3F p, Vec3F pdir, float& t );
	HELPAPI Vec3F	projectPointPlane(Vec3F p, Vec3F p0, Vec3F pn );
	HELPAPI	float		distancePointPlane(Vec3F p, Vec3F p0, Vec3F pn );
	HELPAPI bool		pointInTriangle(Vec3F pnt, Vec3F& v0, Vec3F& v1, Vec3F& v2, double& u, double& v);
	HELPAPI bool 		checkHit3D(Camera3D* cam, int x, int y, Vec3F target, float radius);
	HELPAPI Vec3F	moveHit3D(Camera3D* cam, int x, int y, Vec3F target, Vec3F plane_norm);
  
#endif

