#pragma once

/*! ycRay was included to differentiate itself from line and segment
in this way the client doesn't have to care as much about context
when working with Rays, Lines, and Segments

The ray starts at the ray.origin, and extends to infinity in the +direction
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycMtx4;
struct ycCircle;
struct ycGeoRayTriHit;

struct ycRay : public ycShape //%
{
yceditable:

	union
	{
		ycVec3 pos;
		ycVec3 start;
	};
	ycVec3 dir; 

public:

	ycRay() = default;
	constexpr inline ycRay( const ycVec3& pos, const ycVec3& dir );

	//! Get a point along the ray
	ycVec3 GetPoint( f32 fraction ) const;

	//! lerps between two vectors
	inline ycVec3 Lerp( const f32 t ) const;

	//! lerps between two vectors and clamps t value
	inline ycVec3 LerpC( const f32 t, const f32 min = 0.0f, const f32 max = 1.0f ) const;

	//! Get the closest point along the two rays. x is scalar along this and y is scalar along other. returns 0 if parallel or either is 0 direction.
	ycVec3 ClosestPoint( const ycVec3& pt ) const;
	ycVec2 ClosestPoints( const ycRay &other ) const;
	void ClosestPointsRayRay( const ycRay& b, ycVec3* ptOnMe, ycVec3* ptOnB ) const;
	bool ClosestPointsRayLine( const ycLine& line, ycVec3* ptOnMe, ycVec3* ptOnLine ) const;

	f32 Dot( const ycVec3 target ) const { return dir.Dot( target - pos ); }

	//! Gets the distance to the shape
	f32 DistSq( const ycLine& line ) const;
	f32 Dist( const ycLine& line ) const;
	f32 DistSq( const ycRay& ray ) const;
	f32 DistRay( const ycRay& ray ) const;
	f32 DistSq( const ycVec3& pt ) const;
	f32 Dist( const ycVec3& pt ) const;

	//! Detect if intersection with shape
	u32  Intersects( const ycCircle & circle ) const;
	bool Intersects( const ycSphere& sphere ) const;

	//! Detect if intersection with shape and find point
	bool Intersection( f32* hitT, f32 maxT, const ycAABB& ) const;
	bool Intersection( f32* hitT, f32* hitT2, f32 maxT, const ycSphere& ) const;
	bool Intersection( f32* hitT, f32 maxT, const ycPlane& ) const;
	bool Intersection( ycGeoRayTriHit* hit, f32 maxT, const ycTri& ) const;
	bool Intersection( ycVec3* pt_col, const ycLine& ) const;
	u32 Intersection( const ycCircle & circle, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr ) const;
		//fixme: these aren't using the shape classes and probably should be!
	bool IntersectionRect( const ycVec3& ctr, const ycVec3& ext1, const ycVec3& ext2, f32* dist = nullptr, ycVec2* local = nullptr ) const; //! rectangle
	bool IntersectionCuboid( const ycMtx4& cuboid, f32* dist = nullptr, ycVec3* normal = nullptr ) const; //! cube
	bool IntersectionPlane( const ycVec3 center, const ycVec3 normal, ycVec3* point = nullptr, f32* dist = nullptr ) const; //! plane
	bool IntersectionDisc( const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point = nullptr, f32* dist = nullptr ) const; //! disc
	//fixme: this is a disc right now
	bool IntersectionTorus( const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point = nullptr, f32* dist = nullptr ) const; //! disc
	bool IntersectionCone( const ycVec3& bottom, const ycVec3& tip, f32 radiusBottom, f32 radiusTip, f32* dist = nullptr ) const; //! Truncated cone intersection
	bool IntersectionLineXZ( const ycVec3& A, const ycVec3& B, ycVec2* dist = nullptr ) const; //! Intersection of two lines over X and Z, Y ignored. Return dist->x for position along ray, dist->y position along segment

	//! Sweep Shape
	bool Sweep( f32 * fraction, ycVec3 * normal, const ycAABB & b, const ycLineSeg & sweepB ) const;

	//! Operators
	inline bool operator==(	const ycRay & other ) const;
	inline bool operator!=(	const ycRay & other ) const;

	constexpr inline static ycRay ZERO()  { return ycRay( ycVec3::ZERO(), ycVec3::ZERO() ); }
};

constexpr inline ycRay::ycRay( const ycVec3& _pos, const ycVec3& _dir ) 
	: pos( _pos )
	, dir( _dir )
{
}

inline ycVec3 ycRay::Lerp( const f32 t ) const
{
	return GetPoint( t );
}

inline ycVec3 ycRay::LerpC( const f32 t, const f32 min/* = 0.0f*/, const f32 max/* = 1.0f*/ ) const
{
	f32 tc = ycClamp( t, min, max );
	return GetPoint( tc );
}

inline bool	ycRay::operator==( const ycRay & other ) const
{
	return start == other.start && dir == other.dir;
}

inline bool	ycRay::operator!=( const ycRay & other ) const
{
	return start != other.start || dir != other.dir;
}
