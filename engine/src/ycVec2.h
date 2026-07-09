#pragma once
#include "ycMath.h"

/*
\class ycVec2

2 axis vector handling

*/

#include "ycPushDebugOpt.inc"

struct ycVec2 //% 
{
yceditable:
	//! the data!
	union
	{
		struct { f32 x, y; };
		f32 a[ 2 ];
	};

public:
	//! default ctor
	ycVec2() = default;
	//! sets x, y to the same value
	constexpr ycVec2(f32 _xy) : x( _xy ), y( _xy ) {}
	//! sets x, y
	constexpr ycVec2(f32 _x, f32 _y) : x(_x), y(_y) {}

	static ycVec2 Max(const ycVec2 a, const ycVec2 b) { return ycVec2(a.x<b.x ? b.x:a.x, a.y<b.y ? b.y:a.y); }
	static ycVec2 Min(const ycVec2 a, const ycVec2 b) { return ycVec2(a.x>b.x ? b.x:a.x, a.y>b.y ? b.y:a.y); }
	ycVec2 Inv() const { return ycVec2(1.0f/x, 1.0f/y); }
	f32 Major() { f32 _a = x>=0.f ? x : -x, _b = y>0.f ? y : -y; return _a < _b ? _b : _a; }
	void Clear() { x = 0.0f; y = 0.0f; }

	inline f32 LengthSq() const { return x * x + y * y;  }
	inline f32 MagSq() const { return LengthSq(); }
	inline f32 Length() const { return ycSqrt( LengthSq() ); }
	inline f32 Mag() const { return Length(); }

	inline f32 DistSq( const ycVec2& v ) const;
	inline f32 Dist( const ycVec2& v ) const;

	inline f32 Cross( const ycVec2& other, const ycVec2& other2 ) const;
	inline f32 Cross( const ycVec2& other ) const;
	inline f32 Dot( const ycVec2& other ) const;

	inline ycVec2 ComplexProduct( const ycVec2& other ) const;

	//! normalization
	/*! Sets all components to zero for zero-length vectors */
	inline void Normalize();
	inline ycVec2 Normalized() const;
	inline bool IsNormalized() const;

	ycVec2 Abs() const { return ycVec2( ycAbs( x ), ycAbs( y ) ); }
	ycVec2 Floor() const { return ycVec2( ycFloor( x ), ycFloor( y ) ); }

	f32 SignedAngle( const ycVec2 &v ) const { return ycArcTan2( x * v.y - y * v.x, x * v.x + y * v.y ); }

	//! lerps between two vectors
	inline ycVec2 Lerp( const ycVec2& other, const f32 t ) const;
	//! lerps between two vectors
	inline ycVec2 Lerp( const ycVec2& other, const ycVec2& t ) const;
	//! lerps between two vectors and clamps t value
	inline ycVec2 LerpC( const ycVec2& other, const f32 t, const f32 min = 0.0f, const f32 max = 1.0f ) const;

	ycVec2 operator-() const { return ycVec2( -x, -y ); }
	void operator +=(const ycVec2 v) { x += v.x; y += v.y; }
	ycVec2 operator+(const ycVec2 v) const { return ycVec2( x+v.x, y+v.y ); }
	void operator -=(const ycVec2 v) { x -= v.x; y -= v.y; }
	ycVec2 operator-(const ycVec2 v) const { return ycVec2( x-v.x, y-v.y ); }
	void operator *=(const ycVec2 v) { x *= v.x; y *= v.y; }
	void operator *=(f32 s) { x *= s; y *= s; }
	ycVec2 operator*(const ycVec2 v) const { return ycVec2( x*v.x, y*v.y ); }
	bool operator==( const ycVec2& v ) const { return x == v.x && y == v.y; }
	bool operator!=( const ycVec2& v ) const { return x != v.x || y != v.y; }
	bool operator>( const ycVec2& v ) const { return x > v.x && y > v.y; }
	bool operator<( const ycVec2& v ) const { return x < v.x && y < v.y; }
	bool operator>=( const ycVec2& v ) const { return x >= v.x && y >= v.y; }
	bool operator<=( const ycVec2& v ) const { return x <= v.x && y <= v.y; }
	inline ycVec2  operator /  ( const f32   rhs ) const { return ycVec2( x/rhs, y/rhs ); };
	inline ycVec2& operator /= ( const f32   rhs ) { x /= rhs; y /= rhs; return *this; }
	inline ycVec2 operator/( const ycVec2& rhs ) const { return ycVec2( x / rhs.x, y / rhs.y ); }
	inline ycVec2& operator/=( const ycVec2& rhs ) { x /= rhs.x; y /= rhs.y; return *this; }

	bool Equals( const ycVec2& v ) const { return x == v.x && y == v.y; }
	bool Equals( const ycVec2& v, f32 epsilon ) const { return ycAbs( v.x - x ) < epsilon && ycAbs( v.y - y ) < epsilon; }

	constexpr static ycVec2 ZERO() { return ycVec2( 0, 0 ); }
	constexpr static ycVec2 ONE() { return ycVec2( 1, 1 ); }
};

inline ycVec2 operator*(f32 s, const ycVec2 v) { return ycVec2( s*v.x, s*v.y ); }
inline ycVec2 operator*(const ycVec2 v, f32 s) { return ycVec2( s*v.x, s*v.y ); }

f32 ycVec2::DistSq( const ycVec2& v ) const
{
	return ( *this - v ).MagSq();
}

f32 ycVec2::Dist( const ycVec2& v ) const
{
	return ( *this - v ).Mag();
}

void ycVec2::Normalize()
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { /*assume all components are already zero*/ return; }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	x *= invLengthSq;
	y *= invLengthSq;
}

ycVec2 ycVec2::Normalized() const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { return ycVec2::ZERO(); }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycVec2(
		x * invLengthSq,
		y * invLengthSq
	);
}

bool ycVec2::IsNormalized() const
{
	f32 tolerance = 0.000001f;
	return this->LengthSq() > (1.f - tolerance) && this->LengthSq() < (1.f + tolerance);
}

f32 ycVec2::Cross( const ycVec2& other, const ycVec2& other2 ) const
{
	return ((other.x - x)*(other2.y - y) - (other.y - y)*(other2.x - x));
}

f32 ycVec2::Cross( const ycVec2& other ) const
{
	return x*other.y - y*other.x;
}

f32 ycVec2::Dot( const ycVec2& other ) const
{
	return x*other.x + y*other.y;
}

ycVec2 ycVec2::ComplexProduct( const ycVec2& other ) const
{
	return ycVec2( x*other.x - y*other.y, x*other.y + y*other.x );
}

inline ycVec2 ycVec2::Lerp( const ycVec2& other, const f32 t ) const
{
	return ycVec2(
		ycLerp( x, other.x, t ),
		ycLerp( y, other.y, t )
	);
}

inline ycVec2 ycVec2::Lerp( const ycVec2& other, const ycVec2& t ) const
{
	return ycVec2(
		ycLerp( x, other.x, t.x ),
		ycLerp( y, other.y, t.y )
	);
}

inline ycVec2 ycVec2::LerpC( const ycVec2& other, const f32 t, const f32 min, const f32 max ) const
{
	f32 tc = ycClamp( t, min, max );
	return ycVec2(
		ycLerp( x, other.x, tc ),
		ycLerp( y, other.y, tc ) 
	);
}


#include "ycPopDebugOpt.inc"
