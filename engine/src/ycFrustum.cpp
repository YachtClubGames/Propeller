#include "ycFrustum.h"

#include "ycAABB.h"
#include "ycMath.h"
#include "ycMtx4.h"
#include "ycPlane.h"
#include "ycGeo.h"
#include "ycSphere.h"
#include "ycStd.h"

namespace
{
	void AddFace( ycVec3 *pts32, ycVec3 *pts8, u32 i1, u32 i2, u32 i3 )
	{
		pts32[0] = pts8[i1];
		pts32[1] = pts8[i2];
		pts32[2] = pts8[i3];
	}
}

ycFrustum::ycFrustum( const ycMtx4& mtx )
{
#if 1
	planes[ kPlane_Left ].normal.x = mtx.x.w + mtx.x.x;
	planes[ kPlane_Left ].normal.y = mtx.y.w + mtx.y.x;
	planes[ kPlane_Left ].normal.z = mtx.z.w + mtx.z.x;
	planes[ kPlane_Left ].d        = mtx.t.w + mtx.t.x;

	planes[ kPlane_Right ].normal.x = mtx.x.w - mtx.x.x;
	planes[ kPlane_Right ].normal.y = mtx.y.w - mtx.y.x;
	planes[ kPlane_Right ].normal.z = mtx.z.w - mtx.z.x;
	planes[ kPlane_Right ].d        = mtx.t.w - mtx.t.x;

	planes[ kPlane_Top ].normal.x = mtx.x.w - mtx.x.y;
	planes[ kPlane_Top ].normal.y = mtx.y.w - mtx.y.y;
	planes[ kPlane_Top ].normal.z = mtx.z.w - mtx.z.y;
	planes[ kPlane_Top ].d        = mtx.t.w - mtx.t.y;

	planes[ kPlane_Bottom ].normal.x = mtx.x.w + mtx.x.y;
	planes[ kPlane_Bottom ].normal.y = mtx.y.w + mtx.y.y;
	planes[ kPlane_Bottom ].normal.z = mtx.z.w + mtx.z.y;
	planes[ kPlane_Bottom ].d        = mtx.t.w + mtx.t.y;

	planes[ kPlane_Near ].normal.x = mtx.x.z;
	planes[ kPlane_Near ].normal.y = mtx.y.z;
	planes[ kPlane_Near ].normal.z = mtx.z.z;
	planes[ kPlane_Near ].d        = mtx.t.z;

	planes[ kPlane_Far ].normal.x = mtx.x.w - mtx.x.z;
	planes[ kPlane_Far ].normal.y = mtx.y.w - mtx.y.z;
	planes[ kPlane_Far ].normal.z = mtx.z.w - mtx.z.z;
	planes[ kPlane_Far ].d        = mtx.t.w - mtx.t.z;

	for( s32 i = 0; i < kPlane_Count; ++i )
	{
		f32 invMag = 1.0f / planes[ i ].normal.Mag();
		planes[ i ].normal *= invMag;
		planes[ i ].d      *= -invMag;
	}
#else
	f32 magInverse;
	ycVec3 newNormal;
	f32 newD;
	
	// near
	newNormal.x = mtx.x.z;
	newNormal.y = mtx.y.z;
	newNormal.z = mtx.z.z;
	newD = mtx.t.z;
	magInverse = 1.f / newNormal.Mag();
	newNormal   *= magInverse;
	newD *= magInverse;
	planes[ kPlane_Near ].SetNormal( newNormal );
	planes[ kPlane_Near ].SetD( newD );

	// far
	newNormal.x = mtx.x.w - mtx.x.z;
	newNormal.y = mtx.y.w - mtx.y.z;
	newNormal.z = mtx.z.w - mtx.z.z;
	newD = mtx.t.w - mtx.t.z;
	magInverse = 1.f / newNormal.Mag();
	newNormal   *= magInverse;
	newD *= magInverse;
	planes[ kPlane_Far ].SetNormal( newNormal );
	planes[ kPlane_Far ].SetD( newD );

	// left
	newNormal.x = mtx.x.w + mtx.x.x;
	newNormal.y = mtx.y.w + mtx.y.x;
	newNormal.z = mtx.z.w + mtx.z.x;
	newD = mtx.t.w + mtx.t.x;
	magInverse = 1.f / newNormal.Mag();
	newNormal   *= magInverse;
	newD *= magInverse;
	planes[ kPlane_Left ].SetNormal( newNormal );
	planes[ kPlane_Left ].SetD( newD );

	// right
	newNormal.x = mtx.x.w - mtx.x.x;
	newNormal.y = mtx.y.w - mtx.y.x;
	newNormal.z = mtx.z.w - mtx.z.x;
	newD = mtx.t.w - mtx.t.x;
	magInverse = 1.f / newNormal.Mag();
	newNormal   *= magInverse;
	newD *= magInverse;
	planes[ kPlane_Right ].SetNormal( newNormal );
	planes[ kPlane_Right ].SetD( newD );

	// top
	newNormal.x = mtx.x.w - mtx.x.y;
	newNormal.y = mtx.y.w - mtx.y.y;
	newNormal.z = mtx.z.w - mtx.z.y;
	newD = mtx.t.w - mtx.t.y;
	magInverse = 1.f / newNormal.Mag();
	newNormal   *= magInverse;
	newD *= magInverse;
	planes[ kPlane_Top ].SetNormal( newNormal );
	planes[ kPlane_Top ].SetD( newD );

	// bot
	newNormal.x = mtx.x.w + mtx.x.y;
	newNormal.y = mtx.y.w + mtx.y.y;
	newNormal.z = mtx.z.w + mtx.z.y;
	newD = mtx.t.w + mtx.t.y;
	magInverse = 1.f / newNormal.Mag();
	newNormal   *= magInverse;
	newD *= magInverse;
	planes[ kPlane_Bottom ].SetNormal( newNormal );
	planes[ kPlane_Bottom ].SetD( newD );
#endif

	if( ycDepthConfig::IS_DEPTH_REVERSED )
	{
		// near/far planes need to be swapped in reversed depth mode
		ycSwap( planes[ kPlane_Near ], planes[ kPlane_Far ] );
	}
}

void ycFrustum::Expand( const ycVec3& pos )
{
	for( s32 s = 0; s < kPlane_Count; ++s )
	{
		f32 dot = pos.Dot( planes[ s ].normal );
		if( planes[ s ].d > dot ) { planes[ s ].d = dot; }
	}
}

void ycFrustum::Expand( const ycSphere& sphere )
{
	for( s32 s = 0; s < kPlane_Count; ++s )
	{
		f32 dot = sphere.center.Dot( planes[ s ].normal ) - sphere.radius;
		if( planes[ s ].d > dot ) { planes[ s ].d = dot; }
	}
}

ycVec3 ycFrustum::GetNearViewPos() const
{
	ycVec3 xTop;
	ycVec3 xBottom;
	ycVec3 yLeft;
	ycVec3 yRight;
	//find out where the eye should be for width
	planes[ycFrustum::kPlane_Top].Intersection( planes[ycFrustum::kPlane_Left], planes[ycFrustum::kPlane_Right], &xTop );
	planes[ycFrustum::kPlane_Bottom].Intersection( planes[ycFrustum::kPlane_Left], planes[ycFrustum::kPlane_Right], &xBottom );
	ycVec3 xview = 0.5f * ( xTop + xBottom );
	//find out where the eye should be for height
	planes[ycFrustum::kPlane_Left].Intersection( planes[ycFrustum::kPlane_Top], planes[ycFrustum::kPlane_Bottom], &yLeft );
	planes[ycFrustum::kPlane_Right].Intersection( planes[ycFrustum::kPlane_Top], planes[ycFrustum::kPlane_Bottom], &yRight );
	ycVec3 yview = 0.5f * ( yLeft + yRight );
	return planes[ycFrustum::kPlane_Near].SignedDistance( xview ) < planes[ycFrustum::kPlane_Near].SignedDistance( yview ) ? xview : yview;
}

ycVec3 ycFrustum::GetCorner( s32 index ) const
{
	ycVec3 pt;
	switch( index )
	{
	case 0: planes[kPlane_Left].Intersection( planes[kPlane_Bottom], planes[kPlane_Near], &pt ); return pt;
	case 1: planes[kPlane_Right].Intersection( planes[kPlane_Bottom], planes[kPlane_Near], &pt ); return pt;
	case 2: planes[kPlane_Right].Intersection( planes[kPlane_Top], planes[kPlane_Near], &pt ); return pt;
	case 3: planes[kPlane_Left].Intersection( planes[kPlane_Top], planes[kPlane_Near], &pt ); return pt;
	case 4: planes[kPlane_Left].Intersection( planes[kPlane_Bottom], planes[kPlane_Far], &pt ); return pt;
	case 5: planes[kPlane_Right].Intersection( planes[kPlane_Bottom], planes[kPlane_Far], &pt ); return pt;
	case 6: planes[kPlane_Right].Intersection( planes[kPlane_Top], planes[kPlane_Far], &pt ); return pt;
	case 7: planes[kPlane_Left].Intersection( planes[kPlane_Top], planes[kPlane_Far], &pt ); return pt;
	}
	return ycVec3::ZERO();
}

u32 ycFrustum::ToVerts( ycVec3 * pts36 ) const
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

bool ycFrustum::Contains( const ycVec3 & pt ) const
{
	return ycGeo::ContainsFrustumPt( *this, pt );
}

bool ycFrustum::Contains( const ycVec3 & pt, f32 radius ) const
{
	for( s32 i = 0; i < ycFrustum::kPlane_Count; i++)
	{
		const ycPlane& p = planes[ i ];
		if( !p.IsFinite() ) { continue; }

		f32 dist = p.SignedDistance( pt );
		if( dist < -radius )
		{
			return false;
		}
		/*if( ycAbs( dist ) < radius )
		{
			return true;
		}*/
	}
	return true;
}

bool ycFrustum::Intersects( const ycAABB& aabb ) const
{
	return ycGeo::IntersectsAABBFrustum( aabb, *this );
}

bool ycFrustum::Intersects( const ycSphere& sphere ) const
{
	return ycGeo::IntersectsSphereFrustum( sphere, *this );
}
