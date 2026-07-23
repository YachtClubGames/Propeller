#pragma once
#ifndef YC_BYTEBUFFER_H
#define YC_BYTEFUFFER_H

/*! \class ycByteBuffer

Writes and reads bytes from a buffer
*/

#include "ycCommon.h"
class ycAllocator;
class ycStringRef;

class ycByteBuffer
{
public:

	ycByteBuffer( ycAllocator* mem = nullptr, const ycSize_t alignment = YC_MEM_DEFAULT_ALIGN );
	void Init( ycAllocator* mem = nullptr, const ycSize_t alignment = YC_MEM_DEFAULT_ALIGN );
	~ycByteBuffer();

	bool IsInitialized() { return m_mem != nullptr; }

	void Clear();

	void Reserve( const ycSize_t numBytes );

	void AppendRaw( const void* val, const ycSize_t numBytes );
	
	void Append( const uint8_t num );
	void Append( const int8_t num );
	void Append( const char c );
	
	void Append( const uint16_t num );
	void Append( const int16_t num );
	
	void Append( const uint32_t num );
	void Append( const int32_t num );
	
	void Append( const uint64_t num );
	void Append( const int64_t num );
	
	void Append( const float num );
	void Append( const double num );

	void Append( const ycStringRef & ref );

	void PadToAlign( const ycSize_t align );
	void AppendFill( char c, const ycSize_t count );
	inline void AppendZero( const ycSize_t count ) { AppendFill( '\0', count ); }
	u8* AppendUninitialized( const ycSize_t numBytes ); //!< the lifetime of this pointer is only guaranteed until any other ycByteBuffer calls are made
	template< class t_type > t_type* AppendUninitialized() { return ( t_type* )AppendUninitialized( sizeof(t_type) ); }

	inline u8* GetData() { return m_buf; }
	inline const u8* GetData() const { return m_buf; }

	inline uintptr_t Length() const { return uintptr_t(m_writePos - m_buf); }

	void NullTerminate(); //!< add a \0 byte but do *not* advance the write position or length; useful if you want to keep a string terminated but still want to easily append more data

	uintptr_t GetWriteOffset() const;
	void SetWriteOffset( uintptr_t offset );
	void IncrementWriteOffset( const uintptr_t offset );
	inline u8* GetWritePos() { return m_writePos; }

	uintptr_t GetReadOffset() const;
	void SetReadOffset( uintptr_t offset );
	void IncrementReadOffset( const uintptr_t offset );
	inline const u8* GetReadPos() const { return m_readPos; }

	ycByteBuffer( const ycByteBuffer& other ); //!< note: this will inherit the memory allocator of 'other'
	ycByteBuffer& operator = ( const ycByteBuffer& rhs ); //!< note: this will inherit the memory allocator of 'rhs'

	void Fit( const ycSize_t numBytes ); //!< this function determines memory growth policy atm

	inline void SetAllowDynamicResize( const bool allow = true ) { m_allowDynamicResize = allow; }

	void Swap( ycByteBuffer* other );

	inline ycAllocator* GetMem() { return m_mem; }

protected:

	ycAllocator* m_mem;
	uint8_t*     m_buf;
	uint8_t*     m_writePos; // separate read/write pos? readseek/writeseek???
	uint8_t*     m_readPos;
	uint8_t*     m_bufEnd;
	ycSize_t     m_alignment;
	bool         m_allowDynamicResize;
};

#endif // !YC_BYTEBUFFER_H
