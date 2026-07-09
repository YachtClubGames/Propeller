#pragma once

/*! \class ycVec3
Warning: there is no default ctor! if you need to zero initialize, you must do it manually
*/

#include "ycCommon.h"

#include "ycMath.h"
//#include "ycStd.h"

class ycRand;
struct ycVec2;

// printf helpers
#define VEC3_FMT "%.2f, %.2f, %.2f"
#define VEC3_ARG(v) (v).x, (v).y, (v).z

#include "ycPushDebugOpt.inc"

struct ycVec3 //% 
{
yceditable:
	//! the data!
	union
	{
		struct { f32 x, y, z; };
		f32 a[ 3 ];
	};

public:
	//! default ctor
	ycVec3() = default;

	//! sets x, y, and z
	constexpr inline explicit ycVec3( const f32 _x, const f32 _y, const f32 _z );

	//! sets x, y. z as zero
	constexpr inline explicit ycVec3( const f32 _x, const f32 _y );

	//! sets x, y. z as zero
	explicit ycVec3( const ycVec2 & _xy, f32 _z = 0.0f );

	//! sets x, y, and z from an array.
	explicit ycVec3( const f32 * values );

	//! sets x, y, and z to the same value
	constexpr inline explicit ycVec3( const f32 xyz );
	
	inline ycVec3& Set( f32 x, f32 y, f32 z );
	inline ycVec3& Set( const ycVec3& xyz );

	inline ycVec3& SetX( f32 x );
	inline ycVec3& SetY( f32 y );
	inline ycVec3& SetZ( f32 z );

	inline ycVec3& SetXY( const ycVec3 &xy );

	//! returns the dot product 
	inline f32 Dot( const ycVec3& other ) const;
	//! returns the cross product 
	inline ycVec3 Cross( const ycVec3& other ) const;
	//! Does the scalar product of 3 vectors a � (b x c)
	inline f32 ScalarTriple( const ycVec3& b, const ycVec3& c ) const;
	//! lerps between two vectors
	inline ycVec3 Lerp( const ycVec3& other, const f32 t ) const;
	//! lerps between two vectors
	inline ycVec3 Lerp( const ycVec3& other, const ycVec3& t ) const;
	//! lerps between two vectors and clamps t value
	inline ycVec3 LerpC( const ycVec3& other, const f32 t, const f32 min = 0.0f, const f32 max = 1.0f ) const;

	//! length
	inline f32 LengthSq() const;
	inline f32 MagSq() const { return LengthSq(); }
	inline f32 Length() const;
	inline f32 Mag() const { return Length(); }

	//! set the magnitude of the vector, preserving direction
	inline void ScaleToMag( const f32 mag );
	inline ycVec3 ScaledToMag( const f32 mag ) const;
	inline void ClampToMag( f32 min, f32 max );
	inline ycVec3 ClampedToMag( f32 min, f32 max ) const;
	inline void RaiseToMag( f32 min );
	inline void LowerToMag( f32 max );
	inline ycVec3 RaisedToMag( f32 min );
	inline ycVec3 LoweredToMag( f32 max );

	//! distance from another point
	inline f32 DistSq( const ycVec3& other ) const;
	inline f32 Dist( const ycVec3& other ) const;

	//! normalization
	/*! Sets all components to zero for zero-length vectors */
	inline f32 Normalize(); //! returns the length
	inline ycVec3 Normalized() const;
	inline ycVec3 NormalizedOr( const ycVec3& fallbackIfTooSmall ) const;
	inline ycVec3 NormalizedOr( f32 minMagSq, const ycVec3& fallbackIfTooSmall ) const;
	inline bool IsNormalized() const;

	inline bool IsEqual( const ycVec3&, f32 tolerance ) const;

	inline ycVec3 Inverted() const;
	inline ycVec3 SafeInverted() const;

	//! operators
	inline ycVec3  operator +  ( const ycVec3& rhs ) const;
	inline ycVec3  operator -  ( const ycVec3& rhs ) const;
	inline ycVec3  operator /  ( const f32   rhs ) const;
	inline ycVec3  operator -  () const;
	inline ycVec3& operator *= ( const f32   rhs );
	inline ycVec3& operator /= ( const f32   rhs );
	inline ycVec3& operator += ( const ycVec3& rhs );
	inline ycVec3& operator -= ( const ycVec3& rhs );
	inline bool operator > ( const ycVec3& rhs ) const;
	inline bool operator < ( const ycVec3& rhs ) const;
	inline void Negate();
	inline ycVec3 Negated() const;
	inline bool operator == ( const ycVec3& rhs ) const; 
	inline bool operator != ( const ycVec3& rhs ) const;
	inline ycVec3 operator*( const ycVec3& rhs ) const; 
	inline ycVec3 operator/( const ycVec3& rhs ) const;
	inline ycVec3& operator*=( const ycVec3& rhs );
	inline ycVec3& operator/=( const ycVec3& rhs );

	inline ycVec3 Mul( const ycVec3& rhs ) const; // component-wise multiply

	//! to vec2
	ycVec2 ToXY2() const; // not inline, to avoid including ycVec2.h in ycVec3.h
	ycVec2 ToXZ2() const; // not inline, to avoid including ycVec2.h in ycVec3.h
	ycVec2 ToYZ2() const; // not inline, to avoid including ycVec2.h in ycVec3.h

	//! zeroes out the missing component
	inline ycVec3 ToXY() const;
	inline ycVec3 ToYZ() const;
	inline ycVec3 ToXZ() const;
	inline ycVec3 ToX() const;
	inline ycVec3 ToY() const;
	inline ycVec3 ToZ() const;
	inline f32 DistXY( const ycVec3& other ) const;
	inline f32 DistSqXY( const ycVec3& other ) const;

	//! Project onto n
	inline ycVec3 Project( const ycVec3& n ) const;
	inline ycVec3 Onto( const ycVec3& n ) const;
	inline ycVec3 OntoUnit( const ycVec3& n ) const;
	//! Minus the projection onto n (positive or negative) - `x - x.Dot(n)`
	inline ycVec3 Without( const ycVec3& n ) const;
	inline ycVec3 WithoutUnit( const ycVec3& n ) const;
	//! Minus the positive projection onto n - `x - Min( 0.0f, x.Dot( n ) )`
	inline ycVec3 WithoutPositive( const ycVec3& n ) const;
	inline ycVec3 WithoutPositiveUnit( const ycVec3& n ) const;
	//! Calculates reflection vector for a given normal. 
	inline ycVec3 Reflect( const ycVec3 & n ) const;
	//! Returns a direction vector that is perpendicular to this vector and towards first.  
	//if the perpendicular does not point toward first, defaultReturn is the result
	ycVec3 Perpendicular( const ycVec3 & first = ycVec3::YAXIS(), const ycVec3 & defaultReturn = ycVec3::ZAXIS()) const;
	//! Returns a direction vector that is perpendicular to this vector and the vector returned by Perpendicular
	ycVec3 PerpendicularAlt( const ycVec3 & first = ycVec3::YAXIS(), const ycVec3 & defaultReturn = ycVec3::ZAXIS() ) const;

	//! Returns two arbitrary perpendicular basis vectors. (requires normalized vector)
	void OrthoNormalBasis( ycVec3* b1, ycVec3* b2 );

	//! Min / Max
	static inline ycVec3 Min( const ycVec3& a, const ycVec3& b );
	static inline ycVec3 Max( const ycVec3& a, const ycVec3& b );

	static inline ycVec3 Clamp( const ycVec3& v, const ycVec3& min, const ycVec3& max );
	inline ycVec3 Abs() const;
	inline ycVec3 Sign() const;
	inline ycVec3 SignZero() const; // same as sign but returns 0 for axis if it started as zero

	inline bool Within( const ycVec3& min, const ycVec3& max ) const { return x>=min.x && x<=max.x && y>=min.y && y<=max.y && z>=min.z && z<=max.z; }
	static inline ycVec3 AsAxis( ureg axis, f32 value );
	inline u32 LongestAxis() const;
	inline u32 LongestAxisAbs() const;
	inline ycVec3 AsLongestAxis() const;
	inline ycVec3 AsLongestAxisAbs() const;
	inline f32 FirstNonZeroAxis() const;

	bool Equals( const ycVec3& v ) const { return x==v.x && y==v.y && z==v.z; }
	bool Equals( const ycVec3& v, f32 epsilon ) const;

	//! random vectors
	static ycVec3 RandUnitXY( ycRand* rng );
	static ycVec3 RandUnitYZ( ycRand* rng );
	static ycVec3 RandUnitXZ( ycRand* rng );
	static ycVec3 RandUnit( ycRand* rng );

	//! utility constants
	constexpr inline static ycVec3 XAXIS() { return ycVec3( 1.0f, 0.0f, 0.0f ); }
	constexpr inline static ycVec3 YAXIS() { return ycVec3( 0.0f, 1.0f, 0.0f ); }
	constexpr inline static ycVec3 ZAXIS() { return ycVec3( 0.0f, 0.0f, 1.0f ); }
	constexpr inline static ycVec3 NEGXAXIS() { return ycVec3( -1.0f,  0.0f,  0.0f ); }
	constexpr inline static ycVec3 NEGYAXIS() { return ycVec3(  0.0f, -1.0f,  0.0f ); }
	constexpr inline static ycVec3 NEGZAXIS() { return ycVec3(  0.0f,  0.0f, -1.0f ); }
	constexpr inline static ycVec3 ONE()   { return ycVec3( 1.0f, 1.0f, 1.0f ); }
	constexpr inline static ycVec3 ZERO()  { return ycVec3( 0.0f, 0.0f, 0.0f ); }

	constexpr inline static ycVec3 ONEAXIS( const u32 axis ) { return ycVec3( axis==0?1.0f:0.0f, axis==1?1.0f:0.0f, axis==2?1.0f:0.0f ); }

	constexpr inline static ycVec3 UP()         { return ycVec3( 0.0f, 1.0f, 0.0f ); }
	constexpr inline static ycVec3 RIGHT()      { return ycVec3( 1.0f, 0.0f, 0.0f ); }
	constexpr inline static ycVec3 FORWARD()    { return ycVec3( 0.0f, 0.0f, 1.0f ); }

	constexpr inline static ycVec3 DOWN()       { return ycVec3( 0.0f, -1.0f, 0.0f ); }
	constexpr inline static ycVec3 LEFT()       { return ycVec3( -1.0f, 0.0f, 0.0f ); }
	constexpr inline static ycVec3 BACKWARD()   { return ycVec3( 0.0f, 0.0f, -1.0f ); }

	constexpr inline static ycVec3 FLTMAX() { return ycVec3( YC_F32_MAX, YC_F32_MAX, YC_F32_MAX ); }
};

inline ycVec3  operator*  ( const ycVec3& v, f32 s );
inline ycVec3  operator*  ( f32 s, const ycVec3& v );

inline constexpr ycVec3::ycVec3( const f32 _x, const f32 _y, const f32 _z )
	: x( _x )
	, y( _y )
	, z( _z )
{
}

inline constexpr ycVec3::ycVec3( const f32 xyz )
	: x( xyz )
	, y( xyz )
	, z( xyz )
{
}

inline constexpr ycVec3::ycVec3( const f32 _x, const f32 _y )
	: x( _x )
	, y( _y )
	, z( 0.0f )
{
}

inline ycVec3& ycVec3::Set( f32 _x, f32 _y, f32 _z )
{
	x = _x;
	y = _y;
	z = _z;
	return *this;
}

inline ycVec3& ycVec3::Set( const ycVec3& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	return *this;
}

inline ycVec3& ycVec3::SetX( f32 _x )
{
	x = _x;
	return *this;
}

inline ycVec3& ycVec3::SetY( f32 _y )
{
	y = _y;
	return *this;
}

inline ycVec3& ycVec3::SetZ( f32 _z )
{
	z = _z;
	return *this;
}

inline ycVec3& ycVec3::SetXY( const ycVec3 &xy )
{
	x = xy.x;
	y = xy.y;
	return *this;
}

inline f32 ycVec3::Dot( const ycVec3& other ) const
{
	return x*other.x + y*other.y + z*other.z;
}

inline ycVec3 ycVec3::Cross( const ycVec3& other ) const
{
	#if defined YC_ARM && defined YC_CLANG && __clang_major__ >= 14
		#pragma STDC FP_CONTRACT OFF
	#endif
	return ycVec3(
		y*other.z - z*other.y,
		z*other.x - x*other.z,
		x*other.y - y*other.x
	);
}

inline f32 ycVec3::ScalarTriple( const ycVec3& b, const ycVec3& c ) const
{
	return Dot( b.Cross( c ) );
}

inline ycVec3 ycVec3::Lerp( const ycVec3& other, const f32 t ) const
{
	return ycVec3(
		ycLerp( x, other.x, t ),
		ycLerp( y, other.y, t ),
		ycLerp( z, other.z, t )
	);
}

inline ycVec3 ycVec3::Lerp( const ycVec3& other, const ycVec3& t ) const
{
	return ycVec3(
		ycLerp( x, other.x, t.x ),
		ycLerp( y, other.y, t.y ),
		ycLerp( z, other.z, t.z )
	);
}

inline ycVec3 ycVec3::LerpC( const ycVec3& other, const f32 t, const f32 min, const f32 max ) const
{
	f32 tc = ycClamp( t, min, max );
	return ycVec3(
		ycLerp( x, other.x, tc ),
		ycLerp( y, other.y, tc ),
		ycLerp( z, other.z, tc )
	);
}

inline f32 ycVec3::LengthSq() const
{
	return x*x + y*y + z*z;
}

inline f32 ycVec3::Length() const
{
	return ycSqrt( LengthSq() );
}

inline void ycVec3::ScaleToMag( const f32 mag )
{
	const f32 cur = Mag();
	const f32 scale = mag / cur;
	if( cur < YC_F32_EPSILON ) { *this = ycVec3( 0.0f ); return; }
	x *= scale;
	y *= scale;
	z *= scale;
}

inline ycVec3 ycVec3::ScaledToMag( const f32 mag ) const
{
	const f32 cur = Mag();
	if( cur < YC_F32_EPSILON ) { return ycVec3( 0.0f ); }
	const f32 scale = mag / cur;
	return ycVec3( x*scale, y*scale, z*scale );
}

inline void ycVec3::ClampToMag( f32 min, f32 max )
{
	const f32 cur = Mag();
	if( cur < YC_F32_EPSILON ) { return; }
	*this *= ycClamp( cur, min, max ) / cur;
}

inline ycVec3 ycVec3::ClampedToMag( f32 min, f32 max ) const
{
	ycVec3 result = *this;
	result.ClampToMag( min, max );
	return result;
}

inline void ycVec3::RaiseToMag( f32 min )
{
	const f32 cur = Mag();
	if( cur < YC_F32_EPSILON ) { *this = ycVec3( 0.0f ); return; }
	if( cur >= min ) { return; }

	const f32 scale = min / cur;
	x *= scale;
	y *= scale;
	z *= scale;
}

inline void ycVec3::LowerToMag( f32 max )
{
	const f32 cur = Mag();
	if( cur < YC_F32_EPSILON ) { *this = ycVec3( 0.0f ); return; }
	if( cur <= max ) { return; }

	const f32 scale = max / cur;
	x *= scale;
	y *= scale;
	z *= scale;
}

inline ycVec3 ycVec3::RaisedToMag( f32 min )
{
	ycVec3 result = *this;
	result.RaiseToMag( min );
	return result;
}

inline ycVec3 ycVec3::LoweredToMag( f32 max )
{
	ycVec3 result = *this;
	result.LowerToMag( max );
	return result;
}

inline f32 ycVec3::DistSq( const ycVec3& other ) const
{
	return ycVec3( x - other.x, y - other.y, z - other.z ).LengthSq();
}

inline f32 ycVec3::Dist( const ycVec3& other ) const
{
	return ycSqrt( DistSq( other ) );
}

inline f32 ycVec3::Normalize()
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { /*assume all components are already zero*/ return 0.0f; }
	const f32 length = ycSqrt( lengthSq );
	const f32 invLengthSq = 1.0f / length;
	x *= invLengthSq;
	y *= invLengthSq;
	z *= invLengthSq;
	return length;
}

inline ycVec3 ycVec3::Normalized() const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { return ycVec3::ZERO(); }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycVec3(
		x * invLengthSq,
		y * invLengthSq,
		z * invLengthSq
	);
}

inline ycVec3 ycVec3::NormalizedOr( const ycVec3& fallbackIfTooSmall ) const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { return fallbackIfTooSmall; }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycVec3(
		x * invLengthSq,
		y * invLengthSq,
		z * invLengthSq
	);
}

inline ycVec3 ycVec3::NormalizedOr( f32 minMagSq, const ycVec3& fallbackIfTooSmall ) const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq < minMagSq ) ) { return fallbackIfTooSmall; }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycVec3(
		x * invLengthSq,
		y * invLengthSq,
		z * invLengthSq
	);
}

inline bool ycVec3::IsNormalized() const
{
	f32 tolerance = 0.0001f;
	return this->LengthSq() > (1.f - tolerance) && this->LengthSq() < (1.f + tolerance);
}

inline bool ycVec3::IsEqual( const ycVec3& other, f32 tolerance ) const
{
	return ( ( *this - other ).MagSq( ) <= tolerance );
}

inline ycVec3 ycVec3::Inverted() const
{
	return ycVec3( 1.0f / x, 1.0f / y, 1.0f / z );
}

inline ycVec3 ycVec3::SafeInverted() const
{
	return ycVec3(	x == 0.0f ? 0.0f : 1.0f / x,
					y == 0.0f ? 0.0f : 1.0f / y,
					z == 0.0f ? 0.0f : 1.0f / z	);
}

inline ycVec3 ycVec3::operator+( const ycVec3& rhs ) const
{
	return ycVec3( x+rhs.x, y+rhs.y, z+rhs.z );
}

inline ycVec3 ycVec3::operator-( const ycVec3& rhs ) const
{
	return ycVec3( x-rhs.x, y-rhs.y, z-rhs.z );
}

inline ycVec3 ycVec3::operator/( const f32 rhs ) const
{
	return ycVec3( x/rhs, y/rhs, z/rhs );
}

inline ycVec3 ycVec3::operator-() const
{
	return ycVec3( -x, -y, -z );
}

inline ycVec3& ycVec3::operator+=( const ycVec3& rhs )
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	return *this;
}

inline ycVec3& ycVec3::operator-=( const ycVec3& rhs )
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	return *this;
}


inline ycVec3& ycVec3::operator*=( const f32 rhs )
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	return *this;
}

inline ycVec3& ycVec3::operator/=( const f32 rhs )
{
	x /= rhs;
	y /= rhs;
	z /= rhs;
	return *this;
}

inline bool ycVec3::operator>( const ycVec3 & rhs ) const
{
	return x > rhs.x && y > rhs.y && z > rhs.z;
}

inline bool ycVec3::operator<( const ycVec3 & rhs ) const
{
	return x < rhs.x && y < rhs.y && z < rhs.z;
}

inline bool ycVec3::operator == ( const ycVec3& rhs ) const
{
	return Equals( rhs );
}

inline bool ycVec3::operator != ( const ycVec3& rhs ) const
{
	return !Equals( rhs );
}

inline ycVec3 operator*( const ycVec3& v, const f32 s )
{
	return ycVec3( v.x*s, v.y*s, v.z*s );
}

inline ycVec3 operator*( const f32 s, const ycVec3& v )
{
	return ycVec3( v.x*s, v.y*s, v.z*s );
}

inline ycVec3 ycVec3::operator*( const ycVec3& rhs ) const
{
	return ycVec3( x * rhs.x, y * rhs.y, z * rhs.z );
}

inline ycVec3 ycVec3::operator/( const ycVec3& rhs ) const
{
	return ycVec3( x / rhs.x, y / rhs.y, z / rhs.z );
}

inline ycVec3& ycVec3::operator*=( const ycVec3& rhs )
{
	x *= rhs.x;
	y *= rhs.y;
	z *= rhs.z;
	return *this;
}

inline ycVec3& ycVec3::operator/=( const ycVec3& rhs )
{
	x /= rhs.x;
	y /= rhs.y;
	z /= rhs.z;
	return *this;
}

inline void ycVec3::Negate()
{
	x = -x;
	y = -y;
	z = -z;
}

inline ycVec3 ycVec3::Negated() const
{
	return ycVec3( -x, -y, -z );
}

inline ycVec3 ycVec3::Mul( const ycVec3& rhs ) const
{
	return ycVec3( x*rhs.x, y*rhs.y, z*rhs.z );
}

inline ycVec3 ycVec3::ToXY() const
{
	return ycVec3( x, y, 0.0f );
}

inline ycVec3 ycVec3::ToYZ() const
{
	return ycVec3( 0.0f, y, z );
}

inline ycVec3 ycVec3::ToXZ() const
{
	return ycVec3( x, 0.0f, z );
}

inline ycVec3 ycVec3::ToX() const
{
	return ycVec3( x, 0.0f, 0.0f );
}

inline ycVec3 ycVec3::ToY() const
{
	return ycVec3( 0.0f, y, 0.0f );
}

inline ycVec3 ycVec3::ToZ() const
{
	return ycVec3( 0.0f, 0.0f, z );
}

inline f32 ycVec3::DistSqXY( const ycVec3& other ) const
{
	f32 dx = other.x - x;
	f32 dy = other.y - y;
	return dx * dx + dy * dy;
}

inline f32 ycVec3::DistXY( const ycVec3& other ) const
{
	return ycSqrt( DistSqXY( other ) );
}

inline ycVec3 ycVec3::Project( const ycVec3& n ) const
{
	return OntoUnit( n.Normalized() );
}

inline ycVec3 ycVec3::Onto( const ycVec3& n ) const
{
	return OntoUnit( n.Normalized() );
}

inline ycVec3 ycVec3::OntoUnit( const ycVec3& n ) const
{
	return n * n.Dot( *this );
}

inline ycVec3 ycVec3::Without( const ycVec3& n ) const
{
	return WithoutUnit( n.Normalized() );
}

inline ycVec3 ycVec3::WithoutUnit( const ycVec3& n ) const
{
	return (*this) - n.Dot( *this ) * n;
}

inline ycVec3 ycVec3::WithoutPositive( const ycVec3& n ) const
{
	return WithoutPositiveUnit( n.Normalized() );
}

inline ycVec3 ycVec3::WithoutPositiveUnit( const ycVec3& n ) const
{
	return *this - n * ycMax( 0.0f, n.Dot( *this ) );
}

inline ycVec3 ycVec3::Reflect( const ycVec3 & n ) const
{
	return *this - ( n * 2.0f * Dot( n ) );
}

inline ycVec3 ycVec3::Min( const ycVec3& a, const ycVec3& b )
{
	return ycVec3( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z );
}

inline ycVec3 ycVec3::Max( const ycVec3& a, const ycVec3& b )
{
	return ycVec3( a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y, a.z < b.z ? b.z : a.z );
}

inline ycVec3 ycVec3::Clamp( const ycVec3& v, const ycVec3& vmin, const ycVec3& vmax )
{
	return ycVec3( 
		v.x < vmin.x ? vmin.x : ( v.x > vmax.x ? vmax.x : v.x ),
		v.y < vmin.y ? vmin.y : ( v.y > vmax.y ? vmax.y : v.y ),
		v.z < vmin.z ? vmin.z : ( v.z > vmax.z ? vmax.z : v.z ) );
}

inline ycVec3 ycVec3::Abs() const
{
	return ycVec3( x < 0.0f ? -x : x, y < 0.0f ? -y : y, z < 0.0f ? -z : z );
}

inline ycVec3 ycVec3::Sign() const
{
	return ycVec3( x < 0.0f ? -1.0f : 1.0f, y < 0.0f ? -1.0f : 1.0f, z < 0.0f ? -1.0f : 1.0f );
}

inline ycVec3 ycVec3::SignZero() const
{
	return ycVec3( x ? (x < 0.0f ? -1.0f : 1.0f) : 0.0f, y ? (y < 0.0f ? -1.0f : 1.0f) : 0.0f, z ? (z < 0.0f ? -1.0f : 1.0f) : 0.0f );
}

inline ycVec3 ycVec3::AsAxis( ureg axis, f32 value )
{
	ycVec3 r( 0, 0, 0 );
	r.a[ axis ] = value;
	return r;
}

inline f32 ycVec3::FirstNonZeroAxis() const
{
	for( s32 i = 0; i < 3; ++i )
	{
		if( a[i] )
		{
			return a[i];
		}
	}
	return 0.0f;
}

inline u32 ycVec3::LongestAxis() const
{
	if( x > y ) 
	{
		return ( x > z ) ? 0 : 2;
	} 
	else 
	{
		return ( y > z ) ? 1 : 2;
	}
}

inline u32 ycVec3::LongestAxisAbs() const
{
	ycVec3 abs = (*this).Abs();
	if( abs.x > abs.y ) 
	{
		return ( abs.x > abs.z ) ? 0 : 2;
	} 
	else 
	{
		return ( abs.y > abs.z ) ? 1 : 2;
	}
}

inline ycVec3 ycVec3::AsLongestAxis() const
{
	ycVec3 axis = ycVec3::ZERO();
	axis.a[ LongestAxis() ] = 1.0f;
	return axis;
}

inline ycVec3 ycVec3::AsLongestAxisAbs() const
{
	ycVec3 axis = ycVec3::ZERO();
	axis.a[ LongestAxisAbs() ] = 1.0f;
	return axis;
}

#include "ycPopDebugOpt.inc"
