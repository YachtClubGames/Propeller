#include "ycPagedList.h"

#include "ycMath.h"

ycPagedListBase::IterBase::IterBase() :
	m_page( nullptr ),
	m_localIndex( 0 ),
	m_list( nullptr )
{}

//ycPagedListBase::IterBase::IterBase( ycPagedListBase* list ) :
//	m_page( nullptr ),
//	m_localIndex( 0 ),
//	m_list( list )
//{}

ycPagedListBase::IterBase::IterBase( const IterBase& copy ) :
	m_page( copy.m_page ),
	m_localIndex( copy.m_localIndex ),
	m_list( copy.m_list )
{}

ycPagedListBase::IterBase::IterBase( ycPagedListBase* list, void* page, ureg localIndex ) :
	m_page( page ),
	m_localIndex( localIndex ),
	m_list( list )
{
}

bool ycPagedListBase::IterBase::IsValid() const
{
	return m_page != nullptr;
}

ycPagedListBase::IterBase& ycPagedListBase::IterBase::operator++() // prefix
{
	ycAssert( m_localIndex < m_list->m_numItemsPerPage );
	ycPagedListBase::PageDesc* page = (ycPagedListBase::PageDesc*)m_page;
	if( ++m_localIndex == page->numItems )// m_list->m_numItemsPerPage )
	{
		m_page = page->next;
		m_localIndex = 0;
	}
	return *this;
}

ycPagedListBase::IterBase ycPagedListBase::IterBase::operator++( int ) // postfix
{
	ycPagedListBase::IterBase cur = *this;
	IterBase::operator++();
	return *this;
}

ycPagedListBase::IterBase& ycPagedListBase::IterBase::operator--() // prefix
{
	ycAssert( m_localIndex < m_list->m_numItemsPerPage );
	if( m_localIndex == 0 )
	{
		ycPagedListBase::PageDesc* page = (ycPagedListBase::PageDesc*)m_page;
		m_page = page->prev;
		m_localIndex = m_page ? page->numItems-1 : 0 ;
	}
	else
	{
		--m_localIndex;
	}
	return *this;
}

ycPagedListBase::IterBase ycPagedListBase::IterBase::operator--( int) // postfix
{
	ycPagedListBase::IterBase cur = *this;
	IterBase::operator--();
	return *this;
}

void* ycPagedListBase::IterBase::GetItem()
{
	return m_list->GetPageItem( (ycPagedListBase::PageDesc*)m_page, m_localIndex );
}

ycPagedListBase::ycPagedListBase( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, ycSize_t pageSizeInBytesOrCount, const ycSize_t itemAlignment ) :
	m_head( nullptr ),
	m_tail( nullptr ),
	m_mem( mem )
#ifdef YC_ENABLE_DEBUG_STRINGS
	, m_debugName( debugName )
#endif
{
	YC_UNUSED( debugName );
	ycAssert( ycIsPow2( itemAlignment ) );

	m_descSize = ycMax( itemAlignment, ycSize_t( sizeof( PageDesc ) ) );
	m_itemSize  = ycRoundUpPow2( itemSize, itemAlignment );
	m_pageSize  = pageSizeInBytesOrCount;
	m_itemAlign = itemAlignment;
	m_numItems  = 0;

	const ycSize_t itemSpace = pageSizeInBytesOrCount - m_descSize;
	m_numItemsPerPage = itemSpace / m_itemSize;
	ycAssert( m_numItemsPerPage > 0 );
}

ycPagedListBase::ycPagedListBase() :
	m_head( nullptr ),
	m_tail( nullptr ),
	m_mem( nullptr )
#ifdef YC_ENABLE_DEBUG_STRINGS
	, m_debugName( nullptr )
#endif
{
	m_descSize        = 0;
	m_itemSize        = 0;
	m_pageSize        = 0;
	m_itemAlign       = 0;
	m_numItems        = 0;
	m_numItemsPerPage = 0;
}

void ycPagedListBase::Init( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, ycSize_t pageSizeInBytesOrCount, const ycSize_t itemAlignment )
{
	ycAssert( m_mem == nullptr );
	m_head = nullptr;
	m_tail = nullptr;
	m_mem  = mem ? mem : g_defaultMem;
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif

	ycAssert( ycIsPow2( itemAlignment ) );

	m_descSize  = ycMax( itemAlignment, ycSize_t( sizeof(PageDesc) ) );
	m_itemSize  = ycRoundUpPow2( itemSize, itemAlignment );
	m_pageSize  = pageSizeInBytesOrCount;
	m_itemAlign = itemAlignment;
	m_numItems  = 0;

	const ycSize_t itemSpace = pageSizeInBytesOrCount - m_descSize;
	m_numItemsPerPage = itemSpace / m_itemSize;
	ycAssert( m_numItemsPerPage > 0 );
}

void ycPagedListBase::Swap( ycPagedListBase* list )
{
	ycSwap( m_descSize, list->m_descSize );
	ycSwap( m_itemSize, list->m_itemSize ); //!< padded to m_itemAlign
	ycSwap( m_pageSize, list->m_pageSize );
	ycSwap( m_numItemsPerPage, list->m_numItemsPerPage );
	ycSwap( m_itemAlign, list->m_itemAlign );
	ycSwap( m_numItems, list->m_numItems );

	ycSwap( m_head, list->m_head );
	ycSwap( m_tail, list->m_tail );

	ycSwap( m_mem, list->m_mem );
#ifdef YC_ENABLE_DEBUG_STRINGS
	ycSwap( m_debugName, list->m_debugName );
#endif
}

void* ycPagedListBase::AllocItem( bool unsorted )
{
	PageDesc* page = nullptr;

	// in unsorted, only search the middle if the tail is full
	if( unsorted && m_tail && m_tail->numItems == m_numItems )
	{
		for( page = m_head; page != m_tail; ++page )
		{
			if( page->numItems < m_numItemsPerPage ) { break; }
		}
	}
	else
	{
		page = m_tail;
	}

	ycAssert( page == nullptr || page->numItems <= m_numItemsPerPage );
	if( YC_UNLIKELY( page == nullptr || page->numItems == m_numItemsPerPage ) )
	{
		page = AllocPage();
	}

	void* dst = GetPageItem( page, page->numItems );
	++page->numItems;
	++m_numItems;
	return dst;
}

ycPagedListBase::PageDesc* ycPagedListBase::AllocPage()
{
	const char* debugName = "ycPagedListBase";
#ifdef YC_ENABLE_DEBUG_STRINGS
	debugName = m_debugName;
#else
	YC_UNUSED( debugName );
#endif
	PageDesc* page = (PageDesc*)YC_ALLOC( m_mem, m_pageSize, m_itemAlign, debugName );
	page->prev = m_tail;
	page->next = nullptr;
	page->numItems = 0;
	if( m_head == nullptr )
	{
		ycAssert( m_tail == nullptr );
		m_head = page;
	}
	else
	{
		ycAssert( m_tail );
		m_tail->next = page;
	}
	m_tail = page;
	return page;
}

void* ycPagedListBase::GetPageItem( PageDesc* page, const ureg localIndex )
{
	ycAssert( localIndex < m_numItemsPerPage );
	uint8_t* firstItem = (uint8_t*)page + m_descSize;
	return firstItem + m_itemSize*localIndex;
}

void ycPagedListBase::FreePage( PageDesc* page )
{
	if( page->prev ) { page->prev->next = page->next; }
	else { ycAssert( m_head == page ); m_head = page->next; }
	if( page->next ) { page->next->prev = page->prev; }
	else { ycAssert( m_tail == page ); m_tail = page->prev; }
	YC_FREE( m_mem, page );
}

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycPagedList_StressTest )
{
	ycAllocator* mem = ycTest::GetMem();
	struct Test
	{
		int a, b, c;
	};
	ycPagedList< Test > list( "Test", mem );
	enum
	{
		kIterations = 1024*16
	};
	for( ureg i = 0; i != kIterations; ++i )
	{
		Test test;
		list.Append( test );
	}

	YC_TEST_PASS( "ycPagedList stress test" );
}

#endif // YC_TEST
