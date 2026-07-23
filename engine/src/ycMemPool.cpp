#include "ycMemPool.h"

#include "ycAllocator.h"
#include "ycMath.h"
#include "ycStd.h"
#include "ycMemTrack.h"

// TODO HACK FIXME: blockAlign on the individual blocks!!! (just round up blocksize?)

ycMemPool::ycMemPool( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, ycAllocator* mem, const ycSize_t blockAlign /*= YC_MEM_DEFAULT_ALIGN*/ ) :
	ycAllocator( debugName ),
	m_blockSize( ycRoundUpPow2( blockSize, blockAlign ) ),
	m_blockAlign( blockAlign ),
	m_numBlocks( numBlocks ),
	m_mem( mem ? mem : g_defaultMem )
{
	m_blocks = ( uint8_t* )YC_ALLOC( m_mem, m_blockSize*m_numBlocks, blockAlign, debugName );
	Clear();
}

ycMemPool::ycMemPool( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, uint8_t* mem, const ycSize_t blockAlign /*= YC_MEM_DEFAULT_ALIGN*/ ) :
	ycAllocator( debugName ),
	m_blockSize( blockSize ),
	m_blockAlign( blockAlign ),
	m_numBlocks( numBlocks ),
	m_mem( nullptr )
{
#ifdef YC_ENABLE_DEBUG_STRINGS
	m_name = debugName;
#endif
	ycAssert( blockSize % blockAlign == 0 );
	m_blocks = (uint8_t*)ycAlignPtr( mem, blockAlign );
	Clear();
}

void ycMemPool::Clear()
{
	ycAssert( m_blockSize >= sizeof( FreeLink ) );

	// mark all blocks free
	m_firstFreeIdx = 0;
	for( ycSize_t i = 0; i != m_numBlocks; ++i )
	{
		reinterpret_cast< FreeLink* >( GetBlock(i) )->nextFreeIdx = i+1;
	}
	reinterpret_cast< FreeLink* >( GetBlock(m_numBlocks-1) )->nextFreeIdx = kInvalidFreeIdx;

	ycMemTrack::ClearAllocator( this );
}

ycMemPool::~ycMemPool()
{
	if( m_mem ) { YC_FREE( m_mem, m_blocks ); }
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycMemPool )
{
	ycAssert( bytes != 0 );

	ycAssert( align <= m_blockAlign );
	YC_UNUSED( align );
	ycAssert( bytes <= m_blockSize );
	ycAssertMsg(bytes <= m_blockSize, "blocks %d m_blockSize %d", bytes, m_blockSize); 
	if( YC_UNLIKELY( m_firstFreeIdx == kInvalidFreeIdx ) )
	{
		ycAssert( flags&kAllocFlag_NoAssert );
		return nullptr;
	}
	ycAssert( m_firstFreeIdx != kInvalidFreeIdx );
	FreeLink* free = (FreeLink*)GetBlock( m_firstFreeIdx );
	m_firstFreeIdx = free->nextFreeIdx;

	if( ( flags & kAllocFlag_NoTrack ) == 0 )
	{
		ycMemTrack::TrackAlloc( this, free, bytes, debugName, file, line );
	}

	return free;
}

YC_ALLOCATOR_DECLARATION_FREE( ycMemPool )
{
	if( ( flags & kAllocFlag_NoTrack ) == 0 )
	{
		ycMemTrack::TrackFree( this, ptr );
	}

	reinterpret_cast< FreeLink* >( ptr )->nextFreeIdx = m_firstFreeIdx;
	m_firstFreeIdx = GetBlockIdx( ptr ); // this will assert if ptr is not a valid block (although the line above will already have corrupted some memory)
}

YC_ALLOCATOR_DECLARATION_SIZE( ycMemPool )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	return m_blockSize;
}

void* ycMemPool::GetBlock( const ycSize_t idx ) const
{
	return m_blocks + ( m_blockSize*idx );
}

ycSize_t ycMemPool::GetBlockIdx( const void* pPtr ) const
{
	const uintptr_t ptr   = (uintptr_t)pPtr;
	const uintptr_t start = (uintptr_t)GetBlock( 0 );
	const uintptr_t end   = (uintptr_t)GetBlock( m_numBlocks+1 );
	ycAssert( ptr >= start && ptr < end ); YC_UNUSED( end );
	const uintptr_t offset = ptr - start;
	ycAssert( offset % m_blockSize == 0 );
	return offset / m_blockSize;
}

////////////////////

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycMemPool_Fill_Empty )
{
	enum
	{
		kMaxBlocks = 512,
		kBlockSize = 16
	};

	{
		ycMemPool blockAlloc( "Test", kBlockSize, kMaxBlocks, ycTest::GetMem() );

		YC_TEST_CHECK( blockAlloc.GetMaxBlocks() == kMaxBlocks );
		YC_TEST_CHECK( blockAlloc.GetBlockSize() == kBlockSize );

		void* allocs[ kMaxBlocks ];
		ycMemSet( allocs, 0, sizeof(allocs) );
		for( ycSize_t i = 0; i != kMaxBlocks; ++i )
		{
			allocs[ i ] = blockAlloc.Alloc( kBlockSize, YC_PTR_SIZE, "Test", __FILE__, __LINE__, ycAllocator::kAllocFlag_NoAssert );
			YC_TEST_CHECK( allocs[ i ] );
		}
		YC_TEST_CHECK( blockAlloc.Alloc( kBlockSize, YC_PTR_SIZE, "Test", __FILE__, __LINE__, ycAllocator::kAllocFlag_NoAssert ) == nullptr );
		for( ycSize_t i = 0; i != kMaxBlocks; ++i )
		{
			blockAlloc.Free( allocs[ i ] );
		}
	} // include dtor in the test

	YC_TEST_PASS( "Fill and empty an ycMemPool" );
}

#include "ycRand.h"

YC_BEGIN_TEST( ycMemPool_Random )
{
	enum
	{
		kMaxBlocks  = 1024,
		kBlockSize  = 32,
		kIterations = 1024*1024
	};
	ycRand randCtx;
	
	{
		ycMemPool blockAlloc( "Test", kBlockSize, kMaxBlocks, ycTest::GetMem() );

		YC_TEST_CHECK( blockAlloc.GetMaxBlocks() == kMaxBlocks );
		YC_TEST_CHECK( blockAlloc.GetBlockSize() == kBlockSize );

		void* allocs[ kMaxBlocks ];
		ycMemSet( allocs, 0, sizeof(allocs) );

		for( uint64_t i = 0; i != kIterations; ++i )
		{
			const int32_t j = randCtx.RangeFast( 0, kMaxBlocks-1 );
			if( allocs[ j ] == nullptr )
			{
				allocs[ j ] = blockAlloc.Alloc( kBlockSize, YC_PTR_SIZE, "Test", __FILE__, __LINE__, ycAllocator::kAllocFlag_NoAssert );
				YC_TEST_CHECK( allocs[ j ] );
			}
			else
			{
				blockAlloc.Free( allocs[ j ] );
				allocs[ j ] = nullptr;
			}
		}

		for( ycSize_t i = 0; i != kMaxBlocks; ++i )
		{
			if( allocs[ i ] )
			{
				blockAlloc.Free( allocs[ i ] );
			}
		}
	} // include dtor in the test

	YC_TEST_PASS( "Random pattern ycMemPool" );
}

#endif // YC_TEST
