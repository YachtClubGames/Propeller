#pragma once
#ifndef YC_ARRAY_H
#define YC_ARRAY_H

#include "ycCommon.h"

/*! \class ycArray
A wrapper around standard static arrays, with additional error checking when
asserts are enabled.  With asserts disabled and compiler optimations on, this
should compile to the same thing as a vanilla array, hopefully.
*/

template< class t_type, ureg t_numElements >
class ycArray
{
public:

	YC_INLINE
	t_type& operator[]( const ureg index )
	{
		ycAssert( index >= 0 && index < t_numElements );
		return values[ index ];
	}

	YC_INLINE
	const t_type& operator[]( const ureg index ) const
	{
		ycAssert( index >= 0 && index < t_numElements );
		return values[ index ];
	}

protected:

	t_type values[ t_numElements ];
};

#endif // !YC_ARRAY_H
