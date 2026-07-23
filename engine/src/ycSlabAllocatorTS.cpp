#include "ycSlabAllocatorTS.h"

ycSlabAllocatorTS::ycSlabAllocatorTS( const char *debugName )
	: ycAllocatorTS( debugName )
	, m_guard( debugName )
	, m_mem( debugName )
{
}

void ycSlabAllocatorTS::Init( const char* debugName, const ycSize_t blockSize, ycAllocator* mem, const ycSize_t pageSize, const ycSize_t blockAlign  )
{
	m_mem.Init( debugName, blockSize, mem, pageSize, blockAlign );
}

ycSlabAllocatorTS::ycSlabAllocatorTS( const char* debugName, const ycSize_t blockSize, ycAllocator* mem, const ycSize_t pageSize, const ycSize_t blockAlign )
	: ycAllocatorTS( debugName )
	, m_guard( debugName )
	, m_mem( debugName, blockSize, mem, pageSize, blockAlign )
{
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycSlabAllocatorTS )
{
	m_guard.Lock();
	void* ptr = m_mem.Alloc( bytes, align, debugName, file, line, flags );
	m_guard.Unlock();
	return ptr;
}

YC_ALLOCATOR_DECLARATION_FREE( ycSlabAllocatorTS )
{
	m_guard.Lock();
	m_mem.Free( ptr, flags );
	m_guard.Unlock();
}

YC_ALLOCATOR_DECLARATION_SIZE( ycSlabAllocatorTS )
{
	YC_UNUSED( align );
	m_guard.Lock();
	const ycSize_t size = m_mem.Size( ptr );
	m_guard.Unlock();
	return size;
}

void ycSlabAllocatorTS::Clear()
{
	m_guard.Lock();
	m_mem.Clear();
	m_guard.Unlock();
}
