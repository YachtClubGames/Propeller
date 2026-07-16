#pragma once

#include "ycCompilerConfig.h"

#include "ycVec3.h"
#include "ycMtx4.h"
#include "ycStd.h"

struct ycCollisionMtx3
{
public:

	union
	{
		struct { ycVec3 x, y, z; };
		f32 a[ 9 ];
		f32 m[ 3 ][ 3 ];
	};

	//! default ctor
	ycCollisionMtx3() = default;

	//! basis vector ctor
	constexpr inline explicit ycCollisionMtx3( const ycVec3& _x, const ycVec3& _y, const ycVec3& _z );

	//! takes only the upper corner
	inline explicit ycCollisionMtx3( const ycMtx4& _m4 );

	//! all the values!
	constexpr inline explicit ycCollisionMtx3(
		const float _00, const float _01, const float _02,
		const float _10, const float _11, const float _12,
		const float _20, const float _21, const float _22
	);

	//! vector transformation
	inline ycVec3 Transform( const ycVec3& rhs ) const;

	inline void Transpose();
	inline ycCollisionMtx3 Transposed() const;

	// multiplies with own transpose
	void MulWithTranspose();

	// returns true if matrix has no shear and uniform scale
	bool IsOrthoScalar( f32* scale );

	//! operators
	inline ycCollisionMtx3  operator +  ( const ycCollisionMtx3& rhs ) const;
	inline ycCollisionMtx3  operator -  ( const ycCollisionMtx3& rhs ) const;
	inline ycCollisionMtx3  operator *  ( const ycCollisionMtx3& rhs ) const;
	inline ycCollisionMtx3  operator *  ( const float rhs ) const;
	inline ycCollisionMtx3& operator += ( const ycCollisionMtx3& rhs );
	inline ycCollisionMtx3& operator -= ( const ycCollisionMtx3& rhs );
	inline ycCollisionMtx3& operator *= ( const ycCollisionMtx3& rhs );
	inline ycCollisionMtx3& operator *= ( const float rhs );

	inline ycVec3  operator *  ( const ycVec3& rhs ) const;

	//! utility constants
	constexpr inline static ycCollisionMtx3 ZERO() { return ycCollisionMtx3( ycVec3::ZERO(), ycVec3::ZERO(), ycVec3::ZERO() ); }
	constexpr inline static ycCollisionMtx3 IDENTITY() { return ycCollisionMtx3( ycVec3::XAXIS(), ycVec3::YAXIS(), ycVec3::ZAXIS() ); }
};

constexpr inline ycCollisionMtx3::ycCollisionMtx3( const ycVec3& _x, const ycVec3& _y, const ycVec3& _z )
	: x( _x ), y( _y ), z( _z )
{}

inline ycCollisionMtx3::ycCollisionMtx3( const ycMtx4& _m4 )
	: x( _m4.x.xyz() ), y( _m4.y.xyz() ), z( _m4.z.xyz() )
{}

constexpr ycCollisionMtx3::ycCollisionMtx3(
	const float _00, const float _01, const float _02,
	const float _10, const float _11, const float _12,
	const float _20, const float _21, const float _22
)
	: x( _00, _01, _02 )
	, y( _10, _11, _12 )
	, z( _20, _21, _22 )
{}

inline void ycCollisionMtx3::Transpose()
{
	ycSwap( m[ 0 ][ 1 ], m[ 1 ][ 0 ] );
	ycSwap( m[ 0 ][ 2 ], m[ 2 ][ 0 ] );
	ycSwap( m[ 1 ][ 2 ], m[ 2 ][ 1 ] );
}

inline ycCollisionMtx3 ycCollisionMtx3::Transposed() const
{
	ycCollisionMtx3 out = *this;
	ycSwap( out.m[ 0 ][ 1 ], out.m[ 1 ][ 0 ] );
	ycSwap( out.m[ 0 ][ 2 ], out.m[ 2 ][ 0 ] );
	ycSwap( out.m[ 1 ][ 2 ], out.m[ 2 ][ 1 ] );
	return out;
}

inline ycCollisionMtx3  ycCollisionMtx3::operator +  ( const ycCollisionMtx3& rhs ) const { return ycCollisionMtx3( *this ) += rhs; }
inline ycCollisionMtx3  ycCollisionMtx3::operator -  ( const ycCollisionMtx3& rhs ) const { return ycCollisionMtx3( *this ) -= rhs; }
inline ycCollisionMtx3  ycCollisionMtx3::operator *  ( const float rhs ) const { return ycCollisionMtx3( *this ) *= rhs; }

inline ycCollisionMtx3& ycCollisionMtx3::operator += ( const ycCollisionMtx3& rhs )
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	return *this;
}

inline ycCollisionMtx3& ycCollisionMtx3::operator -= ( const ycCollisionMtx3& rhs )
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	return *this;
}

inline ycCollisionMtx3& ycCollisionMtx3::operator *= ( const float rhs )
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	return *this;
}

inline ycCollisionMtx3  ycCollisionMtx3::operator *  ( const ycCollisionMtx3& rhs ) const 
{
	const ycVec3 lhsX = x;
	const ycVec3 lhsY = y;
	const ycVec3 lhsZ = z;

	const ycVec3 rhsX = rhs.x;
	const ycVec3 rhsY = rhs.y;
	const ycVec3 rhsZ = rhs.z;

	return ycCollisionMtx3(
		lhsX * rhsX.x + lhsY * rhsX.y + lhsZ * rhsX.z,
		lhsX * rhsY.x + lhsY * rhsY.y + lhsZ * rhsY.z,
		lhsX * rhsZ.x + lhsY * rhsZ.y + lhsZ * rhsZ.z
	);
}

inline ycCollisionMtx3& ycCollisionMtx3::operator *= ( const ycCollisionMtx3& rhs ) { *this = ( *this * rhs ); return *this; }

inline ycVec3  ycCollisionMtx3::operator *  ( const ycVec3& rhs ) const
{
	return ycVec3(
		x.x * rhs.x + y.x * rhs.y + z.x * rhs.z,
		x.y * rhs.x + y.y * rhs.y + z.y * rhs.z,
		x.z * rhs.x + y.z * rhs.y + z.z * rhs.z
	);
}

ycVec3 ycCollisionMtx3::Transform( const ycVec3& local ) const
{
	return ycVec3(
		x.x * local.x + y.x * local.y + z.x * local.z,
		x.y * local.x + y.y * local.y + z.y * local.z,
		x.z * local.x + y.z * local.y + z.z * local.z
	);
}
