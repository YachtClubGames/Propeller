#include "ycByteBuffer.h"

#include "ycAllocator.h"
#include "ycMath.h"
#include "ycStd.h"
#include "ycStringRef.h"

ycByteBuffer::ycByteBuffer( ycAllocator* mem, const ycSize_t alignment ) :
	m_mem(                mem ? mem : g_defaultMem ),
	m_buf(                nullptr ),
	m_writePos(           nullptr ),
	m_readPos(            nullptr ),
	m_bufEnd(             nullptr ),
	m_alignment(          alignment ),
	m_allowDynamicResize( true )
{
}

void ycByteBuffer::Init( ycAllocator* mem, const ycSize_t alignment )
{
	Clear();
	m_mem       = mem ? mem : g_defaultMem ;
	m_alignment = alignment;
}

ycByteBuffer::~ycByteBuffer()
{
	if( m_buf ) { YC_FREE( m_mem, m_buf ); }
}

void ycByteBuffer::Clear()
{
	if( m_buf ) { YC_FREE( m_mem, m_buf ); }
	m_buf      = nullptr;
	m_writePos = nullptr;
	m_readPos  = nullptr;
	m_bufEnd   = nullptr;
}

void ycByteBuffer::Reserve( const ycSize_t numBytes )
{
	const ycSize_t curBufSize = m_bufEnd - m_buf;
	if( numBytes <= curBufSize ) { return; }

	uint8_t* newBuf = (uint8_t*)YC_ALLOC( m_mem, numBytes, m_alignment, "ycByteBuffer" );
	if( curBufSize )
	{
		ycMemCpy( newBuf, m_buf, curBufSize );
		YC_FREE( m_mem, m_buf );
	}
	
	const uintptr_t writeOffset = m_writePos - m_buf;
	const uintptr_t readOffset  = m_readPos - m_buf;

	m_buf      = newBuf;
	m_writePos = newBuf + writeOffset;
	m_readPos  = newBuf + readOffset;
	m_bufEnd   = newBuf + numBytes;
}

void ycByteBuffer::Fit( const ycSize_t numBytes )
{
	enum
	{
		kInitialBufSize = 1024 // should probably make this a ctor arg; desirable values will vary a lot
	};
	if( m_writePos == nullptr || m_writePos + numBytes > m_bufEnd ) // nullptr check necessary for behavior sanitizer
	{
		ycAssert( m_allowDynamicResize );
		const ycSize_t needSize = (m_writePos-m_buf) + numBytes;
		const ycSize_t curBufSize = m_bufEnd - m_buf;
		ycSize_t newBufSize = curBufSize*2;
		if( newBufSize < needSize ) { newBufSize = ycRoundUpPow2( needSize, kInitialBufSize ); }
		ycAssert( newBufSize >= needSize );
		Reserve( newBufSize );
	}
}

void ycByteBuffer::AppendRaw( const void* val, const ycSize_t numBytes )
{
	Fit( numBytes );
	for( ycSize_t i = 0; i != numBytes; ++i )
	{
		m_writePos[ i ] = *( reinterpret_cast< const uint8_t* >( val ) + i );
	}
	m_writePos += numBytes;
}

void ycByteBuffer::Append( const uint8_t num )
{
	AppendRaw( &num, sizeof(uint8_t) );
}

void ycByteBuffer::Append( const int8_t num )
{
	AppendRaw( &num, sizeof(int8_t) );
}

void ycByteBuffer::Append( const char c )
{
	AppendRaw( &c, sizeof(char) );
}

void ycByteBuffer::Append( const uint16_t num )
{
	AppendRaw( &num, sizeof(uint16_t) );
}

void ycByteBuffer::Append( const int16_t num )
{
	AppendRaw( &num, sizeof(int16_t) );
}

void ycByteBuffer::Append( const uint32_t num )
{
	AppendRaw( &num, sizeof(uint32_t) );
}

void ycByteBuffer::Append( const int32_t num )
{
	AppendRaw( &num, sizeof(int32_t) );
}

void ycByteBuffer::Append( const uint64_t num )
{
	AppendRaw( &num, sizeof(uint64_t) );
}

void ycByteBuffer::Append( const int64_t num )
{
	AppendRaw( &num, sizeof(int64_t) );
}

void ycByteBuffer::Append( const float num )
{
	AppendRaw( &num, sizeof(float) );
}

void ycByteBuffer::Append( const double num )
{
	AppendRaw( &num, sizeof(double) );
}

void ycByteBuffer::Append( const ycStringRef & ref )
{
	AppendRaw( ref.Get(), ref.Length() );
}

void ycByteBuffer::PadToAlign( const ycSize_t align )
{
	ycAssert( ycIsPow2( align ) );
	const ycSize_t pad = ycRoundUpPow2( Length(), align ) - Length();
	AppendZero( pad );
}

void ycByteBuffer::AppendFill( char c, const ycSize_t count )
{
	Fit( count );
	for( ycSize_t i = 0; i < count; ++i )
	{
		m_writePos[ i ] = c;
	}
	m_writePos += count;
}

u8* ycByteBuffer::AppendUninitialized( const ycSize_t numBytes )
{
	Fit( numBytes );
	u8* dst = m_writePos;
	m_writePos += numBytes;
	return dst;
}

void ycByteBuffer::NullTerminate()
{
	Fit( 1 );
	*m_writePos = 0;
}

uintptr_t ycByteBuffer::GetWriteOffset() const
{
	return m_writePos-m_buf;
}

void ycByteBuffer::SetWriteOffset( uintptr_t offset )
{
	ycAssert( uintptr_t(m_bufEnd-m_buf) >= offset );
	m_writePos = m_buf + offset;
}

void ycByteBuffer::IncrementWriteOffset( uintptr_t offset )
{
	ycAssert( m_writePos+offset <= m_bufEnd );
	m_writePos += offset;
}

uintptr_t ycByteBuffer::GetReadOffset() const
{
	return m_readPos-m_buf;
}

void ycByteBuffer::SetReadOffset( uintptr_t offset )
{
	ycAssert( uintptr_t(m_bufEnd-m_buf) >= offset );
	m_readPos = m_buf + offset;
}

void ycByteBuffer::IncrementReadOffset( uintptr_t offset )
{
	ycAssert( m_readPos+offset <= m_bufEnd );
	m_readPos += offset;
}

ycByteBuffer::ycByteBuffer( const ycByteBuffer& other )
{
	m_mem = other.m_mem;
	m_alignment = other.m_alignment;
	m_allowDynamicResize = other.m_allowDynamicResize;

	const uintptr_t bufSize = other.m_bufEnd - other.m_buf;
	if( bufSize )
	{
		m_buf = (u8*)YC_ALLOC( m_mem, bufSize, other.m_alignment, "ycByteBuffer" );
		ycMemCpy( m_buf, other.m_buf, bufSize );
		m_writePos  = m_buf + other.GetWriteOffset();
		m_readPos   = m_buf + other.GetReadOffset();
		m_bufEnd    = m_buf + bufSize;
	}
	else
	{
		m_buf = nullptr;
		m_writePos = nullptr;
		m_readPos = nullptr;
		m_bufEnd = nullptr;
	}
}

ycByteBuffer& ycByteBuffer::operator = ( const ycByteBuffer& rhs )
{
	Clear();
	m_mem = rhs.m_mem;
	m_alignment = rhs.m_alignment;
	m_allowDynamicResize = rhs.m_allowDynamicResize;
	const uintptr_t bufSize = rhs.m_bufEnd - rhs.m_buf;
	if( bufSize )
	{
		m_buf = (u8*)YC_ALLOC( m_mem, bufSize, rhs.m_alignment, "ycByteBuffer" );
		ycMemCpy( m_buf, rhs.m_buf, bufSize );
		m_writePos  = m_buf + rhs.GetWriteOffset();
		m_readPos   = m_buf + rhs.GetReadOffset();
		m_bufEnd    = m_buf + bufSize;
	}
	return *this;
}

void ycByteBuffer::Swap( ycByteBuffer* other )
{
	ycAllocator* mem                = m_mem;
	uint8_t*     buf                = m_buf;
	uint8_t*     writePos           = m_writePos;
	uint8_t*     readPos            = m_readPos;
	uint8_t*     bufEnd             = m_bufEnd;
	ycSize_t     alignment          = m_alignment;
	bool         allowDynamicResize = m_allowDynamicResize;

	m_mem                = other->m_mem;
	m_buf                = other->m_buf;
	m_writePos           = other->m_writePos;
	m_readPos            = other->m_readPos;
	m_bufEnd             = other->m_bufEnd;
	m_alignment          = other->m_alignment;
	m_allowDynamicResize = other->m_allowDynamicResize;

	other->m_mem                = mem;
	other->m_buf                = buf;
	other->m_writePos           = writePos;
	other->m_readPos            = readPos;
	other->m_bufEnd             = bufEnd;
	other->m_alignment          = alignment;
	other->m_allowDynamicResize = allowDynamicResize;
}
