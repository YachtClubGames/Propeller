
#include "ycFile.h"

#if defined YC_FILESYSTEM_WIN32

#include "ycDateTime.h"
#include "ycDir.h"
#include "ycStackString.h"
#include "ycStringUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h> // PathIsRelative

#if YC_NDA
#endif

namespace
{
	constexpr uintptr_t kInvalidFh = 0;

	static void CheckPathCase( const char* path, HANDLE handle )
	{
		// TODO(jstpierre): wide char version of this?
#ifdef CHECK_CASE
		// FILE_FLAG_POSIX_SEMANTICS may be able to do this more simply
		char canonicalPath[ YC_MAX_PATH ];
		ycSize_t len = GetFinalPathNameByHandleA( handle, canonicalPath, YC_MAX_PATH, FILE_NAME_OPENED );
		bool passed = true;
		char * canonical = canonicalPath + ( len - 1 );
		const char * check = path + ( ycStringUtils::Length( path ) - 1 );
		while( check >= path && canonical >= canonicalPath )
		{
			if( *canonical == '\\' && *check != '\\' ) { *canonical = '/'; }
			if( check[ 0 ] == ':' && ( check[ 1 ] == '/' || check[ 1 ] == '\\' ) ) // stop checking if we've hit :/
			{
				break;
			}
			if( *canonical != *check )
			{
				if( !( ( *check == ':' || *check == '.' ) && ( *( check + 1 ) == '/' || *( check + 1 ) == '\\' ) ) )  // stop checking if there's a '../' in the path
				{
					passed = false;
				}
				break;
			}
			--canonical;
			--check;
		}
		if( !passed )
		{
			canonical = canonicalPath;
			if( ycStringUtils::BeginsWith( canonicalPath, "\\\\?\\" ) ) { canonical += 4; }
			char relativePath[ YC_MAX_PATH ];
			ycDir::RelativePath( relativePath, YC_MAX_PATH, canonical, ycDir::GetCurrent().Get() );
			ycAssertMsg( false, "File case mismatch!\nInput path:   %s\nCorrect path: %s", path, relativePath );
		}
#else
		YC_UNUSED( path );
		YC_UNUSED( handle );
#endif
	}

	struct Utf8To16
	{
		Utf8To16( const char* str ) { MultiByteToWideChar( CP_UTF8, 0, str, -1, buf, YC_ARRAY_SIZE(buf) ); }
		wchar_t buf[ YC_MAX_PATH ] = {};
		operator LPCWSTR() { return buf; }
	};
}

#define YC_FILE_LOG( ... ) //ycLog( "ycFile: " __VA_ARGS__ )

/*static*/ u8 * ycFile::Read( const char * fileName, ycAllocator * mem, ycSize_t * outFileSize/* = nullptr*/ )
{
	YC_FILE_LOG( "Read( %s )\n", fileName );

	u8 * out = nullptr;

	HANDLE fh = CreateFileW( Utf8To16(fileName), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		BY_HANDLE_FILE_INFORMATION fileInfo;

		CheckPathCase( fileName, fh );

		BOOL ret = GetFileInformationByHandle( fh, &fileInfo );
		if( ret )
		{
			ycAssert( fileInfo.nFileSizeHigh == 0 ); // reading larger files requires multiple ReadFile calls or using OVERLAPPED to pass in a 64 bit size, if you're reading this, you've been nominated to implement that path
			const ycSize_t fileSize = ( u64( fileInfo.nFileSizeHigh ) << 32 ) | u64( fileInfo.nFileSizeLow );
			if( outFileSize ) { *outFileSize = fileSize; }
			ycSize_t allocSize = fileSize;
			if( allocSize == 0 ) { allocSize = 1; }
			out = (u8*)YC_ALLOC( mem, allocSize, YC_MEM_DEFAULT_ALIGN, ycAllocInfo( fileName, true ) );
			out[ 0 ] = '\0';
			DWORD bytesRead;
			ret = ReadFile( fh, out, DWORD( fileSize ), &bytesRead, nullptr );
			ycAssert( ret && bytesRead == fileSize );
		}

		CloseHandle( fh );
	}

	return out;
}

/*static*/ bool ycFile::Exists( const char * fileName )
{
	YC_FILE_LOG( "Exists( %s )\n", fileName );
	DWORD attrib = GetFileAttributesW( Utf8To16(fileName) );
	return attrib != INVALID_FILE_ATTRIBUTES;
}

/*static*/ bool ycFile::GetSize( const char * fileName, ycSize_t * fileSize )
{
	YC_FILE_LOG( "GetSize( %s )\n", fileName );
	bool out = false;
	HANDLE fh = CreateFileW( Utf8To16(fileName), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		BY_HANDLE_FILE_INFORMATION fileInfo;
		BOOL ret = GetFileInformationByHandle( fh, &fileInfo );
		if( ret )
		{
			*fileSize = (u64(fileInfo.nFileSizeHigh)<<32) | fileInfo.nFileSizeLow;
			out = true;
		}
		CloseHandle( fh );
	}

	return out;
}

/*static*/ bool ycFile::GetModifiedTime( const char * fileName, ycFileTime_t * mtime )
{
	YC_FILE_LOG( "GetModifiedTime( %s )\n", fileName );
	bool out = false;
	HANDLE fh = CreateFileW( Utf8To16(fileName), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		BY_HANDLE_FILE_INFORMATION fileInfo;
		BOOL ret = GetFileInformationByHandle( fh, &fileInfo );
		if( ret )
		{
			*mtime = ycDateTime::ConvertFromFILETIME( fileInfo.ftLastWriteTime );
			out = true;
		}
		CloseHandle( fh );
	}
	return out;
}

/*static*/ bool ycFile::Write( const char * fileName, const void * src, const ycSize_t len )
{
	YC_FILE_LOG( "Write( %s )\n", fileName );
	bool out = false;
	Utf8To16 fileName16( fileName );
	MoveFileW( fileName16, fileName16 ); // fix case on filename (eg rename FOO.TXT -> foo.txt)
	HANDLE fh = CreateFileW( fileName16, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		ycAssert( len <= 0xffffffffu ); // could do larger w/ OVERLAPPED
		DWORD bytesWritten;
		const BOOL ret = WriteFile( fh, src, DWORD( len ), &bytesWritten, nullptr );
		ycAssertMsg( ret && bytesWritten == len, "Error writing file: '%s'.", fileName );
		out = true;
		CloseHandle( fh );
	}
	return out;
}

/*static*/ bool ycFile::Write( const char * fileName, const ureg numChunks, const u8 ** chunks, const ycSize_t * chunkSizes )
{
	YC_FILE_LOG( "Write( %s )\n", fileName );
	bool out = false;
	Utf8To16 fileName16( fileName );
	MoveFileW( fileName16, fileName16 ); // fix case on filename (eg rename FOO.TXT -> foo.txt)
	HANDLE fh = CreateFileW( fileName16, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		for( ureg i = 0; i != numChunks; ++i )
		{
			ycAssert( chunkSizes[ i ] <= 0xffffffffu ); // could do larger w/ OVERLAPPED
			DWORD bytesWritten;
			const BOOL ret = WriteFile( fh, chunks[ i ], DWORD( chunkSizes[ i ] ), &bytesWritten, nullptr );
			if( ret == FALSE || bytesWritten == chunkSizes[ i ] )
			{
				out = false;
				break;
			}
		}
		CloseHandle( fh );
	}
	return out;
}

/*static*/ bool ycFile::Copy( const char* srcPath, const char* dstPath )
{
	YC_FILE_LOG( "Copy( %s -> %s )\n", srcPath, dstPath );
	const BOOL ret = CopyFileW( Utf8To16(srcPath), Utf8To16(dstPath), FALSE );
	return ret == TRUE;
}

/*static*/ bool ycFile::Delete( const char* srcPath )
{
#ifdef  YC_NDA
#else
	const char* src = srcPath;
	ycStackString<> tmp;
	if( PathIsRelativeA( srcPath ) ) // hopefully this only actually analyzes the beginning of the string, so that utf8 wont matter
	{
		tmp.Set( ycDir::GetCurrent() );
		tmp.Append( "/" );
		tmp.Append( srcPath );
		src = tmp.ToCStr();
	}
	Utf8To16 fileName16( src );
	fileName16.buf[ wcslen(fileName16.buf)+1 ] = L'\0'; // SHFileOperation requires double NUL termination!

	// source:	https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shfileoperationa
	SHFILEOPSTRUCTW shFileOp = {};
	shFileOp.wFunc = FO_DELETE;
	shFileOp.pFrom = fileName16;
	shFileOp.fFlags = /*FOF_ALLOWUNDO |*/ FOF_NOERRORUI | FOF_NORECURSION | FOF_NOCONFIRMMKDIR | FOF_SILENT | FOF_NOCONFIRMATION;
	shFileOp.fAnyOperationsAborted = TRUE;
	int res = SHFileOperationW( &shFileOp );
	return res == 0;
#endif
}

/*static*/ bool ycFile::SetReadOnly( const char* srcPath, bool ro )
{
	Utf8To16 fileName16( srcPath );
	DWORD attrib = GetFileAttributesW( fileName16 );
	if( ro )
	{
		if( attrib & FILE_ATTRIBUTE_READONLY ) { return true; }
		attrib |= FILE_ATTRIBUTE_READONLY;
	}
	else
	{
		if( attrib & FILE_ATTRIBUTE_READONLY ) { return true; }
		attrib &= ~FILE_ATTRIBUTE_READONLY;
	}
	SetFileAttributesW( fileName16, attrib );
	return true;
}

/*static*/ void ycFile::Init()
{
}

//////////
// Reader
//////////

ycFileReader::ycFileReader()
{
	m_file = kInvalidFh;
}

ycFileReader::~ycFileReader()
{
	if( m_file != kInvalidFh ) { Close(); }
}

bool ycFileReader::Open( const char* fileName )
{
	ycAssert( m_file == kInvalidFh );
	YC_FILE_LOG( "FileReader::Open( %s )\n", fileName );
	HANDLE fh = CreateFileW( Utf8To16(fileName), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		CheckPathCase( fileName, fh );
		ycAssert( fh ); // some code assumes !=0, could be fixed if necessary
		m_file = uintptr_t( fh );
		m_offset = 0;
		return true;
	}
	return false;
}

bool ycFileReader::IsOpen() const
{
	return m_file != kInvalidFh;
}

ycSize_t ycFileReader::GetSize()
{
	ycAssert( m_file != kInvalidFh );
	BY_HANDLE_FILE_INFORMATION fileInfo;
	const BOOL ret = GetFileInformationByHandle( HANDLE( m_file ), &fileInfo );
	return ret ? ( (u64(fileInfo.nFileSizeHigh)<<32) | u64(fileInfo.nFileSizeLow) ) : 0;
}

void ycFileReader::Read( void * dst, const ycSize_t desiredBytes, ycSize_t * readBytes )
{
	ycAssert( desiredBytes <= 0xffffffffu ); // could break up across multiple reads / use OVERLAPPED
	DWORD bytesRead;
	const BOOL ret = ReadFile( HANDLE( m_file ), dst, DWORD( desiredBytes ), &bytesRead, nullptr );
	ycAssert( ret );
	*readBytes = bytesRead;
	m_offset += bytesRead;
}

bool ycFileReader::Read( void * dst, const ycSize_t desiredBytes )
{
	ycSize_t bytesRead;
	Read( dst, desiredBytes, &bytesRead );
	return bytesRead == desiredBytes;
}

void ycFileReader::Close()
{
	ycAssert( m_file != kInvalidFh );
	CloseHandle( HANDLE( m_file ) );
	m_file = kInvalidFh;
}

void ycFileReader::SetPos( const ycSize_t offset )
{
	ycAssert( m_file != kInvalidFh );
	LARGE_INTEGER offs;
	offs.QuadPart = offset;
	const BOOL ret = SetFilePointerEx( HANDLE( m_file ), offs, nullptr, FILE_BEGIN );
	ycAssert( ret );
	m_offset = offset;
}

ycSize_t ycFileReader::GetPos() const
{
	return m_offset;
}

//////////
// Writer
//////////

ycFileWriter::ycFileWriter()
{
	m_file = kInvalidFh;
}

ycFileWriter::~ycFileWriter()
{
	if( m_file != kInvalidFh ) { Close(); }
}

bool ycFileWriter::Open( const char* fileName, const ureg flags )
{
	ycAssert( m_file == kInvalidFh );
	YC_FILE_LOG( "FileWriter::Open( %s )\n", fileName );
	HANDLE fh = CreateFileW( Utf8To16(fileName), ( flags & kAppend ) ? FILE_APPEND_DATA : GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, 0 );
	if( fh != INVALID_HANDLE_VALUE )
	{
		ycAssert( fh ); // some code assumes !=0, could be fixed if necessary
		m_file = uintptr_t( fh );
		m_offset = 0;
		return true;
	}
	return false;
}

bool ycFileWriter::IsOpen() const
{
	return m_file != kInvalidFh;
}

bool ycFileWriter::Write( const void * src, const ycSize_t numBytes )
{
	ycAssert( m_file != kInvalidFh );
	ycAssert( numBytes <= 0xffffffffu ); // could break up across multiple writes / use OVERLAPPED
	DWORD bytesWritten;
	const BOOL ret = WriteFile( HANDLE( m_file ), src, DWORD( numBytes ), &bytesWritten, nullptr );
	return ret && bytesWritten == numBytes;
}

void ycFileWriter::Close()
{
	ycAssert( m_file != kInvalidFh );
	CloseHandle( HANDLE( m_file ) );
	m_file = kInvalidFh;
}

void ycFileWriter::SetPos( const ycSize_t offset )
{
	ycAssert( m_file != kInvalidFh );
	LARGE_INTEGER offs;
	offs.QuadPart = offset;
	const BOOL ret = SetFilePointerEx( HANDLE( m_file ), offs, nullptr, FILE_BEGIN );
	ycAssert( ret );
	m_offset = offset;
}

ycSize_t ycFileWriter::GetPos() const
{
	return m_offset;
}

#endif
