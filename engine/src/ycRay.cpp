#include "ycRay.h"

#include "ycMtx4.h"
#include "ycGeo.h"

//! Get a point along the ray
ycVec3 ycRay::GetPoint( f32 fraction ) const
{
	return pos + dir * fraction;
}

ycVec3 ycRay::ClosestPoint( const ycVec3& pt ) const
{
	ycVec3 closest;
	ycGeo::ClosestPointRay( *this, pt, &closest );
	return closest;
}

void ycRay::ClosestPointsRayRay( const ycRay& b, ycVec3* ptOnMe, ycVec3* ptOnB ) const
{
	return ycGeo::ClosestPointsRayRay( *this, b, ptOnMe, ptOnB );
}

bool ycRay::ClosestPointsRayLine( const ycLine& line, ycVec3* ptOnMe, ycVec3* ptOnLine ) const
{
	return ycGeo::ClosestPointsRayLine( *this, line, ptOnMe, ptOnLine );
}

ycVec2 ycRay::ClosestPoints( const ycRay & other ) const
{
	return ycGeo::ClosestPointsRayRay( *this, other );
}

f32 ycRay::DistSq( const ycLine& line ) const
{	
	return ycGeo::DistSqRayLine( *this, line );
}

f32 ycRay::Dist( const ycLine& line ) const
{	
	return ycGeo::DistRayLine( *this, line );
}

f32 ycRay::DistSq( const ycRay& ray ) const
{	
	return ycGeo::DistSqRayRay( *this, ray );
}

f32 ycRay::DistRay( const ycRay& ray ) const
{	
	return ycGeo::DistRayRay( *this, ray );
}

f32 ycRay::DistSq( const ycVec3& pt ) const
{	
	return ycGeo::DistSqRayPoint( *this, pt );
}

f32 ycRay::Dist( const ycVec3& pt ) const
{	
	return ycGeo::DistRayPoint( *this, pt );
}

u32 ycRay::Intersects( const ycCircle & circle ) const
{
	return ycGeo::IntersectsCircleRay( circle, *this );
}

bool ycRay::Intersects( const ycSphere& sphere ) const
{
	return ycGeo::IntersectsRaySphere( *this, sphere );
}

bool ycRay::Intersection( f32* hitT, f32 maxT, const ycAABB& aabb ) const
{
	return ycGeo::IntersectionRayAABB( hitT, *this, maxT, aabb );
}

bool ycRay::Intersection( f32* hitT, f32* hitT2, f32 maxT, const ycSphere& sphere ) const
{
	return ycGeo::IntersectionRaySphere( hitT, hitT2, *this, maxT, sphere );
}

bool ycRay::Intersection( f32* hitT, f32 maxT, const ycPlane& plane ) const
{
	return ycGeo::IntersectionRayPlane( hitT, *this, maxT, plane );
}

bool ycRay::Intersection( ycGeoRayTriHit* hit, f32 maxT, const ycTri& tri ) const
{
	return ycGeo::IntersectionRayTri( hit, *this, maxT, tri );
}

bool ycRay::Intersection( ycVec3* pt_col, const ycLine& line ) const
{
	return ycGeo::IntersectionLineRay( pt_col, line, *this );
}

u32 ycRay::Intersection( const ycCircle & circle, ycVec3 * intersection1/* = nullptr*/, ycVec3 * intersection2 /*= nullptr*/ ) const
{
	return ycGeo::IntersectionCircleRay( circle, *this, intersection1, intersection2 );
}

bool ycRay::IntersectionRect( const ycVec3& ctr, const ycVec3 &ext1, const ycVec3 &ext2, f32* dist, ycVec2* local ) const
{
	return ycGeo::IntersectionRayRect( *this, ctr, ext1, ext2, dist, local );
}

bool ycRay::IntersectionCuboid( const ycMtx4& cuboid, f32* dist, ycVec3* normal ) const
{
	return ycGeo::IntersectionRayCuboid( *this, cuboid, dist, normal );
}

bool ycRay::IntersectionPlane( const ycVec3 center, const ycVec3 normal, ycVec3* point, f32* dist ) const
{
	return ycGeo::IntersectionRayPlane( *this, center, normal, point, dist );
}

bool ycRay::IntersectionDisc( const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point, f32* dist ) const
{
	return ycGeo::IntersectionRayDisc( *this, center, normal, rInner, rOuter, point, dist );
}

bool ycRay::IntersectionTorus( const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point, f32* dist ) const
{
	return ycGeo::IntersectionRayTorus( *this, center, normal, rInner, rOuter, point, dist );
}

bool ycRay::IntersectionCone( const ycVec3& bottom, const ycVec3& tip, f32 radiusBottom, f32 radiusTip, f32* dist ) const
{
	return ycGeo::IntersectionRayCone( *this, bottom, tip, radiusBottom, radiusTip, dist );
}

bool ycRay::IntersectionLineXZ( const ycVec3 & A, const ycVec3 & B, ycVec2* dist ) const
{
	return ycGeo::IntersectionRayLineXZ( *this, A, B, dist );
}

bool ycRay::Sweep( f32 * fraction, ycVec3 * normal, const ycAABB & b, const ycLineSeg & sweepB ) const
{
	return ycGeo::SweepRayAABB( fraction, normal, *this, b, sweepB );
}
