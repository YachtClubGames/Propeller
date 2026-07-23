#include "ycFileManager.h"

#include "ycDataSerialization.h"
#include "ycDateTime.h"
#include "ycDir.h"
#include "ycFile.h"
#include "ycFileManagerChangeNotifier.h"
#include "ycFileStreamer.h"
#include "ycManagedFile.h"
#include "ycManagedPak.h"
#include "ycMutex.h"
#include "ycOpenHash.h"
#include "ycPakFormat.h"
#include "ycProfiler.h"
#include "ycSemaphore.h"
#include "ycStackString.h"
#include "ycStringUtils.h"
#include "ycThread.h"
#include "ycVirtualMemoryPoolTS.h"

//#define YC_FILEMANAGER_RANDOM_FILE_NOTIFICATIONS
//#define YC_FILEMANAGER_RANDOM_READ_LATENCY 20
#if defined YC_FILEMANAGER_RANDOM_FILE_NOTIFICATIONS || defined YC_FILEMANAGER_RANDOM_READ_LATENCY
	#include "ycRand.h"
#endif

#if !defined YC_CODEGEN
	#define YC_FILEMANAGER_ENABLE_AUTO_FIXUP
#endif
#ifdef YC_FILEMANAGER_ENABLE_AUTO_FIXUP
	#include "ycEngine.h"
	#include "ycJobQueue.h"
#endif

#if YC_NDA
#endif
//#include <stdio.h>

namespace
{
	#if defined YC_EDITOR
		const ycStringRef kDataPath = "build/pc/data/";
	#elif YC_NDA
	#elif YC_NDA
	#else
		const ycStringRef kDataPath = "data/";
	#endif

	const ycStringRef kDataAlias = "/data/"; // TODO HACK FIXME: remove???
	const ycStringRef kResourcePath = "resources/";
	const ycStringRef kEngineResourcePath = "engine/resources/";

	ycThread::Handle s_thread = ycThread::kInvalidThread;

	/* The table guard protects two things:
	* Modifications to the file table.
	* Access to files/paks while they are being removed from the table.
	  For paks and files this has slightly different meanings. When standalone
	  files reach a refcount of zero they are immediately removed from the table,
	  so they can never be observed externally in the NONE or UNLOADING states.
	  Paks remain in the table during their UNLOADING phase, so they can only
	  never be observed externally in the NONE state.
	  The difference is due to the more complex nature of pak unloading, holding
	  the mutex through their entire unloading process could introduce stalls into
	  other threads, so they have an additional transitional phase. If you get a
	  reference to a pak in the UNLOADING state, you can assume it will eventually
	  re-enter LOADING (and subsequently LOADED).
	*/

	ycAllocatorTS* s_fileMem;
	ycMutex s_tableGuard( "ycFileManager table" );
	ycOpenHash< u64, ycManagedFile* > s_fileTable;
	ycOpenHash< u64, ycManagedPak* > s_pakTable;
	ycVirtualMemoryPoolTS s_managedFileMem( "ycFileManager managed files", ycRoundUpPow2( ycSize_t(sizeof(ycManagedFile)), ycSize_t(YC_MEM_DEFAULT_ALIGN) ), 2*1024*1024 );
	ycVirtualMemoryPoolTS s_managedPakMem( "ycFileManager managed paks", ycRoundUpPow2( ycSize_t(sizeof(ycManagedPak)), ycSize_t(YC_MEM_DEFAULT_ALIGN) ) );

	enum
	{
		kCmdType_LoadFile = 1,
		kCmdType_LoadPak,
		//kCmdType_UnloadFile, // could optionally do the unload work in the file thread
		kCmdType_UnloadPak,

		kCmdType_StreamingInit,  // ycFileReader::Open
		kCmdType_StreamingRead,  // ycFileReader::Read
		kCmdType_StreamingClose, // ycFileReader::Close, and also free the mem!
		#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
			kCmdType_StreamingReinit, // close + reopen (and potentially cancel outdated read requests)
			kCmdType_Pause,
		#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
	};
	struct Cmd
	{
		Cmd* next;
		u32 cmdType;
		u32 _pad;
		u64 fileLoadFlags;
		union
		{
			void* arg;
			ycManagedFile* managedFile; // kCmdType_LoadFile
			ycManagedPak* managedPak; // kCmdType_LoadPak
			ycFileStreamer* streamer; // kCmdType_StreamingInit, kCmdType_StreamingRead, kCmdType_StreamingReinit
			ycFileManager::StreamingRequest* request; // kCmdType_StreamingRead
		};
	};

	ycMutex s_queueGuard( "ycFileManager queue" );
	ycSemaphore s_queueSignal( "ycFileManager queue" );
	ycVirtualMemoryPoolTS s_queueMem( "ycFileManager queue", sizeof(Cmd) );
	Cmd* s_queueHead = nullptr;
	Cmd* s_queueTail = nullptr;

	Cmd* AllocCmd()
	{
		return (Cmd*)YC_ALLOC( &s_queueMem, sizeof(Cmd), alignof(Cmd), "ycFileManager::Cmd" );
	}
	void PushCmd( Cmd* cmd, const bool signal = true )
	{
		s_queueGuard.Lock();
		cmd->next = nullptr;
		if( s_queueTail )
		{
			s_queueTail->next = cmd;
			s_queueTail = cmd;
		}
		else
		{
			s_queueHead = s_queueTail = cmd;
		}
		s_queueGuard.Unlock();
		if( signal )
		{
			s_queueSignal.Signal();
		}
	}
	void PushCmd( const u32 cmdType, void* arg, const u64 fileLoadFlags = 0 )
	{
		Cmd* cmd = AllocCmd();
		cmd->cmdType = cmdType;
		cmd->arg = arg;
		cmd->fileLoadFlags = fileLoadFlags;
		PushCmd( cmd, true );
	}
	Cmd* TakeCmd()
	{
		s_queueGuard.Lock();
		ycAssert( s_queueHead );
		Cmd* cmd = s_queueHead;
		s_queueHead = s_queueHead->next;
		if( s_queueHead == nullptr )
		{
			s_queueTail = nullptr;
		}
		s_queueGuard.Unlock();
		return cmd;
	}
	void FreeCmd( Cmd* cmd )
	{
		YC_FREE( &s_queueMem, cmd );
	}

	// Streamers
	ycVirtualMemoryPoolTS            s_fileStreamerMem( "ycFileManager streamers", ycRoundUpPow2( ycSize_t(sizeof(ycFileStreamer)), ycSize_t(YC_MEM_DEFAULT_ALIGN) ) );
	ycVirtualMemoryPoolTS            s_streamRequests( "ycFileManager stream requests", ycRoundUpPow2( ycSize_t(sizeof(ycFileManager::StreamingRequest)), ycSize_t(YC_MEM_DEFAULT_ALIGN) ) );
	ycMutex                          s_streamRequestListGuard( "ycFileManager stream requests" );
	ycFileManager::StreamingRequest* s_streamRequestHead = nullptr;

	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		// support for pausing/resuming streamers
		ycMutex         s_fileStreamListGuard( "ycFileManager stream list" );
		ycFileStreamer* s_fileStreamerHead = nullptr;
		ureg            s_numFileStreamers = 0;
		void ValidateStreamerList();
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	// File Change Notifications
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		typedef ycOpenHash< ycHashKey_t, ycFileManagerChangeNotifier* > NotifierList;
		ycMutex                 s_notifierGuard( "ycFileManager notifiers" );
		NotifierList            s_notifiers;
		ycMutex                 s_pendingNotificationGuard( "ycFileManager pending notifications" );
		ycVector< ycHashKey_t > s_pendingNotifications;
		bool                    s_processPendingNotifications = true;

		// Pause/Resume
		struct PauseCtx
		{
			PauseCtx() : pausedSignal( "ycFileManager paused" ), resumeSignal( "ycFileManager resumed" ) {}
			ycSemaphore pausedSignal;
			ycSemaphore resumeSignal;
		};
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	#ifdef YC_FILEMANAGER_ENABLE_MODAPI
		ycFileManager::ModProvider s_modProvider = nullptr;
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
}

void PostProcessFileJob( void* userData )
{
	ycManagedFile* managedFile = (ycManagedFile*)userData;
	ycFileManager::PostProcessFile( managedFile );
}

/*static*/ bool ycFileManager::Exists( const ycStringRef& path, const FileType type )
{
	ycStackString< YC_MAX_PATH > fullPath;
	Prefix( &fullPath, path, type );
	return ycFile::Exists( fullPath );
}

/*static*/ bool ycFileManager::Exists( const ycStringRef& path, const bool checkWildCard, const u32 dirRules )
{
	if( ycFile::Exists( path ) )
	{
		return true;
	}

	if( checkWildCard && path.Contains( "*" ) )
	{
		ycStringRef str( path );
		s32 last = str.FindLast( "/" );
		if( last >= 0 )
		{
			str = str.Left( last );
			ycStackString<> dirName( str );
			dirName.Append( "/" );
			dirName.NullTerminate();
			ycDir dir( dirName.Get() );
			ycVector< ycDir::FileDesc > files( nullptr );
			dir.ReadDir( &files, dirRules );
			for( ycSize_t fileIdx = 0; fileIdx != files.Length(); ++fileIdx )
			{
				if( files[fileIdx].path.FindWildcard( path ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

/*static*/ void ycFileManager::PrefixForWrite( ycString * dst, const ycStringRef & basePath, const ycFileManager::FileType type, bool engineResources )
{
	ycAssert( !basePath.BeginsWith( "/data/" ) );

	switch( type )
	{
	case kFile_Data:
		dst->Set( kDataPath );
		break;

	case kFile_Resource:
		dst->Set( engineResources ? kEngineResourcePath : kResourcePath );
		break;

	case kFile_Relative:
		dst->Clear();
		break;
	}

	dst->Append( basePath );
}

/*static*/ ycStringRef ycFileManager::GetBasePath( ycStringRef path )
{
	if( path.Take( kDataPath ) ) { /*nop*/ }
	else if( path.Take( kResourcePath ) ) { /*nop*/ }
	else if( path.Take( kEngineResourcePath ) ) { /*nop*/ }
	return path;
}

/*static*/ bool ycFileManager::GetValidPath( ycString* path, bool wildcard, u32 dirRules )
{
	path->Remove( ycDir::GetCurrent() );
	if( ycFileManager::Exists( path->GetRef(), wildcard, dirRules ) )
	{
		ycDir::CleanupPath( path );
		path->SetLength( ycStringUtils::Length( path->GetPtr() ) );
		path->RemovePrefix( ycDir::GetCurrent() );
		path->RemovePrefix( "/" );
		path->NullTerminate();

		path->Set( GetBasePath( path->GetRef() ) );
		return true;
	}
	return false;
}

/*static*/ u64 ycFileManager::CalcHashKey( const ycStringRef& path, const FileType fileType )
{
	ycAssert( !path.BeginsWith( kDataPath ) );
	ycAssert( !path.BeginsWith( kResourcePath ) );
	ycAssert( !path.BeginsWith( kEngineResourcePath ) );
	ycAssert( fileType != kFile_Relative );
	return ( u64(fileType) << 62 ) | ( path.Hash64() & 0x3fffffffffffffffllu );
}

/*static*/ void ycFileManager::Prefix( ycString* dst, const ycStringRef& basePath, const ycFileManager::FileType type, bool wildcard, u32 dirRules )
{
	// TODO(rthomas) should just remove this and assert on "/data/" paths now, also remove assert below
	ycAssert( !basePath.BeginsWith( "/data/" ) || type == kFile_Relative );
	
	switch( type )
	{
	case kFile_Relative:
		if( basePath.BeginsWith( kDataAlias ) )
		{
			dst->Set( kDataPath );
			dst->Append( basePath.Right( basePath.Length() - kDataAlias.Length() ) );
		}
		else
		{
			dst->Set( basePath);
		}
		break;

	case kFile_Data:
		dst->Set( kDataPath );
		dst->Append( basePath );
		break;

	case kFile_Resource:
		dst->Set( kResourcePath );
		dst->Append( basePath );
		if( !ycFileManager::Exists( dst->GetRef(), wildcard, dirRules ) )
		{
			dst->Set( kEngineResourcePath );
			dst->Append( basePath );
		}
		break;
	}
}

/*static*/ void ycFileManager::Start( ycAllocatorTS* /*allocator*/ )
{
	s_fileMem = g_defaultMem;
	s_fileTable.Init( g_defaultMem );
	s_pakTable.Init( g_defaultMem );

	s_streamRequestHead = nullptr;
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		s_fileStreamerHead = nullptr;
		s_numFileStreamers = 0;
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		s_notifiers.Init( g_defaultMem );
		s_notifiers.Reserve( 1024 * 2 );
		s_pendingNotifications.Init( g_defaultMem, 1024 );
	#endif

	ycAssert( s_thread == ycThread::kInvalidThread );
	ureg coreIdx = 0;
	#if YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#endif
	s_thread = ycThread::Start( "ycFileManager", WorkerMain, nullptr, coreIdx, 64*1024 );

	//PushCmd( kCmdType_Populate, nullptr );
}

/*static*/ void ycFileManager::Stop()
{
	ycAssert( s_thread != ycThread::kInvalidThread );
}

/*static*/ void ycFileManager::WorkerMain( void* )
{
	for(;;)
	{
		s_queueSignal.Wait();
		Cmd* cmd = TakeCmd();
		switch( cmd->cmdType )
		{
		//case kCmdType_Populate: Cmd_Populate(); break;
		case kCmdType_LoadFile:Cmd_LoadFile( cmd->managedFile, cmd->fileLoadFlags ); break;
		case kCmdType_LoadPak: Cmd_LoadPak( cmd->managedPak );  break;
		case kCmdType_UnloadPak: Cmd_UnloadPak( cmd->managedPak );  break;
		case kCmdType_StreamingInit: Cmd_StreamingInit( cmd->streamer ); break;
		case kCmdType_StreamingRead: Cmd_StreamingRead( cmd->request ); break;
		case kCmdType_StreamingClose: Cmd_StreamingClose( cmd->streamer ); break;
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		case kCmdType_StreamingReinit: Cmd_StreamingReinit( cmd->streamer ); break;
		case kCmdType_Pause: Cmd_Pause( cmd->arg ); break;
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		}
		FreeCmd( cmd );
	}
}

/*static*/ void ycFileManager::Cmd_LoadFile( ycManagedFile* managedFile, const u64 fileLoadFlags )
{
	//ycLog( "LoadFile %s\n", managedFile->m_path.Get() );
	#ifdef YC_FILEMANAGER_ENABLE_RESOURCES_ACCESS
		ycStackString<> filePath;
		if( managedFile->GetFileType() == kFile_Data )
		{
			filePath.Set( kDataPath );
			filePath.Append( managedFile->m_path );
		}
		else
		{
			filePath.Set( kResourcePath );
			filePath.Append( managedFile->m_path );
			if( !ycFile::Exists( filePath ) )
			{
				filePath.Set( kEngineResourcePath );
				filePath.Append( managedFile->m_path );
			}
		}
	#else
		ycAssert( managedFile->GetFileType() == kFile_Data );
		ycStackString<> filePath( kDataPath );
		filePath.Append( managedFile->m_path );
	#endif

	#ifdef YC_ENABLE_PROFILER
		const ycTime_t start = ycDateTime::GetTimeHiRes();
	#endif
	#ifdef YC_FILEMANAGER_RANDOM_READ_LATENCY
		ycRand rng;
		ycThread::Sleep( rng.RangeFast( 0, YC_FILEMANAGER_RANDOM_READ_LATENCY ) );
	#endif
	ycAllocatorTS* fileMem = s_fileMem;
	ycSize_t fileSize = 0;
	const u8* fileData = nullptr;

	#ifdef YC_FILEMANAGER_ENABLE_MODAPI
		if( s_modProvider )
		{
			s_modProvider( filePath.GetRef(), &fileData, &fileSize );
			if( fileData )
			{
				fileMem = nullptr;
			}
		}
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	if( fileData == nullptr && (fileLoadFlags&kFileLoadFlag_AllowLoose) )
	{
		fileData = ycFile::Read( filePath, s_fileMem, &fileSize );
		#if defined YC_FILEMANAGER_ENABLE_LOOSE_FILES_WARNING
		if( fileData != nullptr && managedFile->m_pak == nullptr )
		{
			ycLog( "Warning: Loose file not found in pak: %s\n", filePath.Get() );
		}
		#endif // YC_FILEMANAGER_ENABLE_LOOSE_FILES_WARNING
	}
	if( fileData == nullptr )
	{
		// proceed to the NotFound state, if the file still has references
		for(;;)
		{
			const RefCountState prev = managedFile->GetRefCountState();
			ycAssert( prev.state == kFileState_Loading );
			ycAssert( prev.refCount >= 1 );
			if( prev.refCount > 1 )
			{
				const RefCountState next( prev.refCount-1, kFileState_NotFound, prev.flags );
				if( managedFile->SwapState( prev, next ) )
				{
					break;
				}
			}
			else
			{
				const RefCountState next( 0, kFileState_NotFound, prev.flags );
				s_tableGuard.Lock();
				if( managedFile->SwapState( prev, next ) )
				{
					RemoveFromTable( managedFile, false );
					s_tableGuard.Unlock();
					FreeFile( managedFile, false );
					break;
				}
				else
				{
					s_tableGuard.Unlock(); // someone modified the ref count before we swapped state
				}
			}
		}
		return;
	}
	#ifdef YC_ENABLE_PROFILER
		ycProfiler::RegisterFileRead( start, ycDateTime::GetTimeHiRes(), fileSize );
	#endif

	managedFile->m_mem = fileMem;
	managedFile->m_data = fileData;
	managedFile->m_size = fileSize;

	#ifdef YC_FILEMANAGER_ENABLE_AUTO_FIXUP
	if( fileSize >= 4 &&  ycMemEq( fileData, "YCD\0", 4 ) )
	{
		ycEngine::GetJobQueue().Queue( PostProcessFileJob, managedFile );
	}
	else
	#endif // YC_FILEMANAGER_ENABLE_AUTO_FIXUP
	{
		FinishLoad( managedFile );
	}
}

/*static*/ void ycFileManager::PostProcessFile( ycManagedFile* managedFile )
{
	#ifdef YC_FILEMANAGER_ENABLE_AUTO_FIXUP
		ycData::FixupBinary( (u8*)managedFile->m_data );
	#endif // YC_FILEMANAGER_ENABLE_AUTO_FIXUP
	FinishLoad( managedFile );
}

/*static*/ void ycFileManager::FinishLoad( ycManagedFile* managedFile )
{
	for(;;)
	{
		const RefCountState prev = managedFile->GetRefCountState();

		// whoever sent the load msg should've switched into this state, and no one else should be allowed to switch away
		ycAssert( prev.state == kFileState_Loading );

		if( (prev.flags&kFileFlag_PakLoadingCount) == 0 )
		{
			// at least one for the 'loading' reference
			ycAssert( prev.refCount >= 1 );

			// switch into the loaded state if the file still has references
			if( prev.refCount > 1 )
			{
				const RefCountState next( prev.refCount-1, kFileState_Loaded, prev.flags );
				if( managedFile->SwapState( prev, next ) )
				{
					break;
				}
			}
			else
			{
				const RefCountState next( 1, kFileState_Unloading, prev.flags );
				s_tableGuard.Lock();
				/* take the lock _before_ entering the unloading state, so that if it is entered we can be sure no one else can grab the file */
				if( managedFile->SwapState( prev, next ) )
				{
					RemoveFromTable( managedFile, false );
					s_tableGuard.Unlock();
					FreeFile( managedFile, true );
					break;
				}
				else
				{
					s_tableGuard.Unlock(); // someone modified the ref count before we swapped state
				}
			}
		}
		else
		{
			// at least one for the 'loading' reference, and at least one from the pak
			ycAssert( prev.refCount >= 2 );
			ycManagedPak* managedPak = managedFile->m_pak;
			ycAssert( managedPak );
			const RefCountState next( prev.refCount-1, kFileState_Loaded, prev.flags & ~kFileFlag_PakLoadingCount );
			if( managedFile->SwapState( prev, next ) )
			{
				ycAssert( managedPak->GetRefCountState().state == kFileState_Loading );
				if( managedPak->m_loadingCount.Decrement() == 1 )
				{
					FinishLoad( managedPak );
				}
				break;
			}
		}
	}
}

/*static*/ void ycFileManager::Cmd_LoadPak( ycManagedPak* managedPak )
{
//ycLog( "LoadPak %s\n", managedPak->m_name.Get() );
#ifdef YC_CODEGEN
	ycAssert( false );
#else
	// whoever sent the load msg should've switched into this state, and no one else should be allowed to switch away
	ycAssert( managedPak->GetState() == kFileState_Loading && managedPak->m_firstFile == nullptr );

	ycStackString<> pakPath( kDataPath );
	pakPath.Append( managedPak->m_name );
	pakPath.Append( ".pak.yc" );

	#ifdef YC_ENABLE_PROFILER
		const ycTime_t start = ycDateTime::GetTimeHiRes();
	#endif
	#ifdef YC_FILEMANAGER_RANDOM_READ_LATENCY
		ycRand rng;
		ycThread::Sleep( rng.RangeFast( 0, YC_FILEMANAGER_RANDOM_READ_LATENCY ) );
	#endif
	ycSize_t pakSize;
	const u8* pakData = ycFile::Read( pakPath, s_fileMem, &pakSize );
	if( pakData == nullptr )
	{
		// proceed to the NotFound state, if the pak still has references
		for(;;)
		{
			const RefCountState prev = managedPak->GetRefCountState();
			ycAssert( prev.state == kFileState_Loading && prev.refCount > 0 );
			if( prev.refCount > 1 )
			{
				const RefCountState next( prev.refCount-1, kFileState_NotFound, prev.flags );
				if( managedPak->SwapState( prev, next ) )
				{
					break;
				}
			}
			else
			{
				const RefCountState next( 0, kFileState_NotFound, prev.flags );
				s_tableGuard.Lock();
				if( managedPak->SwapState( prev, next ) )
				{
					const bool removed = s_pakTable.Remove( managedPak->m_hashKey );
					ycAssert( removed ); YC_UNUSED( removed );
					s_tableGuard.Unlock();
					FreePak( managedPak );
					break;
				}
				else
				{
					s_tableGuard.Unlock();
				}
			}
		}
		return;
	}
	ycAssertMsg( pakData, "Could not read '%.*s'.", pakPath.Length(), pakPath.Get() );
	#ifdef YC_ENABLE_PROFILER
		ycProfiler::RegisterFileRead( start, ycDateTime::GetTimeHiRes(), pakSize );
	#endif

	// could move much of this work to another thread, but with the interleaved loose-pak reads, would be a bit messy

	ycAssert( managedPak->m_loadingCount.Get() == 0 );
	managedPak->m_loadingCount.Increment(); // set loadingcount to one so it cannot drop to zero while we're still queuing stuff

	ycRtti rootType;
	const ycPakBin* pakRoot = (ycPakBin*)ycData::FixupBinary( ( u8* )pakData, &rootType );
	ycAssert( rootType == ycRtti::Get< ycPakBin >() );

	const ycSize_t numEntries = pakRoot->entries.Length();
	const ycPakEntryBin* entries = pakRoot->entries.begin();
	//ycManagedFile* managedFile = managedPak->m_firstFile;
	ycManagedFile** ppPrevFile = &managedPak->m_firstFile;
	for( ycSize_t i = 0; i != numEntries; ++i )
	{
		const ycPakEntryBin& pakEntry = entries[ i ];
		const u64 hashKey = CalcHashKey( pakEntry.name.GetRef(), kFile_Data );

		ycAllocatorTS* fileMem = nullptr;
		const u8* fileData = pakEntry.data.GetData();
		ycSize_t fileSize = pakEntry.data.GetSize();

		#ifdef YC_FILEMANAGER_ENABLE_MODAPI
			if( s_modProvider )
			{
				ycStackString<> filePath( kDataPath );
				filePath.Append( pakEntry.name.GetRef() );
				s_modProvider( filePath.GetRef(), &fileData, &fileSize );
			}
		#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

		#if defined YC_FILEMANAGER_ENABLE_LOOSE_PAKS
			if( fileData == nullptr )
			{
				ycStackString<> filePath( kDataPath );
				filePath.Append( pakEntry.name.GetRef() );
				#ifdef YC_ENABLE_PROFILER
					const ycTime_t start2 = ycDateTime::GetTimeHiRes();
				#endif
				#ifdef YC_FILEMANAGER_RANDOM_READ_LATENCY
					ycThread::Sleep( rng.RangeFast( 0, YC_FILEMANAGER_RANDOM_READ_LATENCY ) );
				#endif
				fileMem = s_fileMem;
				fileData = ycFile::Read( filePath, s_fileMem, &fileSize );
				ycAssertMsg( fileData, "Could not read '%.*s' (from loose pak '%.*s').", filePath.Length(), filePath.Get(), managedPak->m_name.Length(), managedPak->m_name.Get() );
				#ifdef YC_ENABLE_PROFILER
					ycProfiler::RegisterFileRead( start2, ycDateTime::GetTimeHiRes(), fileSize );
				#endif
			}
		#else
			ycAssert( fileData );
		#endif

		RefCountState initialFileState( 1, kFileState_Loaded, 0 );
		#ifdef YC_FILEMANAGER_ENABLE_AUTO_FIXUP
		if( fileSize >= 4 &&  ycMemEq( fileData, "YCD\0", 4 ) )
		{
			initialFileState.flags = kFileFlag_PakLoadingCount;
			++initialFileState.refCount;
			initialFileState.state = kFileState_Loading;
			managedPak->m_loadingCount.Increment();
		}
		#endif // YC_FILEMANAGER_ENABLE_AUTO_FIXUP

		ycManagedFile* managedFile = CreateFile( pakEntry.name.GetRef(), hashKey, initialFileState ); // bit weird to proceed directly to LOADED, but these files are not in the table yet
		managedFile->m_pak = managedPak;
		*ppPrevFile = managedFile;
		ppPrevFile = &managedFile->m_nextInPak;

		managedFile->m_mem = fileMem;
		managedFile->m_data = fileData;
		managedFile->m_size = fileSize;
		#ifdef YC_FILEMANAGER_ENABLE_AUTO_FIXUP
		if( initialFileState.flags & kFileFlag_PakLoadingCount )
		{
			ycEngine::GetJobQueue().Queue( PostProcessFileJob, managedFile );
		}
		#endif // YC_FILEMANAGER_ENABLE_AUTO_FIXUP
	}

	managedPak->m_mem = s_fileMem;
	managedPak->m_data = pakData;

	// register files
	s_tableGuard.Lock();
	for( ycManagedFile* managedFile = managedPak->m_firstFile; managedFile != nullptr; managedFile = managedFile->m_nextInPak )
	{
#ifdef YC_FILEMANAGER_TRACK_PAKS
		ycManagedFile** ppManagedFile = s_fileTable.Get( managedFile->m_hashKey );
		if( ppManagedFile )
		{
			ycString errorMsg;
			errorMsg.Reserve( 4*1024 );
			errorMsg.AppendF( "Pak '%s' tried to load file '%s', but it was already loaded/loading.\n", managedPak->m_name.Get(), managedFile->m_path.Get() );
			ycManagedPak* loadedByPak = ( *ppManagedFile )->m_pak;
			errorMsg.AppendF( "Already loaded by pak '%s'.\n", loadedByPak ? loadedByPak->m_name.Get() : "<NONE>" );
			if( s_pakTable.Begin().IsValid() )
			{
				const u64 ticksPerSec = ycDateTime::GetTicksPerSecond();
				const u64 now = ycDateTime::GetTimeHiRes();
				errorMsg.Append( "Loaded paks:\n" );
				for( ycOpenHash< u64, ycManagedPak* >::Iter pakIter = s_pakTable.Begin(); pakIter.IsValid(); pakIter.Increment() )
				{
					errorMsg.AppendF( "%s", pakIter.GetValue()->m_name.Get() );
					const u64 loadedTime = now - pakIter.GetValue()->m_loadTime;
					if( loadedTime < ticksPerSec )
					{
						errorMsg.AppendF( " (%.2f ms)\n", ycDateTime::GetSeconds(loadedTime)*1000.0f );
					}
					else if( loadedTime < ticksPerSec*60 )
					{
						errorMsg.AppendF( " (%.2f sec)\n", ycDateTime::GetSeconds(loadedTime) );
					}
					else
					{
						errorMsg.AppendF( " (%.2f min)\n", ycDateTime::GetSeconds(loadedTime)/60.0f );
					}
				}
			}
			ycAssertMsg( false, errorMsg.ToCStr() );
		}
#else
		ycAssertMsg( s_fileTable.Get( managedFile->m_hashKey ) == nullptr, "Pak '%s' tried to load file '%s', but it was already loaded/loading.", managedPak->m_name.Get(), managedFile->m_path.Get() );
#endif
		*s_fileTable.Insert( managedFile->m_hashKey ) = managedFile;
	}
	s_tableGuard.Unlock();

	if( managedPak->m_loadingCount.Decrement() == 1 )
	{
		FinishLoad( managedPak );
	}
#endif
}

/*static*/ void ycFileManager::FinishLoad( ycManagedPak* managedPak )
{
	// proceed to the loaded state, if the pak still has references
	for(;;)
	{
		RefCountState prev = managedPak->GetRefCountState();
		ycAssert( prev.state == kFileState_Loading && prev.refCount > 0 );
		if( prev.refCount > 1 )
		{
			const RefCountState next( prev.refCount-1, kFileState_Loaded, prev.flags );
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				break;
			}
		}
		else
		{
			const RefCountState next( 1, kFileState_Unloading, prev.flags );
			//s_tableGuard.Lock();
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				PushCmd( kCmdType_UnloadPak, managedPak );
				//s_tableGuard.Unlock();
				break;
			}
			//else
			//{
			//	s_tableGuard.Unlock();
			//}
		}
	}
}

/*static*/ void ycFileManager::Cmd_UnloadPak( ycManagedPak* managedPak )
{
	// remove all files from the table
	s_tableGuard.Lock();
	for( ycManagedFile* managedFile = managedPak->m_firstFile; managedFile != nullptr; managedFile = managedFile->m_nextInPak )
	{
		#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
			/* multi version:
			- the original version must be in the LOADED state (we loaded it!)
			- newer versions must be in the LOADING state, any other state indicates that outside systems are holding references
			  they will be stuck in the LOADING state since we're holding the lock, which they would use to enter UNLOADING
			  we'll set the orphan flag so when the LOADING state exits, it does not try to remove itself from the table
			*/
			RemoveFromTable( managedFile, true );
			for( ycManagedFile* ver = managedFile->GetNewest(); ver != nullptr; ver = ver->m_older )
			{
				const RefCountState prev = ver->GetRefCountState();
				if( ver == managedFile )
				{
					ycAssertMsg(
						prev.refCount == 1 && prev.state == kFileState_Loaded,
						"Pak is unloading and file has outstanding references or an unexpected state. Pak '%s', file '%s', refCount %u, state %u.\n",
						managedPak->m_name.Get(), managedFile->m_path.Get(), u32(prev.refCount), u32(prev.state)
					);
				}
				else
				{
					ycAssertMsg(
						prev.refCount == 1 && prev.state == kFileState_Loading,
						"Pak is unloading and file has outstanding references or an unexpected state. Pak '%s', file '%s', refCount %u, state %u.\n",
						managedPak->m_name.Get(), managedFile->m_path.Get(), u32(prev.refCount), u32(prev.state)
					);
					// TODO HACK FIXME: this should probably remove it from the version list as well
					const bool swapped = ver->SwapState( prev, RefCountState( prev.refCount, prev.state, prev.flags|kFileFlag_Orphan ) );
					ycAssert( swapped ); YC_UNUSED( swapped );
				}
			}
		#else
			RemoveFromTable( managedFile, true );
		#endif
	}
	s_tableGuard.Unlock();

	// free the files
	for( ycManagedFile* managedFile = managedPak->m_firstFile; managedFile != nullptr; /*managedFile = managedFile->m_nextInPak*/ )
	{
		ycManagedFile* nextFile = managedFile->m_nextInPak;
		#ifdef YC_ENABLE_ASSERT
			if( managedFile->m_mem == nullptr )
			{
				managedFile->m_data = nullptr;
			}
		#endif // YC_ENABLE_ASSERT
		FreeFile( managedFile, managedFile->m_mem != nullptr );
		managedFile = nextFile;
	}
	managedPak->m_firstFile = nullptr;

	// unload the pak
	for(;;)
	{
		const RefCountState prev = managedPak->GetRefCountState();
		ycAssert( prev.state == kFileState_Unloading );
		if( prev.refCount == 1 ) // the unloading ref is the only one
		{
			const RefCountState next( 0, kFileState_None, prev.flags );
			s_tableGuard.Lock(); // hold the lock so no one can get at the pak while it is being removed
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				const bool removed = s_pakTable.Remove( managedPak->m_hashKey );
				ycAssert( removed ); YC_UNUSED( removed );
				s_tableGuard.Unlock();
				FreePak( managedPak );
				break;
			}
			else
			{
				s_tableGuard.Unlock(); // someone modified the ref count before we swapped state
			}
		}
		else
		{
			const RefCountState next( prev.refCount, kFileState_Loading, prev.flags );
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				PushCmd( kCmdType_LoadPak, managedPak );
				break;
			}
		}
	}
}

/*static*/ void ycFileManager::Cmd_StreamingInit( ycFileStreamer* streamer )
{
#ifdef YC_WIN32
	/* with windows/editor file access can be a little racey, allow some leeway */
	bool opened = false;
	for( ureg i = 0; i != 3; ++i )
	{
		opened = streamer->m_reader.Open( streamer->m_path );
		if( opened ) { break; }
		ycThread::Sleep( 100 );
	}
	ycAssertMsg( opened, streamer->m_path.Get() ); YC_UNUSED( opened );
#else
	const bool opened = streamer->m_reader.Open( streamer->m_path );
	ycAssertMsg( opened, streamer->m_path.Get() ); YC_UNUSED( opened );
#endif
}

/*static*/ void ycFileManager::Cmd_StreamingRead( StreamingRequest* req )
{
	ycFileStreamer* streamer = req->streamer;

	if( streamer->m_tryCancelReads == false && req->tryCancel == false )
	{
		ycSize_t size = req->size; // size may be adjusted if it would go beyond EOF
		if( size == ycSize_t(-1) )
		{
			size = streamer->m_reader.GetSize();
		}
		const ycSize_t offset = req->offset;
		u8* dst = ( u8* )YC_ALLOC( g_defaultMem, size, YC_MEM_DEFAULT_ALIGN, streamer->m_path.Get() );

		streamer->m_reader.SetPos( offset );
		req->dst  = dst;
		#ifdef YC_ENABLE_PROFILER
			const ycTime_t start = ycDateTime::GetTimeHiRes();
		#endif
		#ifdef YC_FILEMANAGER_RANDOM_READ_LATENCY
			ycRand rng;
			ycThread::Sleep( rng.RangeFast( 0, YC_FILEMANAGER_RANDOM_READ_LATENCY ) );
		#endif
		streamer->m_reader.Read( dst, size, &req->size );
		#ifdef YC_ENABLE_PROFILER
			ycProfiler::RegisterFileRead( start, ycDateTime::GetTimeHiRes(), req->size );
		#endif

		ycAssert( req->dst != nullptr );
		#ifdef YC_ENABLE_ASSERT
			ycAssert( req->state.CompareAndSwap( kStreamingState_Pending, kStreamingState_Complete ) );
		#else
			req->state.Set( kStreamingState_Complete );
		#endif
	}
	else
	{
		ycAssert( req->dst == nullptr );
		#ifdef YC_ENABLE_ASSERT
			ycAssert( req->state.CompareAndSwap( kStreamingState_Pending, kStreamingState_Cancelled ) );
		#else
			req->state.Set( kStreamingState_Cancelled );
		#endif
	}
}

/*static*/ void ycFileManager::Cmd_StreamingClose( ycFileStreamer* streamer )
{
	#ifdef YC_ENABLE_ASSERT // check that there are no pending requests
		s_streamRequestListGuard.Lock();
		for( StreamingRequest* req = s_streamRequestHead; req != nullptr; req = req->next )
		{
			if( req->streamer == streamer )
			{
				ycAssert( req->state.Get() == kStreamingState_Complete || req->state.Get() == kStreamingState_Cancelled );
			}
		}
		s_streamRequestListGuard.Unlock();
	#endif // YC_ENABLE_ASSERT

	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		s_fileStreamListGuard.Lock();
		ValidateStreamerList();
		( streamer->m_prev ? streamer->m_prev->m_next : s_fileStreamerHead ) = streamer->m_next;
		if( streamer->m_next ) { streamer->m_next->m_prev = streamer->m_prev; }
		ycAssert( s_numFileStreamers );
		--s_numFileStreamers;
		ValidateStreamerList();
		s_fileStreamListGuard.Unlock();
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	streamer->m_reader.Close();
	ycdelete( &s_fileStreamerMem, streamer );
}

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::Cmd_StreamingReinit( ycFileStreamer* streamer )
{
	streamer->m_reader.Close();
	const bool opened = streamer->m_reader.Open( streamer->m_path );
	ycAssert( opened ); YC_UNUSED( opened );
	streamer->m_tryCancelReads = false;
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::Cmd_Pause( void* pCtx )
{
	PauseCtx* ctx = (PauseCtx*)pCtx;

	s_pendingNotificationGuard.Lock();
	s_processPendingNotifications = false;
	s_pendingNotificationGuard.Unlock();

	struct ClosedStreamer
	{
		ycFileStreamer* streamer;
		u64 mtime;
	};
	ycVector< ClosedStreamer > streamers( g_defaultMem, s_numFileStreamers );
	for( ycFileStreamer* streamer = s_fileStreamerHead; streamer != nullptr; streamer = streamer->m_next )
	{
		if( streamer->m_reader.IsOpen() )
		{
			u64 mtime = 0;
			ycFile::GetModifiedTime( streamer->m_path, &mtime );
			streamers.Append( { streamer, mtime } );
			streamer->m_reader.Close();
		}
	}

	ctx->pausedSignal.Signal();
	ctx->resumeSignal.Wait();

	s_streamRequestListGuard.Lock();
	for( ycSize_t i = 0; i != streamers.Length(); ++i )
	{
		ycFileStreamer* streamer = streamers[ i ].streamer;
		streamer->m_reader.Open( streamer->m_path );
		u64 mtime = 0;
		ycFile::GetModifiedTime( streamer->m_path, &mtime );
		if( mtime != streamers[ i ].mtime )
		{
			for( StreamingRequest* req = s_streamRequestHead; req != nullptr; req = req->next ) // kinda ew, but we shouldn't have that many in-flight at once... if that changes we can optimize then
			{
				if( req->streamer == streamer )
				{
					TryCancelStreamingRead( req );
				}
			}
		}
	}
	s_streamRequestListGuard.Unlock();

	s_pendingNotificationGuard.Lock();
	s_processPendingNotifications = true;
	s_pendingNotificationGuard.Unlock();

	//s_resumedSignal.Signal();
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void* ycFileManager::Pause()
{
	PauseCtx* pauseCtx = ycnew( g_defaultMem, "ycFileManager::PauseCtx" )PauseCtx;
	PushCmd( kCmdType_Pause, pauseCtx );
	pauseCtx->pausedSignal.Wait();
	return pauseCtx;
	//s_pauseRequest.Increment();
	//s_taskSignal.Signal();
	//s_pausedSignal.Wait();
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::Resume( void* pCtx )
{
	PauseCtx* ctx = (PauseCtx*)pCtx;
	ctx->resumeSignal.Signal();
	// TODO HACK FIXME: wait for resume?
	//s_resumeRequest.Signal();
	//s_resumedSignal.Wait();
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::Invalidate( const ycStringRef& path, const FileType fileType, const bool notifyWatchers )
{
	// would be nice to have some way to batch these, many can occur at once
	const u64 hashKey = CalcHashKey( path, fileType );

	s_tableGuard.Lock();
	ycManagedFile** ppManagedFile = s_fileTable.Get( hashKey );
	if( ppManagedFile )
	{
		ycManagedFile* managedFile = *ppManagedFile;
		for(;;)
		{
			const RefCountState prev = managedFile->GetRefCountState();
			if( prev.flags & kFileFlag_OldVersion )
			{
				break;
			}
			const RefCountState next( prev.refCount, prev.state, prev.flags | kFileFlag_OldVersion );
			if( managedFile->SwapState( prev, next ) )
			{
				break;
			}
		}
	}
	s_tableGuard.Unlock();

	if( notifyWatchers )
	{
		s_pendingNotificationGuard.Lock();
		if( s_pendingNotifications.IndexOf( hashKey ) == -1 )
		{
			s_pendingNotifications.Append( hashKey );
		}
		s_pendingNotificationGuard.Unlock();
	}
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

/*static*/ ycManagedFile* ycFileManager::AddRef( const ycStringRef& path, const FileType fileType, ureg fileLoadFlags )
{
	ycAssert( fileType != kFile_Relative );
	#if !defined YC_FILEMANAGER_ENABLE_RESOURCES_ACCESS
		ycAssert( fileType != kFile_Resource );
	#endif
	const u64 hashKey = CalcHashKey( path, fileType );

	s_tableGuard.Lock();
	ycManagedFile** ppManagedFile = s_fileTable.Get( hashKey );
	if( ppManagedFile )
	{
		ycManagedFile* managedFile = *ppManagedFile;
		for(;;)
		{
			const RefCountState prev = managedFile->GetRefCountState();
			#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
			if( prev.flags & kFileFlag_OldVersion )
			{
				// create a new entry, link it to the previous one, and immediately queue loading
				ycManagedFile* newManagedFile = CreateFile( path, hashKey, RefCountState( 2, kFileState_Loading, 0 ) ); // kinda gross to have memory allocation in the lock
				ycAssert( managedFile->m_newer == nullptr );
				managedFile->m_newer = newManagedFile;
				newManagedFile->m_older = managedFile;
				ValidateVersions( newManagedFile );
				newManagedFile->m_pak = managedFile->m_pak;
				*ppManagedFile = newManagedFile;
				s_tableGuard.Unlock();
				PushCmd( kCmdType_LoadFile, newManagedFile, fileLoadFlags|kFileLoadFlag_AllowLoose );
				return newManagedFile;
			}
			else
			#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
			{
				ycAssert( prev.refCount > 0 && prev.state != kFileState_None );
				const RefCountState next( prev.refCount+1, prev.state, prev.flags );
				if( managedFile->SwapState( prev, next ) )
				{
					s_tableGuard.Unlock(); // confirm that we've increased the refcount before dropping the lock, someone else could've dropped it to zero
					return managedFile;
				}
			}

		}
	}
	else
	{
		// create a new entry and immediately queue loading
		ycManagedFile* managedFile = CreateFile( path, hashKey, RefCountState( 2, kFileState_Loading, 0 ) ); // kinda gross to have memory allocation in the lock
		*s_fileTable.Insert( hashKey ) = managedFile;
		s_tableGuard.Unlock();
		#if defined YC_FILEMANAGER_ENABLE_LOOSE_FILES
			fileLoadFlags |= kFileLoadFlag_AllowLoose;
		#elif defined YC_FILEMANAGER_ENABLE_MODAPI
			if( s_modProvider )
			{
				fileLoadFlags |= kFileLoadFlag_AllowLoose;
			}
		#endif
		PushCmd( kCmdType_LoadFile, managedFile, fileLoadFlags );
		return managedFile;
	}
}

#ifdef YC_FILEMANAGER_ENABLE_MODAPI
/*static*/ void ycFileManager::RegisterModProvider( ModProvider modProvider )
{
	s_modProvider = modProvider;
}
#endif

/*static*/ void ycFileManager::AddRef( ycManagedFile* managedFile )
{
	for(;;)
	{
		// just increment the refcount, no state changes should be possible here
		const RefCountState prev = managedFile->GetRefCountState();
		ycAssert( prev.refCount > 0 && prev.state != kFileState_None ); // what managedFile were we passed that has no ref?
		RefCountState next( prev.refCount+1, prev.state, prev.flags );
		if( managedFile->SwapState( prev, next ) )
		{
			break;
		}
	}
}

/*static*/ void ycFileManager::ReleaseRef( ycManagedFile* managedFile )
{
	for(;;)
	{
		const RefCountState prev = managedFile->GetRefCountState();
		if( prev.refCount == 1 && ( prev.state == kFileState_Loaded || prev.state == kFileState_NotFound ) )
		{
			// free the file; if we're in the notfound state don't change it (but i dont think it actually matters)
			RefCountState next( 1, kFileState_Unloading, prev.flags );
			if( prev.state == kFileState_NotFound )
			{
				next.refCount = 0;
				next.state = kFileState_NotFound;
			}
			s_tableGuard.Lock(); // hold the lock so no one can grab the file before we remove it
			if( managedFile->SwapState( prev, next ) )
			{
				RemoveFromTable( managedFile, false );
				s_tableGuard.Unlock();
				FreeFile( managedFile, prev.state == kFileState_Loaded );
				return;
			}
			else
			{
				// the ref count changed, re-evaluate
				s_tableGuard.Unlock();
			}
		}
		else
		{
			// no state change, just reduce the ref count
			const RefCountState next( prev.refCount-1, prev.state, prev.flags );
			if( managedFile->SwapState( prev, next ) )
			{
				return;
			}
		}
	}
}

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::ValidateVersions( ycManagedFile* managedFile )
{
	managedFile = managedFile->GetNewest();
	while( managedFile )
	{
		if( managedFile->m_newer )
		{
			ycAssert( managedFile->GetRefCountState().flags & ycFileManager::kFileFlag_OldVersion );
			ycAssert( managedFile->m_newer->m_older == managedFile );
		}
		//else
		//{
		//	ycAssert( (managedFile->GetRefCountState().flags&ycFileManager::kFileFlag_OldVersion) == 0 );
		//}
		if( managedFile->m_older )
		{
			ycAssert( managedFile->m_older->m_newer == managedFile );
		}
		managedFile = managedFile->m_older;
	}
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

/*static*/ ycManagedPak* ycFileManager::AddPakRef( const char* pakName )
{
	const ycStringRef dataPath( pakName );
	const u64 hashKey = dataPath.Hash64();

	//if( dataPath.ContainsI( "gator" ) )
	//{
	//	__debugbreak();
	//}

	s_tableGuard.Lock();
	ycManagedPak** ppManagedPak = s_pakTable.Get( hashKey );
	if( ppManagedPak )
	{
		ycManagedPak* managedPak = *ppManagedPak;
		for(;;)
		{
			const RefCountState prev = managedPak->GetRefCountState();
			ycAssert( prev.state != kFileState_None ); // pak ref counts can be observed at zero, if we caught it during unloading
			const RefCountState next( prev.refCount+1, prev.state, prev.flags );
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				s_tableGuard.Unlock(); // dont allow ref count to drop to zero before we've committed the ref
				return managedPak;
			}
		}
	}
	else
	{
		// kinda gross to have memory allocation in the lock
		ycManagedPak* managedPak = (ycManagedPak*)YC_ALLOC( &s_managedPakMem, sizeof(ycManagedPak), alignof(ycManagedPak), "ycManagedPak" );
		new( managedPak )ycManagedPak( RefCountState( 2, kFileState_Loading, 0 ) );
		managedPak->m_name.Set( pakName );
		managedPak->m_hashKey = hashKey;
		#ifdef YC_FILEMANAGER_TRACK_PAKS
			managedPak->m_loadTime = ycDateTime::GetTimeHiRes();
		#endif
		//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, 0, kFileState_None, 2, kFileState_Loading, __LINE__ );
		*s_pakTable.Insert( hashKey ) = managedPak;
		s_tableGuard.Unlock();
		PushCmd( kCmdType_LoadPak, managedPak );
		return  managedPak;
	}
}

/*static*/ void ycFileManager::AddPakRef( ycManagedPak* managedPak )
{
	for(;;)
	{
		const RefCountState prev = managedPak->GetRefCountState();
		ycAssert( prev.refCount > 0 && prev.state != kFileState_None ); // what managedFile were we passed that has no ref? 
		RefCountState next( prev.refCount+1, prev.state, prev.flags );
		if( managedPak->SwapState( prev, next ) )
		{
			//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
			break;
		}
	}
}

/*static*/ void ycFileManager::ReleasePakRef( ycManagedPak* managedPak )
{
	for(;;)
	{
		const RefCountState prev = managedPak->GetRefCountState();
		ycAssert( prev.refCount );
		if( prev.refCount == 1 && ( prev.state == kFileState_Loaded || prev.state == kFileState_NotFound ) )
		{
			RefCountState next( 1, kFileState_Unloading, prev.flags );
			if( prev.state == kFileState_NotFound )
			{
				// if we're in the NotFound state, stay there, and take the lock so we can free the pak immediately
				next.refCount = 0;
				next.state = kFileState_NotFound;
				s_tableGuard.Lock();
			}
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				if( next.state == kFileState_NotFound )
				{
					ycAssert( managedPak->m_firstFile == nullptr );
					const bool removed = s_pakTable.Remove( managedPak->m_hashKey );
					ycAssert( removed ); YC_UNUSED( removed );
					s_tableGuard.Unlock();
					FreePak( managedPak );
				}
				else
				{
					PushCmd( kCmdType_UnloadPak, managedPak );
				}
				break;
			}
			else
			{
				if( next.state == kFileState_NotFound )
				{
					s_tableGuard.Unlock();
				}
			}
		}
		else
		{
			// no state change, just reduce the ref count
			const RefCountState next( prev.refCount-1, prev.state, prev.flags );
			if( managedPak->SwapState( prev, next ) )
			{
				//ycLog( "pak %p ( %u, %u ) -> ( %u, %u ) %u\n", managedPak, prev.refCount, prev.state, next.refCount, next.state, __LINE__ );
				return;
			}
		}
	}
}

/*static*/ ycManagedFile* ycFileManager::CreateFile( const ycStringRef& path, const u64 hashKey, const RefCountState& initialState )
{
	ycManagedFile* managedFile = (ycManagedFile*)YC_ALLOC( &s_managedFileMem, sizeof(ycManagedFile), alignof(ycManagedFile), "ycManagedFile" );
	new( managedFile )ycManagedFile( initialState );
	managedFile->m_path.Set( path );
	managedFile->m_hashKey = hashKey;
	return managedFile;
}

/*static*/ void ycFileManager::RemoveFromTable( ycManagedFile* managedFile, const bool allVersions )
{
#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
	ValidateVersions( managedFile );
	if( allVersions )
	{
		const bool removed = s_fileTable.Remove( managedFile->m_hashKey );
		ycAssert( removed ); YC_UNUSED( removed );
	}
	else
	{
		if( managedFile->m_newer )
		{
			managedFile->m_newer->m_older = managedFile->m_older;
		}
		else
		{
			if( managedFile->m_older )
			{
				ycAssert( *s_fileTable.Get( managedFile->m_hashKey ) == managedFile );
				*s_fileTable.Get( managedFile->m_hashKey ) = managedFile->m_older;
			}
			else
			{
				const bool removed = s_fileTable.Remove( managedFile->m_hashKey );
				ycAssert( removed ); YC_UNUSED( removed );
			}
		}
		if( managedFile->m_older )
		{
			managedFile->m_older->m_newer = managedFile->m_newer;
		}
	}
#else // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
	YC_UNUSED( allVersions );
	const bool removed = s_fileTable.Remove( managedFile->m_hashKey );
	ycAssert( removed ); YC_UNUSED( removed );
#endif // !YC_FILEMANAGER_ENABLE_FILE_VERSIONS
}

/*static*/ void ycFileManager::FreeFile( ycManagedFile* managedFile, const bool hasData )
{
	if( hasData )
	{
	#ifdef YC_FILEMANAGER_ENABLE_MODAPI
		/* TODO HACK FIXME: clean up ownership of file data
		is there a reason this doesnt just YC_FREE if m_mem?
		that may be reasonable, but then why wasn't it like that already?

		really should clean this up, but too risky to change too much file mgr on mina so close to release
		*/
		ycAssert( managedFile->m_data );
		if( s_modProvider )
		{
			if( managedFile->m_mem )
			{
				YC_FREE( managedFile->m_mem, (void*)managedFile->m_data );
			}
		}
		else
		{
			ycAssert( managedFile->m_mem && managedFile->m_data );
			YC_FREE( managedFile->m_mem, (void*)managedFile->m_data );
		}
	#else
		ycAssert( managedFile->m_mem && managedFile->m_data );
		YC_FREE( managedFile->m_mem, (void*)managedFile->m_data );
	#endif
	}
	else
	{
		ycAssert( managedFile->m_mem == nullptr && managedFile->m_data == nullptr );
	}
	managedFile->~ycManagedFile();
	YC_FREE( &s_managedFileMem, managedFile );
}

/*static*/ void ycFileManager::FreePak( ycManagedPak* managedPak )
{
	if( managedPak->m_data ) { YC_FREE( managedPak->m_mem, (void*)managedPak->m_data ); }
	managedPak->~ycManagedPak();
	YC_FREE( &s_managedPakMem, managedPak );
}

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::ProcessPendingNotifications()
{
	s_pendingNotificationGuard.Lock();
	if( s_processPendingNotifications )
	{
		s_notifierGuard.Lock();
		#ifdef YC_FILEMANAGER_RANDOM_FILE_NOTIFICATIONS
		{
			ycRand rng;
			const ycSize_t slot = rng.RangeFast( 0, u32(s_notifiers.GetCapacity()-1) );
			const ycHashKey_t key = s_notifiers.KeyAt( slot );
			ycFileManagerChangeNotifier* notifier = s_notifiers.ValueAt( slot );
			if( key && notifier )
			{
				s_tableGuard.Lock();
				ycManagedFile** ppManagedFile = s_fileTable.Get( key );
				if( ppManagedFile )
				{
					ycManagedFile* managedFile = *ppManagedFile;
					for(;;)
					{
						const RefCountState prev = managedFile->GetRefCountState();
						if( prev.flags & kFileFlag_OldVersion )
						{
							break;
						}
						const RefCountState next( prev.refCount, prev.state, prev.flags | kFileFlag_OldVersion );
						if( managedFile->SwapState( prev, next ) )
						{
							break;
						}
					}
				}
				s_tableGuard.Unlock();
				notifier->m_callback( notifier->m_userData );
			}
		}
		#endif
		for( ureg i = 0; i != s_pendingNotifications.Length(); ++i )
		{
			ycFileManagerChangeNotifier** ppNotifier = s_notifiers.Get( s_pendingNotifications.At( i ) );
			if( ppNotifier )
			{
				for( ycFileManagerChangeNotifier* notifier = *ppNotifier; notifier != nullptr; notifier = notifier->m_next )
				{
					notifier->m_callback( notifier->m_userData );
				}
			}
		}
		s_pendingNotifications.Clear( false );
		s_notifierGuard.Unlock();
	}
	s_pendingNotificationGuard.Unlock();
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::ClearPendingNotifications()
{
	s_pendingNotificationGuard.Lock();
	s_pendingNotifications.Clear( false );
	s_pendingNotificationGuard.Unlock();
}
#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

/*static*/ bool ycFileManager::IsRunning()
{
	return s_thread != ycThread::kInvalidThread;
}

//
// Streaming
//

namespace
{
	void ValidateStreamerList()
	{
	#if 0
		ycFileStreamer* prev = nullptr;
		for( ycFileStreamer* streamer = s_fileStreamerHead; streamer != nullptr; streamer = streamer->m_next )
		{
			ycAssert( streamer->m_prev == prev );
			prev = streamer;
			if( streamer->m_prev )
			{
				ycAssert( streamer->m_prev->m_next == streamer );
			}
			else
			{
				ycAssert( s_fileStreamerHead == streamer );
			}
			if( streamer->m_next )
			{
				ycAssert( streamer->m_next->m_prev == streamer );
			}
		}
		#endif
	}
}

/*static*/ ycFileStreamer* ycFileManager::InitStreaming( const char* path, const FileType fileType )
{
	ycStackString< YC_MAX_PATH > fullPath;
	ycFileManager::Prefix( &fullPath, ycStringRef(path), fileType );

	ycFileStreamer* streamer = ycnew( &s_fileStreamerMem, fullPath.ToCStr() ) ycFileStreamer( fullPath.ToCStr() );

	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		s_fileStreamListGuard.Lock();
		ValidateStreamerList();
		streamer->m_next = s_fileStreamerHead;
		if( s_fileStreamerHead ) { s_fileStreamerHead->m_prev = streamer; }
		s_fileStreamerHead = streamer;
		++s_numFileStreamers;
		ValidateStreamerList();
		s_fileStreamListGuard.Unlock();
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	PushCmd( kCmdType_StreamingInit, streamer );

	return streamer;
}

/*static*/ ycFileManager::StreamingRequest* ycFileManager::BeginStreamingRead( ycFileStreamer* streamer, const ycSize_t offset, const ycSize_t size )
{
	StreamingRequest* req = ycnew( &s_streamRequests, streamer->m_path.Get() )StreamingRequest;
	req->state     = kStreamingState_Pending;
	req->dst       = nullptr;
	req->size      = size;
	req->offset    = offset;
	req->streamer  = streamer;
	req->prev      = nullptr;
	req->next      = nullptr;
	req->tryCancel = false;
	req->userData  = 0;

	s_streamRequestListGuard.Lock();
	if( s_streamRequestHead )
	{
		req->next = s_streamRequestHead;
		ycAssert( s_streamRequestHead->prev == nullptr );
		s_streamRequestHead->prev = req;
		s_streamRequestHead = req;
	}
	else
	{
		s_streamRequestHead = req;
	}
	s_streamRequestListGuard.Unlock();

	PushCmd( kCmdType_StreamingRead, req );

	return req;
}

/*static*/ void ycFileManager::TryCancelStreamingRead( StreamingRequest* req )
{
	req->tryCancel = true;
}

/*static*/ void ycFileManager::FinishStreamingRead( StreamingRequest* req )
{
	ycAssert( req->state.Get() != kStreamingState_Pending );

	s_streamRequestListGuard.Lock();
	if( req->prev )
	{
		ycAssert( s_streamRequestHead != req );
		ycAssert( req->prev->next == req );
		req->prev->next = req->next;
		if( req->next )
		{
			ycAssert( req->next->prev == req );
			req->next->prev = req->prev;
		}
	}
	else
	{
		ycAssert( s_streamRequestHead == req );
		if( req->next )
		{
			ycAssert( req->next->prev == req );
			req->next->prev = nullptr;
			s_streamRequestHead = req->next;
		}
		else
		{
			s_streamRequestHead = nullptr;
		}
	}
	s_streamRequestListGuard.Unlock();
	if( req->state.Get() == kStreamingState_Cancelled )
	{
		ycAssert( req->dst == nullptr );
	}
	else
	{
		ycdelete( g_defaultMem, req->dst );
	}
	ycdelete( &s_streamRequests, req );
}

/*static*/ void ycFileManager::FinishStreaming( ycFileStreamer* streamer, const bool tryCancelReads )
{
	streamer->m_tryCancelReads = tryCancelReads;
	PushCmd( kCmdType_StreamingClose, streamer );
}

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
/*static*/ void ycFileManager::ReinitStreaming( ycFileStreamer* streamer, const bool tryCancelReads )
{
	streamer->m_tryCancelReads = tryCancelReads;
	PushCmd( kCmdType_StreamingReinit, streamer );
}
#endif

//
// File Change Notifier
//

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS

ycFileManagerChangeNotifier::ycFileManagerChangeNotifier()
{
	m_callback = nullptr;
	m_userData = 0;
	m_prev     = nullptr;
	m_next     = nullptr;
}

ycFileManagerChangeNotifier::ycFileManagerChangeNotifier( const ycStringRef& path, const ycFileManager::FileType fileType, ChangeCallBack callback, const u64 userData )
{
	m_callback = nullptr;
	m_userData = 0;
	m_prev     = nullptr;
	m_next     = nullptr;
	Init( path, fileType, callback, userData );
}

void ycFileManagerChangeNotifier::Init( const ycStringRef& path, const ycFileManager::FileType fileType, ChangeCallBack callback, const u64 userData )
{
	//ycLog( "ycFileManagerChangeNotifier::Init( %s )\n", path );
	ycAssert( fileType != ycFileManager::kFile_Relative ); // cannot use relative paths, they are not portable
	Clear();
	ycAssert( !path.IsEmpty() );
	m_path.Set( path );
	m_hashKey  = ycFileManager::CalcHashKey( path, fileType );
	m_callback = callback;
	m_userData = userData;
	m_prev     = nullptr;
	s_notifierGuard.Lock();
	ycFileManagerChangeNotifier** ppNotifier = s_notifiers.Get( m_hashKey );
	if( ppNotifier )
	{
		m_next = *ppNotifier;
		m_next->m_prev = this;
		*ppNotifier = this;
	}
	else
	{
		m_next = nullptr;
		s_notifiers.Insert( m_hashKey, this );
	}
	s_notifierGuard.Unlock();
}

void ycFileManagerChangeNotifier::Clear()
{
	if( !m_path.IsEmpty() )
	{
		//ycLog( "ycFileManagerChangeNotifier::Clear( %s )\n", m_path.Get() );
		s_notifierGuard.Lock();
		ycFileManagerChangeNotifier** ppNotifier = s_notifiers.Get( m_hashKey );
		ycAssert( ppNotifier );
		if( m_prev == nullptr )
		{
			ycAssert( this == *ppNotifier );
			if( m_next )
			{
				*ppNotifier = m_next;
				m_next->m_prev = nullptr;
			}
			else
			{
				s_notifiers.Remove( m_hashKey );
			}
		}
		else
		{
			ycAssert( this != *ppNotifier );
			m_prev->m_next = m_next;
			if( m_next )
			{
				m_next->m_prev = m_prev;
			}
		}
		s_notifierGuard.Unlock();
		m_path.Clear();
		m_hashKey  = 0;
		m_callback = nullptr;
		m_userData = 0;
	}
}

ycFileManagerChangeNotifier::~ycFileManagerChangeNotifier()
{
	Clear();
}

#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
