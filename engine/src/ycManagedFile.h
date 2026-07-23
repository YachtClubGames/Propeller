#pragma once
#ifndef YC_MANAGEDFILE_H
#define YC_MANAGEDFILE_H

#include "ycFileManager.h"

class ycManagedPak;

class ycManagedFile
{
public:

	inline ycManagedFile( const ycFileManager::RefCountState& initialState )
		: m_data( nullptr )
		, m_refCountState( initialState.AsU32() )
		, m_mem( nullptr )
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		, m_newer( nullptr )
		, m_older( nullptr )
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		, m_pak( nullptr )
		, m_nextInPak( nullptr )
	{}

	ycFileManager::FileType GetFileType() const { return ycFileManager::FileType(m_hashKey>>62); }

	const u8*      m_data;
	ycSize_t       m_size;
	ycAtomic       m_refCountState;
	ycString       m_path; // for pak'd files, could ref into the pak's copy of the filename?
	u64            m_hashKey;
	ycAllocatorTS* m_mem; // allocator to use to free m_data
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		ycManagedFile* m_newer; // protected by the table lock
		ycManagedFile* m_older;
		inline ycManagedFile* GetNewest()
		{
			ycManagedFile* ver = this;
			while( ver->m_newer ) { ver = ver->m_newer; }
			return ver;
		}
		inline ycManagedFile* GetOldest()
		{
			ycManagedFile* ver = this;
			while( ver->m_older ) { ver = ver->m_older; }
			return ver;
		}
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	// pak'd files
	ycManagedPak*  m_pak;
	ycManagedFile* m_nextInPak; // set during pak LOADING state, not modified after (may not point to the latest version of data); newer versions of data only have m_pak, not m_nextInPak!

	inline ycFileManager::RefCountState GetRefCountState() const { return ycFileManager::RefCountState( m_refCountState ); }
	inline ureg GetState() const { return GetRefCountState().state; }
	inline bool SwapState( const ycFileManager::RefCountState prev, const ycFileManager::RefCountState next ) { return m_refCountState.CompareAndSwap( prev.AsU32(), next.AsU32() ); }
};

#endif // !YC_MANAGEDFILE_H
