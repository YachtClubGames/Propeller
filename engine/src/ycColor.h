#pragma once

#include "ycCommon.h"
#include "ycIsPOD.h"
#include "ycMath.h"

#include "ycPushDebugOpt.inc"

enum ycColorSpace : u8 //%
{
	kColorSpace_sRGB,       // your typical web/hex color components
	kColorSpace_Linear,     // non-gamma-corrected colors with linear luminance
	kColorSpace_HSV,        // hue/saturation/value
	kColorSpace_Oklab,      // perceptual color, accounting for interactions between hue and perceived luminance
	kColorSpace_LinearSqrt, // approximate linear with sqrt -- only use if perf matters
	kColorSpace_Count
};

struct ycColor //%
{	
yceditable:
	//! the data!
	union
	{
		struct { u8 r, g, b, a; };
		u32 color; // type-punning. OK in C but technically UB in C++ (though all our compilers apply C semantics)
		u8 arr[4];
	};

public:

	//! default ctor
	YC_INLINE ycColor() = default;
	//! sets red green blue alpha
	constexpr YC_INLINE ycColor( const u8 _r, const u8 _g, const u8 _b, const u8 _a ) : r(_r), g(_g), b(_b), a(_a) {}

	// useful? (some older hardware uses different ordering for vertex and pixel colors...)
	//YC_INLINE u32 GetARGB() const { ycFatal(); return 0; }
	//YC_INLINE u32 GetRGBA() const { ycFatal(); return 0; }

	//! Operators
	YC_INLINE bool operator == ( const ycColor& rhs ) const { return r==rhs.r && g==rhs.g && b==rhs.b && a==rhs.a; }
	YC_INLINE bool operator != ( const ycColor& rhs ) const { return r!=rhs.r || g!=rhs.g || b!=rhs.b || a!=rhs.a; }
	YC_INLINE ycColor operator * ( const ycColor& rhs ) const { return ycColor(
		u8( ureg(r) * ureg(rhs.r) / 255 ),
		u8( ureg(g) * ureg(rhs.g) / 255 ),
		u8( ureg(b) * ureg(rhs.b) / 255 ),
		u8( ureg(a) * ureg(rhs.a) / 255 )
	); }
	YC_INLINE ycColor& operator *= ( const ycColor& rhs ) 
	{ 
		r = u8( ureg(r) * ureg(rhs.r) / 255 );
		g = u8( ureg(g) * ureg(rhs.g) / 255 );
		b = u8( ureg(b) * ureg(rhs.b) / 255 );
		a = u8( ureg(a) * ureg(rhs.a) / 255 );
		return *this;
	}

	//! Convenience values
	constexpr YC_INLINE static ycColor ZERO() { return ycColor( 0, 0, 0, 0 ); }
	constexpr YC_INLINE static ycColor OPAQUE_BLACK()      { return ycColor( 0, 0, 0, 255 ); }
	constexpr YC_INLINE static ycColor TRANSPARENT_BLACK() { return ycColor( 0, 0, 0, 0 ); }
	constexpr YC_INLINE static ycColor OPAQUE_WHITE()      { return ycColor( 255, 255, 255, 255 ); }
	constexpr YC_INLINE static ycColor TRANSPARENT_WHITE() { return ycColor( 255, 255, 255, 0 ); }
	
	constexpr YC_INLINE static ycColor RED()   { return ycColor( 255, 0, 0, 255 ); }
	constexpr YC_INLINE static ycColor GREEN() { return ycColor( 0, 255, 0, 255 ); }
	constexpr YC_INLINE static ycColor BLUE()  { return ycColor( 0, 0, 255, 255 ); }

	constexpr YC_INLINE static ycColor CYAN()    { return ycColor( 0, 255, 255, 255 ); }
	constexpr YC_INLINE static ycColor MAGENTA() { return ycColor( 255, 0, 255, 255 ); }
	constexpr YC_INLINE static ycColor YELLOW()  { return ycColor( 255, 255, 0, 255 ); }

	constexpr YC_INLINE static ycColor ORANGE() { return ycColor( 255, 128, 0, 255 ); }

	constexpr YC_INLINE static ycColor XAXIS() { return ycColor( 255, 0, 0, 255 ); }
	constexpr YC_INLINE static ycColor YAXIS() { return ycColor( 0, 255, 0, 255 ); }
	constexpr YC_INLINE static ycColor ZAXIS() { return ycColor( 0, 0, 255, 255 ); }

	ycColor Lerp( ycColor b, f32 t ) const;
	ycColor MulAlpha( u8 alpha ) const { return *this * ycColor{ 255, 255, 255, alpha }; }
	ycColor MulAlpha( f32 alpha ) const { return MulAlpha( (u8)( alpha * 255.f ) ); }
	u8 MinChannel() const { return ycMin( ycMin( r, g ), ycMin( b, a ) ); }
	u8 MaxChannel() const { return ycMax( ycMax( r, g ), ycMax( b, a ) ); }
	void SetRGB( ycColor color );

	ycColor Lighter( f32 t ) const;
	ycColor Darker( f32 t ) const;

	//! Conversion
	static ycColor From565( const u16 rgb );
	u16 To565() const;

	u32 ToR8G8B8A8() const { return r << 24 | g << 16 | b << 8 | a; }
	u32 ToA8B8G8R8() const { return color; }

	static ycColor FromA8B8G8R8( u32 _color ) { ycColor result; result.color = _color; return result; }

	// mixes colors in the target-space assuming that a and b are sRGB
	static inline ycColor Mix( ycColor a, ycColor b, f32 t, ycColorSpace mixSpace = kColorSpace_Linear );
};

// Ref: http://cbloomrants.blogspot.com/2020/09/topics-in-quantization-for-games.html
#define YC_F32_TO_U8(ch) (ch <= 0.0f ? u8(0) : ch >= 1.0f ? u8(255) : u8(ch * 255.0f + 0.5f))
#define YC_U8_TO_F32(ch) (f32(ch)/255.0f)

struct ycColorF //%
{
yceditable:
	union
	{
		struct { f32 r, g, b, a; };
		struct { f32 h, s, v; };
		struct { f32 okl, oka, okb; };
		struct { f32 color[4]; };
	};

public:

	YC_INLINE ycColorF() = default;
	constexpr YC_INLINE ycColorF( const f32 _r, const f32 _g, const f32 _b, const f32 _a ) : r(_r), g(_g), b(_b), a(_a) {}
	YC_INLINE ycColorF( const ycColor& color ) : r( YC_U8_TO_F32 (color.r) ), g( YC_U8_TO_F32(color.g) ), b( YC_U8_TO_F32(color.b) ), a( YC_U8_TO_F32(color.a) ) {}

	YC_INLINE operator ycColor() const { return ycColor( YC_F32_TO_U8(r), YC_F32_TO_U8(g), YC_F32_TO_U8(b), YC_F32_TO_U8(a) ); }

	YC_INLINE bool operator == ( const ycColorF& rhs ) const { return r==rhs.r && g==rhs.g && b==rhs.b && a==rhs.a; }
	YC_INLINE bool operator != ( const ycColorF& rhs ) const { return r!=rhs.r || g!=rhs.g || b!=rhs.b || a!=rhs.a; }
	YC_INLINE ycColorF operator * ( const ycColorF& rhs ) const { return ycColorF( r*rhs.r, g*rhs.g, b*rhs.b, a*rhs.a ); }
	YC_INLINE ycColorF operator * ( f32 k ) const { 	return ycColorF( r * k , g * k, b * k, a * k ); }
	YC_INLINE ycColorF& operator *= ( const ycColorF& rhs )
	{ 
		r = (r * rhs.r);
		g = (g * rhs.g);
		b = (b * rhs.b);
		a = (a * rhs.a);
		return *this;
	}
	YC_INLINE ycColorF operator + ( const ycColorF& rhs ) const { return ycColorF(
		(r + rhs.r),
		(g + rhs.g),
		(b + rhs.b),
		(a + rhs.a)
	); }
	YC_INLINE ycColorF& operator += ( const ycColorF& rhs )
	{
		r = ( r + rhs.r );
		g = ( g + rhs.g );
		b = ( b + rhs.b );
		a = ( a + rhs.a );
		return *this;
	}

	YC_INLINE void ToFloat4( f32* dst ) const { dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a; }

	//! Convenience values
	constexpr YC_INLINE static ycColorF OPAQUE_BLACK()      { return ycColorF( 0.0f, 0.0f, 0.0f, 1.0f ); }
	constexpr YC_INLINE static ycColorF TRANSPARENT_BLACK() { return ycColorF( 0.0f, 0.0f, 0.0f, 0.0f ); }
	constexpr YC_INLINE static ycColorF OPAQUE_WHITE()      { return ycColorF( 1.0f, 1.0f, 1.0f, 1.0f ); }
	constexpr YC_INLINE static ycColorF TRANSPARENT_WHITE() { return ycColorF( 1.0f, 1.0f, 1.0f, 0.0f ); }
	
	constexpr YC_INLINE static ycColorF RED()   { return ycColorF( 1.0f, 0, 0, 1.0f ); }
	constexpr YC_INLINE static ycColorF GREEN() { return ycColorF( 0, 1.0f, 0, 1.0f ); }
	constexpr YC_INLINE static ycColorF BLUE()  { return ycColorF( 0, 0, 1.0f, 1.0f ); }

	ycColorF Saturate() const { return ycColorF( ycSaturate(r), ycSaturate(g), ycSaturate(b), ycSaturate(a) ); }

	ycColorF Lerp( ycColorF b, f32 alpha ) const;

	//! Convert between color spaces. Alpha is unchanged.
	
	ycColorF RGBToHSV() const;
	ycColorF HSVToRGB() const;
	
	ycColorF RGBToLinear() const;
	ycColorF LinearToRGB() const;

	ycColorF LinearToOklab() const;
	ycColorF OklabToLinear() const;

	YC_INLINE ycColorF RGBToLinearSqrt() const { return ycColorF( r*r, g*g, b*b, a ); }
	YC_INLINE ycColorF LinearSqrtToRGB() const { return ycColorF( ycSqrt(r), ycSqrt(g), ycSqrt(b), a ); }

	// mixes colors in the target-space assuming that a and b are sRGB
	static ycColorF Mix( const ycColorF& a, const ycColorF& b, f32 t, ycColorSpace argSpace = kColorSpace_sRGB, ycColorSpace mixSpace = kColorSpace_Linear );
};


/* static */ inline ycColor ycColor::Mix( ycColor a, ycColor b, f32 t, ycColorSpace mixSpace )
{
	if( t < 1e-4f ) { return a; }
	if( t > 0.9999f ) { return b; }
	return mixSpace == kColorSpace_sRGB ? a.Lerp( b, ycSaturate( t ) ) : ycColor( ycColorF::Mix( a, b, t, kColorSpace_sRGB, mixSpace ) );
}

YC_IS_POD( ycColor );
YC_IS_POD( ycColorF );

#include "ycPopDebugOpt.inc"
