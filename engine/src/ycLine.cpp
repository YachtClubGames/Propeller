#include "ycLine.h"

#include "ycGeo.h"

void ycLine::ClosestPoint( const ycVec3& pt, ycVec3* ptOnLine )
{
	return ycGeo::ClosestPointLine( *this, pt, ptOnLine );
}

void ycLine::ClosestPoints( const ycLine& b, ycVec3* ptOnMe, ycVec3* ptOnB )
{
	return ycGeo::ClosestPointsLineLine( *this, b, ptOnMe, ptOnB );
}

bool ycLine::ClosestPoints( const ycRay& ray, ycVec3* ptOnMe, ycVec3* ptOnRay )
{
	return ycGeo::ClosestPointsLineRay( *this, ray, ptOnMe, ptOnRay );
}

f32 ycLine::DistSq( const ycVec3& pt )
{
	return ycGeo::DistSqLinePoint( *this, pt );
}

f32 ycLine::Dist( const ycVec3& pt )
{
	return ycGeo::DistLinePoint( *this, pt );
}

f32 ycLine::DistSq( const ycLine& line )
{
	return ycGeo::DistSqLineLine( *this, line );
}

f32 ycLine::Dist( const ycLine& line )
{
	return ycGeo::DistLineLine( *this, line );
}

f32 ycLine::DistSq( const ycRay& ray )
{
	return ycGeo::DistSqLineRay( *this, ray );
}

f32 ycLine::Dist( const ycRay& ray )
{
	return ycGeo::DistLineRay( *this, ray );
}

bool ycLine::Intersection( ycVec3* pt_col, const ycPlane& plane )
{
	return ycGeo::IntersectionLinePlane( pt_col, *this, plane );
}

bool ycLine::Intersection( ycVec3* pt_col, const ycAABB& aabb)
{
	return ycGeo::IntersectionLineAABB( pt_col, *this, aabb );
}

bool ycLine::Intersection( ycVec3* pt_col, const ycTri& tri )
{
	return ycGeo::IntersectionLineTri( pt_col, *this, tri );
}

bool ycLine::Intersection( ycVec3* pt_col, const ycSphere& sphere )
{
	return ycGeo::IntersectionLineSphere( pt_col, *this, sphere );
}

bool ycLine::Intersection( ycVec3* pt_col, const ycLine& line )
{
	return ycGeo::IntersectionLineLine( pt_col, *this, line );
}

bool ycLine::Intersection( ycVec3* pt_col, const ycRay& ray )
{
	return ycGeo::IntersectionLineRay( pt_col, *this, ray );
}
