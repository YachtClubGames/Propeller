#include "ycRtti.h"

#include "ycStringRef.h"

namespace
{
	enum
	{
		kRttiTableSize = 1024*4, // the more oversized this is, the fewer loops a lookup requires on average
		kSlotMask = kRttiTableSize - 1,
		kUnusedSlotKey = 0
	};

	struct Entry // having string names in here, at least for debug, might be nice!
	{
		u64 key;
		ycRtti::IsaFuncPtr isaFunc;
	};

	Entry s_lookup[ kRttiTableSize ];

	inline Entry* GetEntry( const u64 typeId )
	{
		for( u64 steps = 0; /*nop*/; ++steps )
		{
			const u64 slot = ( typeId + steps ) & kSlotMask;
			Entry* entry = s_lookup + slot;
			if( entry->key == typeId ) { return entry; }
			if( entry->key == kUnusedSlotKey ) { return nullptr; }
		}
	}
}

/*static*/ ycRtti ycRtti::Create( const ycStringRef& str )
{
	return ycRtti( str.Hash64() );
}

bool ycRtti::Isa( const ycRtti parent ) const
{
	if( typeId == parent.typeId ) { return true; }
	Entry* rttiEntry = GetEntry( typeId );
	if( !rttiEntry )
	{
		rttiEntry = GetEntry( typeId );;
	}
	return rttiEntry->isaFunc( parent );
}

/*static*/ void ycRtti::Register( const u64 typeId, IsaFuncPtr isaFunc )
{
	Entry* entry = nullptr;
	for( u64 steps = 0; /*nop*/; ++steps )
	{
		const u64 slot = ( typeId + steps ) & kSlotMask;
		entry = s_lookup + slot;
		if( entry->key == kUnusedSlotKey )
		{
			break;
		}
	}
	entry->key = typeId;
	entry->isaFunc = isaFunc;
}

#ifdef YC_DEBUG
/*static*/ void ycRtti::DumpLookupStats()
{
	u64 maxSteps = 0;
	u64 numEntries = 0;
	u64 totalSteps = 0;
	for( u64 i = 0; i != kRttiTableSize; ++i )
	{
		const u64 key = s_lookup[ i ].key;
		if( key == kUnusedSlotKey ) { continue; }
		++numEntries;

		u64 steps = 0;
		for(;;)
		{
			const u64 slot = ( key + steps ) & kSlotMask;
			Entry* entry = s_lookup + slot;
			if( entry->key == key )
			{
				break;
			}
			++steps;
		}
		if( steps > maxSteps ) { maxSteps = steps; }
		totalSteps += steps;

		ycAssert( GetEntry( key )->key == key && GetEntry( key )->isaFunc == s_lookup[ i ].isaFunc ); // sanity check lookup while we're here
	}
	ycLog( "### DumpRttiLookupStats\n" );
	ycLog( "tableSize:  %u\n", u32(kRttiTableSize) );
	ycLog( "numEntries: %u\n", u32(numEntries) );
	ycLog( "maxSteps:   %u\n", u32(maxSteps) );
	ycLog( "avgSteps:   %.2f\n", float(totalSteps)/float(numEntries) );
}
#endif // YC_DEBUG
