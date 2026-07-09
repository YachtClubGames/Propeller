#pragma once

#include "ycCommon.h"
#include "ycPlatformConfig.h"
#include "ycVirtualMemoryPool.h"

/* \class ycTLSFGpuHeap
A heap based on the "Two Level Segregated Fit" algorithm:
http://www.gii.upv.es/tlsf/main/docs.html
With some tweaks for GPU memory, since Block metadata is not stored in
the allocated memory.

This version is for memory that cannot be modified by the CPU, so metadata
is not stored in the allocated memory. To keep things quick, this uses a
different interface than ycAllocator; since the "pointers" cannot store any
additional information, the caller keeps a little extra context.
*/

//#define YC_TLFSGPUHEAP_DEBUG
#ifdef YC_TLFSGPUHEAP_DEBUG
	#include "ycVector.h"
#endif

class ycTLSFGpuHeap
{
public:

	struct Block;

	ycTLSFGpuHeap();
	~ycTLSFGpuHeap();

	/*
	There are two ways to operate the heap:
	- Provide a grow function
	  The grow function will be called when the heap is exhausted, and it should
	  call AddRegion
	- Manually specify free ranges
	  Provide at least one region before allocating
	*/

	typedef void( *GrowFunc )( ycTLSFGpuHeap* heap, const ycSize_t allocSize );
	void Init( GrowFunc growFunc );

	// userData will propagate to split blocks, memory in this range will always have this userData
	void AddRegion( const ycSize_t base, const ycSize_t size, void* userData = nullptr );

	/*
	When multiple regions are used, blocks between them will never be merged, even
	if the regions are contiguous. An allocation cannot span multiple ranges.
	Multiple regions can overlap 'offset space', eg you could AddRegion(0,1024)
	multiple times, and it works correctly. It will hand out the same region twice,
	and it will not merge or confuse them.
	*/

	struct AllocDesc
	{
		ureg offset;
		Block* block;
	};
	
	AllocDesc Alloc( const ycSize_t size );
	AllocDesc Alloc( const ycSize_t size, const ycSize_t align );

	void Free( const AllocDesc alloc );
	void Free( Block* block ); //!< can free with Block* directly, don't need to keep the whole AllocDesc if you don't care about it

	static void* GetUserData( Block* block );

	struct Block
	{
		/*
		This is a little large, but i dont think its worth compacting unless it
		could go all the way down to 32b (half cache line).
		I think you could get it down that far, but you'd have to bitfield a bunch
		of stuff (replace Block ptrs with indices, and 16 bits is not quite enough).
		In practice, you aren't gonna be accessing a bunch of linearly-allocated
		Blocks, so the chance of reducing cache misses is very very small, and the
		memory usage alone is not significant.
		*/
		Block*   prevPhys;
		Block*   nextPhys;
		ycSize_t offset;
		ycSize_t size;
		ureg     flags; // ref impl stores flags in bottom bits of size, but we have extra space
		Block*   prevFree;
		Block*   nextFree;
		void*    userData;
	};

protected:

	static Block kNullBlock;

	/*
	It would be nice if some of this was runtime configurable, but I don't
	think it's very important for the GPU version, since we don't have a bunch
	of different heaps.
	*/
	enum : ureg
	{
		// configurable
		kSlIndexCountLog2 = 4,  //!< log2 of second level subdivisions, values of 4-5 are common
		kFlMaxIndex       = 32, //!< controls max alloc size (1<<maxIndex), increases number of look up tables
#ifdef YC_RENDERER_DX12
		kGranularityLog2 = 12, // dx12 minimum alignment is big -- even using all the tricks to sometimes lower it!
#else
		kGranularityLog2  = 3,  //!< size/align rounded to kGranularity, fl/sl do not need to care about these low bits
#endif

		// derived
		kGranularity    = 1 << kGranularityLog2,
		kMaxAllocSize   = (1ull<<kFlMaxIndex) - 1,
		kSlIndexCount   = 1 << kSlIndexCountLog2,
		kFlIndexShift   = kSlIndexCountLog2 + kGranularityLog2, //!< how many bits until the fl index, cuz the lower bits are a mix of ignored (kGranularity) and sl
		kFlIndexCount   = kFlMaxIndex - kFlIndexShift + 1, //!< how many first level indices are left after taking kSmallBlockSize into account
		kSmallBlockSize = 1 << kFlIndexShift, //!< blocks smaller than this are in fl=0, the fl is not indicated by the high bit, but by the absence of one
	};

	enum : ycSize_t
	{
		/*
		The reference implementation of the TLFS algorithm has an additional
		"previous block is free" flag, but it is not necessary in the GPU iteration,
		since block metadata is not stored inside the allocated memory.
		*/
		kFlag_BlockFree = 1<<0,
	};

	/*
	TODO HACK FIXME: cache align
	TODO HACK FIXME: is 64 bit worth it? these bitmaps are so shallow they dont need it
	*/
	ureg m_flBitmap; // first level
	// does it make more sense to struct the sl bitmap ureg + free lists together? likely to go from bitmap -> associated free list
	ureg m_slBitmap[ kFlIndexCount ]; // second level
	Block* m_free[ kFlIndexCount ][ kSlIndexCount ]; // free lists

	GrowFunc m_growFunc;
	ycVirtualMemoryPool m_blockMem;

	Block* AllocBlock();
	void FreeBlock( Block* block );

	static ureg GetBitIndex( const ureg x );
	static ureg CalcFl( const ycSize_t size );
	static void CalcFlSl( const ycSize_t size, ureg* pFl, ureg* pSl ); //!< calc fl/sl that contain size (and potentially smaller sizes too)

	Block* FindFree( ycSize_t size ); // locate a free block and mark it used
	void PrepareBlock( Block* block, const ycSize_t size );
	ycSize_t PrepareBlockAligned( Block* block, const ycSize_t size, const ycSize_t align );
	void SplitFront( Block* block, const ycSize_t size ); // split a block off the beginning and mark it free
	void SplitBack( Block* block, const ycSize_t size ); // split a block off the end and mark it free
	void AddToFreeList( Block* block ); // add a block to the free lists (but do not modify any flags)
	void RemoveFreeBlock( Block* block, const ureg fl, const ureg sl ); // remove a block from the free lists (but dont actually mark it used)
	void RemoveFreeBlock( Block* block ); // convenience if you dont have fl/sl handy

	/* TODO HACK FIXME:
	- RemoveFreeBlock permutations could be more different, the fl/sl one doesnt need to take an arbitrary block, it can assume head
	- check how much we really need kNullBlock, im not sure the branch saves are worth it
	*/

	static bool BlockIsFree( Block* block );
	static void BlockSetFree( Block* block );
	static void BlockSetUsed( Block* block );

	void UnlinkPhys( Block* block );

	void Check();
#ifdef YC_TLSFGPUHEAP_DEBUG
	ycVector< Block* > m_physHead;
	ureg m_blockCount;
	ureg m_totalBytes;
public:
	static void Test();
#endif // YC_TLSFGPUHEAP_DEBUG
};
