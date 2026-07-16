#include "ycCircle.h"

#include "ycAABB.h"
#include "ycQuat.h"
#include "ycGeo.h"
#include "ycPlane.h"

ycVec3 ycCircle::GetBasisU() const
{
	return normal.Perpendicular();
}

ycVec3 ycCircle::GetBasisV() const
{
	return normal.PerpendicularAlt();
}

ycVec3 ycCircle::GetPoint( const ycVec3 & _normal ) const
{
	ycVec3 newNormal = ycQuat( ycVec3( 0, 1.0f, 0 ), normal ).RotateInverse( _normal );
	return center + newNormal * radius;
}

ycAABB ycCircle::ToAABB() const
{
	ycAABB aabb;
	aabb.center = center;
	ycVec3 _radius = normal * radius;
	aabb.extents.x = ycAbs( _radius.x );
	aabb.extents.y = ycAbs( _radius.y );
	aabb.extents.z = ycAbs( _radius.z );
	return aabb;
}

ycPlane ycCircle::ToPlane() const
{
	return ycPlane( normal, center );
}

u32 ycCircle::ToVerts( ycVec3 * pts, u32 vertCount, bool fill/* = true*/ ) const
{
	s32 sections;
	if( fill )
	{
		sections = (vertCount / 3)-1;
	}
	else
	{
		sections = (vertCount / 2 - 1)-1;
	}
	ycVec3 lastPoint, currentPoint, basisU, basisV;
	u32 index = 0;
	f32 t;
	lastPoint = currentPoint = center;
	basisU = GetBasisU();
	basisV = GetBasisV();
	ycAssert(sections > 0);
	for( s32 i = 0; i <= sections;i++) 
	{
		t = i * YC_TWO_PI / sections;
		lastPoint = currentPoint;
		currentPoint = center + (basisU * ycCos(t) + basisV * ycSin(t)) * radius;

		if( fill )
		{
			pts[index] = center;
			index++;
		}
		else if( i == 0 )
		{
			lastPoint = currentPoint;
		}
		pts[index] = lastPoint;
		index++;
		pts[index] = currentPoint;
		index++;
	}
	return index;
}

ycVec3 ycCircle::ClosestPoint( const ycVec3 & pt ) const
{
	ycVec3 closest;
	ycGeo::ClosestPointCircle( *this, pt, &closest );
	return closest;
}

f32 ycCircle::Distance( const ycVec3 & pt ) const
{
	return ycGeo::DistCirclePoint( *this, pt );
}

f32 ycCircle::Distance( const ycPlane & plane ) const
{
	return ycGeo::DistPlaneCircle( plane, *this );
}

bool ycCircle::Contains( const ycVec3 & pt ) const
{
	return ycGeo::ContainsCirclePt( *this, pt );
}

u32	ycCircle::Intersects( const ycPlane & plane ) const
{
	return ycGeo::IntersectsCirclePlane( *this, plane );
}

u32 ycCircle::Intersects( const ycCircle & other ) const
{
	return ycGeo::IntersectsCircleCircle( *this, other );
}

u32 ycCircle::Intersects( const ycLineSeg & lineSeg ) const
{
	return ycGeo::IntersectsCircleLineSeg( *this, lineSeg );
}

u32 ycCircle::Intersects( const ycRay& ray ) const
{
	return ycGeo::IntersectsCircleRay( *this, ray );
}

bool ycCircle::Intersects( const ycSphere & sphere ) const
{
	return ycGeo::IntersectsCircleSphere( *this, sphere );
}

u32	ycCircle::Intersection( const ycPlane & plane, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2 /*= nullptr*/ ) const
{
	return ycGeo::IntersectionCirclePlane( *this, plane, intersection1, intersection2 );
}

u32 ycCircle::Intersection( const ycLineSeg & lineSeg, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2 /*= nullptr*/ ) const
{
	return ycGeo::IntersectionCircleLineSeg( *this, lineSeg, intersection1, intersection2 );
}

u32 ycCircle::Intersection( const ycRay& ray, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2 /*= nullptr*/ ) const
{
	return ycGeo::IntersectionCircleRay( *this, ray, intersection1, intersection2 );
}
