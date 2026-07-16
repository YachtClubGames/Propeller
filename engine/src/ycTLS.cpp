#include "ycTLS.h"

namespace
{
	#if defined YC_MSVC
		__declspec( thread ) ureg s_tls[ ycTLS::kSlot_Max ];
	#elif defined YC_CLANG || defined YC_GCC
		__thread ureg s_tls[ ycTLS::kSlot_Max ];
	#else
		#error ycTLS is not implemented on the current platform
	#endif
}

void ycTLS::Init()
{
	for( ureg i = 0; i != kSlot_Max; ++i )
	{
		Set( i, 0 );
	}
}

void ycTLS::Set( const ureg slot, const ureg value )
{
	ycAssert( slot < kSlot_Max );
	s_tls[ slot ] = value;
}

ureg ycTLS::Get( const ureg slot )
{
	ycAssert( slot < kSlot_Max );
	return s_tls[ slot ];
}

void ycTLS::SetPtr( const ureg slot, const void* value )
{
	Set( slot, ureg(value) );
}

void* ycTLS::GetPtr( const ureg slot )
{
	return (void*)Get( slot );
}
