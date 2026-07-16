#include "ycThread.h"

#ifndef YC_OSX

// sdk
#if defined YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
	#include "Avrt.h" // kFlag_HighPriority_Audio >= Vista
	#pragma comment( lib, "Avrt.lib" )
	#ifdef YC_WIN32
		#include <VersionHelpers.h>
	#endif
#elif YC_NDA
#elif YC_NDA
#elif defined YC_PTHREAD
	#include <pthread.h>
	#include <unistd.h> // usleep
#endif

// core
#include "ycAllocator.h"
#include "ycStd.h"
#include "ycSysAllocator.h"
#include "ycTLS.h"
#ifdef YC_ENABLE_THREAD_FUZZER
	#include "ycCircArray.h"
	#include "ycMutex.h"
	#include "ycRand.h"
	#include "ycVector.h"
	#define YC_THREAD_FUZZER_SLEEP_MIN 0
	#define YC_THREAD_FUZZER_SLEEP_MAX 5
#endif

const ycThread::Handle ycThread::kInvalidThread = 0;

ycAllocator* ycThread::s_allocator = ycSysAllocator::GetDefault();

namespace
{
	struct ThreadArgs
	{
		ThreadArgs( ycThread::EntryPoint _entryPt, void* _arg ) : entryPt( _entryPt ), arg( _arg ) {}
		static ThreadArgs* Create( ycThread::EntryPoint entryPt, void* arg ) { return new( YC_ALLOC( ycThread::s_allocator, sizeof(ThreadArgs), alignof(ThreadArgs), "ThreadArgs" ) )ThreadArgs( entryPt, arg ); }
		static void Destroy( ThreadArgs* threadArgs ) { YC_FREE( ycThread::s_allocator, threadArgs ); }
		ycThread::EntryPoint entryPt;
		void*                arg;
		#if YC_NDA
		#elif defined YC_MSFT
			HANDLE osHandle;
			DWORD  osId;
			bool   mmcssBoost;
		#endif
	};
	#if YC_NDA
	#elif defined YC_MSFT
		DWORD WINAPI ThreadMain_MSFT( void* pArg )
		{
			ycTLS::Init();
			ThreadArgs* args = (ThreadArgs*)pArg;
			ycTLS::SetPtr( ycTLS::kSlot_Internal, args );
			if( args->mmcssBoost )
			{
				DWORD taskIndex = 0;
				/*HANDLE h =*/ AvSetMmThreadCharacteristicsA( "Audio", &taskIndex ); // leaked handle, surely it gets destroyed with the thread... right?
			}
			args->entryPt( args->arg );
			ycTLS::SetPtr( ycTLS::kSlot_Internal, nullptr );
			HANDLE timer = ycTLS::GetPtr( ycTLS::kSlot_WaitableTimer );
			if( timer != 0 )
			{
				CloseHandle( timer );
			}
			return 0;
		}
		void SetThreadNameMSFT( DWORD dwThreadID, HANDLE dwThreadHandle, const char* threadName)
		{
			YC_UNUSED( dwThreadID );
			YC_UNUSED( dwThreadHandle );
			YC_UNUSED( threadName );
			#ifdef YC_ENABLE_DEBUG_STRINGS
				wchar_t buf[ 128 ] = {};
				MultiByteToWideChar( CP_UTF8, 0x00, threadName, -1, buf, YC_ARRAY_SIZE(buf) );
				SetThreadDescription( dwThreadHandle, buf ); // This only exists on Win10+
				#if 0//ndef YC_UWP
					// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2017
					#pragma pack(push,8)
					typedef struct tagTHREADNAME_INFO
					{
						DWORD dwType; // Must be 0x1000.
						LPCSTR szName; // Pointer to name (in user addr space).
						DWORD dwThreadID; // Thread ID (-1=caller thread).
						DWORD dwFlags; // Reserved for future use, must be zero.
					} THREADNAME_INFO;
					#pragma pack(pop)
					THREADNAME_INFO info;
					info.dwType = 0x1000;
					info.szName = threadName;
					info.dwThreadID = dwThreadID;
					info.dwFlags = 0;
					__try { RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info ); }
					__except(EXCEPTION_EXECUTE_HANDLER) {  }
				#endif
			#endif // YC_ENABLE_DEBUG_STRINGS
		}
	#elif YC_NDA
	#elif defined YC_PTHREAD
		void* ThreadMain_PThread( void* pArgs )
		{
			ycTLS::Init();
			ThreadArgs args = *( (ThreadArgs*)pArgs ); // copy args to local stack
			ThreadArgs::Destroy( (ThreadArgs*)pArgs ); // then free the original heap-allocated args
			args.entryPt( args.arg );
			return nullptr;
		}
	#endif
	#ifdef YC_ENABLE_THREAD_FUZZER
		ycMutex s_threadListGuard( "Thread List" );
		ycCircArray< ycThread::Handle, 128 > s_runningThreadList;
		ycCircArray< ycThread::Handle, 128 > s_pausedThreadList;
		bool s_fuzzerInitialized = false;
		void Fuzzer_PauseThread( ycThread::Handle threadHandle )
		{
		#ifdef YC_MSFT
			ThreadArgs* threadArgs = (ThreadArgs*)threadHandle;
			/*const DWORD ret =*/ SuspendThread( threadArgs->osHandle );
			//ycAssert( ret == 0 );
		#else
			#error thread fuzzer is not implemented on this platform
		#endif
		}
		void Fuzzer_ResumeThread( ycThread::Handle threadHandle )
		{
		#ifdef YC_MSFT
			ThreadArgs* threadArgs = (ThreadArgs*)threadHandle;
			//DWORD ret;
			//do
			//{
			//	ret = ResumeThread( threadArgs->osHandle );
			//} while( ret != DWORD(-1) && ret > 1 );
			/*const DWORD ret =*/ ResumeThread( threadArgs->osHandle );
			//ycAssert( ret == 1 );
		#else
			#error thread fuzzer is not implemented on this platform
		#endif
		}
		void Fuzzer_RegisterThread( ycThread::Handle threadHandle )
		{
			s_threadListGuard.Lock();
			if( !s_fuzzerInitialized )
			{
				s_fuzzerInitialized = true;
				ycThread::Start( "Thread Fuzzer", [](void*) {
					ycRand rng;
					for(;;)
					{
						ycThread::Sleep( rng.RangeFast( YC_THREAD_FUZZER_SLEEP_MIN, YC_THREAD_FUZZER_SLEEP_MAX ) );
						if( s_threadListGuard.TryLock() )
						{
							if( rng.Rand() & 1 )
							{
								if( s_pausedThreadList.Length() )
								{
									ycThread::Handle threadHandle = s_pausedThreadList.TakeUnsorted( rng.RangeFast( 0, u32(s_pausedThreadList.Length()-1) ) );
									Fuzzer_ResumeThread( threadHandle );
									s_runningThreadList.Append( threadHandle );
								}
							}
							else
							{
								if( s_runningThreadList.Length() )
								{
									ycThread::Handle threadHandle = s_runningThreadList.TakeUnsorted( rng.RangeFast( 0, u32(s_runningThreadList.Length()-1) ) );
									Fuzzer_PauseThread( threadHandle );
									s_pausedThreadList.Append( threadHandle );
								}
							}
							s_threadListGuard.Unlock();
						}
					}
				}, nullptr, 0, ycThread::kDefaultStackSize, ycThread::kFlag_Internal_ThreadFuzzer );
			}
			s_runningThreadList.Append( threadHandle );
			s_threadListGuard.Unlock();
		}
		void Fuzzer_UnregisterThread( ycThread::Handle threadHandle )
		{
			s_threadListGuard.Lock();
			for( ycSize_t i = 0; i != s_runningThreadList.Length(); ++i )
			{
				if( s_runningThreadList.Get( i ) == threadHandle )
				{
					s_runningThreadList.TakeUnsorted( i );
					break;
				}
			}
			for( ycSize_t i = 0; i != s_pausedThreadList.Length(); ++i )
			{
				if( s_pausedThreadList.Get( i ) == threadHandle )
				{
					Fuzzer_ResumeThread( threadHandle ); // dont leave it paused so it can be joined
					s_pausedThreadList.TakeUnsorted( i );
					break;
				}
			}
			s_threadListGuard.Unlock();
		}
		struct GlobalDtor
		{
			~GlobalDtor()
			{
				// unlock all threads and hold the lock to prevent pausing any more
				s_threadListGuard.Lock();
				while( s_pausedThreadList.Length() )
				{
					ycThread::Handle threadHandle = s_pausedThreadList.TakeFirst();
					Fuzzer_ResumeThread( threadHandle );
					s_runningThreadList.Append( threadHandle );
				}
				// intentionally leak the lock!
				//s_threadListGuard.Unlock();
			}
		} g_globalDtor;
	#endif // YC_ENABLE_THREAD_FUZZER
}

void ycThread::YieldThread()
{
	#if YC_NDA
	#elif defined YC_MSFT
		if( ::SwitchToThread() == FALSE ) { ::Sleep( 0 ); }
	#elif YC_NDA
	#elif defined YC_OSX
		sched_yield();
	#elif defined YC_PTHREAD
		pthread_yield();
	#else
		#error ycThread::YieldThread is not implemented on the current platform!
	#endif
}

void ycThread::Sleep( ureg milliseconds )
{
	#if YC_NDA
	#elif defined YC_MSFT
		if( HasMicroSleep() ) // timeBeginPeriod(1) is not as reliable as it used to be, let's try to avoid needing it
		{
			MicroSleep( milliseconds * 1000 );
		}
		else
		{
			::Sleep( DWORD(milliseconds) );
		}
	#elif YC_NDA
	#elif defined YC_PTHREAD
		usleep( uint32_t(milliseconds) * 1000 );
	#else
		#error ycThread::Sleep is not implemented on the current platform!
	#endif
}

bool ycThread::HasMicroSleep()
{
#if defined YC_WIN32
	return IsWindows10OrGreater();
#else
	return true;
#endif
}

void ycThread::MicroSleep( ureg microseconds )
{
	#if YC_NDA
	#elif defined YC_MSFT
		HANDLE timer = ycTLS::GetPtr( ycTLS::kSlot_WaitableTimer );
		if( timer == 0 )
		{
			timer = CreateWaitableTimerExW( nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS );
			ycAssert( timer );
		}
		LARGE_INTEGER duration;
		duration.QuadPart = -LONGLONG( microseconds * 10 ); // 100 nanosecond intervals, negative is relative (not absolute) time
		BOOL timerSet = SetWaitableTimer( timer, &duration, 0, nullptr, nullptr, FALSE );
		ycAssert( timerSet ); YC_UNUSED( timerSet );
		DWORD ret = WaitForSingleObject( timer, INFINITE );
		ycAssert( ret == WAIT_OBJECT_0 ); YC_UNUSED( ret );
	#elif YC_NDA
	#elif defined YC_PTHREAD
		usleep( uint32_t(microseconds) );
	#else
		#error ycThread::Sleep is not implemented on the current platform!
	#endif
}

ycThread::Handle ycThread::Start( const char* name, EntryPoint entryPt, void* arg, const ureg coreIdx, const ycSize_t stackSize, const ureg flags )
{
	// TODO HACK FIXME: validate coreIdx, better priority support
	Handle threadHandle;
	#if YC_NDA
	#elif defined YC_MSFT
		YC_UNUSED( coreIdx );
		ThreadArgs* threadArgs = ThreadArgs::Create( entryPt, arg ); // freed when the thread is joined
		threadArgs->mmcssBoost = (flags&kFlag_HighPriority_Audio) != 0; // could do this for the fuzzer too...
		threadArgs->osHandle = ::CreateThread( NULL, stackSize, ThreadMain_MSFT, threadArgs, CREATE_SUSPENDED, &threadArgs->osId ); // create suspended so we ensure we get threadHandle.osId before proceeding
		ycAssert( threadArgs->osHandle != NULL );
		int priority = THREAD_PRIORITY_NORMAL;
		if( flags & kFlag_Internal_ThreadFuzzer )
		{
			priority = THREAD_PRIORITY_TIME_CRITICAL;
		}
		if( flags&(kFlag_HighPriority_Audio|kFlag_HighPriority) )
		{
			priority = THREAD_PRIORITY_ABOVE_NORMAL;
		}
		else if( flags&kFlag_LowPriority )
		{
			priority = THREAD_PRIORITY_BELOW_NORMAL;
		}
		if( priority != THREAD_PRIORITY_NORMAL )
		{
			BOOL ret = SetThreadPriority( threadArgs->osHandle, priority );
			ycAssert( ret == TRUE ); YC_UNUSED( ret );
		}
		ResumeThread( threadArgs->osHandle );
		SetThreadNameMSFT( threadArgs->osId, threadArgs->osHandle, name );
		threadHandle = threadArgs;
	#elif YC_NDA
	#elif defined YC_PTHREAD
		ycAssert( (flags&~(kFlag_HighPriority|kFlag_HighPriority_Audio|kFlag_LowPriority)) == 0 ); // ignore priority-based flags, anything else NYI/TODO
		YC_UNUSED( coreIdx );
		ThreadArgs* threadArgs = ThreadArgs::Create( entryPt, arg ); // freed by the spawned thread
		pthread_attr_t pthreadAttrs;
		pthread_attr_init( &pthreadAttrs );
		pthread_attr_setstacksize( &pthreadAttrs, stackSize );
		pthread_t pthreadHandle;
		int ret = pthread_create( &pthreadHandle, &pthreadAttrs, ThreadMain_PThread, threadArgs );
		ycAssert( ret == 0 );
		#ifdef __USE_GNU
			pthread_setname_np( pthreadHandle, name );
		#endif
		threadHandle = Handle(pthreadHandle);
	#else
		#error ycThread::Start not implemented!
	#endif
	#ifdef YC_ENABLE_THREAD_FUZZER
	if( (flags&kFlag_Internal_ThreadFuzzer) == 0 )
	{
		Fuzzer_RegisterThread( threadHandle );
	}
	#endif // YC_ENABLE_THREAD_FUZZER
	return threadHandle;
}

void ycThread::Join( Handle threadHandle )
{
	#ifdef YC_ENABLE_THREAD_FUZZER
		Fuzzer_UnregisterThread( threadHandle );
	#endif // YC_ENABLE_THREAD_FUZZER
	#if YC_NDA
	#elif defined YC_MSFT
		ThreadArgs* threadArgs = (ThreadArgs*)threadHandle;
		DWORD ret = WaitForSingleObject( threadArgs->osHandle, INFINITE );
		ycAssert( ret == WAIT_OBJECT_0 ); YC_UNUSED( ret );
		BOOL closed = CloseHandle( threadArgs->osHandle );
		ycAssert( closed == TRUE ); YC_UNUSED( closed );
		ThreadArgs::Destroy( threadArgs );
	#elif YC_NDA
	#elif defined YC_PTHREAD
		const int ret = pthread_join( (pthread_t)threadHandle, nullptr );
		ycAssert( ret == 0 );
	#else
		#error ycThread::Join not implemented!
	#endif
}

ycThread::Handle ycThread::GetCurrent()
{
	#if YC_NDA
	#elif defined YC_MSFT
		return ycTLS::GetPtr( ycTLS::kSlot_Internal );
	#elif YC_NDA
	#elif defined YC_PTHREAD
		return Handle(pthread_self());
	#else
		#error ycThread::GetCurrent not implemented!
	#endif
}

#if YC_NDA
#endif

u64 ycThread::GetPrintableID( Handle threadHandle )
{
	#if YC_NDA
	#elif defined YC_MSFT
		ThreadArgs* args = (ThreadArgs*)threadHandle;
		return args->osId;
	#elif YC_NDA
	#elif defined YC_PTHREAD
		return uintptr_t( threadHandle );
	#else
		#error ycThread::GetPrintableID not implemented!
	#endif
}

#if defined YC_WIN32
void* ycThread::Win32_GetHandle( Handle threadHandle )
{
	ThreadArgs* args = (ThreadArgs*)threadHandle;
	return args->osHandle;
}
unsigned long ycThread::Win32_GetId( Handle threadHandle )
{
	ThreadArgs* args = (ThreadArgs*)threadHandle;
	return args->osId;
}
#endif // YC_WIN32

#endif // !YC_OSX

#ifdef YC_TEST
#include "ycTest.h"

#if defined YC_PTHREAD && !defined YC_OSX
YC_BEGIN_TEST( ycThread_pthread_t )
{
	YC_TEST_CHECK( sizeof(pthread_t) <= sizeof(uint64_t) );
	YC_TEST_PASS( "sizeof(pthread_t)" );
}
#endif

namespace
{
	void ThreadMain_Basic( void* pArg )
	{
		int* arg = (int*)pArg;
		for( uint32_t i = 0; i != 10; ++i )
		{
			++*arg;
		}
	}
}

YC_BEGIN_TEST( ycThread_Basic )
{
	int arg = 0;
	ycThread::Handle thread = ycThread::Start( "Basic Thread", ThreadMain_Basic, &arg, 0 );
	ycThread::Join( thread );

	YC_TEST_CHECK( arg == 10 );

	YC_TEST_PASS( "Basic Thread" );
}

#endif // YC_TEST
