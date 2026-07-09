#include "ycPagedVector.h"
#include "ycMath.h"

ycPagedVectorBase::IterBase::IterBase()
	: m_pageIndex( 0 )
	, m_localIndex( 0 )
	, m_list( nullptr )
{
}

ycPagedVectorBase::IterBase::IterBase( const IterBase& copy )
	: m_pageIndex( copy.m_pageIndex )
	, m_localIndex( copy.m_localIndex )
	, m_list( copy.m_list )
{
}

ycPagedVectorBase::IterBase::IterBase( ycPagedVectorBase* list, ureg pageIndex, ureg localIndex )
	: m_pageIndex( pageIndex )
	, m_localIndex( localIndex )
	, m_list( list )
{
}

bool ycPagedVectorBase::IterBase::IsValid() const
{
	return m_pageIndex < m_list->m_pagesLength;
}

ycPagedVectorBase::IterBase& ycPagedVectorBase::IterBase::operator++() // prefix
{
	ycAssert( m_localIndex < m_list->m_numItemsPerPage );
	ycPagedVectorBase::PageDesc* page = (ycPagedVectorBase::PageDesc*)m_list->m_pages[ m_pageIndex ];
	if( ++m_localIndex == page->numItems )
	{
		m_pageIndex++;
		m_localIndex = 0;
	}
	return *this;
}

ycPagedVectorBase::IterBase ycPagedVectorBase::IterBase::operator++( int ) // postfix
{
	ycPagedVectorBase::IterBase cur = *this;
	IterBase::operator++();
	return *this;
}

ycPagedVectorBase::IterBase& ycPagedVectorBase::IterBase::operator--() // prefix
{
	ycAssert( m_localIndex < m_list->m_numItemsPerPage );
	if( m_localIndex == 0 )
	{
		m_pageIndex--;
		m_localIndex = IsValid() ? m_list->m_pages[ m_pageIndex ]->numItems-1 : 0 ;
	}
	else
	{
		--m_localIndex;
	}
	return *this;
}

ycPagedVectorBase::IterBase ycPagedVectorBase::IterBase::operator--( int) // postfix
{
	ycPagedVectorBase::IterBase cur = *this;
	IterBase::operator--();
	return *this;
}

void* ycPagedVectorBase::IterBase::GetItem()
{
	return m_list->GetPageItem( m_pageIndex, m_localIndex );
}

ycPagedVectorBase::ycPagedVectorBase( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, const ycSize_t pageSizeInBytes, const ycSize_t itemAlignment )
	: m_mem( mem )
#ifdef YC_ENABLE_DEBUG_STRINGS
	, m_debugName( debugName )
#endif
{
	YC_UNUSED( debugName );
	ycAssert( ycIsPow2( itemAlignment ) );

	m_pages.Init( m_mem );
	m_descSize  = ycMax( itemAlignment, ycSize_t( sizeof(PageDesc) ) );
	m_itemSize  = ycRoundUpPow2( itemSize, itemAlignment );
	m_pageSize  = pageSizeInBytes;
	m_itemAlign = itemAlignment;
	m_numItems  = 0;
	m_pagesLength = 0;

	const ycSize_t itemSpace = pageSizeInBytes - m_descSize;
	m_numItemsPerPage = itemSpace / m_itemSize;
	ycAssert( m_numItemsPerPage > 0 );
}

ycPagedVectorBase::ycPagedVectorBase()
	: m_mem( nullptr )
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
	m_pagesLength = 0;
}

void ycPagedVectorBase::Init( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, const ycSize_t pageSizeInBytes, const ycSize_t itemAlignment )
{
	ycAssert( m_mem == nullptr );
	m_mem  = mem ? mem : g_defaultMem;
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif

	ycAssert( ycIsPow2( itemAlignment ) );
	m_pages.Init( m_mem );
	m_descSize  = ycMax( itemAlignment, ycSize_t( sizeof(PageDesc) ) );
	m_itemSize  = ycRoundUpPow2( itemSize, itemAlignment );
	m_pageSize  = pageSizeInBytes;
	m_itemAlign = itemAlignment;
	m_numItems  = 0;
	m_pagesLength = 0;

	const ycSize_t itemSpace = pageSizeInBytes - m_descSize;
	m_numItemsPerPage = itemSpace / m_itemSize;
	ycAssert( m_numItemsPerPage > 0 );
}

void* ycPagedVectorBase::AllocItem()
{
	// allocate a new page if necessary
	PageDesc* curPage = nullptr;
	if( m_pagesLength == 0 || m_pages[m_pagesLength-1]->numItems == m_numItemsPerPage )
	{
		m_pagesLength++;
		if( m_pagesLength > m_pages.Length() )
		{
			const char* debugName = "ycPagedVector";
			#ifdef YC_ENABLE_DEBUG_STRINGS
				debugName = m_debugName;
			#else
				YC_UNUSED( debugName );
			#endif
			curPage = (PageDesc*)YC_ALLOC( m_mem, m_pageSize, m_itemAlign, debugName );
			curPage->numItems = 0;
			m_pages.Append( curPage );
		}
		else
		{
			curPage = m_pages[m_pagesLength-1];
		}
	}
	else
	{
		curPage = m_pages[m_pagesLength-1];
	}

	void* dst = GetPageItem( m_pagesLength-1, curPage->numItems );
	++curPage->numItems;
	++m_numItems;
	return dst;
}

void* ycPagedVectorBase::GetItem( const ureg index )
{
	return GetPageItem( index / m_numItemsPerPage, index % m_numItemsPerPage );
}

void* ycPagedVectorBase::GetPageItem( const ureg pageIndex, const ureg localIndex )
{
	ycAssert( pageIndex < m_pagesLength );
	ycAssert( localIndex < m_numItemsPerPage );
	uint8_t* firstItem = (uint8_t*)m_pages[pageIndex] + m_descSize;
	return firstItem + m_itemSize*localIndex;
}

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycPagedVector_StressTest )
{
	ycAllocator* mem = ycTest::GetMem();
	struct Test
	{
		s32 a, b, c;
	};
	ycPagedVector< Test > list( "Test", mem );
	enum
	{
		kIterations = 1024*16
	};
	for( ureg i = 0; i != kIterations; ++i )
	{
		Test test;
		list.Append( test );
	}

	YC_TEST_PASS( "ycPagedVector stress test" );
}

#endif // YC_TEST
