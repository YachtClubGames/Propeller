#pragma once

/*! \struct ycCircle
A Circle in 3d space
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycAABB;
struct ycPlane;

struct ycCircle : public ycShape //%
{
yceditable:

	struct
	{
		ycVec3 center;
		ycVec3 normal;
		f32 radius;
	};

public:

	ycCircle() = default;

	constexpr inline ycCircle( const ycVec3& center, const f32& radius, const ycVec3 & _normal = ycVec3::ZAXIS() );

	//! Get the direction vector to the 'U direction' of the circle
	ycVec3 GetBasisU() const;

	//! Get the direction vector to the 'V direction' of the circle
	ycVec3 GetBasisV() const;
	ycVec3 GetPoint( const ycVec3 & normal ) const;

	ycAABB ToAABB() const;
	ycPlane ToPlane() const;

	//! Creates the tris for the current circle with max vertCount vertices
	u32 ToVerts( ycVec3 * pts, u32 vertCount, bool fill = true ) const;

	//! Determines the closets point inside the circle to the point
	ycVec3 ClosestPoint( const ycVec3 & pt ) const;

	//! Determine the distance from the circle to the shape
	f32 Distance( const ycVec3 & pt ) const;
	f32 Distance( const ycPlane & plane ) const;

	bool Contains( const ycVec3 & pt ) const;

	//! Detect intersections with shapes
	//warning:		These use circle's plane to do collision and are currently one sided based on normal
	u32	 Intersects( const ycPlane & plane ) const;
	u32  Intersects( const ycCircle & other ) const;
	u32  Intersects( const ycLineSeg & lineSeg ) const;
	u32  Intersects( const ycRay& ray ) const;
	bool Intersects( const ycSphere & sphere ) const;

	u32	Intersection( const ycPlane & plane, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr ) const;
	u32 Intersection( const ycLineSeg & lineSeg, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr ) const;
	u32 Intersection( const ycRay& ray, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr ) const;

	//! Operators
	inline bool	operator==( const ycCircle & other ) const;
	inline bool	operator!=( const ycCircle & other ) const;

	constexpr inline static ycCircle ZERO()  { return ycCircle( ycVec3::ZERO(), 0.0f, ycVec3::ZERO() ); }

};

constexpr inline ycCircle::ycCircle( const ycVec3& _center, const f32& _radius, const ycVec3& _normal )
	: center( _center )
	, normal( _normal )
	, radius( _radius )
{
}

inline bool	ycCircle::operator==( const ycCircle & other ) const
{
	return center == other.center && normal == other.normal && radius == other.radius;
}

inline bool	ycCircle::operator!=( const ycCircle & other ) const
{
	return center != other.center || normal != other.normal || radius != other.radius;
}
