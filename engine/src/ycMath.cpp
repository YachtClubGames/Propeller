#include "ycMath.h"

// 3rdparty
#include "math.h" // sqrt
#include "ycVec2.h"
#include "ycVec3.h"
#include "ycVec4.h"
#include "ycQuat.h"
#include "ycMtx4.h"
//#include "ycSimd.h"

f32 ycInfinity()
{
	return INFINITY;
}

u32 ycRoundUp( u32 value, u32 base )
{
	return ( ( value + ( base - 1 ) ) / base ) * base;
}

s32 ycRoundUp( s32 value, s32 base )
{
	return ( ( value + ( base - 1 ) ) / base ) * base;
}

f32 ycRoundDown( f32 value, f32 base )
{
	f32 sign = ycSign( value );
	value = ycAbs( value );
	f32 remain = ycMod( value, base );
	value = value - remain;
	return sign * value;
}

f32 ycRoundUp( f32 value, f32 base )
{
	return ycRoundDown( value+base, base );
}

uint32_t ycRoundUpPow2( uint32_t x )
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	++x;
	return x;
}

#ifdef YC_64
ycSize_t ycRoundUpPow2( ycSize_t x )
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	++x;
	return x;
}
#endif

uint32_t ycRoundUpPow2( const uint32_t value, const uint32_t base )
{
	return (value + (base-1)) & ~(base-1);
}

#ifdef YC_64
ycSize_t ycRoundUpPow2( const ycSize_t value, const ycSize_t base )
{
	return (value + (base-1)) & ~(base-1);
}
#endif

void* ycAlignPtr( void* ptr, const ycSize_t base )
{
	return (void*)ycRoundUpPow2( uintptr_t(ptr), base );
}

bool ycIsPow2( const uint32_t num )
{
	return (num & (num-1)) == 0;
}

f32 ycLog2( f32 num )
{
	return log2f( num );
}

f32 ycNaturalLog( f32 num )
{
	return logf( num );
}

#ifdef YC_64
bool ycIsPow2( const ycSize_t num )
{
	return (num & (num-1)) == 0;
}
#endif

f32 ycWrapAngNear( const f32 radians )
{
	return ycWrapNear( radians, -YC_PI, YC_PI );
}

f32 ycWrapAng( const f32 radians )
{
	return ycWrap( radians, -YC_PI, YC_PI );
}

f32 ycWrap( const f32 v, const f32 min, const f32 max )
{
	f32 w = fmodf( v - min, max - min );
	return w >= 0.0 ? ( w + min ) : ( w + max );
}

s32 ycWrap( const s32 v, const s32 min, const s32 max )
{
	s32 w = ( v - min ) % ( max - min );
	return w >= 0 ? ( w + min ) : ( w + max );
}

f32 ycWrap( const f32 v, const f32 max )
{
	f32 w = fmodf( v, max );
	return w >= 0.0 ? w : ( w + max );
}

s32 ycWrap( const s32 v, const s32 max )
{
	s32 w = v % max;
	return w >= 0 ? w : ( w + max );
}

f32 ycMod( const f32 v, const f32 m )
{
	return fmodf( v, m );
}

f64 ycMod( const f64 v, const f64 m )
{
	return fmod( v, m );
}

f32 ycModParts( const f32 x, f32* intptr )
{
	return modff( x, intptr );
}

f64 ycModParts( const f64 x, f64* intptr )
{
	return modf( x, intptr );
}

f32 ycSnap( const f32 v, const f32 g )
{
	// half step
	f32 h = ycSign( v ) * 0.5f * g;
	// snap to grid
	return v + h - fmodf( v + h, g );
}

f64 ycAbs( const f64 x )
{
	return fabs( x );
}

s32 ycAbs( const s32 x )
{
	return x > 0 ? x : -x ;
}

s64 ycAbs( const s64 x )
{
	return x > 0 ? x : -x ;
}

u32 ycAbs( const u32 x )
{
	ycFatal(); // abs on this type makes no sense!
	return x;
}

u64 ycAbs( const u64 x )
{
	ycFatal(); // abs on this type makes no sense!
	return x;
}

s32 ycFloatAsIntBits( const f32 val )
{
	ycFloatAsInt tmp;
	tmp.fp32 = val;
	return tmp.i32;
}

f32 ycIntAsFloatBits( const s32 val )
{
	ycFloatAsInt tmp;
	tmp.i32 = val;
	return tmp.fp32;
}

u32 ycFloatAsUIntBits( const f32 val )
{
	ycFloatAsInt tmp;
	tmp.fp32 = val;
	return tmp.ui32;
}

f32 ycUIntAsFloatBits( const u32 val )
{
	ycFloatAsInt tmp;
	tmp.ui32 = val;
	return tmp.fp32;
}

f16 ycf32Tof16( f32 f )
{
	ycFloatAsInt fi;
	fi.fp32 = f;
	u16 r = (fi.ui32 >> 31) << 5;		// sign
	u16 t = (fi.ui32 >> 23) & 0xff;	// exponent
	t = (t - 0x70) & ((unsigned int)((int)(0x70 - t) >> 4) >> 27);	// exponent bias
	r = (r | t) << 10;			// new sign+exponent
	r |= (fi.ui32 >> 13) & 0x3ff;		// mantissa
	return f16(r);
}

f32 ycf16Tof32( f16 w )
{
	ycFloatAsInt fi;
	u32 t1 = ((w & 0x7fff) << 13) + 0x38000000;	// shift up mantissa + exponent and add bias
	u32 t2 = (w & 0x8000) << 16;				// sign bit
	if( (w & 0x7c00) == 0 )						// 0 exponent => denormal as 0
		fi.ui32 = t2;
	else
		fi.ui32 = t1 | t2;
	return fi.fp32;
}

f32 ycGetLerpT( const f32 a, const f32 b, const f32 val )
{
	f32 range = b - a;
	return ( val - a ) / (ycSign( range ) * ycMax( ycAbs( range ), 0.00000001f ));
}

f64 ycGetLerpT( const f64 a, const f64 b, const f64 val )
{
	f64 range = b - a;
	return (val - a) / (ycSign( range ) * ycMax( ycAbs( range ), 0.00000001 ));
}

f32 ycGetClampedLerpT( const f32 a, const f32 b, const f32 val )
{
	return ycSaturate( ycGetLerpT( a, b, val ) );
}

f64 ycGetClampedLerpT( const f64 a, const f64 b, const f64 val )
{
	return ycSaturate( ycGetLerpT( a, b, val ) );
}

f32 ycSmoothStep( const f32 a, const f32 b, const f32 val )
{
	f32 t = ycSaturate( ycGetLerpT( a, b, val ) );
	return t * t * (3 - 2 * t);
}

#if YC_NDA
#endif

f64 ycSqrt( const f64 x )
{
	return sqrt( x );
}

f32 ycSin( const f32 x )
{
	return sinf( x );
}

f64 ycSin( const f64 x )
{
	return sin( x );
}

f32 ycArcSin( const f32 x )
{
	return asinf( x );
}

f32 ycCos( const f32 x )
{
	return cosf( x );
}

f64 ycCos( const f64 x )
{
	return cos( x );
}

/*static*/f32 ycArcCos( const f32 x )
{
	return acosf( x );
}

f32 ycTan( const f32 x )
{
	return tanf( x );
}

f32 ycArcTan2( const f32 y, const f32 x )
{
	return atan2f( y, x );
}

f64 ycArcTan2( const f64 y, const f64 x )
{
	return atan2( y, x );
}

f32 ycArcTan( const f32 v )
{
	return atanf( v );
}

f32 ycExp( const f32 x )
{
	return expf( x );
}

f32 ycExp2( const f32 x )
{
	return exp2f( x );
}

f32 ycPow( const f32 x, const f32 y )
{
	return powf( x, y );
}

f64 ycPowD( const f64 x, const f64 y )
{
	return pow( x, y );
}

f32 ycCeil(const f32 x)
{
	return ceilf( x );
}

f64 ycCeil( const f64 x )
{
	return ceil( x );
}

f32 ycFloor(const f32 x)
{
	return floorf(x);
}

f64 ycFloor( const f64 x )
{
	return floor( x );
}

f32 ycRound( const f32 x )
{
	return roundf( x );
}

u32 ycRoundI( const f32 x )
{
	return lroundf( x );
}

f32 ycRound( const f32 x, const f32 base )
{
	return roundf( x / base ) * base;
}

f64 ycRound( const f64 x )
{
	return round( x );
}

bool ycIsZero( const float x, f32 tolerance ) { return ycAbs( x ) < tolerance; }
bool ycIsAnyZero( const ycVec3& v, f32 tol ) { return ycIsZero( v.x, tol ) || ycIsZero( v.y, tol ) || ycIsZero( v.z, tol ); }

u32 ycQuadratic( const f32 a, const f32 b, const f32 c, f32 * sol1, f32 * sol2 )
{
	f32 discriminant  = b * b - 4.0f * a * c;
	if( discriminant  <= -YC_F32_EPSILON )
	{
		return 0;
	}
	f32 denom = 1.0f / (2.0f * a);
	if( discriminant  < YC_F32_EPSILON )
	{
		*sol1 = -b * denom;
		return 1;
	}
	discriminant  = ycSqrt( discriminant  );
	*sol1 = (-b + discriminant ) * denom;
	*sol2 = (-b - discriminant ) * denom;
	return 2;
}

u16 ycSwap16( const void* pSrc )
{
	const u16 src = *( (u16*)pSrc );
	return (src << 8) | (src >> 8);
}

u32 ycSwap32( const void* pSrc )
{
	const u32 src = *( (u32*)pSrc );
	return ( (src << 24) | ((src << 8) & 0x00FF0000) | ((src >> 8) & 0x0000FF00) | (src >> 24) );
}

u64 ycSwap64( const void* pSrc )
{
	const u64 src = *( (u64*)pSrc );
	return ( (src >> 56) ) |  ( (src << 40) & 0x00FF000000000000LL ) | ( (src << 24) & 0x0000FF0000000000LL ) | ( (src << 8 ) & 0x000000FF00000000LL ) |
	       ( (src >> 8 ) & 0x00000000FF000000LL ) | ( (src >> 24) & 0x0000000000FF0000LL ) | ( (src >> 40) & 0x000000000000FF00LL ) | ( (src << 56) )  ;
}

bool ycIsNan( f32 v ) { return v != v; }
bool ycIsNan( const ycVec2& v ) { return ycIsNan( v.x ) || ycIsNan( v.y ); }
bool ycIsNan( const ycVec3& v ) { return ycIsNan( v.x ) || ycIsNan( v.y ) || ycIsNan( v.z ); }
bool ycIsNan( const ycVec4& v ) { return ycIsNan( v.x ) || ycIsNan( v.y ) || ycIsNan( v.z ) || ycIsNan( v.w ); }
bool ycIsNan( const ycQuat& v ) { return ycIsNan( v.x ) || ycIsNan( v.y ) || ycIsNan( v.z ) || ycIsNan( v.w ); }
bool ycIsNan( const ycMtx4& v ) { return ycIsNan( v.x ) || ycIsNan( v.y ) || ycIsNan( v.z ) || ycIsNan( v.t ); }

bool ycIsInf( f32 v ) { return isinf( v ); }
bool ycIsInf( const ycVec2& v ) { return ycIsInf( v.x ) || ycIsInf( v.y ); }
bool ycIsInf( const ycVec3& v ) { return ycIsInf( v.x ) || ycIsInf( v.y ) || ycIsInf( v.z ); }
bool ycIsInf( const ycVec4& v ) { return ycIsInf( v.x ) || ycIsInf( v.y ) || ycIsInf( v.z ) || ycIsInf( v.w ); }
bool ycIsInf( const ycQuat& v ) { return ycIsInf( v.x ) || ycIsInf( v.y ) || ycIsInf( v.z ) || ycIsInf( v.w ); }
bool ycIsInf( const ycMtx4& v ) { return ycIsInf( v.x ) || ycIsInf( v.y ) || ycIsInf( v.z ) || ycIsInf( v.t ); }

bool ycIsNanOrInf( f32 v )			 { return ycIsNan( v ) || ycIsInf( v ); }
bool ycIsNanOrInf( const ycVec2& v ) { return ycIsNan( v ) || ycIsInf( v ); }
bool ycIsNanOrInf( const ycVec3& v ) { return ycIsNan( v ) || ycIsInf( v ); }
bool ycIsNanOrInf( const ycVec4& v ) { return ycIsNan( v ) || ycIsInf( v ); }
bool ycIsNanOrInf( const ycQuat& v ) { return ycIsNan( v ) || ycIsInf( v ); }
bool ycIsNanOrInf( const ycMtx4& v ) { return ycIsNan( v ) || ycIsInf( v ); }

