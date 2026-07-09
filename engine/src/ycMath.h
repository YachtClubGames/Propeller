#pragma once

#include "ycPlatformConfig.h"
#include "ycCommon.h"

#define YC_PI        3.141592654f
#define YC_PI_OVER_2 1.570796327f
#define YC_TWO_PI    6.283185307f

#define YC_RADIANS_PER_DEGREE 0.0174532925f
#define YC_DEGREES_PER_RADIAN 57.2957795f

#define YC_LOG2_8BIT( v )          ( 8 - 90/(((v)/4+14)|1) - 2/((v)/2+1) )
#define YC_LOG2_16BIT( v )         ( 8*((v)>255) + YC_LOG2_8BIT((v) >>8*((v)>255)) ) 
#define YC_LOG2_32BIT( v )         ( 16*((v)>65535L) + YC_LOG2_16BIT((v)*1L >>16*((v)>65535L)) )
#define YC_ROUNDUP_POW2_32BIT( x ) ( 1<<(YC_LOG2_32BIT((x)-1)+1) )

#define YC_INV_SQRT_2 0.707106781187f
#define YC_LN_2 0.69314718056f

#define YC_NUM_BITS_REQUIRED( maxPlusOne ) YC_LOG2_32BIT(YC_ROUNDUP_POW2_32BIT(maxPlusOne)) // eg (256) for 0-255

#define YC_F32_MIN				   0x1p-126f               // min normalized positive value, roughly 1.175494351e-38f
#define YC_F32_MAX				   0x1.fffffep+127f		   // max positive value, roughly 3.402823e+38f
#define YC_F32_EPSILON			   1.192092896e-07F
#define YC_U32_MIN				   0u
#define YC_U32_MAX				   0xFFFFFFFFu
#define YC_U64_MAX				   0xFFFFFFFFFFFFFFFFu
#define YC_S32_MIN				   0x80000000
#define YC_S32_MAX				   0x7FFFFFFF

typedef u16 f16;

struct ycVec2;
struct ycVec3;
struct ycVec4;
struct ycQuat;
struct ycMtx4;

inline f32 operator ""_degrees( long double degs ) { return (f32)degs * YC_RADIANS_PER_DEGREE; }
inline f32 operator ""_degrees( unsigned long long int degs ) { return (f32)degs * YC_RADIANS_PER_DEGREE; }

f32 ycInfinity();

template< class t_type >  // n/d
t_type ycRoundUpDiv( const t_type n, const t_type d ) { return (n + (d - 1)) / d; }

//! Rounds a number up a number to the next increment of base
u32 ycRoundUp( u32 value, u32 base );
s32 ycRoundUp( s32 value, s32 base );
f32 ycRoundUp( f32 value, f32 base );

//! Rounds a downto the next increment of base
f32 ycRoundDown( f32 value, f32 base );

//! Rounds a number up to a power of two
/*! If the number is already a power of two, it is returned unchanged */
uint32_t ycRoundUpPow2( uint32_t x );
#ifdef YC_64
ycSize_t ycRoundUpPow2( ycSize_t x );
#endif

//! Rounds a number up to a pow2 base
uint32_t ycRoundUpPow2( const uint32_t value, const uint32_t base );
#ifdef YC_64
ycSize_t ycRoundUpPow2( const ycSize_t value, const ycSize_t base );
#endif

void* ycAlignPtr( void* ptr, const ycSize_t base ); // may want to template this since we'll always need to cast back from void


inline bool ycGetBitLE( void* p, ureg bit ) { u8* buf = (u8*)p; return ( buf[ bit / 8 ] & ( 1 << ( bit % 8 ) ) ) != 0; }
inline void ycSetBitLE( void* p, ureg bit, bool flag )
{
	u8* buf = (u8*)p;
	if( flag ) { buf[ bit / 8 ] |=  ( 1 << ( bit % 8 ) ); }
	else       { buf[ bit / 8 ] &= ~( 1 << ( bit % 8 ) ); }
}

//! Returns true if a number is a power of two
bool ycIsPow2( const uint32_t num );
#ifdef YC_64
bool ycIsPow2( const ycSize_t num );
#endif

f32 ycLog2( f32 num );
f32 ycNaturalLog( f32 num );

template< class t_type >
t_type ycMin( const t_type a, const t_type b );

template< class t_type >
t_type ycMax( const t_type a, const t_type b );

template< class t_type >
t_type ycClamp( const t_type x, const t_type min, const t_type max );

template< class t_type >
t_type ycSaturate( const t_type x );

template< class t_type >
constexpr t_type ycDegToRad( const t_type x );

template< class t_type >
constexpr t_type ycRadToDeg( const t_type x );

//! WrapNear requires that the value is within { min-range, max+range } where range = max-min, such as adding two angles, faster than calling Wrap
inline f32 ycWrapNear( const f32 v, const f32 min, const f32 max ) { return v >= min ? (v < max ? v : (v - (max - min))) : (v + max - min); }
inline s32 ycWrapNear( const s32 v, const s32 min, const s32 max ) { return v >= min ? (v < max ? v : (v - (max - min))) : (v + max - min); }
inline f32 ycWrapNear( const f32 v, const f32 max ) { return v >= 0 ? (v < max ? v : (v - max)) : (v + max); }

inline f32 ycWrapRadNear( const f32 v, const f32 rad ) { return v > -rad ? (v < rad ? v : (v - 2 * rad)) : (v + 2 * rad); }
f32 ycWrapAngNear( const f32 radians );
f32 ycWrapAng( const f32 radians );
f32 ycWrap( const f32 v, const f32 min, const f32 max );
s32 ycWrap( const s32 v, const s32 min, const s32 max );
f32 ycWrap( const f32 v, const f32 max );
s32 ycWrap( const s32 v, const s32 max );
f32 ycMod( const f32 v, const f32 m );
f64 ycMod( const f64 v, const f64 m );
f32 ycModParts( const f32 x, f32* intptr );
f64 ycModParts( const f64 x, f64* intptr );
f32 ycSnap( const f32 v, const f32 g );
inline constexpr f32 ycAbs( const f32 x );
f64 ycAbs( const f64 x );
s32 ycAbs( const s32 x );
s64 ycAbs( const s64 x );
u32 ycAbs( const u32 x ); //!< asserts on use!
u64 ycAbs( const u64 x ); //!< asserts on use!

template< class t_type >
t_type ycSign( const t_type x );

//! Bit reinterpretation

union ycFloatAsInt
{
	f32 fp32;
	u32 ui32;
	s32 i32;
};

s32 ycFloatAsIntBits( const f32 val );
f32 ycIntAsFloatBits( const s32 val );

u32 ycFloatAsUIntBits( const f32 val );
f32 ycUIntAsFloatBits( const u32 val );

//! f16 conversion
f16 ycf32Tof16( f32 f );
f32 ycf16Tof32( f16 w );

//! Linear Interpolation

template< class v_type, class t_type >
v_type ycLerp( const v_type& a, const v_type& b, const t_type& t );

f32 ycGetLerpT( const f32 a, const f32 b, const f32 val );
f64 ycGetLerpT( const f64 a, const f64 b, const f64 val );

template< class v_type, class t_type >
v_type ycLerpClamped( const v_type& a, const v_type& b, const t_type& t );
f32 ycGetClampedLerpT( const f32 a, const f32 b, const f32 val );
f64 ycGetClampedLerpT( const f64 a, const f64 b, const f64 val );

f32 ycSmoothStep( const f32 a, const f32 b, const f32 val );

template< class t_type >
bool ycIsBetween( const t_type a, const t_type b, const t_type val );

template< class t_type >
t_type ycApproach( const t_type a, const t_type b, const t_type val );

// TODO: approx/fast sqrt, trig funcs (some platforms have builtins, could hand impl for others)
f32 ycSqrt( const f32 x );
f64 ycSqrt( const f64 x );
f32 ycInvSqrt( const f32 x );
inline f32 ycSquare( const f32 x ) { return x * x; }
f32 ycSin( const f32 x );
f64 ycSin( const f64 x );
f32 ycArcSin( const f32 x );
f32 ycCos( const f32 x );
f64 ycCos( const f64 x );
f32 ycArcCos( const f32 x );
f32 ycTan( const f32 x );
f32 ycArcTan2( const f32 y, const f32 x );
f64 ycArcTan2( const f64 y, const f64 x );
f32 ycArcTan( const f32 v );

f32 ycExp( const f32 x );
f32 ycExp2( const f32 x );

f32 ycPow( const f32 x, const f32 y );
f64 ycPowD( const f64 x, const f64 y );

f32 ycCeil( const f32 x );
f64 ycCeil( const f64 x );
f32 ycFloor( const f32 x );
f64 ycFloor( const f64 x );
f32 ycRound( const f32 x );
u32 ycRoundI( const f32 x );
f32 ycRound( const f32 x, const f32 base );
f64 ycRound( const f64 x );
bool ycIsZero( const f32 x, f32 tolerance = YC_F32_EPSILON );
bool ycIsAnyZero( const ycVec3& v, f32 tolerance = YC_F32_EPSILON );

//! Solve for the quadratic equation. returns number of solutions
u32 ycQuadratic( const f32 a, const f32 b, const f32 c, f32* sol1, f32* sol2 );

//! Endian swapping
u16 ycSwap16( const void* src );
u32 ycSwap32( const void* src );
u64 ycSwap64( const void* src );

bool ycIsNan( f32 v );
bool ycIsNan( const ycVec2& v );
bool ycIsNan( const ycVec3& v );
bool ycIsNan( const ycVec4& v );
bool ycIsNan( const ycQuat& v );
bool ycIsNan( const ycMtx4& v );

bool ycIsInf( f32 v );
bool ycIsInf( const ycVec2& v );
bool ycIsInf( const ycVec3& v );
bool ycIsInf( const ycVec4& v );
bool ycIsInf( const ycQuat& v );
bool ycIsInf( const ycMtx4& v );

bool ycIsNanOrInf( f32 v );
bool ycIsNanOrInf( const ycVec2& v );
bool ycIsNanOrInf( const ycVec3& v );
bool ycIsNanOrInf( const ycVec4& v );
bool ycIsNanOrInf( const ycQuat& v );
bool ycIsNanOrInf( const ycMtx4& v );

// implementations
template< class t_type >
t_type ycMin( const t_type a, const t_type b )
{
	return a < b ? a : b ;
}

template< class t_type >
t_type ycMax( const t_type a, const t_type b )
{
	return a > b ? a : b ;
}

template< class t_type >
t_type ycClamp( const t_type x, const t_type min, const t_type max )
{
	// clamp should imo clamp "between" the extents regardless of which is higher - callers
	// become more complicated if they can't assume this
	return ycMax( ycMin( min, max ), ycMin( ycMax( min, max ), x ) );
}

template< class t_type >
t_type ycSaturate( const t_type x )
{
	return ycClamp( x, t_type(0), t_type(1) );
}

template< class t_type >
constexpr t_type ycDegToRad( const t_type x )
{
	return YC_RADIANS_PER_DEGREE * x;
}

template< class t_type >
constexpr t_type ycRadToDeg( const t_type x )
{
	return YC_DEGREES_PER_RADIAN * x;
}

template< class t_type >
t_type ycSign( const t_type x )
{
	return x >= t_type(0) ? t_type(1) : t_type(-1) ;
}

template< class v_type, class t_type >
v_type ycLerp( const v_type& a, const v_type& b, const t_type& t )
{
	return ((b - a) * t) + a;
}

template< class v_type, class t_type >
v_type ycLerpClamped( const v_type& a, const v_type& b, const t_type& t )
{
	return ((b - a) * ycClamp( t, t_type( 0 ), t_type( 1 ) )) + a;
}

template< class t_type >
bool ycIsBetween( const t_type val, const t_type a, const t_type b )
{
	return ( b < a ) ?
	       ( b <= val && val <= a ) :
	       ( a <= val && val <= b ) ;
}

template< class t_type >
t_type ycApproach( const t_type val, const t_type goal, const t_type speed )
{
	if( val < goal )
	{
		t_type next = val + speed;
		if( next > goal ) { return goal; }
		return next;
	}
	else if( val > goal )
	{
		t_type next = val - speed;
		if( next < goal ) { return goal; }
		return next;
	}
	return val;
}

#if defined( YC_SIMD_SSE ) && defined( YC_64 )
	#if defined YC_MSVC
		#define _INC_MALLOC // prevent xmmintrin from pulling in malloc.. msft why?
		#include <xmmintrin.h>
		#include <emmintrin.h>
		#undef _INC_MALLOC
	#else
		#include <xmmintrin.h>
		#include <emmintrin.h>
	#endif

inline f32 ycSqrt( const f32 x )
{
	// Cmath sqrtf() is unfortunately a whole function call, since the standard requires it to do some checks.
	// This bit of inlineable sse yuck produces IEEE-compliant results but doesn't have compliant error handling.
	return  _mm_cvtss_f32( _mm_sqrt_ss( _mm_set_ss( x ) ) );
}

inline f32 ycSign( const f32 x )
{
	// for whatever reason the >= 0 test in float is producing a branch so doing the sign-bit OR here
	// note, using 0x7fffffff as sign mask because abs() uses the same mask so it can share a reg if they're in the same fn
	return _mm_cvtss_f32(_mm_or_ps( _mm_set_ss( 1.0f ),
		_mm_andnot_ps(_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)),  _mm_set_ss(x) ) )); 
}
#endif

inline f32 ycInvSqrt( const f32 x )
{
	return 1.0f / ycSqrt( x );
}

inline constexpr f32 ycAbs( const f32 x )
{
	return x > 0 ? x : -x;
}
