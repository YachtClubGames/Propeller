#include "ycMutex.h"

#if YC_NDA
#elif defined YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
#elif YC_NDA
#elif defined YC_PTHREAD
	#include <errno.h> // EBUSY
#endif

#if defined YC_OSX_RECURSIVE_MUTEX
	#include <Foundation/Foundation.h>
#endif

ycMutex::ycMutex( const char* debugName )
{
	#if defined YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif

	#if YC_NDA
	#elif defined YC_MSFT
		static_assert( sizeof(data) == sizeof(CRITICAL_SECTION), "ycMutex wrong data size" );
		static_assert( alignof(ycMutex) == alignof(CRITICAL_SECTION), "ycMutex wrong data alignment" );
		BOOL ret = InitializeCriticalSectionAndSpinCount( (CRITICAL_SECTION*)data, kSpinCount );
		ycAssert( ret == TRUE );
		YC_UNUSED( ret );
	#elif YC_NDA
	#elif defined YC_OSX  && !defined YC_OSX_PTHREAD_MUTEX
		// nothing to do, lock is already initialized
	#elif defined YC_PTHREAD
		#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
			data = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
		#elif defined PTHREAD_RECURSIVE_MUTEX_INITIALIZER
			data = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
		#else
			pthread_mutexattr_t attr;
			pthread_mutexattr_init( &attr );
			pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
			const int ret = pthread_mutex_init( &data, &attr );
			ycAssert( ret == 0 );
		#endif
		//const int ret = pthread_mutex_init( &data, &attrs );
	#else
		#error not implemented on this platform!
	#endif
}

ycMutex::~ycMutex()
{
	#if YC_NDA
	#elif defined YC_MSFT
		DeleteCriticalSection( (CRITICAL_SECTION*)data );
	#elif YC_NDA
	#elif defined YC_OSX && !defined YC_OSX_PTHREAD_MUTEX
		os_unfair_lock_assert_not_owner( &data );
	#elif defined YC_PTHREAD
		const int ret = pthread_mutex_destroy( &data );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}

void ycMutex::Lock()
{
	#if YC_NDA
	#elif defined YC_MSFT
		EnterCriticalSection( (CRITICAL_SECTION*)data );
	#elif YC_NDA
	#elif defined YC_OSX && !defined YC_OSX_PTHREAD_MUTEX
		os_unfair_lock_assert_not_owner( &data );
		os_unfair_lock_lock( &data );
	#elif defined YC_PTHREAD
		const int ret = pthread_mutex_lock( &data );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}

bool ycMutex::TryLock()
{
	#if YC_NDA
	#elif defined YC_MSFT
		return TryEnterCriticalSection( (CRITICAL_SECTION*)data ) == TRUE;
	#elif YC_NDA
	#elif defined YC_OSX && !defined YC_OSX_PTHREAD_MUTEX
		os_unfair_lock_assert_not_owner( &data );
		return os_unfair_lock_trylock( &data );
	#elif defined YC_PTHREAD
		const int ret = pthread_mutex_trylock( &data );
		ycAssert( ret == 0 || ret == EBUSY );
		return ret == 0;
	#else
		#error not implemented on this platform!
	#endif
}

void ycMutex::Unlock()
{
	#if YC_NDA
	#elif defined YC_MSFT
		LeaveCriticalSection( (CRITICAL_SECTION*)data );
	#elif YC_NDA
	#elif defined YC_OSX && !defined YC_OSX_PTHREAD_MUTEX
		os_unfair_lock_assert_owner( &data );
		os_unfair_lock_unlock( &data );
	#elif defined YC_PTHREAD
		const int ret = pthread_mutex_unlock( &data );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}

#ifdef YC_TEST
#include "ycTest.h"

#include "ycThread.h"

namespace
{
	struct Args
	{
		Args() : m_guard( "Test" ), m_acquired( ureg(-1) ) {}
		ycMutex  m_guard;
		ureg     m_acquired;
	};
	void ThreadMain_Basic( void* pArgs )
	{
		Args* args = (Args*)pArgs;
		if( args->m_guard.TryLock() )
		{
			args->m_acquired = 1;
			args->m_guard.Unlock();
		}
		else
		{
			args->m_acquired = 0;
		}
	}
}

YC_BEGIN_TEST( ycMutex_Basic )
{
	Args args;
	ycThread::Handle thread = ycThread::Start( "Basic Mutex", ThreadMain_Basic, &args, 0 );
	ycThread::Join( thread );
	YC_TEST_CHECK( args.m_acquired == 1 );

	args.m_guard.Lock();

	thread = ycThread::Start( "Basic Mutex", ThreadMain_Basic, &args, 0 );
	ycThread::Join( thread );
	YC_TEST_CHECK( args.m_acquired == 0 );

	args.m_guard.Unlock();

	YC_TEST_PASS( "Basic Mutex" );
}

#endif // YC_TEST
