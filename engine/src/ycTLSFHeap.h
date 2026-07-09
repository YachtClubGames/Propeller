#pragma once

#include "ycAllocator.h"

/* \class ycTLSFHeap
A heap based on the "Two Level Segregated Fit" algorithm:
http://www.gii.upv.es/tlsf/main/docs.html

With some adjustments to accommodate dynamically resizing the heap and more
restrictive alignments.
*/

//#define YC_TLSFHEAP_DEBUG

class ycTLSFHeap : public ycAllocator
{
public:

	ycTLSFHeap( const char* debugName );

	/*! Initialize with a backing ycAllocator
	- `backingAllocSize` is how much to request from the backing allocator when the tlsf heap is exhausted
	- Allocations larger than `maxAllocSize` will bypass the tlsf heap and go to the backing allocator
	Note: Due to overhead, maxAllocSize should be smaller than backingAllocSize
	*/
	void Init( ycAllocator* parent, const ycSize_t maxAllocSize, const ycSize_t backingAllocSize );

	/*! Initialize with virtual memory
	- `virtualReserve` is the maximum size the allocator can grow to
	- `commitChunk` is how much of the reserve space to commit when the heap is exhausted
	- Allocations larger than `maxAllocSize` will bypass the tlsf heap and go to the system allocator
	Note: While this can handle large allocations, for fragmentation it is probably better to let them
	go to the system allocator.
	*/
	void Init( const ycSize_t virtualReserve, const ycSize_t commitChunk, const ycSize_t maxAllocSize );

	virtual ~ycTLSFHeap();

	YC_ALLOCATOR_DECLARATIONS

protected:

	/*
	The block header in the TLSF white paper is unnecessarily small for our use
	case. It allows for a minimum alignment of 8 (or even 4), but 16 (SIMD) or
	64 (cache line) are probably better.

	Cache-line alignment in particular avoids a lot of edge case performance
	issues, eg the first 64 bytes of the allocation will be acquired in one load
	instead of wasting part of a load on alloc header data. For multi-core
	processing, it also alleviates the risk of false-sharing between core
	complexes, two physically separate cores accessing the same cache line at
	the same time.

	We can of course use larger minimum alignments without making this header
	bigger, but the original implementation goes through pains to allow for
	such a tiny header. So we're gonna make the header bigger, and simplify
	some of the code. We could also use some of this space for extra features.
	*/

	struct Block
	{
		Block*   prevPhys; // checked to merge a newly-free'd block
		Block*   nextFree; // only valid if kFlag_BlockFree
		Block*   prevFree; // only valid if kFlag_BlockFree
		union
		{
			struct
			{
				Block*   next;
				ycSize_t size;
				ycSize_t commit;
			} region;      // only valid if kFlag_Region
			struct
			{
				Block* next;
				Block* prev;
				void*  base;
			} external;    // only valid if kFlag_External
		};
		ureg     flags;
		ycSize_t size;     // stores the _usable_ size of the block, so it doesn't include this header!
	};

	enum : ycSize_t
	{
		kFlag_BlockFree = 1<<0,
		kFlag_Region    = 1<<1,
		kFlag_External  = 1<<2,
	};

	ycAllocator* m_parent;

	// configurable
	u8 m_flMaxIndex; // not accessed outside of init
	u8 m_flIndexShift;
	u8 m_flIndexCount;
	u8 _pad0;
	//bool m_largePages; // if using vmem
	u32 m_smallBlockSize; // cant really be >u16

	ycSize_t m_maxAllocSize;
	union
	{
		ycSize_t m_backingAllocSize; // how much to request from m_parent (when using a backing ycAllocator)
		ycSize_t m_commitChunk; // how much to commit at a time when using vmem
	};

	ureg m_flBitmap; // first level
	// does it make more sense to struct the sl bitmap ureg + free lists together? likely to go from bitmap -> associated free list
	ureg* m_slBitmap; // second level
	Block** m_free; // free lists

	void InitBitmaps();
	void* ExternalAlloc( const ycSize_t bytes, const ycSize_t align, ycAllocInfo debugName, const char* file, const uint32_t line, const ureg flags );

	static ureg GetBitIndex( const ureg x );
	ureg CalcFl( const ycSize_t size );
	void CalcFlSl( const ycSize_t size, ureg* pFl, ureg* pSl ); //!< calc fl/sl that contain size (and potentially smaller sizes too)

	Block* FindFree( ycSize_t size ); // locate a free block and mark it used
	void PrepareBlock( Block* block, const ycSize_t size );
	void* PrepareBlockAligned( Block** ppBlock, const ycSize_t size, const ycSize_t align );
	Block* SplitFront( Block* block, const ycSize_t size ); // split a block off the beginning and mark it free
	void SplitBack( Block* block, const ycSize_t size ); // split a block off the end and mark it free; the passed size _includes_ the new block's header (so `block` loses exactly `size` bytes)
	void AddRegion( u8* base, const ycSize_t size, const ycSize_t commit );
	bool ExpandRegion( const ycSize_t allocSize );
	void AddToFreeList( Block* block ); // add a block to the free lists
	Block* RemoveFreeBlock( const ureg fl, const ureg sl ); // take the block from the head of the free list and mark it used
	void RemoveFreeBlock( Block* block ); // remove a block from the free lists (may or may not be the head) (but dont actually mark it used)

	Block** GetFreeList( const ureg fl, const ureg sl );

	static ycSize_t BlockSize( Block* block );
	static bool BlockIsFree( Block* block );
	static void BlockSetFree( Block* block );
	static Block* BlockNextPhys( Block* block );
	static void* BlockGetPtr( Block* block );
	static Block* BlockFromPtr( void* ptr );
	static Block* GetRegionTail( Block* region );

	void Check();

	enum : ycSize_t
	{
		kGranularityLog2 = 6,
		kGranularity = 1 << kGranularityLog2,
		kMinBlockSize = sizeof(Block) + kGranularity, // block header + alloc of min size
		kSlIndexCountLog2 = 5,
		kSlIndexCount = 1<<kSlIndexCountLog2,
	};
	static_assert( sizeof(Block) == kGranularity, "" );

	Block* m_regionHead;
	Block* m_externalHead;
#ifdef YC_TLSFHEAP_DEBUG
	ureg m_blockCount;
	ureg m_totalBytes;
	ureg m_externalCount;
public:
	static void Test();
#endif // YC_TLSFHEAP_DEBUG
#ifdef YC_NDA
#endif 
};

#include "ycMutex.h"

class ycTLSFHeapTS : public ycAllocatorTS
{
public:

	ycTLSFHeapTS( const char* debugName );

	void Init( const ycSize_t virtualReserve, const ycSize_t commitChunk, const ycSize_t maxAllocSize );
	void Init( ycAllocator* parent, const ycSize_t maxAllocSize, const ycSize_t backingAllocSize );

	YC_ALLOCATOR_DECLARATIONS

protected:

	ycTLSFHeap m_heap;
	ycMutex m_guard;
};
