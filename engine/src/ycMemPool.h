#pragma once

/*! \class ycMemPool
Allocates fixed-size blocks of memory from a preallocated pool.

\sa https://en.wikipedia.org/wiki/Memory_pool

Warning: this only accepts a size, it knows nothing about types! Constructors and destructors are your responsibility!
*/

#include "ycAllocator.h"

class ycMemPool : public ycAllocator
{
public:

	//! Create a new memory pool, with the initial memory allocated from the provided allocator
	ycMemPool( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, ycAllocator* mem = nullptr, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN );

	//! Destroys a memory pool
	/*! Any allocations remaining the pool are implicitely freed. */
	~ycMemPool();

	YC_ALLOCATOR_DECLARATIONS

	//! Marks all blocks free
	// TODO HACK FIXME: reset vs clear?
	void Clear();
	
	inline ycSize_t GetBlockSize()  const { return m_blockSize; } //!< Alignment may cause this to be larger than the originally requested size
	inline ycSize_t GetBlockAlign() const { return m_blockAlign; }
	inline ycSize_t GetMaxBlocks()  const { return m_numBlocks; }

	void* GetBlock( const ycSize_t idx ) const;
	ycSize_t GetBlockIdx( const void* ptr ) const;

#ifdef YC_ENABLE_MEM_DEBUG
	virtual bool IsDumpSummarized() const override { return true; }
#endif

protected:

	friend class ycMemPoolTS;

	static const ycSize_t kInvalidFreeIdx = uintptr_t(-1LL);

	ycSize_t     m_blockSize;
	ycSize_t     m_blockAlign;
	ycSize_t     m_numBlocks;
	ycSize_t     m_firstFreeIdx;
	uint8_t*     m_blocks;
	ycAllocator* m_mem;

	struct FreeLink
	{
		ycSize_t nextFreeIdx;
	};

	ycMemPool( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, uint8_t* mem, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN );
};

/*! \class ycStaticMemPool
An ycMemPool that embeds the pool memory within the object, requiring no
dynamic allocation during construction.

Warnings: Be conscious of how large these can be when placing them inside other
objects or on the stack!
*/

template< ycSize_t t_blockSize, ycSize_t t_numBlocks, const ycSize_t t_blockAlign = YC_MEM_DEFAULT_ALIGN >
class ycStaticMemPool : public ycMemPool
{
protected:
	static const ycSize_t s_paddedBlockSize = (t_blockSize + (t_blockAlign-1)) & ~(t_blockAlign-1);
public:
	ycStaticMemPool( const char* debugName ) :
		ycMemPool( debugName, s_paddedBlockSize, t_numBlocks, m_data, t_blockAlign )
	{}
	uint8_t m_data[ s_paddedBlockSize * t_numBlocks ];
};