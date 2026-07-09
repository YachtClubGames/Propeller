#pragma once

/*! \class ycSemaphore
*/

#include "ycCommon.h"
#include "ycPlatformConfig.h"
#include "ycStd.h"

#if defined YC_OSX
	#include <dispatch/dispatch.h>
#elif defined YC_PTHREAD
	#ifdef __linux__
		#define _POSIX_C_SOURCE 200809L // request semaphore functions from <semaphore.h>
	#endif
	#include <semaphore.h>
#endif

class ycSemaphore : public ycNoCopy
{
public:

	ycSemaphore( const char* debugName, const uint32_t initialCount = 0 ); //!< debug name is NOT copied
	~ycSemaphore();

	void Wait();
	bool Wait( u32 milliseconds ); //!< 0 to wait forever
	bool TryWait();

	void Signal();
	void Signal( const uint32_t count ); //!< may be emulated by looping the single-count Signal()

	#if defined YC_ENABLE_DEBUG_STRINGS
		inline void SetDebugName( const char* name ) { m_debugName = name; }
	#endif

protected:

	#if defined YC_MSFT
		void* m_semaphore; // typedef HANDLE
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_OSX
		dispatch_semaphore_t m_semaphore;
	#elif defined YC_PTHREAD
		sem_t m_semaphore;
	#endif

	#if defined YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif
};
