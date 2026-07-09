#include "ycPlane.h"
#include "ycGeo.h"
#include "ycLineSeg.h"
#include "ycSphere.h"
#include "ycAABB.h"
#include "ycQuat.h"

ycVec3 ycPlane::Mirror( const ycVec3 &pt ) const
{
	return pt - normal * ( 2.0f * ( pt.Dot( normal ) - d ) );
}

ycPlane& ycPlane::Translate( const ycVec3 & offset )
{
	d += normal.Dot( offset );
	return *this;
}

ycPlane& ycPlane::Rotate( const ycVec3 & origin, const ycQuat & rotation )
{
	d -= origin.Dot( normal );
	normal = rotation.Rotate( normal );
	d += origin.Dot( normal );
	return *this;
}

bool ycPlane::Clip( const ycLineSeg & lineSeg, ycVec3 * dstEnd ) const
{
	f32 hit;
	bool intersect = ycGeo::IntersectionLineSegPlane( &hit, lineSeg, *this );
	if( intersect )
	{
		*dstEnd = lineSeg.Lerp( hit );
		return true;
	}
	*dstEnd = lineSeg.GetEnd();
	return false;
}

void ycPlane::FitDistance( const ycVec3& pt )
{
	d = pt.Dot( normal );
}

bool ycPlane::IsFrontFacing(const ycVec3 & lookDir) const
{
	return normal.Dot( lookDir ) <= 0.0f;
}

bool ycPlane::IsFinite() const
{
	return !ycIsInf( d );
}

u32	ycPlane::Side( const ycVec3 & pt, const f32 extents/* = YC_F32_EPSILON*/ ) const
{
	f32 dist = SignedDistance(pt);
	if( dist < -extents )
	{
		return kPlaneSide_Back;
	}
	else if( dist > extents )
	{
		return kPlaneSide_Front;
	}
	return kPlaneSide_Planar;
}

u32	ycPlane::Side( const ycSphere & sphere, const f32 extents/* = YC_F32_EPSILON*/ ) const
{
	f32 dist = SignedDistance( sphere.center );
	if( dist < (-sphere.radius - extents) )
	{
		return kPlaneSide_Back;
	}
	else if( dist > (sphere.radius + extents) )
	{
		return kPlaneSide_Front;
	}
	else
	{
		return kPlaneSide_Planar;
	}
}

u32	ycPlane::Side( const ycAABB & box, const f32 extents /*= YC_F32_EPSILON*/ ) const
{
	f32 dist1, dist2;
	dist1 = SignedDistance( box.center );
	dist2 = ycAbs( box.extents.x + normal.x ) + ycAbs( box.extents.y + normal.y ) + ycAbs( box.extents.z + normal.z );

	if( (dist1 - dist2) > extents )
	{
		return kPlaneSide_Front;
	}
	else if( (dist1 + dist2) < -extents )
	{
		return kPlaneSide_Back;
	}
	return kPlaneSide_Planar;
}

bool ycPlane::IsSameSide( const ycVec3 & pt1, const ycVec3 & pt2 ) const
{
	return SignedDistance( pt1 ) * SignedDistance( pt2 ) >= 0.0f;
}

f32 ycPlane::Distance( const ycCircle & circle ) const
{
	return ycGeo::DistPlaneCircle( *this, circle );
}

f32 ycPlane::Distance( const ycSphere & sphere ) const
{
	return ycGeo::DistPlaneSphere( *this, sphere );
}

f32 ycPlane::Distance( const ycLineSeg & lineSeg ) const
{
	return ycGeo::DistPlaneLineSeg( *this, lineSeg );
}

ycVec3 ycPlane::ClosestPoint( const ycVec3 &pt ) const
{
	ycVec3 point;
	ycGeo::ClosestPointPlane( *this, pt, &point );
	return point;
}

ycVec3 ycPlane::ClosestPoint( const ycLineSeg & lineSeg ) const
{
	ycVec3 point;
	ycGeo::ClosestPointPlaneLineSeg( *this, lineSeg, &point );
	return point;
}

f32 ycPlane::Dist( const ycVec3 & pt ) const
{
	return ycGeo::DistPlanePoint( *this, pt );
}

f32 ycPlane::Dist( const ycLineSeg & lineSeg ) const
{
	return ycGeo::DistPlaneLineSeg( *this, lineSeg );
}

f32 ycPlane::Dist( const ycSphere & sphere ) const
{
	return ycGeo::DistPlaneSphere( *this, sphere );
}

f32 ycPlane::Dist( const ycAABB& aabb )
{
	return ycGeo::DistAABBPlane( aabb, *this );
}

f32 ycPlane::Dist( const ycCapsule & capsule ) const
{
	return ycGeo::DistCapsulePlane( capsule, *this );
}

bool ycPlane::Contains( const ycVec3 & pt ) const
{
	return ycGeo::ContainsPlanePt( *this, pt );
}

bool ycPlane::Intersects( const ycLineSeg &lineSeg ) const
{
	return ycGeo::IntersectionLineSegPlane( nullptr, lineSeg, *this );
}

bool ycPlane::Intersects( const ycRay& ray, f32 maxT ) const
{
	return ycGeo::IntersectionRayPlane( nullptr, ray, maxT, *this );
}

bool ycPlane::Intersects( const ycPlane &other ) const
{
	return ycGeo::IntersectsPlanePlane( *this, other );
}

u32	 ycPlane::Intersects( const ycPlane &other1, const ycPlane &other2 ) const
{
	ycVec3 ptCt;
	return ycGeo::IntersectionPlanePlanePlane( &ptCt, *this, other1, other2 );
}

bool ycPlane::Intersects( const ycAABB & box ) const
{
	return ycGeo::IntersectsPlaneAABB( *this, box );
}

bool ycPlane::Intersects( const ycSphere & sphere ) const
{
	return ycGeo::IntersectsPlaneSphere( *this, sphere );
}

bool ycPlane::Intersects( const ycTri & tri ) const
{
	return ycGeo::IntersectsPlaneTri( *this, tri );
}

bool ycPlane::Intersects( const ycCapsule & capsule ) const
{
	return ycGeo::IntersectsCapsulePlane( capsule, *this );
}

u32 ycPlane::Intersects( const ycCircle &circle ) const
{
	return ycGeo::IntersectsCirclePlane( circle, *this );
}

u32 ycPlane::Intersection( const ycCircle &circle, ycVec3 * intersection1/* = nullptr*/, ycVec3 * intersection2/* = nullptr*/ ) const
{
	return ycGeo::IntersectionCirclePlane( circle, *this, intersection1, intersection2 );
}

bool ycPlane::Intersection( f32* hitT, const ycLineSeg &lineSeg ) const
{
	return ycGeo::IntersectionLineSegPlane( hitT, lineSeg, *this );
}

bool ycPlane::Intersection( f32* hitT, const ycRay& ray, f32 maxT ) const
{
	return ycGeo::IntersectionRayPlane( hitT, ray, maxT, *this );
}

bool ycPlane::Intersection( const ycPlane &other, ycLine * intersection ) const
{
	return ycGeo::IntersectionPlanePlane( intersection, *this, other);
}

bool ycPlane::Intersection( const ycPlane &other1, const ycPlane &other2, ycVec3 * intersectionPt ) const
{
	return ycGeo::IntersectionPlanePlanePlane( intersectionPt, *this, other1, other2 );
}