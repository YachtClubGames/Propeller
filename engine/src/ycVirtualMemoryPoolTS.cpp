#include "ycVirtualMemoryPoolTS.h"

#include "ycMath.h"
#include "ycVirtualMem.h"

#include <atomic>
static_assert( sizeof(std::atomic<u32>) == sizeof(u32), "" );
static_assert( alignof(std::atomic<u32>) == alignof(u32), "" );
static_assert( sizeof(std::atomic<u64>) == sizeof(u64), "" );
static_assert( alignof(std::atomic<u64>) == alignof(u64), "" );

ycVirtualMemoryPoolTS::ycVirtualMemoryPoolTS( const char* debugName )
	: ycAllocatorTS( debugName )
	, m_growGuard( "ycVirtualMemoryPoolTS" )
{
}

ycVirtualMemoryPoolTS::ycVirtualMemoryPoolTS( const char* debugName, const ycSize_t blockSize, const ycSize_t commitIncrement, const ycSize_t maxBytes )
	: ycAllocatorTS( debugName )
	, m_growGuard( "ycVirtualMemoryPoolTS" )
{
	Init( blockSize, commitIncrement, maxBytes );
}

void ycVirtualMemoryPoolTS::Init( const ycSize_t blockSize, const ycSize_t _commitIncrement, const ycSize_t _maxBytes )
{
	ycAssert( m_base == nullptr );
	ycAssert( _commitIncrement > 1 ); // catch old callsites that are passing a 'largePages' bool

	const ycSize_t commitIncrement = ycVirtualMem::RoundToPageSize( _commitIncrement );
	const ycSize_t maxBytes = ycRoundUpPow2( _maxBytes, commitIncrement );
	ycAssert( maxBytes );

	m_base = (u8*)ycVirtualMem::Reserve( maxBytes );
	m_blockSize = blockSize;
	m_commitIncrement = commitIncrement;
	m_maxSize = maxBytes;
}

/*virtual*/ ycVirtualMemoryPoolTS::~ycVirtualMemoryPoolTS()
{
	Clear();
}

void ycVirtualMemoryPoolTS::Clear()
{
	#if YC_NDA
	#endif 
	if( m_base  )
	{
		ycVirtualMem::Release( m_base, m_maxSize );
	}
	m_base = 0;
	m_committed = 0;
	reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->store( kInvalidTaggedIndex.asU64 );
	reinterpret_cast< std::atomic< u32 >* >( &m_tag )->store( 0 );
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycVirtualMemoryPoolTS )
{
	YC_UNUSED( debugName ); YC_UNUSED( file ); YC_UNUSED( line ); YC_UNUSED( flags );
	ycAssert( bytes <= m_blockSize );
	ycAssert( align <= m_blockSize );

	TaggedIndex head;
	head.asU64 = reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->load();
	for(;;)
	{
		if( head.asU64 == kInvalidTaggedIndex.asU64 )
		{
			// grow, reload head, retry
			Expand();
			head.asU64 = reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->load(); // maybe return from expand?
			continue;
		}

		u8* ptr = m_base + head.idx*m_blockSize;
		TaggedIndex next = *( (TaggedIndex*)ptr );
		if( reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->compare_exchange_strong( head.asU64, next.asU64 ) )
		{
			ycAssert( (uintptr_t(ptr)%align) == 0 );
			ycAssert( ptr >= m_base && ptr < m_base + m_committed );
			return ptr;
		}
		else
		{
			// try again
		}
	}
}

YC_ALLOCATOR_DECLARATION_FREE( ycVirtualMemoryPoolTS )
{
	YC_UNUSED( flags );
	FreeList( (u8*)ptr, (u8*)ptr );
}

YC_ALLOCATOR_DECLARATION_SIZE( ycVirtualMemoryPoolTS )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	ycAssert( false );
	return 0;
}

void ycVirtualMemoryPoolTS::FreeList( u8* head, u8* tail )
{
	const u32 tag = reinterpret_cast< std::atomic< u32 >* >( &m_tag )->fetch_add( 1, std::memory_order_relaxed );

	TaggedIndex newHead;
	newHead.tag = tag;
	newHead.idx = IndexOf( head );

	TaggedIndex refHead;
	refHead.asU64 = reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->load();
	for(;;)
	{
		// point tail at current head
		*reinterpret_cast< TaggedIndex* >( tail ) = refHead;

		// swap new head
		if( reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->compare_exchange_strong( refHead.asU64, newHead.asU64 ) )
		{
			return;
		}
		else
		{
			// try again
		}
	}
}

// TODO HACK FIXME: maxsize
void ycVirtualMemoryPoolTS::Expand()
{
	m_growGuard.Lock();

	// re-check head inside the lock, someone may have beat us to expanding
	if( reinterpret_cast< std::atomic< u64 >* >( &m_freeHead )->load() != kInvalidTaggedIndex.asU64 )
	{
		m_growGuard.Unlock();
		return;
	}

	const ycSize_t oldCount = m_committed / m_blockSize;
	const ycSize_t commit = m_commitIncrement;
	const ycSize_t newCount = (m_committed+commit) / m_blockSize;
	const ycSize_t newBlocks = newCount - oldCount;

	#if YC_NDA
	#else
		ycAssert( newBlocks );
		ycAssert( m_committed + commit <= m_maxSize );
		ycVirtualMem::Commit( m_base + m_committed, commit );
		u8* newBlocksStart = m_base+(m_blockSize*oldCount);
		m_committed += commit;
		Link( IndexOf(newBlocksStart), newBlocks );
		FreeList( newBlocksStart, newBlocksStart + m_blockSize*(newBlocks-1) );
	#endif
	m_growGuard.Unlock();
}

void ycVirtualMemoryPoolTS::Link( const ycSize_t idx, const ycSize_t count )
{
	const u32 tag = reinterpret_cast< std::atomic< u32 >* >( &m_tag )->load();
	u8* base = m_base + m_blockSize*idx;
	for( ycSize_t i = 0; i != count; ++i )
	{
		TaggedIndex* tagged = ( TaggedIndex* )( base + m_blockSize*i );
		tagged->tag = tag;
		tagged->idx = u32(idx+i+1); // the last one will point to junk but we're about to override that with the list head
	}
}

u32 ycVirtualMemoryPoolTS::IndexOf( u8* block )
{
	ycAssert( block >= m_base );
	const ycSize_t offs = block - m_base;
	ycAssert( (uintptr_t(offs)%m_blockSize) == 0 );
	return u32( offs / m_blockSize );
}
