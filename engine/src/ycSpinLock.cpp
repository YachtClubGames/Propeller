#include "ycSpinLock.h"

#include "ycThread.h"

namespace
{
	enum
	{
		kState_Unlocked = 0,
		kState_Locked   = 1
	};
}

ycSpinLock::ycSpinLock( const char* debugName ) :
	m_lock( kState_Unlocked )
{
#ifdef YC_ENABLE_DEBUG_STRINGS
	m_debugName = debugName;
#else
	YC_UNUSED( debugName );
#endif
}

ycSpinLock::~ycSpinLock()
{
	ycAssert( m_lock.Get() == kState_Unlocked );
}

void ycSpinLock::Lock( const ureg methodOrMs )
{
	if( methodOrMs == kSpinMethod_Loop )
	{
		while( !TryLock() ) { /**/ }
	}
	else if( methodOrMs == kSpinMethod_Yield )
	{
		while( !TryLock() ) { ycThread::YieldThread(); }
	}
	else
	{
		while( !TryLock() ) { ycThread::Sleep( methodOrMs ); }
	}
}

bool ycSpinLock::TryLock()
{
	return m_lock.CompareAndSwap( kState_Unlocked, kState_Locked );
}

void ycSpinLock::Unlock()
{
#ifdef YC_ENABLE_ASSERT
	ycAssert( m_lock.CompareAndSwap( kState_Locked, kState_Unlocked ) );
#else
	m_lock.Set( kState_Unlocked );
#endif
}
