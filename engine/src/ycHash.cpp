#include "ycHash.h"

// 3rdparty
#include "../3rdparty/lookup3.h"

#include "ycStd.h"
#include "ycStringUtils.h"
#include "ycColor.h"
#include "ycVec3.h"

ycHashKey_t ycHashString( const char* str )
{
	return ycHashBytes( str, ycStringUtils::Length(str) );
}

ycHashKey_t ycHashString( const char* str, const ycSize_t length )
{
	return ycHashBytes( str, length );
}

ycHashKey_t ycHash4Fast( const void* pData )
{
#ifdef YC_64
	u32 src;
	YC_UNALIGNED_READ( u32, &src, pData );
	u64 tmp = u64(src) * u64(2654435761); // simple knuth hash
	return ( tmp >> 32 ) | ( tmp << 32 ); // swap the high bits into the low bits; a lot of our hash-using code will mask down to the lower bits instead of shifting, and the high bits are the good ones
#else
	return ycHashKey_t( *reinterpret_cast< const ycHashKey_t* >( pData ) * ycHashKey_t(2654435761) );
#endif
}

ycHashKey_t ycHash8Fast( const void* pData )
{
#ifdef YC_64
	uint64_t x = *reinterpret_cast< const uint64_t* >( pData );
#if 0
	// https://gist.github.com/degski/6e2069d6035ae04d5d6f64981c995ec2
	// MIT, would have to put in credits
	x = ( ( x >> 32 ) ^ x ) * 0xD6E8FEB86659FD93;
	x = ( ( x >> 32 ) ^ x ) * 0xD6E8FEB86659FD93;
	x = ( ( x >> 32 ) ^ x );
#else
	// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp#L81
	// finalize function from MurmurHash3, public domain
	x ^= x >> 33;
	x *= 0xff51afd7ed558ccdllu;
	x ^= x >> 33;
	x *= 0xc4ceb9fe1a85ec53llu;
	x ^= x >> 33;
#endif
	return ycHashKey_t{ x };
#else
	return hashword( (uint32_t*)pData, 2, 1337 );
#endif
}

ycHashKey_t ycHashBytes( const void* pData, const ycSize_t numBytes )
{
#ifdef YC_64
	return ycHash64( pData, numBytes );
#else
	return ycHash32( pData, numBytes );
#endif
}

ycHashKey_t ycHashKey( const char* key )
{
	return ycHashString( key );
}

ycHashKey_t ycHashKey( int32_t key )
{
	return ycHash4Fast( &key );
}

ycHashKey_t ycHashKey( uint32_t key )
{
	return ycHash4Fast( &key );
}

ycHashKey_t ycHashKey( int64_t key )
{
	return ycHash8Fast( &key );
}

ycHashKey_t ycHashKey( uint64_t key )
{
	return ycHash8Fast( &key );
}

ycHashKey_t ycHashKey( const void* key )
{
#ifdef YC_64
	return ycHash8Fast( &key );
#else
	return ycHash4Fast( &key );
#endif
}

ycHashKey_t ycHashKey( const ycVec3& value ) 
{ 
	return ycHashBytes( &value, sizeof( value ) );
}

ycHashKey_t ycHashKey( const ycColor& value ) 
{ 
	// TODO HACK FIXME: this is _not good_, it should give a more pseudorandom distribution
	ycHashKey_t hash = 0x0;
	hash = hash | value.r;
	hash = ( hash << 8 ) | value.g;
	hash = ( hash << 8 ) | value.b;
	hash = ( hash << 8 ) | value.a;
	return hash;
}

u64 ycHashString64( const char* str )
{
	return ycHash64( str, ycStringUtils::Length(str) );
}

u32 ycHashString32( const char* str )
{
	return ycHash32( str, ycStringUtils::Length(str) );
}

u64 ycHash64( const void* pData, const ycSize_t numBytes )
{
	union Hash64
	{
		u32 hash32[2];
		u64 hash;
	} hash;
	hash.hash32[0] = 13;
	hash.hash32[1] = 37;
	hashlittle2( pData, size_t(numBytes), &hash.hash32[0], &hash.hash32[1] );
	return hash.hash;
}

u32 ycHash32( const void* pData, const ycSize_t numBytes )
{
	return hashlittle( pData, size_t(numBytes), 1337 );
}

u64 ycHash64( const void* pData, const ycSize_t numBytes, const u32 seed0, const u32 seed1 )
{
	union Hash64
	{
		u32 hash32[2];
		u64 hash;
	} hash;
	hash.hash32[0] = seed0;
	hash.hash32[1] = seed1;
	hashlittle2( pData, size_t(numBytes), &hash.hash32[0], &hash.hash32[1] );
	return hash.hash;
}

u32 ycHash32( const void* pData, const ycSize_t numBytes, const u32 seed )
{
	return hashlittle( pData, size_t(numBytes), seed );
}

u32 ycHashCombine32( const u32 nextData, u32 hash )
{
	hash ^= nextData + 0x9e3779b9u + (hash << 6) + (hash >> 2);
	return hash;
}

u64 ycHashCombine64( const u64 nextData, u64 hash )
{
	hash ^= nextData + 0x9e3779b97f4a7c13ull + (hash << 6) + (hash >> 2); // ideally this would have another set of shifts and rotates, but this should be OK
	return hash;
}

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycHash_CStringKeyOverloading )
{
	const char* myCString = "Test";
	ycHashKey_t hashA = ycHashKey( myCString );
	ycHashKey_t hashB = ycHashKey( (void*)myCString );
	ycHashKey_t hashC = ycHashKey( (void*)myCString );
	YC_TEST_CHECK( hashA != hashB ); // not perfect.. but eh, good enough
	YC_TEST_CHECK( hashB == hashC ); // may as well check that hashing remained deterministic while we're here
	YC_TEST_PASS( "ycHash string key overloading" );
}

#endif // YC_TEST
