#pragma once

#include "ycAllocator.h"

class ycBumpAllocator : public ycAllocator
{
public:

	/*
	Unlike most ycAllocators, ycBumpAllocator does not proxy to a parent allocator. It relies on virtual memory,
	so it must utilize OS calls directly and cannot easily manage the underlying memory through a ycAllocator
	interface. It could be sensible to specify a parent ycSysAllocator, but ycSysAllocator is effectively a
	global/singleton anyway.
	Page size is platform dependent, large pages are probably 2-4MB and small pages are probably 4-64KB.
	*/

	static constexpr ycSize_t kDefaultMaxSize = 16 * 1024*1024; // low default so large amounts must be explicit

	ycBumpAllocator( const char* debugName );
	ycBumpAllocator( const char* debugName, const bool largePages = false, const ycSize_t maxSize = kDefaultMaxSize );
	void Init( const bool largePages = false, const ycSize_t maxSize = kDefaultMaxSize );
	virtual ~ycBumpAllocator();
	void Clear(); // allocator must be re-initialized before being used again

	YC_ALLOCATOR_DECLARATIONS

protected:

	u8* m_base = nullptr;
	ycSize_t m_offset = 0;
	ycSize_t m_committed = 0;
	ycSize_t m_maxSize = 0;
	ycSize_t m_pageSize;

	#ifdef YC_NDA
	#endif 
};
