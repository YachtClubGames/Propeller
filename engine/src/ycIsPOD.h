
#pragma once

#include "ycCompilerConfig.h"

template< typename t_type > struct ycIsPod { enum { val = 0 }; };
template< typename t_type > struct ycIsPod< t_type * > { enum { val = 1 }; };

#define YC_IS_POD( T ) template<> struct ycIsPod<T> { enum { val = 1 }; }

YC_IS_POD( s8 );
YC_IS_POD( s16 );
YC_IS_POD( s32 );
YC_IS_POD( s64 );
YC_IS_POD( u8 );
YC_IS_POD( u16 );
YC_IS_POD( u32 );
YC_IS_POD( u64 );
YC_IS_POD( f32 );
YC_IS_POD( f64 );
