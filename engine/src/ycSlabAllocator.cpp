#include "ycSlabAllocator.h"

#include "ycAllocator.h"
#include "ycMath.h"
#include "ycMemTrack.h"


#if 1
	#define MEM_LOG( ... )
#else
	#define MEM_LOG( ... ) ycLog( __VA_ARGS__ )
#endif

/* Sorting tries to arrange allocations to make it less likely to have large
numbers of pages with low utilization.  However, in practice it doesn't seem to
matter */
//#define ENABLE_SORTING

ycSlabAllocator::ycSlabAllocator( const char* debugName ) : ycAllocator( debugName )
{
	// so dtor would do nothing if not initialized
	m_firstFreeToc = nullptr;
	m_firstFullToc = nullptr;
}

void ycSlabAllocator::Init( const char* debugName, const ycSize_t blockSize, ycAllocator* mem, const ycSize_t pageSize /*= YC_MEM_DEFAULT_PAGE_SIZE*/, const ycSize_t blockAlign /*= YC_MEM_DEFAULT_ALIGN */ )
{
	m_pageSize     = pageSize;
	//m_blockSize    = blockSize;
	m_blockAlign   = ycMax( blockAlign, ycSize_t(YC_MEM_DEFAULT_ALIGN) );
	m_blockStride  = ycRoundUpPow2( blockSize + sizeof( AllocHeader ), m_blockAlign );
	m_firstFreeToc = nullptr;
	m_firstFullToc = nullptr;
	m_mem          = mem ? mem : g_defaultMem;
#ifdef YC_ENABLE_DEBUG_STRINGS
	m_name = debugName;
#else
	YC_UNUSED( debugName );
#endif
#ifdef YC_ENABLE_MEM_DEBUG
	m_allocatedBytes = 0;
	m_usedBytes      = 0;
#endif
	m_numBlocksPerPage    = CalcNumBlocksPerPage();
	m_numPageDescsPerPage = CalcNumPageDescsPerPage();
	m_numBlocksPerToc     = m_numBlocksPerPage*m_numPageDescsPerPage;
	ycAssert( m_numBlocksPerPage > 0 );
	Check();
}

ycSlabAllocator::ycSlabAllocator( const char* debugName, const ycSize_t blockSize, ycAllocator* mem, const ycSize_t pageSize /*= YC_MEM_DEFAULT_PAGE_SIZE*/, const ycSize_t blockAlign /*= YC_MEM_DEFAULT_ALIGN*/ )
	: ycAllocator( debugName )
{
	Init( debugName, blockSize, mem, pageSize, blockAlign );
}

ycSlabAllocator::~ycSlabAllocator()
{
	Clear();
}

void ycSlabAllocator::Clear()
{
	Check();
	if( m_firstFreeToc )
	{
		for( TocHeader* toc = m_firstFreeToc; toc; toc = toc->next )
		{
			PageDesc* pageDesc = (PageDesc*)( toc + 1 );
			for( ycSize_t i = 0; i != m_numPageDescsPerPage; ++i, ++pageDesc )
			{
				if( pageDesc->blocks ) { FreeInternal( pageDesc->blocks ); }
			}
		}
		m_firstFreeToc = nullptr;
	}
	if( m_firstFullToc )
	{
		for( TocHeader* toc = m_firstFullToc; toc; toc = toc->next )
		{
			PageDesc* pageDesc = (PageDesc*)( toc + 1 );
			for( ycSize_t i = 0; i != m_numPageDescsPerPage; ++i, ++pageDesc )
			{
				if( pageDesc->blocks ) { FreeInternal( pageDesc->blocks ); }
			}
		}
		m_firstFullToc = nullptr;
	}
#ifdef YC_ENABLE_MEM_DEBUG
	m_allocatedBytes = 0;
	m_usedBytes      = 0;
#endif
	Check();

	ycMemTrack::ClearAllocator(this);
}

ycSize_t ycSlabAllocator::CalcNumBlocksPerPage() const
{
	return m_pageSize / m_blockStride;
}

ycSize_t ycSlabAllocator::CalcNumPageDescsPerPage() const
{
	const ycSize_t usableSize   = m_pageSize - sizeof( TocHeader );
	const ycSize_t pageDescSize = sizeof( PageDesc );
	return usableSize / pageDescSize;
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycSlabAllocator )
{
	YC_UNUSED( debugName );
	YC_UNUSED( file );
	YC_UNUSED( line );
	ycAssert( align <= m_blockAlign );
	YC_UNUSED( align );
	ycAssert( bytes <= m_blockStride );
	YC_UNUSED( bytes );

	ycAssert( bytes != 0 );

	MEM_LOG( "Alloc\n" );
	void* alloc = nullptr;

	Check();

	// if all TOCs are full
	if( m_firstFreeToc == nullptr )
	{ // create a new toc, allocate a page in it, and allocate a block from that page
		MEM_LOG( "  A1\n" );
		alloc = AllocBlock( AllocPage( AllocToc() ) ) + 1 /* move past the alloc header to the ptr the user's ptr */ ;
	}
	else
	{
		// grab a toc with free space
		TocHeader* toc = m_firstFreeToc;
		ycAssert( toc->numUsedBlocks < m_numBlocksPerPage*m_numPageDescsPerPage );

		if( toc->firstFreePage )
		{
			MEM_LOG( "  B1\n" );
			alloc = AllocBlock( toc->firstFreePage ) + 1;
		}
		else
		{
			MEM_LOG( "  C1\n" );
			alloc = AllocBlock( AllocPage( toc ) ) + 1;
		}
	}

	Check();

	ycAssert( (flags&kAllocFlag_NoAssert) || alloc );
	YC_UNUSED( flags );
	return alloc;
}

YC_ALLOCATOR_DECLARATION_FREE( ycSlabAllocator )
{
	YC_UNUSED( flags );
	Check();

	AllocHeader* allocHeader = reinterpret_cast< AllocHeader* >( (uint8_t*)ptr - sizeof( AllocHeader ) );
	PageDesc* pageDesc = allocHeader->pageDesc;

	MEM_LOG( "Free\n" );
	if( pageDesc->firstFreeIdx != kInvalidFreeIdx )
	{
		MEM_LOG( "  A1\n" );
		AllocHeader* nextFree = (AllocHeader*)( pageDesc->blocks + m_blockStride*pageDesc->firstFreeIdx );
		allocHeader->nextFree = nextFree;
	}
	else
	{
		MEM_LOG( "  B1\n" );
		allocHeader->nextFree = nullptr;
	}
	pageDesc->firstFreeIdx = GetBlockPageIdx( pageDesc, allocHeader );
	ycAssert( pageDesc->numUsedBlocks > 0 );
	const ycSize_t pageNumUsedBlocks = --pageDesc->numUsedBlocks;
	TocHeader* toc = GetTocHeader( pageDesc );
	ycAssert( toc->numUsedBlocks > 0 );
	const ycSize_t tocNumUsedBlocks = --toc->numUsedBlocks;

	// if the page was full, move it to the free list
	if( pageNumUsedBlocks == m_numBlocksPerPage-1 )
	{
		MEM_LOG( "  A2\n" );
		// remove from full list
		if( pageDesc->prev == nullptr )
		{
			MEM_LOG( "    1\n" );
			ycAssert( toc->firstFullPage == pageDesc );
			toc->firstFullPage = pageDesc->next;
		}
		else
		{
			MEM_LOG( "    2\n" );
			pageDesc->prev->next = pageDesc->next;
		}
		if( pageDesc->next ) { pageDesc->next->prev = pageDesc->prev; }

		// add to front of free list (a page with one free spot would always be at the front of the free list, or tied for that spot)
		pageDesc->prev = nullptr;
		pageDesc->next = toc->firstFreePage;
		if( toc->firstFreePage ) { toc->firstFreePage->prev = pageDesc; }
		toc->firstFreePage = pageDesc;
	}
	// if the page is now empty, unallocate its memory
	else if( pageNumUsedBlocks == 0 )
	{
		MEM_LOG( "  B2\n" );
		FreeInternal( pageDesc->blocks );

		ycAssert( toc->numAllocatedBlocks >= m_numBlocksPerPage );
		toc->numAllocatedBlocks -= m_numBlocksPerPage;

		// if the toc is not about to be removed, update the toc/page
		if( toc->numAllocatedBlocks > 0 )
		{
			MEM_LOG( "    1\n" );
			pageDesc->blocks = nullptr;
			#if defined YC_ENABLE_MEM_DEBUG || defined YC_ENABLE_ASSERT
				pageDesc->firstFreeIdx = kInvalidFreeIdx;
			#endif

			// remove from free list
			if( pageDesc->prev == nullptr )
			{
				MEM_LOG( "      a\n" );
				ycAssert( toc->firstFreePage == pageDesc );
				toc->firstFreePage = pageDesc->next;
			}
			else
			{
				MEM_LOG( "      b\n" );
				pageDesc->prev->next = pageDesc->next;
			}
			if( pageDesc->next ) { pageDesc->next->prev = pageDesc->prev; }

			// add to front of unallocated list (all unallocated pages within the toc are equal, no sorting)
			pageDesc->prev = nullptr;
			pageDesc->next = toc->firstUnallocatedPage;
			if( toc->firstUnallocatedPage ) { toc->firstUnallocatedPage->prev = pageDesc; }
			toc->firstUnallocatedPage = pageDesc;
		}
		else
		{
			ycAssert( tocNumUsedBlocks == 0 );
		}
	}
	else
	{
		MEM_LOG( "  C2\n" );
		SortPage( toc, pageDesc );
	}

	// if the TOC was full, move it to the free list
	if( tocNumUsedBlocks == m_numBlocksPerToc-1 )
	{
		MEM_LOG( "  A3\n" );
		// remove from full list
		if( m_firstFullToc == toc )
		{
			MEM_LOG( "    1\n" );
			ycAssert( toc->prev == nullptr );
			m_firstFullToc = toc->next;
		}
		else
		{
			MEM_LOG( "    2\n" );
			ycAssert( toc->prev != nullptr );
			toc->prev->next = toc->next;
		}
		if( toc->next ) { toc->next->prev = toc->prev; }
		// add to front of free list (a page with one free spot would always be at the front of the free list, or tied for that spot)
		toc->prev = nullptr;
		toc->next = m_firstFreeToc;
		if( m_firstFreeToc ) { m_firstFreeToc->prev = toc; }
		m_firstFreeToc = toc;
	}
	// if the toc is now empty, destroy it
	else if( tocNumUsedBlocks == 0 )
	{
		MEM_LOG( "  B3\n" );
		// remove from free list
		if( toc->prev == nullptr )
		{
			MEM_LOG( "    1\n" );
			ycAssert( m_firstFreeToc == toc );
			m_firstFreeToc = toc->next;
		}
		else
		{
			MEM_LOG( "    2\n" );
			toc->prev->next = toc->next;
		}
		if( toc->next ) { toc->next->prev = toc->prev; }

		// deallocate mem
		FreeInternal( toc );
		#ifdef YC_ENABLE_MEM_DEBUG // always consider a toc 'fully used' ?
			m_usedBytes -= m_pageSize;
		#endif
	}
	else
	{
		MEM_LOG( "  C3\n" );
		// if the toc was already in the free list, move it if necessary
		SortToc( toc );
	}
	Check();

	#ifdef YC_ENABLE_MEM_DEBUG
		m_usedBytes -= m_blockStride;
	#endif

	//m_critSec.Leave();
}


YC_ALLOCATOR_DECLARATION_SIZE( ycSlabAllocator )
{
	YC_UNUSED( align );
	Check();
	AllocHeader* allocHeader = reinterpret_cast< AllocHeader* >( ( uint8_t* )ptr - sizeof( AllocHeader ) );
	PageDesc* pageDesc = allocHeader->pageDesc;
	return pageDesc->numUsedBlocks * m_pageSize;
}

void ycSlabAllocator::SortToc( TocHeader* toc )
{
#if !defined ENABLE_SORTING
	YC_UNUSED( toc );
#else
	const u32 numUsedBlocks = toc->numUsedBlocks;
	while( toc->prev && toc->prev->numUsedBlocks < numUsedBlocks )
	{
		if( m_firstFreeToc == toc->prev ) { m_firstFreeToc = toc; } // only needs to happen once (and if it does happen, this is the only iteration)
		if( toc->next ) { toc->next->prev = toc->prev; }
		TocHeader* newPrev = toc->prev->prev;
		toc->prev->next = toc->next;
		toc->prev->prev = toc;
		toc->next = toc->prev;
		toc->prev = newPrev;
		if( newPrev ) { newPrev->next = toc; }
	}
	while( toc->next && toc->next->numUsedBlocks > numUsedBlocks )
	{
		if( m_firstFreeToc == toc ) { m_firstFreeToc = toc->next; } // only needs to happen on the first iteration...
		if( toc->prev ) { toc->prev->next = toc->next; }
		TocHeader* newNext = toc->next->next;
		toc->next->prev = toc->prev;
		toc->next->next = toc;
		toc->prev = toc->next;
		toc->next = newNext;
		if( newNext ) { newNext->prev = toc; }
	}
#endif
}

void ycSlabAllocator::SortPage( TocHeader* toc, PageDesc* pageDesc )
{
#if !defined ENABLE_SORTING
	YC_UNUSED( toc );
	YC_UNUSED( pageDesc );
#else
	const u32 numUsedBlocks = pageDesc->numUsedBlocks;
	while( pageDesc->prev && pageDesc->prev->numUsedBlocks < numUsedBlocks )
	{
		if( toc->firstFreePage == pageDesc->prev ) { toc->firstFreePage = pageDesc; } // only needs to happen once (and if it does happen, this is the only iteration)
		if( pageDesc->next ) { pageDesc->next->prev = pageDesc->prev; }
		PageDesc* newPrev = pageDesc->prev->prev;
		pageDesc->prev->next = pageDesc->next;
		pageDesc->prev->prev = pageDesc;
		pageDesc->next = pageDesc->prev;
		pageDesc->prev = newPrev;
		if( newPrev ) { pageDesc->next = pageDesc; }
	}
	while( pageDesc->next && pageDesc->next->numUsedBlocks > numUsedBlocks )
	{
		if( toc->firstFreePage == pageDesc ) { toc->firstFreePage = pageDesc->next; } // only needs to happen on the first iteration...
		if( pageDesc->prev ) { pageDesc->prev->next = pageDesc->next; }
		PageDesc* newNext = pageDesc->next->next;
		pageDesc->next->prev = pageDesc->prev;
		pageDesc->next->next = pageDesc;
		pageDesc->prev = pageDesc->next;
		pageDesc->next = newNext;
		if( newNext ) { newNext->prev = pageDesc; }
	}
#endif
}

ycSlabAllocator::TocHeader* ycSlabAllocator::AllocToc()
{
	ycAssertMsg( m_firstFreeToc == nullptr, "Trying to allocate a new TOC if there are other free ones." );

	// alloc toc mem
	TocHeader* toc = (TocHeader*)AllocInternal();

	// alloc page mem
	//AllocHeader* firstAllocHeader = (AllocHeader*)AllocInternal();

	// set as free list head
	m_firstFreeToc = toc;

	// grab ptr to first page desc
	PageDesc* pageDesc = (PageDesc*)( toc + 1 );

	// init toc header
	toc->allocator            = this;
	toc->prev                 = nullptr;
	toc->next                 = nullptr;
	toc->numUsedBlocks        = 0;
	toc->numAllocatedBlocks   = 0;
	toc->firstFreePage        = nullptr;
	toc->firstFullPage        = nullptr;
	toc->firstUnallocatedPage = pageDesc;

	// init pages
	const ycSize_t numPageDescsPerPage = m_numPageDescsPerPage;
	for( ycSize_t i = 0; i != numPageDescsPerPage; ++i, ++pageDesc )
	{
		pageDesc->prev          = pageDesc - 1;
		pageDesc->next          = pageDesc + 1;
		pageDesc->numUsedBlocks = 0;
		pageDesc->firstFreeIdx  = kInvalidFreeIdx;
		pageDesc->blocks        = nullptr;
		pageDesc->pageIdx       = i;
	}
	( pageDesc - 1 )->next = nullptr; // no next page for the last one
	reinterpret_cast< PageDesc* >( toc + 1 )->prev = nullptr; // no prev page for the first one

	#ifdef YC_ENABLE_MEM_DEBUG // always consider a toc 'fully used' ?
		m_usedBytes += m_pageSize;
	#endif

	return toc;
}

ycSlabAllocator::PageDesc* ycSlabAllocator::AllocPage( TocHeader* toc )
{
	// take first unallocated page and remove it from the unallocated list
	ycAssertMsg( toc->firstUnallocatedPage, "TOC has no unallocated pages." );
	PageDesc* pageDesc = toc->firstUnallocatedPage;
	toc->firstUnallocatedPage = pageDesc->next;
	if( pageDesc->next ) { pageDesc->next->prev = nullptr; }

	// add the page to the free list

// FIXME: unallocated list? just give in and chain the functions already.
	ycAssertMsg( toc->firstFreePage == nullptr, "Trying to alloc a page, but TOC still has free pages." );
	toc->firstFreePage = pageDesc;

	// update toc
	toc->numAllocatedBlocks += m_numBlocksPerPage;

	// init page
	pageDesc->prev          = pageDesc->next = nullptr;
	pageDesc->numUsedBlocks = 0;
	pageDesc->firstFreeIdx  = 0;
	pageDesc->blocks        = (uint8_t*)AllocInternal();

	// init blocks
	const ycSize_t blockStride = m_blockStride;
	uint8_t* pBlock = pageDesc->blocks;
	for( ycSize_t i = 0; i != m_numBlocksPerPage; ++i )
	{
		AllocHeader* block     = (AllocHeader*)pBlock;
		AllocHeader* nextBlock = (AllocHeader*)( pBlock + blockStride );
		block->pageDesc = pageDesc;
		block->nextFree = nextBlock;
		pBlock = (uint8_t*)nextBlock;
	}
	reinterpret_cast< AllocHeader* >( pBlock - blockStride )->nextFree = nullptr; // no nextFree for the last block

	return pageDesc;
}

ycSlabAllocator::AllocHeader* ycSlabAllocator::AllocBlock( PageDesc* pageDesc )
{
	uint8_t* blockBase = pageDesc->blocks;

	ycAssertMsg( pageDesc->numUsedBlocks < m_numBlocksPerPage, "Trying to alloc a block, but page is full." );
	ycAssertMsg( pageDesc->firstFreeIdx != kInvalidFreeIdx, "Trying to allocate a block, but no blocks are free." );
	AllocHeader* block = (AllocHeader*)( blockBase + m_blockStride*pageDesc->firstFreeIdx );

	ycAssert( uintptr_t(block->nextFree) != uintptr_t(kInvalidFreeIdx) );

	uint8_t* nextFree = (uint8_t*)block->nextFree;
	const ycSize_t blockStride = m_blockStride;
	pageDesc->firstFreeIdx = ycSize_t( (nextFree-blockBase) / blockStride );

#if defined YC_ENABLE_MEM_DEBUG || defined YC_ENABLE_ASSERT
	block->nextFree = (AllocHeader*)uintptr_t(kInvalidFreeIdx);
#endif
	
	TocHeader* toc = GetTocHeader( pageDesc );

	// if the page is now full, move it from the free list to the full list
	if( ++pageDesc->numUsedBlocks == m_numBlocksPerPage )
	{
		#if defined YC_ENABLE_MEM_DEBUG || defined YC_ENABLE_ASSERT
			pageDesc->firstFreeIdx = kInvalidFreeIdx;
		#endif
		if( pageDesc->prev ) { pageDesc->prev->next = pageDesc->next; }
		else
		{
			ycAssert( toc->firstFreePage == pageDesc );
			toc->firstFreePage = pageDesc->next;
		}
		if( pageDesc->next ) { pageDesc->next->prev = pageDesc->prev; }
		pageDesc->prev = nullptr;
		pageDesc->next = toc->firstFullPage;
		if( toc->firstFullPage ) { toc->firstFullPage->prev = pageDesc; }
		toc->firstFullPage = pageDesc;
	}
	else
	{
		SortPage( toc, pageDesc );
	}
	ycAssert( pageDesc->numUsedBlocks <= m_numBlocksPerPage );

	// if the toc is now full, move it from the free list to the full list
	if( ++toc->numUsedBlocks == m_numBlocksPerToc )
	{
		// remove from free list
		ycAssert( toc->firstFreePage == nullptr );
		ycAssert( toc->firstUnallocatedPage == nullptr );
		if( m_firstFreeToc == toc )
		{
			ycAssert( toc->prev == nullptr );
			m_firstFreeToc = toc->next;
		}
		else
		{
			ycAssert( toc->prev != nullptr );
			toc->prev->next = toc->next;
		}
		if( toc->next ) { toc->next->prev = toc->prev; }
		// add to full list
		toc->prev = nullptr;
		toc->next = m_firstFullToc;
		if( m_firstFullToc ) { m_firstFullToc->prev = toc; }
		m_firstFullToc = toc;
	}
	else
	{
		// if the toc was already in the free list, move it if necessary
		SortToc( toc );
	}

	#ifdef YC_ENABLE_MEM_DEBUG
		m_usedBytes += m_blockStride;
	#endif

	return block;
}

void* ycSlabAllocator::AllocInternal()
{
	const char* debugName = "ycSlabAllocator";
	#ifdef YC_ENABLE_DEBUG_STRINGS
		debugName = m_name;
	#else
		YC_UNUSED( debugName );
	#endif
	#ifdef YC_ENABLE_MEM_DEBUG
		m_allocatedBytes += m_pageSize;
	#endif
	return YC_ALLOC( m_mem, m_pageSize, YC_MEM_DEFAULT_ALIGN, debugName );
}

void ycSlabAllocator::FreeInternal( void* ptr )
{
	#ifdef YC_ENABLE_MEM_DEBUG
		m_allocatedBytes -= m_pageSize;
	#endif
	YC_FREE( m_mem, ptr );
}

ycSlabAllocator::TocHeader* ycSlabAllocator::GetTocHeader( PageDesc* pageDesc )
{
	return reinterpret_cast< TocHeader* >( (uint8_t*)( pageDesc - pageDesc->pageIdx ) - sizeof(TocHeader) );
}

ycSize_t ycSlabAllocator::GetBlockPageIdx( PageDesc* pageDesc, AllocHeader* allocHeader )
{
	ycAssert( pageDesc == allocHeader->pageDesc );
	const ycSize_t offset = (uint8_t*)allocHeader - pageDesc->blocks;
	ycAssert( offset % m_blockStride == 0 );
	return ycSize_t( offset / m_blockStride );
}

void ycSlabAllocator::Check()
{
#ifdef YC_HEAP_VALIDATE
	if( m_firstFreeToc )
	{
		ycAssert( m_firstFreeToc->prev == nullptr );
		for( TocHeader* toc = m_firstFreeToc; toc; toc = toc->next )
		{
			Check( toc );
		}
	}
	if( m_firstFullToc )
	{
		ycAssert( m_firstFullToc->prev == nullptr );
		for( TocHeader* toc = m_firstFullToc; toc; toc = toc->next )
		{
			Check( toc );
		}
	}
#endif
}

void ycSlabAllocator::Check( TocHeader* toc )
{
#if !defined YC_HEAP_VALIDATE
	YC_UNUSED( toc );
#else
	ycAssert( toc->allocator == this );
	ycAssert( toc->prev == nullptr || toc->prev->next == toc );
	ycAssert( toc->next == nullptr || toc->next->prev == toc );

	u32 tocNumUsedBlocksFound      = 0;
	u32 tocNumAllocatedBlocksFound = 0;
	u32 tocNumPagesFound           = 0;

	if( toc->firstFreePage )
	{
		ycAssert( toc->firstFreePage->prev == nullptr );
		for( PageDesc* pageDesc = toc->firstFreePage; pageDesc; pageDesc = pageDesc->next )
		{
			ycAssert( pageDesc->blocks );
			Check( pageDesc );
			++tocNumPagesFound;
			tocNumAllocatedBlocksFound += m_numBlocksPerPage;
			tocNumUsedBlocksFound += pageDesc->numUsedBlocks;
		}
	}

	if( toc->firstFullPage )
	{
		ycAssert( toc->firstFullPage->prev == nullptr );
		for( PageDesc* pageDesc = toc->firstFullPage; pageDesc; pageDesc = pageDesc->next )
		{
			ycAssert( pageDesc->blocks );
			Check( pageDesc );
			++tocNumPagesFound;
			tocNumAllocatedBlocksFound += m_numBlocksPerPage;
			tocNumUsedBlocksFound += pageDesc->numUsedBlocks;
		}
	}

	if( toc->firstUnallocatedPage )
	{
		ycAssert( toc->firstUnallocatedPage->prev == nullptr );
		for( PageDesc* pageDesc = toc->firstUnallocatedPage; pageDesc; pageDesc = pageDesc->next )
		{
			ycAssert( pageDesc->blocks == nullptr );
			Check( pageDesc );
			++tocNumPagesFound;
			tocNumUsedBlocksFound += pageDesc->numUsedBlocks;
		}
	}

	ycAssert( tocNumUsedBlocksFound == toc->numUsedBlocks );
	ycAssert( tocNumAllocatedBlocksFound == toc->numAllocatedBlocks );
	ycAssert( tocNumPagesFound == m_numPageDescsPerPage );
#endif
}

void ycSlabAllocator::Check( PageDesc* pageDesc )
{
#if !defined YC_HEAP_VALIDATE
	YC_UNUSED( pageDesc );
#else
	ycAssert( pageDesc->prev == nullptr || pageDesc->prev->next == pageDesc );
	ycAssert( pageDesc->next == nullptr || pageDesc->next->prev == pageDesc );
	if( pageDesc->blocks )
	{
		ycAssert( pageDesc->numUsedBlocks > 0 );
		u32 numUsedBlocks = 0;
		for( u32 i = 0; i != m_numBlocksPerPage; ++i )
		{
			AllocHeader* block  = (AllocHeader*)( pageDesc->blocks + m_blockStride*i );
			ycAssert( block->pageDesc == pageDesc );
			if( block->nextFree == (AllocHeader*)uintptr_t(kInvalidFreeIdx) )
			{
				++numUsedBlocks;
			}
		}
		ycAssert( numUsedBlocks == pageDesc->numUsedBlocks );
		if( pageDesc->firstFreeIdx != kInvalidFreeIdx )
		{
			u32 numFreeBlocks = 0;
			AllocHeader* firstFree  = (AllocHeader*)( pageDesc->blocks + m_blockStride*pageDesc->firstFreeIdx );
			for( AllocHeader* block = firstFree; block; block = block->nextFree )
			{
				++numFreeBlocks;
			}
			ycAssert( numFreeBlocks == m_numBlocksPerPage - numUsedBlocks );
		}
	}
	else
	{
		ycAssert( pageDesc->numUsedBlocks == 0 );
		ycAssert( pageDesc->firstFreeIdx == kInvalidFreeIdx );
	}
#endif
}

///*static*/ void ycSlabAllocator::Dump( void* pAlloc, const char* indent )
//{
//#ifdef YC_ENABLE_MEM_DEBUG
//	ycSlabAllocator* pThis = reinterpret_cast< ycSlabAllocator* >( pAlloc );
//	if( pThis->m_allocatedBytes )
//	{
//		ycLog( "%s  %s %uKB [%.2f%%]\n", indent, pThis->GetDebugName(), pThis->m_allocatedBytes/1024, f32(pThis->m_usedBytes)/f32(pThis->m_allocatedBytes) * 100.0f );
//	}
//	else
//	{
//		ycLog( "%s  %s 0KB\n", indent, pThis->GetDebugName() );
//	}
//#endif
//}

ycSize_t ycSlabAllocator::GetBlockSize() const
{
	//return m_blockSize;
	return m_blockStride;
}

ycSize_t ycSlabAllocator::GetBlockAlign() const
{
	return m_blockAlign;
}

////////////////////

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycSlabAllocator_Sanity )
{
	// HACK FIXME TODO: what are we gonna do about this?
	//YC_TEST_CHECK( sizeof( ycSlabAllocator::AllocHeader ) % 16 == 0 ); // YC_MEM_DEFAULT_ALIGN ?
	YC_TEST_PASS( "ycSlabAllocator sanity check" );
}

YC_BEGIN_TEST( ycSlabAllocator_Fill_Empty )
{
	enum
	{
		kNumAllocs = 8192,
		kBlockSize = 16
	};

	{
		ycAllocator* sysMem = ycTest::GetMem();
		ycSlabAllocator slabAlloc( "Test", kBlockSize, sysMem, YC_MEM_DEFAULT_PAGE_SIZE );

		YC_TEST_CHECK( slabAlloc.GetBlockSize() >= kBlockSize );

		void** allocs = (void**)sysMem->Alloc( sizeof(void*) * kNumAllocs, YC_PTR_SIZE, "allocs", __FILE__, __LINE__ );
		ycMemSet( allocs, 0, sizeof(void*) * kNumAllocs );

		for( ycSize_t i = 0; i != kNumAllocs; ++i )
		{
			allocs[ i ] = slabAlloc.Alloc( kBlockSize );
		}
		for( ycSize_t i = 0; i != kNumAllocs; ++i )
		{
			slabAlloc.Free( allocs[ i ] );
		}

		sysMem->Free( allocs );
	} // include dtor in the test

	YC_TEST_PASS( "Simple allocation and free check" );
}

#include "ycRand.h"

YC_BEGIN_TEST( ycSlabAllocator_Random )
{
	enum
	{
		kBlockSize  = 32,
		#ifdef YC_ENABLE_SLOW_TESTS
			kIterations = 1024*1024,
		#else
			kIterations = 1024*64,
		#endif
		kMaxAllocs = 8192,
	};
	ycRand randCtx;
	
	{
		ycAllocator* sysMem = ycTest::GetMem();
		ycSlabAllocator slabAlloc( "Test", kBlockSize, sysMem, YC_MEM_DEFAULT_PAGE_SIZE );

		YC_TEST_CHECK( slabAlloc.GetBlockSize() >= kBlockSize );

		void** allocs = (void**)sysMem->Alloc( sizeof(void*) * kMaxAllocs, YC_PTR_SIZE, "allocs", __FILE__, __LINE__ );
		ycMemSet( allocs, 0, sizeof(void*) * kMaxAllocs );

		for( uint64_t i = 0; i != kIterations; ++i )
		{
			const int32_t j = randCtx.RangeFast( 0, kMaxAllocs-1 );
			if( allocs[ j ] == nullptr )
			{
				allocs[ j ] = slabAlloc.Alloc( randCtx.RangeFast(1,kBlockSize) );
			}
			else
			{
				slabAlloc.Free( allocs[ j ] );
				allocs[ j ] = nullptr;
			}
		}

		for( ycSize_t i = 0; i != kMaxAllocs; ++i )
		{
			if( allocs[ i ] )
			{
				slabAlloc.Free( allocs[ i ] );
			}
		}

		sysMem->Free( allocs );
	} // include dtor in the test

	YC_TEST_PASS( "Random pattern of allocations and frees" );
}

#endif // YC_TEST
