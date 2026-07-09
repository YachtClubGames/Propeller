#pragma once

#include "ycCommon.h"
#include "ycRand.h" //!< shuffle

/*
\class ycCircArray

This acts like a resizable array, with a maximum upper size.  Internally it is implemented as a
circular array so that operations on the front do not have significantly worse performance than
the back (vs something like a vector).

Call ToArray() to convert it to a contiguous array.  The pointer returned by ToArray() will
remain valid as long as the ycCircArray is not modified [exceptions: Append/TakeLast are OK].
*/

template< class t_type, ycSize_t t_maxItems = 32 >
class ycCircArray
{
public:

	//! Returns an item given an array-style index
	t_type& Get( const ycSize_t idx );
	const t_type& Get( const ycSize_t idx ) const;
	t_type&       operator[]( const ycSize_t index )       { return Get( index ); }
	const t_type& operator[]( const ycSize_t index ) const { return Get( index ); }

	//! Returns the number of items in the vector
	ycSize_t GetSize() const;
	ycSize_t Length() const { return GetSize(); }

	//! Returns the maximum number of items this vector can store
	ycSize_t GetMaxItems() const;

	//! Adds an item to the end of the array
	void Append( const t_type& item );

	//! Increases the length of the array and returns a pointer to the slot
	/*!
	Warning: the data at this pointer should be treated as garbage until populated!
	*/
	t_type* AppendUninitialized();

	//! Adds an item to the end of the array; if the array is full, overwrites the first element
	void AppendOverwrite( const t_type& item );

	//! Increases the length of the array (if full, cycles back to first element) and returns a pointer to the slot
	/*!
	Warning: the data at this pointer should be treated as garbage until populated!
	*/
	t_type* AppendOverwriteUninitialized();

	//! Adds an item to the beginning of the vector
	void Prepend( const t_type& item );

	//! Inserts an item into an arbitrary location
	//! * idx will be the items index after insertion, so inserting at 0 would be the same as Prepend(), but less efficient
	void InsertBefore( const ycSize_t idx, const t_type& item );

	//! Removes and returns the first item
	t_type TakeFirst();

	//! Removes and returns the last item
	t_type TakeLast();

	//! Removes and returns an arbitrary item.
	//! * It is better to use TakeLast()/TakeFirst() then Take() when possible
	t_type Take( ycSize_t idx );

	//! Removes an item and places either the first or last item in its place
	//! * This is more efficient than Take() if you don't mind re-ordering your vector, but less efficient than TakeFirst()/TakeLast()
	t_type TakeUnsorted( const ycSize_t idx );

	// NYI
	//! If necessary re-arranges the internal data structure, and returns a pointer that can be used as a standard c array
	//t_type* ToArray();

	//! Swaps the values of two items in the vector
	void Swap( const ycSize_t aIdx, const ycSize_t bIdx );

	//! Resets the vector to an empty state
	void Clear();

	//! Resets the array to an empty state without running destructors
	/*!
	- This should only be used for POD and/or if you really know what you're doing!
	*/
	void ClearPOD();

	//! Returns the index of the first found instance of this item in the vector, or -1 if not found
	int32_t IndexOf( const t_type& item );

	//! Randomly re-orders the contents of the vector
	void Shuffle();

	//! Returns a reference to the first item in the vector
	const t_type& First() const;
	t_type& First();

	//! Returns a reference to the last item in the vector
	const t_type& Last() const;
	t_type& Last();

	bool IsFull() const { return GetSize() == GetMaxItems(); };

	ycCircArray();
	ycCircArray( const ycCircArray& other );
	ycCircArray& operator = ( const ycCircArray& rhs );

protected:

	ycSize_t GetInternalIdx( const ycSize_t idx ) const;

	ycSize_t m_start;    // idx of the first item
	ycSize_t m_numItems; // number of items stored
	t_type   m_items[ t_maxItems ];
};

template< class t_type, ycSize_t t_maxItems >
t_type& ycCircArray< t_type, t_maxItems >::Get( const ycSize_t idx )
{	
	ycAssert( idx < GetSize() );
	return m_items[ GetInternalIdx( idx ) ];
}

template< class t_type, ycSize_t t_maxItems >
const t_type& ycCircArray< t_type, t_maxItems >::Get( const ycSize_t idx ) const
{
	ycAssert( idx < GetSize() );
	return m_items[ GetInternalIdx( idx ) ];
}

template< class t_type, ycSize_t t_maxItems >
ycSize_t ycCircArray< t_type, t_maxItems >::GetSize() const
{
	return m_numItems;
}

template< class t_type, ycSize_t t_maxItems >
ycSize_t ycCircArray< t_type, t_maxItems >::GetMaxItems() const
{
	return t_maxItems;
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::Append( const t_type& item )
{
	ycAssert( GetSize() < GetMaxItems() );
	if( GetSize() )
	{
		m_items[ GetInternalIdx( m_numItems ) ] = item;
		++m_numItems;
	}
	else
	{
		m_items[ m_start ] = item;
		m_numItems = 1;
	}
}

template< class t_type, ycSize_t t_maxItems >
t_type* ycCircArray< t_type, t_maxItems >::AppendUninitialized()
{
	ycAssert( GetSize() < GetMaxItems() );
	t_type* item = nullptr;
	if( GetSize() )
	{
		item = &m_items[ GetInternalIdx( m_numItems ) ];
		++m_numItems;
	}
	else
	{
		item = &m_items[ m_start ];
		m_numItems = 1;
	}
	return item;
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::AppendOverwrite( const t_type& item )
{
	if( GetSize() == GetMaxItems() )
	{
		m_items[ GetInternalIdx( m_numItems ) ] = item;
		if( ++m_start == t_maxItems ) { m_start = 0; }
	}
	else
	{
		Append( item );
	}
}

template< class t_type, ycSize_t t_maxItems >
t_type* ycCircArray< t_type, t_maxItems >::AppendOverwriteUninitialized()
{
	t_type* item = nullptr;
	if( GetSize() == GetMaxItems() )
	{
		item = &m_items[ GetInternalIdx( m_numItems ) ];
		if( ++m_start == t_maxItems ) { m_start = 0; }
	}
	else
	{
		item = AppendUninitialized();
	}
	return item;
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::Prepend( const t_type& item )
{
	ycAssert( GetSize() < GetMaxItems() );
	if( GetSize() )
	{
		if( m_start == 0 ) { m_start = t_maxItems - 1; } else { --m_start; }
		m_items[ m_start ] = item;
		++m_numItems;
	}
	else
	{
		Append( item );
	}
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::InsertBefore( const ycSize_t idx, const t_type& item )
{
	ycAssert( GetSize() < GetMaxItems() );
	ycSize_t moveItems = m_numItems - idx;
	ycSize_t moveIdx = GetInternalIdx( m_numItems );
	ycSize_t lastMoveIdx = moveIdx;
	while( moveItems )
	{
		if( moveIdx == 0 ) { moveIdx = m_numItems - 1; } else { --moveIdx; }
		--moveItems;
		m_items[ lastMoveIdx ] = m_items[ idx ];
		lastMoveIdx = idx;
	}
	m_items[ GetInternalIdx( idx ) ] = item;
	++m_numItems;
}

template< class t_type, ycSize_t t_maxItems >
t_type ycCircArray< t_type, t_maxItems >::TakeFirst()
{
	ycAssert( GetSize() > 0 );
	const ycSize_t oldStart = m_start;
	if( ++m_start == t_maxItems ) { m_start = 0; }
	--m_numItems;
	const t_type item = m_items[ oldStart ];
	m_items[ oldStart ] = t_type();
	return item;
}

template< class t_type, ycSize_t t_maxItems >
t_type ycCircArray< t_type, t_maxItems >::TakeLast()
{
	ycAssert( GetSize() > 0 );
	--m_numItems;
	const ycSize_t idx = GetInternalIdx( m_numItems );
	const t_type item = m_items[ idx ];
	m_items[ idx ] = t_type();
	return item;
}

template< class t_type, ycSize_t t_maxItems >
t_type ycCircArray< t_type, t_maxItems >::Take( ycSize_t idx )
{
	ycAssert( idx < GetSize() );
	idx = GetInternalIdx( idx );
	const t_type item = m_items[ idx ];
	ycSize_t lastIdx = idx;
	ycSize_t moveItems = m_numItems - idx - 1;
	while( moveItems )
	{
		if( ++idx == t_maxItems ) { idx = 0; }
		--moveItems;
		m_items[ lastIdx ] = m_items[ idx ];
		lastIdx = idx;
	}
	--m_numItems;
	m_items[ GetInternalIdx( m_numItems ) ] = t_type(); // i think idx or lastIdx already has the index we need...
	return item;
}

template< class t_type, ycSize_t t_maxItems >
t_type ycCircArray< t_type, t_maxItems >::TakeUnsorted( const ycSize_t idx )
{
	ycAssert( idx < GetSize() );
	if( idx == GetSize() - 1 )
	{
		return TakeLast();
	}
	else
	{
		const ycSize_t takeIdx = GetInternalIdx( idx );
		const t_type take = m_items[ takeIdx ];
		m_items[ takeIdx ] = TakeLast();
		return take;
	}
}

//template< class t_type, ycSize_t t_maxItems >
//t_type* ycCircArray< t_type, t_maxItems >::ToArray()
//{
//	if( m_numItems != 0 && m_start != 0 )
//	{
//		// if we are simply moving items down we can use a simple algorithm
//		if( m_start + m_numItems <= t_maxItems ) // existing array is offset, but does not circle around
//		{
//			u32 src      = m_start;
//			u32 dst      = 0;
//			u32 numItems = m_numItems;
//			do
//			{
//				ycAssert( src != 0 );
//				m_items[ dst ] = m_items[ src ];
//				++dst;
//				++src;
//				--numItems;
//			} while( numItems );
//			do
//			{
//				m_items[ dst ] = t_type();
//				++dst;
//				--m_start;
//			} while( m_start );
//		}
//		// otherwise it gets a little trickier...
//		else
//		{
//			ycFatal(); // NYI
//			//u32 numItems = m_numItems;
//			//u32 move     = 0;
//			//u32 dst      = 0;
//			//t_type tmp;
//			//do
//			//{
//			//	u32 moveIdx = GetInternalIdx( move );
//			//	tmp = m_items[ dst ];
//			//	m_items[ dst ] = m_items[ moveIdx ];
//			//	move = dst < m_start ?  :  ;
//			//	++dst;
//			//	--numItems;
//			//} while( numItems );
//			//u32 remain = t_maxItems - m_numItems;
//			//while( remain )
//			//{
//			//	m_items[ dst ] = t_type();
//			//	++dst;
//			//	--remain;
//			//}
//			//m_start = 0;
//		}
//	}
//	return &m_items[0];
//}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::Swap( const ycSize_t aIdx, const ycSize_t bIdx )
{
	const t_type tmp = Get( aIdx );
	Get( aIdx ) = Get( bIdx );
	Get( bIdx ) = tmp;
}

template< class t_type, ycSize_t t_maxItems >
ycCircArray< t_type, t_maxItems >::ycCircArray() :
	m_start( 0 ),
	m_numItems( 0 )
{}

template< class t_type, ycSize_t t_maxItems >
ycCircArray< t_type, t_maxItems >::ycCircArray( const ycCircArray& other )
{
	u32 i = 0;
	for( /**/; i != other.GetSize(); ++i )
	{
		m_items[ i ] = other[ i ];
	}
	for( /**/; i != t_maxItems; ++i )
	{
		m_items[ i ] = t_type();
	}
	m_start = other.m_start;
	m_numItems = other.m_numItems;
}

template< class t_type, ycSize_t t_maxItems >
ycCircArray< t_type, t_maxItems >& ycCircArray< t_type, t_maxItems >::operator = ( const ycCircArray& rhs )
{
	u32 i = 0;
	for( /**/; i != rhs.GetSize(); ++i )
	{
		m_items[ i ] = rhs[ i ];
	}
	for( /**/; i != t_maxItems; ++i )
	{
		m_items[ i ] = t_type();
	}
	m_start = 0;
	m_numItems = rhs.GetSize();
	return *this;
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::Clear()
{
	m_start = 0;
	m_numItems = 0;
	for( size_t i = 0; i != t_maxItems; ++i )
	{
		m_items[ i ] = t_type();
	}
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::ClearPOD()
{
	m_start = 0;
	m_numItems = 0;
}

template< class t_type, ycSize_t t_maxItems >
int32_t ycCircArray< t_type, t_maxItems >::IndexOf( const t_type& item )
{
	const int32_t size = ( int32_t )GetSize();
	for( int32_t idx = 0; idx < size; ++idx )
	{
		if( Get( idx ) == item )
		{
			return idx;
		}
	}

	return -1;
}

template< class t_type, ycSize_t t_maxItems >
void ycCircArray< t_type, t_maxItems >::Shuffle()
{
	ycRand rng;
	// http://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#The_modern_algorithm
	for( ycSize_t i = m_numItems - 1; i >= 1; i-- )
	{
		ycSize_t randIdx = rng.RangeFast( 0, i );
		Swap( i, randIdx );
	}
}

template< class t_type, ycSize_t t_maxItems >
const t_type& ycCircArray< t_type, t_maxItems >::First() const
{
	return Get( 0 );
}

template< class t_type, ycSize_t t_maxItems >
t_type& ycCircArray< t_type, t_maxItems >::First()
{
	return Get( 0 );
}

template< class t_type, ycSize_t t_maxItems >
const t_type& ycCircArray< t_type, t_maxItems >::Last() const
{
	return Get( GetSize() - 1 );
}

template< class t_type, ycSize_t t_maxItems >
t_type& ycCircArray< t_type, t_maxItems >::Last()
{
	return Get( GetSize() - 1 );
}

template< class t_type, ycSize_t t_maxItems >
ycSize_t ycCircArray< t_type, t_maxItems >::GetInternalIdx( const ycSize_t idx ) const
{
	return ( m_start + idx ) % t_maxItems;
}
