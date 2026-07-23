#include "ycStd.h"

#include <string.h>

void* ycMemSet( void* dst, const int value, const size_t numBytes )
{
	return memset( dst, value, numBytes );
}

void* ycMemCpy( void* YC_RESTRICT dst, const void* YC_RESTRICT src, const size_t numBytes )
{
	return memcpy( dst, src, numBytes );
}

void* ycMemMove( void* dst, const void* src, const size_t numBytes )
{
	return memmove( dst, src, numBytes );
}

bool ycMemEq( const void* a, const void* b, const size_t numBytes )
{
	u8* pa = (u8 *)a, *pb = (u8 *)b;
	for( size_t i = 0; i != numBytes; ++i )
	{
		if( pa[i] != pb[i] ) { return false; }
	}
	return true;
}
