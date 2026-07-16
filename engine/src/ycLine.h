#pragma once

/*! \struct ycLine
 *  a ycLine represents a geometry where ycLine.origin is on the line, and
 *  the line extends to (+) and (-) infinity in line.direction.w
 */

#include "ycShape.h"
#include "ycVec3.h"

struct ycLine : public ycShape //%
{
yceditable:

	union
	{
		struct { ycVec3 origin; ycVec3 slope; };
		struct { ycVec3 start; ycVec3 dir; };
	};

public:

	ycLine() = default;
	constexpr inline ycLine( const ycVec3& origin, const ycVec3& slope );

	inline ycLine& SetOrigin( const ycVec3& _origin );
	inline ycLine& SetSlope( const ycVec3& _slope );

	inline ycVec3 GetOrigin() const;
	inline ycVec3 GetSlope() const;

	//! Determines the closets point inside the line to the point
	void ClosestPoint( const ycVec3& pt, ycVec3* ptOnLine );
	void ClosestPoints( const ycLine& b, ycVec3* ptOnMe, ycVec3* ptOnB );
	bool ClosestPoints( const ycRay& ray, ycVec3* ptOnMe, ycVec3* ptOnRay );
		
	//! Determine the distance from the line to the shape
	f32 DistSq( const ycVec3& pt );
	f32 Dist( const ycVec3& pt );
	f32 DistSq( const ycLine& line );
	f32 Dist( const ycLine& line );
	f32 DistSq( const ycRay& ray );
	f32 Dist( const ycRay& ray );
	
	//! Detect intersections with shapes
	bool Intersection( ycVec3* pt_col, const ycPlane& plane );
	bool Intersection( ycVec3* pt_col, const ycAABB& aabb);
	bool Intersection( ycVec3* pt_col, const ycTri& tri );
	bool Intersection( ycVec3* pt_col, const ycSphere& sphere );
	bool Intersection( ycVec3* pt_col, const ycLine& line );
	bool Intersection( ycVec3* pt_col, const ycRay& ray );

	//! Operators
	inline bool operator==(	const ycLine & other ) const;
	inline bool operator!=(	const ycLine & other ) const;

	constexpr inline static ycLine ZERO() { return ycLine( ycVec3::ZERO(), ycVec3::ZERO() ); }
};

constexpr inline ycLine::ycLine( const ycVec3& _origin, const ycVec3& _slope ) 
	: origin( _origin )
	, slope( _slope )
{
}

inline ycVec3 ycLine::GetOrigin() const
{
	return origin;
}

inline ycVec3 ycLine::GetSlope() const
{
	return slope;
}

inline ycLine& ycLine::SetOrigin( const ycVec3& _origin )
{
	origin = _origin;
	return *this;
}

inline ycLine& ycLine::SetSlope( const ycVec3& _slope )
{
	slope = _slope;
	return *this;
}

inline bool	ycLine::operator==( const ycLine & other ) const
{
	return start == other.start && dir == other.dir;
}

inline bool	ycLine::operator!=( const ycLine & other ) const
{
	return start != other.start || dir != other.dir;
}
