#pragma once

#include "ycCommon.h"

// core
#include "ycString.h"
#include "ycStringRef.h"
#include "ycVector.h"

/*
\class ycDir

Implements directory handling
*/

typedef u64 ycFileTime_t;

class ycDir
{
public:

	ycDir();
	void Init( const char* path );
	void Init( const ycStringRef& path );
	ycDir( const char* path );
	ycDir( const ycStringRef& path );
	~ycDir();

	bool Exists();
	static bool Exists( const char* path );
	static bool Exists( const ycStringRef& path );

	enum
	{
		kFilter_File			= 1<<0,
		kFilter_Directory		= 1<<1,
		kFilter_NoExclamation	= 1<<2,
		kFilter_Default			= kFilter_File | kFilter_Directory,
	};

	enum
	{
		kFlag_Recursive			= 1<<0,
		kFlag_GetModifiedTimes	= 1<<1, // free on some platforms, manually invokes ycFile::GetModifiedTime otherwise
		kFlag_Default			= 0
	};

	struct FileDesc
	{
		ycString     path;
		ycFileSize_t size;
		ycFileTime_t mtime; // only set if kFlag_GetModifiedTimes is passed; matches the values ycFile::GetModifiedTime would return, see documentation for potential limitations
		bool         isDir;
	};

	bool ReadDir( ycVector< FileDesc >* dst, const ureg filter = kFilter_Default, const ureg flags = kFlag_Default );

	/*! search for and set the current working directory
	- This traverses upward until it can find the specified file.
	- This is not implemented on platforms where launching from a variable CWD is not a concern, and a fatal error will be triggered.
	- If 'findFile' ends in a / it is treated as a directory name.
	*/
	static bool SetCurrentNextTo( const ycStringRef& findFile );
	
	/*! set current working directory
	- This is not implemented on platforms where launching from a variable CWD is not a concern, and a fatal error will be triggered.
	*/
	static bool SetCurrent( const ycStringRef& path );

	/*! get current working directory
	- Returns a cached copy with no mutex, but we usually set the cwd once at boot and leave it alone.
	*/
	static ycStringRef GetCurrent();

	//! remove redundant /.. from path, make sure slashes are consistent
	static void CleanupPath( ycString* path, const bool toLower = false );
	static ycSize_t CleanupPath( char* path, const bool toLower = false );

	//! convert absolute path to relative, root path should be cleaned up of "/.." before calling
	static const char* RelativePath( char *dest, ycSize_t destSize, const char* absolute, const char* root );

	//! get the canonical case spelling for the input path
	static ycString GetCanonicalPathCase( const char* path );

	//! create a directory, including its parents if necessary
	/*!
	- This won't currently work for UNC paths.
	- Returns "true" if the path was created, or already exists
	*/
	static bool MakePath( ycStringRef path );

	//! copies directory and all files
	static bool Copy( const ycStringRef& srcPath, const ycStringRef& dstPath );

	//! deletes a directory and all files
	static bool Delete( const ycStringRef& srcPath ); 

	//! returns the path to built data for the currently running platform
	static const char* GetDataPath();

	//! returns the path where save data can be stored across engine
	//! this is effectively a tools-only function, do not write shipping save data here.
	static const char* GetEngineSavePath();

protected:

	ycString m_path;
	static bool ReadDir_Recursive( const ycStringRef src, ycVector< ycDir::FileDesc >* dst, const ureg filter, const ureg flags );
};
