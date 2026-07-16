#include "ycCollisionTest.h"

#include "ycTri.h"
#include "ycSphere.h"
#include "ycCollisionSAT.h"

static u32 captureOpts = 0;

void ycCollisionTest::StartCapture( u32 bits )
{
	captureOpts |= bits;
}

bool ycCollisionTest::ShouldCapture( Capture bits )
{
	if( captureOpts & bits )
	{
		if( captureOpts & kCapture_EndAfterOne )
		{
			captureOpts = 0;
		}

		return true;
	}

	return false;
}

void ycCollisionTest::EndCapture( u32 bits )
{
	captureOpts &= ~bits;
}

// shoehorn in support for float hex literals via a user literal, since only MSVC offers support with c++14
constexpr f32 flit_exptwo( char c )
{
	if( c == 0 ) return 1.0f;
	//else
	return 2.0f * flit_exptwo( c - 1 );
}

constexpr f32 flit_exp( const char* Cs )
{

	if( *Cs == '-' )
	{
		return 1.0f / flit_exp( Cs + 1 );
	}
	if( *Cs == '+' )
	{
		return flit_exp( Cs + 1 );
	}

	char v = 0;

	while( *Cs )
	{
		v *= 10;

		char d = *Cs;
		if( d >= '0' && d <= '9' )
		{
			v += ( d - '0' );
		}
		Cs++;
	}

	return flit_exptwo(v);
}

constexpr f32 flit_mantissa( const char* Cs, f32 mul )
{
	char d = Cs[ 0 ];
	if( d >= '0' && d <= '9' )
	{
		return f32( d - '0' ) * mul + flit_mantissa( Cs+1, mul / 16.0f );
	}
	else if( d >= 'a' && d <= 'f' )
	{
		return f32( d - 'a' + 10 ) * mul + flit_mantissa( Cs + 1, mul / 16.0f );
	}
	else if( d == '.' )
	{
		return flit_mantissa( Cs + 1, mul );
	}
	return 0.0f;
}

constexpr f32 flit_digit_count( const char* Cs )
{
	f32 count = 1.0f;

	while( *Cs )
	{
		char e = Cs[1];
		if( e == 'p' || e == '.' )
		{
			return count;
		}

		count *= 16.0f;
		++Cs;
	}

	return count;
}

constexpr f32 flit_expand_unsigned( const char* Cs )
{
	//ycAssert( Cs[0] == '0' );
	//ycAssert( Cs[1] == 'x' );
	Cs += 2;

	f32 mantissa = flit_mantissa( Cs, flit_digit_count( Cs ) );

	while( *Cs != 'p' ) Cs++;
	Cs++;
	f32 exponent = flit_exp( Cs );

	return mantissa * exponent;
}


constexpr f32 operator ""_flit( const char* Cs, size_t )
{
	if( *Cs == '-' )
	{
		return -flit_expand_unsigned(Cs+1);
	}
	return flit_expand_unsigned(Cs);
}

#ifdef YC_MSVC
	#define FLIT( a, b ) b##f
#else
	#define FLIT( a, b ) #b##_flit
#endif


void AppendLiteralFloat( const f32& f, ycString& fs )
{
	if( f == 0.0f )
	{
		fs.Append( "0.0f" );
	}
	else if( f == 1.0f )
	{
		fs.Append( "1.0f" );
	}
	else if( f == -1.0f )
	{
		fs.Append( "-1.0f" );
	}
	else
	{
		fs.AppendF( "FLIT( %ff, %a )", f, f );
	}
}

void AppendLiteralMtx4( const ycMtx4& mtx, ycString& ms )
{
	ms.AppendF( "ycMtx4( " );
	AppendLiteralFloat( mtx.x.x, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.x.y, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.x.z, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.x.w, ms ); ms.Append( ", " );

	AppendLiteralFloat( mtx.y.x, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.y.y, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.y.z, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.y.w, ms ); ms.Append( ", " );

	AppendLiteralFloat( mtx.z.x, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.z.y, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.z.z, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.z.w, ms ); ms.Append( ", " );

	AppendLiteralFloat( mtx.t.x, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.t.y, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.t.z, ms ); ms.Append( ", " );
	AppendLiteralFloat( mtx.t.w, ms ); ms.Append( " )" );
}

void AppendLiteralVec3( const ycVec3& vec, ycString& ms )
{
	ms.Append( "ycVec3( " );
	AppendLiteralFloat( vec.x, ms );
	ms.Append( ", " );
	AppendLiteralFloat( vec.y, ms );
	ms.Append( ", " );
	AppendLiteralFloat( vec.z, ms );
	ms.Append( " )" );
}

ycString ycCollisionTest::MakeLiteral( const f32& ff, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "f32 %s = ", name.Get() );
	AppendLiteralFloat( ff, b );
	b.Append( ";" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycVec3& ff, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycVec3 %s = ", name.Get() );
	AppendLiteralVec3( ff, b );
	b.Append( ";" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycMtx4& ff, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycMtx4 %s = ", name.Get() );
	AppendLiteralMtx4( ff, b );
	b.Append( ";" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycAABB& ff, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycAABB %s = ycAABB::FromCenterExtents(", name.Get() );
	AppendLiteralVec3( ff.center, b );
	AppendLiteralVec3( ff.extents, b );
	b.Append( ");" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCollisionBox& box, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCollisionBox %s", name.Get() );
	b.Append( " = { \n" );
	b.Append( "         " ); AppendLiteralMtx4( box.forward, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralMtx4( box.inverse, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralVec3( box.invExtents, b ); b.Append( ",\n" );
	b.Append( "};" );

	return b;
}


ycString ycCollisionTest::MakeLiteral( const ycTri& tri, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycTri %s( \n", name.Get() );
	b.Append( "         " ); AppendLiteralVec3( tri.a, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralVec3( tri.b, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralVec3( tri.c, b ); b.Append( "\n" );
	b.Append( ");" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycSphere& sphere, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycSphere %s", name.Get() );
	b.Append( " = { \n" );
	b.Append( "         " ); AppendLiteralVec3( sphere.center, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralFloat( sphere.radius, b ); b.Append( "\n" );
	b.Append( "};" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCollisionSphere& sphere, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCollisionSphere %s", name.Get() );
	b.Append( " = { \n" );
	b.Append( "         " ); AppendLiteralMtx4( sphere.forward, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralMtx4( sphere.inverse, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralFloat( sphere.radius, b ); b.Append( "\n" );
	b.Append( "};" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCapsule& capsule, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCapsule %s", name.Get() );
	b.Append( "( " ); AppendLiteralVec3( capsule.center[ 0 ], b );
	b.Append( ", " ); AppendLiteralVec3( capsule.center[ 1 ], b );
	b.Append( ", " ); AppendLiteralFloat( capsule.radius, b );
	b.Append( " );" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCollisionCapsule& capsule, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCollisionCapsule %s", name.Get() );
	b.Append( " = { \n" );
	b.Append( "         " ); AppendLiteralMtx4( capsule.forward, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralMtx4( capsule.inverse, b ); b.Append( ",\n" );
	b.Append( "        { " ); AppendLiteralVec3( capsule.pts[0], b ); b.Append( ", " ); AppendLiteralVec3( capsule.pts[1], b ); b.Append( " },\n" );
	b.Append( "         " ); AppendLiteralFloat( capsule.radius, b ); b.Append( "\n" );
	b.Append( "};" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCollisionCylinder& cylinder, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCollisionCylinder %s", name.Get() );
	b.Append( " = { \n" );
	b.Append( "         " ); AppendLiteralVec3( cylinder.center, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralVec3( cylinder.upDir, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralFloat( cylinder.radius, b ); b.Append( ",\n" );
	b.Append( "         " ); AppendLiteralFloat( cylinder.height, b ); b.Append( "\n" );
	b.Append( "};" );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCollisionOptions& opt, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCollisionOptions %s = ycCollisionSweepOptions", name.Get() );
	b.AppendF( "( ycCollisionMask( %d ) )", opt.mask );
	b.AppendF( ".Welding( %s )", opt.welding ? "true" : "false" );
	b.AppendF( ".BackfaceBehavior( ycCollisionOptions::BackfaceBehavior( %d ) )", opt.backfaceBehavior );
	b.AppendF( ".ExtraRadius( FLIT( %ff, %a ) );", opt.extraRadius, opt.extraRadius );

	return b;
}

ycString ycCollisionTest::MakeLiteral( const ycCollisionSweepOptions& opt, ycStringRef name )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "ycCollisionSweepOptions %s = ycCollisionSweepOptions", name.Get() );
	b.AppendF( "( ycCollisionMask( %d ) )", opt.mask );
	b.AppendF( ".Welding( %s )", opt.welding? "true" : "false" );
	b.AppendF( ".BackfaceBehavior( ycCollisionOptions::BackfaceBehavior( %d ) )", opt.backfaceBehavior );
	b.AppendF( ".ExtraRadius( FLIT( %ff, %a ) )", opt.extraRadius, opt.extraRadius );
	b.AppendF( ".ResultsBehavior( ycCollisionSweepOptions::SweepResultsBehavior( %d ) );", opt.resultBehavior );

	return b;
}

ycString ycCollisionTest::TestMtd( const ycCollisionResult& mtd, ycStringRef mtdName )
{
	ycString b;
	b.Reserve( 128 );

	b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .dir.x - FLIT( %ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.dir.x, mtd.dir.x );
	b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .dir.y - FLIT( %ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.dir.y, mtd.dir.y );
	b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .dir.z - FLIT( %ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.dir.z, mtd.dir.z );
	b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .mag   - FLIT( %ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.mag, mtd.mag );
	//b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .pointB.x - FLIT( %.2ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.pointB.x, mtd.pointB.x );
	//b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .pointB.y - FLIT( %.2ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.pointB.y, mtd.pointB.y );
	//b.AppendF( "YC_TEST_CHECK( ycIsZero( %s .pointB.z - FLIT( %.2ff, %a ), tolerance ) );\n", mtdName.Get(), mtd.pointB.z, mtd.pointB.z );
	return b;
}



#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycSweep_CapturedTests )
{
	static_assert( "0x1p0"_flit == 1.0f, "check float lit works" );
	static_assert( "0x10p0"_flit == 16.0f, "check float lit works" );
	static_assert( "0x10.1p0"_flit == 16.0625f, "check float lit works" );
	static_assert( "0x1p1"_flit == 2.0f, "check float lit works" );
	static_assert( "0x1p2"_flit == 4.0f, "check float lit works" );
	static_assert( "0x1p3"_flit == 8.0f, "check float lit works" );
	static_assert( "0x1p4"_flit == 16.0f, "check float lit works" );
	static_assert( "0x1p5"_flit == 32.0f, "check float lit works" );
	static_assert( "0x1p6"_flit == 64.0f, "check float lit works" );
	static_assert( "0x1p7"_flit == 128.0f, "check float lit works" );
	static_assert( "0x1p8"_flit == 256.0f, "check float lit works" );
	static_assert( "0x1p9"_flit == 512.0f, "check float lit works" );
	static_assert( "0x1p10"_flit == 1024.0f, "check float lit works" );

#ifdef YC_MSVC
	// msvc allows hex float lits as an extension so we can check some of these

	static_assert( "0x1p8"_flit == 0x1p8, "check float lit works" );
	static_assert( "-0x1p8"_flit == -0x1p8, "check float lit works" );
	static_assert( "-0x1p+5"_flit == -0x1p+5, "check float lit works" );
	static_assert( "-0x1.41ep+0"_flit == -0x1.41ep+0, "check float lit works" );
	static_assert( "-0x1.41eb980000000p+5"_flit == -0x1.41eb980000000p+5, "check float lit works" );
	static_assert( "0x1.52190a0000000p-2"_flit == 0x1.52190a0000000p-2, "check float lit works" );
	static_assert( "-0x1.63311e0000000p+5"_flit == -0x1.63311e0000000p+5, "check float lit works" );
	static_assert( "0x1.a36e2e0000000p-14"_flit == 0x1.a36e2e0000000p-14, "check float lit works" );
	static_assert( 0x1p10 == 1024.0f, "check float lit works" );
#endif

	f32 tolerance = 0.0001f;

	{
		ycCollisionCylinder sA = {
				 ycVec3( 0.0f, FLIT( 0.870000f, 0x1.bd70a40000000p-1 ), FLIT( 3.000000f, 0x1.8000000000000p+1 ) ),
				 ycVec3( 0.0f, 1.0f, 0.0f ),
				 FLIT( 0.450000f, 0x1.cccccc0000000p-2 ),
				 FLIT( 1.620000f, 0x1.9eb8520000000p+0 )
		};
		ycTri sB(
			ycVec3( FLIT( 24.289177f, 0x1.84a0780000000p+4 ), 0.0f, FLIT( -24.000000f, -0x1.8000000000000p+4 ) ),
			ycVec3( FLIT( -23.710823f, -0x1.7b5f880000000p+4 ), 0.0f, FLIT( 24.000000f, 0x1.8000000000000p+4 ) ),
			ycVec3( FLIT( 24.289177f, 0x1.84a0780000000p+4 ), 0.0f, FLIT( 24.000000f, 0x1.8000000000000p+4 ) )
		);
		ycVec3 dir = ycVec3( 0.0f, -1.0f, 0.0f );
		f32 dist = FLIT( 0.061000f, 0x1.f3b6460000000p-5 );
		f32 kPrecision = FLIT( 0.000100f, 0x1.a36e2e0000000p-14 );

		ycCollisionResult res;
		ycCollisionSAT::SatIterativeSweep( sA, sB, dir, dist, &res, ycCollisionOptions::BackfaceBehavior( 1 ), kPrecision );

		YC_TEST_CHECK( ycIsZero( res.dir.x - FLIT( 0.000000f, 0x0.0000000000000p+0 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.dir.y - FLIT( 1.000000f, 0x1.0000000000000p+0 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.dir.z - FLIT( 0.000000f, 0x0.0000000000000p+0 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.mag - FLIT( 0.000000f, 0x0.0000000000000p+0 ), tolerance ) );
	}

	{
		ycMtx4 sA = ycMtx4( FLIT( 0.050000f, 0x1.99999a0000000p-5 ), 0.0f, 0.0f, 0.0f, 0.0f, FLIT( 0.100000f, 0x1.99999a0000000p-4 ), 0.0f, 0.0f, 0.0f, 0.0f, FLIT( 0.050000f, 0x1.99999a0000000p-5 ), 0.0f, FLIT( -62.385201f, -0x1.f314e40000000p+5 ), FLIT( 12.210194f, 0x1.86b9e80000000p+3 ), FLIT( 9.212136f, 0x1.26c9d20000000p+3 ), 1.0f );
		ycCollisionCylinder sB = {
				 ycVec3( FLIT( -72.000000f, -0x1.2000000000000p+6 ), FLIT( 6.000000f, 0x1.8000000000000p+2 ), FLIT( 16.000000f, 0x1.0000000000000p+4 ) ),
				 ycVec3( 0.0f, 1.0f, 0.0f ),
				 FLIT( 3.500000f, 0x1.c000000000000p+1 ),
				 FLIT( 12.000000f, 0x1.8000000000000p+3 )
		};
		ycVec3 dir = ycVec3( FLIT( -0.724548f, -0x1.72f7e40000000p-1 ), FLIT( -0.599577f, -0x1.32fbc60000000p-1 ), FLIT( 0.339909f, 0x1.5c11280000000p-2 ) );
		f32 dist = FLIT( 17.913000f, 0x1.1e9ba60000000p+4 );
		f32 kPrecision = FLIT( 0.000100f, 0x1.a36e2e0000000p-14 );

		ycCollisionResult res;
		ycCollisionSAT::SatIterativeSweep( sA, sB, dir, dist, &res, ycCollisionOptions::BackfaceBehavior( 1 ), kPrecision );

		YC_TEST_CHECK( ycIsZero( res.dir.x - FLIT( 0.488825f, 0x1.f48e740000000p-2 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.dir.y - FLIT( 0.000000f, 0x0.0000000000000p+0 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.dir.z - FLIT( -0.872382f, -0x1.bea8de0000000p-1 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.mag - FLIT( 0.000000f, 0x0.0000000000000p+0 ), tolerance ) );
	}

	{
		ycCollisionCylinder sA = {
				 ycVec3( FLIT( -78.339767f, -0x1.395bec0000000p+6 ), FLIT( 5.177955f, 0x1.4b639e0000000p+2 ), FLIT( -19.001108f, -0x1.30048a0000000p+4 ) ),
				 ycVec3( 0.0f, 1.0f, 0.0f ),
				 FLIT( 0.610000f, 0x1.3851ec0000000p-1 ),
				 FLIT( 1.040000f, 0x1.0a3d700000000p+0 )
		};
		ycTri sB(
			ycVec3( FLIT( -76.513695f, -0x1.320e060000000p+6 ), FLIT( -0.288461f, -0x1.2762380000000p-2 ), FLIT( -17.689613f, -0x1.1b08a80000000p+4 ) ),
			ycVec3( FLIT( -70.356560f, -0x1.196d1e0000000p+6 ), FLIT( 4.368509f, 0x1.1795a60000000p+2 ), FLIT( -19.517242f, -0x1.3846a00000000p+4 ) ),
			ycVec3( FLIT( -76.700508f, -0x1.32cd520000000p+6 ), FLIT( 2.684119f, 0x1.5791340000000p+1 ), FLIT( -23.039396f, -0x1.70a15e0000000p+4 ) )
		);
		ycVec3 dir = ycVec3( FLIT( 0.812250f, 0x1.9fdf320000000p-1 ), FLIT( -0.572018f, -0x1.24df8c0000000p-1 ), FLIT( -0.114218f, -0x1.d3d6a20000000p-4 ) );
		f32 dist = FLIT( 7.677000f, 0x1.eb53f80000000p+2 );
		f32 kPrecision = FLIT( 0.000100f, 0x1.a36e2e0000000p-14 );

		ycCollisionResult res;
		ycCollisionSAT::SatIterativeSweep( sA, sB, dir, dist, &res, ycCollisionOptions::BackfaceBehavior( 1 ), kPrecision );

		YC_TEST_CHECK( ycIsZero( res.dir.x - FLIT( -0.452349f, -0x1.cf34a40000000p-2 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.dir.y - FLIT( 0.772780f, 0x1.8ba9d00000000p-1 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.dir.z - FLIT( 0.445187f, 0x1.c7df220000000p-2 ), tolerance ) );
		YC_TEST_CHECK( ycIsZero( res.mag - FLIT( 0.000000f, 0x0.0000000000000p+0 ), tolerance ) );
	}
}
#endif
