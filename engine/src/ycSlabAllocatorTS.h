#pragma once

/*! \class ycSlabAllocatorTS
A thread safe version of ycSlabAllocator
*/

// TODO HACK FIXME: block alignment

#include "ycSlabAllocator.h"

#include "ycMutex.h"

class ycSlabAllocatorTS : public ycAllocatorTS
{
public:

	ycSlabAllocatorTS( const char* debugName );
	void Init( const char* debugName, const ycSize_t blockSize, ycAllocator* mem = nullptr, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN ); // TODO HACK FIXME: no default nullptr allocators, if you want nullptr you must pass it explictely
	ycSlabAllocatorTS( const char* debugName, const ycSize_t blockSize, ycAllocator* mem = nullptr, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE, const ycSize_t blockAlign = YC_MEM_DEFAULT_ALIGN ); // TODO HACK FIXME: no default nullptr allocators, if you want nullptr you must pass it explictely

	YC_ALLOCATOR_DECLARATIONS

	void Clear();

protected:
	
	ycMutex m_guard;
	ycSlabAllocator m_mem;
};
