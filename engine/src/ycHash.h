#pragma once
#ifndef YC_HASH_H
#define YC_HASH_H

#include "ycCommon.h"

struct ycColor;
struct ycVec3;

/*! Hash Functions
You can extend this to custom key types by defining your own ycHashKey().
You do not have to do that in this file.
*/

#ifdef YC_64
	typedef uint64_t ycHashKey_t;
#else
	typedef uint32_t ycHashKey_t;
#endif

ycHashKey_t ycHashKey( const char* key );
ycHashKey_t ycHashKey( int32_t     key );
ycHashKey_t ycHashKey( uint32_t    key );
ycHashKey_t ycHashKey( int64_t     key );
ycHashKey_t ycHashKey( uint64_t    key );
ycHashKey_t ycHashKey( const void* key );
ycHashKey_t ycHashKey( const ycVec3&  key );
ycHashKey_t ycHashKey( const ycColor& key );

ycHashKey_t ycHashString( const char* str ); // jenkin's hash
ycHashKey_t ycHashString( const char* str, const ycSize_t length );
ycHashKey_t ycHash4Fast( const void* pData ); // knuth hash
ycHashKey_t ycHash8Fast( const void* pData );
ycHashKey_t ycHashBytes( const void* pData, const ycSize_t numBytes ); // jenkin's hash

// type-explicit hash functions
u64 ycHashString64( const char* str );
u32 ycHashString32( const char* str );
u64 ycHash64( const void* pData, const ycSize_t numBytes );
u32 ycHash32( const void* pData, const ycSize_t numBytes );
u64 ycHash64( const void* pData, const ycSize_t numBytes, const u32 seed, const u32 seed1 );
u32 ycHash32( const void* pData, const ycSize_t numBytes, const u32 seed );

/*! Hash Combining Functions
If you have multiple hashes that you need to combine, or simply wish to implement an incremental hash, these
convenience functions may be what you are looking for.

These are not as strong as a proper hashing function, but are pretty good for most purposes. Use them with care.

Example Usage:

u32 hashes[4];
u32 finalHash = u32(-1);
for( ureg i = 0; i != 4; ++i )
{
	finalHash = ycHashCombine32( hashes[i], finalHash );
}
*/
u32 ycHashCombine32( const u32 nextData, u32 hash = u32(-1) );
u64 ycHashCombine64( const u64 nextData, u64 hash = u64(-1) );

#endif // !YC_HASH_H
