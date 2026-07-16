#include "ycBox.h"

#include "ycAABB.h"
#include "ycGeo.h"
#include "ycMtx4.h"
#include "ycStd.h"

ycBox::ycBox( const ycAABB& other )
	: ycBox( other.GetMin(), other.GetMax() )
{
}

ycVec3 ycBox::GetNormal( const ycVec3 & pt ) const
{
	ycVec3 center = GetCenter();
	ycVec3 extents = GetExtents();
	ycVec3 point = pt - center;
	f32 distance;
	f32 best = YC_F32_MAX;
	ycVec3 normal;

	distance = extents.x - ycAbs( point.x );
	if( distance < best )
	{
		best = distance;
		normal = ycVec3( point.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f );
	}
	else
	{
		normal = ycVec3::ZERO(); // uninitialized warning 
	}

	distance = extents.y - ycAbs( point.y );
	if( distance < best )
	{
		best = distance;
		normal = ycVec3( 0.0f, point.y >= 0.0f ? 1.0f : -1.0f, 0.0f );
	}

	distance = extents.z - ycAbs( point.z );
	if( distance < best )
	{
		best = distance;
		normal = ycVec3( 0.0f, 0.0f, point.z >= 0.0f ? 1.0f : -1.0f );
	}
	return normal;
}

ycBox ycBox::Overlap( const ycBox & other ) const
{
	ycVec3 otherMin = other.GetMin();
	ycVec3 otherMax = other.GetMax();
	ycVec3 vNewMin = ycVec3::Max( min, otherMin ); 
	ycVec3 vNewMax = ycVec3::Min( max, otherMax ); 
	return ycBox( vNewMin, vNewMax );
}

bool ycBox::Intersects( const ycBox & other ) const
{
	return ycGeo::IntersectsBoxBox( *this, other );
}

void ycBox::AxisProjection( const ycVec3 &dir, f32 * _min, f32 * _max ) const
{
	ycVec3 extents = GetExtents();
	f32 r = extents.x * ycAbs( dir.x ) + extents.y * ycAbs( dir.y ) + extents.z * ycAbs( dir.z );
	f32 s = dir.Dot( GetCenter() );
	*_min = s - r;
	*_max = s + r;
}

//! Check that the extents are valid
bool ycBox::IsValid() const
{
	ycVec3 extents = GetExtents();
	if( extents.x < 0.0f || extents.y < 0.0f || extents.z < 0.0f )
	{
		return false;
	}
	return true;
}

ycBox ycBox::Transformed( const ycMtx4& mtx ) const
{
	const ycVec3 boxMin = GetMin();
	const ycVec3 boxMax = GetMax();

	ycVec3 newBoxMin, newBoxMax;
	newBoxMin = newBoxMax = mtx.t.xyz();

	for( ureg j = 0; j < 3; ++j )
	{
		for( ureg i = 0; i < 3; ++i )
		{
			const f32 a = mtx.m[ i ][ j ] * boxMin.a[ i ];
			const f32 b = mtx.m[ i ][ j ] * boxMax.a[ i ];
			if( a < b )
			{
				newBoxMin.a[ j ] += a;
				newBoxMax.a[ j ] += b;
			}
			else
			{
				newBoxMin.a[ j ] += b;
				newBoxMax.a[ j ] += a;
			}
		}
	}

	return ycBox( newBoxMin, newBoxMax );
}

ycPlane ycBox::GetPlane( const ureg plane ) const
{
	// negative on D here because this is a shortcut to bypass dot product in position passing constuctor
	switch( plane )
	{
	case kSide_MinX: return ycPlane( ycVec3::NEGXAXIS(), -min.x );
	case kSide_MaxX: return ycPlane( ycVec3::XAXIS(),     max.x );
	case kSide_MinY: return ycPlane( ycVec3::NEGYAXIS(), -min.y);
	case kSide_MaxY: return ycPlane( ycVec3::YAXIS(),     max.y );
	case kSide_MinZ: return ycPlane( ycVec3::NEGZAXIS(), -min.z );
	case kSide_MaxZ: return ycPlane( ycVec3::ZAXIS(),     max.z );
	YC_NO_DEFAULT_ASSERT
	}
}

void ycBox::GetPlanes( ycPlane* planes ) const
{
	for( ureg i = 0; i != kSide_Count; ++i )
	{
		planes[ i ] = GetPlane( i );
	}
}

float ycBox::GetValue( Side side ) const
{
	switch ( side ) 
	{
	case kSide_MinX: return min.x;
	case kSide_MaxX: return max.x;
	case kSide_MinY: return min.y;
	case kSide_MaxY: return max.y;
	case kSide_MinZ: return min.z;
	case kSide_MaxZ: return max.z;
	YC_NO_DEFAULT_ASSERT
	}
}

void ycBox::SetValue( Side side, f32 value )
{

	switch ( side ) {
	case kSide_MinX: min.x = value; break;
	case kSide_MaxX: max.x = value; break;
	case kSide_MinY: min.y = value; break;
	case kSide_MaxY: max.y = value; break;
	case kSide_MinZ: min.z = value; break;
	case kSide_MaxZ: max.z = value; break;
	YC_NO_DEFAULT_ASSERT
	}
}

ycVec3 ycBox::GetLocalVertex( const ycVec3 & dir, f32 margin ) const
{	
	ycVec3 vertExtents = GetExtents() + ycVec3( margin );
	return ycVec3(
		(dir.x >= 0.0f ? vertExtents.x : -vertExtents.x), 
		(dir.y >= 0.0f ? vertExtents.y : -vertExtents.y), 
		(dir.z >= 0.0f ? vertExtents.z : -vertExtents.z)
	);
}
