#pragma once

#include "ycCommon.h"

/*
\namespace ycBitField

Any common tools for operating on bit-wise data

*/

namespace ycBitUtils
{
	template< class t_type >
	t_type NumBitsToMask( const t_type numBits ) { return (t_type(1)<<numBits)-t_type(1); }
	
	ureg CountLeadingZeros( const uint32_t x ); //!< returns 0 if the high bit is set (aka 0x80000000); returns 32 if no bits are set
	ureg CountLeadingZeros64( const uint64_t x ); //!< returns 0 if the high bit is set; returns 64 if no bits are set

	ureg CountTrailingZeros( const uint32_t x ); //!< returns 0 if the low bit is set (aka 0x00000001); returns 32 if no bits are set
	ureg CountTrailingZeros64( const uint64_t x ); //!< returns 0 if the low bit is set; returns 64 if no bits are set

	ureg PopCount( const uint32_t x ); //!< returns the number of set bits in the bit 
	ureg PopCount64( const uint64_t x ); //!< returns the number of set bits in the bit 

	inline
	ureg CountLeadingZerosUreg( const ureg x )
	{
		#ifdef YC_64
			return CountLeadingZeros64( x );
		#else
			return ycBitUtils::CountLeadingZeros( x );
		#endif
	}

	inline
	ureg CountTrailingZerosUreg( const ureg x )
	{
		#ifdef YC_64
			return CountTrailingZeros64( x );
		#else
			return CountTrailingZeros( x );
		#endif
	}

	template< class t_type >
	void ClearBit( t_type& x, const t_type flag )
	{
		x &= ~flag;
	}

	template< class t_type >
	void BitFlip( t_type & x, const t_type flag ) 
	{ 
		if( (x & flag) == 0 )
		{
			x |= flag;
		}
		else
		{
			x &= ~flag;
		}
	}
	template< class t_type >
	void SetBit( t_type & x, const t_type flag, bool enable ) 
	{ 
		if( enable )
		{
			x |= flag;
		}
		else
		{
			x &= ~flag;
		}
	}
	template< class t_type >
	bool IsBitSet( t_type & x, const t_type flag ) 
	{ 
		return ( (x & flag) != 0 );
	}


}

struct ycBitmaskIterator
{
	// iterates through all the "1"s in a bitmask
	ureg remaining = 0;
	ureg index = 0;

	ycBitmaskIterator( ureg mask ) : remaining( mask ) {}

	u64 GetMask() const { return ( 1ULL << index ); }

	bool MoveNext()
	{
		if( remaining )
		{
			index = ycBitUtils::CountTrailingZeros64( remaining );
			remaining ^= ( 1ULL << index );
			return true;
		}

		return false;
	}
};

