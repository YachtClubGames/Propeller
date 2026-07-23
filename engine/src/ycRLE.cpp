#include "ycRLE.h"

// could template this to do different block sizes!

ycSize_t ycRLE_encode( const u8* in, const ycSize_t inSize, u8* out )
{
	//ycLog( "RLE_encode\n" );
	ycSize_t bytesLeft = inSize;
	const u8* src = in;
	u8* dst = out;
	while( bytesLeft >= 3 )
	{
		ureg runLength = 0;
		u8 repeat;
		ureg numLiterals = 0;
		//u8* literals;

		if( src[0] == src[1] )
		{
			repeat = src[0];
			do
			{
				++runLength;
				--bytesLeft;
				++src;
			} while( bytesLeft && src[0] == repeat );
		}

		if( bytesLeft )
		{
			do
			{
				--bytesLeft;
				++numLiterals;
				++src;
			} while( bytesLeft && src[0] != src[1] );
		}

		//ycLog( "%u x 0x%02X + %u literals\n", runLength, repeat, numLiterals );
		while( runLength || numLiterals )
		{
			if( runLength && numLiterals )
			{
				//const u8 cmdType = ( runLength > 0x7 || numLiterals&0x7 ) ?
				*dst = 0xC0 | ( (runLength&0x7)<<3 ) | (numLiterals&0x7);
				runLength >>= 3;
				numLiterals >>= 3;
			}
			else if( runLength )
			{
				*dst = 0x80 | ( runLength&0x3F );
				runLength >>= 6;
			}
			else
			{ // just literals
				*dst = 0x40 | ( numLiterals&0x3F );
				numLiterals >>= 6;
			}
		}
	}

	return dst - out;
}

ycSize_t ycRLE_decode( const u8* in, u8* out )
{
	//ycLog( "RLE_decode\n" );
	const u8* src = in;
	u8* dst = out;
	for(;;)
	{
		const u8 cmd = *src;
		++src;
		ureg len = cmd&0x7f;
		if( cmd&0x80 )
		{
			const u8 repeat = *src;
			//ycLog( "%u x 0x%02X\n", u32(len), repeat );
			++src;
			do
			{
				*dst = repeat;
				++dst;
				--len;
			}
			while( len );
		}
		else if( len )
		{
			//ycLog( "%u literals\n", u32(len) );
			do
			{
				*dst = *src;
				++dst;
				++src;
				--len;
			}
			while( len );
		}
		else
		{
			//ycLog( "0 literals\n" );
			break;
		}
	}
	return dst - out;
}

//namespace
//{
//	const u8 s_inData[] =
//	{
//		#include "logo.inl"
//	};
//}
//	#if 0
//	ycSize_t inSize = YC_ARRAY_SIZE(s_inData);
//	
//	u8* encoded = (u8*)malloc( YC_ARRAY_SIZE(s_inData) * 2 );
//	ycSize_t rleSize = RLE_encode( s_inData, inSize, encoded );
//
//	u8* decoded = (u8*)malloc( YC_ARRAY_SIZE(s_inData) );
//
//	ycSize_t decodedSize = RLE_decode( encoded, decoded );
//
//	ycAssert( decodedSize == YC_ARRAY_SIZE( s_inData ) );
//
//	int cmp = memcmp( s_inData, decoded, decodedSize );
//
//	int q = 1;
//	#endif
