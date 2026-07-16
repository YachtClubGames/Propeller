#pragma once

/*! \struct ycAABB
Axis-aligned bounding box stored by center and extents (half width, half height, half depth)
*/

#include "ycShape.h"
#include "ycVec3.h"
#include "ycFloatRange.h"

struct ycMtx4;
struct ycLineSeg;
struct ycSphere;
struct ycPlane;
struct ycTri;
struct ycFrustum;
struct ycBox;
class ycRand;

struct ycAABB : public ycShape //%
{
yceditable:

	ycVec3 center;
	ycVec3 extents;

public:

	static const f32 InvalidExtent;

	//! default ctor
	ycAABB() = default;

	//! sets min max of aabb
	inline ycAABB( const ycVec3& _min, const ycVec3& _max );

	//! sets aabb by center and extents
	constexpr static inline ycAABB FromCenterExtents( const ycVec3& center, const ycVec3& extents );

	//! sets aabb by center and extents
	constexpr inline ycAABB( const ycVec3& center, const f32 extentsX, const f32 extentsY, const f32 extentsZ = 0.0f );

	//! sets aabb by three ranges
	constexpr inline ycAABB( const ycFloatRange& rangeX, const ycFloatRange& rangeY, const ycFloatRange& rangeZ );

	//! sets from ycBox
	explicit ycAABB( const ycBox& other );

	//! Set bounding box data
	inline void SetMin( const ycVec3& min );
	inline void SetMax( const ycVec3& max );
	inline void SetCenter( const ycVec3& center );
	inline void SetExtents( const ycVec3& extents );
	inline void SetRangeX( const ycFloatRange & rangeX );
	inline void SetRangeY( const ycFloatRange & rangeX );
	inline void SetRangeZ( const ycFloatRange & rangeX );

	//! sets aabb to be invalid (where extents are negative). this is the quickest way to invalidate for IsValid() to return false
	inline void SetInvalidFast();
	//! sets aabb to be invalid with mostly valid values so the first expand will properly take on the first box's values
	inline void SetInvalid();

	//! Get bounding box data
	inline ycVec3 GetMin() const;
	inline ycVec3 GetMax() const;
	inline ycVec3 GetCenter() const;
	inline ycVec3 GetExtents() const;
	//! Size is double the extents
	inline ycVec3 GetSize() const;
	inline ycFloatRange GetRangeX() const;
	inline ycFloatRange GetRangeY() const;
	inline ycFloatRange GetRangeZ() const;
	inline f32 GetAspectRatioXY() const;
	//! Calculate the volume
	inline f32	  Volume() const;
	//! Calculate surface area
	inline f32    GetSurfaceArea() const;
	
	//! Grows the box to encompass the position
	inline void Expand( const ycVec3& pos );
	inline ycAABB Expanded( const ycVec3& pos ) const;
	//! Grows the box to encompass the box
	inline void Expand( const ycAABB& box );
	inline ycAABB Expanded( const ycAABB& box ) const;

	//! Grows the box by adding an extra amount to each side.
	inline void Inflate( f32 extra );
	inline ycAABB Inflated( f32 extra ) const;
	inline void Inflate( const ycVec3 extra );
	inline ycAABB Inflated( const ycVec3 extra ) const;

	//! Rotate the box about its center
	ycAABB &		RotateExtents( const ycQuat & _rotation );
	//! Rotate the box center and extents
	ycAABB &		Rotate( const ycQuat & _rotation );
	//! Scale the box about its center
	ycAABB &		ScaleExtents( const f32 scale );

	//! Gets the surface normal from a point
	ycVec3			GetNormal( const ycVec3 & pt ) const;

	//! Area that rects overlap
	ycAABB			Overlap( const ycAABB & other) const;

	//! Calculate axis projection
	void			AxisProjection( const ycVec3 &dir, f32 * min, f32 * max ) const;

	//! Determines the closets point inside the box to the point
	ycVec3			ClosestPoint( const ycVec3 & pt ) const;

	//! Get a random point within the box
	ycVec3			RandomPoint( ycRand* rng ) const;

	//! Determine distance from point
	f32				DistSq( const ycVec3 &pt ) const;
	f32				Dist( const ycVec3 &pt ) const;
	//! Determine distance from sphere
	f32				DistSq( const ycSphere &sphere ) const;
	f32				Dist( const ycSphere &sphere ) const;
	//! Determine distance from plane
	f32				Dist( const ycPlane & plane ) const;

	//! Detect if point is inside box
	bool			Contains( const ycVec3 & pt ) const;
	//!	Detects if other box is completely contained in this box
	bool			Contains( const ycAABB & other ) const;
	
	//!	Detects if shape touches any part of this box
	bool			Intersects( const ycAABB & other ) const;
	bool			Intersects( const ycLineSeg & other ) const;
	bool			Intersects( const ycSphere & other ) const;
	bool			Intersects( const ycPlane & other ) const;
	bool			Intersects( const ycTri & other ) const;
	bool			Intersects( const ycFrustum & other ) const;
	bool			Intersects( const ycRay & other, f32 maxT ) const;

	bool			Intersection( f32* hit, const ycRay & ray, f32 maxT ) const;

	//!	Detects if sweep motion of box intersects other shape
	bool			Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycAABB & other, const ycLineSeg & otherSweep );
	bool			Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycLineSeg & other, const ycLineSeg & otherSweep );
	bool			Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycTri & other, const ycLineSeg & otherSweep );

	//! Check that the extents are valid
	bool			IsValid() const;

	//! check the bounds are equal within a range of a given epsilon
	bool Equals( const ycAABB& other, f32 epsilon ) const;

	ycAABB Transformed( const ycMtx4& mtx ) const;
	
	//! planes point inward

	enum Side
	{
		kSide_MinX = 0,
		kSide_MaxX,
		kSide_MinY,
		kSide_MaxY,
		kSide_MinZ,
		kSide_MaxZ,

		kSide_Count
	};
	ycPlane GetPlane( const ureg plane ) const;
	void GetPlanes( ycPlane* planes ) const;

	f32 GetValue( Side side ) const;
	void SetValue( Side side, f32 value );

	//! get 8 corners of the box
	ycVec3 GetCorner( u32 corner ) const;
	ycVec3 GetCornerExtent( u32 corner ) const;
	//! vertices for tris of box
	u32 ToVerts( ycVec3 * pts36 ) const;
	//! vertices for tris of transformed by mtx
	u32 ToVerts( ycVec3 * pts36, const ycMtx4 & mtx ) const;

	ycVec3 GetLocalVertex( const ycVec3 & dir, f32 margin ) const;
	ycVec3 GetFacePoint( const ycVec3 & dir ) const;

	//! calculate aabb of a large number of points
	/*! warning: these functions use SIMD vec4s and read 4 bytes past the final point for an unused 'w'
	*/
	static ycAABB FromPoints( const ycVec3* points, const ycSize_t numPoints );
	static ycAABB FromPoints( const ycVec3* points, const ycSize_t numPoints, const ycSize_t stride ); //!< stride=sizeof(ycVec3) is the same as the no-stride version

	//! Operators
	inline bool			operator==( const ycAABB & other ) const;
	inline bool			operator!=( const ycAABB & other ) const;

	inline static ycAABB INVALID() { return ycAABB( ycVec3::ZERO(), InvalidExtent, InvalidExtent, InvalidExtent ); }

	constexpr inline static ycAABB ZERO()  { return ycAABB( ycVec3::ZERO(), 0.0f, 0.0f, 0.0f ); }

};

inline ycAABB::ycAABB( const ycVec3& _min, const ycVec3& _max ) 
{
	extents = ( (_max - _min) * 0.5f );
	center = ( _min + extents );
}

constexpr inline ycAABB ycAABB::FromCenterExtents( const ycVec3& center, const ycVec3& extents )
{
	return ycAABB( center, extents.x, extents.y, extents.z );
}

constexpr inline ycAABB::ycAABB( const ycVec3& _center, const f32 extentsX, const f32 extentsY, const f32 extentsZ )
	: center( _center )
	, extents( extentsX, extentsY, extentsZ )
{
}

constexpr inline ycAABB::ycAABB( const ycFloatRange& rangeX, const ycFloatRange& rangeY, const ycFloatRange& rangeZ )
	: center( ycVec3{ rangeX.GetCenter(), rangeY.GetCenter(), rangeZ.GetCenter() })
	, extents( ycVec3{ rangeX.GetRange() * 0.5f, rangeY.GetRange() * 0.5f, rangeZ.GetRange() * 0.5f })
{
}

inline void ycAABB::SetInvalidFast()
{ 
	extents.x = -1.0f;
}

inline void ycAABB::SetInvalid()
{ 
	center = ycVec3::ZERO();
	extents = ycVec3( InvalidExtent, InvalidExtent, InvalidExtent );
}

inline void ycAABB::SetMin( const ycVec3& min )
{
	ycVec3 max = GetMax();
	extents = (max - min) / 2.0f;
	center = min + extents;
}

inline void ycAABB::SetMax( const ycVec3& max )
{
	ycVec3 min = GetMin();
	extents = (max - min) / 2.0f;
	center = min + extents;
}

inline void ycAABB::SetCenter( const ycVec3& _center )
{
	center = _center;
}

inline void ycAABB::SetExtents( const ycVec3& _extents )
{
	extents = _extents;
}

inline void ycAABB::SetRangeX( const ycFloatRange & range )
{
	center.x = range.GetCenter();
	extents.x = range.GetRange() * 0.5f;
}

inline void ycAABB::SetRangeY( const ycFloatRange & range )
{
	center.y = range.GetCenter();
	extents.y = range.GetRange() * 0.5f;
}

inline void ycAABB::SetRangeZ( const ycFloatRange & range )
{
	center.z = range.GetCenter();
	extents.z = range.GetRange() * 0.5f;
}

inline ycVec3 ycAABB::GetMin() const
{
	return center - extents;
}

inline ycVec3 ycAABB::GetMax() const
{
	return center + extents;
}

inline ycVec3 ycAABB::GetCenter() const
{
	return center;
}

inline ycVec3 ycAABB::GetExtents() const
{
	return extents;
}

inline ycVec3 ycAABB::GetSize() const
{
	return extents*2.0f;
}

inline ycFloatRange ycAABB::GetRangeX() const
{
	return ycFloatRange{ GetMin().x, GetMax().x };
}

inline ycFloatRange ycAABB::GetRangeY() const
{
	return ycFloatRange{ GetMin().y, GetMax().y };
}

inline ycFloatRange ycAABB::GetRangeZ() const
{
	return ycFloatRange{ GetMin().z, GetMax().z };
}

inline f32 ycAABB::GetAspectRatioXY() const
{
	return extents.x / extents.y;
}

f32	ycAABB::Volume() const
{
	return (extents * 2.0f).Mag();
}

f32 ycAABB::GetSurfaceArea() const
{
	ycVec3 size = GetSize();
	return 2.0f * ( size.x * size.y + size.x * size.z + size.y * size.z );
}

inline void ycAABB::Expand( const ycVec3& pos )
{ 
	ycVec3 min = ycVec3::Min( GetMin(), pos );
	ycVec3 max = ycVec3::Max( GetMax(), pos );
	extents = (max - min) * 0.5f;
	center = min + extents;
}

inline ycAABB ycAABB::Expanded( const ycVec3& pos ) const
{
	return ycAABB( ycVec3::Min( GetMin(), pos ), ycVec3::Max( GetMax(), pos ) );
}

inline void ycAABB::Expand( const ycAABB& box ) 
{ 
	ycVec3 min = ycVec3::Min( GetMin(), box.GetMin() );
	ycVec3 max = ycVec3::Max( GetMax(), box.GetMax() );
	extents = (max - min) * 0.5f;
	center = min + extents;	
}

inline ycAABB ycAABB::Expanded( const ycAABB& box ) const
{
	return ycAABB( ycVec3::Min( GetMin(), box.GetMin() ), ycVec3::Max( GetMax(), box.GetMax() ) );
}

inline void ycAABB::Inflate( f32 extra )
{
	extents += ycVec3( extra );
}

inline ycAABB ycAABB::Inflated( f32 extra ) const
{
	ycAABB aabb = *this;
	aabb.Inflate( extra );
	return aabb;
}

inline void ycAABB::Inflate( const ycVec3 extra )
{
	extents += extra;
}

inline ycAABB ycAABB::Inflated( const ycVec3 extra ) const
{
	ycAABB aabb = *this;
	aabb.Inflate( extra );
	return aabb;
}

inline bool ycAABB::operator==( const ycAABB & other ) const
{
	return center == other.center && extents == other.extents;
}

inline bool ycAABB::operator!=( const ycAABB & other ) const
{
	return center != other.center || extents != other.extents;
}