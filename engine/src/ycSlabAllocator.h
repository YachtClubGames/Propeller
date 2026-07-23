#pragma once

/*! \class ycSlabAllocator
https://en.wikipedia.org/wiki/Slab_allocation
// TODO HACK FIXME: blockAlign is not properly respected (see AllocInternal -- it's used for both ToC pages and blocks though so dont just change it to always use block align!)

*/

#include "ycAllocator.h"

class ycSlabAllocator : public ycAllocator
{
public:

	ycSlabAllocator( const char* debugName );
	void Init( const char* debugName, const ycSize_t blockSize, ycAllocator* mem, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN );
	ycSlabAllocator( const char* debugName, const ycSize_t blockSize, ycAllocator* mem, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN );
	virtual ~ycSlabAllocator();

	YC_ALLOCATOR_DECLARATIONS

	void Clear();

	ycSize_t GetBlockSize() const; //!< This may be slightly larger than the originally requested size, to meet alignment restrictions
	ycSize_t GetBlockAlign() const;

	#ifdef YC_ENABLE_MEM_DEBUG
		ycSize_t m_allocatedBytes;
		ycSize_t m_usedBytes;
	#endif

protected:
	
	ycSize_t m_pageSize;
	//ycSize_t m_blockSize;
	ycSize_t m_blockStride;

	ycSize_t m_numBlocksPerPage;
	ycSize_t m_numPageDescsPerPage;
	ycSize_t m_numBlocksPerToc;
	
	struct TocHeader;
	TocHeader* m_firstFreeToc;
	TocHeader* m_firstFullToc;
	
	ycSize_t CalcNumBlocksPerPage() const;
	ycSize_t CalcNumPageDescsPerPage() const;

	ycAllocator* m_mem;

	ycSize_t m_blockAlign;

	/*
	Page
		AllocHeader
		Block
		AllocHeader
		Block
		....

	TocPage
		TocHeader
		PageDesc
		PageDesc
		...
	*/

	static const ycSize_t kInvalidFreeIdx = ycSize_t(-1LL);

	struct PageDesc // possibly better to soa these, but do this for ease of first implementation; some fields could be smaller too
	{
		PageDesc* prev;
		PageDesc* next;
		ycSize_t  numUsedBlocks;
		ycSize_t  firstFreeIdx; //!< index of first free block
		uint8_t*  blocks;
		ycSize_t  pageIdx; //!< offset from TocHeader
	};

	struct TocHeader
	{
		ycSlabAllocator* allocator;
		TocHeader*       prev;
		TocHeader*       next;
		ycSize_t         numUsedBlocks;        //!< allocated by ycSlabAllocatorTS
		ycSize_t         numAllocatedBlocks;   //!< occupying memory; allocated by the external mem mgr
		PageDesc*        firstFreePage;        //!< list of non-full pages, with the fullest first
		PageDesc*        firstFullPage;        //!< list of full pages
		PageDesc*        firstUnallocatedPage; //!< no external memory allocated
	};

	struct AllocHeader
	{
		PageDesc*    pageDesc;
		AllocHeader* nextFree; //!< used if free, otherwise kInvalidFreeIdx
		#if !defined YC_64 && YC_MEM_DEFAULT_ALIGN >= 16
			uint32_t pad[2];
		#endif
	};

	TocHeader* AllocToc(); //!< assumes m_firstFreeToc is NULL; also allocates one page of blocks, and one block out of that page.  Sets new TOC to m_firstFreeToc and returns the allocted block.

	PageDesc* AllocPage( TocHeader* toc );

	AllocHeader* AllocBlock( PageDesc* pageDesc );

	void* AllocInternal();
	void FreeInternal( void* ptr );

	TocHeader* GetTocHeader( PageDesc* pageDesc );
	ycSize_t GetBlockPageIdx( PageDesc* pageDesc, AllocHeader* allocHeader );

	void Check(); //!< Walks over all data structures, verifying that they look sane
	void Check( TocHeader* toc );
	void Check( PageDesc* page );
	
	void SortToc( TocHeader* toc );
	void SortPage( TocHeader* toc, PageDesc* pageDesc );
};
