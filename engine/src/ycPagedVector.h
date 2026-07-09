#pragma once

#include "ycAllocator.h"
#include "ycCommon.h"
#include "ycStd.h"
#include "ycVector.h"

/*! \class ycPagedVector
A variable-length list, allocated in pages, that provides fast iteration, appending, prepending, deletion, and
generally-fast insertion.  Random access is slow.

Implementation Notes:

If all items on a page are deleted, the page will be deleted. Under some usage patterns it may be possible to add many
items, remove most of them, but still have many pages allocated. If you see that memory utilization on this data
structure is poor you should ... TODO HACK FIXME: add a Compact() and also maybe an incremental defragmenter.

TODO HACK FIXME MAYBE: a sparse version of this would be useful (bitmap per page?)
*/

class ycPagedVectorBase : public ycNoCopy
{
public:

	ycSize_t Length() const { return m_numItems; }
	bool IsNull() const { return m_mem == nullptr; }

protected:
	
	/*! \class Iterator
	* Beware some methods invalidate iterators!
	*/
	class IterBase
	{
	public:

		IterBase();
		//IterBase( ycPagedListBase* list );
		IterBase( const IterBase& copy );

		//! Returns true if the IterBase points to something
		bool IsValid() const;

		//! Moves the IterBase to the next list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		IterBase& operator++(); // prefix
		IterBase operator++( int /*unused*/ ); // postfix

		//! Moves the IterBase to the previous list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		IterBase& operator--(); // prefix
		IterBase operator--( int /*unused*/ ); // postfix

		//! Returns a pointer to the current list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		void* GetItem();
		inline void* operator->() { return GetItem(); }
		inline void* operator*() { return GetItem(); }

	protected:
		IterBase( ycPagedVectorBase* list, ureg pageIndex, ureg localIndex = 0 );

		ureg             m_pageIndex;
		ureg             m_localIndex;
		ycPagedVectorBase* m_list; // does this need to know its list?
	};

	ycPagedVectorBase( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, const ycSize_t pageSizeInBytes, const ycSize_t itemAlignment );
	ycPagedVectorBase();
	void Init( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, const ycSize_t pageSizeInBytes, const ycSize_t itemAlignment );
	~ycPagedVectorBase() {} // DOES NOTHING! DOES NOT FREE MEMORY! TEMPLATE IS RESPONSIBLE FOR THIS

	void* AllocItem();
	//void* TakeLast();

	struct PageDesc
	{
		ycSize_t  numItems;
	};

	void* GetItem( const ureg index );
	void* GetPageItem( const ureg pageIndex, const ureg localIndex );

	ycSize_t m_descSize; //!< padded to m_itemAlign
	ycSize_t m_itemSize; //!< padded to m_itemAlign
	ycSize_t m_pageSize;
	ycSize_t m_numItemsPerPage;
	ycSize_t m_itemAlign;
	ycSize_t m_numItems;

	ycVector< PageDesc* > m_pages;
	ycSize_t m_pagesLength;
	ycAllocator* m_mem;
#ifdef YC_ENABLE_DEBUG_STRINGS
	const char* m_debugName;
#endif
};

template< class t_type >
class ycPagedVector : public ycPagedVectorBase
{
public:

	class Iter : public IterBase
	{
	public:

		Iter();
		//Iter( ycPagedVector* list ) : IterBase( list ) {}
		Iter( const Iter& copy ) : IterBase( copy ) {}

		//! Returns a pointer to the current list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		t_type* GetItem() { return (t_type*)IterBase::GetItem(); }
		t_type* operator->() { return GetItem(); }
		t_type* operator*() { return GetItem(); }

	protected:
		friend class ycPagedVector;

		inline Iter( ycPagedVector* list ) : IterBase( list, 0, 0 ) {}
	};

	Iter Begin()
	{
		return Iter( this );
	}

	ycPagedVector( const char* debugName, ycAllocator* mem = nullptr, const ycSize_t pageSizeInBytes = YC_MEM_DEFAULT_PAGE_SIZE );
	ycPagedVector() {}
	void Init( const char* debugName, ycAllocator* mem = nullptr, const ycSize_t pageSizeInBytes = YC_MEM_DEFAULT_PAGE_SIZE )
	{
		ycPagedVectorBase::Init( debugName, sizeof( t_type ), mem, pageSizeInBytes, alignof( t_type ) );
	}
	~ycPagedVector() { Clear(); }

	t_type* Append()
	{
		return (t_type*)AllocItem();
	}

	t_type* Append( const t_type& item )
	{
		t_type* dst = (t_type*)AllocItem();
		*dst = item;
		return dst;
	}

	void Clear( bool freeMem = true )
	{
		if( freeMem )
		{
			for( ycSize_t i = 0; i < m_pages.Length(); ++i )
			{
				YC_FREE( m_mem, m_pages[i] );
			}
			m_pages.Clear( freeMem );
		}
		else
		{
			for( ycSize_t i = 0; i < m_pagesLength; ++i )
			{
				m_pages[i]->numItems = 0;
			}
		}
		m_pagesLength = 0;
		m_numItems = 0;
	}

	t_type TakeUnsorted( const ycSize_t pageIndex, const ycSize_t itemIndex )
	{
		ycAssert( pageIndex < m_pages.Length() );
		ycAssert( itemIndex < m_pages[ pageIndex ]->numItems );
		ycSwap( At( pageIndex, itemIndex ), At( m_pages.Length() - 1, m_pages[ pageIndex ]->numItems - 1 ) );
		m_pages[ pageIndex ]->numItems--;
		m_numItems--;
		return t_type( YC_MOVE( At( m_pagesLength - 1, m_pages[ pageIndex ]->numItems ) ) );
	}

	t_type TakeUnsorted( const ycSize_t idx )
	{
		return TakeUnsorted( idx / m_numItemsPerPage, idx % m_numItemsPerPage );
	}

	t_type TakeUnsorted( const Iter& iter )
	{
		return TakeUnsorted( iter.m_pageIndex, iter.m_localIndex );
	}

	t_type&       operator[]( const ycSize_t index )       { return *((t_type*)GetItem( index )); }
	const t_type& operator[]( const ycSize_t index ) const { return *((t_type*)GetItem( index )); }
	t_type&       At( const ycSize_t index )       { return *((t_type*)GetItem( index )); }
	const t_type& At( const ycSize_t index ) const { return *((t_type*)GetItem( index )); }
	t_type&       At( const ycSize_t pageIndex, const ycSize_t index )       { return *((t_type*)GetPageItem( pageIndex, index )); }
	const t_type& At( const ycSize_t pageIndex, const ycSize_t index ) const { return *((t_type*)GetPageItem( pageIndex, index )); }
};

template< class t_type >
ycPagedVector< t_type >::ycPagedVector( const char* debugName, ycAllocator* mem, const ycSize_t pageSizeInBytes /*= YC_MEM_DEFAULT_PAGE_SIZE*/ ) :
	ycPagedVectorBase( debugName, sizeof(t_type), mem ? mem : g_defaultMem, pageSizeInBytes, alignof( t_type ) )
{
}