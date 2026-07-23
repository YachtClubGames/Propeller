#pragma once

/*! \class ycChainedHash

Implements an unordered associative array with arbitrary key and value
types.

For implementation details check out the "Separate Chaining" section at:
https://en.wikipedia.org/wiki/Hash_table

The t_hashBits parameter determines how many of the lower hash bits we
use to decide binning.  Using more bits will ensure faster access over
larger data sets, but requires more memory overhead.

The memory required is sizeof(void*) * (1<<t_hashBits).  This overhead
is constant, not per item.  Memory required on a 32-bit system:

t_hashBits    bytes
1             8
2             16
3             32
4             64
5             128
6             256
7             512
8             1024
9             2048
10            4096
11            8192
12            16384
13            32768
14            65536
15            131072
16            262144

Each hash entry requires a memory allocation.  You can query the size of this
allocation with s_blockSize.  If possible, use a block allocator so your hash
will not cause fragmentation.

WARNING: If you are using the string-key variant, the string is NOT copied
and iter.GetKey() will return the original pointer.
*/

// TODO HACK FIXME: memory layout is not good for searches of heavily populated hashes... ycPagedChainedHash with separate pages for keys?

#include "ycCommon.h"

// core
#include "ycAllocator.h"
#include "ycHash.h" //!< include standard hash algorithms
#include "ycStd.h"

template< class t_key, class t_value, int t_hashBits = 8 >
class ycChainedHash : public ycNoCopy
{
protected: struct Node;
public:
	
	explicit ycChainedHash( const char* debugName, ycAllocator* allocator = nullptr );
	ycChainedHash();
	bool IsNull() const { return m_alloc == nullptr; } //!< returns true if a array was initialized with the blank ctor, and is useless until initialized
	void Init( const char* debugName, ycAllocator* allocator = nullptr );
	~ycChainedHash();

	//! Adds a new item to the table
	/*!
	* There is no protection against collision and you *can* put duplicate entries into the hash.
	* The value and keys will be copied, be sure to have proper copy constructors/assignment operators
	  if necessary.
	* A reference to the copied value is returned.
	* Optionally returns the underlying hash key
	*/
	t_value& Insert( const t_key& key, const t_value& value ); // TODO HACK FIXME: an insert that returns an iterator would be nice, maybe more ways to work directly with the underlying hash keys

	t_value& InsertOrUpdate( const t_key& key, const t_value& value );

	t_value& Insert( const t_key& key ); //!< does not initialize the value, you must do that with the returned reference

	//! Removes an item from the table, returns true if successful.
	bool Remove( const t_key& key );

	//! Retrieves a value from the table
	t_value& Get( const t_key& key );
	const t_value& Get( const t_key& key ) const;

	t_value&       operator[]( const t_key& key )       { return Get( key ); }
	const t_value& operator[]( const t_key& key ) const { return Get( key ); }

	//! Checks whether the hash contains a value for a given key
	bool Contains( const t_key& key ) const;

	//! Removes all entries from the hash
	void Clear();

	/* \class Iter
	Provides a way to iterate over items in a ycChainedHash.  Order is arbitrary and should not be
	depended on.
	*/
	class Iter
	{
	public:

		//! Returns true if the iterator points to a valid item
		bool IsValid() const;

		//! Returns true if the iterator does not point to a valid item
		bool IsNull() const;

		const t_key& GetKey() const;
		t_value& GetValue();
		const t_value& GetValue() const;

		Iter& operator++() { Increment(); return *this; } //!< prefix increment
		Iter& operator++( int ) { const Iter iter( *this ); Increment(); return iter; } //!< postfix increment

		Iter();
		Iter( const Iter& iter );

	protected:
		friend class ycChainedHash< t_key, t_value, t_hashBits >;
		Iter( ycChainedHash< t_key, t_value, t_hashBits >* hash );
		Iter( ycChainedHash< t_key, t_value, t_hashBits >* hash, const t_key& key );
		void Increment();
		ycChainedHash* hash;
		Node*          node;
		ycHashKey_t    bin;
	};

	//! Returns an iterator to the first item
	/*!
	* This is not horribly expensive, but is more expensive than the equivalent operation
	  for vectors and lists, since we do not immediately know where the first item is and
	  must locate it.
	*/
	Iter Begin() { return Iter( this ); }

	//! Returns an iterator to a specific element
	/*!
	* Returns an invalid iterator if the item could not be found.
	*/
	Iter Find( const t_key& key ) { return Iter( this, key ); }

	void Erase( Iter& iter );

protected:

	enum
	{
		kNumHashBins = 1 << t_hashBits,
		kHashBinMask = kNumHashBins - 1
	};

	struct Node
	{
		ycHashKey_t hashedKey;
		Node*       prev;
		Node*       next;
		t_key       key;
		t_value     value;
	};

	Node*        m_bins[ kNumHashBins ];
	ycAllocator* m_alloc;

	#ifdef YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif

public:

	static const uint32_t s_blockSize = sizeof( Node );
};

//
// ycChainedHash
//

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::ycChainedHash( const char* debugName, ycAllocator* allocator ) :
	m_alloc( allocator ? allocator : g_defaultMem )
{
	ycAssert( allocator );
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
#	else
		YC_UNUSED( debugName );
	#endif
	for( uint32_t i = 0; i != kNumHashBins; ++i ) { m_bins[ i ] = nullptr; } // could add check to Clear() to check for m_alloc, this feels a little safer?
}

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::ycChainedHash() :
	m_alloc( nullptr )
{
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = nullptr;
	#endif
	for( uint32_t i = 0; i != kNumHashBins; ++i ) { m_bins[ i ] = nullptr; }
}

template< class t_key, class t_value, int t_hashBits >
void ycChainedHash< t_key, t_value, t_hashBits >::Init( const char* debugName, ycAllocator* allocator )
{
	ycAssert( m_alloc == nullptr );
	m_alloc = allocator ? allocator : g_defaultMem;
	ycAssert( allocator );
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif
	for( uint32_t i = 0; i != kNumHashBins; ++i ) { m_bins[ i ] = nullptr; }
}

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::~ycChainedHash()
{
	Clear();
}

template< class t_key, class t_value, int t_hashBits >
t_value& ycChainedHash< t_key, t_value, t_hashBits >::Insert( const t_key& key, const t_value& value )
{
	t_value& val = Insert( key );
	val = value;
	return val;
}

template< class t_key, class t_value, int t_hashBits >
t_value& ycChainedHash< t_key, t_value, t_hashBits >::Insert( const t_key& key )
{
	Node* node = new( YC_ALLOC( m_alloc, sizeof( Node ), alignof( Node ), m_debugName ) )Node;
	node->hashedKey = ycHashKey( key );
	const ycHashKey_t bin = node->hashedKey & kHashBinMask;
	node->prev = nullptr;
	node->next = m_bins[ bin ];
	if( m_bins[ bin ] ) { m_bins[ bin ]->prev = node; }
	m_bins[ bin ] = node;
	node->key = key;
	return node->value;
}

template< class t_key, class t_value, int t_hashBits >
t_value& ycChainedHash< t_key, t_value, t_hashBits >::InsertOrUpdate( const t_key& key, const t_value& value )
{
	// TODO HACK FIXME: optimize this, it hashes twice on insert
	Node* res						= nullptr;
	bool found						= false;
	const ycHashKey_t hashedKey		= ycHashKey( key );
	const ycHashKey_t bin			= hashedKey & kHashBinMask;
	for( Node* node = m_bins[ bin ]; node != nullptr; node = node->next )
	{
		if( node->hashKey == hashedKey )
		{
			found = true;
			res = node;
			break;
		}
	}

	// update the value
	if( found )
	{
		res->value = value;
		return res->value;
	}
	// insert the value
	else
	{
		Node* node			= new( YC_ALLOC( m_alloc, sizeof( Node ), alignof( Node ), m_debugName ) )Node;
		node->hashedKey		= hashedKey;
		node->prev			= nullptr;
		node->next			= m_bins[ bin ];
		if( m_bins[ bin ] ) { m_bins[ bin ]->prev = node; }
		m_bins[ bin ]		= node;
		node->key			= key;
		return node->value;
	}
}

template< class t_key, class t_value, int t_hashBits >
bool ycChainedHash< t_key, t_value, t_hashBits >::Remove( const t_key& key )
{
	const ycHashKey_t hashedKey = ycHashKey( key );
	const ycHashKey_t bin       = hashedKey & kHashBinMask;
	Node** prevNextLink = &m_bins[ bin ];
	for( Node* node = m_bins[ bin ]; node != nullptr; /**/ )
	{
		Node* nextNode = node->next;
		if( node->hashedKey == hashedKey )
		{
			*prevNextLink = node->next;
			if( node->next ) { node->next->prev = node->prev; }
			node->~Node();
			YC_FREE( m_alloc, node );
			return true;
		}
		prevNextLink = &node->next;
		node = nextNode;
	}
	return false;
}

template< class t_key, class t_value, int t_hashBits >
t_value& ycChainedHash< t_key, t_value, t_hashBits >::Get( const t_key& key )
{
	const ycHashKey_t hashedKey = ycHashKey( key );
	const ycHashKey_t bin       = hashedKey & kHashBinMask;
	for( Node* node = m_bins[ bin ]; node != nullptr; node = node->next )
	{
		if( node->hashedKey == hashedKey )
		{
			return node->value;
		}
	}
	ycAssertMsg( false, "ycChainedHash::Get() could not find a value associated with key." );
	#ifdef YC_CLANG // TODO HACK FIXME
		return m_bins[0]->value;
	#else
		return ycTypedNullRef< t_value >(); // fix warning w/ all control paths returning a value (and probably create a warning about null refs)
	#endif
}

template< class t_key, class t_value, int t_hashBits >
const t_value& ycChainedHash< t_key, t_value, t_hashBits >::Get( const t_key& key ) const
{
	const ycHashKey_t hashedKey = ycHashKey( key );
	const ycHashKey_t bin       = hashedKey & kHashBinMask;
	for( Node* node = m_bins[ bin ]; node != nullptr; node = node->next )
	{
		if( node->hashedKey == hashedKey )
		{
			return node->value;
		}
	}
	YC_ASSUME( 0 );
	//return t_value();
}

template< class t_key, class t_value, int t_hashBits >
bool ycChainedHash< t_key, t_value, t_hashBits >::Contains( const t_key& key ) const
{
	const ycHashKey_t hashedKey = ycHashKey( key );
	const ycHashKey_t bin       = hashedKey & kHashBinMask;
	for( Node* node = m_bins[ bin ]; node != nullptr; node = node->next )
	{
		if( node->hashedKey == hashedKey ) { return true; }
	}
	return false;
}

template< class t_key, class t_value, int t_hashBits >
void ycChainedHash< t_key, t_value, t_hashBits >::Clear()
{
	for( int32_t i = 0; i != kNumHashBins; ++i )
	{
		for( Node* node = m_bins[ i ]; node != nullptr; /**/ )
		{
			Node* nextNode = node->next;
			node->~Node();
			YC_FREE( m_alloc, node );
			node = nextNode;
		}
		m_bins[ i ] = nullptr;
	}
}

template< class t_key, class t_value, int t_hashBits >
void ycChainedHash< t_key, t_value, t_hashBits >::Erase( Iter& iter )
{
	ycAssert( iter.IsValid() );

	Node* node = iter.node;
	if( node->prev )
	{
		ycAssert( m_bins[ iter.bin ] != node );
		node->prev->next = node->next;
	}
	else
	{
		ycAssert( m_bins[ iter.bin ] == node );
		m_bins[ iter.bin ] = node->next;
	}
	if( node->next )
	{
		node->next->prev= node->prev;
	}
	
	node->~Node();
	YC_FREE( m_alloc, node );
}

//
// ycChainedHash::Iter
//

template< class t_key, class t_value, int t_hashBits >
bool ycChainedHash< t_key, t_value, t_hashBits >::Iter::IsValid() const
{
	return node != nullptr;
}

template< class t_key, class t_value, int t_hashBits >
bool ycChainedHash< t_key, t_value, t_hashBits >::Iter::IsNull() const
{
	return node == nullptr;
}

template< class t_key, class t_value, int t_hashBits >
const t_key& ycChainedHash< t_key, t_value, t_hashBits >::Iter::GetKey() const
{
	return node->key;
}

template< class t_key, class t_value, int t_hashBits >
t_value& ycChainedHash< t_key, t_value, t_hashBits >::Iter::GetValue()
{
	return node->value;
}


template< class t_key, class t_value, int t_hashBits >
const t_value& ycChainedHash< t_key, t_value, t_hashBits >::Iter::GetValue() const
{
	return node->value;
}

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::Iter::Iter() :
	node( nullptr ), bin( 0 ), hash( nullptr )
{}

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::Iter::Iter( const Iter& iter )
{ // clang doesnt like initializer list on this?
	hash = iter.hash;
	bin  = iter.bin;
	node = iter.node;
}

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::Iter::Iter( ycChainedHash< t_key, t_value, t_hashBits >* _hash ) :
	hash( _hash )
{
	for( bin = 0; bin != kNumHashBins; ++bin ) // bin is a member, not local!
	{
		if( hash->m_bins[ bin ] ) // first item found
		{
			node = hash->m_bins[ bin ];
			return;
		}
	}

	// nothing found, create an invalid iter
	bin = 0;
	node = nullptr;
}

template< class t_key, class t_value, int t_hashBits >
ycChainedHash< t_key, t_value, t_hashBits >::Iter::Iter( ycChainedHash< t_key, t_value, t_hashBits >* _hash, const t_key& key ) :
	hash( _hash )
{
	const ycHashKey_t hashedKey = ycHashKey( key );
	bin = hashedKey & kHashBinMask;
	for( node = hash->m_bins[ bin ]; node != nullptr; node = node->next ) // node is a member, not a local!
	{
		if( node->hashedKey == hashedKey ) { return; }
	}

	// nothing found, create an invalid iter
	bin = 0;
	node = nullptr;
}

template< class t_key, class t_value, int t_hashBits >
void ycChainedHash< t_key, t_value, t_hashBits >::Iter::Increment()
{
	// try to move to a neighbor in the current bin
	node = node->next;
	if( node ) { return; }

	// otherwise, search for the next bin
	for( ++bin; bin != kNumHashBins; ++bin ) // bin is a member, not local!
	{
		if( hash->m_bins[ bin ] )
		{
			node = hash->m_bins[ bin ];
			return;
		}
	}

	// nothing found, create an invalid iter
	bin = 0;
	node = nullptr;
}
