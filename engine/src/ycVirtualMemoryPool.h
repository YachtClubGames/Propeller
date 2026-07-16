#pragma once

/*! \class ycVirtualMemoryPool
A very simple virtual memory allocator. Is it not very fast, but for debugging
or low traffic situations, simplicity may be desirable.
*/

#include "ycAllocator.h"

class ycVirtualMemoryPool : public ycAllocator
{
public:

	static constexpr ycSize_t kDefaultCommitIncrement = 64 * 1024;
	static constexpr ycSize_t kDefaultMaxSize = 16 * 1024*1024; // low default so large amounts must be explicit

	ycVirtualMemoryPool( const char* debugName );
	ycVirtualMemoryPool( const char* debugName, const ycSize_t blockSize, const ycSize_t commitIncrement = kDefaultCommitIncrement, const ycSize_t maxBytes = kDefaultMaxSize );
	void Init( const ycSize_t blockSize, const ycSize_t commitIncrement = kDefaultCommitIncrement, const ycSize_t maxBytes = kDefaultMaxSize );
	virtual ~ycVirtualMemoryPool();
	void Clear();

	YC_ALLOCATOR_DECLARATIONS

protected:

	ycSize_t m_blockSize = 0;
	ycSize_t m_commitIncrement = 0;
	u8* m_base = nullptr;
	ycSize_t m_committed = 0;
	ycSize_t m_maxSize = 0;

	struct FreeNode
	{
		FreeNode* next;
	};
	FreeNode* m_freeHead = nullptr;

	void AddToFreeList( u8* base, const ycSize_t count );

#ifdef YC_NDA
#endif 
};
