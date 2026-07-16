#pragma once

/*! \struct ycLineSeg
Represents 3d line segment between two points. Stored as the first point and and offset to the second point
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycGeoRayTriHit;
struct ycCircle;
struct ycCapsule;

struct ycLineSeg : public ycShape //%
{
yceditable:

	union
	{
		struct { ycVec3 pos; ycVec3 offset; };
		struct { ycVec3 start; ycVec3 dir; };
	};

public:

	//! default ctor
	ycLineSeg() = default;

	constexpr explicit inline ycLineSeg( const ycVec3& start );
	explicit inline ycLineSeg( const ycVec3& start, const ycVec3& end );

	static inline ycLineSeg FromStartOffset( const ycVec3& start, const ycVec3& offset );

	ycVec3 GetOffset() const;
	ycVec3 GetStart() const;
	ycVec3 GetEnd() const;
	ycVec3 GetPoint( f32 lerp ) const;

	//! normalized direction
	ycVec3 GetDir() const;

	//! lerps between two vectors
	inline ycVec3 Lerp( const f32 t ) const;

	//! lerps between two vectors and clamps t value
	inline ycVec3 LerpC( const f32 t, const f32 min = 0.0f, const f32 max = 1.0f ) const;
	
	//! Detect if point is on line
	bool Contains( const ycVec3& point ) const;

	//! Find closest point to shape
	ycVec3 ClosestPoint( const ycVec3 & point ) const;
	f32 ClosestPointValue( const ycVec3 & point ) const;
	ycVec3 ClosestPoint( const ycTri& tri, ycVec3 * closestOnLine = nullptr ) const;
	ycVec3 ClosestPoint( const ycLineSeg & lineSegA, ycVec3 * potOnLineSegA, f32 * distSegA = nullptr, f32 * distMySeg = nullptr ) const;
	
	//! Determine the distance from the line seg
	f32 DistSq( const ycVec3& pt ) const;
	f32 Dist( const ycVec3& pt ) const;
	f32 DistSq( const ycLineSeg& lineSeg ) const;
	f32 Dist( const ycLineSeg& lineSeg ) const;
	f32 DistSq( const ycSphere& sphere ) const;
	f32 Dist( const ycSphere& sphere ) const;
	f32 Dist( const ycPlane& plane ) const;
	f32	DistSq( const ycCapsule& capsule ) const;
	f32	Dist( const ycCapsule& capsule ) const;

	//! Detect if intersection with shape
	bool Intersects( const ycAABB& aabb ) const;
	bool Intersects( const ycTri& tri ) const;
	bool Intersects( const ycLineSeg & lineSeg ) const;
	bool IntersectsXY( const ycLineSeg & lineSeg ) const;
	bool Intersects( const ycCircle & circle ) const;
	bool Intersects( const ycCapsule & circle ) const;

	//! Find point of intersection
	u32 Intersection( const ycCircle & circle, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr ) const;
	bool Intersection( ycGeoRayTriHit* hit, const ycTri& tri ) const;
	bool IntersectionXY( const ycLineSeg&, ycVec3* pt_col, f32 * fraction = nullptr ) const;
	
	//! Sweep Shape
	bool Sweep( f32 * fraction, ycVec3 * normal, const ycAABB & b, const ycLineSeg & sweepB ) const;
	bool Sweep( f32 * fraction, ycVec3 * normal, const ycTri & b, const ycLineSeg & sweepB ) const;
	bool SweepXY( f32 * fraction, ycVec3 * normal, const ycLineSeg & a ) const;
	
	inline ycLineSeg operator+( const ycLineSeg& rhs ) const;
	inline bool operator==( const ycLineSeg& other ) const;
	inline bool operator!=( const ycLineSeg& other ) const;

	constexpr inline static ycLineSeg ZERO()  { return ycLineSeg( ycVec3::ZERO() ); }
};

constexpr inline ycLineSeg::ycLineSeg( const ycVec3& _start ) 
	: start( _start ), dir( ycVec3::ZERO() )
{
}

inline ycLineSeg::ycLineSeg( const ycVec3& _start, const ycVec3& end ) 
	: start( _start ), dir( end - _start ) 
{
}

/*static*/ ycLineSeg ycLineSeg::FromStartOffset( const ycVec3& _start, const ycVec3& _offset )
{
	ycLineSeg seg;
	seg.start = _start;
	seg.offset = _offset;
	return seg;
}

inline ycVec3 ycLineSeg::GetOffset() const 
{ 
	return offset; 
}

inline ycVec3 ycLineSeg::GetStart() const 
{ 
	return start; 
}

inline ycVec3 ycLineSeg::GetEnd() const 
{ 
	return start + offset; 
}

inline ycVec3 ycLineSeg::GetPoint( f32 lerp ) const 
{ 
	return start + offset * lerp; 
}

inline ycVec3 ycLineSeg::GetDir() const
{
	return offset.Normalized();
}

inline ycVec3 ycLineSeg::Lerp( const f32 t ) const
{
	return GetPoint( t );
}

inline ycVec3 ycLineSeg::LerpC( const f32 t, const f32 min/* = 0.0f*/, const f32 max/* = 1.0f*/ ) const
{
	f32 tc = ycClamp( t, min, max );
	return GetPoint( tc );
}

ycLineSeg ycLineSeg::operator+( const ycLineSeg& rhs ) const
{
	ycLineSeg seg;
	seg.start = start + rhs.start;
	seg.offset = offset + rhs.offset;
	return seg;
}

bool ycLineSeg::operator==( const ycLineSeg& other ) const
{
	return start == other.start && offset == other.offset;
}

bool ycLineSeg::operator!=( const ycLineSeg& other ) const
{
	return start != other.start || offset != other.offset;
}
