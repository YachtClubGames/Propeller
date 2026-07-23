
#include "ycFile.h"

#if defined YC_FILESYSTEM_CSTD

// std
#include <stdio.h>

// sdk
#if defined YC_NIX || defined YC_OSX
	#if defined __FreeBSD__
		#include <sys/stat.h>
		#include <stdlib.h>
	#else
		#include <sys/types.h>
		#include <sys/stat.h>
		#include <fcntl.h>
		#include <unistd.h>
		#if defined __APPLE__
			#include <copyfile.h>
		#else
			#include <sys/sendfile.h>
		#endif
	#endif
	#define _XOPEN_SOURCE 700 // min 500 for nftw()
	#include <ftw.h>
#endif

// core
#include "ycAllocator.h"
#include "ycDir.h"
#include "ycStackString.h"
#include "ycStd.h"
#include "ycStringUtils.h"

namespace
{
	constexpr uintptr_t kInvalidFh = 0;
	#define FILEMODESTR_READ "rb"
	#define FILEMODESTR_WRITE "wb"
	#define FILEMODESTR_APPEND "ab"
}

//////////
// Utilities
//////////

#define YC_FILE_LOG( ... ) //ycLog( "ycFile: " __VA_ARGS__ )

/*static*/ u8* ycFile::Read( const char* fileName, ycAllocator* mem, ycSize_t* outFileSize/* = nullptr*/ )
{
	YC_FILE_LOG( "Read( %s )\n", fileName );

	#if defined YC_USE_FOPEN_S
		FILE* fh = nullptr;
		fopen_s( &fh, fileName, FILEMODESTR_READ );
	#else
		FILE* fh = fopen( fileName, FILEMODESTR_READ );
	#endif

	if( fh )
	{
		fseek( fh, 0, SEEK_END );
		const long int offset = ftell( fh );
		if( offset != -1 )
		{
			if( outFileSize ) { *outFileSize = ycSize_t(offset); }
			u8* buf = ( u8* )YC_ALLOC( mem, offset == 0 ? 1 : ycSize_t(offset), YC_MEM_DEFAULT_ALIGN, ycAllocInfo(fileName, true) );
			fseek( fh, 0, SEEK_SET );
			fread( buf, size_t(offset), 1, fh );
			fclose( fh );
			return buf;
		}
		fclose( fh );
		return nullptr;
	}
	return nullptr;
}

/*static*/ bool ycFile::Exists( const char* fileName )
{
	YC_FILE_LOG( "Exists( %s )\n", fileName );

	#if defined YC_NIX || defined YC_OSX
		struct stat unused;
		return stat( fileName, &unused ) == 0 ;
	#else
		ycWarnOnce( "Using ycFile::Exists() fallback implementation, this returns false on directories!" );
		#ifdef YC_USE_FOPEN_S
			FILE* fh = nullptr;
			const errno_t ret = fopen_s( &fh, fileName, FILEMODESTR_READ );
		#else
			FILE* fh = fopen( fileName, FILEMODESTR_READ );
		#endif
		if( fh )
		{
			fclose( fh );
			return true;
		}
		return false;
	#endif
}

/*static*/ bool ycFile::GetSize( const char* fileName, ycSize_t* fileSize )
{
	YC_FILE_LOG( "GetSize( %s )\n", fileName );

	#if defined YC_NIX || defined YC_OSX
		struct stat statData;
		if( stat( fileName, &statData ) == 0 )
		{
			ycAssert( statData.st_mtime >= 0 );
			*fileSize = ycSize_t( statData.st_size );
			return true;
		}
	#else
		#ifdef YC_USE_FOPEN_S
			FILE* fh = nullptr;
			const errno_t ret = fopen_s( &fh, fileName, FILEMODESTR_READ );
		#else
			FILE* fh = fopen( fileName, FILEMODESTR_READ );
		#endif
		if( fh )
		{
			fseek( fh, 0, SEEK_END );
			const long int offset = ftell( fh );
			fclose( fh );
			if( offset != -1 )
			{
				*fileSize = ycSize_t( offset );
				return true;
			}
		}
	#endif
	return false;
}

/*static*/ bool ycFile::GetModifiedTime( const char* fileName, ycFileTime_t* mtime )
{
	YC_FILE_LOG( "GetModifiedTime( %s )\n", fileName );

	#if defined YC_NIX || defined YC_OSX
		struct stat statData;
		if( stat( fileName, &statData ) == 0 )
		{
			ycAssert( statData.st_mtime >= 0 );
			*mtime = u64( statData.st_mtime );
			return true;
		}
		return false;
	#else
		ycFatal();
		return false;
	#endif
}

/*static*/ bool ycFile::Write( const char* fileName, const void* src, const ycSize_t len )
{
	YC_FILE_LOG( "Write( %s )\n", fileName );

	#ifdef YC_USE_FOPEN_S
		FILE* fh = nullptr;
		fopen_s( &fh, fileName, FILEMODESTR_WRITE );
	#else
		FILE* fh = fopen( fileName, FILEMODESTR_WRITE );
	#endif
	if( fh )
	{
		const size_t elementsWritten = fwrite( src, len, 1, fh );
		fclose( fh );
		return elementsWritten == 1;
	}
	return false;
}

/*static*/ bool ycFile::Write( const char* fileName, const ureg numChunks, const u8** chunks, const ycSize_t* chunkSizes )
{
	YC_FILE_LOG( "Write( %s )\n", fileName );

	#ifdef YC_USE_FOPEN_S
		FILE* fh = nullptr;
		fopen_s( &fh, fileName, FILEMODESTR_WRITE );
	#else
		FILE* fh = fopen( fileName, FILEMODESTR_WRITE );
	#endif
	if( fh )
	{
		for( ureg i = 0; i != numChunks; ++i )
		{
			const size_t elementsWritten = fwrite( chunks[i], chunkSizes[i], 1, fh );
			if( elementsWritten != 1 )
			{
				fclose( fh );
				return false;
			}
		}
		fclose( fh );
		return true;
	}
	return false;
}

/*static*/ bool ycFile::Copy( const char* srcPath, const char* dstPath )
{
	YC_FILE_LOG( "Copy( %s -> %s )\n", srcPath, dstPath );

	#if defined YC_NIX || defined YC_OSX
		#if defined __FreeBSD__
			// freebsd does have copy_file_range https://man.freebsd.org/cgi/man.cgi?copy_file_range
			enum { kBufSize = 128*1024 };
			u8* buf = (u8*)malloc( kBufSize );
			FILE* srcFh = fopen( srcPath, FILEMODESTR_READ );
			if( srcFh == nullptr ) { free( buf ); return false; }
			FILE* dstFh = fopen( srcPath, FILEMODESTR_WRITE );
			if( dstFh == nullptr ) { fclose( srcFh ); free( buf ); return false; }
			fseek( srcFh, 0, SEEK_END );
			long int remain = ftell( srcFh );
			fseek( srcFh, 0, SEEK_SET );
			while( remain )
			{
				const long int chunk = remain > kBufSize ? kBufSize : remain ;
				remain -= chunk;
				fread( buf, 1, chunk, srcFh );
				fwrite( buf, 1, chunk, dstFh );
			}
			fclose( srcFh );
			fclose( dstFh );
			free( buf );
		#else
			int src, dst;
			src = open( srcPath, O_RDONLY );
			if( src == -1 ) { return false; }
			dst = creat( dstPath, 0660 );
			if( dst == -1 ) { close( src ); return false; }
			#if defined __APPLE__
				const bool copied = fcopyfile( src, dst, 0, COPYFILE_ALL ) == 0;
			#else
				// only works on linux kernel 2.6.33+
				off_t bytesCopied = 0;
				struct stat fileinfo = {0};
				fstat( src, &fileinfo );
				// should probably be using copy_file_range instead
				const bool copied = sendfile( dst, src, &bytesCopied, fileinfo.st_size ) == fileinfo.st_size;
			#endif
			close( src );
			close( dst );
			return copied;
		#endif
	#else
		// don't #else a simple fopen/std based fallback! that is certainly slower than a platform's optimal copy, and this is used a lot! if a simple implementation like that is necessary, make it #elif opt-in!
		ycFatal();
		return false;
	#endif
}

#ifndef YC_OSX
namespace
{
	int DeleteRecursive( const char* path, const struct stat*, int flag, struct FTW* )
	{
		int ret = ( flag == FTW_DP || flag == FTW_D ) ? rmdir( path ) : unlink( path );
		return ret == 0 ? 0 : -1 ;
	}
}
/*static*/ bool ycFile::Delete( const char* srcPath )
{
	struct stat statData;
	if( stat( srcPath, &statData ) != 0 )
	{
		return false;
	}
	if( S_ISDIR(statData.st_mode) )
	{
		int flags = FTW_DEPTH | FTW_PHYS; // recurse + don't traverse into symlinks
		if( nftw( srcPath, DeleteRecursive, 10, flags ) == -1 )
		{
			return false;
		}
	}
	else
	{
		unlink( srcPath );
	}
	return true;
}
#endif

/*static*/ bool ycFile::SetReadOnly( const char * /*srcPath*/, bool v )
{
	ycWarn( "ycFile::SetReadOnly() is not implemented on this platform." );
	return false;
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

	#ifdef YC_USE_FOPEN_S
		FILE* fh = nullptr;
		fopen_s( &fh, fileName, FILEMODESTR_READ );
	#else
		FILE* fh = fopen( fileName, FILEMODESTR_READ );
	#endif
	if( fh )
	{
		m_file   = uintptr_t(fh);
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
	FILE* fh = (FILE*)m_file;
	fseek( fh, 0, SEEK_END );
	const long int fileSize = ftell( fh );
	fseek( fh, long(m_offset), SEEK_SET );
	return ycSize_t( fileSize );
}

void ycFileReader::Read( void* dst, const ycSize_t desiredBytes, ycSize_t* readBytes )
{
	ycAssert( m_file != kInvalidFh );
	const ycSize_t bytesRead = (ycSize_t)fread( dst, 1, desiredBytes, (FILE*)m_file );
	*readBytes = bytesRead;
	m_offset += bytesRead;
}

bool ycFileReader::Read( void* dst, const ycSize_t desiredBytes )
{
	ycSize_t bytesRead;
	Read( dst, desiredBytes, &bytesRead );
	return bytesRead == desiredBytes;
}

void ycFileReader::Close()
{
	ycAssert( m_file != kInvalidFh );
	fclose( (FILE*)m_file );
	m_file = kInvalidFh;
}

void ycFileReader::SetPos( const ycSize_t offset )
{
	ycAssert( m_file != kInvalidFh );
	fseek( (FILE*)m_file, long(offset), SEEK_SET );
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

	#ifdef YC_USE_FOPEN_S
		FILE* fh = nullptr;
		fopen_s( &fh, fileName, ( flags & kAppend ) ? FILEMODESTR_APPEND : FILEMODESTR_WRITE );
	#else
		FILE* fh = fopen( fileName, ( flags & kAppend ) ? FILEMODESTR_APPEND : FILEMODESTR_WRITE );
	#endif
	if( fh )
	{
		m_file   = uintptr_t(fh);
		m_offset = 0;
		return true;
	}
	return false;
}

bool ycFileWriter::IsOpen() const
{
	return m_file != kInvalidFh;
}

bool ycFileWriter::Write( const void* src, const ycSize_t numBytes )
{
	ycAssert( m_file != kInvalidFh );
	const size_t elementsWritten = fwrite( src, numBytes, 1, (FILE*)m_file );
	m_offset += numBytes;
	return elementsWritten == 1;
}

void ycFileWriter::Close()
{
	ycAssert( m_file != kInvalidFh );
	fclose( (FILE*)m_file );
	m_file = kInvalidFh;
}

void ycFileWriter::SetPos( const ycSize_t offset )
{
	ycAssert( m_file != kInvalidFh );
	fseek( (FILE*)m_file, long(offset), SEEK_SET );
	m_offset = offset;
}

ycSize_t ycFileWriter::GetPos() const
{
	return m_offset;
}

#endif // YC_FILESYSTEM_CSTD
