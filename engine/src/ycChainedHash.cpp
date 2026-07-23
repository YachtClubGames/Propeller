#include "ycChainedHash.h"

#ifdef YC_TEST
#include "ycTest.h"

#include "ycRand.h"

YC_BEGIN_TEST( ycChainedHash_Basic )
{
	ycChainedHash< ureg, ureg > myHash( "ycChainedHash_Basic test", ycTest::GetMem() );
	enum
	{
		#ifdef YC_ENABLE_SLOW_TESTS
			kMinNumEntries = 1024*16,
			kMaxNumEntries = 1024*32
		#else
			kMinNumEntries = 1024,
			kMaxNumEntries = 2048
		#endif
	};
	ycRand randCtx;
	const ureg numEntries = randCtx.RangeFast( kMinNumEntries, kMaxNumEntries );
	ureg* keys   = ( ureg* )YC_ALLOC( ycTest::GetMem(), sizeof(ureg) * numEntries, YC_PTR_SIZE, "" );
	ureg* values = ( ureg* )YC_ALLOC( ycTest::GetMem(), sizeof(ureg) * numEntries, YC_PTR_SIZE, "" );
	for( ureg i = 0; i != numEntries; ++i )
	{
		// TODO HACK FIXME: no protection against getting the same Rand() twice...
		#ifdef YC_64
			const ureg key   = randCtx.Rand64();
			const ureg value = randCtx.Rand64();
		#else
			const ureg key   = randCtx.Rand();
			const ureg value = randCtx.Rand();
		#endif
		YC_TEST_CHECK( myHash.Contains( key ) == false );
		myHash.Insert( key, value );
		YC_TEST_CHECK( myHash.Contains( key ) == true );
		keys[   i ] = key;
		values[ i ] = value;
	}
	for( ureg i = 0; i != numEntries; ++i )
	{
		const ureg key   = keys[   i ];
		const ureg value = values[ i ];
		YC_TEST_CHECK( myHash.Contains( key ) == true );
		YC_TEST_CHECK( myHash.Get( key ) == value );
		myHash.Remove( key );
		YC_TEST_CHECK( myHash.Contains( key ) == false );
	}
	YC_FREE( ycTest::GetMem(), keys );
	YC_FREE( ycTest::GetMem(), values );
	YC_TEST_PASS( "ycChainedHash sanity test" );
}

#endif // YC_TEST
