#pragma once

#include "ycCommon.h"

//#define YC_VALIDATE_LIST	// validate the list whenever it is modified

/*
\class ycInlineList

Implements linked list where the elements are being managed through ycNode, the list does not manage the creation or deletion of nodes at all.
This list cannot contain non-pointer types; do not include the * in the template arguments, it is implied.
There is very little error checking in ycInlineList and its node/iterator.  It is very possible to corrupt a list by passing
incorrect arguments around.

*/

class ycAllocator;

template< class T >
class ycInlineList
{
public:

	void Check()
	{
#ifdef YC_VALIDATE_LIST
		if( m_head == nullptr )
		{
			ycAssert( GetSize() == 0 );
			ycAssert( m_tail == nullptr );
		}
		else
		{
			u32 count = 1;
			ycNode* node = m_head;
			ycNode* prev = nullptr;
			ycNode* last = node;
			while( node->m_next )
			{
				ycAssert( prev == node->m_prev );
				prev = node;
				node = node->m_next;
				if( node ) last = node;
				++count;
			}
			ycAssert( last == m_tail );
			ycAssert( count == GetSize() );
		}
#endif
	}

	/*!
	\class ycInlineList::ycNode

	Each list item should have a ycNode as a member.  At construction, the ycNode should be passed the pointer to the item.
	*/
	class ycNode
	{
	public:

		// you could use if( m_prev == m_next && m_prev != nullptr ) to flag something, at no extra memory cost

		inline ycNode() 
			: m_list( nullptr )
			, m_item( nullptr )
			, m_prev( nullptr )
			, m_next( nullptr )
		{}

		inline void Init( T* item )
		{
			m_item = item;
		}

		inline ycNode( T* item )
			: m_list( nullptr )
			, m_item( item )
			, m_prev( nullptr )
			, m_next( nullptr )
		{}

		//! If a ycNode is a member of a list, it will remove itself upon destruction.
		inline ~ycNode()
		{
			if( m_list )
			{
				m_list->Remove( this );
			}
		}

		inline T * GetNext() const
		{
			return m_next != nullptr ? m_next->m_item : nullptr;
		}

		inline T * GetPrev() const
		{
			return m_prev != nullptr ? m_prev->m_item : nullptr;
		}

//protected:
	//	template< class T > friend class ycInlineList;

		ycInlineList< T >*	m_list;
		T*				m_item;
		ycNode*			m_prev;
		ycNode*			m_next;
	};

	/*!
	\class ycInlineList::ycIter
	*/
	class ycIter
	{
		friend class ycInlineList<T>;
	public:

		//! Creates an invalid ycIterator
		/*
		An iterator created with this constructor can only be made into a valid iterator through assignment to a valid iterator.
		*/
		inline ycIter()
			: m_list( nullptr )
			, m_current( nullptr )
		{}

		//! Returns true if the ycIterator points to something
		inline bool IsValid() const
		{
			return m_current != nullptr;
		}

		//! Moves the ycIterator to the next list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		inline ycIter& operator++() // prefix
		{
			m_current = m_current->m_next;
			return *this;
		}

		inline ycIter operator++( int ) // postfix
		{
			ycIter prevIter( *this );
			m_current = m_current->m_next;
			return prevIter;
		}

		//! Moves the ycIterator to the previous list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		inline ycIter& operator--() // prefix
		{
			m_current = m_current->m_prev;
			return *this;
		}

		inline ycIter operator--( int ) // postfix
		{
			ycIter prevIter( *this );
			m_current = m_current->m_prev;
			return prevIter;
		}

		//! Returns a pointer to the current list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		inline T* GetItem() const
		{
			return m_current->m_item;
		}
		inline T* operator->() const
		{
			return GetItem();
		}
		inline T* operator*() const
		{
			return GetItem();
		}

		//! Returns the ycNode for the current list item
		/*!
		Direct access to the node should be rarely used, maybe for a situation such as deleting the node
		*/
		inline ycNode* GetNode() { return m_current; }

		//! Returns a pointer to the list that this iterator points to (regardless of whether it points to a valid item inside of that list)
		inline ycInlineList< T >* GetList() { return m_list; }
		inline const ycInlineList< T >* GetList() const { return m_list; }

	//protected:
	//	template< class T > friend class ycInlineList;

		//! Used internally by ycInlineList::Begin/End
		inline ycIter( ycInlineList< T >* list, ycNode* item )
			: m_list( list )
			, m_current( item )
		{}

	private:
		ycInlineList< T >*	m_list;
		ycNode*			m_current;
	};

	// Const interator
	class ycConstIter
	{
		friend class ycInlineList<T>;
	public:

		//! Creates an invalid ycIterator
		/*
		An iterator created with this constructor can only be made into a valid iterator through assignment to a valid iterator.
		*/
		inline ycConstIter()
			: m_list( nullptr )
			, m_current( nullptr )
		{}

		//! Returns true if the ycIterator points to something
		inline bool IsValid() const
		{
			return m_current != nullptr;
		}

		//! Moves the ycIterator to the next list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		inline ycConstIter& operator++() // prefix
		{
			m_current = m_current->m_next;
			return *this;
		}

		inline ycConstIter operator++( int ) // postfix
		{
			ycConstIter prevIter( *this );
			m_current = m_current->m_next;
			return prevIter;
		}

		//! Moves the ycIterator to the previous list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		inline ycConstIter& operator--() // prefix
		{
			m_current = m_current->m_prev;
			return *this;
		}

		inline ycConstIter operator--( int ) // postfix
		{
			ycConstIter prevIter( *this );
			m_current = m_current->m_prev;
			return prevIter;
		}

		//! Returns a pointer to the current list item
		/*!
		Using this when !IsValid() has undefined behavior and may crash with invalid memory access.
		*/
		inline const T* GetItem()
		{
			return m_current->m_item;
		}
		inline const T* operator->()
		{
			return GetItem();
		}
		inline const T* operator*()
		{
			return GetItem();
		}

		//! Returns the ycNode for the current list item
		/*!
		Direct access to the node should be rarely used, maybe for a situation such as deleting the node
		*/
		inline const ycNode* GetNode() { return m_current; }

		//! Returns a pointer to the list that this iterator points to (regardless of whether it points to a valid item inside of that list)
		inline const ycInlineList< T >* GetList() { return m_list; }

	//protected:
	//	template< class T > friend class ycInlineList;

		//! Used internally by ycInlineList::Begin/End
		inline ycConstIter( const ycInlineList< T >* list, const ycNode* item )
			: m_list( list )
			, m_current( item )
		{}

	private:
		const ycInlineList< T >*	m_list;
		const ycNode*		m_current;
	};

	//! Initializes an empty list
	inline ycInlineList()
		: m_head( nullptr )
		, m_tail( nullptr )
		, m_size( 0 )
	{}

	~ycInlineList() 
	{
		ycAssertMsg( m_head == nullptr && m_tail == nullptr && m_size == 0, "Inline Lists expect to be cleared before being destructed." );
	}

	//! A single ycNode CANNOT BE IN TWO LISTS! If you want to move a ycNode from one list to another, call Remove() first!
	inline void Prepend( ycNode* node )
	{
		Check();
		if( m_head == nullptr )
		{
			ycAssert( m_size == 0 );
			m_head = m_tail = node;
		}
		else
		{
			m_head->m_prev = node;
			node->m_next = m_head;
			m_head = node;
		}
		node->m_list = this;
		++m_size;
		Check();
	}

	//! *Moves* items from one list to the end of another
	/*!
	This WILL modify 'list', removing all items from it!
	*/
	inline void Prepend( ycInlineList< T >* list )
	{
		Check();
		for( ycInlineList< T >::ycIter iter = list->Begin(); iter.IsValid(); ++iter )
		{
			iter.m_current->m_list = this;
		}

		if( m_head == nullptr )
		{
			m_head = list->m_head;
			m_tail = list->m_tail;
			m_size = list->m_size;
		}
		else
		{
			m_head->m_prev = list->m_tail;
			list->m_tail->m_next = m_head;
			m_head = list->m_head;
			m_size += list->m_size;
		}

		list->m_head = list->m_tail = nullptr;
		list->m_size = 0;
		Check();
	}

	//! A single ycNode CANNOT BE IN TWO LISTS! If you want to move a ycNode from one list to another, call Remove() first!
	inline void Append( ycNode* node )
	{
		Check();
#ifdef YC_VALIDATE_LIST
		{
			bool found = false;
			for( ycInlineList< T >::ycIter iter = Begin(); iter.IsValid(); ++iter )
			{
				if( iter.GetNode() == node )
				{
					found = true;
					break;
				}
			}
			ycAssert( !found );
		}
#endif
		if( m_head == nullptr )
		{
			ycAssert( m_size == 0 );
			m_head = m_tail = node;
		}
		else
		{
			m_tail->m_next = node;
			node->m_prev = m_tail;
			m_tail = node;
		}
		node->m_list = this;
		++m_size;
		Check();
	}

	//! *Moves* items from one list to the end of another
	/*!
	This WILL modify 'list', removing all items from it!
	*/
	inline void Append( ycInlineList< T >* list )
	{
		if( list->GetSize() == 0 ) return;
		Check();
		for( ycInlineList< T >::ycIter iter = list->Begin(); iter.IsValid(); ++iter )
		{
			iter.m_current->m_list = this;
		}

		if( m_head == nullptr )
		{
			m_head = list->m_head;
			m_tail = list->m_tail;
			m_size = list->m_size;
		}
		else
		{
			m_tail->m_next = list->m_head;
			list->m_head->m_prev = m_tail;
			m_tail = list->m_tail;
			m_size += list->m_size;
		}

		list->m_head = list->m_tail = nullptr;
		list->m_size = 0;
		Check();
	}

	//! Removes a node from the list
	/*!
	This will NOT delete a node, only remove it from the list.
	*/
	inline void Remove( ycNode* node )
	{
		Check();
		if( node->m_list == nullptr )
		{
			return;
		}
#ifdef YC_VALIDATE_LIST
		{
			bool found = false;
			for( ycInlineList< T >::ycIter iter = Begin(); iter.IsValid(); ++iter )
			{
				if( iter.GetNode() == node )
				{
					found = true;
					break;
				}
			}
			ycAssert( found );
		}
#endif
		if( node == m_head )
		{
			m_head = node->m_next;
		}
		else
		{
			node->m_prev->m_next = node->m_next;
		}
		if( node == m_tail )
		{
			m_tail = node->m_prev;
		}
		else
		{
			node->m_next->m_prev = node->m_prev;
		}

		node->m_next = node->m_prev = nullptr;

		node->m_list = nullptr;

		ycAssert( m_size > 0 );

/*
#ifdef YC_ENABLE_ASSERT
		if( m_size == 1 )
		{
			ycAssert( m_head == nullptr && m_tail == nullptr );
		}
		else
		{
			ycAssert( m_head != nullptr && m_tail != nullptr );
		}
#endif
*/

		--m_size;

		if( m_head == nullptr )
		{
			ycAssert( m_size == 0 );
		}

		Check();
	}

	inline ycIter Remove( const ycIter& iterToRemove )
	{
		Check();
		ycIter nextIter = iterToRemove;
		++nextIter; 

		Remove( iterToRemove.m_current );
		Check();
		return nextIter;
	}

	//! Returns a pointer to the first item in the list
	inline T* GetFirst()
	{
		if(m_head)
			return m_head->m_item;
		return nullptr;
	}

	inline const T* GetFirst() const
	{
		if(m_head)
			return m_head->m_item;
		return nullptr;
	}

	//! Returns a pointer to the last item in the list
	inline T* GetLast()
	{
		if(m_tail)
			return m_tail->m_item;
		return nullptr;
	}
	
	inline const T* GetLast() const
	{
		if(m_tail)
			return m_tail->m_item;
		return nullptr;
	}

	//! Returns the number of items in the list
	inline u32 GetSize() const
	{
		return m_size;
	}

	//! Returns the number of elements in the vector
	inline ycSize_t Length() const { return m_size; }

	//! Returns an iterator pointing to the first item in the list
	inline ycIter Begin()
	{
		return ycIter( this, m_head );
	}

	inline ycConstIter ConstBegin() const
	{
		return ycConstIter( this, m_head );
	}

	//! Returns an iterator pointing to the last item in the list
	/*!
	This DOES NOT behave like std::list::end()!  The returned iterator points to the last item, not beyond it.
	*/
	inline ycIter End()
	{
		return ycIter( this, m_tail );//, m_size - 1 );
	}

	inline ycConstIter ConstEnd() const
	{
		return ycConstIter( this, m_tail );
	}

	//! Runs a common method on all list items
	// if you are getting compile errors on the line below, you are probably using ycInlineList< T* > instead of ycInlineList< T > (* is implied)
	typedef void ( T::*ForeachCallback )( void ); 
	inline void Foreach( ForeachCallback callback )
	{
		ycNode* entry = m_head;
		while( entry )
		{
			( *( entry->m_item ).*( callback ) )();
			entry = entry->m_next;
		}
	}

	//! Inserts a new node before a node that is already in the list
	inline void InsertBefore( ycNode* insertBefore, ycNode* node )
	{
		Check();
		node->m_prev = insertBefore->m_prev;
		node->m_next = insertBefore;

		if( insertBefore == m_head )
		{
			m_head = node;
		}
		else
		{
			node->m_prev->m_next = node;
		}

		insertBefore->m_prev = node;

		node->m_list = this;

		++m_size;
		Check();
	}

	//! Returns a pointer to the first item in the list and Remove()'s its node
	inline T* TakeFirst()
	{
		Check();
		if( m_head )
		{
			ycNode* first = m_head;
			Remove( first );
			Check();
			return first->m_item;
		}
		else
		{
			return nullptr;
		}
	}

	//! Returns a pointer to the last item in the list and Remove()'s its node
	inline T* TakeLast()
	{
		Check();
		if( m_tail )
		{
			ycNode* last = m_tail;
			Remove( last );
			Check();
			return last->m_item;
		}
		else
		{
			return nullptr;
		}
	}

	//! Returns true if the provided item is within this list.
	inline bool Contains( const T* pItem ) const
	{
		ycNode* pNode = m_head;
		while( pNode )
		{
			if(pNode->m_item == pItem)
				return true;
			pNode = pNode->m_next;
		}
		return false;
	}

	typedef bool (*ComparisonFunctionPtr)( const T* item1, const T* item2 );

	inline void InsertionSort( ComparisonFunctionPtr comparisonFunctionPtr )
	{
		ycNode* outer, *inner;
		outer = m_head;
		while( outer )
		{
			ycNode* tmp = outer->m_next;
			bool insert = false;
			inner = outer->m_prev;
			while( inner )
			{
				if( comparisonFunctionPtr( inner->m_item, outer->m_item ) ) //> 0
				{
					if( !insert )
					{
						break;
					}
					else
					{
						if( outer == m_tail )
						{
							m_tail = outer->m_prev;
						}

						outer->m_prev->m_next = outer->m_next;
						if( outer->m_next )
						{
							outer->m_next->m_prev = outer->m_prev;
						}

						outer->m_prev = inner;
						outer->m_next = inner->m_next;

						inner->m_next->m_prev = outer;
						inner->m_next = outer;

						insert = false;
						break;
					}
				}
				else
				{
					insert = true;
				}
				inner = inner->m_prev;
			}
			if( insert )
			{
				if( outer == m_tail )
				{
					m_tail = outer->m_prev;
				}

				outer->m_prev->m_next = outer->m_next;
				if( outer->m_next )
				{
					outer->m_next->m_prev = outer->m_prev;
				}

				m_head->m_prev = outer;
				outer->m_next = m_head;
				outer->m_prev = nullptr;
				m_head = outer;
			}
			outer = tmp;
		}
	}

	inline void MergeSort( ComparisonFunctionPtr comparisonFunctionPtr )
	{
		ycNode *p, *q, *e, *tail, *head;
		s32 insize, nmerges, psize, qsize, i;

		head = m_head;

		//if the head is empty, stop
		if( head == nullptr )
		{
			return;
		}

		insize = 1;

		while( 1 ) 
		{
			p = head;
			head = nullptr;
			tail = nullptr;

			//count the number of merges this pass
			nmerges = 0;

			while( p ) 
			{
				nmerges++;  /* there exists a merge to be done */
				// step `insize' places along from p
				q = p;
				psize = 0;
				for( i = 0; i < insize; i++ ) 
				{
					psize++;
					
					q = q->m_next;
					if( q == nullptr )
					{
						break;
					}
				}

				// if q hasn't fallen off end, we have two lists to merge
				qsize = insize;
				
				//now we have two lists; merge them
				while( psize > 0 || (qsize > 0 && q) ) 
				{
					// decide whether next element of merge comes from p or q
					if( psize == 0 ) 
					{
						// p is empty; e must come from q. 
						e = q; 
						q = q->m_next; 
						qsize--;
					} 
					else if( qsize == 0 || q == nullptr )
					{
						// q is empty; e must come from p.
						e = p; 
						p = p->m_next; 
						psize--;
					}
					else if( comparisonFunctionPtr( p->m_item, q->m_item ) )
					{
						// comparitor says p is lower; e must come from p
						e = p; 
						p = p->m_next; 
						psize--;
					} 
					else
					{
						// First element of q is lower; e must come from q. 
						e = q;
						q = q->m_next; 
						qsize--;
					}

					// add the next element to the merged list
					if( tail ) 
					{
						tail->m_next = e;
					} 
					else 
					{
						head = e;
					}

					// Maintain reverse pointers in a doubly linked list.
					e->m_prev = tail;

					tail = e;
				}

				// now p has stepped `insize' places along, and q has too
				p = q;
			}
		
		   tail->m_next = nullptr;

			// If we have done only one merge, we're finished.
			if( nmerges <= 1 )   // allow for nmerges==0, the empty list case 
			{
				m_head = head;
				m_tail = tail;
				return;
			}

			/* Otherwise repeat, merging lists twice the size */
			insize *= 2;
		}
	}

	inline void Swap( ycNode* lhs, ycNode* rhs )
	{
		if( lhs == rhs ) { return; }
		ycAssert( lhs->m_list == this );
		ycAssert( rhs->m_list == this );
		ycSwap(lhs->m_prev, rhs->m_prev);
		ycSwap(lhs->m_next, rhs->m_next);
		if( m_head == lhs ) { m_head = rhs; }
		else if( m_head == rhs ) { m_head = lhs; }
		if( m_tail == lhs ) { m_tail = rhs; }
		else if( m_tail == rhs ) { m_tail = lhs; }
	}

	//! Removes all objects from the list.
	inline void Clear()
	{
		while( m_size )
		{
			TakeFirst();
		}
	}

	//! Deletes and removes all objects from the list.
	inline void ClearDelete( ycAllocator* alloc )
	{
		T* pDel;
		while( m_size )
		{
			pDel = TakeFirst();
			ycdelete( alloc, pDel );
		}
	}

	//! Linearly searches for item. Slow! Don't use except in debug code!
	inline s32 IndexOf( const T * item ) const
	{
		s32 idx = 0;
		for( ycNode * entry = m_head; entry != nullptr; entry = entry->m_next, idx++ )
		{
			if( entry->m_item == item ) { return idx; }
		}
		return -1;
	}

	template<typename cmpType>
	inline s32 IndexOf( const cmpType& item ) const
	{
		s32 idx = 0;
		for( ycNode * entry = m_head; entry != nullptr; entry = entry->m_next, idx++ )
		{
			if( *entry->m_item == item ) { return idx; }
		}
		return -1;
	}

protected:
	ycNode*	m_head;
	ycNode*	m_tail;
	u32		m_size;
};