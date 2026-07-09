#include "ycVec3.h"

// core
#include "ycRand.h"
#include "ycVec2.h"

#include "ycPushDebugOpt.inc"

ycVec3::ycVec3( const ycVec2 & _xy, f32 _z )
	: x( _xy.x )
	, y( _xy.y )
	, z( _z )
{
}

ycVec3::ycVec3( const f32 * values )
	: x( values[0] )
	, y( values[1] )
	, z( values[2] )
{
}

/*static*/ ycVec3 ycVec3::RandUnitXY( ycRand* rng )
{
	const f32 azimuth = rng->RandF() * YC_TWO_PI;
	return ycVec3( ycCos(azimuth), ycSin(azimuth), 0.0f );
}

/*static*/ ycVec3 ycVec3::RandUnitYZ( ycRand* rng )
{
	const f32 azimuth = rng->RandF() * YC_TWO_PI;
	return ycVec3( 0.0f, ycCos(azimuth), ycSin(azimuth) );
}

/*static*/ ycVec3 ycVec3::RandUnitXZ( ycRand* rng )
{
	const f32 azimuth = rng->RandF() * YC_TWO_PI;
	return ycVec3( ycCos(azimuth), 0.0f, ycSin(azimuth) );
}

/*static*/ ycVec3 ycVec3::RandUnit( ycRand* rng )
{
	const f32 a = rng->RandF() * YC_TWO_PI;
	const f32 b = rng->RandF() * YC_PI;
	return ycVec3( ycCos(a)*ycSin(b), ycSin(a)*ycSin(b), ycCos(b) );
}

ycVec2 ycVec3::ToXY2() const
{
	return ycVec2( x, y );
}

ycVec2 ycVec3::ToXZ2() const
{
	return ycVec2( x, z );
}

ycVec2 ycVec3::ToYZ2() const
{
	return ycVec2( y, z );
}

ycVec3 ycVec3::Perpendicular( const ycVec3 & first, const ycVec3 & defaultReturn ) const
{
	ycVec3 cross = Cross( first );
	if( cross.Normalize() == 0.0f )
	{
		return defaultReturn;
	}
	else
	{
		return cross;
	}
}

ycVec3 ycVec3::PerpendicularAlt( const ycVec3 & first, const ycVec3 & defaultReturn ) const
{
	return Cross( Perpendicular( first, defaultReturn ) ).Normalized();
}


void ycVec3::OrthoNormalBasis( ycVec3* b1, ycVec3* b2 )
{
	// see "Building an Orthonormal Basis, Revisited" (Duff et al, 2017)
	f32 sign = ( z > 0 ) ? 1.0f : -1.0f;
	const f32 ta = -1.0f / ( sign + z );
	const f32 tb = x * y * ta;
	if( b1 )
		*b1 = ycVec3( 1.0f + sign * x * x * ta, sign * tb, -sign * x );
	if( b2 )
		*b2 = ycVec3( tb, sign + y * y * ta, -y );
}

bool ycVec3::Equals( const ycVec3& v, f32 epsilon ) const 
{
	if( ycAbs( v.x - x ) < epsilon && ycAbs( v.y - y ) < epsilon && ycAbs( v.z - z ) < epsilon )
	{
		return true;
	}
	return false;
}

#include "ycPopDebugOpt.inc"

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycVec3_RandUnitXY ) // only both testing XY since YZ and XZ use the same approach
{
	ycRand rng;
	enum
	{
		#ifdef YC_ENABLE_SLOW_TESTS
			kIterations = 1024*1024,
		#else
			kIterations = 1024*64,
		#endif
	};

	ycVec3 drift( 0.0f );
	for( ureg i = 0; i != kIterations; ++i )
	{
		drift += ycVec3::RandUnitXY( &rng );
	}
	
	YC_TEST_PASS( "ycVec3::RandUnitXY() drift[ %.2f, %.2f, %.2f ] [ iterations: %u ]", drift.x, drift.y, drift.z, u32(kIterations) );
}

YC_BEGIN_TEST( ycVec3_RandUnit )
{
	ycRand rng;
	enum
	{
		#ifdef YC_ENABLE_SLOW_TESTS
			kIterations = 1024*1024,
		#else
			kIterations = 1024*64,
		#endif
	};

	ycVec3 drift( 0.0f );
	for( ureg i = 0; i != kIterations; ++i )
	{
		drift += ycVec3::RandUnit( &rng );
	}
	
	YC_TEST_PASS( "ycVec3::RandUnit() drift[ %.2f, %.2f, %.2f ] [ iterations: %u ]", drift.x, drift.y, drift.z, u32(kIterations) );
}

#endif // YC_TEST
