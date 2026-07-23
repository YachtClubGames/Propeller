#include "ycAtomic.h"

#include "ycAllocator.h" // prevent <atomic> from getting funny ideas about <new>
#include <atomic>

static_assert( sizeof(std::atomic<u32>) == sizeof(ycAtomic), "" );
static_assert( alignof(std::atomic<u32>) == alignof(ycAtomic), "" );
static_assert( sizeof(std::atomic<void*>) == sizeof(ycAtomicPtr), "" );
static_assert( alignof(std::atomic<void*>) == alignof(ycAtomicPtr), "" );
static_assert( sizeof(std::atomic<u64>) == sizeof(ycAtomicPtr), "" );
static_assert( alignof(std::atomic<u64>) == alignof(ycAtomicPtr), "" );
static_assert( int(kMemoryOrder_Relaxed) == int(std::memory_order_relaxed), "" );
static_assert( int(kMemoryOrder_Consume) == int(std::memory_order_consume), "" );
static_assert( int(kMemoryOrder_Acquire) == int(std::memory_order_acquire), "" );
static_assert( int(kMemoryOrder_Release) == int(std::memory_order_release), "" );
static_assert( int(kMemoryOrder_AcqRel) == int(std::memory_order_acq_rel), "" );
static_assert( int(kMemoryOrder_SeqCst) == int(std::memory_order_seq_cst), "" );

//////////
// ycAtomic
//////////

ycAtomic::ycAtomic( const u32 initialValue )
{
	new( this )std::atomic<u32>( initialValue );
}

u32 ycAtomic::Get() const
{
	return reinterpret_cast< const std::atomic<u32>* >( this )->load();
}

u32 ycAtomic::Load( const ycMemoryOrder order ) const
{
	return reinterpret_cast< const std::atomic<u32>* >( this )->load( std::memory_order( order ) );
}

void ycAtomic::Set( const u32 value )
{
	reinterpret_cast< std::atomic<u32>* >( this )->store( value );
}

void ycAtomic::Store( const u32 value, const ycMemoryOrder order )
{
	reinterpret_cast< std::atomic<u32>* >( this )->store( value, std::memory_order( order ) );
}

bool ycAtomic::CompareAndSwap( const u32 compare, const u32 swap )
{
	u32 _compare = compare;
	return reinterpret_cast< std::atomic<u32>* >( this )->compare_exchange_strong( _compare, swap );
}
bool ycAtomic::CompareAndSwap_Weak( const u32 compare, const u32 swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail )
{
	u32 _compare = compare;
	return reinterpret_cast< std::atomic<u32>* >( this )->compare_exchange_weak( _compare, swap, std::memory_order( orderSuccess ), std::memory_order( orderFail ) );
}

bool ycAtomic::CompareAndSwap_Strong( const u32 compare, const u32 swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail )
{
	u32 _compare = compare;
	return reinterpret_cast< std::atomic<u32>* >( this )->compare_exchange_strong( _compare, swap, std::memory_order( orderSuccess ), std::memory_order( orderFail ) );
}

u32 ycAtomic::Increment( const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_add( 1, std::memory_order( order ) );
}

u32 ycAtomic::Decrement( const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_sub( 1, std::memory_order( order ) );
}

u32 ycAtomic::BitwiseOr( const u32 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_or( val, std::memory_order( order ) );
}

u32 ycAtomic::BitwiseAnd( const u32 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_and( val, std::memory_order( order ) );
}

u32 ycAtomic::BitwiseXor( const u32 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_xor( val, std::memory_order( order ) );
}

u32 ycAtomic::Add( const u32 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_add( val, std::memory_order( order ) );
}

u32 ycAtomic::Subtract( const u32 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u32>* >( this )->fetch_sub( val, std::memory_order( order ) );
}

#if defined YC_ATOMIC_HAVE_MSFT_WAITONADDRESS

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
	#pragma comment( lib, "Synchronization.lib" )

	/*static*/ bool ycAtomic::HasWaitOnAddress()
	{
		return true;
	}

	void ycAtomic::WaitOnAddress( const u32 val )
	{
		const BOOL ret = ::WaitOnAddress( &m_value, (void*)&val, sizeof(u32), INFINITE );
		ycAssert( ret == TRUE ); YC_UNUSED( ret );
	}

	void ycAtomic::WakeByAddressAll()
	{
		::WakeByAddressAll( &m_value );
	}

#elif defined YC_ATOMIC_MAYBE_HAVE_MSFT_WAITONADDRESS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
	namespace
	{
		// HACK: using RtlWaitOnAddress which actually has a slightly different signature than WaitOnAddress! is it possible to dynamically load WaitOnAddress???
		//typedef BOOL (WINAPI *WaitOnAddressFuncPtr)( VOID volatile *, PVOID, SIZE_T, DWORD );
		typedef NTSTATUS (WINAPI *WaitOnAddressFuncPtr)( const void*, const void*, SIZE_T, const LARGE_INTEGER* );
		WaitOnAddressFuncPtr pWaitOnAddress = nullptr;
		typedef VOID (WINAPI *WakeByAddressAllFuncPtr)( PVOID );
		WakeByAddressAllFuncPtr pWakeByAddressAll = nullptr;

		bool s_waitOnAddressInitialized = false;
		void InitWaitOnaddress()
		{
			if( s_waitOnAddressInitialized ) { return; }
			s_waitOnAddressInitialized = true;
			// these aren't actually in kernel32? hacking around via Rtl functions atm
			//HMODULE kernel32dll = GetModuleHandleA( "kernel32.dll" );
			//pWaitOnAddress = (WaitOnAddressFuncPtr)GetProcAddress( kernel32dll, "WaitOnAddress" );
			//pWakeByAddressAll = (WakeByAddressAllFuncPtr)GetProcAddress( kernel32dll, "WakeByAddressAll" );
			HMODULE kernel32dll = GetModuleHandleA( "ntdll.dll" );
			pWaitOnAddress = (WaitOnAddressFuncPtr)GetProcAddress( kernel32dll, "RtlWaitOnAddress" );
			pWakeByAddressAll = (WakeByAddressAllFuncPtr)GetProcAddress( kernel32dll, "RtlWakeAddressAll" );
			if( pWaitOnAddress == nullptr || pWakeByAddressAll == nullptr ) // all or nothing
			{
				pWaitOnAddress = nullptr;
				pWakeByAddressAll = nullptr;
			}
		}

		/*static*/ bool ycAtomic::HasWaitOnAddress()
		{
			InitWaitOnaddress();
			return pWaitOnAddress != nullptr;
		}

		void ycAtomic::WaitOnAddress( const u32 val )
		{
			NTSTATUS ret = pWaitOnAddress( &m_value, (void*)&val, sizeof(u32), nullptr );
			ycAssert( ret == 0/*STATUS_SUCCESS*/ ); YC_UNUSED( ret );
		}

		void ycAtomic::WakeByAddressAll()
		{
			pWakeByAddressAll( &m_value );
		}
	}
#endif

//////////
// ycAtomicPtr
//////////

ycAtomicPtr::ycAtomicPtr( void* initialValue )
{
	new( this )std::atomic<void*>( initialValue );
}

void* ycAtomicPtr::Get() const
{
	return reinterpret_cast< const std::atomic<void*>* >( this )->load();
}

void* ycAtomicPtr::Load( const ycMemoryOrder order ) const
{
	return reinterpret_cast< const std::atomic<void*>* >( this )->load( std::memory_order( order ) );
}

void ycAtomicPtr::Set( void* value )
{
	reinterpret_cast< std::atomic<void*>* >( this )->store( value );
}

void ycAtomicPtr::Store( void* value,  ycMemoryOrder order )
{
	reinterpret_cast< std::atomic<void*>* >( this )->store( value, std::memory_order( order ) );
}

bool ycAtomicPtr::CompareAndSwap( void* compare, void* swap )
{
	void* _compare = compare;
	return reinterpret_cast< std::atomic<void*>* >( this )->compare_exchange_strong( _compare, swap );
}

bool ycAtomicPtr::CompareAndSwap_Weak( void* compare, void* swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail )
{
	void* _compare = compare;
	return reinterpret_cast< std::atomic<void*>* >( this )->compare_exchange_weak( _compare, swap, std::memory_order( orderSuccess ), std::memory_order( orderFail ) );
}

bool ycAtomicPtr::CompareAndSwap_Strong( void* compare, void* swap, const ycMemoryOrder orderSuccess, const ycMemoryOrder orderFail )
{
	void* _compare = compare;
	return reinterpret_cast< std::atomic<void*>* >( this )->compare_exchange_strong( _compare, swap, std::memory_order( orderSuccess ), std::memory_order( orderFail ) );
}

u64 ycAtomicPtr::Increment( const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_add( 1, std::memory_order( order ) );
}

u64 ycAtomicPtr::Decrement( const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_sub( 1, std::memory_order( order ) );
}

u64 ycAtomicPtr::BitwiseOr( const u64 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_or( val, std::memory_order( order ) );
}

u64 ycAtomicPtr::BitwiseAnd( const u64 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_and( val, std::memory_order( order ) );
}

u64 ycAtomicPtr::BitwiseXor( const u64 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_xor( val, std::memory_order( order ) );
}

u64 ycAtomicPtr::Add( const u64 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_add( val, std::memory_order( order ) );
}

u64 ycAtomicPtr::Subtract( const u64 val, const ycMemoryOrder order )
{
	return reinterpret_cast< std::atomic<u64>* >( this )->fetch_sub( val, std::memory_order( order ) );
}

#ifdef YC_TEST
#include "ycTest.h"
#include "ycThread.h"
#include "ycDateTime.h"

namespace
{
	enum
	{
		kNumThreads         = 4,
		#ifdef YC_ENABLE_SLOW_TESTS
			kTestDurationMs = 1000*30,
			kTestSpins      = 500000
		#else
			kTestDurationMs = 1000*3,
			kTestSpins      = 10000
		#endif
	};
	struct IncrementWorkerData
	{
		ycAtomic*        globalVal;
		u32         localVal;
		ycThread::Handle thread;
	};
	void ThreadMain_Increment( void* pArgs )
	{
		IncrementWorkerData* workerData = (IncrementWorkerData*)pArgs;
		u32 localVal = 0;
		const uint64_t startTime = ycDateTime::GetTimeHiRes();
		while( ycDateTime::GetElapsedMilliseconds( startTime ) < kTestDurationMs )
		{
			for( ureg i = 0; i != 10000; ++i )
			{
				workerData->globalVal->Increment();
				++localVal;
			}
		}
		workerData->localVal = localVal;
	}
	struct SwapWorkerData
	{
		ycAtomic*        lockVar;
		u32*        globalVal;
		u32         localVal;
		ycThread::Handle thread;
	};
	enum
	{
		SPINLOCK_UNLOCKED,
		SPINLOCK_LOCKED
	};
	void ThreadMain_Swap( void* pArgs )
	{
		SwapWorkerData* workerData = (SwapWorkerData*)pArgs;
		u32 localVal = 0;
		const uint64_t startTime = ycDateTime::GetTimeHiRes();
		while( ycDateTime::GetElapsedMilliseconds( startTime ) < kTestDurationMs )
		{
			for( ureg i = 0; i != kTestSpins; ++i )
			{
				// acquire spin lock
				while( workerData->lockVar->CompareAndSwap( SPINLOCK_UNLOCKED, SPINLOCK_LOCKED ) == false )
				{
					ycThread::YieldThread();
				}

				// increment guarded variable
				++( *workerData->globalVal );
				++localVal;

				// release spin lock
				workerData->lockVar->Set( SPINLOCK_UNLOCKED );
			}
		}
		workerData->localVal = localVal;
	}
}

YC_BEGIN_TEST( ycAtomic_Increment )
{
	ycAtomic globalVal( 0 );
	IncrementWorkerData workerData[ kNumThreads ];
	for( ureg i = 0; i != kNumThreads; ++i )
	{
		workerData[ i ].globalVal = &globalVal;
		workerData[ i ].localVal  = 0;
		workerData[ i ].thread    = ycThread::Start( "Atomic Increment", ThreadMain_Increment, &workerData[i], 0 );
	}
	u32 expected = 0;
	for( ureg i = 0; i != kNumThreads; ++i )
	{
		ycThread::Join( workerData[ i ].thread );
		expected += workerData[ i ].localVal;
	}

	YC_TEST_CHECK( globalVal.Get() == expected );

	YC_TEST_PASS( "Atomic Increment" );
}

YC_BEGIN_TEST( ycAtomic_SpinLock )
{
	ycAtomic lockVar( SPINLOCK_UNLOCKED );
	u32 result = 0;
	SwapWorkerData workerData[ kNumThreads ];

	for( ureg i = 0; i != kNumThreads; ++i )
	{
		workerData[ i ].lockVar   = &lockVar;
		workerData[ i ].globalVal = &result;
		workerData[ i ].localVal  = 0;
		workerData[ i ].thread    = ycThread::Start( "Atomic Swap", ThreadMain_Swap, &workerData[i], 0 );
	}

	u32 expected = 0;
	for( ureg i = 0; i != kNumThreads; ++i )
	{
		ycThread::Join( workerData[ i ].thread );
		expected += workerData[ i ].localVal;
	}

	YC_TEST_CHECK( result == expected );

	YC_TEST_PASS( "Atomic Swap" );
}

#endif // YC_TEST
