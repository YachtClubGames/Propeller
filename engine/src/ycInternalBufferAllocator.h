#pragma once

/* \class ycInternalBufferAllocator
Allocates from an internal block of memory, a la ycStackString
Falls back on a different allocator when the internal block is full
*/
#include "ycAllocator.h"

// basic case has no definition, to create an error
template< ycSize_t size, ycSize_t align >
struct ycAlignedBuf;

// each specific alignment needs a specialization defined, for reasons
#define DEF_ALIGNBUF( N ) template< ycSize_t size > struct ycAlignedBuf<size, N> { YC_ALIGN( N ) u8 m_stkbuf[ size ]; u8* GetBuf() { return m_stkbuf; } static constexpr ycSize_t stackBufSize = size; }

// define all reasonable alignments
// and/or show off how many powers of 2 I have memorized
DEF_ALIGNBUF( 1 );
DEF_ALIGNBUF( 2 );
DEF_ALIGNBUF( 4 );
DEF_ALIGNBUF( 8 );
DEF_ALIGNBUF( 16 );
DEF_ALIGNBUF( 32 );
DEF_ALIGNBUF( 64 );
DEF_ALIGNBUF( 128 );
DEF_ALIGNBUF( 256 );
DEF_ALIGNBUF( 512 );
DEF_ALIGNBUF( 1024 );
DEF_ALIGNBUF( 2048 );
DEF_ALIGNBUF( 4096 );
DEF_ALIGNBUF( 8192 );

#undef DEF_ALIGNBUF


template< ycSize_t t_size, ycSize_t t_align >
class ycInternalBufferAllocator : public ycAllocator
{
public:
	ycInternalBufferAllocator( const char* debugName, ycAllocator* fallback ) : ycAllocator( debugName ), m_fallback( fallback ) {}

	YC_ALLOCATOR_DECLARATIONS

	virtual bool IsStorageEmbedded() override;
	virtual bool IsPtrEmbedded( void* ptr ) override;
	virtual void SetFallbackAllocator( ycAllocator* ) override;

protected:
	ycAllocator* m_fallback = nullptr; 
#if defined YC_ENABLE_ASSERT
	bool m_allocated = false;
#endif
	ycAlignedBuf< t_size, t_align > m_buf;
};


template< ycSize_t t_size, ycSize_t t_align >
void* ycInternalBufferAllocator<t_size, t_align>::Alloc( const ycSize_t bytes, const ycSize_t align, ycAllocInfo debugName, const char* file, const uint32_t line, const ureg flags )
{
	ycAssert( align <= t_align );

	if( bytes > t_size )
	{
		return m_fallback->Alloc( bytes, align, debugName, file, line, flags );
	}

#if defined YC_ENABLE_ASSERT
	ycAssert( !m_allocated );
	m_allocated = true;
#endif
	return m_buf.GetBuf();
}

template< ycSize_t t_size, ycSize_t t_align >
void ycInternalBufferAllocator<t_size, t_align>::Free( void* ptr, const ureg flags )
{
	if( ptr != nullptr && ptr != m_buf.GetBuf() )
	{
		m_fallback->Free( ptr, flags );
		return;
	}

#if defined YC_ENABLE_ASSERT
	ycAssert( m_allocated );
	m_allocated = false;
#endif
}

template< ycSize_t t_size, ycSize_t t_align >
ycSize_t ycInternalBufferAllocator<t_size, t_align>::Size( void* ptr, ycSize_t align )
{
	if( ptr != nullptr && ptr != m_buf.GetBuf() )
	{
		return m_fallback->Size( ptr, align );
	}

	return t_size;
}

template< ycSize_t t_size, ycSize_t t_align >
bool ycInternalBufferAllocator<t_size, t_align>::IsStorageEmbedded()
{
	return true;
}

template< ycSize_t t_size, ycSize_t t_align >
bool ycInternalBufferAllocator<t_size, t_align>::IsPtrEmbedded( void* ptr )
{
	return ptr == m_buf.GetBuf();
}

template< ycSize_t t_size, ycSize_t t_align >
void ycInternalBufferAllocator<t_size, t_align>::SetFallbackAllocator( ycAllocator* newFallback )
{
	m_fallback = newFallback;
}

