#include "ycMtx4.h"
#include "ycPlane.h"

#include "ycPushDebugOpt.inc"
#include "ycDataSchema.h"

#ifndef YC_CODEGEN
void ycDataMetaStartup()
{
	ycData::Schema::SetConstructor( ycData::GetRecordDef< ycMtx4 >(), []( void* ptr ) {
		ycMtx4* q = ( ycMtx4* )ptr;
		q->SetIdentity();
	} );
}
#endif // !YC_CODEGEN

ycVec3 ycMtx4::ToEuler() const
{
	ycVec3 euler;

	// Remove any scale component.
	ycVec3 xn = GetAxis( 0 ).Normalized();
	ycVec3 yn = GetAxis( 1 ).Normalized();
	ycVec3 zn = GetAxis( 2 ).Normalized();

	euler.x = ycArcSin( -zn.y );      // pitch
	if( ycAbs( ycCos( euler.x ) ) < 0.001f )  // handle gimbal lock
	{
		euler.y = 0.0f;
		euler.z = ycArcTan2( -yn.x, xn.x );
	}
	else
	{
		euler.y = ycArcTan2( zn.x, zn.z ); // yaw
		euler.z = ycArcTan2( xn.y, yn.y ); // roll
	}

	return euler;
}

void ycMtx4::RotFromEuler( const ycVec3 rot )
{
	f32 sp = ycSin( rot.x ); // pitch
	f32 cp = ycCos( rot.x ); // pitch
	f32 sy = ycSin( rot.y ); // yaw
	f32 cy = ycCos( rot.y ); // yaw
	f32 sr = ycSin( rot.z ); // roll
	f32 cr = ycCos( rot.z ); // roll

	m[0][0] = cy*cr+sy*sp*sr;
	m[0][1] = cp*sr;
	m[0][2] = sr*cy*sp-sy*cr;
	m[0][3] = 0.0f;
	m[1][0] = cr*sy*sp-sr*cy;
	m[1][1] = cr*cp;
	m[1][2] = sy*sr+cr*cy*sp;
	m[1][3] = 0.0f;
	m[2][0] = cp*sy;
	m[2][1] = -sp;
	m[2][2] = cp*cy;
	m[2][3] = 0.0f;
}

// rotate around vertical and horizontal axes without introducing roll
void ycMtx4::RotVertHoriz( f32 vertRot, f32 horizRot )
{
	f32 cv = ycCos( vertRot ), sv = ycSin( vertRot );
	f32 ch = ycCos( horizRot ), sh = ycSin( horizRot );

	f32 b = m[0][0]*cv + m[0][2]*sv;
	f32 c = m[0][2]*cv - m[0][0]*sv;
	f32 d = (m[1][0]*cv + m[1][2]*sv)*ch + (m[2][0]*cv + m[2][2]*sv)*sh;
	f32 e = m[1][1]*ch + m[2][1]*sh;
	f32 f = (m[1][2]*cv - m[1][0]*sv)*ch + (m[2][2]*cv - m[2][0]*sv)*sh;
	f32 g = (m[2][0]*cv + m[2][2]*sv)*ch - (m[1][0]*cv + m[1][2]*sv)*sh;
	f32 h = m[2][1]*ch - m[1][1]*sh;
	f32 i = (m[2][2]*cv - m[2][0]*sv)*ch - (m[1][2]*cv - m[1][0]*sv)*sh;

	a[0] = b;
	a[2] = c;
	a[4] = d;
	a[5] = e;
	a[6] = f;
	a[8] = g;
	a[9] = h;
	a[10] = i;
}

ycVec3 ycMtx4::ToScale3() const
{
	return ycVec3( x.xyz().Length(), y.xyz().Length(), z.xyz().Length() );
}

void ycMtx4::ApplyScale3( const ycVec3 scale )
{
	m[0][0] *= scale.x;
	m[0][1] *= scale.x;
	m[0][2] *= scale.x;
	m[1][0] *= scale.y;
	m[1][1] *= scale.y;
	m[1][2] *= scale.y;
	m[2][0] *= scale.z;
	m[2][1] *= scale.z;
	m[2][2] *= scale.z;
}

ycMtx4 ycMtx4::ExtractRotation() const
{
	// Zero out translation.
	ycMtx4 mtxR = *this;
	mtxR.t = ycVec4::WAXIS();

	// Extract out scale
	const ycVec3 mtxS = mtxR.ToScale3();
	mtxR.ApplyScale3( ycVec3::ONE() / mtxS );

	return mtxR;
}

// From Lengyel - Foundations of Game Development Vol. 1: Mathematics, 3.4.3.
// Note that his plane definition has d negated compared to ours, hence the
// difference in the matrix t component.
void ycMtx4::MakePlaneReflection( const ycPlane& plane )
{
	const ycVec3& n = plane.normal;
	const f32 d = plane.d;
	x = ycVec4( 1.0f - 2.0f * n.x * n.x, -2.0f * n.x * n.y, -2.0f * n.x * n.z, 0.0f );
	y = ycVec4( -2.0f * n.x * n.y, 1.0f - 2.0f * n.y * n.y, -2.0f * n.y * n.z, 0.0f );
	z = ycVec4( -2.0f * n.x * n.z, -2.0f * n.y * n.z, 1.0f - 2.0f * n.z * n.z, 0.0f );
	t = ycVec4( 2.0f * n.x * d, 2.0f * n.y * d, 2.0f * n.z * d, 1.0f );
}

void ycMtx4::FromScaleRotPos( const ycVec3& scale, const ycVec3& euler, const ycVec3& pos )
{
	RotFromEuler( euler );
	a[0] *= scale.x;
	a[1] *= scale.x;
	a[2] *= scale.x;
	a[3] = 0;
	a[4] *= scale.y;
	a[5] *= scale.y;
	a[6] *= scale.y;
	a[7] = 0;
	a[8] *= scale.z;
	a[9] *= scale.z;
	a[10] *= scale.z;
	a[11] = 0;
	a[12] = pos.x;
	a[13] = pos.y;
	a[14] = pos.z;
	a[15] = 1;
}

void ycMtx4::SetAxis( ureg axis, ycVec3 value )
{
	m[axis][0] = value.x;
	m[axis][1] = value.y;
	m[axis][2] = value.z;
}

ycVec3 ycMtx4::GetAxis( ureg axis ) const
{
	return ycVec3( m[axis][0], m[axis][1], m[axis][2] );
}

#include "ycPopDebugOpt.inc"

