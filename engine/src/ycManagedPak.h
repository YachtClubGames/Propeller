#pragma once

#include "ycFileManager.h"

class ycManagedFile;

class ycManagedPak
{
public:

	inline ycManagedPak( const ycFileManager::RefCountState& initialState )
		: m_firstFile( nullptr )
		, m_loadingCount( 0 )
		, m_data( nullptr )
		, m_mem( nullptr )
		, m_refCountState( initialState.AsU32() )
	{}

	ycHashKey_t m_hashKey;
	ycString m_name;
	ycManagedFile* m_firstFile;
	ycAtomic m_loadingCount; // if post-processing is being done externally, whoever decrements this to zero is responsible for transitioning the pak from LOADING to LOADED
	const u8* m_data;
	ycAllocatorTS* m_mem; // allocator to use to free m_data when m_refCount reaches zero

	ycAtomic m_refCountState;

#ifdef YC_FILEMANAGER_TRACK_PAKS
	u64 m_loadTime = 0;
#endif

	inline ycFileManager::RefCountState GetRefCountState() const { return ycFileManager::RefCountState( m_refCountState ); }
	inline ureg GetState() const { return GetRefCountState().state; }
	inline bool SwapState( const ycFileManager::RefCountState prev, const ycFileManager::RefCountState next ) { return m_refCountState.CompareAndSwap( prev.AsU32(), next.AsU32() ); }
};
