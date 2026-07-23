#include "ycBumpAllocator.h"

#include "ycMath.h"
#include "ycVirtualMem.h"

ycBumpAllocator::ycBumpAllocator( const char* debugName )
	: ycAllocator( debugName )
{
}

ycBumpAllocator::ycBumpAllocator( const char* debugName, const bool largePages, const ycSize_t maxSize )
	: ycAllocator( debugName )
{
	Init( largePages, maxSize );
}

void ycBumpAllocator::Init( const bool largePages, const ycSize_t _maxSize )
{
	ycAssert( m_base == nullptr );

	const ycSize_t pageSize = largePages ? ycVirtualMem::GetLargePageSize() : ycVirtualMem::GetPageSize() ;
	const ycSize_t maxSize = ycRoundUpPow2( _maxSize, pageSize );
	ycAssert( maxSize );

	m_base = (u8*)ycVirtualMem::Reserve( maxSize );

	m_offset = 0;
	m_committed = 0;
	m_maxSize = maxSize;
	m_pageSize = pageSize;
}

/*virtual*/ ycBumpAllocator::~ycBumpAllocator()
{
	Clear();
}

void ycBumpAllocator::Clear()
{
	#if YC_NDA
	#endif
	if( m_base )
	{
		ycVirtualMem::Release( m_base, m_maxSize );
	}
	m_base = nullptr;
	m_offset = 0;
	m_committed = 0;
	m_maxSize = 0;
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycBumpAllocator )
{
	YC_UNUSED( debugName ); YC_UNUSED( file ); YC_UNUSED( line ); YC_UNUSED( flags );
	ycAssert( bytes != 0 );

	const ycSize_t alignedOffset = ycRoundUpPow2( m_offset, align ); // TODO HACK FIXME: need to round up the _pointer_ not the offset
	u8* ptr = m_base + alignedOffset;
	m_offset = alignedOffset + bytes;

	// TODO HACK FIXME: m_maxSize assert
	if(YC_UNLIKELY( m_offset > m_committed ))
	{
	#if YC_NDA
	#else
		const ycSize_t need = m_offset - m_committed;
		const ycSize_t commit = m_pageSize * ( (need+m_pageSize-1)/m_pageSize );
		ycAssert( m_committed + commit <= m_maxSize );
		ycVirtualMem::Commit( m_base + m_committed, commit );
		m_committed += commit;
	#endif
	}

	return ptr;
}

YC_ALLOCATOR_DECLARATION_FREE( ycBumpAllocator )
{
	YC_UNUSED( ptr );
	YC_UNUSED( flags );
}

YC_ALLOCATOR_DECLARATION_SIZE( ycBumpAllocator )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	ycAssert( false );
	return 0;
}
