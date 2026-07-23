#include "ycMemPoolTS.h"

ycMemPoolTS::ycMemPoolTS( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, ycAllocator* mem, const ycSize_t blockAlign /*= YC_MEM_DEFAULT_ALIGN*/ )
	: ycAllocatorTS( debugName )
	, m_guard( debugName )
	, m_mem( debugName, blockSize, numBlocks, mem, blockAlign )
{
}

ycMemPoolTS::ycMemPoolTS( const char* debugName, const ycSize_t blockSize, const ycSize_t numBlocks, uint8_t* mem, const ycSize_t blockAlign /*= YC_MEM_DEFAULT_ALIGN*/ )
	: ycAllocatorTS( debugName )
	, m_guard( debugName )
	, m_mem( debugName, blockSize, numBlocks, mem, blockAlign )
{
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycMemPoolTS )
{
	m_guard.Lock();
	void* mem = m_mem.Alloc( bytes, align, debugName, file, line, flags );
	m_guard.Unlock();
	return mem;
}

YC_ALLOCATOR_DECLARATION_FREE( ycMemPoolTS )
{
	m_guard.Lock();
	m_mem.Free( ptr, flags );
	m_guard.Unlock();
}

YC_ALLOCATOR_DECLARATION_SIZE( ycMemPoolTS )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	return m_mem.m_blockSize;
}


void ycMemPoolTS::Clear()
{
	m_guard.Lock();
	m_mem.Clear();
	m_guard.Unlock();
}
