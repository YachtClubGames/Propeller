#pragma once

/*! \class ycBox
Axis-aligned bounding box stored by min and max 
*/

#include "ycShape.h"
#include "ycPlane.h"
#include "ycVec3.h"

struct ycAABB;
struct ycFrustum;
struct ycLineSeg;
struct ycMtx4;
struct ycPlane;
struct ycSphere;
struct ycTri;

struct ycBox : public ycShape //%
{
yceditable:

	ycVec3 min;
	ycVec3 max;

public:

	//! default ctor
	ycBox() = default;

	//! sets min max of aabb
	explicit constexpr inline ycBox( const ycVec3& _min, const ycVec3& _max );

	//! sets aabb by center and extents
	explicit constexpr inline ycBox( const ycVec3 & center, const f32 & extentsX, const f32 & extentsY, const f32 & extentsZ = 0.0f );

	//! sets from ycAABB
	explicit ycBox( const ycAABB& other );

	//! Set bounding box data
	inline void SetMin( const ycVec3& min );
	inline void SetMax( const ycVec3& max );
	inline void SetCenter( const ycVec3& center );
	inline void SetExtents( const ycVec3& extents );

	//! sets aabb to be invalid with mostly valid values so the first expand will properly take on the first box's values
	inline void SetInvalid();

	//! Get bounding box data
	inline ycVec3 GetMin() const;
	inline ycVec3 GetMax() const;
	inline ycVec3 GetCenter() const;
	inline ycVec3 GetExtents() const;
	//! Size is double the extents
	inline ycVec3 GetSize() const;
	//! Calculate the volume
	inline f32 GetVolume() const;
	//! Calculate surface area
	inline f32 GetSurfaceArea() const;
	
	//! Grows the box to encompass the position
	inline void Expand( const ycVec3& pos );
	//! Grows the box to encompass the box
	inline void Expand( const ycBox& box );

	//! Grows the box a specified amount
	inline void Inflate( f32 extra );

	//! Gets the surface normal from a point
	ycVec3 GetNormal( const ycVec3 & pt ) const;

	//! Area that rects overlap
	ycBox Overlap( const ycBox & other) const;

	//!	Detects if shape touches any part of this box
	bool Intersects( const ycBox & other ) const;

	//! Calculate axis projection
	void AxisProjection( const ycVec3 &dir, f32 * min, f32 * max ) const;

	//! Check that the extents are valid
	bool IsValid() const;

	ycBox Transformed( const ycMtx4& mtx ) const;

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

	ycVec3 GetLocalVertex( const ycVec3 & dir, f32 margin ) const;

	//! Operators
	inline bool operator==( const ycBox & other ) const;
	inline bool operator!=( const ycBox & other ) const;

	inline static constexpr ycBox INVALID() { return ycBox( ycVec3::ZERO(), -YC_F32_MAX, -YC_F32_MAX, -YC_F32_MAX ); }
	inline static constexpr ycBox EVERYTHING() { return ycBox( ycVec3( -YC_F32_MAX ), ycVec3( YC_F32_MAX ) ); }
	inline static constexpr ycBox NOTHING() { return ycBox( ycVec3( YC_F32_MAX ), ycVec3( -YC_F32_MAX ) ); }

	inline static constexpr ycBox ZERO()  { return ycBox( ycVec3::ZERO(), 0.0f, 0.0f, 0.0f ); }
};

constexpr inline ycBox::ycBox( const ycVec3& _min, const ycVec3& _max ) 
	: min( _min )
	, max( _max )
{
}

constexpr inline ycBox::ycBox( const ycVec3 & center, const f32 & extentsX, const f32 & extentsY, const f32 & extentsZ )
	: min( ycVec3( center.x - extentsX, center.y - extentsY, center.z - extentsZ ) )
	, max( ycVec3( center.x + extentsX, center.y + extentsY, center.z + extentsZ ) )
{
}

inline void ycBox::SetInvalid()
{ 
	min = ycVec3( YC_F32_MAX, YC_F32_MAX, YC_F32_MAX );
	max = ycVec3( -YC_F32_MAX, -YC_F32_MAX, -YC_F32_MAX );
}

inline void ycBox::SetMin( const ycVec3& _min )
{
	min = _min;
}

inline void ycBox::SetMax( const ycVec3& _max )
{
	max = _max;
}

inline void ycBox::SetCenter( const ycVec3& _center )
{
	ycVec3 extents = GetExtents( );
	min.x = _center.x - extents.x;
	min.y = _center.y - extents.y;
	min.z = _center.z - extents.z;
	max.x = _center.x + extents.x;
	max.y = _center.y + extents.y;
	max.z = _center.z + extents.z;
}

inline void ycBox::SetExtents( const ycVec3& extents )
{
	ycVec3 center = GetCenter( );
	min.x = center.x - extents.x;
	min.y = center.y - extents.y;
	min.z = center.z - extents.z;
	max.x = center.x + extents.x;
	max.y = center.y + extents.y;
	max.z = center.z + extents.z;
}

inline ycVec3 ycBox::GetMin() const
{
	return min;
}

inline ycVec3 ycBox::GetMax() const
{
	return max;
}

inline ycVec3 ycBox::GetCenter() const
{
 return ( min + max ) * 0.5f;
}

inline ycVec3 ycBox::GetExtents() const
{
	return ( max - min ) * 0.5f;
}

inline ycVec3 ycBox::GetSize() const
{
	return GetExtents()*2.0f;
}

f32	ycBox::GetVolume() const
{
	return (GetExtents() * 2.0f).Mag();
}

f32 ycBox::GetSurfaceArea() const
{
	if( !IsValid() ) { return 0; }
	ycVec3 size = GetSize();
	return 2.0f * ( size.x * size.y + size.x * size.z + size.y * size.z );
}

inline void ycBox::Expand( const ycVec3& pos )
{ 
	min = ycVec3::Min( min, pos );
	max = ycVec3::Max( max, pos );
}

inline void ycBox::Expand( const ycBox& box ) 
{ 
	min = ycVec3::Min( min, box.GetMin() );
	max = ycVec3::Max( max, box.GetMax() );
}

inline void ycBox::Inflate( f32 extra )
{
	min -= ycVec3( extra );
	max += ycVec3( extra );
}

inline bool ycBox::operator==( const ycBox & other ) const
{
	return min == other.min && max == other.max;
}

inline bool ycBox::operator!=( const ycBox & other ) const
{
	return min != other.min || max != other.max;
}
