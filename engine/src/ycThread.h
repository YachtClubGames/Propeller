#pragma once

/*! \class ycThread
*/

#include "ycCommon.h"
#include "ycPlatformConfig.h"

class ycAllocator;

// randomly freezes threads for brief periods to stir up race conditions
//#define YC_ENABLE_THREAD_FUZZER

namespace ycThread
{
	void YieldThread(); //!< on some (power conscious) platforms this will sleep for the shortest possible period // sigh windows.h #define's Yield()
	void Sleep( ureg milliseconds );

	bool HasMicroSleep();
	void MicroSleep( ureg microseconds ); // wont work on windows 8

	// TODO: might be nice to have a sleep specifically for busy loops (platform-appropriate duration)

	#if YC_NDA
	#elif defined YC_MSFT
		typedef void* Handle; // ptr to custom struct, which contains HANDLE and ID
	#elif defined YC_OSX
		typedef void* Handle;
	#elif defined YC_PTHREAD
		typedef uint64_t Handle;
	#elif YC_NDA
	#else
		#error ycThread is not implemented on the current platform, or the current platform cannot be detected
	#endif

	extern const Handle kInvalidThread;

	typedef void( *EntryPoint )( void* );

	enum
	{
		kDefaultStackSize = 16*1024 //!< low default so higher must be explicit
	};
	enum
	{
		kFlag_HighPriority       = 1<<0,
		kFlag_HighPriority_Audio = 1<<1, //!< some platforms may have special scheduling for multimedia tasks
		kFlag_LowPriority        = 1<<2,

		kFlag_Internal_ThreadFuzzer = 1<<10, //!< do not use
	};

	Handle Start( const char* name, EntryPoint entryPt, void* arg, const ureg coreIdx, const ycSize_t stackSize = kDefaultStackSize, const ureg flags = 0 );
	void Join( Handle threadHandle );

	#if defined YC_OSX
		bool IsDone( Handle threadHandle );
		bool IsUiThread();
		void RunUiThreadTasks();
	#endif // YC_OSX

	#if defined YC_WIN32
		//! expose windows internals, sometimes needed to pass to Win32 APIs
		void* Win32_GetHandle( Handle threadHandle );
		unsigned long Win32_GetId( Handle threadHandle );
	#endif // YC_WIN32

	Handle GetCurrent(); //!< the return value should only be used for comparison, it should not be passed to JoinThread()

	u64 GetPrintableID( Handle threadHandle ); //!< returns a unique identifer for a Thread::Handle

	extern ycAllocator* s_allocator; //!< defaults to ycSysAllocator; if you want to change this, it must be changed before any threads are started!

#if YC_NDA
#endif
}
