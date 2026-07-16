#pragma once

/*! \struct ycCapsule
capsule defined by 2 points and a radius. points are the centers of the semispheres. top/bottom are the ends of the cylinder portion
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycAABB;
struct ycCircle;
struct ycLineSeg;
struct ycPlane;
struct ycSphere;
struct ycTri;

struct ycCapsule : public ycShape //%
{
yceditable:

	union
	{
		struct
		{
			ycVec3 top;
			ycVec3 bottom;
		};
		struct
		{
			ycVec3 center[2];
		};
	};
	f32 radius;

public:

	ycCapsule() = default;

	constexpr inline ycCapsule( const ycVec3& center1, const ycVec3& center2, f32 radius );

	//! Set the Center
	inline ycCapsule& SetCenters( const ycVec3 & _center1, const ycVec3 & _center2 );
	inline ycCapsule& SetRadius( const f32 _radius );

	//! radius offset in up direction from top
	ycVec3 GetEndTop() const;
	//! radius offset in up direction from bottom
	ycVec3 GetEndBottom() const;
	//! center position
	ycVec3 GetCenter() const;
	//! length of the top to bottom
	f32 GetCentersMag() const;
	//! Get the height
	f32 GetHeight() const;
	//! Gets the direction from pt1 to pt2
	ycVec3 GetUp() const;

	//! Calculate the volume
	f32 Volume() const;
	//! Calculate the Surface Area
	f32 SurfaceArea() const;

	//! Gets the circle at the given height of the capsule
	ycCircle CrossSection( const f32 height ) const;

	//! Scale from center
	ycCapsule& Scale( const f32 scale );
	//! Rotate about center
	ycCapsule& Rotate( const ycQuat & rotation );

	//! Calculate the closest point on the shape
	ycVec3 Closest( const ycVec3 & pt ) const;

	//! Determine the distance from the capsule
	f32 DistSq( const ycVec3 & pt ) const;
	f32 Dist( const ycVec3 & pt ) const;
	f32 DistSq( const ycLineSeg & lineSeg ) const;
	f32 Dist( const ycLineSeg & lineSeg ) const;
	f32 DistSq( const ycCapsule & other ) const;
	f32 Dist( const ycCapsule & other ) const;
	f32 DistSq( const ycSphere & sphere ) const;
	f32 Dist( const ycSphere & sphere ) const;
	f32 Dist( const ycPlane & plane ) const;

	//! Detect if shape is inside capsule
	bool Contains( const ycVec3 & pt ) const;

	//! Detect if shape intersects capsule
	bool Intersects( const ycCapsule & other ) const;
	bool Intersects( const ycSphere & sphere ) const;
	bool Intersects( const ycLineSeg & lineSeg ) const;
	bool Intersects( const ycPlane & plane ) const;

	void ToVertCount( u32 * ptsCount, u32* faceCount, const u32 segments = 32 ) const;
	void ToVerts( ycVec3 * pts, u32 * ptsCount, u32 * faces, u32* faceCount, const u32 segments = 32 ) const;

	//! Operators
	inline bool	operator==( const ycCapsule & other ) const;
	inline bool	operator!=( const ycCapsule & other ) const;

	constexpr inline static ycCapsule ZERO()  { return ycCapsule( ycVec3::ZERO(), ycVec3::ZERO(), 0.0f ); }

};

constexpr inline ycCapsule::ycCapsule( const ycVec3& center1, const ycVec3& center2, f32 _radius )
	: top( center1 )
	, bottom( center2 )
	, radius( _radius )
{
}

inline ycCapsule& ycCapsule::SetCenters( const ycVec3 & _center1, const ycVec3 & _center2 )
{
	top = _center1;
	bottom = _center2;
	return *this;
}

inline ycCapsule& ycCapsule::SetRadius( const f32 _radius )
{
	radius = _radius;
	return *this;
}

inline bool	ycCapsule::operator==( const ycCapsule & other ) const
{
	return bottom == other.bottom && top == other.top && radius == other.radius;
}

inline bool	ycCapsule::operator!=( const ycCapsule & other ) const
{
	return bottom != other.bottom || top != other.top || radius != other.radius;
}
