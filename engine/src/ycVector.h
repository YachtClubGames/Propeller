
#pragma once

#include "ycAllocator.h"
#include "ycCommon.h"
#include "ycInternalBufferAllocator.h"
#include "ycIsPOD.h"
#include "ycMath.h"
#include "ycStd.h"

/*
\class ycVectorRef

Implements a reference to some section of an array, with stl-like accessors and iterators
*/
template< class t_type >
class ycVectorRef
{
public:
	typedef t_type*       Iter;
	typedef const t_type* ConstIter;

	//! Constructs a ref to a null vector
	ycVectorRef();

	//! Constructs a VectorRef using an array
	template< size_t t_len > ycVectorRef( t_type( &arr )[ t_len ] ) : m_buf( arr ), m_len( t_len ) {}

	//! Constructs a VectorRef using a ptr and size
	ycVectorRef( t_type* arr, size_t len ) : m_buf( arr ), m_len( len ) {}

	//! Constructs a VectorRef from begin/end pointers
	ycVectorRef( t_type* begin, t_type* end ) : m_buf( begin ), m_len( end-begin ) {}
		//! Returns a pointer to the first element in the vector
	Iter      Begin()       { return m_buf; }
	ConstIter Begin() const { return m_buf; }

	//! Returns a pointer past the last element in the vector
	Iter      End()       { return m_buf + m_len; }
	ConstIter End() const { return m_buf + m_len; }

	//! STL-compatible versions to support "for(x : list)" constructs.
	Iter      begin()       { return m_buf; }
	ConstIter begin() const { return m_buf; }
	Iter      end()       { return m_buf + m_len; }
	ConstIter end() const { return m_buf + m_len; }
	
	//! Returns the number of elements in the vector
	ycSize_t Length() const { return m_len; }

	//! Returns a reference to an element in the vector
	t_type&       operator[]( const ycSize_t index )       { ycAssert( index < m_len ); return m_buf[ index ]; }
	const t_type& operator[]( const ycSize_t index ) const { ycAssert( index < m_len ); return m_buf[ index ]; }
	t_type&       At( const ycSize_t index )       { ycAssert( index < m_len ); return m_buf[ index ]; }
	const t_type& At( const ycSize_t index ) const { ycAssert( index < m_len ); return m_buf[ index ]; }
	
	//! Searches the vector for an element, returns -1 if the element was not found
	int IndexOf( const t_type& val ) const;
	int LastIndexOf( const t_type& val ) const;

	ycSize_t ExactIndexOf( const t_type* ptr ) const;

	//! Returns a reference to the first item in the vector
	t_type& First();
	const t_type& First() const;

	//! Returns a reference to the last item in the vector
	t_type& Last();
	const t_type& Last() const;

	//! Returns the size in bytes of an element of the vector
	ycSize_t GetElementSize() const { return sizeof( t_type ); }

	//! Returns the underlying buffer.
	t_type* GetData() const { return m_buf; }

	ycVectorRef Left( const ycSize_t len ) const; //!< returns a vector ref to the left (beginning) N elements of the vector ref; asserts if too many elements are requested
	ycVectorRef Right( const ycSize_t len ) const; //!< returns a vector ref to the right (ending) N elements of the vector ref; asserts if too many elements are requested
	
	ycVectorRef< const t_type > RefConst() const { return { m_buf, m_len }; }

protected:
	t_type*      m_buf;
	ycSize_t     m_len;
};

/*
\class ycVector

Implements a dynamically-sized array.

Set t_isPod to true if your data type does not need ctors/dtors/assignment operators.

***Be very careful with these, they can cause hard-to-predict memory allocation patterns***

TODO:
- Compact/Squeeze/whatever
- Smarter allocation (ideally optional 'allocation strategy' parameter)
- move ctor
*/

template< class t_type, const bool t_isPOD = ycIsPod<t_type>::val >
class ycVector : public ycVectorRef<t_type>
{
public:
	using Iter = typename ycVectorRef<t_type>::Iter;
	using ConstIter = typename ycVectorRef<t_type>::ConstIter;

	using ycVectorRef<t_type>::m_buf;
	using ycVectorRef<t_type>::m_len;

	using ycVectorRef<t_type>::Length;
	using ycVectorRef<t_type>::GetData;
	using ycVectorRef<t_type>::First;
	using ycVectorRef<t_type>::Last;
	using ycVectorRef<t_type>::At;

	//! Constructs a useless vector, should be populated by the copy ctor, assignment, or Init() later!
	ycVector();
	bool IsNull() const { return m_mem == nullptr; } //!< returns true if a vector was initialized with the blank ctor, and is useless
	ycAllocator* GetAllocator() const { return m_mem; }

	//! Constructs a new vector
	/*!
	* The vector will not allocate into the heap immediately, but only when elements are added.
	* The copy constructor will duplicate the original and the elements will now occur twice in memory.
	*/
	explicit ycVector( ycAllocator* mem, const ureg initialReserve = 0 );
	void Init( ycAllocator* mem, const ureg initialReserve = 0 );
	ycVector( const ycVector& vec );
	ycVector( ycVector&& vec ) noexcept;

	~ycVector();

	//! Swaps the internal state of two vectors
	void Swap( ycVector* other );

	//! Destroys all elements in the vector and optionally frees the internal buffer
	void Clear( const bool freeMem = true );

	//! Resets the vector to an empty state without running destructors
	/*!
	- This should only be used for POD and/or if you really know what you're doing!
	*/
	void ClearPOD( const bool freeMem = true );

	//! Pre-allocates a buffer for a number of elements
	/*!
	* If numItems is smaller than the current buffer size, this does nothing.
	*/
	void Reserve( const ycSize_t numItems );

	//! Shrinks or expands the vector
	/*!
	* If the requested number of items is less than the vector is currently storing, the extra memory is not freed.
	*/
	void Resize( const ycSize_t numItems );

	//! Duplicates another vector
	void Copy( const ycVector& vec, const bool clearFreeMem = true );

	//! Adds a new item to the end of the vector
	/*!
	* If the internal buffer is not full, this is very cheap
	* If the internal buffer is full, a new one will be reallocated and the contents of the old
	  buffer will be copied before the new element is appended.
	*/
	ycSize_t Append( const t_type& item );
	ycSize_t Append( t_type&& item );
	void PushBack( const t_type& item ) { Append( item ); }
	template< typename ... t_args > t_type& EmplaceBack( t_args&&... args );

	t_type* Append();

	void AppendUnique( const t_type& item );

	void Append( const ycVectorRef< t_type >& other );

	//! Adds a new item at an arbitrary position in the vector
	/*!
	* index denotes the desired index of the inserted element.  For example index 0 would insert
	  the element at the beginning of the vector.
	* This requires moving any elements that occur after the specified index at the very least,
	  and possibly a full reallocation if the internal buffer is not large enough to contain a
	  new element.
	*/
	void Insert( const t_type& item, const ycSize_t index );

	//! Removes and returns the first element of the vector
	/*!
	* This does not shrink the internal buffer.
	* This will require moving all elements after the first.
	*/
	t_type TakeFirst();
	
	//! Removes and returns the last element of the vector
	/*!
	* This does not shrink the internal buffer.
	*/
	t_type TakeLast();
	t_type PopBack() { return TakeLast(); }

	//! Removes an arbitrary element from the vector
	/*!
	* This does not shrink the internal buffer.
	*/
	t_type Take( const ycSize_t idx );

	//! Removes an arbitrary element from the vector by iterator and returns next iterator
	/*!
	* This does not shrink the internal buffer.
	*/
	Iter Take( const Iter i );

	//! Removes an item and places the last item in its place.
	//! * This is more efficient than Take() if you don't mind re-ordering your vector
	t_type TakeUnsorted( const ycSize_t idx );

	//! Reverses the order of the elements in the vector
	void Reverse();

	//! Reduces the memory used to the size of elements, possibly copying
	void ShrinkToFit();

	//! Duplicates another ycVector
	ycVector& operator=( const ycVector& vec );
	ycVector& operator=( ycVector&& vec ) noexcept;

	bool IsFull() const { return m_len == m_cap; }
	bool IsEmpty() const { return m_len == 0; }

	ycSize_t GetCapacity() const { return m_cap; }

protected:

	ycSize_t     m_cap;
	ycAllocator* m_mem;

	ycSize_t GetGrowSize()
	{
		if( m_len + 1 > m_cap )
		{
			return m_cap? m_cap * 2 : 8;
		}
		return m_cap;
	}

	t_type* ReserveInternal( ycSize_t numItems, bool shrink = false );

	void RebuildVector(
		t_type* sectionA, ycSize_t sectionACount,
		const t_type* newItems, ycSize_t newItemsCount,
		t_type* sectionC, ycSize_t sectionCCount,
		ycSize_t capacity, bool shrink = false );

	void RebuildVectorPOD(
		t_type* sectionA, ycSize_t sectionACount,
		const t_type* newItems, ycSize_t newItemsCount,
		t_type* sectionC, ycSize_t sectionCCount,
		ycSize_t capacity, bool shrink = false );

};

template< class t_type, ycSize_t t_count >
class ycStackVectorContainer   // inheritance used here to cause ycVector destructor to be called first, then the InternalBuffer
{
protected:
	ycStackVectorContainer( ycAllocator* mem ) : m_stackAlloc( "Stack Vector Allocator", mem ) {}

	ycInternalBufferAllocator< sizeof( t_type )* t_count, alignof( t_type ) > m_stackAlloc;
};

template< class t_type, ycSize_t t_count, bool t_isPOD = ycIsPod<t_type>::val >
class ycStackVector : public ycStackVectorContainer<t_type, t_count>, public ycVector < t_type, t_isPOD >
{
	using ycStackVectorContainer<t_type, t_count>::m_stackAlloc;

public:
	//! Constructs a useless vector, should be populated by the copy ctor, assignment, or Init() later!
	ycStackVector();

	explicit ycStackVector( ycAllocator* mem, const ureg initialReserve = 0 );
	ycStackVector( const ycVector< t_type, t_isPOD >& vec );
	ycStackVector& operator=( const ycVector< t_type, t_isPOD >& vec );

	ycStackVector( const ycStackVector& vec );
	ycStackVector& operator=( const ycStackVector& vec );

	ycStackVector( ycStackVector&& vec ) noexcept;
	ycStackVector& operator=( ycStackVector&& vec ) noexcept;
};

template< class t_type >
ycVectorRef< t_type >::ycVectorRef() :
	m_buf( nullptr ),
	m_len( 0 )
{
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >::ycVector() :
	m_cap( 0 ),
	m_mem( g_defaultMem )
{
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >::ycVector( ycAllocator* mem, const ureg initialReserve ) :
	m_cap( 0 ),
	m_mem( mem ? mem : g_defaultMem )
{
	if( initialReserve ) { Reserve( initialReserve ); }
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Init( ycAllocator* mem, const ureg initialReserve )
{
	ycAssert( m_len == 0 );
	if( mem && m_mem && m_mem->IsStorageEmbedded() )
	{
		ycAssert( m_mem->IsPtrEmbedded( m_buf ) );
		m_mem->SetFallbackAllocator( mem );
	}
	else
	{
		ycAssert( m_mem == nullptr || m_mem == g_defaultMem ); // no double initialization
		ycAssert( m_buf == nullptr );
		ycAssert( m_len == 0 );
		ycAssert( m_cap == 0 );

		m_mem = mem ? mem : g_defaultMem;
	}
	if( initialReserve ) { Reserve( initialReserve ); }
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >::ycVector( const ycVector& vec ) :
	m_cap( 0 ),
	m_mem( (vec.m_mem && vec.m_mem->IsStorageEmbedded())? g_defaultMem : vec.m_mem )
{
	Reserve( vec.Length() );
	for( ycSize_t i = 0; i != vec.Length(); ++i )
	{
		Append( vec[ i ] );
	}
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >::ycVector( ycVector&& vec ) noexcept
	: m_cap( 0 )
	, m_mem( (vec.m_mem && vec.m_mem->IsStorageEmbedded())? g_defaultMem : vec.m_mem )
{
	Swap( &vec );
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >::~ycVector()
{
	Clear();
}


template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Swap( ycVector* other )
{
	ycAssert( other != this );

	if( !m_mem->IsStorageEmbedded() && !other->m_mem->IsStorageEmbedded() )
	{
		ycSwap( m_buf, other->m_buf );
		ycSwap( m_len, other->m_len );
		ycSwap( m_cap, other->m_cap );
		ycSwap( m_mem, other->m_mem );
	}
	else
	{
		// allocators don't match, must swap elements individually
		{
			ycSize_t minLen = ycMin( Length(), other->Length() );
			ycSize_t thisLen = this->Length();
			ycSize_t otherLen = other->Length();
			for( ycSize_t ii = 0; ii < minLen; ++ii )
			{
				ycSwap( this->At( ii ), other->At( ii ) );
			}

			if( minLen < otherLen )
			{
				for( ycSize_t ii = minLen; ii < otherLen; ++ii )
				{
					Append( YC_MOVE( other->At( ii ) ) );
				}
				other->Resize( minLen );
			}
			
			if( minLen < thisLen )
			{
				for( ycSize_t ii = minLen; ii < thisLen; ++ii )
				{
					other->Append( YC_MOVE( this->At( ii ) ) );
				}
				Resize( minLen );
			}
		}
	}
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Clear( const bool freeMem )
{
	RebuildVector( nullptr, 0, nullptr, 0, nullptr, 0, 0, freeMem );
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::ClearPOD( const bool freeMem )
{
	RebuildVectorPOD( nullptr, 0, nullptr, 0, nullptr, 0, 0, freeMem );
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Reserve( const ycSize_t numItems )
{
	RebuildVector( GetData(), Length(), nullptr, 0, nullptr, 0, numItems );
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Resize( const ycSize_t numItems )
{
	Reserve( numItems );
	#if 1
		/*
		- the Append() version of this is quite slow for unoptimized/instrumented builds
		- the version below is still not great, cuz it's still gonna loop a ton for a primitive type which doesn't really need to do anything
		  (large ycVector<u8> byte buffers are very slow)
		- this was previously changed r61947, but i think the change there was mainly to replace the operator= with placement new, which this
		  is still doing
		- perhaps this should be implemented via new functionality in RebuildVector (pass nullptr newItems, with non-zero newItemCount); but
		  i am not comfortable enough with the RebuildVector stuff to do that atm
		*/
		if( Length() < numItems )
		{
			for( ycSize_t i = m_len; i != numItems; ++i )
			{
				new( m_buf + i ) t_type();
			}
			m_len = numItems;
		}
		else if( Length() > numItems )
		{
			RebuildVector( GetData(), numItems, nullptr, 0, nullptr, 0, numItems );
		}
	#else
		while( Length() < numItems )
		{
			Append();
		}
		if( Length() > numItems )
		{
			RebuildVector( GetData(), numItems, nullptr, 0, nullptr, 0, numItems );
		}
	#endif
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Copy( const ycVector< t_type, t_isPOD >& vec, const bool clearFreeMem /*= true*/ )
{
	Clear( clearFreeMem );
	if( (!m_mem || !m_mem->IsStorageEmbedded()) && (!vec.m_mem || !vec.m_mem->IsStorageEmbedded()) ) { m_mem = vec.m_mem; }
	RebuildVector( nullptr, 0, vec.GetData(), vec.Length(),  nullptr, 0, vec.Length() );
}


template< class t_type, bool t_isPOD >
t_type* ycVector< t_type, t_isPOD >::ReserveInternal( ycSize_t numItems, bool shrink )
{
	if( numItems > 0 && m_mem && m_mem->IsPtrEmbedded( m_buf ) ) { numItems = ycMax( numItems, m_mem->Size( m_buf ) / sizeof( t_type ) ); }
	if( m_cap == numItems ) { return nullptr; }
	if( m_cap >= numItems && !shrink ) { return nullptr; }

	t_type* oldBuffer = m_buf;
	ycAssert( this->Length() == 0 || oldBuffer );
	ycAssert( m_mem );
	if( numItems > 0 )
	{
		m_buf = (t_type*)YC_ALLOC( m_mem, sizeof( t_type ) * numItems, alignof( t_type ), "" );
	}
	else
	{
		m_buf = nullptr;
	}

	m_cap = numItems;

	if( m_mem && m_mem->IsPtrEmbedded( m_buf ) )
	{
		ycAssert( m_mem->Size( m_buf ) / sizeof( t_type ) >= numItems );
		m_cap = m_mem->Size( m_buf ) / sizeof( t_type );
	}

	return oldBuffer;
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::RebuildVector(
	t_type* sectionA, ycSize_t sectionACount,
	const t_type* newItems, ycSize_t newItemsCount,
	t_type* sectionC, ycSize_t sectionCCount,
	ycSize_t capacity, bool shrink )
{
	if constexpr ( t_isPOD )
	{
		RebuildVectorPOD( sectionA, sectionACount, newItems, newItemsCount, sectionC, sectionCCount, capacity, shrink );
	}
	else
	{
		ycSize_t newLength = sectionACount + newItemsCount + sectionCCount;
		ycSize_t oldLength = Length();
		t_type* oldBuffer = ReserveInternal( ycMax( capacity, newLength ), shrink );

		ycSize_t sectionCStart = sectionACount + newItemsCount;
		ycSize_t newItemsStart = sectionACount;
		ycSize_t sectionAStart = 0;

		if( oldBuffer )
		{
			for( ureg ii = 0; ii < sectionCCount; ++ii )
			{
				new( m_buf + ii + sectionCStart ) t_type( YC_MOVE( sectionC[ ii ] ) );
			}

			for( ureg ii = 0; ii < newItemsCount; ++ii )
			{
				new( m_buf + ii + newItemsStart ) t_type( newItems[ ii ] );
			}

			for( ureg ii = 0; ii < sectionACount; ++ii )
			{
				new( m_buf + ii + sectionAStart ) t_type( YC_MOVE( sectionA[ ii ] ) );
			}

			for( ycSize_t i = 0; i != oldLength; ++i )
			{
				oldBuffer[ i ].~t_type();
			}
			YC_FREE( m_mem, oldBuffer );
		}
		else
		{
			// for now (8/13/2021) just construct/assign any new items
			for( ureg ii = m_len; ii < newLength; ++ii )
			{
				new( m_buf + ii ) t_type();
			}

			auto Move = []( t_type& dst, t_type& src )
			{
				if( &dst == &src ) { return; }
				dst = YC_MOVE( src );
			};

			auto Copy = []( t_type& dst, const t_type& src )
			{
				if( &dst == &src ) { return; }
				dst = src;
			};

			if( newLength > m_len )
			{
				// expanding the vector, start from highest address
				for( sreg ii = sectionCCount - 1; ii >= 0; --ii )
				{
					Move( m_buf[ ii + sectionCStart ], sectionC[ ii ] );
				}
			}
			else
			{
				// contracting the vector, start from bottom
				for( ureg ii = 0; ii < sectionCCount; ++ii )
				{
					Move( m_buf[ ii + sectionCStart ], sectionC[ ii ] );
				}
			}

			for( ureg ii = 0; ii < newItemsCount; ++ii )
			{
				Copy( m_buf[ ii + newItemsStart ], newItems[ ii ] );
			}

			if( m_buf != sectionA )
			{
				for( ureg ii = 0; ii < sectionACount; ++ii )
				{
					Move( m_buf[ ii + sectionAStart ], sectionA[ ii ] );
				}
			}

			// if the vector has shrunk, call destructor on the end items
			for( ureg ii = newLength; ii < m_len; ++ii )
			{
				m_buf[ ii ].~t_type();
			}
		}

		m_len = newLength;
	}
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::RebuildVectorPOD(
	t_type* sectionA, ycSize_t sectionACount,
	const t_type* newItems, ycSize_t newItemsCount,
	t_type* sectionC, ycSize_t sectionCCount,
	ycSize_t capacity, bool shrink )
{
	ycSize_t newLength = sectionACount + newItemsCount + sectionCCount;
	t_type* oldBuffer = ReserveInternal( ycMax( capacity, newLength ), shrink );

	ycSize_t sectionCStart = sectionACount + newItemsCount;
	ycSize_t newItemsStart = sectionACount;

	if( sectionCCount ) { ycMemMove( m_buf + sectionCStart, sectionC, sizeof( t_type ) * sectionCCount ); }
	if( newItemsCount ) { ycMemMove( m_buf + newItemsStart, newItems, sizeof( t_type ) * newItemsCount ); }
	if( sectionACount && m_buf != sectionA ) { ycMemMove( m_buf, sectionA, sizeof( t_type ) * sectionACount ); }

	if( oldBuffer )
	{
		YC_FREE( m_mem, oldBuffer );
	}

	m_len = newLength;
}

template< class t_type, bool t_isPOD >
ycSize_t ycVector< t_type, t_isPOD >::Append( const t_type& item )
{
	if( m_len + 1 > m_cap ) { Reserve( GetGrowSize() ); }
	t_type* ptr = &m_buf[ m_len ];
	new( ptr ) t_type( item );
	++m_len;
	return Length()-1;
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::AppendUnique( const t_type& item )
{
	if( this->ycVectorRef< t_type >::IndexOf( item ) < 0 )
	{
		this->Append( item );
	}
}

template< class t_type, bool t_isPOD >
ycSize_t ycVector< t_type, t_isPOD >::Append( t_type&& item )
{
	if( m_len + 1 > m_cap ) { Reserve( GetGrowSize() ); }
	t_type* ptr = &m_buf[ m_len ];
	new( ptr ) t_type( YC_FORWARD( item ) );
	++m_len;
	return Length() - 1;
}

template< class t_type, bool t_isPOD >
template< typename ... t_args > 
t_type& ycVector< t_type, t_isPOD >::EmplaceBack( t_args&&... args )
{
	if( m_len + 1 > m_cap ) { Reserve( GetGrowSize() ); }
	t_type* ptr = &m_buf[ m_len ];
	new( ptr ) t_type( YC_FORWARD( args )... );
	++m_len;
	return *ptr;
}

template< class t_type, bool t_isPOD >
t_type* ycVector< t_type, t_isPOD >::Append()
{
	return &EmplaceBack();
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Append( const ycVectorRef< t_type >& other )
{
	RebuildVector( GetData(), Length(), other.GetData(), other.Length(), nullptr, 0, 0 );
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Insert( const t_type& item, const ycSize_t index )
{
	ycAssert( index <= Length() );
	RebuildVector( GetData(), index, &item, 1, GetData() + index, Length() - index, GetGrowSize() );
}

template< class t_type, bool t_isPOD >
t_type ycVector< t_type, t_isPOD >::TakeFirst()
{
	ycAssert( Length() > 0 );
	const t_type first = YC_MOVE( First() );
	RebuildVector( GetData() + 1, Length() - 1, nullptr, 0, nullptr, 0, 0 );

	return first;
}

template< class t_type, bool t_isPOD >
t_type ycVector< t_type, t_isPOD >::TakeLast()
{
	ycAssert( Length() );
	t_type take = YC_MOVE( Last() );
	RebuildVector( GetData(), Length() - 1, nullptr, 0, nullptr, 0, 0 );

	return t_type( YC_MOVE( take ) );
}

template< class t_type, bool t_isPOD >
t_type ycVector< t_type, t_isPOD >::Take( const ycSize_t idx )
{
	ycAssert( idx < Length() );
	const t_type take = YC_MOVE( At( idx ) );
	RebuildVector( GetData(), idx, nullptr, 0, GetData()+idx+1, Length()-idx-1, 0 );
	return take;
}

template< class t_type, bool t_isPOD >
typename ycVector< t_type, t_isPOD >::Iter ycVector< t_type, t_isPOD >::Take( const typename ycVector< t_type, t_isPOD >::Iter i )
{
	ycAssert( (t_type*)i >= GetData() );
	ycSize_t idx = (t_type*)i - GetData();
	Take( idx );
	return (Iter)( GetData() + idx );
}

template< class t_type, bool t_isPOD >
t_type ycVector< t_type, t_isPOD >::TakeUnsorted( const ycSize_t idx )
{
	ycSwap( At( idx ), Last() );
	return TakeLast();
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::Reverse()
{
	intptr_t i = 0;
	intptr_t j = intptr_t( Length() )-1;
	while( i < j )
	{
		ycSwap( At( i ), At( j ) );
		++i;
		--j;
	}
}

template< class t_type, bool t_isPOD >
void ycVector< t_type, t_isPOD >::ShrinkToFit()
{
	RebuildVector( GetData(), Length(), nullptr, 0, nullptr, 0, 0, true );
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >& ycVector< t_type, t_isPOD >::operator=( const ycVector< t_type, t_isPOD >& vec )
{
	if( &vec == this ) { return *this; }
	Copy( vec, false );
	return *this;
}

template< class t_type, bool t_isPOD >
ycVector< t_type, t_isPOD >& ycVector< t_type, t_isPOD >::operator=( ycVector< t_type, t_isPOD >&& vec ) noexcept
{
	if( &vec == this ) { return *this; }
	Clear( false );
	Swap( &vec );
	return *this;
}

template< class t_type >
int ycVectorRef< t_type >::IndexOf( const t_type& val ) const
{
	for( ycSize_t i = 0; i != Length(); ++i )
	{
		if( At( i ) == val ) { return int( i ); }
	}
	return -1;
}

template< class t_type >
int ycVectorRef< t_type >::LastIndexOf( const t_type& val ) const
{
	for( ycSize_t i = Length()-1; i >= 0; --i )
	{
		if( At( i ) == val ) { return int( i ); }
	}
	return -1;
}


template< class t_type >
ycSize_t ycVectorRef< t_type >::ExactIndexOf( const t_type* ptr ) const
{
	ycAssert( ptr >= GetData() && ptr < GetData() + Length() );
	return (ycSize_t)( ptr - GetData() );
}

template< class t_type >
t_type& ycVectorRef< t_type >::First()
{
	ycAssert( Length() );
	return At( 0 );
}

template< class t_type >
const t_type& ycVectorRef< t_type >::First() const
{
	ycAssert( Length() );
	return At( 0 );
}

template< class t_type >
t_type& ycVectorRef< t_type  >::Last()
{
	ycAssert( Length() );
	return At( Length() - 1 );
}

template< class t_type >
const t_type& ycVectorRef< t_type  >::Last() const
{
	ycAssert( Length() );
	return At( Length() - 1 );
}

template< class t_type >
ycVectorRef< t_type > ycVectorRef< t_type >::Left( const ycSize_t len ) const
{
	ycAssert( len <= Length() );
	return ycVectorRef( GetData(), len );
}

template< class t_type >
ycVectorRef< t_type > ycVectorRef< t_type >::Right( const ycSize_t len ) const
{
	ycAssert( len <= Length() );
	return ycVectorRef( GetData() + ( Length() - len), len );
}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>::ycStackVector()
	: ycStackVectorContainer<t_type, t_count>( g_defaultMem ), ycVector< t_type, t_isPOD >( &m_stackAlloc, t_count )
{}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>::ycStackVector( ycAllocator* mem, const ureg initialReserve )
	: ycStackVectorContainer<t_type, t_count>( mem ), ycVector< t_type, t_isPOD >( &m_stackAlloc, ycMax( t_count, initialReserve ) )
{}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>::ycStackVector( const ycVector< t_type, t_isPOD >& vec )
	: ycStackVectorContainer<t_type, t_count>( g_defaultMem ), ycVector< t_type, t_isPOD >( &m_stackAlloc, ycMax( t_count, vec.Length() ) )
{
	ycVector< t_type, t_isPOD >::Copy( vec, false );
}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>& ycStackVector<t_type, t_count, t_isPOD>::operator=( const ycVector< t_type, t_isPOD >& vec )
{
	ycVector< t_type, t_isPOD >::Copy( vec, false );
	return *this;
}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>::ycStackVector( const ycStackVector<t_type, t_count, t_isPOD>& vec )
	: ycStackVectorContainer<t_type, t_count>( g_defaultMem ), ycVector< t_type, t_isPOD >( &m_stackAlloc, ycMax( t_count, vec.Length() ) )
{
	ycVector< t_type, t_isPOD >::Copy( vec, false );
}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>::ycStackVector( ycStackVector<t_type, t_count, t_isPOD>&& vec ) noexcept
	: ycStackVectorContainer<t_type, t_count>( g_defaultMem ), ycVector< t_type, t_isPOD >( &m_stackAlloc, ycMax( t_count, vec.Length() ) )
{
	ycVector< t_type, t_isPOD >::Swap( &vec );
}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>& ycStackVector<t_type, t_count, t_isPOD>::operator=( const ycStackVector<t_type, t_count, t_isPOD>& vec )
{
	if( &vec == this ) { return *this; }
	ycVector< t_type, t_isPOD >::Copy( vec, false );

	return *this;
}

template< class t_type, ycSize_t t_count, bool t_isPOD >
ycStackVector<t_type, t_count, t_isPOD>& ycStackVector<t_type, t_count, t_isPOD>::operator=( ycStackVector<t_type, t_count, t_isPOD>&& vec ) noexcept
{
	ycVector< t_type, t_isPOD >::Clear();
	ycVector< t_type, t_isPOD >::Swap( &vec );

	return *this;
}
