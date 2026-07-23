#include "ycBitUtils.h"

#if defined _MSC_VER
	#include <intrin.h>
	#pragma intrinsic( _BitScanReverse )
#endif

//#define YC_CTZ_CLZ_SW_IMPL // TODO HACK FIXME: sw impl could be better! there are smarter bit twiddling ways than big fugly loops
// TODO HACK FIXME: should be able to implement arm/ppc ctz better https://fgiesen.wordpress.com/2013/10/18/bit-scanning-equivalencies/

ureg ycBitUtils::CountLeadingZeros( const uint32_t x )
{
#if defined __GNUC__ && __GNUC__ >= 4
	// http://gcc.gnu.org/onlinedocs/gcc-4.3.4/gcc/Other-Builtins.html
	if( x == 0 ) { return 32; }
	return __builtin_clz( x ); // TODO HACK FIXME: not sure if we need to swap to ctz on big endian to match behavior, tired, brain not work well
#elif defined _MSC_VER // present for at least >=vs2008, dont particularly care about lower, and i'm sure we're counting on other MSVC things in newer versions. (but could use _BitScanReverse for older)
	// https://msdn.microsoft.com/en-us/library/bb384809(v=vs.140).aspx
	//return ureg(__lzcnt( x ));
	// __lzcnt may fall back to _BitScanReverse if the cpu doesnt support lzcnt natively... so let's just use it, since it's return value is different and makes __lzcnt unsafe for us to use unless we check the cpuid
	if( x == 0 ) { return 32; }
	uint32_t index;
	_BitScanReverse( (unsigned long*)&index, (unsigned long)x );
	return 31-index;
#elif defined YC_CTZ_CLZ_SW_IMPL
	u32 mask = 0x80000000u;
	for( ureg i = 0; i != 32; ++i, mask >>= 1 )
	{
		if( x & mask )
		{
			return i;
		}
	}
	return 32;
#else
	#error ycCountLeadingZeros needs to be implemented for this platform!
#endif
}

ureg ycBitUtils::CountLeadingZeros64( const uint64_t x )
{
#if defined __GNUC__ && __GNUC__ >= 4
	if( x == 0 ) { return 64; }
	return __builtin_clzll( x );
#elif defined _MSC_VER
	if( x == 0 ) { return 64; }
	#ifdef YC_64
		uint32_t index;
		_BitScanReverse64( (unsigned long*)&index, (__int64)x );
		return 63-index;
	#else
		uint32_t index;
		const uint32_t hi = uint32_t(x>>32);
		if( 0 != _BitScanReverse( (unsigned long*)&index, (unsigned long)hi ) )
		{
			return 31-index;
		}
		const uint32_t lo = uint32_t(x);
		_BitScanReverse( (unsigned long*)&index, (unsigned long)lo );
		return (31-index) + 32;
	#endif
#elif defined YC_CTZ_CLZ_SW_IMPL
	ureg index;
	const u32 high = u32( (x>>32)&0xffffffffull );
	index = ycCountLeadingZeros( high );
	if( index != 32 )
	{
		return index;
	}
	else
	{
		const u32 low = u32( x & 0xffffffffull );
		return ycCountLeadingZeros( low ) + 32;
	}
#else
	#error ycCountLeadingZeros needs to be implemented for this platform!
#endif
}

ureg ycBitUtils::CountTrailingZeros( const uint32_t x )
{
#if defined __GNUC__ && __GNUC__ >= 4
	if( x == 0 ) { return 32; }
	return __builtin_ctz( x );
#elif defined _MSC_VER
	if( x == 0 ) { return 32; }
	uint32_t index;
	_BitScanForward( (unsigned long*)&index, (unsigned long)x );
	return index;
#elif defined YC_CTZ_CLZ_SW_IMPL
	u32 mask = 1;
	for( ureg i = 0; i != 32; ++i, mask <<= 1 )
	{
		if( x & mask )
		{
			return i;
		}
	}
	return 32;
#else
	#error ycCountLeadingZeros needs to be implemented for this platform!
#endif
}

ureg ycBitUtils::CountTrailingZeros64( const uint64_t x )
{
#if defined __GNUC__ && __GNUC__ >= 4
	if( x == 0 ) { return 64; }
	return __builtin_ctzll( x );
#elif defined _MSC_VER
	if( x == 0 ) { return 64; }
	#ifdef YC_64
		uint32_t index;
		_BitScanForward64( (unsigned long*)&index, (__int64)x );
		return index;
	#else
		const uint32_t lo = uint32_t(x);
		uint32_t index;
		if( 0 != _BitScanForward( (unsigned long*)&index, (unsigned long)lo ) )
		{
			return index;
		}
		const uint32_t hi = uint32_t(x>>32);
		_BitScanForward( (unsigned long*)&index, (unsigned long)hi );
		return index + 32;
	#endif
#elif defined YC_CTZ_CLZ_SW_IMPL
	ureg index;
	const u32 low = u32( x & 0xffffffffu );
	index = ycCountTrailingZeros( low );
	if( index != 32 )
	{
		return index;
	}
	else
	{
		const u32 high = u32( x >> 32 );
		return ycCountTrailingZeros( high ) + 32;
	}
#else
	#error ycCountLeadingZeros needs to be implemented for this platform!
#endif
}

ureg ycBitUtils::PopCount( const uint32_t x )
{
#if defined __GNUC__
	return __builtin_popcount( x );
#elif defined _MSC_VER
	return __popcnt( x );
#elif defined YC_CTZ_CLZ_SW_IMPL
	ureg count = 0;
	for( ; n; ++count ) { n &= n - 1; }
	return count;
#else
	#error ycBitUtils::PopCount needs to be implemented for this platform!
#endif
}

ureg ycBitUtils::PopCount64( const uint64_t x )
{
#if defined __GNUC__
	return __builtin_popcountll( x );
#elif defined _MSC_VER
	uint64_t _x = x;
	_x = _x - ((_x >> 1) & 0x5555555555555555llu);
	_x = (_x & 0x3333333333333333llu) + ((_x >> 2) & 0x3333333333333333llu);
	_x = (_x + (_x >> 4)) & 0x0f0f0f0f0f0f0f0fllu;
	return ((_x * 0x0101010101010101llu) >> 56);
	// some cpus aren't supporting this, seeing it even on a haswell
	// there was a microsoft bug, maybe fixed on newer vs:
	// https://developercommunity.visualstudio.com/t/Illegal-Instruction-POPCNT-emitted-in-MS/10576397
	// note: Mina's release was compiled on a VS version *higher* than this bug says it was fixed in -- still present? something else going on?
	//return __popcnt64( x );
#elif defined YC_CTZ_CLZ_SW_IMPL
	ureg count = 0;
	for( ; n; ++count ) { n &= n - 1; }
	return count;
#else
	#error ycBitUtils::PopCount needs to be implemented for this platform!
#endif
}

#ifdef YC_TEST
#include "ycTest.h"
#include "ycRand.h"

namespace
{
	enum
	{
		kTestIters = 1024*16
	};
}

YC_BEGIN_TEST( CountLeadingZeros )
{
	ycRand rng;
	for( ureg j = 0; j != kTestIters; ++j )
	{
		const uint32_t val = rng.Rand();
		ureg expected = 0;
		for( ureg i = 0; i != 32; ++i )
		{
			if( val & (0x80000000u >> i) )
			{
				expected = i;
				break;
			}
		}
		YC_TEST_CHECK( ycBitUtils::CountLeadingZeros( val ) == expected );
	}
	YC_TEST_PASS( "CountLeadingZeros" );
}

YC_BEGIN_TEST( CountLeadingZeros64 )
{
	ycRand rng;
	for( ureg j = 0; j != kTestIters; ++j )
	{
		union
		{
			uint32_t valParts[2];
			uint64_t val;
		};
		valParts[0] = rng.Rand();
		valParts[1] = rng.Rand();
		uint64_t expected = 0;
		for( uint64_t i = 0; i != 64; ++i )
		{
			if( val & (0x8000000000000000ull >> i) )
			{
				expected = i;
				break;
			}
		}
		YC_TEST_CHECK( ycBitUtils::CountLeadingZeros64( val ) == expected );
	}
	YC_TEST_PASS( "CountLeadingZeros64" );
}

YC_BEGIN_TEST( CountTrailingZeros )
{
	ycRand rng;
	for( ureg j = 0; j != kTestIters; ++j )
	{
		const uint32_t val = rng.Rand();
		ureg expected = 0;
		for( ureg i = 0; i != 32; ++i )
		{
			if( val & (0x00000001u << i) )
			{
				expected = i;
				break;
			}
		}
		YC_TEST_CHECK( ycBitUtils::CountTrailingZeros( val ) == expected );
	}
	YC_TEST_PASS( "CountTrailingZeros" );
}

YC_BEGIN_TEST( CountTrailingZeros64 )
{
	ycRand rng;
	for( ureg j = 0; j != kTestIters; ++j )
	{
		union
		{
			uint32_t valParts[2];
			uint64_t val;
		};
		valParts[0] = rng.Rand();
		valParts[1] = rng.Rand();
		ureg expected = 0;
		for( ureg i = 0; i != 64; ++i )
		{
			if( val & (0x0000000000000001ull << i) )
			{
				expected = i;
				break;
			}
		}
		YC_TEST_CHECK( ycBitUtils::CountTrailingZeros64( val ) == expected );
	}
	YC_TEST_PASS( "CountTrailingZeros64" );
}

#endif // YC_TEST
