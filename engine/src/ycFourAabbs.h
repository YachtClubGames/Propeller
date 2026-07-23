#pragma once

#include "ycCommon.h"

#include "ycBox.h"
#include "ycSimd.h"

struct ycFourAABBs //%
{
yceditable:
	f32 m_minX[ 4 ];
	f32 m_minY[ 4 ];
	f32 m_minZ[ 4 ];

	f32 m_maxX[ 4 ];
	f32 m_maxY[ 4 ];
	f32 m_maxZ[ 4 ];

public:
	ycBox GetBox( u32 ind ) const
	{
		return ycBox( ycVec3( m_minX[ ind ], m_minY[ ind ], m_minZ[ ind ] ), ycVec3( m_maxX[ ind ], m_maxY[ ind ], m_maxZ[ ind ] ) );
	}

	void SetBox( const ycBox& b, u32 ind ) 
	{
		m_minX[ ind ] = b.min.x;
		m_minY[ ind ] = b.min.y;
		m_minZ[ ind ] = b.min.z;

		m_maxX[ ind ] = b.max.x;
		m_maxY[ ind ] = b.max.y;
		m_maxZ[ ind ] = b.max.z;
	}

	bool IntersectionInvRay( const ycVec3& rayStart, const ycVec3& rayInvDir, f32 maxT, u32 out[ 4 ] ) const;
	void IntersectionInvRayPadded( const ycVec3& rayStart, const ycVec3& rayInvDir, const ycVec3& pad, f32 maxT, u32 out[ 4 ] ) const;
	void IntersectionBox( const ycBox& a, u32 out[ 4 ] ) const;
};



inline bool ycFourAABBs::IntersectionInvRay( const ycVec3& rayStart, const ycVec3& rayInvDir, f32 maxT, u32 out[ 4 ] ) const
{
	ycSimd_t minX = ycSimd_Load( m_minX );
	ycSimd_t minY = ycSimd_Load( m_minY );
	ycSimd_t minZ = ycSimd_Load( m_minZ );

	ycSimd_t maxX = ycSimd_Load( m_maxX );
	ycSimd_t maxY = ycSimd_Load( m_maxY );
	ycSimd_t maxZ = ycSimd_Load( m_maxZ );

	//ycSimd_t rayStart_v = ycSimd_Load( rayStart.a );
	ycSimd_t rayStart_x = ycSimd_Splat( rayStart.x );
	ycSimd_t rayStart_y = ycSimd_Splat( rayStart.y );
	ycSimd_t rayStart_z = ycSimd_Splat( rayStart.z );

	//ycSimd_t rayInvDir_v = ycSimd_Load( rayInvDir.a );
	ycSimd_t rayInvDir_x = ycSimd_Splat( rayInvDir.x );
	ycSimd_t rayInvDir_y = ycSimd_Splat( rayInvDir.y );
	ycSimd_t rayInvDir_z = ycSimd_Splat( rayInvDir.z );

	//ycVec3 d0 = ( aabb.GetMin() - rayStart ) * rayInvDir;
	ycSimd_t d0_x = ycSimd_Mul( ycSimd_Sub( minX, rayStart_x ), rayInvDir_x );
	ycSimd_t d0_y = ycSimd_Mul( ycSimd_Sub( minY, rayStart_y ), rayInvDir_y );
	ycSimd_t d0_z = ycSimd_Mul( ycSimd_Sub( minZ, rayStart_z ), rayInvDir_z );

	//ycVec3 d1 = ( aabb.GetMax() - rayStart ) * rayInvDir;
	ycSimd_t d1_x = ycSimd_Mul( ycSimd_Sub( maxX, rayStart_x ), rayInvDir_x );
	ycSimd_t d1_y = ycSimd_Mul( ycSimd_Sub( maxY, rayStart_y ), rayInvDir_y );
	ycSimd_t d1_z = ycSimd_Mul( ycSimd_Sub( maxZ, rayStart_z ), rayInvDir_z );

	//ycVec3 v0 = ycVec3::Min( d0, d1 );
	ycSimd_t v0_x = ycSimd_Min( d0_x, d1_x );
	ycSimd_t v0_y = ycSimd_Min( d0_y, d1_y );
	ycSimd_t v0_z = ycSimd_Min( d0_z, d1_z );

	//ycVec3 v1 = ycVec3::Max( d0, d1 );
	ycSimd_t v1_x = ycSimd_Max( d0_x, d1_x );
	ycSimd_t v1_y = ycSimd_Max( d0_y, d1_y );
	ycSimd_t v1_z = ycSimd_Max( d0_z, d1_z );

	ycSimd_t tmin = ycSimd_Max( ycSimd_Max( v0_x, v0_y ), v0_z );
	ycSimd_t tmax = ycSimd_Min( ycSimd_Min( v1_x, v1_y ), v1_z );

	// if( ( tmax >= 0 ) && ( tmax >= tmin ) && ( tmin <= maxT ) )
	ycSimd_t tmax_g_zero = ycSimd_GreaterEq( tmax, ycSimd_Splat( 0.0f ) );
	ycSimd_t tmax_g_tmin = ycSimd_GreaterEq( tmax, tmin );
	ycSimd_t tmin_l_maxt = ycSimd_LessEq( tmin, ycSimd_Splat( maxT ) );

	ycSimd_t hit = ycSimd_BitAnd( tmax_g_zero, ycSimd_BitAnd( tmax_g_tmin, tmin_l_maxt ) );

	ycSimd_Store( (f32*)out, hit );

	return ycSimd_AnyTrue(hit);

	/*
	for( int ii = 0; ii < 4; ++ii )
	{
	ycAssert( !!out[ ii ] == ycGeo::IntersectionInvRayBox( nullptr, rayStart, rayInvDir, maxT, GetBox( ii ) ) );
	}
	*/

}


inline void ycFourAABBs::IntersectionInvRayPadded( const ycVec3& rayStart, const ycVec3& rayInvDir, const ycVec3& pad, f32 maxT, u32 out[ 4 ] ) const
{
	//ycSimd_t pad_v = ycSimd_Load( pad.a );
	ycSimd_t pad_x = ycSimd_Splat( pad.x );
	ycSimd_t pad_y = ycSimd_Splat( pad.y );
	ycSimd_t pad_z = ycSimd_Splat( pad.z );

	ycSimd_t minX = ycSimd_Sub( ycSimd_Load( m_minX ), pad_x );
	ycSimd_t minY = ycSimd_Sub( ycSimd_Load( m_minY ), pad_y );
	ycSimd_t minZ = ycSimd_Sub( ycSimd_Load( m_minZ ), pad_z );

	ycSimd_t maxX = ycSimd_Add( ycSimd_Load( m_maxX ), pad_x );
	ycSimd_t maxY = ycSimd_Add( ycSimd_Load( m_maxY ), pad_y );
	ycSimd_t maxZ = ycSimd_Add( ycSimd_Load( m_maxZ ), pad_z );

	//ycSimd_t rayStart_v = ycSimd_Load( rayStart.a );
	ycSimd_t rayStart_x = ycSimd_Splat( rayStart.x );
	ycSimd_t rayStart_y = ycSimd_Splat( rayStart.y );
	ycSimd_t rayStart_z = ycSimd_Splat( rayStart.z );

	//ycSimd_t rayInvDir_v = ycSimd_Load( rayInvDir.a );
	ycSimd_t rayInvDir_x = ycSimd_Splat( rayInvDir.x );
	ycSimd_t rayInvDir_y = ycSimd_Splat( rayInvDir.y );
	ycSimd_t rayInvDir_z = ycSimd_Splat( rayInvDir.z );

	//ycVec3 d0 = ( aabb.GetMin() - rayStart ) * rayInvDir;
	ycSimd_t d0_x = ycSimd_Mul( ycSimd_Sub( minX, rayStart_x ), rayInvDir_x );
	ycSimd_t d0_y = ycSimd_Mul( ycSimd_Sub( minY, rayStart_y ), rayInvDir_y );
	ycSimd_t d0_z = ycSimd_Mul( ycSimd_Sub( minZ, rayStart_z ), rayInvDir_z );

	//ycVec3 d1 = ( aabb.GetMax() - rayStart ) * rayInvDir;
	ycSimd_t d1_x = ycSimd_Mul( ycSimd_Sub( maxX, rayStart_x ), rayInvDir_x );
	ycSimd_t d1_y = ycSimd_Mul( ycSimd_Sub( maxY, rayStart_y ), rayInvDir_y );
	ycSimd_t d1_z = ycSimd_Mul( ycSimd_Sub( maxZ, rayStart_z ), rayInvDir_z );

	//ycVec3 v0 = ycVec3::Min( d0, d1 );
	ycSimd_t v0_x = ycSimd_Min( d0_x, d1_x );
	ycSimd_t v0_y = ycSimd_Min( d0_y, d1_y );
	ycSimd_t v0_z = ycSimd_Min( d0_z, d1_z );

	//ycVec3 v1 = ycVec3::Max( d0, d1 );
	ycSimd_t v1_x = ycSimd_Max( d0_x, d1_x );
	ycSimd_t v1_y = ycSimd_Max( d0_y, d1_y );
	ycSimd_t v1_z = ycSimd_Max( d0_z, d1_z );

	ycSimd_t tmin = ycSimd_Max( ycSimd_Max( v0_x, v0_y ), v0_z );
	ycSimd_t tmax = ycSimd_Min( ycSimd_Min( v1_x, v1_y ), v1_z );

	// if( ( tmax >= 0 ) && ( tmax >= tmin ) && ( tmin <= maxT ) )
	ycSimd_t tmax_g_zero = ycSimd_GreaterEq( tmax, ycSimd_Splat( 0.0f ) );
	ycSimd_t tmax_g_tmin = ycSimd_GreaterEq( tmax, tmin );
	ycSimd_t tmin_l_maxt = ycSimd_LessEq( tmin, ycSimd_Splat( maxT ) );

	ycSimd_t hit = ycSimd_BitAnd( tmax_g_zero, ycSimd_BitAnd( tmax_g_tmin, tmin_l_maxt ) );

	ycSimd_Store( (f32*)out, hit );

	/*
	for( int ii = 0; ii < 4; ++ii )
	{
	ycAssert( !!out[ ii ] == ycGeo::IntersectionInvRayBox( nullptr, rayStart, rayInvDir, maxT, GetBox( ii ) ) );
	}
	*/

}

inline void ycFourAABBs::IntersectionBox( const ycBox& a, u32 out[ 4 ] ) const
{
	ycSimd_t a_minX = ycSimd_Splat( a.GetMin().x );
	ycSimd_t a_minY = ycSimd_Splat( a.GetMin().y );
	ycSimd_t a_minZ = ycSimd_Splat( a.GetMin().z );

	ycSimd_t a_maxX = ycSimd_Splat( a.GetMax().x );
	ycSimd_t a_maxY = ycSimd_Splat( a.GetMax().y );
	ycSimd_t a_maxZ = ycSimd_Splat( a.GetMax().z );

	ycSimd_t b_minX = ycSimd_Load( m_minX );
	ycSimd_t b_minY = ycSimd_Load( m_minY );
	ycSimd_t b_minZ = ycSimd_Load( m_minZ );

	ycSimd_t b_maxX = ycSimd_Load( m_maxX );
	ycSimd_t b_maxY = ycSimd_Load( m_maxY );
	ycSimd_t b_maxZ = ycSimd_Load( m_maxZ );

	ycSimd_t result = ycSimd_LessEq( a_minX, b_maxX );
	result = ycSimd_BitAnd( result, ycSimd_LessEq( b_minX, a_maxX ) );

	result = ycSimd_BitAnd( result, ycSimd_LessEq( a_minY, b_maxY ) );
	result = ycSimd_BitAnd( result, ycSimd_LessEq( b_minY, a_maxY ) );

	result = ycSimd_BitAnd( result, ycSimd_LessEq( a_minZ, b_maxZ ) );
	result = ycSimd_BitAnd( result, ycSimd_LessEq( b_minZ, a_maxZ ) );

	ycSimd_Store( (f32*)out, result );
}

