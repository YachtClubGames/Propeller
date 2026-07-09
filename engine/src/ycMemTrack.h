#pragma once

#include "ycAllocator.h"

// allocator get allocation callback, return false to stop further calls
typedef bool( *ycAllocIteratorCB )( ycAllocator* alloc, void *ptr, ycSize_t size );

#ifdef YC_ENABLE_MEM_DEBUG

namespace ycMemTrack
{
	void Init();
	void Shutdown();
	void Update();
	void TrackAlloc( ycAllocator* allocator, void* memory, ycSize_t size, ycAllocInfo name, const char* file, u32 line );
	void TrackFree( ycAllocator* allocator, void* memory );
	void ClearAllocator( ycAllocator* allocator );
	void OutputToFile( ycAllocator* allocator = nullptr, const char* fileName = nullptr );
	void Output( ycAllocator* allocator = nullptr );
	void IncrementCurrentMarker( const char* reason );
	void UI();
}

#else

class ycMemoryMap;

namespace ycMemTrack
{
	inline void Init() {}
	inline void Shutdown() {}
	inline void Update() {}
	inline void TrackAlloc( ycAllocator*, void*, ycSize_t, ycAllocInfo, const char*, u32 ) {}
	inline void TrackFree( ycAllocator*, void* ) {}
	inline void ClearAllocator( ycAllocator* ) {}
	inline void OutputToFile( ycAllocator* = nullptr, const char* = nullptr ) {}
	inline void Output( ycAllocator* = nullptr ) {}
	inline void IncrementCurrentMarker( const char* ) {}
	inline void UI() {}
}

#endif
