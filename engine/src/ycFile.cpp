#include "ycFile.h"

#include "ycStackString.h"

/*static*/ bool ycFile::IsModifiedTimeNewer( const char* fileName, const char* compareAgainst )
{
	u64 srcMtime;
	bool gotMTime = ycFile::GetModifiedTime( fileName, &srcMtime );
	ycAssertMsg( gotMTime, "%s -> %s", fileName, compareAgainst );
	u64 dstMtime;
	gotMTime = ycFile::GetModifiedTime( compareAgainst, &dstMtime );
	if( !gotMTime || dstMtime < srcMtime )
	{
		return true;
	}
	return false;
}

/*static*/ u8* ycFile::Read( const ycStringRef& fileName, ycAllocator* mem, ycSize_t* outFileSize )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::Read( fileNameStr.ToCStr(), mem, outFileSize );
}

/*static*/ bool ycFile::Exists( const ycStringRef& fileName )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::Exists( fileNameStr.ToCStr() );
}

/*static*/ bool ycFile::GetSize( const ycStringRef& fileName, ycSize_t* fileSize )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::GetSize( fileNameStr.ToCStr(), fileSize );
}

/*static*/ bool ycFile::GetModifiedTime( const ycStringRef& fileName, ycFileTime_t* mtime )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::GetModifiedTime( fileNameStr.ToCStr(), mtime );
}

/*static*/ bool ycFile::IsModifiedTimeNewer( const ycStringRef& fileName, const ycStringRef& compareAgainst )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::IsModifiedTimeNewer( fileNameStr.ToCStr(), ycStackString< YC_MAX_PATH >( compareAgainst ).ToCStr() );
}

/*static*/ bool ycFile::Write( const ycStringRef& fileName, const void* src, const ycSize_t len )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::Write( fileNameStr.ToCStr(), src, len );
}

/*static*/ bool ycFile::Write( const ycStringRef& fileName, const ureg numChunks, const u8** chunks, const ycSize_t* chunkSizes )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::Write( fileNameStr.ToCStr(), numChunks, chunks, chunkSizes );
}

/*static*/ bool ycFile::Copy( const ycStringRef& srcPath, const ycStringRef& dstPath )
{
	ycStackString< YC_MAX_PATH > src( srcPath );
	ycStackString< YC_MAX_PATH > dst( dstPath );
	return ycFile::Copy( src.ToCStr(), dst.ToCStr() );
}

/*static*/ bool ycFile::Delete( const ycStringRef& fileName )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::Delete( fileNameStr.ToCStr() );
}

/*static*/ bool ycFile::SetReadOnly( const ycStringRef & fileName, bool v )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return ycFile::SetReadOnly( fileNameStr.ToCStr(), v );
}

bool ycFileReader::Open( const ycStringRef& fileName )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return Open( fileNameStr.ToCStr() );
}

bool ycFileWriter::Open( const ycStringRef& fileName, const ureg flags )
{
	ycStackString< YC_MAX_PATH > fileNameStr( fileName );
	return Open( fileNameStr.ToCStr(), flags );
}
