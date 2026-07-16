#include "ycCollisionMtx3.h"

// multiplies with own transpose
void ycCollisionMtx3::MulWithTranspose()
{
	f32 m00 = x.Dot( x );
	f32 m11 = y.Dot( y );
	f32 m22 = z.Dot( z );

	f32 m01 = x.Dot( y );
	f32 m02 = x.Dot( z );
	f32 m12 = y.Dot( z );

	m[ 0 ][ 0 ] = m00;
	m[ 1 ][ 1 ] = m11;
	m[ 2 ][ 2 ] = m22;

	m[ 0 ][ 1 ] = m[ 1 ][ 0 ] = m01;
	m[ 0 ][ 2 ] = m[ 2 ][ 0 ] = m02;
	m[ 1 ][ 2 ] = m[ 2 ][ 1 ] = m12;
}

// returns true if matrix has no shear and uniform scale
bool ycCollisionMtx3::IsOrthoScalar( f32* scale )
{
	ycCollisionMtx3 trn = *this;
	trn.MulWithTranspose();

	if( ycAbs( trn.m[ 0 ][ 0 ] - trn.m[ 1 ][ 1 ] ) > 0.00001f ) { return false; }
	if( ycAbs( trn.m[ 0 ][ 0 ] - trn.m[ 2 ][ 2 ] ) > 0.00001f ) { return false; }
	if( ycAbs( trn.m[ 1 ][ 1 ] - trn.m[ 2 ][ 2 ] ) > 0.00001f ) { return false; }

	if( ycAbs( trn.m[ 0 ][ 1 ] ) > 0.00001f ) { return false; }
	if( ycAbs( trn.m[ 0 ][ 2 ] ) > 0.00001f ) { return false; }
	if( ycAbs( trn.m[ 1 ][ 2 ] ) > 0.00001f ) { return false; }

	if( scale ) { *scale = ycSqrt( trn.m[ 0 ][ 0 ] ); }
	return true;
}
