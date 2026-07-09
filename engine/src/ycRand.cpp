#include "ycRand.h"

// core
#include "ycDateTime.h" // seeding
#include "ycBitUtils.h"   // RandF_Iterative
#include "ycMath.h"     // RandF_Iterative
#include "ycStd.h"      // YC_UNALIGNED_WRITE / ycMemCpy

ycRand s_globalRand;

u32 ycRandI()
{
	return s_globalRand.Rand();
}

u64 ycRand64()
{
	return s_globalRand.Rand64();
}

u64 ycRandGUID()
{
	return s_globalRand.RandGUID();
}

void ycRandSeed( u32 _seed )
{
	s_globalRand.Seed( _seed );
}

s32 ycRandI( s32 min, s32 max )
{
	return s_globalRand.RangeFast( min, max );
}

bool ycRandB( f32 probability )
{
	if( probability <= 0.0f ) { return false; }
	if( probability >= 1.0f ) { return true; }
	return ycRandF( 0.0f, 1.0f ) <= probability;
}

f32 ycRandF( f32 min, f32 max )
{
	return s_globalRand.RandF( min, max );
}

////////

ycRand::ycRand()
{
	SeedRand();
}

ycRand::ycRand( const u32 _seed )
{
	Seed( _seed );
}

void ycRand::SeedRand()
{
	Seed( (u32)ycDateTime::GetTimeHiRes() );
}

void ycRand::Seed( const u32 _seed )
{
	a = 0xf1ea5eed, b = c = d = _seed;
	for( u32 i = 0; i < 20; ++i )
	{
		(void)Rand();
	}
}

void ycRand::SetState( const u32 _a, const u32 _b, const u32 _c, const u32 _d )
{
	a = _a;
	b = _b;
	c = _c;
	d = _d;
}

void ycRand::GetState( u32& _a, u32& _b, u32& _c, u32& _d )
{
	_a = a;
	_b = b;
	_c = c;
	_d = d;
}

u32 ycRand::Rand()
{
	#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
	const u32 e = a - rot(b, 27);
	a = b ^ rot(c, 17);
	b = c + d;
	c = d + e;
	d = e + a;
	return d;
	#undef rot
}

u64 ycRand::Rand64()
{
	// 64 bit rng would ideally use a different generator, but this'll do for now
	union
	{
		struct { u32 a, b; };
		u64 as64;
	} rng;
	rng.a = Rand();
	rng.b = Rand();
	return rng.as64;
}

u64 ycRand::RandGUID()
{
	u64 gen_id = ycDateTime::GetTimeHiRes();
	// apply a shuffle to the gen_id
	u64 shuffle = ( ( u64 )Rand64() * ( u64 )16777619 ) ^ ( ( u64 )Rand64() * ( u64 )9997954969 );

	// flip the bits of the gen_id to even out the randomness between the lowest and highest bits
	gen_id = ( gen_id << 32 ) | ( gen_id >> 32 );
	gen_id = ( ( gen_id << 16 ) & 0xffff0000ffff0000ULL ) | ( ( gen_id >> 16 ) & 0x0000ffff0000ffffULL );
	gen_id = ( ( gen_id << 8 ) & 0xff00ff00ff00ff00ULL ) | ( ( gen_id >> 8 ) & 0x00ff00ff00ff00ffULL );
	gen_id = ( ( gen_id << 4 ) & 0xf0f0f0f0f0f0f0f0ULL ) | ( ( gen_id >> 4 ) & 0x0f0f0f0f0f0f0f0fULL );
	gen_id = ( ( gen_id << 2 ) & 0xccccccccccccccccULL ) | ( ( gen_id >> 2 ) & 0x3333333333333333ULL );
	gen_id = ( ( gen_id << 1 ) & 0xaaaaaaaaaaaaaaaaULL ) | ( ( gen_id >> 1 ) & 0x5555555555555555ULL );

	// apply shuffle to gen_id
	gen_id += shuffle;
	return gen_id;
}

void ycRand::RandBytes( u8* dst, const ycSize_t numBytes )
{
	ycSize_t remain = numBytes;
	while( remain > 8 )
	{
		const u64 tmp = Rand64();
		YC_UNALIGNED_WRITE( u64, dst, &tmp );
		dst += 8;
		remain -= 8;
	}
	while( remain > 4 )
	{
		const u32 tmp = Rand();
		YC_UNALIGNED_WRITE( u32, dst, &tmp );
		dst += 4;
		remain -= 4;
	}
	if( remain )
	{
		const u32 tmp = Rand();
		ycMemCpy( dst, &tmp, remain );
	}
}

u32 ycRand::RangeFast( const u32 min, const u32 max )
{
	const u32 range = (max+1) - min;
	const u32 rand = Rand();
	return (rand%range) + min;
}

//u32 ycRand::Range( const u32 min, const u32 max )
//{
//   return u32( ((double)Rand() / ((double)YC_RAND_MAX + 1.0)) * (max - min + 1) + min );
//}

f32 ycRand::RandF()
{
	return f32( f32(Rand()) / f32(YC_RAND_MAX) );
}

f32 ycRand::RandF( f32 min, f32 max )
{
	return RandF()*(max - min) + min;
}

s32 ycRand::RandI( s32 min, s32 max )
{
	return RangeFast( min, max );
}

f32 ycRand::RandF_Iterative()
{
	// this looks like a lot, but each 'else' is progressively less likely to be followed, with a very small chance of *any* of the 'else' branches being taken

	// TODO HACK FIXME: review this code
	/*
	* exponent = 1 is not doing anything, exponent is always re-set! so is that comment about low bit important?!
	* clean up branches; just check whether rng==0 before calling clz, rather than checking setBit==32
	*/

	u32 rng      = Rand(); // pull our first, and likely only, random number
	u32 mantissa = rng&0x7FFFFF; // mantissa can be random bits
	u32 exponent = 1; // low bit always set

	u32 setBit = u32(ycBitUtils::CountLeadingZeros( rng & 0xFF800000 ));
	if( setBit != 32 ) // 0-8
	{
		exponent = 126 - (setBit);
	}
	else
	{
		rng = Rand();
		setBit = u32(ycBitUtils::CountLeadingZeros( rng ));
		if( setBit != 32 ) // 0-31
		{
			exponent = 117 - setBit;
		}
		else
		{
			rng = Rand();
			setBit = u32(ycBitUtils::CountLeadingZeros( rng ));
			if( setBit != 32 ) // 0-31
			{
				exponent = 85 - setBit;
			}
			else
			{
				rng = Rand();
				setBit = u32(ycBitUtils::CountLeadingZeros( rng ));
				if( setBit != 32 ) // 0-31
				{
					exponent = 53 - setBit;
				}
				else
				{
					rng = Rand();
					setBit = u32(ycBitUtils::CountLeadingZeros( rng&0xFFFFF800 ));
					if( setBit != 32 ) // 0-20
					{
						exponent = 21 - setBit;
					}
				}
			}
		}
	}
	return ycUIntAsFloatBits( (exponent<<23) | mantissa );
}

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycRand_RangeFast )
{
	enum
	{
	#ifdef YC_ENABLE_SLOW_TESTS
		kIterations = 1024*1024*256
	#else
		kIterations = 1024*1024*32
	#endif
	};
	const u32 ranges[][2] =
	{
		{ 123, 1234 },
		{ 0, 255 }
	};

	for( int32_t rangeIdx = 0; rangeIdx != YC_ARRAY_SIZE( ranges ); ++rangeIdx )
	{
		ycRand randCtx;
		
		const u32 rangeMin = ranges[ rangeIdx ][ 0 ];
		const u32 rangeMax = ranges[ rangeIdx ][ 1 ];
		const u32 range = (rangeMax+1) - rangeMin;
		
		ycAllocator* sysMem = ycTest::GetMem();
		uint64_t* bins = (uint64_t*)sysMem->Alloc( sizeof(uint64_t) * range );
		ycMemSet( bins, 0, sizeof(uint64_t)*range );

		for( uint64_t i = 0; i != kIterations; ++i )
		{
			const u32 j = randCtx.RangeFast( rangeMin, rangeMax );
			YC_TEST_CHECK( j >= rangeMin );
			YC_TEST_CHECK( j <= rangeMax );
			++bins[ j - rangeMin ];
		}

		const uint64_t avg   = uint64_t(kIterations) / uint64_t(range);
		const uint64_t delta = avg / uint64_t(5); // not any particular logic here
		const uint64_t min   = avg - delta;
		const uint64_t max   = avg + delta;
		uint64_t largestDelta = 0;
		for( u32 i = 0; i != range; ++i )
		{
			YC_TEST_CHECK( bins[i] > min && bins[i] < max );
			const uint64_t curDelta = bins[i] > avg ? bins[i]-avg : avg-bins[i];
			if( curDelta > largestDelta ) { largestDelta = curDelta; }
		}

		sysMem->Free( bins );

		YC_TEST_PASS( "ycRand::RangeFast( %u, %u ) distribution [largest delta: %.2f%s]", rangeMin, rangeMax, f32(double(largestDelta)/double(avg))*100.0f, "%" );
	}
}

//YC_BEGIN_TEST( ycRand_Range )
//{
//	enum
//	{
//		kIterations = 1024*1024*100
//	};
//	const u32 ranges[][2] =
//	{
//		{ 123, 1234 },
//		{ 0, 255 }
//	};
//
//	for( int32_t rangeIdx = 0; rangeIdx != YC_ARRAY_SIZE( ranges ); ++rangeIdx )
//	{
//		ycRand randCtx;
//		
//		const u32 rangeMin = ranges[ rangeIdx ][ 0 ];
//		const u32 rangeMax = ranges[ rangeIdx ][ 1 ];
//		const u32 range = (rangeMax+1) - rangeMin;
//
//		uint64_t* bins = new uint64_t[ range ];
//		ycMemSet( bins, 0, sizeof(uint64_t)*range );
//
//		for( uint64_t i = 0; i != kIterations; ++i )
//		{
//			const u32 j = randCtx.Range( rangeMin, rangeMax );
//			YC_TEST_CHECK( j >= rangeMin );
//			YC_TEST_CHECK( j <= rangeMax );
//			++bins[ j - rangeMin ];
//		}
//
//		const uint64_t avg   = uint64_t(kIterations) / uint64_t(range);
//		const uint64_t delta = avg / uint64_t(5); // not any particular logic here
//		const uint64_t min   = avg - delta;
//		const uint64_t max   = avg + delta;
//		uint64_t largestDelta = 0;
//		for( u32 i = 0; i != range; ++i )
//		{
//			YC_TEST_CHECK( bins[i] > min && bins[i] < max );
//			const uint64_t curDelta = bins[i] > avg ? bins[i]-avg : avg-bins[i];
//			if( curDelta > largestDelta ) { largestDelta = curDelta; }
//		}
//
//		delete[] bins;
//
//		YC_TEST_PASS( "ycRand::Range( %u, %u ) distribution [largest delta: %.2f%s]", rangeMin, rangeMax, f32(double(largestDelta)/double(avg))*100.0f, "%" );
//	}
//}

#ifdef YC_ENABLE_SLOW_TESTS
YC_BEGIN_TEST( ycRand_RandF )
{
	ycRand randCtx;
	bool hitZero = false;
	bool hitOne = false;
	uint64_t iterations = 0;
	double drift = 0.0;
	enum
	{
		kMinIterations = 1024*1024*50
	};
	while( !(hitZero&&hitOne) || (iterations&1) || iterations < kMinIterations ) // do an even number of iterations so drift isn't as biased
	{
		const f32 rand = randCtx.RandF();
		YC_TEST_CHECK( rand >= 0.0f );
		YC_TEST_CHECK( rand <= 1.0f );
		if( rand == 0.0f )
		{
			hitZero = true;
		}
		else if( rand == 1.0f )
		{
			hitOne = true;
		}
		drift += double(rand) - 0.5;
		++iterations;
		YC_TEST_CHECK( iterations < 0xffffffffffffffffu ); // we should've probably hit one and zero by now...
	}
	YC_TEST_PASS( "ycRand::RandF() distribution [drift:%.5f] [iterations:%llu]", drift, iterations );
}

YC_BEGIN_TEST( ycRand_RandF_Iterative )
{
	ycRand randCtx;
	uint64_t iterations = 0;
	double drift = 0.0;
	enum
	{
		kMinIterations = 1024*1024*50
	};
	while( (iterations&1) || iterations < kMinIterations ) // do an even number of iterations so drift isn't as biased
	{
		const f32 rand = randCtx.RandF_Iterative();
		YC_TEST_CHECK( rand > 0.0f );
		YC_TEST_CHECK( rand < 1.0f );
		drift += double(rand) - 0.5;
		++iterations;
	}
	YC_TEST_PASS( "ycRand::RandF_Iterative() distribution [drift:%.5f] [iterations:%llu]", drift, iterations );
}
#endif

#endif // YC_TEST
