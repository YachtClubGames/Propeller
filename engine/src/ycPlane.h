#pragma once

/*! \struct ycPlane
Plane representation ax + by + cz = d or in other words, where p is a point on the plane, d = normal.Dot(p)
often you'll find ax + by + cz = d...be careful!
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycPlane;
struct ycAABB;
struct ycGeoRayTriHit;
struct ycLineSeg;
struct ycSphere;
struct ycPlane;
struct ycTri;
struct ycFrustum;
struct ycCircle;
struct ycCapsule;

struct ycPlane : public ycShape //%
{
yceditable:

	//! the data!
	union
	{
		struct { f32 a, b, c; };
		ycVec3 normal; //should be normalized
	};
	f32 d; 

public:

	ycPlane() = default;

	constexpr inline ycPlane( const ycVec3 N, f32 dist );
	inline ycPlane( const ycVec3 & _normal, const ycVec3 & p1 );
	inline ycPlane( const ycVec3& p1, const ycVec3& p2, const ycVec3& p3 );
	
	void FitDistance( const ycVec3& pt );

	//! Calculates the distance to a point from the plane signed 
	inline f32 SignedDistance( const ycVec3 & pt ) const;

	//! Calculates the distance to shape
	inline f32 Distance( const ycVec3 & pt ) const;
	f32 Distance( const ycCircle & circle ) const;
	f32 Distance( const ycSphere & sphere ) const;
	f32 Distance( const ycLineSeg & lineSeg ) const;

	//! Ortho projection onto plane
	inline ycVec3 Project( const ycVec3& pt ) const;

	ycVec3 Mirror( const ycVec3 &pt ) const;

	//! Clip a line against the plane
	bool Clip( const ycLineSeg & lineSeg, ycVec3 * dstEnd ) const;

	ycPlane & Translate( const ycVec3 & offset );
	//! Rotate the plane about the origin
	ycPlane & Rotate( const ycVec3 & origin, const ycQuat & rotation );

	//! Determines if plane is front facing given look
	bool IsFrontFacing(const ycVec3 & lookDir) const;

	//! Whether the plane is finite. Might be false for infinite far planes.
	bool IsFinite() const;

	enum
	{
		kPlaneSide_Front,
		kPlaneSide_Back,
		kPlaneSide_Planar,
		kPlaneSide_Count
	};
	//! Determines if a shape is behind, in front, or within the plane (within the given extents)
	u32				Side( const ycVec3 & pt, const f32 extents = YC_F32_EPSILON ) const;
	u32				Side( const ycSphere & sphere, const f32 extents = YC_F32_EPSILON ) const;
	u32				Side( const ycAABB & box, const f32 extents = YC_F32_EPSILON ) const;
	bool			IsSameSide( const ycVec3 & pt1, const ycVec3 & pt2 ) const;

	//! Calculates the closest point to pt on the plane ( same as what is returned by projection )
	ycVec3			ClosestPoint( const ycVec3 &pt) const;
	ycVec3			ClosestPoint( const ycLineSeg & lineSeg ) const;

	//! Calculates the distance to a point from the plane
	f32				Dist( const ycVec3 & pt ) const;
	f32				Dist( const ycLineSeg & lineSeg ) const;
	f32				Dist( const ycSphere & sphere ) const;
	f32				Dist( const ycAABB& aabb );
	f32				Dist( const ycCapsule & capsule ) const;

	//! Detect if point is inside plane
	bool			Contains( const ycVec3 & pt ) const;

	//! Detect if point is inside plane
	bool			Intersects( const ycLineSeg &lineSeg ) const;
	bool			Intersects( const ycRay& ray, f32 maxT ) const;
	bool			Intersects( const ycPlane &other ) const;
	u32				Intersects( const ycPlane &other1, const ycPlane &other2 ) const;
	bool			Intersects( const ycAABB & box ) const;
	bool			Intersects( const ycSphere & sphere ) const;
	bool			Intersects( const ycTri & tri ) const;
	bool			Intersects( const ycCapsule & capsule ) const;
	u32				Intersects( const ycCircle &circle ) const;

	u32				Intersection( const ycCircle &circle, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr ) const;
	bool			Intersection( f32* hitT, const ycLineSeg &lineSeg ) const;
	bool			Intersection( f32* hitT, const ycRay& ray, f32 maxT ) const;
	bool			Intersection( const ycPlane &other, ycLine * intersection ) const;
	bool			Intersection( const ycPlane &other1, const ycPlane &other2, ycVec3 * intersectionPt ) const;
	
	//! Operators
	inline bool			operator==(	const ycPlane & other ) const;
	inline bool			operator!=(	const ycPlane & other ) const;

	constexpr inline static ycPlane ZERO()  { return ycPlane( ycVec3::ZERO(), 0.0f ); }
};

constexpr inline ycPlane::ycPlane( const ycVec3 N, f32 dist ) 
	: normal( N )
	, d( dist )
{
}

inline ycPlane::ycPlane( const ycVec3 & _normal, const ycVec3 & p1 )
{
	normal = _normal;
	d = p1.Dot( normal );
}

inline ycPlane::ycPlane( const ycVec3& p1, const ycVec3& p2, const ycVec3& p3 )
{
	normal = ( ( p2 - p1 ).Cross( p3 - p1 ) ).Normalized();
	d = normal.Dot( p1 );
}

inline f32 ycPlane::SignedDistance( const ycVec3 & pt ) const 
{ 
	return pt.Dot( normal ) - d; 
}

inline f32 ycPlane::Distance( const ycVec3 & pt ) const
{
	return ycAbs( SignedDistance( pt ) );
}

inline ycVec3 ycPlane::Project( const ycVec3& pt ) const 
{
	return pt - normal*SignedDistance( pt );
}

inline bool ycPlane::operator==( const ycPlane & other ) const
{
	return normal == other.normal && d == other.d;
}

inline bool ycPlane::operator!=( const ycPlane & other ) const
{
	return normal != other.normal || d != other.d;
}