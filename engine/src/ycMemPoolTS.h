#pragma once

/*! \class ycMemPoolTS
A thread safe version of ycMemPool
*/

// TODO HACK FIXME: block alignment

#include "ycMemPool.h"
#include "ycMutex.h"

class ycMemPoolTS : public ycAllocatorTS
{
public:
	
	ycMemPoolTS( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, ycAllocator* mem = nullptr, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN );

	YC_ALLOCATOR_DECLARATIONS

	void Clear();

#ifdef YC_ENABLE_MEM_DEBUG
	virtual bool IsDumpSummarized() const override { return true; }
#endif

protected:

	ycMutex m_guard;
	ycMemPool m_mem;

	ycMemPoolTS( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, uint8_t* mem, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN );
};

template< ycSize_t t_blockSize, ycSize_t t_numBlocks, const ycSize_t t_blockAlign = YC_MEM_DEFAULT_ALIGN >
class ycStaticMemPoolTS : public ycMemPoolTS
{
protected:
	static const ycSize_t s_paddedBlockSize = (t_blockSize + (t_blockAlign-1)) & ~(t_blockAlign-1);
public:
	ycStaticMemPoolTS( const char* debugName ) :
		ycMemPoolTS( debugName, s_paddedBlockSize, t_numBlocks, m_data, t_blockAlign )
	{}
	uint8_t m_data[ s_paddedBlockSize * t_numBlocks ];
};
