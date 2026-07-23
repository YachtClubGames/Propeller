#pragma once

/*! \class ycRWLock

ycRWLock implements a mutex-like lock that allows multiple readers simultaneous access, but only one writer at a time.
Support for recursive locking differs between platforms. Do not utilize it.

Implementation is platform specific and priority between readers and writers may differ.
*/

#include "ycCommon.h"
#include "ycPlatformConfig.h"
#include "ycStd.h"

#if defined YC_PTHREAD
	#include <pthread.h>
#endif

//#define YC_RWLOCK_USE_FALLBACK // use reader count+mutex+sema
#if defined YC_RWLOCK_USE_FALLBACK
	#include "ycMutex.h"
	#include "ycSemaphore.h"
#endif

class ycRWLock : public ycNoCopy
{
public:

	ycRWLock( const char* debugName );
	~ycRWLock();

	void LockRead();
	bool TryLockRead();
	void UnlockRead();

	void LockWrite();
	bool TryLockWrite();
	void UnlockWrite();

	#if defined YC_ENABLE_DEBUG_STRINGS
		inline void SetDebugName( const char* name ) { m_debugName = name; }
	#endif

protected:
	
	#if defined YC_RWLOCK_USE_FALLBACK
		ycMutex     m_readGuard;
		ureg        m_numReaders;
		ycSemaphore m_writeGuard;
	#elif defined YC_MSFT
		void* m_data; // SRWLOCK is just a ptr
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_PTHREAD
		pthread_rwlock_t m_data;
	#endif

	#if defined YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif
};
