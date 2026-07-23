#pragma once
#ifndef YC_FILEMANAGER_H
#define YC_FILEMANAGER_H

/*! \class ycFileManager

ycFileManager centralizes low level file access for various higher level systems, allowing us to schedule and abstract
IO ourselves instead of depending on the OS to handle concurrent file access, callbacks, etc in the way we want.

The most common interaction with ycFileManager is through ycFileRef. ycFileRef provides access to a file's entire
contents with a reference counted backing cache.

Some systems use the streaming API which maps fairly directly to low level file read calls. The streaming API expects
calling code to control access patterns and caching, if necessary.

When combined with our tools, ycFileManager supports multiple versions of the same file.

Pre-existing references can still refer to old data, but if a new reference is created it will refer to the latest
version of the data. This currently does not apply to 'cloned' references; the new copy of the reference will point
to whichever version of the data the previous reference did.

=== Concerning File Paths & Types ===
This is the current file path terminology used in ycFileManager:

Relative:	A file path relative to the current working directory. This should only be used in tools.
Base:		The canonical form of a file path for a game resource file, something like "materials/foo/bar.rmtl.yc". The source
			versions of the imported resources live in `resources/` and `engine/resources`. The built versions live in `data/`.
Resource:	A resource file. If a matching file doesn't exist in `resources/`, a path in `engine/resources/` will be used.
Data:		A built file in `data/`.

Note: ycFileRef/ycFileRefBase and some of the ycFileManager functions take a file type argument, which tell them which kind of file
you're trying to access. If you pass kFile_Relative then these functions will assume that you are passing a relative path,
otherwise they will treat the path as a base path and generate the appropriate relative path based on the file type.
Any function in ycFileManager or things like ycFile and ycDir that don't take a file type assume you are passing a relative path.

TODO HACK FIXME: 
- It may be benificial to have multiple file workers, but it makes sequencing a lot more difficult
*/

#include "ycAtomic.h"
#include "ycString.h"
#include "ycEngineConfig.h"

class ycAtomic;
class ycAllocatorTS;
class ycFileReader;
class ycFileRefBase;
class ycFileStreamer;
class ycManagedFile;
class ycManagedPak;

#if ( defined YC_EDITOR || defined YC_DEBUG || defined YC_LOC_BUILD )
	#define YC_FILEMANAGER_ENABLE_FILE_VERSIONS
#endif

#if YC_NDA
#endif

// track pak load times to display them in errors
#define YC_FILEMANAGER_TRACK_PAKS

#define YC_FILEMANAGER_ENABLE_RESOURCES_ACCESS

class ycFileManager
{
public:

	enum FileType
	{
		kFile_Data = 0,
		kFile_Resource,
		kFile_Relative
	};

	enum
	{
		/* Allow reading a loose file even if the LOOSE_FILES define is not enabled.
		Has no effect if the file is already loaded (does not _force_ a file read).
		*/
		kFileLoadFlag_AllowLoose = 1<<3,

		/* Force checking disk for newer data
		If no new data is available, may return a ref to existing data.

		NYI: can't currently implement this, ycFileManager has no way to distinguish whether there is newer data!
		- could either just force a read/copy (like calling Invalidate)
		- or would need to add mtime tracking or something
		*/
		#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		//	kFileLoadFlag_CheckForLatest = 1<<0,
		#endif
	};

	static void Start( ycAllocatorTS* allocator );
	static void Stop();

	static bool IsRunning();

	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		/*! Pausing the file manager prevents it from accessing files, including closing any file handles open for
		streaming. Pause() will block until the pause has been confirmed and no more file access will occur. Resume()
		blocks until streaming file handles have been re-opened.
		- Uncompleted queued streaming reads will be cancelled if the file was modified between Pause/Resume (since the
		  offset/size may no longer make sense)
		- File requests (streaming or not) issued before Pause() may be delayed until after Resume(); this is done because
		  most use cases of this should complete asap, but if stronger ordering is necessary we could add it as an option
		*/
		static void* Pause();
		static void Resume( void* pauseCtx );

		//! Removes an entry from the central hash table, forcing new requests to re-load the data from disk
		/*!
		- This is so builder can alert that new data is available on disk.
		- Existing references to old data continue to exist
		- This can optionally signal file watchers interested in changes to the file, indicating that new data is available
		*/
		static void Invalidate( const ycStringRef& path, const FileType fileType, const bool notifyWatchers = true );

		static void ProcessPendingNotifications();
		static void ClearPendingNotifications();
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	//! Streaming

	enum// : u32
	{
		kStreamingState_Pending   = 0,
		kStreamingState_Complete  = 1,
		kStreamingState_Cancelled = 2
	};

	struct StreamingRequest
	{
	public:
		ycAtomic state;
		u8*      dst;
		ycSize_t size;
		ycSize_t offset;
		ureg     userData;
	protected: friend class ycFileManager;
		ycFileStreamer*   streamer;
		StreamingRequest* prev;
		StreamingRequest* next;
		bool              tryCancel;
	};

	//! Opens a file for streaming
	static ycFileStreamer* InitStreaming( const char* path, const FileType fileType = kFile_Data );

	//! Request a streaming read
	/*!
	- When the read is complete, StreamingRequest::state will be updated
	- 'size' will be bound to a safe maximum; it is safe to request a size that goes beyond the file bounds. When this
	  occurs StreamingRequest::size will be updated to reflect the actual read size.
	- When calling code is finished with the data it must call FinishStreamingRead().
	*/
	static StreamingRequest* BeginStreamingRead( ycFileStreamer* streamer, const ycSize_t offset, const ycSize_t size );

	//! Tries to cancel a streaming read
	/*!
	- If successful, the request's state will become kStreamingState_Cancelled (eventually)
	- Regardless of success, FinishStreamingRead() must be called on the StreamingRequest
	*/
	static void TryCancelStreamingRead( StreamingRequest* request );

	//! Cleans up after a streaming read
	/*!
	- This should be called reasonably soon after the read completes (eg do not keep a StreamingRequest* ptr for as long
	  as you need the data!), so the data structures can be reused for other streaming requests.
	- It is not safe to access the StreamingRequest or its dst buffer after calling this.
	- This cannot be called on a read that has not completed yet.
	- FinishStreaming() implicitely cleans up any remaining streams.
	*/
	static void FinishStreamingRead( StreamingRequest* request );

	//! Destroys a streamer
	/*!
	- It is not safe to use the ycFileStreamer* after calling this
	- Reads issued before calling this will be completed unless 'tryCancelReads' is true.
	- If 'tryCancelReads' is true, pending reads _may_ be cancelled.
	- All reads issued before this must still have FinishStreamingRead called on them.
	*/
	static void FinishStreaming( ycFileStreamer* streamer, const bool tryCancelReads = false );

	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		/*! Re-opens a file for streaming
		- If 'tryCancelReads' is true, pending reads _may_ be cancelled.
		*/
		static void ReinitStreaming( ycFileStreamer* streamer, const bool tryCancelReads );
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	static void Prefix( ycString* dst, const ycStringRef& basePath, const FileType type = kFile_Data, bool wildcard = false, u32 dirRules = 0 );
	static void PrefixForWrite( ycString* dst, const ycStringRef& basePath, const FileType type = kFile_Data, bool engineResources = false );
	static ycStringRef GetBasePath( ycStringRef path ); // returned ycStringRef is a pointer into the ycStringRef that was passed in
	static bool Exists( const ycStringRef& path, const FileType type = kFile_Data );
	static bool Exists( const ycStringRef& path, const bool checkWildCard, const u32 dirRules );

	//! tries to turn the path (one from your disk) into a path local to the project or the resources directly
	static bool GetValidPath( ycString* path, bool wildcard = false, u32 dirRules = 0 );

	static u64 CalcHashKey( const ycStringRef& path, const FileType fileType );

	//
	// modding api
	//

#ifdef YC_FILEMANAGER_ENABLE_MODAPI
	typedef void( *ModProvider )( const ycStringRef& path, const u8** ppData, ycSize_t* dataSize );
	static void RegisterModProvider( ModProvider modProvider );
#endif

protected:

	friend class ycManagedFile;
	friend class ycFileRefBase;
	static ycManagedFile* AddRef( const ycStringRef& path, const FileType fileType, ureg fileLoadFlags );
	static void AddRef( ycManagedFile* managedFile );
	static void ReleaseRef( ycManagedFile* managedFile );
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		static void ValidateVersions( ycManagedFile* managedFile ); // assumes it is called from within the table lock
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	friend class ycManagedPak;
	friend class ycPakRef;
	static ycManagedPak* AddPakRef( const char* pakName );
	static void AddPakRef( ycManagedPak* pak );
	static void ReleasePakRef( ycManagedPak* pak );

	struct alignas(alignof(u32)) RefCountState
	{
		RefCountState() = default;
		inline RefCountState( const u16 _refCount, const u8 _state, const u8 _flags ) : refCount( _refCount ), state( _state ), flags( _flags ) {}
		inline RefCountState( const ycAtomic& atom ) { *reinterpret_cast< ycAtomic* >( this ) = atom; }
		u16 refCount;
		u8 state;
		u8 flags;
		u32 AsU32() const { return *( (u32*)this ); }
		//static RefCountState* From( ycAtomic* atomic ) { return reinterpret_cast< RefCountState* >( atomic ); }
	};

	static ycManagedFile* CreateFile( const ycStringRef& path, const u64 hashKey, const RefCountState& initialState );
	static void RemoveFromTable( ycManagedFile* managedFile, const bool allVersions );
	static void FreeFile( ycManagedFile* managedFile, const bool hasData );
	static void FreePak( ycManagedPak* managedPak );

	static void WorkerMain( void* );
	static void Cmd_LoadFile( ycManagedFile* managedFile, const u64 fileLoadFlags );
	static void FinishLoad( ycManagedFile* managedFile );
	static void Cmd_LoadPak( ycManagedPak* managedPak );
	static void FinishLoad( ycManagedPak* managedPak );
	static void Cmd_UnloadPak( ycManagedPak* managedPak );
	static void Cmd_StreamingInit( ycFileStreamer* streamer );
	static void Cmd_StreamingRead( StreamingRequest* req );
	static void Cmd_StreamingClose( ycFileStreamer* streamer );
	static void Cmd_StreamingReinit( ycFileStreamer* streamer );
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		static void Cmd_Pause( void* pCtx );
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS

	// used by ycManagedFile / ycManagedPak
	enum
	{
		kFileState_None = 0,
		kFileState_Loading,
		kFileState_Loaded,
		kFileState_Unloading,
		kFileState_NotFound // will not transition out of this state; if multiple versions are enabled, you must re-query from the file table to force a new load attempt
	};

	enum
	{
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		kFileFlag_OldVersion = 1<<0, // if this entry is encountered in the table, a new entry should be created and the file should be read from disk
		kFileFlag_Orphan = 1<<1, // the file has been removed from the table and it has no external references, it can be destroyed whenever loading completes with no possibility its ref count may increase
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		kFileFlag_PakLoadingCount = 1<<2, // this file contributed to the m_loadingCount of a pak
	};

	static void PostProcessFile( ycManagedFile* managedFile );
	friend void PostProcessFileJob( void* userData );
};

#endif // !YC_FILEMANAGER_H
