#include "ycDir.h"

#include "ycFile.h"
#include "ycStackString.h"
#include "ycStringUtils.h"
#include "ycSort.h"

#if defined YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
	#include <shellapi.h>
	#include <shlwapi.h> // PathIsRelative
	//#include <direct.h> // _mkdir
#endif // YC_MSFT

#if defined YC_SDL_MAIN
	#include <../3rdparty/SDL2/include/SDL_filesystem.h>
	#include <../3rdparty/SDL2/include/SDL_error.h>
#endif

#if YC_NDA
#endif 

#if YC_NDA
#endif 

#if YC_NDA
#endif 

#if defined YC_NIX || defined YC_OSX
	#include <errno.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <unistd.h> // access, getcwd
	#include <dirent.h>
#endif

namespace
{
	#if defined YC_MSFT
		struct Utf8To16
		{
			Utf8To16( const char* str ) { MultiByteToWideChar( CP_UTF8, 0, str, -1, buf, YC_ARRAY_SIZE(buf) ); }
			Utf8To16( const ycStringRef& str )
			{
				int last = MultiByteToWideChar( CP_UTF8, 0, str.Get(), int(str.Length()), buf, YC_ARRAY_SIZE(buf) );
				buf[last] = '\0'; // if passed an explicit length, MultiByteToWideChar won't NUL terminate
			}
			wchar_t buf[ YC_MAX_PATH ] = {};
			operator LPCWSTR() { return buf; }
		};
		void Utf16To8( const wchar_t* src, ycString* dst, bool append )
		{
			int need = WideCharToMultiByte( CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL );
			if( !append ) { dst->Clear( false ); }
			dst->ReserveMore( ycSize_t(need) );
			WideCharToMultiByte( CP_UTF8, 0, src, -1, (char*)(dst->Get()+dst->Length()), int(dst->Capacity()-dst->Length()), NULL, NULL );
			dst->SetLength( ycStringUtils::Length( dst->Get() ) );
		}
	#endif
	ycStackString< YC_MAX_PATH > s_cwd;
	#if defined YC_SDL_MAIN
		const u32 kMaxSavePath = 1024;
		char s_savePath[ kMaxSavePath ] = { 0 };
	#endif
	void GetCwd( ycString* dst )
	{
		#if defined YC_MSFT
			wchar_t buf[ YC_MAX_PATH ] = {};
			GetCurrentDirectoryW( YC_MAX_PATH, buf );
			Utf16To8( buf, dst, true );
		#elif defined YC_NIX || defined YC_OSX
			dst->Reserve( YC_MAX_PATH );
			getcwd( (char*)dst->Get(), YC_MAX_PATH );
			dst->SetLength( ycStringUtils::Length( dst->Get() ) );
		#else
			YC_UNUSED( dst );
			ycFatal();
		#endif
	}
}

ycDir::ycDir()
{
}

void ycDir::Init( const char* path )
{
	m_path = ycString( path );
}

void ycDir::Init( const ycStringRef& path )
{
	m_path.Set( path );
}

ycDir::ycDir( const char* path )
	: m_path( path )
{
}

ycDir::ycDir( const ycStringRef& path )
	: m_path( path )
{
}

ycDir::~ycDir()
{
}

#if !defined YC_OSX
bool ycDir::Exists()
{
#if defined YC_MSFT
	DWORD attrib = GetFileAttributesW( Utf8To16(m_path) );
	return attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#elif YC_NDA
#elif YC_NDA
#elif YC_NDA
#elif defined YC_NIX 
	return access( m_path.Get(), F_OK ) == 0;
#else
	ycFatal();
	return false;
#endif
}
#endif // YC_OSX

/*static*/ bool ycDir::Exists( const char* path )
{
	return ycDir( path ).Exists();
}

/*static*/ bool ycDir::Exists( const ycStringRef& path )
{
	return ycDir( path ).Exists();
}

#if defined YC_MSFT
/*static*/ bool ycDir::ReadDir_Recursive( const ycStringRef src, ycVector< ycDir::FileDesc >* dst, const ureg filter, const ureg flags )
{
	WIN32_FIND_DATAW findData;
	wchar_t findPathBuf[ MAX_PATH ];
	{
		int last = MultiByteToWideChar( CP_UTF8, 0, src.Get(), int(src.Length()), findPathBuf, YC_ARRAY_SIZE(findPathBuf) );
		ycAssert( last <= MAX_PATH - 3 );
		findPathBuf[ last+0 ] = '/';
		findPathBuf[ last+1 ] = '*';
		findPathBuf[ last+2 ] = '\0';
	}
	HANDLE find = FindFirstFileW( findPathBuf, &findData );
	if( find == INVALID_HANDLE_VALUE ) { return false; }
	do
	{
		const ureg curItemFilter = ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? ycDir::kFilter_Directory : ycDir::kFilter_File ;
		if( (curItemFilter&filter) == 0 ) { continue; }
		if(
			findData.cFileName[0] == '.' &&
			(
				( findData.cFileName[1] == '\0' ) || ( findData.cFileName[1] == '.' && findData.cFileName[2] == '\0' )
			)
		)
		{
			continue;
		}
		if( (filter & kFilter_NoExclamation) != 0 && findData.cFileName[0] == '!' )
		{
			continue;
		}
		if( dst->IsFull() ) { dst->Reserve( dst->GetCapacity()*2 ); }
		ycDir::FileDesc* fileDesc = dst->Append();
		fileDesc->isDir = (findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0;
		fileDesc->path.Set( src );
		if( fileDesc->path.Last() != '/' ) { fileDesc->path.Append( '/' ); }
		Utf16To8( findData.cFileName, &fileDesc->path, true );
		if( flags & kFlag_GetModifiedTimes )
		{
			LARGE_INTEGER mtime;
			mtime.LowPart  = findData.ftLastWriteTime.dwLowDateTime;
			mtime.HighPart = findData.ftLastWriteTime.dwHighDateTime;
			fileDesc->mtime = ycFileTime_t( mtime.QuadPart );
		}
		else
		{
			fileDesc->mtime = 0;
		}
		if( curItemFilter & ycDir::kFilter_Directory )
		{
			fileDesc->size  = 0;
			if( flags&ycDir::kFlag_Recursive )
			{
				ReadDir_Recursive( fileDesc->path, dst, filter, flags ); // TODO HACK FIXME: ignores failure!
				// do not access fileDesc after this! ReadDir_Recursive could've moved it!
			}
		}
		else
		{
			LARGE_INTEGER fileSize;
			fileSize.LowPart  = findData.nFileSizeLow;
			fileSize.HighPart = findData.nFileSizeHigh;
			ycAssert( fileSize.QuadPart >= 0 );
			fileDesc->size = ycFileSize_t( fileSize.QuadPart );
		}
	}
	while( FindNextFileW( find, &findData ) != 0 );
	const bool success = GetLastError() == ERROR_NO_MORE_FILES;
	FindClose( find );
	return success;
}
#endif // YC_MSFT

#ifdef YC_NDA
#endif 

#ifdef YC_NDA
#endif 

#if YC_NDA
#endif 

#if defined YC_NIX || defined YC_OSX
/*static*/ bool ycDir::ReadDir_Recursive( const ycStringRef src, ycVector< ycDir::FileDesc >* dst, const ureg filter, const ureg flags )
{
	DIR* dh = opendir( src.Get() );
	if( dh == nullptr ) { return false; }
	for(;;)
	{
		dirent* entry = readdir( dh );
		if( entry == nullptr ) { break; }

		ycStringRef fileName( (char*)entry->d_name ); // cast to avoid calling into the fixed-size-array template
		//const char* fileName = entry->d_name;
		if( fileName.Equals( ".", 1 ) || fileName.Equals( "..", 2 ) ) { continue; }
			
		struct stat statBuf;

		ycStackString<> subPath( src );
		if( subPath.Last() != '/' ) { subPath.Append( '/' ); }
		subPath.Append( fileName );
		const int ret = stat( subPath.ToCStr(), &statBuf );
		ycAssert( ret == 0 );

		const ureg curItemFilter = ( (statBuf.st_mode&S_IFMT) == S_IFDIR ) ? ycDir::kFilter_Directory : ycDir::kFilter_File ;
		if( (curItemFilter&filter) == 0 ) { continue; }

		if( (filter&kFilter_NoExclamation) != 0 && fileName.GetPtr()[0] == '!' )
		{
			continue;
		}

		if( dst->IsFull() ) { dst->Reserve( dst->GetCapacity()*2 ); }
		ycDir::FileDesc* fileDesc = dst->Append();
		fileDesc->isDir = ( curItemFilter & ycDir::kFilter_Directory ) != 0;
		fileDesc->path.Set( subPath );

		if( curItemFilter & ycDir::kFilter_Directory )
		{
			fileDesc->size  = 0;
			fileDesc->mtime = 0;
			if( flags&ycDir::kFlag_Recursive )
			{
				if( !ReadDir_Recursive( fileDesc->path, dst, filter, flags ) )
				{
					closedir( dh );
					return false;
				}
				#ifdef YC_ENABLE_ASSERT // do not access fileDesc after this! ReadDir_Recursive could've moved it!
					fileDesc = nullptr;
				#endif
			}
		}
		else
		{
			fileDesc->size = ycFileSize_t( statBuf.st_size );
			if( flags & kFlag_GetModifiedTimes )
			{
				#ifdef YC_FILE_CONVERT_UNIX_MTIME_TO_MSFT
					#define NT_OFFSET_TO_UNIX_EPOCH_SECONDS u64(11644473600)
					fileDesc->mtime = ( (statBuf.st_mtim.tv_sec+NT_OFFSET_TO_UNIX_EPOCH_SECONDS) * 10000000 ) + ( statBuf.st_mtim.tv_nsec / 100 );
				#else
					fileDesc->mtime = ycFileTime_t( statBuf.st_mtime );
				#endif
			}
			else
			{
				fileDesc->mtime = 0;
			}
		}
	}
	closedir( dh );
	//if( buf.pPtr ) { YC_FREE( g_defaultMem, buf.pPtr ); }
	return true;// ret == 0;
}
#endif // YC_NIX

bool ycDir::ReadDir( ycVector< FileDesc >* dst, const ureg filter, const ureg flags )
{
	if( flags&kFlag_Recursive ) { ycAssert( filter&kFilter_Directory ); } // can't recurse if we skip directories; TODO: detangle the 'add to list' and 'recursive if directory' code!
	#if defined YC_MSFT
		return ReadDir_Recursive( m_path, dst, filter, flags );
	#elif YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX
		return ReadDir_Recursive( m_path, dst, filter, flags );
	#elif defined YC_OSX
		bool success = ReadDir_Recursive( m_path, dst, filter, flags );
		if( success )
		{
			// sort files for parity with windows
			ycSort::QuickSort( dst->GetData(), dst->Length(), []( const ycDir::FileDesc& lhs, const ycDir::FileDesc& rhs ) { return lhs.path.CompareI( rhs.path.Get() ); } );
		}
		return success;
	#else
		YC_UNUSED( dst );
		ycFatal();
		return false;
	#endif
}

/*static*/ bool ycDir::SetCurrentNextTo( const ycStringRef& findFile )
{
#if defined YC_MSFT || defined YC_NIX || defined YC_OSX
	ycStackString<> tmp;
	bool findDir = ( findFile.Last() == '/' );

	GetCwd( &tmp );
	CleanupPath( &tmp );
	if( tmp.Last() != '/' ) { tmp.Append( '/' ); }

	for(;;)
	{
		const ycSize_t pathLen = tmp.Length();
		tmp.Append( findFile );
		const bool found = findDir ? ycDir::Exists( tmp ) : ycFile::Exists( tmp );
		tmp.SetLength( pathLen ); // undo the append
		if( found )
		{
			ycDir::SetCurrent( tmp );
			return true;
		}
		// didn't find, back up a directory
		ycAssert( tmp.Last() == '/' );
		do
		{
			tmp.TakeLast();
			if( tmp.Length() == 0 ) { return false; }
		} while( tmp.Last() != '/' );
	}
#else
	YC_UNUSED( findFile );
	ycFatal();
	return false;
#endif
}

/*static*/ bool ycDir::SetCurrent( const ycStringRef& path )
{
	ycStackString< YC_MAX_PATH > pathBuf( path );
	#if defined YC_MSFT
		const bool ret = SetCurrentDirectoryW( Utf8To16(path) ) == TRUE;
	#elif defined YC_NIX || defined YC_OSX
		const bool ret = chdir( pathBuf.ToCStr() ) == 0;
	#else
		YC_UNUSED( pathBuf );
		ycFatal();
		const bool ret = false;
	#endif

	// cache the result
	if( s_cwd.GetAllocator() == nullptr ) { s_cwd.SetAllocator( ycString::GetDefaultAllocator() ); } // init allocator since this is a global var that may not have gotten one when it initialized
	GetCwd( &s_cwd );
	CleanupPath( &s_cwd );

	return ret;
}

/*static*/ ycStringRef ycDir::GetCurrent()
{
	return s_cwd;
}

/*static*/ void ycDir::CleanupPath( ycString* path, const bool toLower )
{
	// should be safe since it can only shorten the string
	const ycSize_t len = CleanupPath( const_cast< char* >( path->Get() ), toLower );
	path->SetLength( len );
}

/*static*/ ycSize_t ycDir::CleanupPath( char* path, const bool toLower )
{
	// special case network path, don't try to clean up double initial slashes, but force forward
	if( ( path[0] == '/' && path[1] == '/' ) || ( path[0] == '\\' && path[1] == '\\' ) )
	{
		path[0] = '/';
		path[1] = '/';
		path += 2;
	}

	const char* read = path;
	char* dest = path;
	while( *read )
	{
		char c = *read;
		while( (c == '\\' || c=='/' ) && ( read[1] == '\\' || read[1] == '/' ) ) { c = *++read; }
		if( c=='\\' ) { c='/'; }
		if( c=='/' && read[1]=='.' && read[2]=='.' && dest > path )
		{
			read += 2;
			while( dest > path )
			{
				--dest;
				if( *dest=='/' ) { break; }
			}
			// special case for going up to the root folder to avoid first character of path being '/' unintentionally
			if( dest == path && *read && ( read[1] == '/' || read[1] == '\\' ) )
			{
				++read;
			}
		}
		else
		{
			*dest++ = (!toLower) ? c : ycStringUtils::ToLower( c ) ;
		}
		++read;
	}
	*dest = 0;
	return dest - path;
}

/*static*/ const char* ycDir::RelativePath( char *dest, ycSize_t destSize, const char* absolute, const char* root )
{
	if( !root ) { return absolute; }

	const char* chk = root;
	const char* src = absolute;
	char* dst = dest;
	const char* end = dest + destSize;

	// as long as absolute and root are identical that part of the path can be skipped.
	// assume case insensitivity and forward-backward slash insensitivity
	while( *src && *chk )
	{
		char a = *src;
		char b = *chk;
		if( !( ( a=='/' || a=='\\' ) && ( b=='/' || b=='\\' ) ) && ycStringUtils::ToLower( a ) != ycStringUtils::ToLower( b ) )
		{
			break;
		}
		++src;
		++chk;
	}
	// assuming root is a folder there may be another slash that can be ignored
	if( !*chk && ( *src == '/' || *src == '\\' ) ) { ++src; }

	while( *src && dst<end ) { *dst++ = *src++; }
	if( dst == end ) { return nullptr; }
	if( dst > dest && ( dst[-1] == '/' || dst[-1] == '\\' ) ) { --dst; }
	*dst++ = 0;
	CleanupPath( dest );

	// check how far root & absolute path matches
	dst = dest;
	src = dst;
	u32 num_up = 0;
	if( *chk != 0 && chk[1]!=0 && *chk!='/' && *chk!='\\' )
	{
		++num_up;
		// count how many folders up needs to be traversed
		while( char c = *chk++ )
		{
			if( *chk && ( c == '/' || c=='\\' ) ) { num_up++; }
		}
	}
	// may need to push out the relative path if too many up folders
	if( ycSize_t( src-dest ) < ycSize_t( 3 * num_up ) )
	{
		char* srcup = dst;
		while( *srcup ) { ++srcup; }
		char* dstup = srcup - ( src-dest ) + ( 3 * num_up );
		if( dstup>=end ) { return nullptr; }
		*dstup = 0;
		while( srcup > dst ) { *--dstup = *--srcup; }
		src = dstup;
	}
	// insert ../ num_up times
	dst = dest;
	for( u32 u=0; u<num_up; ++u ) { *dst++='.'; *dst++='.'; *dst++='/'; }
	// copy down from src
	if( src != dst )
	{
		while( *src ) { *dst++ = *src++; }
		*dst = 0;
	}
	return dest;
}

/*static*/ ycString ycDir::GetCanonicalPathCase( const char* path )
{
	ycString canonical( path );
#if defined YC_MSFT
	HANDLE fh = CreateFileW( Utf8To16(path), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		wchar_t canonicalPath16[ YC_MAX_PATH ];
		GetFinalPathNameByHandleW( fh, canonicalPath16, YC_MAX_PATH, FILE_NAME_OPENED );
		ycStackString<> tmp;
		Utf16To8( canonicalPath16, &tmp, false );
		CloseHandle( fh );
		tmp.Take( "\\\\?\\", 4 );
		char canonicalRelative[YC_MAX_PATH];
		RelativePath( canonicalRelative, YC_MAX_PATH, tmp.ToCStr(), GetCurrent().Get() );
		CleanupPath( canonicalRelative );
		canonical.Set( canonicalRelative );
	}
#endif
	return canonical;
}

#if !defined YC_OSX || defined YC_EDITOR

/*static*/ bool ycDir::MakePath( ycStringRef path )
{
	const ycSize_t srcLen = path.Length();
	ycAssert( srcLen < YC_MAX_PATH );
	char cleanPath[ YC_MAX_PATH ];
	ycMemCpy( cleanPath, path.GetPtr(), srcLen );
	cleanPath[ srcLen ] = 0;
	CleanupPath( cleanPath );

	char newPath[ YC_MAX_PATH ];
	ureg off = 0;
	for(;;)
	{
		ycAssert( off < sizeof(newPath) );
		const char ch = cleanPath[off];
		if( off && ( ch == '\0' || ch == '/' || ch == '\\' ) )
		{
			newPath[ off ] = '\0';
			if( !Exists( newPath ) )
			{
			#if defined YC_MSFT
				if( CreateDirectoryW( Utf8To16(newPath), nullptr ) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS )
				{
					return false;
				}
			#elif YC_NDA
			#elif YC_NDA
			#elif defined YC_NIX || defined YC_OSX
				if( mkdir( newPath, 0777 ) != 0 && errno != EEXIST )
				{
					return false;
				}
			#else
				ycFatal(); // NYI
			#endif
			}
			if( ch == '\0' ) { break; }
		}
		newPath[ off ] = ch;
		++off;
	}
	return true;
}

#endif

#if !defined YC_OSX
/*static*/ bool ycDir::Copy( const ycStringRef& srcPath, const ycStringRef& dstPath )
{
#if defined YC_MSFT
	#ifdef YC_NDA
	#else
		if( !ycDir::Exists( srcPath ) )
		{
			return false;
		}

		ycStackString<> tmp( srcPath );
		if( PathIsRelativeA( tmp.ToCStr() ) ) // hopefully this only actually analyzes the beginning of the string, so that utf8 wont matter
		{
			tmp.Set( ycDir::GetCurrent() );
			tmp.Append( "/" );
			tmp.Append( srcPath );
		}
		tmp.Append( "/*" );
		tmp.Replace( '/', '\\' ); // FO_COPY appears to require backslashes
		Utf8To16 src16( tmp.ToCStr() );

		tmp.Set( dstPath );
		if( PathIsRelativeA( tmp.ToCStr() ) )
		{
			tmp.Set( ycDir::GetCurrent() );
			tmp.Append( "/" );
			tmp.Append( dstPath );
		}
		tmp.Replace( '/', '\\' ); // FO_COPY appears to require backslashes
		Utf8To16 dst16( tmp.ToCStr() );

		src16.buf[ wcslen(src16.buf)+1 ] = L'\0'; // SHFileOperation requires double NUL termination!
		dst16.buf[ wcslen(dst16.buf)+1 ] = L'\0';

		SHFILEOPSTRUCTW s = { 0 };
		s.wFunc = FO_COPY;
		s.pFrom = src16;
		s.pTo   = dst16;
		s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
		int res = SHFileOperationW( &s );
		return res == 0;
	#endif
#else
	YC_UNUSED( srcPath );
	YC_UNUSED( dstPath );
	return false;
#endif
}
#endif

/*static*/ bool ycDir::Delete( const ycStringRef& srcPath )
{
	return ycFile::Delete( srcPath );
}

/*static*/ const char* ycDir::GetDataPath()
{
#if YC_NDA
#elif YC_NDA
#elif defined YC_MSFT || defined YC_NIX
	return "build/PC/data/";
#elif YC_NDA
#elif YC_NDA
#elif defined YC_OSX
	return "build/OSX/data/";
#elif YC_NDA
#else
	#error ycDir::GetDataPath() has no implementation!
	return nullptr;
#endif
}

/*static*/ const char* ycDir::GetEngineSavePath()
{
#if defined YC_SDL_MAIN
	if( !s_savePath[0] )
	{
		char* savePath = SDL_GetPrefPath( "Yacht Club Games", "Propeller" );
		ycAssertMsg( savePath, "Creating save directory failed: %s", SDL_GetError() );
		ycStringUtils::Copy( s_savePath, savePath, kMaxSavePath );
		SDL_free( savePath );
	}
	return s_savePath;
#else
	return nullptr;
#endif
}
