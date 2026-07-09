#pragma once

#include "ycAllocator.h"

/*! These provide direct access to system allocations
Our analogs for malloc/free, but you should generally be using ycAllocator* based allocation.
*/

void* ycSysAlloc( ycSize_t bytes, ycSize_t align, const char* debugName = "<UNKNOWN>", const char* file = "<UNKNOWN>", const uint32_t line = 0, const ureg flags = ycAllocator::kAllocFlag_None );
void ycSysFree( void* ptr );

/* \class ycSysAllocator
Wraps ycSysAlloc and ycSysFree in an ycAllocator interface. Multiple
instances of this class do not allocate memory any differently, but it can
still be useful to create them, they will ycMemTrack separately.
*/

class ycSysAllocator : public ycAllocatorTS
{
public:

	enum
	{
		kSysAllocFlag_VRAM = 1<<16 //!< silently ignored on platforms that do not implement it
	};

	static ycSysAllocator* GetDefault(); //!< A convenience function to get a global system allocator

	YC_ALLOCATOR_DECLARATIONS

	ycSysAllocator( const char *debugName ) : ycAllocatorTS( debugName ) {}
};
