#include "ycVirtualMemoryPool.h"

#include "ycMath.h"
#include "ycVirtualMem.h"

ycVirtualMemoryPool::ycVirtualMemoryPool( const char* debugName )
	: ycAllocator( debugName )
{
}

ycVirtualMemoryPool::ycVirtualMemoryPool( const char* debugName, const ycSize_t blockSize, const ycSize_t commitIncrement, const ycSize_t maxBytes )
	: ycAllocator( debugName )
{
	Init( blockSize, commitIncrement, maxBytes );
}

void ycVirtualMemoryPool::Init( const ycSize_t blockSize, const ycSize_t _commitIncrement, const ycSize_t _maxBytes )
{
	ycAssert( m_base == nullptr );
	ycAssert( _commitIncrement > 1 ); // catch old callsites that are passing a 'largePages' bool

	const ycSize_t commitIncrement = ycVirtualMem::RoundToPageSize( _commitIncrement );
	const ycSize_t maxBytes = ycRoundUpPow2( _maxBytes, commitIncrement );
	ycAssert( maxBytes );

	m_blockSize = blockSize;
	m_commitIncrement = commitIncrement;
	m_base = (u8*)ycVirtualMem::Reserve( maxBytes );
	m_maxSize = maxBytes;

	#if YC_NDA
	#endif
}

/*virtual*/ ycVirtualMemoryPool::~ycVirtualMemoryPool()
{
	Clear();
}

void ycVirtualMemoryPool::Clear()
{
	#if YC_NDA
	#endif 
	if( m_base )
	{
		ycVirtualMem::Release( m_base, m_maxSize );
	}
	m_base = nullptr;
	m_committed = 0;
	m_freeHead = nullptr;
}

// TODO HACK FIXME: maxsize
YC_ALLOCATOR_DECLARATION_ALLOC( ycVirtualMemoryPool )
{
	YC_UNUSED( debugName ); YC_UNUSED( file ); YC_UNUSED( line ); YC_UNUSED( flags );
	ycAssert( bytes <= m_blockSize );
	ycAssert( bytes != 0 );

	// TODO HACK FIXME: m_maxSize assert
	if(YC_UNLIKELY( m_freeHead == nullptr ))
	{
		const ycSize_t oldCount = m_committed / m_blockSize;
		const ycSize_t commit = m_commitIncrement;
		const ycSize_t newCount = (m_committed+commit) / m_blockSize;
		const ycSize_t newBlocks = newCount - oldCount;
		ycAssert( newBlocks );
		#if YC_NDA
		#else
			ycVirtualMem::Commit( m_base + m_committed, commit );
			m_committed += commit;
			AddToFreeList( m_base + (oldCount*m_blockSize), newBlocks );
		#endif
	}

	ycAssert( m_freeHead );
	FreeNode* freeNode = m_freeHead;
	m_freeHead = freeNode->next;
	ycAssert( (uintptr_t(freeNode)%align) == 0 );
	return freeNode;
}

YC_ALLOCATOR_DECLARATION_FREE( ycVirtualMemoryPool )
{
	YC_UNUSED( flags );
	ycAssert( m_base <= ptr && ptr < m_base + m_committed );
	ycAssert( ( ((u8*)ptr-m_base) % m_blockSize ) == 0 );
	FreeNode* freeNode = ( FreeNode* )ptr;
	freeNode->next = m_freeHead;
	m_freeHead = freeNode;
}

YC_ALLOCATOR_DECLARATION_SIZE( ycVirtualMemoryPool )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	ycAssert( false );
	return 0;
}

void ycVirtualMemoryPool::AddToFreeList( u8* base, const ycSize_t count )
{
	// walk backward so that if someone requests a bunch of blocks, they get the nice forward order
	u8* block = base + (count*m_blockSize);
	FreeNode* nextFree = nullptr;
	for( ycSize_t i = 0; i != count; ++i )
	{
		block -= m_blockSize;
		FreeNode* freeNode = reinterpret_cast< FreeNode* >( block );
		freeNode->next = nextFree;
		nextFree = freeNode;
	}
	m_freeHead = nextFree;
}

#ifdef YC_NDA
#endif 
