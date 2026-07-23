#pragma once

/*! \struct ycCylinder
An elliptical cylinder. 
center is the base of the ellipse
--assuming it's direct up like a can of soda
normal is the direction towards the top
radius.x is the width of the cylinder 
radius.y is the half the height of the cylinder
radius.z is the depth of the cylinder
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycAABB;

struct ycCylinder : public ycShape //%
{
yceditable:
	//! the data!
	union
	{
		struct
		{
			ycVec3 center;
			ycVec3 normal;
			ycVec3 radius;
		};
	};
	
public:

	ycCylinder() = default;

	constexpr inline ycCylinder( const ycVec3& center, const ycVec3& normal, const ycVec3& radius );

	ycVec3 GetPoint( const ycVec3 & normal ) const;

	ycAABB ToAABB() const;

	//! Return the number of verts based on the segments (always 3 * (segments * 2), if there is a top cap add (segments-2)*3 and same for the bottom
	void ToVertCount( u32 * ptsCount, u32* faceCount, const u32 segments = 32, bool capCenterVert = false, bool capTop = true, bool capBot = true, bool centerFaces = true ) const;
	void ToVerts( ycVec3 * pts, u32 * ptsCount, u32 * faces, u32* faceCount, const u32 segments = 32, bool capCenterVert = false, bool capTop = true, bool capBot = true, f32 radiusPerc = 1.0f, bool centerFaces = true ) const;

	//! Operators
	inline bool	operator==( const ycCylinder & other ) const;
	inline bool	operator!=( const ycCylinder & other ) const;

	constexpr inline static ycCylinder ZERO()  { return ycCylinder( ycVec3::ZERO(), ycVec3::ZERO(), ycVec3::ZERO() ); }

};

constexpr inline ycCylinder::ycCylinder( const ycVec3& _center, const ycVec3& _normal, const ycVec3& _radius )
	: center( _center )
	, normal( _normal )
	, radius( _radius )
{
}

inline bool	ycCylinder::operator==( const ycCylinder & other ) const
{
	return center == other.center && normal == other.normal && radius == other.radius;
}

inline bool	ycCylinder::operator!=( const ycCylinder & other ) const
{
	return center != other.center || normal != other.normal || radius != other.radius;
}
