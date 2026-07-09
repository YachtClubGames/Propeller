#pragma once

/*! \struct ycMtx4
Warning: the default ctor does nothing! if you need identity, you must do it manually
*/

#include "ycMath.h"
#include "ycVec3.h"
#include "ycVec4.h"
#include "ycDepthConfig.h"

struct ycPlane;

struct ycMtx4 //%
{
yceditable:

	//! the data!
	union
	{
		struct { ycVec4 x, y, z, t; };
		f32 a[ 16 ];
		f32 m[ 4 ][ 4 ];
	};

public:

	//! default ctor
	ycMtx4() = default;

	//! basis vector ctor
	constexpr inline explicit ycMtx4( const ycVec4& _x, const ycVec4& _y, const ycVec4& _z, const ycVec4& _t );
	constexpr inline explicit ycMtx4( const ycVec3& _x, const ycVec3& _y, const ycVec3& _z, const ycVec3& _t );

	//! all the values!
	constexpr inline explicit ycMtx4(
		const f32 _00, const f32 _01, const f32 _02, const f32 _03,
		const f32 _10, const f32 _11, const f32 _12, const f32 _13,
		const f32 _20, const f32 _21, const f32 _22, const f32 _23,
		const f32 _30, const f32 _31, const f32 _32, const f32 _33
	);

	//! matrix generation utilities
	inline void SetIdentity();
	inline void OrthoLH( const f32 width, const f32 height, const f32 zNear, const f32 zFar );
	inline void OrthoRH( const f32 width, const f32 height, const f32 zNear, const f32 zFar );
	inline void PerspectiveFovLH( const f32 fovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar );
	inline void PerspectiveFovRH( const f32 fovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar );
	inline void PerspectiveTanFovLH( const f32 tanFovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar );
	inline void PerspectiveTanFovRH( const f32 tanFovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar );
	inline void LookAtLH( const ycVec3& eye, const ycVec3& target, const ycVec3& up );
	inline void LookAtRH( const ycVec3& eye, const ycVec3& target, const ycVec3& up );

	//! vector transformation
	inline ycVec4 Transform( const ycVec4 rhs ) const; //!< no w divide
	inline ycVec3 TransformProj( const ycVec4 rhs ) const; //!< performs w divide

	inline ycVec3 TransformPos( const ycVec3& local ) const; //!< transforms a point to the space
	inline ycVec3 TransformDir( const ycVec3& local ) const; //!< transforms a point to the space
	inline ycVec3 TransformPosProj( const ycVec3& local ) const; //!< transforms a point to the space, performs w divide
	inline ycVec3 TransformDirProj( const ycVec3& local ) const; //!< transforms a point to the space, performs w divide

	inline void Transpose();
	inline ycMtx4 Transposed() const;

	inline void Invert();
	inline ycMtx4 Inverted() const;

	inline void RigidInvert();
	inline ycMtx4 RigidInverted() const;

	inline void ReverseDepth( bool shouldReverse = ycDepthConfig::IS_DEPTH_REVERSED ); //!< reverses the depth remap of a matrix. don't call this unless you know what you're doing!

	//! operators
	inline ycMtx4  operator +  ( const ycMtx4& rhs ) const;
	inline ycMtx4  operator -  ( const ycMtx4& rhs ) const;
	inline ycMtx4  operator *  ( const ycMtx4& rhs ) const;
	inline ycVec4  operator *  ( const ycVec4& rhs ) const;
	inline ycVec3  operator *  ( const ycVec3& rhs ) const; //!< only uses the upper 3x3
	inline ycMtx4  operator *  ( const f32   rhs ) const;
	inline ycMtx4& operator += ( const ycMtx4& rhs );
	inline ycMtx4& operator -= ( const ycMtx4& rhs );
	inline ycMtx4& operator *= ( const ycMtx4& rhs );
	inline ycMtx4& operator *= ( const f32   rhs );

	//! Approx-equality checks
	inline bool IsIdentity( f32 tolerance = 1e-4f ) const;
	inline bool IsZero( f32 tolerance = 1e-4f ) const;

	//! Min / Max
	inline ycVec4 Min();
	inline ycVec4 Max();

	//! Gets forward dir axis (z), Normalizes output
	inline ycVec3 GetForwardAxis() const;
	//! Gets up dir axis (y), Normalizes output
	inline ycVec3 GetUpAxis() const;
	//! Gets right dir axis (x), Normalizes output
	inline ycVec3 GetRightAxis() const;

	//! Eulering
	//! Euler angles are represented as counter-clockwise rotations (when in RH coordinates)
	//! about each axis in radians. [x,y,z] = [pitch,yaw,roll]
	//! Operations are concatenated in the order Roll, Pitch, then finally Yaw (to match
	//! the standard aeroplane model)
	ycVec3 ToEuler() const;
	void RotFromEuler( const ycVec3 rot );

	void RotVertHoriz( f32 vertRot, f32 horizRot );

	//! Scaling
	ycVec3 ToScale3() const;
	void ApplyScale3( const ycVec3 scale );

	//! Remove scaling
	ycMtx4 ExtractRotation() const;

	//! Reflection
	void MakePlaneReflection( const ycPlane& plane );

	//! Entity Data Transform (note euler is in radians)
	void FromScaleRotPos( const ycVec3& scale, const ycVec3& euler, const ycVec3& pos );

	//! Vector access
	void SetAxis( ureg axis, ycVec3 value );
	ycVec3 GetAxis( ureg axis ) const;

	//! utility constants
	constexpr inline static ycMtx4 ZERO()     { return ycMtx4( ycVec4::ZERO(),  ycVec4::ZERO(),  ycVec4::ZERO(),  ycVec4::ZERO()  ); }
	constexpr inline static ycMtx4 IDENTITY() { return ycMtx4( ycVec4::XAXIS(), ycVec4::YAXIS(), ycVec4::ZAXIS(), ycVec4::WAXIS() ); }
};

constexpr ycMtx4::ycMtx4( const ycVec4& _x, const ycVec4& _y, const ycVec4& _z, const ycVec4& _t )
	: x( _x )
	, y( _y )
	, z( _z )
	, t( _t )
{
}

constexpr ycMtx4::ycMtx4( const ycVec3& _x, const ycVec3& _y, const ycVec3& _z, const ycVec3& _t )
	: x( _x, 0.0f )
	, y( _y, 0.0f )
	, z( _z, 0.0f )
	, t( _t, 1.0f )
{
}

constexpr ycMtx4::ycMtx4(
	const f32 _00, const f32 _01, const f32 _02, const f32 _03,
	const f32 _10, const f32 _11, const f32 _12, const f32 _13,
	const f32 _20, const f32 _21, const f32 _22, const f32 _23,
	const f32 _30, const f32 _31, const f32 _32, const f32 _33
)
	: x( _00, _01, _02, _03 )
	, y( _10, _11, _12, _13 )
	, z( _20, _21, _22, _23 )
	, t( _30, _31, _32, _33 )
{
}

void ycMtx4::SetIdentity()
{
	x = ycVec4( 1.0f, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, 1.0f, 0.0f, 0.0f );
	z = ycVec4( 0.0f, 0.0f, 1.0f, 0.0f );
	t = ycVec4( 0.0f, 0.0f, 0.0f, 1.0f );
}

void ycMtx4::OrthoLH( const f32 width, const f32 height, const f32 zNear, const f32 zFar )
{
	ycAssertMsg( !ycIsInf( zFar ), "Attempting to create an orthographic projection with infinite far plane." );
	x = ycVec4( 2.0f/width, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, 2.0f/height, 0.0f, 0.0f );
	const f32 d = zFar - zNear;
	z = ycVec4( 0.0f, 0.0f, 1.0f / d, 0.0f );
	t = ycVec4( 0.0f, 0.0f, -zNear / d, 1.0f );

	ReverseDepth();
}

void ycMtx4::OrthoRH( const f32 width, const f32 height, const f32 zNear, const f32 zFar )
{
	ycAssertMsg( !ycIsInf( zFar ), "Attempting to create an orthographic projection with infinite far plane." );
	x = ycVec4( 2.0f/width, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, 2.0f/height, 0.0f, 0.0f );
	const f32 d = zFar - zNear;
	z = ycVec4( 0.0f, 0.0f, -1.0f / d, 0.0f );
	t = ycVec4( 0.0f, 0.0f, -zNear / d, 1.0f );

	ReverseDepth();
}

void ycMtx4::PerspectiveFovLH( const f32 fovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar )
{
	const f32 h = 1.0f / ycTan( 0.5f * fovYRad );
	const f32 w = h * height / width;
	x = ycVec4( w, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, h, 0.0f, 0.0f );
	if( ycIsInf( zFar ) )
	{
		z = ycVec4( 0.0f, 0.0f, 1.0f, 1.0f );
		t = ycVec4( 0.0f, 0.0f, -zNear, 0.0f );
	}
	else
	{
		const f32 d = zFar - zNear;
		z = ycVec4( 0.0f, 0.0f, zFar / d, 1.0f );
		t = ycVec4( 0.0f, 0.0f, -( zFar * zNear ) / d, 0.0f );
	}

	ReverseDepth();
}

void ycMtx4::PerspectiveFovRH( const f32 fovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar )
{
	const f32 h = 1.0f / ycTan( 0.5f * fovYRad );
	const f32 w = h * height / width;
	x = ycVec4( w, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, h, 0.0f, 0.0f );
	const f32 d = zFar - zNear;
	z = ycVec4( 0.0f, 0.0f, -zFar / d, -1.0f );
	t = ycVec4( 0.0f, 0.0f, -( zFar * zNear ) / d, 0.0f );

	ReverseDepth();
}

void ycMtx4::PerspectiveTanFovLH( const f32 tanFovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar )
{
	x = ycVec4( height / width / tanFovYRad, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, 1.0f / tanFovYRad, 0.0f, 0.0f );
	if( ycIsInf( zFar ) )
	{
		z = ycVec4( 0.0f, 0.0f, 1.0f, 1.0f );
		t = ycVec4( 0.0f, 0.0f, -zNear, 0.0f );
	}
	else
	{
		const f32 d = zFar - zNear;
		z = ycVec4( 0.0f, 0.0f, zFar / d, 1.0f );
		t = ycVec4( 0.0f, 0.0f, -( zFar * zNear ) / d, 0.0f );
	}

	ReverseDepth();
}

void ycMtx4::PerspectiveTanFovRH( const f32 tanFovYRad, const f32 width, const f32 height, const f32 zNear, const f32 zFar )
{
	x = ycVec4( height / width / tanFovYRad, 0.0f, 0.0f, 0.0f );
	y = ycVec4( 0.0f, 1.0f / tanFovYRad, 0.0f, 0.0f );
	const f32 d = zFar - zNear;
	z = ycVec4( 0.0f, 0.0f, -zFar / d, -1.0f );
	t = ycVec4( 0.0f, 0.0f, -( zFar * zNear ) / d, 0.0f );

	ReverseDepth();
}

void ycMtx4::LookAtLH( const ycVec3& eye, const ycVec3& target, const ycVec3& refUp )
{
	const ycVec3 dir  = (target - eye).Normalized();
	const ycVec3 side = refUp.Cross( dir ).Normalized();
	const ycVec3 up   = dir.Cross( side );

	x = ycVec4( side.x, up.x, dir.x, 0.0f );
	y = ycVec4( side.y, up.y, dir.y, 0.0f );
	z = ycVec4( side.z, up.z, dir.z, 0.0f );
	t = ycVec4( -side.Dot( eye ), -up.Dot( eye ), -dir.Dot( eye ), 1.0f );
}

void ycMtx4::LookAtRH( const ycVec3& eye, const ycVec3& target, const ycVec3& refUp )
{
	const ycVec3 dir  = (target - eye).Normalized();
	const ycVec3 side = dir.Cross( refUp ).Normalized();
	const ycVec3 up   = side.Cross( dir );

	x = ycVec4( side.x, up.x, -dir.x, 0.0f );
	y = ycVec4( side.y, up.y, -dir.y, 0.0f );
	z = ycVec4( side.z, up.z, -dir.z, 0.0f );
	t = ycVec4( -side.Dot( eye ), -up.Dot( eye ), dir.Dot( eye ), 1.0f );
}

ycVec4 ycMtx4::Transform( const ycVec4 rhs ) const
{
	const f32 _x    =        x.x*rhs.x + y.x*rhs.y + z.x*rhs.z + t.x*rhs.w;
	const f32 _y    =        x.y*rhs.x + y.y*rhs.y + z.y*rhs.z + t.y*rhs.w;
	const f32 _z    =        x.z*rhs.x + y.z*rhs.y + z.z*rhs.z + t.z*rhs.w;
	const f32 _w    =        x.w*rhs.x + y.w*rhs.y + z.w*rhs.z + t.w*rhs.w;
	return ycVec4( _x, _y, _z, _w );
}

ycVec3 ycMtx4::TransformProj( const ycVec4 rhs ) const
{
	const f32 _x    =        x.x*rhs.x + y.x*rhs.y + z.x*rhs.z + t.x*rhs.w;
	const f32 _y    =        x.y*rhs.x + y.y*rhs.y + z.y*rhs.z + t.y*rhs.w;
	const f32 _z    =        x.z*rhs.x + y.z*rhs.y + z.z*rhs.z + t.z*rhs.w;
	const f32 invW = 1.0f / (x.w*rhs.x + y.w*rhs.y + z.w*rhs.z + t.w*rhs.w);
	return ycVec3( _x*invW, _y*invW, _z*invW );
}

ycVec3 ycMtx4::TransformPos( const ycVec3 & local ) const
{
	return ycVec3(
		x.x*local.x + y.x*local.y + z.x*local.z + t.x,
		x.y*local.x + y.y*local.y + z.y*local.z + t.y,
		x.z*local.x + y.z*local.y + z.z*local.z + t.z
	);
}

ycVec3 ycMtx4::TransformDir( const ycVec3 & local ) const
{
	return ycVec3(
		x.x*local.x + y.x*local.y + z.x*local.z,
		x.y*local.x + y.y*local.y + z.y*local.z,
		x.z*local.x + y.z*local.y + z.z*local.z
	);
}

ycVec3 ycMtx4::TransformPosProj( const ycVec3 & rhs ) const
{
	const f32 _x    =        x.x*rhs.x + y.x*rhs.y + z.x*rhs.z + t.x;
	const f32 _y    =        x.y*rhs.x + y.y*rhs.y + z.y*rhs.z + t.y;
	const f32 _z    =        x.z*rhs.x + y.z*rhs.y + z.z*rhs.z + t.z;
	const f32 invW = 1.0f / (x.w*rhs.x + y.w*rhs.y + z.w*rhs.z + t.w);
	return ycVec3( _x*invW, _y*invW, _z*invW );
}

ycVec3 ycMtx4::TransformDirProj( const ycVec3 & rhs ) const
{
	const f32 _x    =        x.x*rhs.x + y.x*rhs.y + z.x*rhs.z;
	const f32 _y    =        x.y*rhs.x + y.y*rhs.y + z.y*rhs.z;
	const f32 _z    =        x.z*rhs.x + y.z*rhs.y + z.z*rhs.z;
	const f32 invW = 1.0f / (x.w*rhs.x + y.w*rhs.y + z.w*rhs.z + t.w);
	return ycVec3( _x*invW, _y*invW, _z*invW );
}

void ycMtx4::Transpose()
{
	*this = Transposed();
	// HACK TODO FIXME: check if it's worth optimizing this, the compiler may do a pretty good job already
}

ycMtx4 ycMtx4::Transposed() const
{
	return ycMtx4(
		x.x, y.x, z.x, t.x,
		x.y, y.y, z.y, t.y,
		x.z, y.z, z.z, t.z,
		x.w, y.w, z.w, t.w
	);
}

void ycMtx4::Invert()
{
	*this = Inverted();
	// HACK TODO FIXME: check if it's worth optimizing this, the compiler may do a pretty good job already
}

ycMtx4 ycMtx4::Inverted() const
{
	// implementation taken with permission from GLM http://glm.g-truc.net/0.9.7/index.html
	const f32 Coef00 = m[2][2]*m[3][3] - m[3][2]*m[2][3];
	const f32 Coef02 = m[1][2]*m[3][3] - m[3][2]*m[1][3];
	const f32 Coef03 = m[1][2]*m[2][3] - m[2][2]*m[1][3];

	const f32 Coef04 = m[2][1]*m[3][3] - m[3][1]*m[2][3];
	const f32 Coef06 = m[1][1]*m[3][3] - m[3][1]*m[1][3];
	const f32 Coef07 = m[1][1]*m[2][3] - m[2][1]*m[1][3];

	const f32 Coef08 = m[2][1]*m[3][2] - m[3][1]*m[2][2];
	const f32 Coef10 = m[1][1]*m[3][2] - m[3][1]*m[1][2];
	const f32 Coef11 = m[1][1]*m[2][2] - m[2][1]*m[1][2];

	const f32 Coef12 = m[2][0]*m[3][3] - m[3][0]*m[2][3];
	const f32 Coef14 = m[1][0]*m[3][3] - m[3][0]*m[1][3];
	const f32 Coef15 = m[1][0]*m[2][3] - m[2][0]*m[1][3];

	const f32 Coef16 = m[2][0]*m[3][2] - m[3][0]*m[2][2];
	const f32 Coef18 = m[1][0]*m[3][2] - m[3][0]*m[1][2];
	const f32 Coef19 = m[1][0]*m[2][2] - m[2][0]*m[1][2];

	const f32 Coef20 = m[2][0]*m[3][1] - m[3][0]*m[2][1];
	const f32 Coef22 = m[1][0]*m[3][1] - m[3][0]*m[1][1];
	const f32 Coef23 = m[1][0]*m[2][1] - m[2][0]*m[1][1];

	const ycVec4 Fac0(Coef00, Coef00, Coef02, Coef03);
	const ycVec4 Fac1(Coef04, Coef04, Coef06, Coef07);
	const ycVec4 Fac2(Coef08, Coef08, Coef10, Coef11);
	const ycVec4 Fac3(Coef12, Coef12, Coef14, Coef15);
	const ycVec4 Fac4(Coef16, Coef16, Coef18, Coef19);
	const ycVec4 Fac5(Coef20, Coef20, Coef22, Coef23);

	const ycVec4 Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
	const ycVec4 Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
	const ycVec4 Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
	const ycVec4 Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

	const ycVec4 Inv0(Vec1.Mul(Fac0) - Vec2.Mul(Fac1) + Vec3.Mul(Fac2));
	const ycVec4 Inv1(Vec0.Mul(Fac0) - Vec2.Mul(Fac3) + Vec3.Mul(Fac4));
	const ycVec4 Inv2(Vec0.Mul(Fac1) - Vec1.Mul(Fac3) + Vec3.Mul(Fac5));
	const ycVec4 Inv3(Vec0.Mul(Fac2) - Vec1.Mul(Fac4) + Vec2.Mul(Fac5));

	const ycVec4 SignA(+1.0f, -1.0f, +1.0f, -1.0f);
	const ycVec4 SignB(-1.0f, +1.0f, -1.0f, +1.0f);
	const ycMtx4 Inverse(Inv0.Mul(SignA), Inv1.Mul(SignB), Inv2.Mul(SignA), Inv3.Mul(SignB));

	const ycVec4 Row0(Inverse.m[0][0], Inverse.m[1][0], Inverse.m[2][0], Inverse.m[3][0]);

	const ycVec4 Dot0(x.Mul(Row0));
	const f32 Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

	const f32 OneOverDeterminant = 1.0f / Dot1;

	return Inverse * OneOverDeterminant;
}

void ycMtx4::RigidInvert()
{
	*this = RigidInverted();
}

ycMtx4 ycMtx4::RigidInverted() const
{
	ycMtx4 result( ycVec3(x.x, y.x, z.x), ycVec3(x.y, y.y, z.y), ycVec3(x.z, y.z, z.z), ycVec3::ZERO() );
	result.t = ycVec4( result.TransformDir( -ycVec3(t.x, t.y, t.z) ), 1.0f );
	return result;
}

void ycMtx4::ReverseDepth( bool shouldReverse )
{
	if( !shouldReverse ) { return; }

	ycMtx4 reverseMtx = ycMtx4::IDENTITY();
	reverseMtx.z.z = -1.0;
	reverseMtx.t.z = 1.0;
	*this = reverseMtx * ( *this );
}

ycMtx4 ycMtx4::operator+( const ycMtx4& rhs ) const
{
	return ycMtx4( x+rhs.x, y+rhs.y, z+rhs.z, t+rhs.t );
}

ycMtx4 ycMtx4::operator-( const ycMtx4& rhs ) const
{
	return ycMtx4( x-rhs.x, y-rhs.y, z-rhs.z, t-rhs.t );
}

ycMtx4 ycMtx4::operator*( const ycMtx4& rhs ) const
{
	const ycVec4 lhsX = x;
	const ycVec4 lhsY = y;
	const ycVec4 lhsZ = z;
	const ycVec4 lhsT = t;

	const ycVec4 rhsX = rhs.x;
	const ycVec4 rhsY = rhs.y;
	const ycVec4 rhsZ = rhs.z;
	const ycVec4 rhsT = rhs.t;

	return ycMtx4(
		lhsX*rhsX.x + lhsY*rhsX.y + lhsZ*rhsX.z + lhsT*rhsX.w,
		lhsX*rhsY.x + lhsY*rhsY.y + lhsZ*rhsY.z + lhsT*rhsY.w,
		lhsX*rhsZ.x + lhsY*rhsZ.y + lhsZ*rhsZ.z + lhsT*rhsZ.w,
		lhsX*rhsT.x + lhsY*rhsT.y + lhsZ*rhsT.z + lhsT*rhsT.w
	);
}

ycVec4 ycMtx4::operator*( const ycVec4& rhs ) const
{
	return ycVec4(
		x.x*rhs.x + y.x*rhs.y + z.x*rhs.z + t.x*rhs.w,
		x.y*rhs.x + y.y*rhs.y + z.y*rhs.z + t.y*rhs.w,
		x.z*rhs.x + y.z*rhs.y + z.z*rhs.z + t.z*rhs.w,
		x.w*rhs.x + y.w*rhs.y + z.w*rhs.z + t.w*rhs.w
	);
}

ycVec3 ycMtx4::operator*( const ycVec3& rhs ) const
{
	return ycVec3(
		x.x*rhs.x + y.x*rhs.y + z.x*rhs.z,
		x.y*rhs.x + y.y*rhs.y + z.y*rhs.z,
		x.z*rhs.x + y.z*rhs.y + z.z*rhs.z
	);
}

ycMtx4 ycMtx4::operator*( const f32 rhs ) const
{
	return ycMtx4( x*rhs, y*rhs, z*rhs, t*rhs );
}

ycMtx4& ycMtx4::operator+=( const ycMtx4& rhs )
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	t += rhs.t;
	return *this;
}

ycMtx4& ycMtx4::operator-=( const ycMtx4& rhs )
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	t -= rhs.t;
	return *this;
}

ycMtx4& ycMtx4::operator*=( const ycMtx4& rhs )
{
	const ycVec4 lhsX = x;
	const ycVec4 lhsY = y;
	const ycVec4 lhsZ = z;
	const ycVec4 lhsT = t;

	const ycVec4 rhsX = rhs.x;
	const ycVec4 rhsY = rhs.y;
	const ycVec4 rhsZ = rhs.z;
	const ycVec4 rhsT = rhs.t;

	x = lhsX*rhsX.x + lhsY*rhsX.y + lhsZ*rhsX.z + lhsT*rhsX.w;
	y = lhsX*rhsY.x + lhsY*rhsY.y + lhsZ*rhsY.z + lhsT*rhsY.w;
	z = lhsX*rhsZ.x + lhsY*rhsZ.y + lhsZ*rhsZ.z + lhsT*rhsZ.w;
	t = lhsX*rhsT.x + lhsY*rhsT.y + lhsZ*rhsT.z + lhsT*rhsT.w;

	return *this;
}

ycMtx4& ycMtx4::operator*=( const f32 rhs )
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	t *= rhs;
	return *this;
}

bool ycMtx4::IsIdentity( f32 tolerance /*= 1e-4f*/ ) const
{
	return x.IsNearlyEqual( ycVec4( 1,0,0,0 ), tolerance )
	    && y.IsNearlyEqual( ycVec4( 0,1,0,0 ), tolerance )
		&& z.IsNearlyEqual( ycVec4( 0,0,1,0 ), tolerance )
		&& t.IsNearlyEqual( ycVec4( 0,0,0,1 ), tolerance );
}

bool ycMtx4::IsZero( f32 tolerance /*= 1e-4f*/ ) const
{
	return x.IsZero( tolerance ) && y.IsZero( tolerance ) && z.IsZero( tolerance ) && t.IsZero( tolerance );
}

ycVec4 ycMtx4::Min()
{
	return ycVec4::Min( ycVec4::Min( x, y ), ycVec4::Min( z, t ) );
}

ycVec4 ycMtx4::Max()
{
	return ycVec4::Min( ycVec4::Min( x, y ), ycVec4::Min( z, t ) );
}

ycVec3 ycMtx4::GetForwardAxis() const
{
	return z.xyz().Normalized();
}

ycVec3 ycMtx4::GetUpAxis() const
{
	return y.xyz().Normalized();
}

ycVec3 ycMtx4::GetRightAxis() const
{
	return x.xyz().Normalized();
}

#include "ycPopDebugOpt.inc"
