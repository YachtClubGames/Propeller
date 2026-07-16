#include "ycConeRay.h"

#include "ycGeo.h"

bool ycConeRay::IntersectionRing( const ycVec3 center, const ycVec3 normal, f32 radius, ycVec3* closest, f32* dist ) const
{
	return ycGeo::IntersectionConeRayRing( *this, center, normal, radius, closest, dist );
}

bool ycConeRay::IntersectionPoint( const ycVec3 point, f32* dist ) const
{
	return ycGeo::IntersectionConeRayPoint( *this, point, dist );
}

bool ycConeRay::IntersectionLine( const ycVec3 _start, const ycVec3 end, f32 * dist ) const
{
	return ycGeo::IntersectionConeRayLine( *this, _start, end, dist);
}
