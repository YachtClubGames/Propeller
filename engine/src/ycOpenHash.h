#pragma once

#include "ycAllocator.h"
#include "ycStd.h"
#include "ycHash.h"

//
// ycOpenHashTable Implementation notes
//	* This implementation does not allow deleted slots, hash table is fixed up
//	  on deletion so that there are no empty slots between an element and its optimal slot
//	* Insertion may shuffle elements if the number of steps for the inserted element
//	  is less than the number of steps from the optimal slot for the element it replaces
//	* By default the hash table size must be power of two but can be switched to any
//	  size by setting t_limit to ycOpenHashModSlot instead of ycOpenHashMaskSlot
//	* It is intended that the implementation wraps the insert/delete in a cpp file
//	  to reduce unnecessary code duplication, Finding slots and getting values are smaller.
//	* Keys with a value of 0 are not valid, 0 means slot is available. If 0 values are
//	  needed use something else.
//

/*
TODO:
Iter Find();
*/

template< class t_size = ycSize_t > struct ycOpenHashMaskSlot
{
	static t_size Limit( t_size value, t_size size ) { return value & ( size - 1 ); }
	static t_size Next( t_size value, t_size size ) { return Limit( value + 1, size ); }
	static t_size Prev( t_size value, t_size size ) { return Limit( value - 1, size ); }
	static t_size Steps( t_size start, t_size end, t_size size ) { return ( end - start ) & ( size - 1 ); }
	static bool ValidSize( t_size size ) { return ( size & ( size - 1 ) ) == 0; }
};

template< class t_size = ycSize_t > struct ycOpenHashModSlot
{
	static t_size Limit( t_size value, t_size size ) { return value % size; }
	static t_size Next( t_size value, t_size size ) { ++value; return value == size ? 0 : value; }
	static t_size Prev( t_size value, t_size size ) { return value ? ( value - 1 ) : ( size - 1 ); }
	static t_size Steps( t_size start, t_size end, t_size size ) { return ( size + end - start ) % size; }
	static bool ValidSize( t_size size ) { return !!size; }
};

//! search keys only base class for an open hash table (does not allocate or manage growth, no insert or delete)
template< class t_key, class t_size = ycSize_t, class t_limit = ycOpenHashMaskSlot< t_size > > class ycOpenHashBase
{
protected:
	t_key* m_keys;
	t_size m_size;
	t_size m_maxSteps;

public:
	// internal helper for swapping two slots
	void Swap( t_size slot1, t_size slot2 )
	{
		if( slot1 != slot2 )
		{
			t_key k = m_keys[ slot2 ];
			m_keys[ slot2 ] = m_keys[ slot1 ];
			m_keys[ slot1 ] = k;
		}
	}

	void EvalMaxSteps( t_size first, t_size slot )
	{
		t_size steps = slot > first ? ( slot - first ) : ( m_size + slot - first );
		if( steps > m_maxSteps ) { m_maxSteps = steps; }
	}

	t_size CalcSize() const
	{
		t_size size = 0;
		for( t_size i = 0; i < m_size; i++ )
		{
			if( !m_keys[ i ] ) { continue; }
			size++;
		}
		return size;
	}

	t_size GetCapacity() const { return m_size; }
	t_size GetMaxSteps() const { return m_maxSteps; }

	//! Hashing functions based on number of bits in key
	static t_size HashValue( u16 v ) { return t_size( ( ( ( v << 8 ) + ( v >> 8 ) + v ) + 284931295 ) * 0xcc9e2d51 ); }
	static t_size HashValue( s16 v ) { return HashValue( u16( v ) ); }
	static t_size HashValue( u32 v ) { return t_size( ( ( ( v << 12 ) + ( v >> 20 ) + v ) + 284931295 ) * 0xcc9e2d51 ); }
	static t_size HashValue( s32 v ) { return HashValue( u32( v ) ); }
	static t_size HashValue( u64 v ) { return t_size( ( ( ( v << 24 ) + ( v >> 40 ) + v ) + 284931295 ) * 0x1b873593cc9e2d51ULL ); }
	static t_size HashValue( s64 v ) { return HashValue( u64( v ) ); }
	template< class t_ptrType >
	static t_size HashValue( t_ptrType* ptr ) { return ( t_size )ycHashKey( (void*)ptr ); }

	//! Get the first slot that this key could be present at
	static t_size HashSlot( t_key key, t_size size ) { return t_limit::Limit( HashValue( key ), size ); }

	t_size HashSlot( t_key key ) { return HashSlot( key, m_size ); }

	//! Get a slot that contains this key or the last place to insert it
	static t_size FindSlot( t_key hash, t_size size, t_key* keys )
	{
		t_size idx = HashSlot( hash, size );
		while( t_key k = keys[ idx ] )
		{
			if( k == hash ) { return idx; }
			idx = t_limit::Next( idx, size );
		}
		return idx;
	}

	static t_size FindSlot( t_key hash, t_size size, t_key* keys, t_size maxSteps )
	{
		t_size idx = HashSlot( hash, size );
		while( keys )
		{
			t_key k = keys[ idx ];
			if( !k || k == hash ) { return idx; }
			idx = t_limit::Next( idx, size );
			if( !maxSteps--) { break; }
		}
		return idx;
	}

	//! Delete helper, checks if this key would be ok in this slot
	bool CanSwap( t_key key, t_size slot )
	{
		t_size check = HashSlot( key, m_size );
		while( check != slot )
		{
			if( !m_keys[check] ) { return false; }
			check = t_limit::Next( check, m_size );
		}
		return true;
	}

		//! Get a slot to insert at that is empty or less steps to find than the current occupant. Pass in first slot.
	t_size FindSlotRH( t_key key, t_size slot )
	{
		const t_key* keys = m_keys;
		t_size size = m_size;
		t_size steps = 0;
		while( t_key k = keys[ slot ] )
		{
			if( k == key ) { return slot; }  // key already exists
			t_size kfirst = HashSlot( k, size );
			t_size ksteps = kfirst > slot ? ( size + slot - kfirst ) : ( slot - kfirst );
			if( steps > ksteps ) { return slot; }
			slot = t_limit::Next( slot, m_size );
			++steps;
		}
		return slot;
	}

	// Look up a key's slot
	t_size FindSlot( t_key hash ) const
	{
		return FindSlot( hash, m_size, m_keys, m_maxSteps );
	}

	// Return number of steps to find this value. Will return number of steps to determine it doesn't exist if non-existent
	t_size Steps( t_key hash )
	{
		t_size idx = HashSlot( hash, m_size );
		t_size steps = 0;
		while( m_keys[ idx ] && m_keys[ idx ] != hash )
		{
			++steps;
			idx = t_limit::Next( idx, m_size );
		}
		return steps;
	}

	// Determine if key is in table
	bool KeyExists( t_key key ) const
	{
		if( !m_keys ) { return false; }
		t_size idx = FindSlot( key );
		return m_keys[ idx ] == key;
	}

	// Determine if key is in table with a known maximum number of steps for any key in table
	bool KeyExists( t_key key, t_size maxSteps ) const
	{
		if( !m_keys ) { return false; }
		t_size idx = FindSlot( key, m_size, m_keys, maxSteps );
		return m_keys[ idx ] == key;
	}

	t_key KeyAt( t_size index )
	{
		return m_keys[ index ];
	}

	ycOpenHashBase( t_key *keys = nullptr, t_size size = 0, t_size maxSteps = 0 ) : m_keys( keys ), m_size( size ), m_maxSteps( maxSteps )
	{
	}
};

template< class t_key = u64, class t_size = ycSize_t, t_size t_initSize = 256, t_size t_saturation = 14, class t_limit = ycOpenHashMaskSlot< t_size > >
class ycOpenHashKeys : public ycOpenHashBase< t_key, t_size, t_limit >
{
protected:
	t_size m_used;

	ycAllocator* m_mem;
	ureg m_mem_flags;

	enum { kAlignmentKey = alignof( t_key ) };

public:
	typedef ycOpenHashBase<t_key,t_size,t_limit> t_base;

	// no reallocation check insertion
	t_size InsertNoReserve( t_key key )
	{
		ycAssert( key );	// 0 keys are reserved for empty slots
		t_size first = t_base::HashSlot( key );
		t_size slot = t_base::FindSlotRH( key, first );
		t_base::EvalMaxSteps( first, slot );
		if( t_base::m_keys[ slot ] )
		{
			if( t_base::m_keys[ slot ] == key )
			{
				return slot;
			}
			else
			{
				t_key prev_key = t_base::m_keys[ slot ];
				t_base::m_keys[ slot ] = key;
				for( ;; )
				{
					t_size prev_first = t_base::HashSlot( prev_key );
					t_size slotRH = t_base::FindSlotRH( prev_key, prev_first );
					t_base::EvalMaxSteps( first, slot );
					if( t_base::m_keys[ slotRH ] && t_base::m_keys[ slotRH ] != prev_key )
					{
						t_key temp_key = t_base::m_keys[ slotRH ];
						t_base::m_keys[ slotRH ] = prev_key;
						prev_key = temp_key;
					}
					else
					{
						t_base::m_keys[ slotRH ] = prev_key;
						++m_used;
						return slot;
					}
				}
			}
		}
		t_base::m_keys[ slot ] = key;
		++m_used;
		return slot;
	}

	ycOpenHashKeys( ycAllocator* mem = nullptr, ureg flags = 0 ) : m_used( 0 ), m_mem( mem ? mem : g_defaultMem ), m_mem_flags( flags ) {}
	void Init( ycAllocator* mem = nullptr, ureg flags = 0 )
	{
		ycAssert( m_used == 0 && t_base::m_keys == nullptr );
		m_used = 0;
		m_mem = mem ? mem : g_defaultMem;
		m_mem_flags = flags;
		t_base::m_keys = nullptr;
	}
	~ycOpenHashKeys()
	{
		if( m_mem )
		{
			if( t_base::m_keys ) { m_mem->Free( t_base::m_keys, m_mem_flags ); }
		}
	}
	void Clear()
	{
		if( m_mem )
		{
			if( t_base::m_keys ) { m_mem->Free( t_base::m_keys, m_mem_flags ); }
			t_base::m_keys = nullptr;
			t_base::m_size = 0;
			m_used = 0;
		}
	}
	t_size GetUsed() const { return m_used; }
	bool Saturated() const { return m_used && ( m_used << 4 ) >= ( t_base::m_size * t_saturation ); }

	bool IsInitialized()
	{
		return m_mem != nullptr;
	}

	//! increase the size of the hash table and recalculate all used slots
	void Reserve( t_size new_size )
	{
		ycAssert( t_limit::ValidSize( new_size ) );
		if( new_size > t_base::m_size )
		{
			t_key* keys = t_base::m_keys;
			t_size size = t_base::m_size;

			t_base::m_size = new_size;
			t_base::m_keys = ( t_key* )m_mem->Alloc( new_size * sizeof( t_key ), kAlignmentKey, "HashTable", YC_FILE_LINE, m_mem_flags );
			ycMemSet( t_base::m_keys, 0, new_size * sizeof( t_key ) );

			if( m_used )
			{
				m_used = 0;
				for( t_size n = 0; n < size; n++ )
				{
					if( t_key key = keys[ n ] )
					{
						InsertNoReserve( key );
					}
				}
				m_mem->Free( keys, m_mem_flags );
			}
		}
	}

	//! put a value into a slot, returns false if key already exists, returns pointer to value
	t_size Insert( t_key key )
	{
		if( !t_base::m_size ) { Reserve( t_initSize ); }
		if( Saturated() ) { Reserve( t_base::m_size << 1 ); }
		return InsertNoReserve( key );
	}

	//! removes a key/value from table, it is not safe to just clear the key since key may be overlapping in sequence
	bool Remove( t_key key )
	{
		// only remove if there are values
		if( t_base::m_size == 0 || m_used == 0 ) { return false; }

		// find the slot to remove
		t_size slot = t_base::FindSlot( key );

		// check that key really was in the table
		if( t_base::m_keys[ slot ] != key ) { return false; }

		// clear out the slot
		t_base::m_keys[ slot ] = 0;
		m_used--;

		// if there is a valid key following this slot it may need to be swapped in for a later one
		t_size size = t_base::m_size;
		for( ;; )
		{
			t_size succ = t_limit::Next( slot, size );
			t_key key_s = t_base::m_keys[ succ ];
			if( !key_s ) { break;  }

			bool sequence = t_base::CanSwap( key_s, slot );
			t_size swap = succ;
			for( ;; )
			{
				succ = t_limit::Next( succ, size );
				key_s = t_base::m_keys[ succ ];
				if( !key_s ) { break; }
				if( t_base::CanSwap( key_s, slot ) )
				{
					sequence = true;
					swap = succ;  // don't break, want to swap in the last possible slot for least follow-up swaps
				}
			}
			// if there was no sequence there is no need to swap anything, just exit
			if( !sequence ) { break; }

			// swap the entries
			t_base::Swap( slot, swap );

			// check for sequence again with newly opened slot
			slot = swap;
		}
		return true;
	}

	//! Get the key at a given slot
	t_key KeyAt( t_size slot )
	{
		ycAssert( t_base::m_keys && slot < t_base::m_size );
		return t_base::m_keys ? t_base::m_keys[ slot ] : 0;
	}

	/* \class Iter
	Order is arbitrary and should not be depended on.
	
	TODO: FindValidSlot() should probably take into account the max search dist
	*/
	class Iter
	{
	public:

		bool IsValid() const { return slot != t_size(-1); } //! Returns true if the iterator points to a valid item
		bool IsNull() const { return slot == t_size(-1); } //! Returns true if the iterator does not point to a valid item

		const t_key& GetKey() const { return hash->m_keys[ slot ]; }

		Iter& operator++() { Increment(); return *this; }
		Iter& operator++( int ) { const Iter iter( *this ); Increment(); return iter; }

		bool operator!=( const Iter& o ) 
		{ 
			return hash != o.hash || slot != o.slot; 
		}
		Iter operator*() 
		{ 
			return Iter( hash, slot ); 
		}

		void Increment()
		{
			if( ++slot < hash->m_size ) 
			{ 
				FindValidSlot(); 
			}
			else
			{
				hash = nullptr;
				slot = t_size(-1); 
			}
		}

		Iter()
		{
			hash = nullptr;
			slot = t_size(-1);
		}
		Iter( const Iter& iter )
		{
			hash = iter.hash;
			slot = iter.slot;
		}

	protected:
		typedef ycOpenHashKeys< t_key,  t_size, t_initSize, t_saturation, t_limit > t_hashType;
		friend t_hashType;

		Iter( t_hashType* _hash )
		{
			hash = _hash;
			slot = 0;
			FindValidSlot();
		}
		Iter( t_hashType* _hash, const t_size _slot ) // assumes a valid slot is passed
		{
			hash = _hash;
			slot = _slot;
		}
		void FindValidSlot()
		{
			// assumes 'slot' has already been bounds-checked
			while( !hash->m_keys[ slot ] )
			{
				if( ++slot >= hash->m_size )
				{
					hash = nullptr;
					slot = t_size(-1);
					break;
				}
			}
		}
		t_hashType* hash;
		t_size      slot;
	};

	Iter Begin()
	{
		return t_base::m_keys ? Iter( this ) : Iter();
	}

	//! Returns a pointer past the last element in the vector
	Iter      End()       { return Iter(); }
	//! STL-compatible versions to support "for(x : list)" constructs.
	Iter      begin()       { return t_base::m_keys ? Iter( this ) : Iter();  }
	Iter      end()       { return Iter(); }
};


//! Reliable open addressing linear hash table
template< class t_key, class t_value, class t_size = ycSize_t, t_size t_initSize = 256, t_size t_saturation = 14, class t_limit = ycOpenHashMaskSlot< t_size > >
class ycOpenHash : public ycOpenHashBase< t_key, t_size, t_limit >
{
	typedef ycOpenHashBase<t_key,t_size,t_limit> t_base;
	t_size m_used;

	ycAllocator* m_mem;
	ureg m_mem_flags;

	t_value* m_values;

	enum
	{
		kAlignmentKey = alignof( t_key ),
		kAlignmentValue = alignof( t_value )
	};

	// internal helper for swapping two slots
	void Swap( t_size slot1, t_size slot2 )
	{
		if( slot1 != slot2 )
		{
			t_key k = YC_MOVE( t_base::m_keys[ slot2 ] );
			t_base::m_keys[ slot2 ] = YC_MOVE( t_base::m_keys[ slot1 ] );
			t_base::m_keys[ slot1 ] = YC_MOVE( k );
			t_value v = YC_MOVE( m_values[ slot2 ] );
			m_values[ slot2 ] = YC_MOVE( m_values[ slot1 ] );
			m_values[ slot1 ] = YC_MOVE( v );
		}
	}
public:

	// no reallocation check insertion
	t_value* InsertNoReserve( t_key key )
	{
	ycAssert( key );	// 0 keys are reserved for empty slots
		t_size first = t_base::HashSlot( key );
		t_size slot = t_base::FindSlotRH( key, first );
		t_base::EvalMaxSteps( first, slot );
		if( t_base::m_keys[ slot ] )
		{
			if( t_base::m_keys[ slot ] == key )
			{
				return m_values + slot;
			}
			else
			{
				t_key prev_key = YC_MOVE( t_base::m_keys[ slot ] );
				t_value prev_value = YC_MOVE( m_values[ slot ] );
				new( m_values + slot ) t_value;
				t_base::m_keys[ slot ] = YC_MOVE( key );
				for( ;; )
				{
					t_size prev_first = t_base::HashSlot( prev_key );
					t_size slotRH = t_base::FindSlotRH( prev_key, prev_first );
					t_base::EvalMaxSteps( prev_first, slotRH );
					if( t_base::m_keys[ slotRH ] && t_base::m_keys[ slotRH ] != prev_key )
					{
						ycSwap( t_base::m_keys[ slotRH ], prev_key );
						ycSwap( m_values[ slotRH ], prev_value );
					}
					else
					{
						t_base::m_keys[ slotRH ] = YC_MOVE( prev_key );
						m_values[ slotRH ] = YC_MOVE( prev_value );
						++m_used;
						return m_values + slot;
					}
				}
			}
		}
		t_base::m_keys[ slot ] = key;
		++m_used;
		return m_values + slot;
	}

	ycOpenHash( ycAllocator* mem = nullptr, ureg flags = 0 ) : m_used( 0 ), m_mem( mem ? mem : g_defaultMem ), m_mem_flags( flags ), m_values( nullptr ) {}
	void Init( ycAllocator* mem = nullptr, ureg flags = 0 )
	{
		ycAssert( m_used == 0 && m_values == nullptr && t_base::m_keys == nullptr );
		m_used = 0;
		m_mem = mem ? mem : g_defaultMem;
		m_mem_flags = flags;
		m_values = nullptr;
		t_base::m_keys = nullptr;
	}
	bool IsInitialized() const { return m_mem != nullptr; }
	bool IsAllocated() const { return t_base::m_keys != nullptr; }
	~ycOpenHash()
	{
		if( m_mem )
		{
			if( m_values )
			{
				for( Iter i = Begin(); i.IsValid(); ++i )
				{
					i.GetValue().~t_value();
				}
				m_mem->Free( m_values, m_mem_flags );
			}
			if( t_base::m_keys ) { m_mem->Free( t_base::m_keys, m_mem_flags ); }
			m_mem = nullptr;
			m_values = nullptr;
			t_base::m_keys = nullptr;
			m_used = 0;
		}
	}

	ycOpenHash( const ycOpenHash& other ) = delete;
	ycOpenHash& operator=( const ycOpenHash& other ) = delete;

	ycOpenHash( ycOpenHash&& other ) noexcept
	{
		t_base::m_keys = other.m_keys;
		t_base::m_size = other.m_size;
		t_base::m_maxSteps = other.m_maxSteps;
		other.m_keys = nullptr;
		other.m_size = 0;
		other.m_maxSteps = 0;

		m_mem = other.m_mem;
		m_mem_flags = other.m_mem_flags;
		m_values = other.m_values;
		m_used = other.m_used;
		other.m_mem = nullptr;
		other.m_mem_flags = 0;
		other.m_values = nullptr;
		other.m_used = 0;
		other.m_size = 0;
		other.m_maxSteps = 0;
	}

	ycOpenHash& operator=( ycOpenHash&& other ) noexcept
	{
		ycOpenHash::~ycOpenHash();

		t_base::m_keys = other.m_keys;
		t_base::m_size = other.m_size;
		t_base::m_maxSteps = other.m_maxSteps;
		other.m_keys = nullptr;
		other.m_size = 0;
		other.m_maxSteps = 0;

		m_mem = other.m_mem;
		m_mem_flags = other.m_mem_flags;
		m_values = other.m_values;
		m_used = other.m_used;
		other.m_mem = nullptr;
		other.m_mem_flags = 0;
		other.m_values = nullptr;
		other.m_used = 0;
		other.m_size = 0;
		other.m_maxSteps = 0;

		return *this;
	}

	void Copy( ycOpenHash& other )
	{
		Clear();
		m_mem = other.m_mem;
		m_mem_flags = other.m_mem_flags;
		m_used = other.m_used;
		t_base::m_size = other.m_size;
		t_base::m_maxSteps = other.m_maxSteps;
		if( t_base::m_size )
		{
			t_base::m_keys = ( t_key* )m_mem->Alloc( t_base::m_size * sizeof( t_key ), kAlignmentKey, "HashTable", YC_FILE_LINE, m_mem_flags );
			m_values = ( t_value* )m_mem->Alloc( t_base::m_size * sizeof( t_value ), kAlignmentValue, "HashTable", YC_FILE_LINE, m_mem_flags );
			ycMemCpy( t_base::m_keys, other.m_keys, t_base::m_size * sizeof( t_key ) );
			for( t_size i = 0; i != t_base::m_size; ++i )
			{
				new( m_values + i )t_value;
			}
			for( t_size i = 0; i != t_base::m_size; ++i )
			{
				m_values[ i ] = other.m_values[ i ];
			}
		}
	}

	void CopyPOD( ycOpenHash& other )
	{
		Clear();
		m_mem = other.m_mem;
		m_mem_flags = other.m_mem_flags;
		m_used = other.m_used;
		t_base::m_size = other.m_size;
		t_base::m_maxSteps = other.m_maxSteps;
		if( t_base::m_size )
		{
			t_base::m_keys = ( t_key* )m_mem->Alloc( t_base::m_size * sizeof( t_key ), kAlignmentKey, "HashTable", YC_FILE_LINE, m_mem_flags );
			m_values = ( t_value* )m_mem->Alloc( t_base::m_size * sizeof( t_value ), kAlignmentValue, "HashTable", YC_FILE_LINE, m_mem_flags );
			ycMemCpy( t_base::m_keys, other.m_keys, t_base::m_size * sizeof( t_key ) );
			ycMemCpy( m_values, other.m_values, t_base::m_size * sizeof( t_value ) );
		}
	}

	void Clear()
	{
		if( m_mem )
		{
			if( t_base::m_keys ) { m_mem->Free( t_base::m_keys, m_mem_flags ); }
			if( m_values )
			{
				for( t_size i = 0; i != t_base::m_size; ++i )
				{
					m_values[ i ].~t_value();
				}
				m_mem->Free( m_values, m_mem_flags );
			}
			t_base::m_keys = nullptr;
			t_base::m_size = 0;
			m_values = nullptr;
			m_used = 0;
		}
	}

	t_size GetUsed() const { return m_used; }
	bool Saturated() const { return m_used && ( m_used << 4 ) >= ( t_base::m_size * t_saturation ); }

	//! increase the size of the hash table and recalculate all used slots
	void Reserve( t_size new_size )
	{
		ycAssert( t_limit::ValidSize( new_size ) );
		if( new_size > t_base::m_size )
		{
			t_key* keys = t_base::m_keys;
			t_value* values = m_values;
			t_size size = t_base::m_size;

			t_base::m_size = new_size;
			t_base::m_keys = ( t_key* )m_mem->Alloc( new_size * sizeof( t_key ), kAlignmentKey, "HashTable", YC_FILE_LINE, m_mem_flags );
			m_values = ( t_value* )m_mem->Alloc( new_size * sizeof( t_value ), kAlignmentValue, "HashTable", YC_FILE_LINE, m_mem_flags );

			ycMemSet( t_base::m_keys, 0, new_size * sizeof( t_key ) );
			for( t_size i = 0; i != new_size; ++i )
			{
				new( m_values + i )t_value;
			}
			//ycMemSet( m_values, 0, new_size * sizeof( t_value ) );

			if( m_used )
			{
				m_used = 0;
				t_base::m_maxSteps = 0;
				for( t_size n = 0; n < size; n++ )
				{
					if( t_key key = YC_MOVE( keys[ n ] ) )
					{
						t_value* value_ptr = InsertNoReserve( key );
						*value_ptr = YC_MOVE( values[ n ] );
						//values[ n ].~t_value();
						keys[ n ].~t_key();
					}
				}
			}
			if( keys ) { m_mem->Free( keys, m_mem_flags ); }

			if( values )
			{
				for( t_size i = 0; i != size; ++i )
				{
					values[ i ].~t_value();
				}
				m_mem->Free( values, m_mem_flags );
			}
		}
	}

	//! put a value into a slot, returns pointer to value
	t_value* Insert( t_key key )
	{
		if( !t_base::m_size ) { Reserve( t_initSize ); }
		if( Saturated() ) { Reserve( t_base::m_size << 1 ); }
		return InsertNoReserve( key );
	}

	t_value* Insert( t_key key, const t_value& value )
	{
		t_value* value_ptr = Insert( key );
		*value_ptr = value;
		return value_ptr;
	}

	bool RemoveSlot( t_size slot ) // returns true if another item was moved into the removed slot
	{
		// clear out the slot
		t_base::m_keys[ slot ] = t_key( 0 );
		m_values[ slot ] = t_value(); // all m_values are assumed to remain valid, so they can be e.g. swapped into
		m_used--;

		// if there is a valid key following this slot it may need to be swapped in for a later one
		t_size mask = t_base::m_size - 1;
		t_size wasSlot = slot;
		while( t_base::m_keys[ ( slot + 1 ) & mask ] )
		{
			t_size succ = ( slot + 1 ) & mask;
			bool sequence = t_base::CanSwap( t_base::m_keys[ succ ], slot );
			t_size swap = succ;
			while( t_base::m_keys[ ( succ + 1 ) & mask ] )
			{
				succ = ( succ + 1 ) & mask;
				if( t_base::CanSwap( t_base::m_keys[ succ ], slot ) )
				{
					sequence = true;
					swap = succ;  // don't break, want to swap in the last possible slot for least follow-up swaps
				}
			}
			// if there was no sequence there is no need to swap anything, just exit
			if( !sequence ) { break; }

			// swap the entries
			ycAssert( slot != swap );
			Swap( slot, swap );

			// check for sequence again with newly opened slot
			slot = swap;
		}
		return slot != wasSlot;
	}

	//! removes a key/value from table, it is not safe to just clear the key since key may be overlapping in sequence
	bool Remove( t_key key )
	{
		// only remove if there are values
		if( t_base::m_size == 0 || m_used == 0 ) { return false; }

		// find the slot to remove
		t_size slot = t_base::FindSlot( key );

		// check that key really was in the table
		if( t_base::m_keys[ slot ] != key ) { return false; }

		RemoveSlot( slot );

		return true;
	}

	//! Get the value of a key or nullptr if key not found
	t_value* Get( t_key key )
	{
		if( t_base::m_size && key )
		{
			t_size slot = t_base::FindSlot( key );
			if( t_base::m_keys[ slot ] == key )
			{
				return m_values + slot;
			}
		}
		return nullptr;
	}

	//! Get the value of a key or nullptr if key not found
	const t_value* Get( t_key key ) const
	{
		if( t_base::m_size && key )
		{
			t_size slot = t_base::FindSlot( key );
			if( t_base::m_keys[ slot ] == key )
			{
				return m_values + slot;
			}
		}
		return nullptr;
	}

	//! Get the key at a given slot
	t_key KeyAt( t_size slot )
	{
		ycAssert( t_base::m_keys && slot < t_base::m_size );
		return t_base::m_keys ? t_base::m_keys[ slot ] : 0;
	}

	t_value& ValueAt( t_size slot )
	{
		ycAssert( m_values && slot < t_base::m_size );
		return m_values ? m_values[ slot ] : *(t_value*)nullptr;
	}

	/* \class Iter
	Order is arbitrary and should not be depended on.
	
	TODO: FindValidSlot() should probably take into account the max search dist
	*/
	class Iter
	{
	public:

		bool IsValid() const { return slot != t_size(-1); } //! Returns true if the iterator points to a valid item
		bool IsNull() const { return slot == t_size(-1); } //! Returns true if the iterator does not point to a valid item

		const t_key& GetKey() const { return hash->m_keys[ slot ]; }
		t_value& GetValue() { return hash->m_values[ slot ]; }
		const t_value& GetValue() const { return hash->m_values[ slot ]; }
		void SetValue( const t_value& value ) { hash->m_values[ slot ] = value; }

		Iter& operator++() 
		{ 
			Increment(); 
			return *this; 
		}
		Iter& operator++( int ) 
		{ 
			const Iter iter( *this ); 
			Increment(); 
			return iter; 
		}

		bool operator!=( const Iter& o ) 
		{ 
			return hash != o.hash || slot != o.slot; 
		}
		Iter operator*() 
		{ 
			return Iter( hash, slot ); 
		}

		void Increment()
		{
			if( ++slot < hash->m_size ) { FindValidSlot(); }
			else { hash = nullptr; slot = t_size(-1); }
		}

		Iter()
		{
			hash = nullptr;
			slot = t_size(-1);
		}
		Iter( const Iter& iter )
		{
			hash = iter.hash;
			slot = iter.slot;
		}
		Iter( Iter&& other ) noexcept
		{
			hash = other.hash;
			slot = other.slot;
			other.hash = nullptr;
			other.slot = 0;
		}
		Iter& operator=( Iter&& other ) noexcept
		{
			hash = other.hash;
			slot = other.slot;
			other.hash = nullptr;
			other.slot = 0;
			return *this;
		}
		Iter& operator=( const Iter& other )
		{
			hash = other.hash;
			slot = other.slot;
		}

	protected:
		typedef ycOpenHash< t_key, t_value, t_size, t_initSize, t_saturation, t_limit > t_hashType;
		friend t_hashType;

		Iter( t_hashType* _hash )
		{
			hash = _hash;
			slot = 0;
			FindValidSlot();
		}
		Iter( t_hashType* _hash, const t_size _slot ) // assumes a valid slot is passed
		{
			hash = _hash;
			slot = _slot;
		}
		void FindValidSlot()
		{
			// assumes 'slot' has already been bounds-checked
			while( !hash->m_keys[ slot ] )
			{
				if( ++slot >= hash->m_size )
				{
					hash = nullptr;
					slot = t_size(-1);
					break;
				}
			}
		}
		t_hashType* hash;
		t_size      slot;
	};

	Iter Erase( const Iter& iter )
	{
		// only remove if there are values
		ycAssert( t_base::m_size && m_used );

		// find the slot to remove
		t_size slot = iter.slot;

		// check that key really was in the table
		//if( t_base::m_keys[ slot ] != key ) { return false; }

		const bool swapped = RemoveSlot( slot );
		if( swapped )
		{
			return iter;
		}
		else
		{
			Iter newIter( iter.hash, iter.slot );
			newIter.FindValidSlot();
			return newIter;
		}
	}

	Iter      Begin()       { return t_base::m_keys ? Iter( this ) : Iter(); }

	//! Returns a pointer past the last element in the vector
	Iter      End()       { return Iter(); }
	//! STL-compatible versions to support "for(x : list)" constructs.
	Iter      begin()       { return t_base::m_keys ? Iter( this ) : Iter();  }
	Iter      end()       { return Iter(); }
};

