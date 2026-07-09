#include "ycSemaphore.h"

#if YC_NDA
#elif defined YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
#elif YC_NDA
#elif defined YC_OSX
	#include <unistd.h>
	//#include "ycStringUtils.h"  // making unique semaphore names
	#include <errno.h>
#elif defined YC_PTHREAD
	#include <time.h>
	#include <errno.h>
#endif

namespace
{
	enum { kMaxCount = 65536 };
}

ycSemaphore::ycSemaphore( const char* debugName, const uint32_t initialCount )
{
	#if defined YC_ENABLE_DEBUG_STRINGS
		m_debugName = debugName;
	#else
		YC_UNUSED( debugName );
	#endif

	#if YC_NDA
	#elif defined YC_MSFT
		static_assert( sizeof(m_semaphore) == sizeof(HANDLE), "" );
		static_assert( alignof(ycSemaphore) == alignof(HANDLE), "" );
		m_semaphore = CreateSemaphoreExW( NULL, initialCount, kMaxCount, NULL, 0, SEMAPHORE_ALL_ACCESS );
		ycAssert( m_semaphore != INVALID_HANDLE_VALUE );
	#elif YC_NDA
	#elif defined YC_OSX
		//static int semInd = 0;
		//ycStringUtils::SprintF( m_semUniqueName, 64, "%d%d", getpid(), semInd );
		//semInd++;
		//m_semaphore = sem_open( m_semUniqueName, O_CREAT | O_EXCL, S_IWUSR, initialCount );
		//ycAssertMsg( m_semaphore != SEM_FAILED, "Semaphore %s failed %d", m_semUniqueName, errno );
		m_semaphore = dispatch_semaphore_create( initialCount );
	#elif defined YC_PTHREAD
		const int ret = sem_init( &m_semaphore, 0, initialCount );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}

ycSemaphore::~ycSemaphore()
{
	#if YC_NDA
	#elif defined YC_MSFT
		const BOOL ret = CloseHandle( m_semaphore );
		ycAssert( ret == TRUE );
		YC_UNUSED( ret );
	#elif YC_NDA
	#elif defined YC_OSX
		m_semaphore = nullptr; // this is an obj refcounted object
	#elif defined YC_PTHREAD
		const int ret = sem_destroy( &m_semaphore );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}


void ycSemaphore::Wait()
{
	#if YC_NDA
	#elif defined YC_MSFT
		const DWORD ret = WaitForSingleObject( m_semaphore, INFINITE );
		ycAssert( ret == WAIT_OBJECT_0 );
		YC_UNUSED( ret );
	#elif YC_NDA
	#elif defined YC_OSX
		const intptr_t ret = dispatch_semaphore_wait( m_semaphore, DISPATCH_TIME_FOREVER );
		ycAssert( ret == 0 );
	#elif defined YC_PTHREAD
		const int ret = sem_wait( &m_semaphore );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}

bool ycSemaphore::Wait( u32 milliseconds )
{
	#if YC_NDA
	#elif defined YC_MSFT
		const DWORD ret = WaitForSingleObject( m_semaphore, milliseconds == 0 ? INFINITE : (DWORD)milliseconds );
		ycAssert( ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT );
		return ret != WAIT_TIMEOUT;
	#elif YC_NDA
	#elif defined YC_OSX
		dispatch_time_t timeout = DISPATCH_TIME_FOREVER;
		if( milliseconds > 0 ) timeout = dispatch_time( DISPATCH_TIME_NOW, int64_t(milliseconds) * 1000000ll );
		const intptr_t ret = dispatch_semaphore_wait( m_semaphore, timeout );
		return ret == 0;
	#elif defined YC_PTHREAD
		if( milliseconds > 0 )
		{
			struct timespec tt;
			clock_gettime( CLOCK_REALTIME, &tt );
			s64 nsec = s64(milliseconds) * 1000000ll;
			tt.tv_nsec += nsec;
			if( tt.tv_nsec >= 1000000000 )
			{
				tt.tv_nsec -= 1000000000;
				tt.tv_sec += 1;
			}
			const int ret = sem_timedwait( &m_semaphore, &tt );
			ycAssert( ret == 0 || errno == ETIMEDOUT );
			return errno != ETIMEDOUT;
		}
		else
		{
			Wait();
			return true;
		}
	#else
		#error not implemented on this platform!
		return false;
	#endif
}

bool ycSemaphore::TryWait()
{
	#if YC_NDA
	#elif defined YC_MSFT
		return WaitForSingleObject( m_semaphore, 0 ) == WAIT_OBJECT_0;
	#elif YC_NDA
	#elif defined YC_OSX
		const intptr_t ret = dispatch_semaphore_wait( m_semaphore, DISPATCH_TIME_NOW );
		return ret == 0;
	#elif defined YC_PTHREAD
		const int ret = sem_trywait( &m_semaphore );
		ycAssert( ret == 0 || ( ret == -1 && errno == EAGAIN ) );
		return ret == 0;
	#else
		#error not implemented on this platform!
	#endif
}

void ycSemaphore::Signal()
{
	#if YC_NDA
	#elif defined YC_MSFT
		const BOOL ret = ReleaseSemaphore( m_semaphore, 1, NULL );
		ycAssert( ret == TRUE ); YC_UNUSED( ret );
	#elif YC_NDA
	#elif defined YC_OSX
		dispatch_semaphore_signal( m_semaphore );
		//const int ret = sem_post( m_semaphore );
		//ycAssert( ret == 0 );
	#elif defined YC_PTHREAD
		const int ret = sem_post( &m_semaphore );
		ycAssert( ret == 0 );
	#else
		#error not implemented on this platform!
	#endif
}

void ycSemaphore::Signal( const uint32_t count )
{
	ycAssert( count );
	#if YC_NDA
	#elif defined YC_MSFT
		const BOOL ret = ReleaseSemaphore( m_semaphore, count, NULL );
		ycAssert( ret == TRUE ); YC_UNUSED( ret );
	#elif YC_NDA
	#else
		for( uint32_t i = 0; i != count; ++i )
		{
			Signal();
		}
	#endif
}

#ifdef YC_TEST
#include "ycTest.h"

#include "ycThread.h"

namespace
{
	struct Args
	{
		Args() : m_signal( "Test1" ), m_signalled( "Test2" ), m_quitSema( "Test3" ), m_count( 0 ) {}
		ycSemaphore m_signal;
		ycSemaphore m_signalled;
		ycSemaphore m_quitSema;
		int         m_count;
	};
	void ThreadMain_Basic( void* pArgs )
	{
		Args* args = (Args*)pArgs;
		for(;;)
		{
			args->m_signal.Wait();
			if( args->m_quitSema.TryWait() ) { break; }
			++args->m_count;
			args->m_signalled.Signal();
		}
	}
}

YC_BEGIN_TEST( ycSemaphore_Basic )
{
	Args args;
	ycThread::Handle thread = ycThread::Start( "Basic Semaphore", ThreadMain_Basic, &args, 0 );
	for( uint32_t i = 0; i != 10; ++i )
	{
		args.m_signal.Signal();
		args.m_signalled.Wait();
	}
	args.m_quitSema.Signal();
	args.m_signal.Signal();
	ycThread::Join( thread );

	YC_TEST_CHECK( args.m_count == 10 );

	YC_TEST_PASS( "Basic Semaphore" );
}

YC_BEGIN_TEST( ycSemaphore_InitialValue )
{
	enum { kInitialCount = 10 };
	ycSemaphore sema( "InitialValue Test", kInitialCount );
	for( ureg i = 0; i != kInitialCount; ++i )
	{
		YC_TEST_CHECK( sema.TryWait() == true );
	}
	YC_TEST_CHECK( sema.TryWait() == false );

	YC_TEST_PASS( "Initial Value" );
}

#endif // YC_TEST
