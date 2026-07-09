#pragma once

/*
\class ycVec4

4 axis vector handling

*/

#include "ycVec2.h"
#include "ycVec3.h"

#include "ycPushDebugOpt.inc"

struct ycVec4 //%
{
yceditable:

	//! the data!
	union
	{
		struct { float x, y, z, w; };
		float a[ 4 ];
	};

public:

	//! default ctor
	ycVec4() = default;

	//! sets x, y, and z
	constexpr inline explicit ycVec4( const float _x, const float _y, const float _z, const float _w );

	//! sets x, y, and z to the same value
	constexpr inline explicit ycVec4( const float xyzw );

	//! from vec3
	constexpr inline explicit ycVec4( const ycVec3 xyz, const float _w );

	//! from vec2
	constexpr inline explicit ycVec4( const ycVec2 xy, const ycVec2 zw );

	//!
	inline float Dot( const ycVec4& other ) const;
	inline void Negate();
	inline ycVec4 Negated() const;

	//! length
	inline float LengthSq() const;
	inline float MagSq() const { return LengthSq(); }
	inline float Length() const;
	inline float Mag() const { return Length(); }

	//! set the magnitude of the vector, preserving direction
	inline void ScaleToMag( const f32 mag );
	inline ycVec4 ScaledToMag( const f32 mag ) const;

	//! normalization
	/*! Sets all components to zero for zero-length vectors */
	inline void Normalize();
	inline ycVec4 Normalized() const;
	inline bool IsNormalized() const;

	//! operators
	inline ycVec4  operator +  ( const ycVec4& rhs ) const;
	inline ycVec4  operator +  ( const float   rhs ) const;
	inline ycVec4  operator -  ( const ycVec4& rhs ) const;
	inline ycVec4  operator -  ( const float   rhs ) const;
	inline ycVec4  operator /  ( const float   rhs ) const;
	inline ycVec4  operator -  () const;
	inline ycVec4& operator += ( const ycVec4& rhs );
	inline ycVec4& operator -= ( const ycVec4& rhs );
	inline ycVec4& operator *= ( const float   rhs );
	inline ycVec4& operator /= ( const float   rhs );
	//inline bool operator == ( const ycVec4& rhs ) const; // intentionally omitting... should we be doing perfect fp comparison?
	//inline bool operator != ( const ycVec4& rhs ) const;
	//inline ycVec4 operator*( const ycVec4& rhs ) const; // intentionally omitting since this can be misinterpretted as dot product; also pretty rare to need component-wise mul
	//inline ycVec4 operator/( const ycVec4& rhs ) const;
	//inline ycVec4& operator*=( const ycVec4& rhs );
	//inline ycVec4& operator/=( const ycVec4& rhs );

	inline bool Equals( const ycVec4& v ) const { return x==v.x && y==v.y && z==v.z && w == v.w; }
	inline bool IsNearlyEqual( const ycVec4& v, float tolerance ) const { return ( *this - v ).IsZero( tolerance ); }
	inline bool IsZero( float tolerance = 1e-4f ) const { return MagSq() < tolerance * tolerance; }

	inline ycVec4 Mul( const ycVec4& rhs ) const; // component-wise multiply

	//! to vec3
	inline ycVec3 xyz() const;

	//! from vec3
	inline void SetXYZ( const ycVec3 v );
	inline void SetXYZ( const f32 x, const f32 y, const f32 z );

	//! Min / Max
	inline static ycVec4 Min( const ycVec4& a, const ycVec4& b );
	inline static ycVec4 Max( const ycVec4& a, const ycVec4& b );

	inline static ycVec4 Clamp( const ycVec4& src, const ycVec4& min, const ycVec4& max );

	inline static ycVec4 CrossAxes( const ycVec4& a, const ycVec4& b );

	//! utility constants
	constexpr inline static ycVec4 XAXIS() { return ycVec4( 1.0f, 0.0f, 0.0f, 0.0f ); }
	constexpr inline static ycVec4 YAXIS() { return ycVec4( 0.0f, 1.0f, 0.0f, 0.0f ); }
	constexpr inline static ycVec4 ZAXIS() { return ycVec4( 0.0f, 0.0f, 1.0f, 0.0f ); }
	constexpr inline static ycVec4 WAXIS() { return ycVec4( 0.0f, 0.0f, 0.0f, 1.0f ); }
	constexpr inline static ycVec4 ONE()   { return ycVec4( 1.0f, 1.0f, 1.0f, 1.0f ); }
	constexpr inline static ycVec4 ZERO()  { return ycVec4( 0.0f, 0.0f, 0.0f, 0.0f ); }
};

inline ycVec4  operator*  ( const ycVec4& v, f32 s );
inline ycVec4  operator*  ( f32 s, const ycVec4& v );


constexpr ycVec4::ycVec4( const float _x, const float _y, const float _z, const float _w )
	: x( _x )
	, y( _y )
	, z( _z )
	, w( _w )
{
}

constexpr ycVec4::ycVec4( const float xyzw )
	: x( xyzw )
	, y( xyzw )
	, z( xyzw )
	, w( xyzw )
{
}

constexpr ycVec4::ycVec4( const ycVec3 xyz, const float _w )
	: x( xyz.x )
	, y( xyz.y )
	, z( xyz.z )
	, w( _w )
{
}

constexpr ycVec4::ycVec4( const ycVec2 xy, const ycVec2 zw )
	: x( xy.x )
	, y( xy.y )
	, z( zw.x )
	, w( zw.y )
{
}

float ycVec4::Dot( const ycVec4& other ) const
{
	return x*other.x + y*other.y + z*other.z + w*other.w;
}

void ycVec4::Negate()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;
}

ycVec4 ycVec4::Negated() const
{
	return ycVec4( -x, -y, -z, -w );
}

float ycVec4::LengthSq() const
{
	return x*x + y*y + z*z + w*w;
}

float ycVec4::Length() const
{
	return ycSqrt( LengthSq() );
}

void ycVec4::ScaleToMag( const f32 mag )
{
	const f32 cur = Mag();
	const f32 scale = mag / cur;
	x *= scale;
	y *= scale;
	z *= scale;
	w *= scale;
}

ycVec4 ycVec4::ScaledToMag( const f32 mag ) const
{
	const f32 cur = Mag();
	const f32 scale = mag / cur;
	return ycVec4( x*scale, y*scale, z*scale, w*scale );
}

void ycVec4::Normalize()
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { /*assume all components are already zero*/ return; }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	x *= invLengthSq;
	y *= invLengthSq;
	z *= invLengthSq;
	w *= invLengthSq;
}

ycVec4 ycVec4::Normalized() const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { return ycVec4::ZERO(); }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycVec4(
		x * invLengthSq,
		y * invLengthSq,
		z * invLengthSq,
		w * invLengthSq
	);
}

bool ycVec4::IsNormalized() const
{
	f32 tolerance = 0.000001f;
	return this->LengthSq() > (1.f - tolerance) && this->LengthSq() < (1.f + tolerance);
}

ycVec4 ycVec4::operator+( const ycVec4& rhs ) const
{
	return ycVec4( x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w );
}

ycVec4 ycVec4::operator+( const float rhs ) const
{
	return ycVec4( x+rhs, y+rhs, z+rhs, w+rhs );
}

ycVec4 ycVec4::operator-( const ycVec4& rhs ) const
{
	return ycVec4( x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.w );
}

ycVec4 ycVec4::operator-( const float rhs ) const
{
	return ycVec4( x-rhs, y-rhs, z-rhs, w-rhs );
}

inline ycVec4 operator*( const ycVec4& v, const f32 s )
{
	return ycVec4( v.x * s, v.y * s, v.z * s, v.w * s );
}

inline ycVec4 operator*( const f32 s, const ycVec4& v )
{
	return ycVec4( v.x * s, v.y * s, v.z * s, v.w * s );
}

ycVec4 ycVec4::operator/( const float rhs ) const
{
	return ycVec4( x/rhs, y/rhs, z/rhs, w/rhs );
}

ycVec4 ycVec4::operator-() const
{
	return ycVec4( -x, -y, -z, -w );
}

ycVec4& ycVec4::operator+=( const ycVec4& rhs )
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
	return *this;
}

ycVec4& ycVec4::operator-=( const ycVec4& rhs )
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
	return *this;
}

ycVec4& ycVec4::operator*=( const float rhs )
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;
	return *this;
}

ycVec4& ycVec4::operator/=( const float rhs )
{
	x /= rhs;
	y /= rhs;
	z /= rhs;
	w /= rhs;
	return *this;
}

ycVec4 ycVec4::Mul( const ycVec4& rhs ) const
{
	return ycVec4( x*rhs.x, y*rhs.y, z*rhs.z, w*rhs.w );
}

ycVec3 ycVec4::xyz() const
{
	return ycVec3( x, y, z );
}

void ycVec4::SetXYZ( const ycVec3 v )
{
	x = v.x;
	y = v.y;
	z = v.z;
}

void ycVec4::SetXYZ( const f32 _x, const f32 _y, const f32 _z )
{
	x = _x;
	y = _y;
	z = _z;
}

/*static*/ ycVec4 ycVec4::Min( const ycVec4& a, const ycVec4& b )
{
	return ycVec4( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w );
}

/*static*/ ycVec4 ycVec4::Max( const ycVec4& a, const ycVec4& b )
{
	return ycVec4( a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y, a.z < b.z ? b.z : a.z, a.w < b.w ? b.w : a.w );
}

/*static*/ ycVec4 ycVec4::Clamp( const ycVec4& src, const ycVec4& min, const ycVec4& max )
{
	return Max( Min( src, max ), min );
}

/*static*/ ycVec4 ycVec4::CrossAxes( const ycVec4& a, const ycVec4& b )
{
	return ycVec4{ a.xyz().Cross( b.xyz() ).Normalized(), 0.0f };
}

#include "ycPopDebugOpt.inc"
