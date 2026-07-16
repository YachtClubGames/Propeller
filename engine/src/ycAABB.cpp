#include "ycAABB.h"

#include "ycMtx4.h"
#include "ycGeo.h"
#include "ycLineSeg.h"
#include "ycSimd.h"
#include "ycQuat.h"
#include "ycBox.h"
#include "ycRand.h"

namespace
{
	void AddFace( ycVec3 *pts32, ycVec3 *pts8, u32 i1, u32 i2, u32 i3 )
	{
		pts32[0] = pts8[i1];
		pts32[1] = pts8[i2];
		pts32[2] = pts8[i3];
	}
}

const f32 ycAABB::InvalidExtent = -YC_F32_MAX / 4.0f;

ycAABB::ycAABB( const ycBox& box )
	: center( box.GetCenter() )
	, extents( box.GetExtents() )
{
}

ycAABB & ycAABB::RotateExtents( const ycQuat & _rotation )
{
	extents = _rotation.Rotate( extents ).Abs();
	return *this;
}

ycAABB & ycAABB::Rotate( const ycQuat & _rotation )
{
	center = _rotation.Rotate( center );
	extents = _rotation.Rotate( extents ).Abs();
	return *this;
}

ycAABB & ycAABB::ScaleExtents( const f32 scale )
{
	extents *= scale;
	return *this;
}

ycVec3 ycAABB::GetNormal( const ycVec3 & pt ) const
{
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

ycAABB ycAABB::Overlap( const ycAABB & other ) const
{
	ycVec3 min = GetMin();
	ycVec3 max = GetMax();
	ycVec3 otherMin = other.GetMin();
	ycVec3 otherMax = other.GetMax();
	ycVec3 vNewMin = ycVec3::Max( min, otherMin ); 
	ycVec3 vNewMax = ycVec3::Min( max, otherMax ); 
	return ycAABB( vNewMin, vNewMax );
}

void ycAABB::AxisProjection( const ycVec3 &dir, f32 * min, f32 * max ) const
{
	f32 r = extents.x * ycAbs( dir.x ) + extents.y * ycAbs( dir.y ) + extents.z * ycAbs( dir.z );
	f32 s = dir.Dot( GetCenter() );
	*min = s - r;
	*max = s + r;
}

ycVec3 ycAABB::ClosestPoint( const ycVec3 & pt ) const
{
	ycVec3 vec;
	ycGeo::ClosestPointAABB( *this, pt, &vec );
	return vec;
}

ycVec3 ycAABB::RandomPoint( ycRand* rng ) const
{
	return center + ycVec3( extents.x * rng->RandF( -1.0f, 1.0f ), extents.y * rng->RandF( -1.0f, 1.0f ), extents.z * rng->RandF( -1.0f, 1.0f ) );
}

f32 ycAABB::DistSq( const ycVec3 &pt ) const
{
	return ycGeo::DistSqAABBPoint( *this, pt );
}

f32 ycAABB::Dist( const ycVec3 &pt ) const
{
	return ycGeo::DistSqAABBPoint( *this, pt );
}

f32 ycAABB::DistSq( const ycSphere &sphere ) const
{
	return ycGeo::DistSqAABBSphere( *this, sphere );
}

f32 ycAABB::Dist( const ycSphere &sphere ) const
{
	return ycGeo::DistAABBSphere( *this, sphere );
}

f32 ycAABB::Dist( const ycPlane & plane ) const
{
	return ycGeo::DistAABBPlane( *this, plane );
}

bool ycAABB::Contains( const ycVec3 & pt ) const 
{
	return ycGeo::ContainsAABBPt( *this, pt );
}

bool ycAABB::Contains( const ycAABB & other ) const
{
	return ycGeo::ContainsAABBAABB( *this, other );
}

bool ycAABB::Intersects( const ycAABB & other ) const
{
	return ycGeo::IntersectsAABBAABB( *this, other );
}

bool ycAABB::Intersects( const ycLineSeg & other ) const
{
	return ycGeo::IntersectsAABBLineSeg( *this, other );
}

bool ycAABB::Intersects( const ycSphere & other ) const
{
	return ycGeo::IntersectsAABBSphere( *this, other );
}

bool ycAABB::Intersects( const ycPlane & other ) const
{
	return ycGeo::IntersectsAABBPlane( *this, other );
}

bool ycAABB::Intersects( const ycTri & other ) const
{
	return ycGeo::IntersectsAABBTri( *this, other );
}

bool ycAABB::Intersects( const ycFrustum & other ) const
{
	return ycGeo::IntersectsAABBFrustum( *this, other );
}

bool ycAABB::Intersects( const ycRay & other, f32 maxT ) const
{
	return ycGeo::IntersectsAABBRay( *this, other, maxT );
}

bool ycAABB::Intersection( f32* hit, const ycRay & ray, f32 maxT ) const
{
	return ycGeo::IntersectionRayAABB( hit, ray, maxT, *this );
}

bool ycAABB::Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycAABB & other, const ycLineSeg & otherSweep )
{
	return ycGeo::SweepAABBAABB( fraction, normal, *this, mySweep, other, otherSweep );
}

bool ycAABB::Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycLineSeg & other, const ycLineSeg & otherSweep )
{
	ycLineSeg sweep = other + otherSweep;
	return ycGeo::SweepLineSegAABB( fraction, normal, sweep, *this, mySweep );
}

bool ycAABB::Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycTri & other, const ycLineSeg & otherSweep )
{
	return ycGeo::SweepAABBTri( fraction, normal, *this, mySweep, other, otherSweep );
}

//! Check that the extents are valid
bool ycAABB::IsValid() const
{
	if( extents.x < 0.0f || extents.y < 0.0f || extents.z < 0.0f )
	{
		return false;
	}
	return true;
}

bool ycAABB::Equals( const ycAABB& other, f32 epsilon ) const
{
	return center.Equals( other.center, epsilon ) && extents.Equals( other.extents, epsilon );
}

ycAABB ycAABB::Transformed( const ycMtx4& mtx ) const
{
	if( !IsValid() )
	{
		ycAABB newBox( *this );
		newBox.center += mtx.t.xyz();
		return newBox;
	}
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

	return ycAABB( newBoxMin, newBoxMax );
}

ycPlane ycAABB::GetPlane( const ureg plane ) const
{
	// negative on D here because this is a shortcut to bypass dot product in position passing constructor
	switch( plane )
	{
	case kSide_MinX: return ycPlane( ycVec3::XAXIS(),    (center.x-extents.x) );
	case kSide_MaxX: return ycPlane( ycVec3::NEGXAXIS(), -(center.x+extents.x) );
	case kSide_MinY: return ycPlane( ycVec3::YAXIS(),    (center.y-extents.y) );
	case kSide_MaxY: return ycPlane( ycVec3::NEGYAXIS(), -(center.y+extents.y) );
	case kSide_MinZ: return ycPlane( ycVec3::ZAXIS(),    (center.z-extents.z) );
	case kSide_MaxZ: return ycPlane( ycVec3::NEGZAXIS(), -(center.z+extents.z) );
	YC_NO_DEFAULT_ASSERT
	}
}

void ycAABB::GetPlanes( ycPlane* planes ) const
{
	for( ureg i = 0; i != kSide_Count; ++i )
	{
		planes[ i ] = GetPlane( i );
	}
}

float ycAABB::GetValue( Side side ) const
{
	switch ( side ) 
	{
	case kSide_MinX: return (center.x-extents.x);
	case kSide_MaxX: return (center.x+extents.x);
	case kSide_MinY: return (center.y-extents.y);
	case kSide_MaxY: return (center.y+extents.y);
	case kSide_MinZ: return (center.z-extents.z);
	case kSide_MaxZ: return (center.z+extents.z);
	YC_NO_DEFAULT_ASSERT
	}
}

void ycAABB::SetValue( Side side, f32 value )
{

	switch ( side ) {
	case kSide_MinX: SetMin( GetMin().SetX( value ) ); break;
	case kSide_MaxX: SetMax( GetMax().SetX( value ) ); break;
	case kSide_MinY: SetMin( GetMin().SetY( value ) ); break;
	case kSide_MaxY: SetMax( GetMax().SetY( value ) ); break;
	case kSide_MinZ: SetMin( GetMin().SetZ( value ) ); break;
	case kSide_MaxZ: SetMax( GetMax().SetZ( value ) ); break;
	YC_NO_DEFAULT_ASSERT
	}
}

ycVec3 ycAABB::GetCorner( u32 corner ) const
{
	switch( corner )
	{
	case 0: return center + ycVec3( -extents.x, -extents.y, -extents.z);
	case 1: return center + ycVec3( extents.x, -extents.y, -extents.z);
	case 2: return center + ycVec3( extents.x, extents.y, -extents.z);
	case 3: return center + ycVec3( -extents.x, extents.y, -extents.z);
	case 4: return center + ycVec3( -extents.x, -extents.y, extents.z);
	case 5: return center + ycVec3( extents.x, -extents.y, extents.z);
	case 6: return center + ycVec3( extents.x, extents.y, extents.z);
	case 7: return center + ycVec3( -extents.x, extents.y, extents.z);
	YC_NO_DEFAULT_ASSERT
	}
}

ycVec3 ycAABB::GetCornerExtent( u32 corner ) const
{
	switch( corner )
	{
	case 0: return ycVec3( -extents.x, -extents.y, -extents.z);
	case 1: return ycVec3( extents.x, -extents.y, -extents.z);
	case 2: return ycVec3( extents.x, extents.y, -extents.z);
	case 3: return ycVec3( -extents.x, extents.y, -extents.z);
	case 4: return ycVec3( -extents.x, -extents.y, extents.z);
	case 5: return ycVec3( extents.x, -extents.y, extents.z);
	case 6: return ycVec3( extents.x, extents.y, extents.z);
	case 7: return ycVec3( -extents.x, extents.y, extents.z);
	YC_NO_DEFAULT_ASSERT
	}
}

u32 ycAABB::ToVerts( ycVec3 * pts36 ) const
{
	ycVec3 pts8[8];
	for( s32 i = 0; i < 8; i++ )
	{
		pts8[i] = GetCorner( i );
	}

	//front
	AddFace( &pts36[0], &pts8[0], 2, 1, 0 );
	AddFace( &pts36[3], &pts8[0], 0, 3, 2 );
	//back
	AddFace( &pts36[6], &pts8[0], 5, 6, 7 );
	AddFace( &pts36[9], &pts8[0], 7, 4, 5 );
	//bottom
	AddFace( &pts36[12], &pts8[0], 0, 1, 5 );
	AddFace( &pts36[15], &pts8[0], 5, 4, 0 );
	//top
	AddFace( &pts36[18], &pts8[0], 6, 2, 3 );
	AddFace( &pts36[21], &pts8[0], 3, 7, 6 );
	//right
	AddFace( &pts36[24], &pts8[0], 6, 5, 1 );
	AddFace( &pts36[27], &pts8[0], 1, 2, 6 );
	//left
	AddFace( &pts36[30], &pts8[0], 0, 4, 7 );
	AddFace( &pts36[33], &pts8[0], 7, 3, 0 );
	return 36;
}

u32 ycAABB::ToVerts( ycVec3 * pts36, const ycMtx4 & mtx ) const
{
	ycVec3 pts8[8];
	for( s32 i = 0; i < 8; i++ )
	{
		pts8[i] = GetCorner( i );
		pts8[i] = mtx.TransformPos( pts8[i] );
	}
	//front
	AddFace( &pts36[0], &pts8[0], 2, 1, 0 );
	AddFace( &pts36[3], &pts8[0], 0, 3, 2 );
	//back
	AddFace( &pts36[6], &pts8[0], 5, 6, 7 );
	AddFace( &pts36[9], &pts8[0], 7, 4, 5 );
	//bottom
	AddFace( &pts36[12], &pts8[0], 0, 1, 5 );
	AddFace( &pts36[15], &pts8[0], 5, 4, 0 );
	//top
	AddFace( &pts36[18], &pts8[0], 6, 2, 3 );
	AddFace( &pts36[21], &pts8[0], 3, 7, 6 );
	//right
	AddFace( &pts36[24], &pts8[0], 6, 5, 1 );
	AddFace( &pts36[27], &pts8[0], 1, 2, 6 );
	//left
	AddFace( &pts36[30], &pts8[0], 0, 4, 7 );
	AddFace( &pts36[33], &pts8[0], 7, 3, 0 );
	return 36;
}

ycVec3 ycAABB::GetLocalVertex( const ycVec3 & dir, f32 margin ) const
{	
	ycVec3 vertExtents = GetExtents() + ycVec3( margin );
	return ycVec3(	(dir.x >= 0.0f ? vertExtents.x : -vertExtents.x), 
					(dir.y >= 0.0f ? vertExtents.y : -vertExtents.y), 
					(dir.z >= 0.0f ? vertExtents.z : -vertExtents.z) );
}

ycVec3 ycAABB::GetFacePoint( const ycVec3 & dir ) const
{	
	ycVec3 vertExtents = GetExtents();
	s32 longestAxis = dir.LongestAxisAbs();
	ycVec3 facePoint;
	facePoint = vertExtents * dir;
	f32 oldLongest = facePoint.a[longestAxis];
	f32 newLongest = dir.a[longestAxis] >= 0.0f ? vertExtents.a[longestAxis] : -vertExtents.a[longestAxis];
	if( oldLongest )
	{
		for( s32 i = 0; i < 3; ++i )
		{
			if( facePoint.a[i] )
			{
				facePoint.a[i] *= (newLongest/oldLongest);
			}
		}
	}
	return facePoint;
}

#include "ycPushDebugOpt.inc"

/*static*/ ycAABB ycAABB::FromPoints( const ycVec3* points, const ycSize_t numPoints )
{
	return FromPoints( points, numPoints, sizeof(ycVec3) );
}

/*static*/ ycAABB ycAABB::FromPoints( const ycVec3* points, const ycSize_t numPoints, const ycSize_t stride )
{
#if 0
	ycSimd_t min = ycSimd_Splat( YC_F32_MAX );
	ycSimd_t max = ycSimd_Splat( -YC_F32_MAX );
	for( ycSize_t i = 0; i != numPoints; ++i )
	{
		const ycSimd_t pos = ycSimd_Load( (f32*)( (u8*)points + i*stride ) );
		min = ycSimd_Min( min, pos );
		max = ycSimd_Max( max, pos );
	}
	return ycAABB(
		ycVec3( ycSimd_GetX( min ), ycSimd_GetY( min ), ycSimd_GetZ( min ) ),
		ycVec3( ycSimd_GetX( max ), ycSimd_GetY( max ), ycSimd_GetZ( max ) )
	);
#else // ~25% faster
	const u8* bytes = (const u8*)points;
	enum { kUnroll = 5 }; // 10 min/max regs + 5 pos, 1 shy of 16 sse regs
	ycSimd_t min0, max0, min1, max1, min2, max2, min3, max3, min4, max4; // calculate several separate min/max to minimize instruction dependencies and let the cpu reorder more; this is actually what makes this faster than the not-unrolled version
	min0 = max0 = min1 = max1 = min2 = max2 = min3 = max3 = min4 = max4 = ycSimd_Load( (const f32*)bytes );
	ycSimd_t pos0, pos1, pos2, pos3, pos4;
	const ycSize_t unroll = numPoints / kUnroll;
	for( ycSize_t i = 0; i != unroll; ++i )
	{
		pos0 = ycSimd_Load( (const f32*)( bytes + (i*kUnroll  )*stride ) );
		pos1 = ycSimd_Load( (const f32*)( bytes + (i*kUnroll+1)*stride ) );
		pos2 = ycSimd_Load( (const f32*)( bytes + (i*kUnroll+2)*stride ) );
		pos3 = ycSimd_Load( (const f32*)( bytes + (i*kUnroll+3)*stride ) );
		pos4 = ycSimd_Load( (const f32*)( bytes + (i*kUnroll+4)*stride ) );
		min0 = ycSimd_Min( min0, pos0 );
		max0 = ycSimd_Max( max0, pos0 );
		min1 = ycSimd_Min( min1, pos1 );
		max1 = ycSimd_Max( max1, pos1 );
		min2 = ycSimd_Min( min2, pos2 );
		max2 = ycSimd_Max( max2, pos2 );
		min3 = ycSimd_Min( min3, pos3 );
		max3 = ycSimd_Max( max3, pos3 );
		min4 = ycSimd_Min( min4, pos4 );
		max4 = ycSimd_Max( max4, pos4 );
	}
	const u8* remain = bytes + unroll * kUnroll * stride;
	switch( numPoints % kUnroll )
	{
	case 4:
		pos4 = ycSimd_Load( (const f32*)( remain + 3*stride ) );
		min4 = ycSimd_Min( min4, pos4 );
		max4 = ycSimd_Max( max4, pos4 );
		YC_FALLTHROUGH;
	case 3:
		pos3 = ycSimd_Load( (const f32*)( remain + 2*stride ) );
		min3 = ycSimd_Min( min3, pos3 );
		max3 = ycSimd_Max( max3, pos3 );
		YC_FALLTHROUGH;
	case 2:
		pos2 = ycSimd_Load( (const f32*)( remain + 1*stride ) );
		min2 = ycSimd_Min( min2, pos2 );
		max2 = ycSimd_Max( max2, pos2 );
		YC_FALLTHROUGH;
	case 1:
		pos1 = ycSimd_Load( (const f32*)( remain + 0*stride ) );
		min1 = ycSimd_Min( min1, pos1 );
		max1 = ycSimd_Max( max1, pos1 );
		YC_FALLTHROUGH;
	case 0: break;
	}
	min0 = ycSimd_Min( min0, ycSimd_Min( min1, ycSimd_Min( min2, ycSimd_Min( min3, min4 ) ) ) );
	max0 = ycSimd_Max( max0, ycSimd_Max( max1, ycSimd_Max( max2, ycSimd_Max( max3, max4 ) ) ) );
	return ycAABB(
		ycVec3( ycSimd_GetX( min0 ), ycSimd_GetY( min0 ), ycSimd_GetZ( min0 ) ),
		ycVec3( ycSimd_GetX( max0 ), ycSimd_GetY( max0 ), ycSimd_GetZ( max0 ) )
	);
#endif
}

#include "ycPopDebugOpt.inc"
