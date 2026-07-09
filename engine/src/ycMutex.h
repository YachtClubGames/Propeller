#pragma once

/*! \class ycMutex
*/

#include "ycCommon.h"
#include "ycPlatformConfig.h"
#include "ycStd.h"

#ifdef YC_OSX
	// osx default is to use os_unfair_lock
	// might be nice to have a NSRecursiveLock fallback too, but I think that requires obj-c
	#define YC_OSX_PTHREAD_MUTEX // use pthreads
	#if defined YC_OSX_PTHREAD_MUTEX
	#else
		#include <os/lock.h>
	#endif
#endif

#if defined YC_PTHREAD
	#include <pthread.h>
#endif

class ycMutex : public ycNoCopy
{
public:

	ycMutex( const char* debugName ); //!< debug name is NOT copied
	~ycMutex();

	void Lock();
	bool TryLock();

	void Unlock();

	#if defined YC_ENABLE_DEBUG_STRINGS
		inline void SetDebugName( const char* name ) { m_debugName = name; }
	#endif

protected:

	#if defined YC_MSFT
		enum { kSpinCount = 10000 };
		#if defined YC_64
			YC_ALIGN(8) uint8_t data[40]; // sizeof CRITICAL_SECTION
		#else
			YC_ALIGN(4) uint8_t data[24]; // sizeof CRITICAL_SECTION
		#endif
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_OSX && !defined YC_OSX_PTHREAD_MUTEX
		os_unfair_lock data = OS_UNFAIR_LOCK_INIT;
	#elif defined YC_PTHREAD
		pthread_mutex_t data;
	#endif

	#if defined YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif
};

class ycMutexScoped
{
public:
	inline ycMutexScoped( ycMutex* _mutex ) : mutex( _mutex ) { _mutex->Lock(); }
	inline ~ycMutexScoped() { mutex->Unlock(); }
protected:
	ycMutex* mutex;
};
