#pragma once
#ifndef YC_PAGEDLIST_H
#define YC_PAGEDLIST_H

#include "ycAllocator.h"
#include "ycCommon.h"
#include "ycStd.h"

/*! \class ycPagedList
A variable-length list, allocated in pages, that provides fast iteration, appending, prepending, deletion, and
generally-fast insertion.  Random access is slow.

Deletion will become slower the more pages there are, as it must find the page the allocation is on first.

Implementation Notes:

If all items on a page are deleted, the page will be deleted. Under some usage patterns it may be possible to add many
items, remove most of them, but still have many pages allocated. If you see that memory utilization on this data
structure is poor you should ... TODO HACK FIXME: add a Compact() and also maybe an incremental defragmenter.

TODO HACK FIXME MAYBE: a sparse version of this would be useful (bitmap per page?)
*/

class ycPagedListBase : public ycNoCopy
{
public:

	ycSize_t Length() const { return m_numItems; }

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
		IterBase( ycPagedListBase* list, void* page, ureg localIndex = 0 );

		void*            m_page;
		ureg             m_localIndex;
		ycPagedListBase* m_list; // does this need to know its list?
		template <typename t_type> friend class ycPagedList;
	};

	ycPagedListBase( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, ycSize_t pageSizeInBytesOrCount, const ycSize_t itemAlignment );
	ycPagedListBase();
	void Init( const char* debugName, const ycSize_t itemSize, ycAllocator* mem, ycSize_t pageSizeInBytesOrCount, const ycSize_t itemAlignment );
	~ycPagedListBase() {} // DOES NOTHING! DOES NOT FREE MEMORY! TEMPLATE IS RESPONSIBLE FOR THIS
	void Swap( ycPagedListBase* list );

	void* AllocItem( bool unsorted = false );
	//void* TakeLast();

	struct PageDesc
	{
		PageDesc* prev; // these ptrs really shouldn't be together... next should be at the end of the page and prev at the beginning... (more wasteful with large alignments though....)
		PageDesc* next;
		ycSize_t  numItems;
	};

	void* GetPageItem( PageDesc* page, const ureg localIndex );
	PageDesc* AllocPage();
	void FreePage( PageDesc* page );

	ycSize_t m_descSize; //!< padded to m_itemAlign
	ycSize_t m_itemSize; //!< padded to m_itemAlign
	ycSize_t m_pageSize;
	ycSize_t m_numItemsPerPage;
	ycSize_t m_itemAlign;
	ycSize_t m_numItems;

	PageDesc* m_head;
	PageDesc* m_tail;

	ycAllocator* m_mem;
#ifdef YC_ENABLE_DEBUG_STRINGS
	const char* m_debugName;
#endif
};

template< class t_type >
class ycPagedList : public ycPagedListBase
{
public:
	ycPagedList( const char* debugName, ycAllocator* mem = nullptr, const ycSize_t pageSizeInBytesOrCount = YC_MEM_DEFAULT_PAGE_SIZE );
	ycPagedList() {}
	void Init( const char* debugName, ycAllocator* mem = nullptr, const ycSize_t pageSizeInBytesOrCount = YC_MEM_DEFAULT_PAGE_SIZE )
	{
		ycPagedListBase::Init( debugName, sizeof( t_type ), mem, pageSizeInBytesOrCount, alignof( t_type ) );
	}
	~ycPagedList() { Clear(); }

	ycPagedList( ycPagedList< t_type >&& list )
	{
		Swap( &list );
	}
	ycPagedList& operator= ( ycPagedList< t_type >&& list )
	{
		if( &list == this ) { return *this; }
		Clear();
		Swap( &list );
		return *this;
	}

	t_type* Append()
	{
		return (t_type*)AllocItem();
	}

	t_type* AppendUnsorted()
	{
		return (t_type*)AllocItem( true );
	}

	t_type* AppendInitialized()
	{
		t_type* result = (t_type*)AllocItem();
		new( result ) t_type();
		return result;
	}

	t_type* AppendInitializedUnsorted()
	{
		t_type* result = (t_type*)AllocItem( true );
		new(result) t_type();
		return result;
	}

	t_type* Append( const t_type& item )
	{
		t_type* dst = (t_type*)AllocItem();
		*dst = item;
		return dst;
	}

	t_type* AppendUnsorted( const t_type& item )
	{
		t_type* dst = (t_type*)AllocItem( true );
		*dst = item;
		return dst;
	}

	template< typename ... t_args > t_type& EmplaceBack( t_args&&... args );

	bool Contains( const t_type& val ) const
	{
		for( PageDesc* page = m_head; page != nullptr; page = page->next )
		{
			const uint8_t* item = reinterpret_cast< uint8_t* >( page ) + m_descSize;
			for( ureg i = 0; i != page->numItems; ++i )
			{
				const t_type& cmp = *reinterpret_cast< const t_type* >( item );
				if( cmp == val ) { return true; }
				item += m_itemSize;
			}
		}
		return false;
	}

	void Remove( const t_type& val )
	{
		ycAssert( m_numItems );
		for( PageDesc* page = m_head; page != nullptr; page = page->next )
		{
			uint8_t* item = reinterpret_cast< uint8_t* >( page ) + m_descSize;
			for( ureg i = 0; i != page->numItems; ++i )
			{
				const t_type& cmp = *reinterpret_cast< t_type* >( item );
				if( cmp == val )
				{
					Take( page, i );
					return;
				}
				item += m_itemSize;
			}
		}
	}

	void Take( IterBase& iter )
	{
		PageDesc* page = (PageDesc*)iter.m_page;
		PageDesc* next = page->next;
		bool pageFreed = Take( page, iter.m_localIndex );
		if( pageFreed || iter.m_localIndex >= page->numItems )
		{
			iter.m_page = next;
			iter.m_localIndex = 0;
		}
	}

	void TakeUnsorted( IterBase& iter )
	{
		PageDesc* page = (PageDesc*)iter.m_page;
		PageDesc* next = page->next;
		bool pageFreed = TakeUnsorted( page, iter.m_localIndex );
		if( pageFreed || iter.m_localIndex >= page->numItems )
		{
			iter.m_page = next;
			iter.m_localIndex = 0;
		}
	}

	bool IsLast( IterBase& iter )
	{
		PageDesc* page = (PageDesc*)iter.m_page;
		ureg localIndex = iter.m_localIndex;
		t_type* arrayBase = reinterpret_cast<t_type*>(reinterpret_cast<uint8_t*>(page) + m_descSize);
		t_type* tailArrayBase = reinterpret_cast<t_type*>(reinterpret_cast<uint8_t*>(m_tail) + m_descSize);
		t_type* item = arrayBase + localIndex;
		t_type* tailLastItem = tailArrayBase + m_tail->numItems - 1;
		if( tailLastItem == item )
		{
			return true;
		}
		return false;
	}

	void Clear()
	{
		PageDesc* page = m_head;
		while( page != nullptr )
		{
			uint8_t* item = reinterpret_cast< uint8_t* >( page ) + m_descSize;
			for( ureg i = 0; i != page->numItems; ++i )
			{
				reinterpret_cast< t_type* >( item )->~t_type();
				item += m_itemSize;
			}
			PageDesc* next = page->next;
			YC_FREE( m_mem, page );
			page = next;
		}
		m_numItems = 0;
		m_head     = nullptr;
		m_tail     = nullptr;
	}

	t_type TakeLast()
	{
		ycAssert( m_numItems && m_tail && m_tail->numItems );
		t_type ret = *( (t_type*)GetPageItem( m_tail, --m_tail->numItems ) );
		*( (t_type*)GetPageItem( m_tail, m_tail->numItems ) ) = t_type();
		--m_numItems;
		if( m_tail->numItems == 0 )
		{
			FreePage( m_tail );
		}
		return ret;
	}

	t_type* Last()
	{
		ycAssert( m_numItems && m_tail && m_tail->numItems );
		return (t_type*)GetPageItem( m_tail, m_tail->numItems );
	}

	class Iter : public IterBase
	{
	public:

		Iter();
		//Iter( ycPagedList* list ) : IterBase( list ) {}
		Iter( const Iter& copy ) : IterBase( copy ) {}

		//! Returns a pointer to the current list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		t_type* GetItem()    { return (t_type*)IterBase::GetItem(); }
		t_type* operator->() { return GetItem(); }
		t_type* operator*()  { return GetItem(); }

	protected:
		friend class ycPagedList;

		inline Iter( ycPagedList* list ) : IterBase( list, list->m_head, 0 ) {}
	};

	Iter Begin()
	{
		return Iter( this );
	}

private:
	// returns true if provided page was freed
	bool Take( PageDesc* page, ureg localIndex )
	{
		ureg lastItemIdx = page->numItems - 1;
		uint8_t* item = reinterpret_cast<uint8_t*>(page) + m_descSize + m_itemSize * localIndex;

		// move any items after this down the page
		for( /**/; localIndex != lastItemIdx; ++localIndex )
		{
			*reinterpret_cast<t_type*>(item) = *reinterpret_cast<t_type*>(item + m_itemSize);
			item += m_itemSize;
		}

		// default the last item
		*reinterpret_cast<t_type*>(item) = t_type();

		// update counts
		--page->numItems;
		--m_numItems;

		if( page->numItems == 0 )
		{
			FreePage( page );
			return true;
		}

		return false;
	}

	// returns true if provided page was freed
	bool TakeUnsorted( PageDesc* page, ureg localIndex )
	{
		ycAssert( m_tail->numItems > 0 );
		const bool wasTail = page == m_tail;
		t_type* arrayBase = reinterpret_cast<t_type*>(reinterpret_cast<uint8_t*>(page) + m_descSize);
		t_type* tailArrayBase = reinterpret_cast<t_type*>(reinterpret_cast<uint8_t*>(m_tail) + m_descSize);
		t_type* item = arrayBase + localIndex;
		t_type* tailLastItem = tailArrayBase + m_tail->numItems - 1;

		// swap tail's last item into this item's spot
		if( item != tailLastItem )
		{
			*item = YC_MOVE( *tailLastItem );
		}

		// default the last item
		*tailLastItem = t_type();

		// update counts
		--m_tail->numItems;
		--m_numItems;

		if( m_tail->numItems == 0 )
		{
			FreePage( m_tail );
			return wasTail;
		}

		return false;
	}
};

template< class t_type >
ycPagedList< t_type >::ycPagedList( const char* debugName, ycAllocator* mem, const ycSize_t pageSizeInBytesOrCount /*= YC_MEM_DEFAULT_PAGE_SIZE*/ ) :
	ycPagedListBase( debugName, sizeof(t_type), mem ? mem : g_defaultMem, pageSizeInBytesOrCount, alignof( t_type ) )
{
}

template< class t_type >
template< typename ... t_args >
t_type& ycPagedList< t_type >::EmplaceBack( t_args&&... args )
{
	t_type* ptr = Append();
	new(ptr) t_type( YC_FORWARD( args )... );
	return *ptr;
}

#endif // !YC_PAGEDLIST_H