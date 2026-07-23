#include "ycRWLock.h"

#if defined YC_RWLOCK_USE_FALLBACK
	//
#elif defined YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	#include <errno.h> // EBUSY
#endif

ycRWLock::ycRWLock( const char* debugName )
	#if defined YC_RWLOCK_USE_FALLBACK
		:
		#if defined YC_ENABLE_DEBUG_STRINGS
		m_debugName( debugName ),
		#endif
		m_readGuard( debugName ),
		m_numReaders( 0 ),
		m_writeGuard( debugName, 1 )
	#elif defined YC_ENABLE_DEBUG_STRINGS
	: m_debugName( debugName )
	#endif
{
	YC_UNUSED( debugName );
#if defined YC_RWLOCK_USE_FALLBACK
	// nop
#elif defined YC_MSFT
	static_assert( sizeof(SRWLOCK) == sizeof(m_data), "" );
	static_assert( __alignof(SRWLOCK) == __alignof(m_data), "" );
	InitializeSRWLock( (SRWLOCK*)&m_data );
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_init( (pthread_rwlock_t*)&m_data, nullptr );
	ycAssert( ret == 0 );
#else
	#error not implemented on this platform!
#endif
}

ycRWLock::~ycRWLock()
{
#if defined YC_RWLOCK_USE_FALLBACK
	// nop
#elif defined YC_MSFT
	// nop
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_destroy( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 );
#else
	#error not implemented on this platform!
#endif
}

void ycRWLock::LockRead()
{
#if defined YC_RWLOCK_USE_FALLBACK
	m_readGuard.Lock();
	if( ++m_numReaders == 1 ) { m_writeGuard.Wait(); ycAssert( !m_writeGuard.TryWait() ); }
	m_readGuard.Unlock();
#elif defined YC_MSFT
	AcquireSRWLockShared( (SRWLOCK*)&m_data );
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_rdlock( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 );
#else
	#error not implemented on this platform!
#endif
}

bool ycRWLock::TryLockRead()
{
#if defined YC_RWLOCK_USE_FALLBACK
	if( m_readGuard.TryLock() )
	{
		if( ++m_numReaders == 1 )
		{
			if( m_writeGuard.TryWait() )
			{
				ycAssert( !m_writeGuard.TryWait() );
				return true;
			}
			else
			{
				m_numReaders= 0;
				m_readGuard.Unlock();
				return false;
			}
		}
		m_readGuard.Unlock();
		return true;
	}
	return false;
#elif defined YC_MSFT
	return TryAcquireSRWLockShared( (SRWLOCK*)&m_data ) != 0;
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_tryrdlock( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 || ret == EBUSY );
	return ret == 0;
#else
	#error not implemented on this platform!
#endif
}

void ycRWLock::UnlockRead()
{
#if defined YC_RWLOCK_USE_FALLBACK
	m_readGuard.Lock();
	ycAssert( m_numReaders > 0 );
	if( --m_numReaders == 0 ) { ycAssert( !m_writeGuard.TryWait() ); m_writeGuard.Signal(); }
	m_readGuard.Unlock();
#elif defined YC_MSFT
	ReleaseSRWLockShared( (SRWLOCK*)&m_data );
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_unlock( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 );
#else
	#error not implemented on this platform!
#endif
}

void ycRWLock::LockWrite()
{
#if defined YC_RWLOCK_USE_FALLBACK
	m_writeGuard.Wait();
	ycAssert( !m_writeGuard.TryWait() );
#elif defined YC_MSFT
	AcquireSRWLockExclusive( (SRWLOCK*)&m_data );
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_wrlock( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 );
#else
	#error not implemented on this platform!
#endif
}

bool ycRWLock::TryLockWrite()
{
#if defined YC_RWLOCK_USE_FALLBACK
	if( m_writeGuard.TryWait() )
	{
		ycAssert( !m_writeGuard.TryWait() );
		return true;
	}
	return false;
#elif defined YC_MSFT
	return TryAcquireSRWLockExclusive( (SRWLOCK*)&m_data ) != 0;
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_trywrlock( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 || ret == EBUSY );
	return ret == 0;
#else
	#error not implemented on this platform!
#endif
}

void ycRWLock::UnlockWrite()
{
#if defined YC_RWLOCK_USE_FALLBACK
	ycAssert( !m_writeGuard.TryWait() );
	m_writeGuard.Signal();
#elif defined YC_MSFT
	ReleaseSRWLockExclusive( (SRWLOCK*)&m_data );
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	const int ret = pthread_rwlock_unlock( (pthread_rwlock_t*)&m_data );
	ycAssert( ret == 0 );
#else
	#error not implemented on this platform!
#endif
}
