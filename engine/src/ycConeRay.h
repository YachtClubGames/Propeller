#pragma once

/*! \struct ycConeRay
ycConeRay is nearly identical to ycRay but has a radius that grows with distance
from pos to allow for even offset for intersection in a perspective view. It also
has a base radius to allow the same for orthographic views.
 */

//#include "ycShape.h"
#include "ycVec3.h"
#include "ycRay.h"

struct ycConeRay
{
public:

	union
	{
		ycVec3 pos;
		ycVec3 start;
	};
	ycVec3 dir; 
	f32 base; 
	f32 slope;

	ycConeRay() = default;
	constexpr inline ycConeRay( const ycVec3 pos, const ycVec3 dir, f32 base, f32 slope );

	//! Returns true if intersecting, closest is a point on the ring
	//fixme: these aren't using the shape classes and probably should be!
	bool IntersectionRing( const ycVec3 center, const ycVec3 normal, f32 radius, ycVec3 *closest, f32* dist ) const;
	bool IntersectionPoint( const ycVec3 pos, f32* dist = nullptr ) const;
	bool IntersectionLine( const ycVec3 start, const ycVec3 end, f32* dist = nullptr ) const;

	//! Operators
	inline bool operator==(	const ycRay & other ) const;
	inline bool operator!=(	const ycRay & other ) const;

	constexpr inline static ycConeRay ZERO()  { return ycConeRay( ycVec3::ZERO(), ycVec3::ZERO(), 0.0f, 0.0f ); }
};

constexpr inline ycConeRay::ycConeRay( const ycVec3 _pos, const ycVec3 _dir, f32 _base, f32 _slope )
	: pos( _pos )
	, dir( _dir )
	, base( _base )
	, slope( _slope )
{
}
