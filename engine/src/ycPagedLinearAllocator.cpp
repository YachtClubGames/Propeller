#include "ycPagedLinearAllocator.h"

#include "ycMath.h"
#include "ycStd.h"

inline void ycPagedLinearAllocator::SetPageNextPtr(u8* page, u8* next) { reinterpret_cast<PageDesc*>( page + GetPageSize(page) - sizeof(PageDesc) )->next = next; }
inline u8* ycPagedLinearAllocator::GetPageNextPtr(u8* page) { return reinterpret_cast<PageDesc*>( page + GetPageSize(page) - sizeof(PageDesc) )->next; }
inline void ycPagedLinearAllocator::SetPageSize(u8* page, ycSize_t size) { reinterpret_cast<PageHeader*>( page )->size = size; }
inline ycSize_t ycPagedLinearAllocator::GetPageSize(u8* page) { return reinterpret_cast<PageHeader*>( page )->size; }

void ycPagedLinearAllocator::Init( const char* debugName, ycAllocator* mem, const ycSize_t pageSize /*= YC_MEM_DEFAULT_PAGE_SIZE*/, const ycSize_t pageAlign /*= YC_MEM_DEFAULT_ALIGN*/, 
									const bool allowOversize /*= false*/ )
{
	// TODO HACK FIXME: size/align sanity checks, mebbe round up if necessary
	ycAssert( pageSize > sizeof(PageDesc) );
	Clear();
	m_pageSize  = pageSize;
	m_pageAlign = pageAlign;
	m_mem       = mem ? mem : g_defaultMem;
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif
	m_allowOversize = allowOversize;
}

ycPagedLinearAllocator::ycPagedLinearAllocator( const char* debugName, ycAllocator* mem, const ycSize_t pageSize /*= YC_MEM_DEFAULT_PAGE_SIZE*/, const ycSize_t pageAlign /*= YC_MEM_DEFAULT_ALIGN*/,
													bool allowOversize /*= false */ ) :
	ycAllocator( debugName )
{
	m_head = nullptr; // so we can use Init()
	Init( debugName, mem, pageSize, pageAlign, allowOversize );
}

void ycPagedLinearAllocator::Clear()
{
	for( u8* page = m_head; page != nullptr; /**/ )
	{
		u8* nextPage = GetPageNextPtr( page );
		YC_FREE( m_mem, page );
		page = nextPage;
	}
	m_head = m_tail = m_curPos = nullptr;
}

ycPagedLinearAllocator::~ycPagedLinearAllocator()
{
	Clear();
}

YC_ALLOCATOR_DECLARATION_ALLOC(ycPagedLinearAllocator)
{
	YC_UNUSED(debugName);
	YC_UNUSED(file);
	YC_UNUSED(line);
	YC_UNUSED(flags);

	ycAssert( bytes != 0 );

	ycSize_t available;
again:

	// See how much we have left in this page.
	if( m_tail )
	{
		ycAssert( m_curPos > m_tail );
		u8* pageEnd = m_tail + GetPageSize( m_tail ) - sizeof(PageDesc);
		ycAssert( m_curPos <= pageEnd );
		available = pageEnd - m_curPos;
	} else {
		available = 0;
	}

	// Include alignment requirements.
	ycAssert( ycIsPow2( align ) && align > 0 );
	ycSize_t misaligned = ( align - (ycSize_t)m_curPos ) & ( align-1 );
	ycSize_t required = bytes + misaligned;

	// Allocate a new page if we run out.
	if( YC_UNLIKELY(available < required) )
	{
		// byte-size memory :)
		ycSize_t oversizedPageSize = bytes + align + sizeof( PageHeader ) + sizeof( PageDesc );
		ycSize_t allocBytes = ( m_allowOversize && oversizedPageSize > m_pageSize ) ? oversizedPageSize
																					: m_pageSize;

		if( oversizedPageSize > allocBytes )
		{
			ycAssertMsg( flags & kAllocFlag_NoAssert, "Page size too small for allocation." );
			return nullptr;
		}

		u8* newTail = (u8*)YC_ALLOC( m_mem, allocBytes, m_pageAlign, m_debugName );
		SetPageSize( newTail, allocBytes );
		SetPageNextPtr( newTail, nullptr );
		if( m_tail )
			SetPageNextPtr( m_tail, newTail );
		else
			m_head = newTail;
		m_tail = newTail;
		m_curPos = newTail + sizeof(PageHeader);
		goto again; // check alignment again for the new page
	}

	ycAssert(m_head);
	ycAssert(m_tail);
	ycAssert(m_curPos);
	ycAssert(m_curPos >= m_tail);
	
	m_curPos += misaligned;
	u8* ptr = m_curPos;
	m_curPos += bytes;

	ycAssert( ( (ycSize_t)ptr & ( align-1 ) ) == 0 );
	return ptr;
}

YC_ALLOCATOR_DECLARATION_FREE( ycPagedLinearAllocator )
{
	YC_UNUSED( ptr );
	YC_UNUSED( flags );
}

YC_ALLOCATOR_DECLARATION_SIZE( ycPagedLinearAllocator )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	return 0;
}

u8* ycPagedLinearAllocator::Peek( ycSize_t* pAvailBytes, const ycSize_t minimumRequired )
{
	const ycSize_t currPageSize = ( m_allowOversize ) ? GetPageSize( m_tail ) : m_pageSize;
	const ycSize_t maxUsable = currPageSize - sizeof(PageDesc);
	ycAssert( minimumRequired <= maxUsable ); // currPageSize is too small to fit minimumRequired, even on an empty page

	ycSize_t availBytes;
	if( m_head )
	{
		const u8* pageEnd = m_tail + currPageSize - sizeof(PageDesc);
		availBytes = pageEnd - m_curPos;
		if( availBytes < minimumRequired )
		{
			ycAssert( m_tail );
			ycAssert( m_curPos > m_tail );
			ycAssert( m_curPos <= pageEnd );
			ycAssert( pageEnd >= m_curPos );
			u8* newTail = m_curPos = (u8*)YC_ALLOC( m_mem, currPageSize, m_pageAlign, m_debugName );
			SetPageNextPtr( m_tail, newTail );
			m_tail = newTail;
			m_curPos += sizeof(PageHeader);
			SetPageNextPtr( m_tail, nullptr );
			availBytes = maxUsable;
		}
	}
	else
	{
		if( minimumRequired )
		{
			ycAssert( m_tail == nullptr );
			ycAssert( m_curPos == nullptr );
			m_head = m_tail = m_curPos = (u8*)YC_ALLOC( m_mem, currPageSize, m_pageAlign, m_debugName );
			m_curPos += sizeof(PageHeader);
			SetPageSize( m_tail, currPageSize );
			SetPageNextPtr( m_tail, nullptr );
			availBytes = maxUsable;
		}
		else
		{
			*pAvailBytes = 0;
			return nullptr;
		}
	}

	{
		ycAssert( availBytes >= minimumRequired );
		*pAvailBytes = availBytes;
		return m_curPos;
	}
}

void ycPagedLinearAllocator::Advance( const ycSize_t size )
{
	#ifdef YC_ENABLE_ASSERT
		const ycSize_t currPageSize = ( m_allowOversize ) ? GetPageSize( m_tail ) : m_pageSize;
		const u8* pageEnd = m_tail + currPageSize - sizeof(PageDesc);
		const ycSize_t avail = pageEnd - m_curPos;
		ycAssert( size <= avail );
	#endif // YC_ENABLE_ASSERT
	m_curPos += size;
}

u8* ycPagedLinearAllocator::Save()
{
	return m_curPos;
}

void ycPagedLinearAllocator::Restore( u8* ptr )
{
	/*
	Save() will return nullptr if the allocator is completely empty, destroy everything if we receive it
	*/
	if( ptr == nullptr )
	{
		Clear();
		return;
	}

	// iterate till we find the page that the desired location is on (page list isn't doubly-linked!)
	for( u8* page = m_head; /**/; page = GetPageNextPtr( page ) )
	{
		ycAssert( page );
		const ycSize_t currPageSize = GetPageSize( page );
		const u8* pageEnd = page + currPageSize - sizeof(PageDesc);
		if( ptr >= page && ptr < pageEnd )
		{
			// if this isnt the last page, delete subsequent pages
			u8* nextPage = GetPageNextPtr( page );
			if( nextPage != nullptr )
			{
				while( nextPage )
				{
					u8* delPage = nextPage;
					nextPage = GetPageNextPtr( delPage );
					YC_FREE( m_mem, delPage );
				}

				SetPageNextPtr( page, nullptr );
				m_tail = page;
			}

			m_curPos = ptr;
			return;
		}
		
	}
}
