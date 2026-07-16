#pragma once

/*! \struct ycSphere
Sphere defined by center point and radius

*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycAABB;
struct ycCapsule;
struct ycCircle;
struct ycFrustum;
struct ycGeoRayTriHit;
struct ycLineSeg;
struct ycPlane;
struct ycPlane;
struct ycSphere;
struct ycTri;

struct ycSphere : public ycShape //%
{
yceditable:

	ycVec3 center;
	f32	   radius;

public:

	ycSphere() = default;
	constexpr inline ycSphere( ycVec3 _center, f32 _radius );

	inline ycSphere&	SetCenter(const ycVec3 & _center);
	inline ycSphere&	SetRadius(const f32 & _radius);
	inline ycVec3		GetCenter() const;
	inline f32			GetRadius() const;

	//! Calculate the volume
	inline f32		Volume() const;
	//! Calculate the Surface Area
	inline f32		SurfaceArea() const;
	//! Calculates the normal for any point outside or on the sphere
	inline ycVec3	Normal( const ycVec3 & pt ) const;

	ycAABB			ToAABB() const;
	ycVec3			GetLocalVertex( const ycVec3 & dir, f32 margin = 0.0f ) const;

	//! Calculate the closest point on the sphere to the point
	ycVec3			ClosestPoint( const ycVec3 & pt ) const;
	ycVec3			ClosestPoint( const ycSphere & sphere ) const;

	//! Determine the distance to shape
	f32				DistSq( const ycVec3 & pt ) const;
	f32				Dist( const ycVec3 & pt ) const;
	f32				DistSq( const ycLineSeg & lineSeg ) const;
	f32				Dist( const ycLineSeg & lineSeg ) const;
	f32				DistSq( const ycSphere & other ) const;
	f32				Dist( const ycSphere & other ) const;
	f32				DistSq( const ycCapsule & capsule ) const;
	f32				Dist( const ycCapsule & capsule ) const;
	f32				DistSq( const ycAABB & box ) const;
	f32				Dist( const ycAABB & box ) const;
	f32				Dist( const ycPlane & plane ) const;
	f32				DistSq( const ycTri & tri ) const;
	f32				Dist( const ycTri & tri ) const;
	
	//! Detect if point is inside sphere
	bool			Contains( const ycVec3 & pt ) const;

	//! Detects if sphere and shape overlap
	bool			Intersect( const ycSphere & other ) const;
	u32				Intersects( const ycLineSeg & seg ) const;
	bool			Intersects( const ycRay & ray ) const;
	bool			Intersects( const ycPlane & plane ) const;
	bool			Intersects( const ycAABB &aabb ) const;
	bool			Intersects( const ycTri & tri ) const;
	bool			Intersects( const ycFrustum & frustum ) const;
	bool			Intersects( const ycCircle & other ) const;
	bool			Intersects( const ycCapsule & capsule ) const;

	bool			Intersection( f32* hit, const ycLineSeg & seg ) const;
	bool			Intersection( f32* hit, const ycRay & ray, f32 maxT ) const;

	bool			Sweep( ycGeoRayTriHit* hit, const ycVec3& mySweep, const ycTri& tri ) const;

	void ToVertCount( u32 * ptsCount, u32* faceCount, u32 subdivide = 0 ) const;
	void ToVerts( ycVec3 * pts, u32 * ptsCount, u32 * faces, u32* faceCount, u32 subdivide = 0 ) const;

	//! Operators
	inline bool		operator==(	const ycSphere & other ) const;
	inline bool		operator!=(	const ycSphere & other ) const;

	constexpr inline static ycSphere ZERO()  { return ycSphere( ycVec3::ZERO(), 0.0f ); }
};

constexpr inline ycSphere::ycSphere( ycVec3 _center, f32 _radius ) 
	: center( _center )
	, radius( _radius )
{
}

inline ycSphere& ycSphere::SetCenter( const ycVec3 & _center )
{
	center = _center;
	return *this;
}

inline ycSphere& ycSphere::SetRadius( const f32 & _radius )
{
	radius = _radius;
	return *this;
}

inline ycVec3 ycSphere::GetCenter() const
{
	return center;
}

inline f32 ycSphere::GetRadius() const
{
	return radius;
}

inline f32 ycSphere::Volume() const
{
	return (4.0f * YC_PI * radius * radius * radius) / 3.0f;
}

inline f32 ycSphere::SurfaceArea() const
{
	return 4.0f * YC_PI * radius * radius;
}

inline ycVec3 ycSphere::Normal( const ycVec3 & pt ) const
{
	return (pt - center).Normalized();
}

inline bool ycSphere::operator==( const ycSphere & other ) const
{
	return center == other.center && radius == other.radius;
}

inline bool ycSphere::operator!=( const ycSphere & other ) const
{
	return center != other.center || radius != other.radius;
}
