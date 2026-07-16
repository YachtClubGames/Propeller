#pragma once

/*
\class ycBitField

holds `t_size` bits which can be queried and manipulated via index
*/

#include "ycCommon.h"

template< ycSize_t t_size > 
class ycBitField 
{
public:

	ycBitField() { }

	void Clear( ureg b ) 
	{ 
		ycAssert( b < t_size );
		m_bits[b>>5] &= ~(u8(1)<<(b&31)); 
	}

	void Clear() 
	{ 
		for( ureg i=0; i<(sizeof(m_bits)>>2); ++i ) 
		{
			m_bits[i] = 0; 
		}
	}

	void Set() 
	{ 
		for( ureg i=0; i<(sizeof(m_bits)>>2); ++i ) 
		{
			m_bits[i] = u32(~0); 
		}
	}

	void Set( ureg b ) 
	{ 
		ycAssert( b < t_size );
		m_bits[b>>5] |= u8(1)<<(b&31); 
	}

	void Set( ureg b, bool set ) 
	{ 
		if( set ) 
		{ 
			Set( b ); 
		} 
		else 
		{ 
			Clear( b ); 
		} 
	}

	void Toggle( ureg b ) 
	{ 
		ycAssert( b < t_size );
		m_bits[b>>5] ^= u8(1)<<(b&31); 
	}
	
	bool Check( ureg b ) const 
	{ 
		ycAssert( b < t_size );
		return (m_bits[b>>5] & u8(1)<<(b&31)) != 0; 
	}

private:

	u32 m_bits[ (t_size + 31) / 32 ];
};
