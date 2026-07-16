#include "ycLineSeg.h"

#include "ycGeo.h"

bool ycLineSeg::Contains( const ycVec3& pt ) const
{
	return ycGeo::ContainsLineSegPt( *this, pt );
}

ycVec3 ycLineSeg::ClosestPoint( const ycVec3 & point ) const
{
	ycVec3 pt;
	ycGeo::ClosestPointLineSeg( *this, point, &pt );
	return pt;
}

f32 ycLineSeg::ClosestPointValue( const ycVec3 & point ) const
{
	f32 value;
	ycGeo::ClosestPointLineSeg( *this, point, &value );
	return value;
}

ycVec3 ycLineSeg::ClosestPoint( const ycTri& tri, ycVec3 * closestOnLine/* = nullptr*/ ) const
{
	ycVec3 pt;
	ycGeo::ClosestPointTriLineSeg( tri, *this, &pt, closestOnLine );
	return pt;
}

ycVec3 ycLineSeg::ClosestPoint( const ycLineSeg & lineSegA, ycVec3 * pointOnLineSegA, f32 * distSegA /*= nullptr*/, f32 * distMySeg/* = nullptr*/ ) const
{
	ycVec3 pt;
	ycGeo::ClosestPointsLineSegLineSeg( lineSegA, *this, pointOnLineSegA, &pt, distSegA, distMySeg );
	return pt;
}

f32 ycLineSeg::DistSq( const ycVec3& pt ) const
{
	return ycGeo::DistSqLineSegPoint( *this, pt );
}

f32 ycLineSeg::Dist( const ycVec3& pt ) const
{
	return ycGeo::DistLineSegPoint( *this, pt );
}

f32 ycLineSeg::DistSq( const ycLineSeg& lineSeg ) const
{
	return ycGeo::DistSqLineSegLineSeg( *this, lineSeg );
}

f32 ycLineSeg::Dist( const ycLineSeg& lineSeg ) const
{
	 return ycGeo::DistLineSegLineSeg( *this, lineSeg );
}

f32 ycLineSeg::DistSq( const ycSphere& sphere ) const
{
	return ycGeo::DistSqSphereLineSeg( sphere, *this );
}

f32 ycLineSeg::Dist( const ycSphere& sphere ) const
{
	return ycGeo::DistSphereLineSeg( sphere, *this );
}

f32 ycLineSeg::Dist( const ycPlane& plane ) const
{
	return ycGeo::DistPlaneLineSeg( plane, *this );
}

f32	ycLineSeg::DistSq( const ycCapsule& capsule ) const
{
	return ycGeo::DistSqCapsuleLineSeg( capsule, *this );
}

f32	ycLineSeg::Dist( const ycCapsule& capsule ) const
{
	return ycGeo::DistCapsuleLineSeg( capsule, *this );
}
	
bool ycLineSeg::Intersects( const ycAABB& aabb ) const
{
	return ycGeo::IntersectsAABBLineSeg( aabb, *this );
}

bool ycLineSeg::Intersects( const ycTri& tri ) const
{
	return ycGeo::IntersectsTriLineSeg( tri, *this );
}

bool ycLineSeg::IntersectsXY( const ycLineSeg & lineSeg ) const
{
	return ycGeo::IntersectsLineSegLineSegXY( lineSeg, *this );
}

bool ycLineSeg::Intersects( const ycCircle & circle ) const
{
	return !!ycGeo::IntersectsCircleLineSeg( circle, *this );
}

bool ycLineSeg::Intersects( const ycCapsule & capsule ) const
{
	return ycGeo::IntersectsCapsuleLineSeg( capsule, *this );
}

u32 ycLineSeg::Intersection( const ycCircle & circle, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2 /*= nullptr */) const
{
	return ycGeo::IntersectionCircleLineSeg( circle, *this, intersection1, intersection2 );
}

bool ycLineSeg::Intersection( ycGeoRayTriHit* hit, const ycTri& tri ) const
{
	return ycGeo::IntersectionLineSegTri( hit, *this, tri );
}

bool ycLineSeg::IntersectionXY( const ycLineSeg& lineSeg, ycVec3* pt_col, f32 * fraction /*= nullptr*/ ) const
{
	return ycGeo::IntersectionLineSegLineSegXY( pt_col, fraction, *this, lineSeg );
}
	
bool ycLineSeg::Sweep( f32 * fraction, ycVec3 * normal, const ycAABB & b, const ycLineSeg & sweepB ) const
{
	return ycGeo::SweepLineSegAABB( fraction, normal, *this, b, sweepB );
}

bool ycLineSeg::Sweep( f32 * fraction, ycVec3 * normal, const ycTri & b, const ycLineSeg & sweepB ) const
{
	return ycGeo::SweepLineSegTri( fraction, normal, *this, b, sweepB );
}

bool ycLineSeg::SweepXY( f32 * fraction, ycVec3 * normal, const ycLineSeg & a ) const
{
	return ycGeo::SweepLineSegLineSegXY( fraction, normal, *this, a );
}
