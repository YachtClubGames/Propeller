#include "ycColor.h"
#include "ycMath.h"
#include "ycStd.h"

ycColor ycColor::Lerp( ycColor _b, f32 alpha ) const
{
	ycColorF af( *this );
	ycColorF bf( _b );
	return ycColor( (u8)(255.0f * ycLerp( af.r, bf.r, alpha )),
					(u8)(255.0f * ycLerp( af.g, bf.g, alpha )),
					(u8)(255.0f * ycLerp( af.b, bf.b, alpha )),
					(u8)(255.0f * ycLerp( af.a, bf.a, alpha )) );
}

ycColor ycColor::Lighter( f32 t ) const
{
	ycColor ret = Lerp( ycColor::OPAQUE_WHITE(), t );
	ret.a = a;
	return ret;
}

ycColor ycColor::Darker( f32 t ) const
{
	ycColor ret = Lerp( ycColor::TRANSPARENT_BLACK(), t );
	ret.a = a;
	return ret;
}

void ycColor::SetRGB( ycColor colorRGB )
{
	r = colorRGB.r;
	g = colorRGB.g;
	b = colorRGB.b;
}

/*static*/ ycColor ycColor::From565( const u16 rgb )
{
	const u8 r5 = u8( ( rgb >> 11 ) & 0x1f );
	const u8 g6 = u8( ( rgb >>  5 ) & 0x3f );
	const u8 b5 = u8( ( rgb >>  0 ) & 0x1f );

	// linearly map from 565 to 888, matching a rounded float version
	/* note:
	many implementations use another method with slightly different results
	(c<<3) | (c>>2) // 5 bit color
	(c<<2) | (c>>4) // 6 bit color
	it's probably documented somewhere, but i am not sure why! maybe because it doesnt require
	using larger intermediate registers? (may be important for hardware implementation)
	*/
	const u8 r8 = u8( ( r5 * 527 + 23 ) >> 6 );
	const u8 g8 = u8( ( g6 * 259 + 33 ) >> 6 );
	const u8 b8 = u8( ( b5 * 527 + 23 ) >> 6 );

	return ycColor( r8, g8, b8, 255 );
}

u16 ycColor::To565() const
{
	//ycAssert( a == 255 ); // 565 doesn't do alpha!
	#if 0 // should probably do this, which is equivalent to rounding instead of flooring
		const u8 r5 = ( r * 249 + 1014 ) >> 11;
		const u8 g6 = ( g * 253 +  505 ) >> 10;
		const u8 b5 = ( b * 249 + 1014 ) >> 11;
	#endif
	const u8 r5 = u8( r >> 3 );
	const u8 g6 = u8( g >> 2 );
	const u8 b5 = u8( b >> 3 );
	return u16(r5<<11) | u16(g6<<5) | u16(b5);
}

ycColorF ycColorF::Lerp( ycColorF _b, f32 alpha ) const
{
	return ycColorF( ycLerp( r, _b.r, alpha ),
					ycLerp( g, _b.g, alpha ),
					ycLerp( b, _b.b, alpha ),
					ycLerp( a, _b.a, alpha ) );
}

ycColorF ycColorF::RGBToHSV() const
{
	f32 X = ycMax( r, ycMax( g, b ) );
	f32 N = ycMin( r, ycMin( g, b ) );
	f32 D = X-N;

	ycColorF ret( 0.0f, X>YC_F32_MIN ? D / X : 0.0f, X, a );
	if( D <= YC_F32_MIN )
	{
		ret.r = 0.0f; 
	}
	else if( r > g )
	{
		if( b > r )
		{
			ret.r = ((r - g) / D + 4.0f) / 6.0f;
		}
		else
		{
			ret.r = ((g - b) / D) / 6.0f;
			if( ret.r < 0.0f ) { ret.r += 1.0f; }
		}
	}
	else if( b > r )
	{
		ret.r = ((r - g) / D + 4.0f) / 6.0f;
	}
	else
	{
		ret.r = ((b - r) / D + 2.0f) / 6.0f;
	}
	return ret;
}

ycColorF ycColorF::HSVToRGB() const
{
	// note: this ensures hue = 255 != hue = 0
	const float kHueToRange = 6.0f - 1.0f/255.0f;
	const float kHueLoopPoint = 1.0f + 1.0f / 256.0f;

	f32 C = g * b;
	f32 X = C * (1.0f - ycAbs(ycMod( r * kHueToRange, 2.0f )-1.0f ) );
	f32 m = b - C;

	// hue is split into 6 segments repeating around 0.0 - 1.0 where 0.0 is not the same as 1.0
	f32 hue = r;
	if( hue < 0 ) { hue += ycCeil( hue ) + 1.0f; }
	else if( hue > kHueLoopPoint ) { hue = ycMod( hue, 1.0f ); }
	if( hue >= ( 5.0f / 6.0f ) ) { return ycColorF( C+m, 0+m, X+m, a ); }
	if( hue >= ( 4.0f / 6.0f ) ) { return ycColorF( X+m, 0+m, C+m, a ); }
	if( hue >= ( 3.0f / 6.0f ) ) { return ycColorF( 0+m, X+m, C+m, a ); }
	if( hue >= ( 2.0f / 6.0f ) ) { return ycColorF( 0+m, C+m, X+m, a ); }
	if( hue >= ( 1.0f / 6.0f ) ) { return ycColorF( X+m, C+m, 0+m, a ); }
	return ycColorF( C+m, X+m, 0+m, a );
}

namespace
{
	f32 _RGBToLinear( f32 c ) { return c < 0.04045f ? c / 12.92f : ycPow( ( c + 0.055f ) / 1.055f, 2.4f ); }
	f32 _LinearToRGB( f32 c ) { return c > 0.0031308f ? 1.055f * ycPow( c, 1.0f / 2.4f ) - 0.055f : c * 12.92f; }
}

ycColorF ycColorF::RGBToLinear() const
{
	return ycColorF( _RGBToLinear( r ), _RGBToLinear( g ), _RGBToLinear( b ), a );
}

ycColorF ycColorF::LinearToRGB() const
{
	return ycColorF( _LinearToRGB( r ), _LinearToRGB( g ), _LinearToRGB( b ), a );
}

ycColorF ycColorF::LinearToOklab() const
{
	const f32 L = ycPow( 0.41222147079999993f * r + 0.5363325363f * g + 0.0514459929f * b, 1.0f / 3.0f );
	const f32 M = ycPow( 0.2119034981999999f * r + 0.6806995450999999f * g + 0.1073969566f * b, 1.0f / 3.0f );
	const f32 S = ycPow( 0.08830246189999998f * r + 0.2817188376f * g + 0.6299787005000002f * b, 1.0f / 3.0f );
	return ycColorF( 
		0.2104542553f * L + 0.793617785f * M - 0.0040720468f * S,
		1.9779984951f * L - 2.428592205f * M + 0.4505937099f * S,
		0.0259040371f * L + 0.7827717662f * M - 0.808675766f * S,
		a
	);
}

ycColorF ycColorF::OklabToLinear() const
{
	const f32 L = ycPow( 0.99999999845051981432f * okl + 0.39633779217376785678f * oka + 0.21580375806075880339f * okb, 3 );
	const f32 M = ycPow( 1.0000000088817607767f * okl - 0.1055613423236563494f * oka - 0.063854174771705903402f * okb, 3 );
	const f32 S = ycPow( 1.0000000546724109177f * okl - 0.089484182094965759684f * oka - 1.2914855378640917399f * okb, 3 );
	return ycColorF(
		4.076741661347994f * L - 3.307711590408193f * M + 0.230969928729428f * S,
		-1.2684380040921763f * L + 2.6097574006633715f * M - 0.3413193963102197f * S,
		-0.004196086541837188f * L - 0.7034186144594493f * M + 1.7076147009309444f * S,
		a
	);
}

static ycColorF Convert( const ycColorF& c, ycColorSpace fromSpace, ycColorSpace toSpace )
{
	switch( fromSpace )
	{
	case kColorSpace_sRGB:
		switch( toSpace )
		{
		case kColorSpace_sRGB:       return c;
		case kColorSpace_Linear:     return c.RGBToLinear();
		case kColorSpace_HSV:        return c.RGBToHSV();
		case kColorSpace_Oklab:      return c.RGBToLinear().LinearToOklab();
		case kColorSpace_LinearSqrt: return c.RGBToLinearSqrt();
		YC_NO_DEFAULT_ASSERT_RETURN( c );
		}
	case kColorSpace_Linear:
		switch( toSpace )
		{
		case kColorSpace_sRGB:       return c.LinearToRGB();
		case kColorSpace_Linear:     return c;
		case kColorSpace_HSV:        return c.LinearToRGB().RGBToHSV();
		case kColorSpace_Oklab:      return c.LinearToOklab();
		case kColorSpace_LinearSqrt: return c.LinearToRGB().RGBToLinearSqrt();
		YC_NO_DEFAULT_ASSERT_RETURN( c );
		}
	case kColorSpace_HSV:
		switch( toSpace )
		{
		case kColorSpace_sRGB:       return c.HSVToRGB();
		case kColorSpace_Linear:     return c.HSVToRGB().RGBToLinear();
		case kColorSpace_HSV:        return c;
		case kColorSpace_Oklab:      return c.HSVToRGB().RGBToLinear().LinearToOklab();
		case kColorSpace_LinearSqrt: return c.HSVToRGB().RGBToLinearSqrt();
		YC_NO_DEFAULT_ASSERT_RETURN( c );
		}
	case kColorSpace_Oklab:
		switch( toSpace )
		{
		case kColorSpace_sRGB:       return c.OklabToLinear().LinearToRGB();
		case kColorSpace_Linear:     return c.OklabToLinear();
		case kColorSpace_HSV:        return c.OklabToLinear().LinearToRGB().RGBToHSV();
		case kColorSpace_Oklab:      return c;
		case kColorSpace_LinearSqrt: return c.OklabToLinear().LinearToRGB().RGBToLinearSqrt();
		YC_NO_DEFAULT_ASSERT_RETURN( c );
		}
	case kColorSpace_LinearSqrt:
		switch( toSpace )
		{
		case kColorSpace_sRGB:       return c.LinearSqrtToRGB();
		case kColorSpace_Linear:     return c.LinearSqrtToRGB().RGBToLinear();
		case kColorSpace_HSV:        return c.LinearSqrtToRGB().RGBToHSV();
		case kColorSpace_Oklab:      return c.LinearSqrtToRGB().RGBToLinear().LinearToOklab();
		case kColorSpace_LinearSqrt: return c;
		YC_NO_DEFAULT_ASSERT_RETURN( c );
		}
	YC_NO_DEFAULT_ASSERT_RETURN( c );
	}
}

ycColorF ycColorF::Mix( const ycColorF& _a, const ycColorF& _b, f32 t, ycColorSpace argSpace, ycColorSpace mixSpace )
{
	if( t < 1e-4f ) { return _a; }
	if( t > 0.9999f ) { return _b; }

	ycColorF a = Convert( _a, argSpace, mixSpace);
	ycColorF b = Convert( _b, argSpace, mixSpace );
	ycColorF result = a.Lerp( b, ycSaturate( t ) );
	
	// special case for hue-wraparound
	if( mixSpace == kColorSpace_HSV && ycAbs( b.h - a.h ) > 0.5f )
	{
		result.h = result.a + t * ycWrapNear( b.h - a.h, -0.5f, 0.5f );
		result.h = ycWrapNear( result.h, 0.0f, 1.0f );
	}

	return Convert( result, mixSpace, argSpace );
}
