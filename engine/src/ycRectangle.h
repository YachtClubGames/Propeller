#pragma once

/*! \struct 2d rectangle in 3d space. z positon of extents is unused
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycAABB;

struct ycRectangle : public ycShape //%
{
yceditable:
	//! the data!
	union
	{
		struct
		{
			ycVec3 center;
			ycVec3 normal;
			ycVec3 extents;
		};
	};

public:

	ycRectangle() = default;

	constexpr inline ycRectangle( const ycVec3& center, const ycVec3& extents, const ycVec3& normal = ycVec3::ZAXIS() );

	//! Operators
	inline bool	operator==( const ycRectangle & other ) const;
	inline bool	operator!=( const ycRectangle & other ) const;

	constexpr inline static ycRectangle ZERO()  { return ycRectangle( ycVec3::ZERO(), ycVec3::ZERO(), ycVec3::ZERO() ); }

};

constexpr inline ycRectangle::ycRectangle( const ycVec3& _center, const ycVec3& _extents, const ycVec3& _normal )
	: center( _center )
	, normal( _normal )
	, extents( _extents )
{
}

inline bool	ycRectangle::operator==( const ycRectangle & other ) const
{
	return center == other.center && normal == other.normal && extents == other.extents;
}

inline bool	ycRectangle::operator!=( const ycRectangle & other ) const
{
	return center != other.center || normal != other.normal || extents != other.extents;
}
