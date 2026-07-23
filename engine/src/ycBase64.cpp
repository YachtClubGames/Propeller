
#include "ycBase64.h"

#include "ycStringRef.h"

namespace
{
	struct DecodeTable
	{
		u8 m_tbl[ 256 ] = {};
	};

	constexpr DecodeTable MakeDecodeTable( const char * kChars )
	{
		// ycAssert( ycStringUtils::Length( kChars ) == 64 );
		DecodeTable tbl;
		for( u8 i = 0; i < 64; i++ )
		{
			tbl.m_tbl[ (u8)kChars[ i ] ] = i;
		}
		return tbl;
	}

	constexpr const char * kBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	DecodeTable kDecodeTbl = MakeDecodeTable( kBase64Chars );
}

ycSize_t ycBase64::CalcDecodedSize( ycStringRef v )
{
	ycSize_t len = ( v.Length() * 3 ) >> 2;
	while( v.TakeLast() == '=' ) { len--; }
	return len;
}

void ycBase64::Decode( u8 * dst, ycSize_t dstSize, ycStringRef str )
{
	ycAssert( CalcDecodedSize( str ) == dstSize );
	u8 * dstEnd = dst + dstSize;

	while( !str.IsEmpty() )
	{
		ycStringRef group = str.TakeLeft( 4 );

		u32 v = 0;
		for( u32 i = 0; i < group.Length(); i++ )
		{
			u8 c = group[ i ];
			if( c == '=' ) { break; }
			u8 nib = kDecodeTbl.m_tbl[ c ];
			v |= nib << ( 6 * ( 3 - i ) );
		}

		*dst++ = ( v >> 16 ) & 0xFF;
		if( dst >= dstEnd ) { break; }
		*dst++ = ( v >> 8 ) & 0xFF;
		if( dst >= dstEnd ) { break; }
		*dst++ = ( v >> 0 ) & 0xFF;
		if( dst >= dstEnd ) { break; }
	}
}

ycSize_t ycBase64::CalcEncodedSize( ycSize_t inSize )
{
	const ycSize_t tris = inSize / 3;
	const ycSize_t remain = inSize - (tris*3);
	return tris*4 + (remain>0?1:0)*4;
}

void ycBase64::Encode( const u8 * src, ycSize_t srcSize, char* dst, ycSize_t dstSize )
{
	ycAssert( CalcEncodedSize( srcSize ) == dstSize );
	const ycSize_t tris = srcSize / 3;
	const ycSize_t remain = srcSize - (tris*3);
	for( ycSize_t tri = 0; tri != tris; ++tri )
	{
		const ureg b0 = (ureg)src[0];
		const ureg b1 = (ureg)src[1];
		const ureg b2 = (ureg)src[2];
		src += 3;
		dst[0] = kBase64Chars[ b0>>2 ];
		dst[1] = kBase64Chars[ ((0x3&b0)<<4) | (b1>>4) ];
		dst[2] = kBase64Chars[ ((0xf&b1)<<2) | (b2>>6) ];
		dst[3] = kBase64Chars[ 0x3f&b2 ];
		dst += 4;
	}
	if( remain == 2 )
	{
		const ureg b0 = (ureg)src[0];
		const ureg b1 = (ureg)src[1];
		dst[0] = kBase64Chars[ b0>>2 ];
		dst[1] = kBase64Chars[ ((0x3&b0)<<4) | (b1>>4) ];
		dst[2] = kBase64Chars[ ((0xf&b1)<<2) ];
		dst[3] = '=';
	}
	else if( remain == 1 )
	{
		const ureg b0 = (ureg)src[0];
		dst[0] = kBase64Chars[ b0>>2 ];
		dst[1] = kBase64Chars[ ((0x3&b0)<<4) ];
		dst[2] = '=';
		dst[3] = '=';
	}
}
