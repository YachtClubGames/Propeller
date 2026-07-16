#pragma once

/*
\class ycTLS

encapsulates thread local storage functions
*/

#include "ycCommon.h"

namespace ycTLS
{
	void Init(); // zeros out all TLS slots in the calling thread

	enum
	{
		kSlot_Internal = 0, //!< thread-specific engine data may reside here
		kSlot_Profiler,
		kSlot_Builder, //!< if ycBuilder is in used, context for workers/fibers
		#ifdef YC_MSFT
			kSlot_WaitableTimer, // a re-usable waitable timer handle
		#endif

		kSlot_User_Begin = 8, //!< game specific stuff can start here

		kSlot_Max = 16 //!< number of available slots
	};

	void Set( const ureg slot, const ureg value );
	ureg Get( const ureg slot );

	void SetPtr( const ureg slot, const void* value );
	void* GetPtr( const ureg slot );
};
