#pragma once
#ifndef YC_FILE_H
#define YC_FILE_H

/*! \class ycFile

Low level file wrapper. Game code should rarely use this, and should use ycFileRef instead.
*/

#include "ycCommon.h"
#include "ycPlatformConfig.h"

typedef u64 ycFileTime_t; //!< can refer to a duration or an arbitrary fixed point in time

class ycAllocator;
class ycStringRef;

#define YC_MAX_PATH 256

class ycFile
{
public:

	static u8* Read( const char* fileName, ycAllocator* mem, ycSize_t* outFileSize = nullptr ); //!< returns nullptr on failure
	static bool Exists( const char* fileName );
	static bool GetSize( const char* fileName, ycSize_t* fileSize );
	static bool GetModifiedTime( const char* fileName, ycFileTime_t* mtime ); //!< conversion functions to any useful time measures are not (currently) exposed, the returned values are only useful for comparison with eachother
	static bool IsModifiedTimeNewer( const char* fileName, const char* compareAgainst ); 
	static bool Write( const char* fileName, const void* src, const ycSize_t len ); //!< this is for tools!!! dont use this to write saves/preferences/etc
	static bool Write( const char* fileName, const ureg numChunks, const u8** chunks, const ycSize_t* chunkSizes );
	static bool Copy( const char* srcPath, const char* dstPath ); //!< this is for tools!!! dont use this to write saves/preferences/etc
	static bool Delete( const char* fileName ); //!< this is for tools!!! dont use this to delete saves/preferences/etc
	static bool SetReadOnly( const char* fileName, bool ro );

	/* it would be nice to migrate over at some point, but for now we can have both versions, char* and ycStringRef */

	static u8* Read( const ycStringRef& fileName, ycAllocator* mem, ycSize_t* outFileSize = nullptr ); //!< returns nullptr on failure
	static bool Exists( const ycStringRef& fileName );
	static bool GetSize( const ycStringRef& fileName, ycSize_t* fileSize );
	static bool GetModifiedTime( const ycStringRef& fileName, ycFileTime_t* mtime ); //!< conversion functions to any useful time measures are not (currently) exposed, the returned values are only useful for comparison with eachother
	static bool IsModifiedTimeNewer( const ycStringRef& fileName, const ycStringRef& compareAgainst ); 
	static bool Write( const ycStringRef& fileName, const void* src, const ycSize_t len ); //!< this is for tools!!! dont use this to write saves/preferences/etc
	static bool Write( const ycStringRef& fileName, const ureg numChunks, const u8** chunks, const ycSize_t* chunkSizes );
	static bool Copy( const ycStringRef& srcPath, const ycStringRef& dstPath ); //!< this is for tools!!! dont use this to write saves/preferences/etc
	static bool Delete( const ycStringRef& fileName ); //!< this is for tools!!! dont use this to delete saves/preferences/etc
	static bool SetReadOnly( const ycStringRef& fileName, bool ro );

	static void Init(); //!< init platform fs. called by the engine at startup, please don't call.
};

class ycFileReader
{
public:

	ycFileReader();
	~ycFileReader();
	bool Open( const char* fileName );
	bool Open( const ycStringRef& fileName );
	bool IsOpen() const;
	ycSize_t GetSize();
	void Read( void* dst, const ycSize_t desiredBytes, ycSize_t* readBytes );
	bool Read( void* dst, const ycSize_t desiredBytes ); // returns false if the desired number of bytes could not be read
	template< class t_type > bool Read( t_type* dst ) { return Read( dst, sizeof(t_type) ); }
	void Close();

	void SetPos( const ycSize_t offset );
	ycSize_t GetPos() const;

protected:

	uintptr_t m_file;
	ycSize_t  m_offset;
};

//! this is for tools!!! dont use this to write saves/preferences/etc
class ycFileWriter
{
public:
	enum
	{
		kAppend = 0x1
	};

	ycFileWriter();
	~ycFileWriter();
	bool Open( const char* fileName, const ureg flags = 0x0 );
	bool Open( const ycStringRef& fileName, const ureg flags = 0x0 );
	bool IsOpen() const;
	bool Write( const void* src, const ycSize_t numBytes );
	template< class t_type > void Write( t_type* src ) { Write( src, sizeof(t_type) ); }
	void Close();

	void SetPos( const ycSize_t offset );
	ycSize_t GetPos() const;

protected:

	uintptr_t m_file;
	ycSize_t  m_offset;
};

#endif // !YC_FILE_H
