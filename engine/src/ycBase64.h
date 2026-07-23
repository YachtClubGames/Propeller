#pragma once

#include "ycStringRef.h"

namespace ycBase64
{
	ycSize_t CalcDecodedSize( ycStringRef v );
	void Decode( u8 * dst, ycSize_t dstSize, ycStringRef str );

	ycSize_t CalcEncodedSize( ycSize_t inSize );
	void Encode( const u8 * src, ycSize_t srcSize, char* dst, ycSize_t dstSize );
};
