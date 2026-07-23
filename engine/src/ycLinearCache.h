#pragma once
/*! \struct ycLinearCache

This class is designed for small object caches. 'Small' meaning few enough
items that a linear search is sufficient, rather than a hash table.

Internally, the cache is divided into two sets -- a hot set, and a cold set.
Items are not evicted individually, but the entire cold set is evicted at once.
You may need to set t_numItems a little higher than you would using a direct
MRU cache because of this.

- t_numItems must be even
- values are treated as POD
*/

#include "ycCommon.h"
#include "ycStd.h"

template< class t_keyType, class t_valType, uintptr_t t_numItems >
struct ycLinearCache : public ycNoCopy
{
public:

	typedef t_valType(*CreateFunc)( const t_keyType& key, void* createInfo );
	typedef void(*EvictFunc)( t_valType value );

	ycLinearCache( const char* debugName, CreateFunc createCB, EvictFunc evictCB );
	~ycLinearCache();

	t_valType Get( const t_keyType& key, void* createInfo = nullptr );

	void Clear();
	
	typedef bool(*InvalidateCompareFunc)( const t_keyType& key, const t_valType& val, void* userData );
	void InvalidateWhere( InvalidateCompareFunc cmp, void* userData )
	{
		for( ureg setIdx = 0; setIdx != 2; ++setIdx )
		{
			ureg numItems = m_numItems[ setIdx ];
			for( ureg i = 0; i < numItems; ++i )
			{
				if( cmp( m_keys[ setIdx ][ i ], m_values[ setIdx ][ i ], userData ) )
				{
					// destroy this item!
					m_evictCB( m_values[ setIdx ][ i ] );

					// if we arent the last item, put it into our place
					const ureg lastItemIdx = numItems-1;
					if( i != lastItemIdx )
					{
						m_keys[   setIdx ][ i ] = m_keys[   setIdx ][ lastItemIdx ];
						m_values[ setIdx ][ i ] = m_values[ setIdx ][ lastItemIdx ];
					}

					// decrement item count
					--numItems;
				}
			}
			// write the updated item count out
			m_numItems[ setIdx ] = numItems;
		}
	}

protected:

	void SwapSets();

	enum { kNumItemsPerSet = t_numItems/2 };

	CreateFunc m_createCB;
	EvictFunc  m_evictCB;
	ureg       m_hotSet;
	ureg       m_numItems[ 2 ];
	t_keyType  m_keys[     2 ][ kNumItemsPerSet ];
	t_valType  m_values[   2 ][ kNumItemsPerSet ];

	#ifdef YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif
};

template< class t_keyType, class t_valType, uintptr_t t_numItems >
ycLinearCache< t_keyType, t_valType, t_numItems >::ycLinearCache( const char* debugName, CreateFunc createCB, EvictFunc evictCB )
{
	YC_UNUSED( debugName );
	static_assert( (t_numItems&1) == 0, "t_numItems must be even" );
	m_createCB = createCB;
	m_evictCB  = evictCB;
	m_numItems[0] = m_numItems[1] = m_hotSet = 0;
	#ifdef YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif
}

template< class t_keyType, class t_valType, uintptr_t t_numItems >
ycLinearCache< t_keyType, t_valType, t_numItems >::~ycLinearCache()
{
	Clear();
}

template< class t_keyType, class t_valType, uintptr_t t_numItems >
t_valType ycLinearCache< t_keyType, t_valType, t_numItems >::Get( const t_keyType& key, void* createInfo /*= nullptr*/ )
{
	// search the hot set
	const ureg     hotSetIdx   = m_hotSet;
	const ureg     numHotItems = m_numItems[ hotSetIdx ];
	t_keyType*     hotKeys     = m_keys[ hotSetIdx ];
	t_valType*     hotValues   = m_values[ hotSetIdx ];
	for( ureg i = 0; i != numHotItems; ++i )
	{
		if( hotKeys[ i ] == key ) { return hotValues[ i ]; }
	}

	// search the cold set
	const ureg     coldSetIdx   = 1 - hotSetIdx;
	const ureg     numColdItems = m_numItems[ coldSetIdx ];
	t_keyType*     coldKeys     = m_keys[ coldSetIdx ];
	t_valType*     coldValues   = m_values[ coldSetIdx ];
	{
		for( ureg i = 0; i != numColdItems; ++i )
		{
			if( coldKeys[ i ] == key )
			{
				// append the item to the hot set
				const ureg dstHotIdx = numHotItems;
				ycAssert( dstHotIdx != kNumItemsPerSet ); // hot list should never be full when we hit this point!
				hotKeys[   dstHotIdx ] = key;
				hotValues[ dstHotIdx ] = coldValues[ i ];

				// remove the item from the cold set (swapping the last item from the cold set into its place)
				if( numColdItems > 1 )
				{
					const ureg lastColdIdx = numColdItems - 1;
					coldKeys[   i ] = coldKeys[ lastColdIdx ];
					coldValues[ i ] = coldValues[ lastColdIdx ];
				}
				--m_numItems[ coldSetIdx ];

				// if the hot set is now full, swap sets
				if( ++m_numItems[ hotSetIdx ] == kNumItemsPerSet ) { SwapSets(); }

				// return the hot set entry
				return hotValues[ dstHotIdx ];
			}
		}
	}
	// add the item to the cache
	{
		const ureg dstHotIdx = numHotItems;
		ycAssert( dstHotIdx != kNumItemsPerSet ); // hot list should never be full when we hit this point!
		hotKeys[   dstHotIdx ] = key;
		hotValues[ dstHotIdx ] = m_createCB( key, createInfo );

		// if the hot set is now full, swap sets
		if( ++m_numItems[ hotSetIdx ] == kNumItemsPerSet ) { SwapSets(); }

		// return the hot set entry
		return hotValues[ dstHotIdx ];
	}
}

template< class t_keyType, class t_valType, uintptr_t t_numItems >
void ycLinearCache< t_keyType, t_valType, t_numItems >::Clear()
{
	for( ureg setIdx = 0; setIdx != 2; ++setIdx )
	{
		const ureg       numItems = m_numItems[ setIdx ];
		const t_valType* values   = m_values[ setIdx ];
		for( ureg i = 0; i != numItems; ++i )
		{
			m_evictCB( values[ i ] );
		}
		m_numItems[ setIdx ] = 0;
	}
}

template< class t_keyType, class t_valType, uintptr_t t_numItems >
void ycLinearCache< t_keyType, t_valType, t_numItems >::SwapSets()
{
	// evict the cold set
	const ureg       coldSetIdx   = 1 - m_hotSet;
	const ureg       numColdItems = m_numItems[ coldSetIdx ];
	const t_valType* coldValues   = m_values[ coldSetIdx ];
	for( ureg i = 0; i != numColdItems; ++i )
	{
		m_evictCB( coldValues[ i ] );
	}
	m_numItems[ coldSetIdx ] = 0;

	// swap!
	m_hotSet = 1 - m_hotSet;
}
