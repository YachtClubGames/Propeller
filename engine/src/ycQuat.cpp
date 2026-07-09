#include "ycQuat.h"

#include "ycDataSchema.h"
#include "ycMtx4.h"
#include "ycRand.h"

#include "ycPushDebugOpt.inc"

#ifndef YC_CODEGEN
void ycDataMetaStartup()
{
	ycData::Schema::SetConstructor( ycData::GetRecordDef< ycQuat >(), []( void* ptr ) {
		ycQuat* q = ( ycQuat* )ptr;
		q->x = 0.0f;
		q->y = 0.0f;
		q->z = 0.0f;
		q->w = 1.0f;
	} );
}
#endif // !YC_CODEGEN

ycQuat::ycQuat( const ycMtx4& mat )
{
	( *this ) = ycQuat::GetQuat( mat );
}

ycMtx4 ycQuat::GetAsMtx4() const
{
	ycAssert( IsNormalized() );
	ycVec4 mtx_x;
	ycVec4 mtx_y;
	ycVec4 mtx_z;
	ycVec4 mtx_t = ycVec4( 0.f, 0.f, 0.f, 1.f );

	f32 xx = x * x;
	f32 yy = y * y;
	f32 zz = z * z;
	f32 xy = x * y;
	f32 xz = x * z;
	f32 yz = y * z;
	f32 wx = w * x;
	f32 wy = w * y;
	f32 wz = w * z;

	mtx_x.x = 1.f - ( 2.f*yy ) - ( 2.f*zz );
	mtx_x.y = ( 2.f*xy ) + ( 2.f*wz );
	mtx_x.z = ( 2.f*xz ) - ( 2.f*wy );
	mtx_x.w = 0.f;

	mtx_y.x = ( 2.f*xy ) - ( 2.f*wz );
	mtx_y.y = 1.f - ( 2.f*xx ) - ( 2.f*zz );
	mtx_y.z = ( 2.f*yz ) + ( 2.f*wx );
	mtx_y.w = 0.f;

	mtx_z.x = ( 2.f*xz ) + ( 2.f*wy );
	mtx_z.y = ( 2.f*yz ) - ( 2.f*wx );
	mtx_z.z = 1.f - ( 2.f*xx ) - ( 2.f*yy );
	mtx_z.w = 0.f;

	return ycMtx4( mtx_x, mtx_y, mtx_z, mtx_t );
}

/*static*/ ycQuat ycQuat::LookAt( const ycVec3& fwd, const ycVec3& inUp )
{
	ycVec3 z = fwd.Normalized();
	if( z.MagSq() < 1e-4f )
	{
		// edge-case hack
		z = ycVec3::FORWARD();
	}

	ycVec3 up = inUp;
	if( up.MagSq() < 1e-4f )
	{
		// edge-case hack
		up = ycVec3::UP();
	}

	const ycVec3 x = up.Cross( z ).Normalized();
	if( x.MagSq() < 1e-4f )
	{
		// edge-case hack
		return ycQuat( ycVec3::FORWARD(), z );
	}

	const ycVec3 y = z.Cross( x ).Normalized();

	return ycQuat( ycMtx4( ycVec4( x, 0.0f ), ycVec4( y, 0.0f ), ycVec4( z, 0.0f ), ycVec4( 0.0f ) ) ).Normalized();
}

ycVec3 ycQuat::ToEuler() const
{
	f32 test = x*w - z*y;
	if( test < -0.49999f ) 
	{ // singularity at north pole
		f32 yaw = 2 * ycArcTan2( z, w );
		f32 pitch = -YC_PI/2;
		f32 roll = 0;
		return ycVec3( pitch, yaw, roll );
	}
	if( test > 0.49999f ) 
	{ // singularity at south pole
		f32 yaw = -2 * ycArcTan2( z, w );
		f32 pitch = YC_PI/2;
		f32 roll = 0;
		return ycVec3( pitch, yaw, roll );
	}

	f32 pitch = ycArcSin( 2*test );
	f32 yaw   = ycArcTan2( 2*y*w + 2*z*x, 1 - 2*y*y - 2*x*x );
	f32 roll  = ycArcTan2( 2*z*w + 2*y*x, 1 - 2*z*z - 2*x*x );

	return ycVec3( pitch, yaw, roll );
}

#include "ycPopDebugOpt.inc"

#ifdef YC_TEST
#include "ycTest.h"
#include <cstring>

namespace
{
	bool ConstructorTests();
	bool QuatConstructorAxisAngleTest();
	bool QuatConstructorFromMat4Test();

	bool DotTest();

	bool MagnitudeTests();
	bool LengthSqTest();
	bool MagSqTest();
	bool LengthTest();
	bool MagTest();

	bool NormalizationTests();
	bool NormalizeTest();
	bool NormalizedTest();
	bool IsNormalizedTest();

	bool ConjugateTests();
	bool GetConjugateTest();
	bool ConjugateTest();

	bool InverseTests();
	bool GetInverseTest();
	bool InvertTest();

	bool VectorRotationTests();
	bool GetRotatedVec3Test();
	bool GetUnRotatedVec3Test();

	bool InterpolationTests();
	bool GetSlerpTest();
	bool GetLerpTest();

	bool FormatConversionTests();
	bool MatrixConversionTests();
	bool EulerAnglesTests();
	bool GetAxisTest();
	bool GetAngleTest();

	bool WithinTolerance( const f32 expected, const f32 actual, const f32 tolerance );
	bool WithinTolerance( const ycQuat& expected, const ycQuat& actual, const f32 tolerance );
	bool WithinToleranceDual( const ycQuat& expected, const ycQuat& actual, const f32 tolerance );
	bool WithinTolerance( const ycVec3& expected, const ycVec3& actual, const f32 tolerance );
	bool WithinTolerance( const ycVec4& expected, const ycVec4& actual, const f32 tolerance );
	ycVec3 GetRandomVec3( const f32 min, const f32 max );
	static ycQuat GetRandomQuat();
	
	// Tests not conducted: Slerp, Lerp, GetAsMtx4, GetRotatedVec4, GetUnRotatedVec4
		// Note the list of methods above are essentially tested in the precision tests battery
}


YC_BEGIN_TEST( ycQuat_Basic )
{
	YC_TEST_CHECK( ConstructorTests() );
	YC_TEST_PASS( "Constructor Tests" );

	YC_TEST_CHECK( DotTest() );
	YC_TEST_PASS( "DotAndCross Tests");

	YC_TEST_CHECK( MagnitudeTests() );
	YC_TEST_PASS( "Magnitude Tests");

	YC_TEST_CHECK( NormalizationTests() );
	YC_TEST_PASS( "Normalization Tests");

	YC_TEST_CHECK( ConjugateTests() );
	YC_TEST_PASS( "Congugate Tests");

	YC_TEST_CHECK( InverseTests() );
	YC_TEST_PASS( "Inverse Tests");

	YC_TEST_CHECK( VectorRotationTests() );
	YC_TEST_PASS( "Vector Rotation Tests");

	YC_TEST_CHECK( InterpolationTests() );
	YC_TEST_PASS( "Interpolation Tests");

	YC_TEST_CHECK( MatrixConversionTests() );
	YC_TEST_PASS( "Matrix Conversion Tests");

	YC_TEST_CHECK( EulerAnglesTests() );
	YC_TEST_PASS( "Euler Angles Tests");
}


namespace
{

	bool ConstructorTests()
	{
		return QuatConstructorAxisAngleTest() && QuatConstructorFromMat4Test();
	}


	bool QuatConstructorAxisAngleTest()
	{
		bool bTestPassed = true;
		f32 angle;
		f32 retAngle;
		ycVec3 axis;
		ycVec3 retAxis;
		ycQuat q;
		f32 tolerance = 0.000001f;

		angle = YC_PI;
		axis = ycVec3( 1, 1, 1 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		retAxis = q.GetAxis();
		retAngle = q.GetAngle();
		bTestPassed &= WithinTolerance( axis, retAxis, tolerance );
		bTestPassed &= WithinTolerance( angle, retAngle, tolerance );

		angle = YC_PI_OVER_2;
		axis = ycVec3( 3, 4, 5 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		retAxis = q.GetAxis();
		retAngle = q.GetAngle();
		bTestPassed &= WithinTolerance( axis, retAxis, tolerance );
		bTestPassed &= WithinTolerance( angle, retAngle, tolerance );

		return bTestPassed;
	}


	bool QuatConstructorFromMat4Test()
	{
		bool bTestPassed = true;
		f32 tolerance = 0.000001f;
		ycVec4 x, y, z, t;
		t = ycVec4( 0, 0, 0, 0 );
		ycMtx4 mat;
		ycQuat q_expected;
		ycQuat q_actual;

		x = ycVec4( -1, 0, 0, 0 );
		y = ycVec4( 0, 1, 0, 0 );
		z = ycVec4( 0, 0, -1, 0 );
		mat = ycMtx4( x, y, z, t );
		q_expected = ycQuat( ycVec3::YAXIS(), YC_PI );
		q_actual = ycQuat( mat );
		bTestPassed &= WithinTolerance( q_expected.x, q_actual.x, tolerance );
		bTestPassed &= WithinTolerance( q_expected.y, q_actual.y, tolerance );
		bTestPassed &= WithinTolerance( q_expected.z, q_actual.z, tolerance );
		bTestPassed &= WithinTolerance( q_expected.w, q_actual.w, tolerance );
		return bTestPassed;
	}


	bool DotTest()
	{
		bool bTestPassed = true;
		ycQuat q1;
		ycQuat q2;

		q1 = ycQuat( 1, 1, 1, 1 );
		q2 = ycQuat( 1, 1, 1, 1 );
		bTestPassed &= q1.Dot( q2 ) == 4.f;

		q1.Normalize();
		q2.Normalize();
		bTestPassed &= q1.Dot( q2 ) == 1.f;

		q1 = ycQuat( 2, 2, 2, 2 );
		q2 = ycQuat( -2, -2, -2, -2 );
		bTestPassed &= q1.Dot( q2 ) == -16.f;

		q1.Normalize();
		q2.Normalize();
		bTestPassed &= q1.Dot( q2 ) == -1.f;

		q1 = ycQuat( -1, 1, -1, 1 );
		q2 = ycQuat( 1, 1, 1, 1 );
		bTestPassed &= q1.Dot( q2 ) == 0.f;

		return bTestPassed;
	}


	bool MagnitudeTests()
	{
		return LengthSqTest() && MagSqTest() && LengthTest() && MagTest();
	}


	bool LengthSqTest()
	{
		bool bTestPassed = true;
		ycQuat q;
		q = ycQuat( 1, 2, 3, 4 );
		bTestPassed &= q.LengthSq() == ( 1*1 + 2*2 + 3*3 + 4*4 );
		q = ycQuat( -1, -1, 1, 1 );
		bTestPassed &= q.LengthSq() == 4.f;
		q = ycQuat( 0, 0, 0, 0 );
		bTestPassed &= q.LengthSq() == 0.f;
		return bTestPassed;
	}


	bool MagSqTest()
	{
		bool bTestPassed = true;
		ycQuat q;
		q = ycQuat( 1, 2, 3, 4 );
		bTestPassed &= q.MagSq() == ( 1*1 + 2*2 + 3*3 + 4*4 );
		q = ycQuat( -1, -1, 1, 1 );
		bTestPassed &= q.MagSq() == 4.f;
		q = ycQuat( 0, 0, 0, 0 );
		bTestPassed &= q.MagSq() == 0.f;
		return bTestPassed;
	}


	bool LengthTest()
	{
		bool bTestPassed = true;
		ycQuat q;
		q = ycQuat( 3, 0, 4, 0 );
		bTestPassed &= q.Length() == 5.f;
		q = ycQuat( 0, 3, 0, 4 );
		bTestPassed &= q.Length() == 5.f;
		q = ycQuat( 0, 0, 0, 0 );
		bTestPassed &= q.Length() == 0.f;
		return bTestPassed;
	}


	bool MagTest()
	{
		bool bTestPassed = true;
		ycQuat q;

		q = ycQuat( 3, 0, 4, 0 );
		bTestPassed &= q.Mag() == 5.f;

		q = ycQuat( 0, 3, 0, 4 );
		bTestPassed &= q.Mag() == 5.f;

		q = ycQuat( 0, 0, 0, 0 );
		bTestPassed &= q.Mag() == 0.f;

		return bTestPassed;
	}


	bool NormalizationTests()
	{
		return NormalizedTest() && NormalizeTest() && IsNormalizedTest();
	}


	bool NormalizeTest()
	{
		bool bTestPassed = true;
		ycQuat q1;

		q1 = ycQuat( 5, 5, 5, 5 );
		q1.Normalize();
		bTestPassed &= ( q1.x==0.5f && q1.y==0.5f && q1.z==0.5f && q1.w==0.5f );

		return bTestPassed;
	}


	bool NormalizedTest()
	{
		bool bTestPassed = true;
		ycQuat q1;
		ycQuat q2;

		q1 = ycQuat( 5, 5, 5, 5 );
		q2 = q1.Normalized();
		bTestPassed &= ( q2.x == 0.5f && q2.y == 0.5f && q2.z == 0.5f && q2.w == 0.5f );

		return bTestPassed;
	}


	bool IsNormalizedTest()
	{
		bool bIsNormalized = true;
		ycQuat q;

		q = ycQuat( 1, 1, 1, 1 );
		bIsNormalized &= q.IsNormalized() == false;

		q = ycQuat( 1, 0, 0, 0 );
		bIsNormalized &= q.IsNormalized() == true;

		return bIsNormalized;
	}


	bool ConjugateTests()
	{
		return GetConjugateTest() && ConjugateTest();
	}


	bool GetConjugateTest()
	{
		bool bTestPassed = true;
		ycQuat q1;
		ycQuat q2;

		q1 = ycQuat( 1, 1, 1, 5 );
		q2 = q1.GetConjugate();
		bTestPassed &= ( q1.x == -q2.x && q1.y == -q2.y && q1.z == -q2.z && q1.w == q2.w );

		return bTestPassed;
	}


	bool ConjugateTest()
	{
		bool bTestPassed = true;
		ycQuat q1;
		ycQuat q2;

		q1 = ycQuat( 1, 1, 1, 5 );
		q2 = ycQuat( 1, 1, 1, 5 );
		q2.Conjugate();
		bTestPassed &= ( q1.x == -q2.x && q1.y == -q2.y && q1.z == -q2.z && q1.w == q2.w );

		return bTestPassed;
	}


	bool InverseTests()
	{
		return GetInverseTest() && InvertTest();
	}


	bool GetInverseTest()
	{
		bool bTestPassed = true;

		ycQuat q1 = ycQuat( 1, 1, 1, 5 );
		q1.Normalize();
		ycQuat q2 = q1.GetInverse();
		bTestPassed &= ( q1.x == -q2.x && q1.y == -q2.y && q1.z == -q2.z && q1.w == q2.w );

		return bTestPassed;
	}


	bool InvertTest()
	{
		bool bTestPassed = true;

		ycQuat q1 = ycQuat( 1, 1, 1, 5 );
		q1.Normalize();
		ycQuat q2 = ycQuat( 1, 1, 1, 5 );
		q2.Normalize();
		q2.Invert();
		bTestPassed &= ( q1.x==q2.x && q1.y==q2.y && q1.z==q2.z && q1.w==-q2.w );
		bTestPassed |= ( q1.x==-q2.x && q1.y==-q2.y && q1.z==-q2.z && q1.w==q2.w );

		return bTestPassed;
	}


	bool VectorRotationTests()
	{
		bool bTestPassed = true;

		bTestPassed &= GetRotatedVec3Test();
		bTestPassed &= GetUnRotatedVec3Test();

		return bTestPassed;
	}


	bool GetRotatedVec3Test()
	{
		bool bTestPassed = true;
		f32 testTolerance = 0.000001f;
		ycVec3 p;
		ycVec3 pExpected;
		ycQuat q;
		f32 angle;
		ycVec3 axis;

		p = ycVec3::ZAXIS();
		angle = YC_PI_OVER_2;
		axis = ycVec3( ycVec3::YAXIS() );
		q = ycQuat( axis, angle );
		pExpected = ycVec3( ycVec3::XAXIS() );
		p = q.Rotate( p );
		bTestPassed &= WithinTolerance( pExpected.x, p.x, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.y, p.y, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.z, p.z, testTolerance );

		p = ycVec3::ZAXIS();
		angle = -YC_PI_OVER_2;
		axis = ycVec3( ycVec3::YAXIS() );
		q = ycQuat( axis, angle );
		pExpected = ycVec3( -ycVec3::XAXIS() );
		p = q.Rotate( p );
		bTestPassed &= WithinTolerance( pExpected.x, p.x, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.y, p.y, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.z, p.z, testTolerance );

		p = ycVec3::ZAXIS();
		angle = YC_PI_OVER_2;
		axis = -ycVec3( ycVec3::YAXIS() );
		q = ycQuat( axis, angle );
		pExpected = ycVec3( -ycVec3::XAXIS() );
		p = q.Rotate( p );
		bTestPassed &= WithinTolerance( pExpected.x, p.x, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.y, p.y, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.z, p.z, testTolerance );

		return bTestPassed;
	}


	bool GetUnRotatedVec3Test()
	{
		bool bTestPassed = true;
		f32 testTolerance = 0.000001f;
		ycVec3 p;
		ycVec3 pExpected;
		ycQuat q;
		f32 angle;
		ycVec3 axis;

		p = ycVec3::ZAXIS();
		angle = YC_PI_OVER_2;
		axis = ycVec3( ycVec3::YAXIS() );
		q = ycQuat( axis, angle);
		pExpected = ycVec3( -ycVec3::XAXIS() );
		p = q.RotateInverse( p);
		bTestPassed &= WithinTolerance( pExpected.x, p.x, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.y, p.y, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.z, p.z, testTolerance );

		p = ycVec3::ZAXIS();
		angle = -YC_PI_OVER_2;
		axis = ycVec3( ycVec3::YAXIS() );
		q = ycQuat( axis, angle );
		pExpected = ycVec3( ycVec3::XAXIS() );
		p = q.RotateInverse( p );
		bTestPassed &= WithinTolerance( pExpected.x, p.x, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.y, p.y, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.z, p.z, testTolerance );

		p = ycVec3::ZAXIS();
		angle = YC_PI_OVER_2;
		axis = -ycVec3( ycVec3::YAXIS() );
		q = ycQuat( axis, angle );
		pExpected = ycVec3( ycVec3::XAXIS() );
		p = q.RotateInverse( p );
		bTestPassed &= WithinTolerance( pExpected.x, p.x, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.y, p.y, testTolerance );
		bTestPassed &= WithinTolerance( pExpected.z, p.z, testTolerance );

		return bTestPassed;
	}


	bool InterpolationTests()
	{
		bool bTestPassed = true;

		bTestPassed &= GetSlerpTest();
		bTestPassed &= GetLerpTest();

		return bTestPassed;
	}


	bool GetSlerpTest()
	{
		bool bTestPassed = true;
		f32 tolerance = 0.0000001f;
		ycQuat q1;
		ycQuat q2;
		f32 t;
		ycQuat expected;
		ycQuat actual;

		q1 = ycQuat( ycVec3::UP(), YC_PI_OVER_2 / 2 );
		q2 = ycQuat( ycVec3( 0.0f, 0.0f, 1.0f ), YC_PI_OVER_2 );
		q1.Normalize();
		q2.Normalize();

		t = 0;
		expected = q1;
		actual = q1.GetSlerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		t = 1;
		expected = q2;
		actual = q1.GetSlerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		t = 0.5;
		expected = ycQuat( 0, 0.21045114147975094f, 0.38886300669129803f, 0.896936959610094f );
		actual = q1.GetSlerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		return bTestPassed;
	}


	bool GetLerpTest()
	{
		bool bTestPassed = true;
		ycQuat q1;
		ycQuat q2;
		f32 t;
		ycQuat expected;
		ycQuat actual;
		f32 tolerance = 0.0000001f;

		q1 = ycQuat( -1, -1, -1, -1 );
		q2 = ycQuat( 1, 1, 1, 1 );

		t = 0.5f;
		expected = ycQuat( 0.f, 0.f, 0.f, 0.f );
		actual = q1.GetLerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		t = -1;
		expected = ycQuat( -0.5f, -0.5f, -0.5f, -0.5f );
		actual = q1.GetLerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		t = 2;
		expected = ycQuat( 0.5f, 0.5f, 0.5f, 0.5f );
		actual = q1.GetLerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		t = 0.5f;
		q1 = ycQuat( 0.5f, 0.5f, 0.5f, 0.5f );
		expected = q1;
		actual = q1.GetLerp( q2, t );
		bTestPassed &= WithinTolerance( expected, actual, tolerance );

		return bTestPassed;
	}

	bool MatrixConversionTests()
	{
		for( int n = 0; n<100; n++ )
		{
			ycQuat q0 = GetRandomQuat().Normalized();
			ycVec3 v0 = GetRandomVec3( -100.0f, 100.0f );

			// Convert to matrix and back.
			ycMtx4 mtx = q0.GetAsMtx4();
			ycQuat q1 = ycQuat::GetQuat( mtx );

			// Rotate with both quat and matrix and make sure the result is the same.
			ycVec3 v1 = mtx.TransformDir( v0 );
			ycVec3 v2 = q0.Rotate( v0 );
			ycVec3 v3 = q1.Rotate( v0 );

			if( !WithinToleranceDual( q0, q1, 0.01f ) )
				return false;
			if( !WithinTolerance( v1, v2, 0.01f ) )
				return false;
			if( !WithinTolerance( v1, v3, 0.01f ) )
				return false;
		}

		return true;
	}

	bool TestEulerAngle( ycVec3 euler )
	{
		ycVec3 v0 = GetRandomVec3( -100.0f, 100.0f );

		// Compose our own and check it's right.
		ycQuat qX = ycQuat::GetQuat( ycVec3::XAXIS(), euler.x );
		ycQuat qY = ycQuat::GetQuat( ycVec3::YAXIS(), euler.y );
		ycQuat qZ = ycQuat::GetQuat( ycVec3::ZAXIS(), euler.z );
		ycQuat q0 = qY * ( qX * qZ );

		// Get matrix form.
		ycMtx4 m = ycMtx4::IDENTITY();
		m.RotFromEuler( euler );

		// Get quaternion form.
		ycQuat q = ycQuat::FromEuler( euler );
		ycMtx4 qm = q.GetAsMtx4();
		if( !WithinTolerance( m.x, qm.x, 0.01f )
			 || !WithinTolerance( m.y, qm.y, 0.01f )
			 || !WithinTolerance( m.z, qm.z, 0.01f ) )
			return false;

		// Check the quaternion matches what we expect.
		if( !WithinToleranceDual( q0, q, 0.01f ) )
			return false;

		// Check matrix and quaternion results concur.
		ycVec3 v1 = m.TransformDir( v0 );
		ycVec3 v2 = q.Rotate( v0 );
		if( !WithinTolerance( v1, v2, 0.01f ) )
			return false;

		// Check we can get the original angles back out.
		ycVec3 meuler = m.ToEuler();
		ycVec3 qeuler = q.ToEuler();
		if( !WithinTolerance( euler, meuler, 0.01f ) )
			return false;
		if( !WithinTolerance( euler, qeuler, 0.01f ) )
			return false;

		return true;
	}

	bool EulerAnglesTests()
	{
		// Test primary axes. (treated specially as singularities exist)
		for( int z = -360; z<=360; z += 45 )
		{
			for( int y = -360; y<=360; y += 45 )
			{
				for( int x = -360; x<=360; x += 45 )
				{
					ycVec3 euler = ycVec3( (f32)x, (f32)y, (f32)z ) * YC_RADIANS_PER_DEGREE;
					ycQuat q = ycQuat::FromEuler( euler );
					ycMtx4 m = ycMtx4::IDENTITY();
					m.RotFromEuler( euler );

					// Test matrix and quaternion match.
					ycVec3 v0 = GetRandomVec3( -100.0f, 100.0f );
					ycVec3 v1 = q.Rotate( v0 );
					ycVec3 v2 = m.TransformDir( v0 );
					if( !WithinTolerance( v1, v2, 0.01f ) )
						return false;

					// Test we go to/from okay.
					ycVec3 neweuler = q.ToEuler(); // <-- not guaranteed to match original
					ycQuat q2 = ycQuat::FromEuler( neweuler ); // <-- but this should.
					if( !WithinToleranceDual( q, q2, 0.01f ) )
						return false;
				}
			}
		}

		// Test random angles.
		for( int n = 0; n<100; n++ )
		{
			ycVec3 euler = GetRandomVec3( -1.0f, 1.0f );
			if( !TestEulerAngle( euler ) )
				return false;
		}

		return true;
	}

	/*
	bool FormatConversionTests()
	{
		bool bTestPassed = true;

		bTestPassed &= GetAxisTest();
		bTestPassed &= GetAngleTest();

		return bTestPassed;
	}
*/

	bool GetAxisTest()
	{
		bool bTestPassed = true;
		f32 angle;
		ycVec3 axis;
		ycVec3 qAxis;
		ycQuat q;

		angle = YC_PI_OVER_2;
		axis = ycVec3( 1, 1, 1 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		qAxis = q.GetAxis();
		bTestPassed &= ( axis.x == qAxis.x && axis.y == qAxis.y && axis.z == qAxis.z );

		angle = YC_PI_OVER_2 / 2.f;
		axis = ycVec3( -1, 3, 2 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		qAxis = q.GetAxis();
		bTestPassed &= ( axis.x == qAxis.x && axis.y == qAxis.y && axis.z == qAxis.z );

		angle = YC_PI_OVER_2 * 2;
		axis = ycVec3( -10, 4, 9 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		qAxis = q.GetAxis();
		bTestPassed &= ( axis.x == qAxis.x && axis.y == qAxis.y && axis.z == qAxis.z );

		return bTestPassed;
	}


	bool GetAngleTest()
	{
		bool bTestPassed = true;
		f32 angle;
		ycVec3 axis;
		f32 qAngle;
		ycQuat q;

		angle = YC_PI_OVER_2;
		axis = ycVec3( 1, 1, 1 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		qAngle = q.GetAngle();
		bTestPassed &= qAngle == angle;

		angle = YC_PI_OVER_2 / 2.f;
		axis = ycVec3( -1, 3, 2 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		qAngle = q.GetAngle();
		bTestPassed &= qAngle == angle;

		angle = YC_PI_OVER_2 * 2;
		axis = ycVec3( -10, 4, 9 );
		axis.Normalize();
		q = ycQuat( axis, angle );
		qAngle = q.GetAngle();
		bTestPassed &= qAngle == angle;

		return bTestPassed;
	}

/*
	bool QuatConcatTest()
	{
		// test by apply rotation from q1 and q2 to vect1 and comparing the value to
		// a rotation on vect2 with concated quats, and comparing vect1 and vect2
		bool bTestPassed = true;
		f32 tolerance = 0.000005f;
		ycQuat q1, q2, concatQuat;
		ycVec3 vect, vec2Rots, vecConcatRot;

		q1 = GetRandomQuat();
		q1.Normalize();
		q2 = GetRandomQuat();
		q2.Normalize();
		concatQuat = q2 * q1;
		concatQuat.Normalize();
		vect = GetRandomVec3( -10, 10 );
		vec2Rots = q2 * ( q1 * vect );
		vecConcatRot = concatQuat * vect;
		bTestPassed &= WithinTolerance( vec2Rots, vecConcatRot, tolerance );

		q1 = GetRandomQuat();
		q1.Normalize();
		q2 = GetRandomQuat();
		q2.Normalize();
		concatQuat = q2 * q1;
		concatQuat.Normalize();
		vect = GetRandomVec3( -10, 10 );
		vec2Rots = q2 * ( q1 * vect );
		vecConcatRot = concatQuat * vect;
		bTestPassed &= WithinTolerance( vec2Rots, vecConcatRot, tolerance );

		q1 = GetRandomQuat();
		q1.Normalize();
		q2 = GetRandomQuat();
		q2.Normalize();
		concatQuat = q2 * q1;
		concatQuat.Normalize();
		vect = GetRandomVec3( -10, 10 );
		vec2Rots = q2 * ( q1 * vect );
		vecConcatRot = concatQuat * vect;
		bTestPassed &= WithinTolerance( vec2Rots, vecConcatRot, tolerance );

		return bTestPassed;
	}*/


	bool WithinTolerance( const f32 expected, const f32 actual, const f32 tolerance )
	{
		return ( actual > ( expected-tolerance ) && actual < ( expected+tolerance ) );
	}


	bool WithinTolerance( const ycQuat& expected, const ycQuat& actual, const f32 tolerance )
	{
		bool bWithinTolerance = true;

		bWithinTolerance &= WithinTolerance( expected.x, actual.x, tolerance );
		bWithinTolerance &= WithinTolerance( expected.y, actual.y, tolerance );
		bWithinTolerance &= WithinTolerance( expected.z, actual.z, tolerance );
		bWithinTolerance &= WithinTolerance( expected.w, actual.w, tolerance );

		return bWithinTolerance;
	}

	bool WithinToleranceDual( const ycQuat& expected, const ycQuat& actual, const f32 tolerance )
	{
		// Quaternions can still be equal if they are negated.
		bool bWithinTolerance = false;
		bWithinTolerance |= WithinTolerance( expected, actual, tolerance );
		bWithinTolerance |= WithinTolerance( expected, -actual, tolerance );
		return bWithinTolerance;
	}


	bool WithinTolerance(const ycVec3& expected, const ycVec3& actual, const f32 tolerance )
	{
		bool bWithinTolerance = true;

		bWithinTolerance &= WithinTolerance( expected.x, actual.x, tolerance );
		bWithinTolerance &= WithinTolerance( expected.y, actual.y, tolerance );
		bWithinTolerance &= WithinTolerance( expected.z, actual.z, tolerance );

		return bWithinTolerance;
	}

	bool WithinTolerance(const ycVec4& expected, const ycVec4& actual, const f32 tolerance )
	{
		bool bWithinTolerance = true;

		bWithinTolerance &= WithinTolerance( expected.x, actual.x, tolerance );
		bWithinTolerance &= WithinTolerance( expected.y, actual.y, tolerance );
		bWithinTolerance &= WithinTolerance( expected.z, actual.z, tolerance );
		bWithinTolerance &= WithinTolerance( expected.w, actual.w, tolerance );

		return bWithinTolerance;
	}
}

namespace
{
	void GetPrecisionImpactConjugate( s32 numCycles );
	void GetPrecisionImpcactInverse( s32 numCycles );
	void GetPrecisionImpactNormalize( s32 numCycles );
	void GetPrecisionImpactRotation( s32 numCycles );
	void GetPrecisionImpactAxisAngle( s32 numCycles );
	void GetPrecisionImpactMtx4( s32 numCycles );
	void ReportPrecision( const ycQuat& original, const ycQuat& postTest, const s32 numCycles, const char* testName );
	void ReportPrecision( const ycVec3& original, const ycVec3& postTest, const s32 numCycles, const char* testName );
	float GetEpsilonNeededAxisAngleCycle( s32 numTests, s32 cyclesPerTest );
	void  GetAverageErrorRotationCycles( s32 numTests, s32 cyclesPerTest, ycVec3* result );
}


YC_BEGIN_TEST( ycQuat_Precision )
{
	s32 numCycles = 1000;
	GetPrecisionImpactConjugate( numCycles );
	GetPrecisionImpcactInverse( numCycles );
	GetPrecisionImpactNormalize( numCycles );
	GetPrecisionImpactRotation( numCycles );
	GetPrecisionImpactAxisAngle( numCycles );
	GetPrecisionImpactMtx4( numCycles );
	YC_TEST_PASS( "ycQuat Precision Impact Tests Complete" );

	s32 numTests = 1000;
	ycVec3 result;
	GetAverageErrorRotationCycles( numTests, numCycles, &result );
}


namespace
{
	ycRand rng(123456);


	void GetPrecisionImpactConjugate( s32 numCycles )
	{
		ycQuat initialQuat = GetRandomQuat();
		ycQuat guineaPig = initialQuat;
		for( sreg i = 0; i < numCycles; ++i )
		{
			guineaPig.Conjugate();
			guineaPig.Conjugate();
		}
		ReportPrecision( initialQuat, guineaPig, numCycles, "Quat-Conjugate" );
	}


	void GetPrecisionImpcactInverse( s32 numCycles )
	{
		ycQuat initialQuat = GetRandomQuat();
		initialQuat.Normalize();
		ycQuat guineaPig = initialQuat;
		initialQuat.Normalize();
		for( sreg i = 0; i < numCycles; ++i )
		{
			guineaPig.Invert();
			guineaPig.Invert();
		}
		ReportPrecision( initialQuat, guineaPig, numCycles, "Quat-Inverse" );
	}


	void GetPrecisionImpactNormalize( s32 numCycles )
	{
		ycQuat initialQuat = GetRandomQuat();
		initialQuat.Normalize();
		ycQuat guineaPig = initialQuat;
		for( sreg i = 0; i < numCycles; ++i )
		{
			guineaPig.Normalize();
		}
		ReportPrecision( initialQuat, guineaPig, numCycles, "Quat-Normalize" );
	}


	void GetPrecisionImpactRotation( s32 numCycles )
	{
		ycQuat quat = GetRandomQuat();
		quat.Normalize();
		ycVec3 initialVec = GetRandomVec3( -20, 20 );
		ycVec3 guineaPig = initialVec;
		for( sreg i = 0; i < numCycles; ++i )
		{
			guineaPig = quat.Rotate( guineaPig );
			guineaPig = quat.RotateInverse( guineaPig );
		}
		ReportPrecision( initialVec, guineaPig, numCycles, "Quat-Rotation" );
	}


	void GetPrecisionImpactAxisAngle( s32 numCycles )
	{
		ycQuat initialQuat = GetRandomQuat();
		initialQuat.Normalize();
		ycQuat guineaPig = initialQuat;
		ycVec3 axis;
		f32 angle;
		for( sreg i = 0; i < numCycles; ++i )
		{
			axis = guineaPig.GetAxis();
			axis.Normalize();
			angle = guineaPig.GetAngle();
			guineaPig = ycQuat::GetQuat( axis, angle );
		}
		ReportPrecision( initialQuat, guineaPig, numCycles, "Quat->AxisAngle->Quat" );
	}


	void GetPrecisionImpactMtx4( s32 numCycles )
	{
		ycQuat initialQuat = GetRandomQuat();
		initialQuat.Normalize();
		ycQuat guineaPig = initialQuat;
		ycMtx4 mat;
		for( sreg i = 0; i < numCycles; ++i )
		{
			mat = guineaPig.GetAsMtx4();
			guineaPig = ycQuat::GetQuat( mat );
		}
		ReportPrecision( initialQuat, guineaPig, numCycles, "Quat->Mtx4" );
	}

/*
	float GetEpsilonNeededAxisAngleCycle( s32 numTests, s32 cyclesPerTest )
	{
		float maxToleranceNeeded = 0.f;
		for( sreg iTest = 0; iTest < numTests; ++iTest )
		{
			ycQuat initialQuat = GetRandomQuat();
			initialQuat.Normalize();
			ycQuat guineaPig = initialQuat;
			ycVec3 axis;
			f32 angle;
			for( sreg iCycle = 0; iCycle < cyclesPerTest; ++iCycle )
			{
				axis = guineaPig.GetAxis();
				axis.Normalize();
				angle = guineaPig.GetAngle();
				guineaPig = ycQuat::GetQuat( axis, angle );
			}
			float toleranceNeededForCycle = ycAbs( 1.f - guineaPig.Mag() );
			if( toleranceNeededForCycle > maxToleranceNeeded )
			{
				maxToleranceNeeded = toleranceNeededForCycle;
			}
		}
		return maxToleranceNeeded;
	}
*/


	void  GetAverageErrorRotationCycles( s32 numTests, s32 cyclesPerTest, ycVec3* result )
	{
		ycVec3 averageError = ycVec3( 0, 0, 0 );
		for( sreg iTest = 0; iTest < numTests; ++iTest )
		{
			ycQuat quat = GetRandomQuat();
			quat.Normalize();
			ycVec3 initialVec = GetRandomVec3( -20, 20 );
			ycVec3 guineaPig = initialVec;
			for( sreg i = 0; i < cyclesPerTest; ++i )
			{
				guineaPig = quat.Rotate( guineaPig );
				guineaPig = quat.RotateInverse( guineaPig );
			}
			s32 deltaX = ycFloatAsIntBits( guineaPig.x ) - ycFloatAsIntBits( initialVec.x );
			s32 deltaY = ycFloatAsIntBits( guineaPig.y ) - ycFloatAsIntBits( initialVec.y );
			s32 deltaZ = ycFloatAsIntBits( guineaPig.z ) - ycFloatAsIntBits( initialVec.z );
			averageError.x += ycAbs( deltaX );
			averageError.y += ycAbs( deltaY );
			averageError.z += ycAbs( deltaZ );
		}
		averageError /= f32( numTests );
		( *result ) = averageError;
	}


	void ReportPrecision( const ycQuat& original, const ycQuat& postTest, const s32 numCycles, const char * testName )
	{
#ifndef YC_ENABLE_LOG
		YC_UNUSED( original );
		YC_UNUSED( postTest );
		YC_UNUSED( numCycles );
		YC_UNUSED( testName );
#else
		const ycQuat orig = original.GetInCanonicalForm();
		const ycQuat post = postTest.GetInCanonicalForm();
		s32 deltaX = ycFloatAsIntBits( post.x ) - ycFloatAsIntBits( orig.x );
		s32 deltaY = ycFloatAsIntBits( post.y ) - ycFloatAsIntBits( orig.y );
		s32 deltaZ = ycFloatAsIntBits( post.z ) - ycFloatAsIntBits( orig.z );
		s32 deltaW = ycFloatAsIntBits( post.w ) - ycFloatAsIntBits( orig.w );
		YC_TEST_PASS( "Steps Of Imprecision for %s for %d cycles \n\tdeltaX=%d\n\tdeltaY=%d\n\tdeltaZ=%d\n\tdeltaW=%d", testName, numCycles, deltaX, deltaY, deltaZ, deltaW );
#endif
	}


	void ReportPrecision( const ycVec3& original, const ycVec3& postTest, const s32 numCycles, const char * testName )
	{
#ifndef YC_ENABLE_LOG
		YC_UNUSED( original );
		YC_UNUSED( postTest );
		YC_UNUSED( numCycles );
		YC_UNUSED( testName );
#else
		s32 deltaX = ycFloatAsIntBits( postTest.x ) - ycFloatAsIntBits( original.x );
		s32 deltaY = ycFloatAsIntBits( postTest.y ) - ycFloatAsIntBits( original.y );
		s32 deltaZ = ycFloatAsIntBits( postTest.z ) - ycFloatAsIntBits( original.z );
		YC_TEST_PASS( "Steps Of Imprecision for %s for %d cycles \n\tdeltaX=%d\n\tdeltaY=%d\n\tdeltaZ=%d", testName, numCycles, deltaX, deltaY, deltaZ );
#endif
	}


	ycVec3 GetRandomVec3( const f32 min, const f32 max )
	{
		ycVec3 randVec;
		f32 t;

		t = rng.RandF();
		randVec.x = ( 1-t )*min + t*max;

		t = rng.RandF();
		randVec.y = ( 1-t )*min + t*max;

		t = rng.RandF();
		randVec.z = ( 1-t )*min + t*max;

		return randVec;
	}


	/*static*/ ycQuat GetRandomQuat()
	{
		ycQuat randomQuat;

		randomQuat.w = ( rng.RandF() - 0.5f ) * 2.f;
		randomQuat.x = ( rng.RandF() - 0.5f ) * 2.f;
		randomQuat.y = ( rng.RandF() - 0.5f ) * 2.f;
		randomQuat.z = ( rng.RandF() - 0.5f ) * 2.f;

		return randomQuat;
	}


}//end: anon. namespace

#endif//YC_TEST
