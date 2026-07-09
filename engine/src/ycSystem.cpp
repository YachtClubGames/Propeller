#include "ycSystem.h"

// core
#include "ycAllocator.h"
#include "ycBitUtils.h"
#include "ycStringUtils.h"

#include <stdlib.h> // _Exit

// sdk
#ifdef YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define NOMINMAX
	#include <Windows.h>
	#include <stdlib.h> // getenv/setenv
#endif // YC_MSFT

#if defined YC_NIX || defined YC_OSX
	#include <stdlib.h> // getenv/setenv
	#include <string.h> // strlen, memcpy
	#include <stdio.h> // popen/pclose
	#include <unistd.h> // gethostname
	#include "ycStringUtils.h"
#endif // YC_NIX

#if defined YC_OSX
	#include <sys/types.h>
	#include <sys/sysctl.h>
#endif // YC_OSX

#if YC_NDA
#endif

#if YC_NDA
#endif

// std
//#include <stdlib.h> // getenv_s

void ycSystem::GetCpuCount( ureg* physicalCores, ureg* logicalCores )
{
	#if defined YC_MSFT
		DWORD bufSize = 0;
		BOOL ret = GetLogicalProcessorInformation( nullptr, &bufSize );
		ycAssert( ret == FALSE && bufSize > 0 );
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION* procInfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)YC_ALLOC( g_defaultMem, bufSize, YC_MEM_DEFAULT_ALIGN, "SYSTEM_LOGICAL_PROCESSOR_INFORMATION" );
		DWORD retSize = bufSize;
		ret = GetLogicalProcessorInformation( procInfo, &retSize );
		if( ret == TRUE && retSize == bufSize )
		{
			ycAssert( (retSize%sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) == 0 );
			const ureg numProcInfos = retSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ureg numPhysicalCores = 0;
			ureg numLogicalCores = 0;
			for( ureg i = 0; i != numProcInfos; ++i )
			{
				if( procInfo[i].Relationship != RelationProcessorCore ) { continue; }
				++numPhysicalCores;
				numLogicalCores += ycBitUtils::PopCount64( procInfo[i].ProcessorMask );
			}
			*physicalCores = numPhysicalCores;
			*logicalCores  = numLogicalCores;
		}
		else
		{
			SYSTEM_INFO sysInfo;
			GetSystemInfo( &sysInfo );
			const DWORD numProcessors = sysInfo.dwNumberOfProcessors;
			*physicalCores = numProcessors; // might be better do max( 1, numProcessors/2 ) and assume HT is on; on older AMD this probably fine because of their poor scaling.
			*logicalCores  = numProcessors;
		}
		YC_FREE( g_defaultMem, procInfo );
	#elif YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX
		*physicalCores = 4; // guessy defaults because the code below can fail
		*logicalCores  = 4;
		FILE* fh = popen( "lscpu", "r" );
		ycAssert( fh );
		char buf[ 16*1024 ];
		const size_t bytesRead = fread( buf, 1, sizeof(buf), fh );
		buf[ bytesRead ] = '\0';
		pclose( fh );
		const char* pos;
		int threadsPerCore = 0;
		pos = strstr( buf, "Thread(s) per core:" );
		if( pos )
		{
			pos += 19;
			while( *pos == ' ' ) { ++pos; }
			threadsPerCore = ycStringUtils::ToInt( pos );
		}
		int coresPerSocket = 0;
		pos = strstr( buf, "Core(s) per socket:" );
		if( pos )
		{
			pos += 19;
			while( *pos == ' ' ) { ++pos; }
			coresPerSocket = ycStringUtils::ToInt( pos );
		}
		int sockets = 0;
		pos = strstr( buf, "Socket(s):" );
		if( pos )
		{
			pos += 10;
			while( *pos == ' ' ) { ++pos; }
			sockets = ycStringUtils::ToInt( pos );
		}
		if( threadsPerCore && coresPerSocket && sockets )
		{
			*physicalCores = coresPerSocket * sockets;
			*logicalCores = coresPerSocket * sockets * threadsPerCore;
		}
	#elif defined YC_OSX
		*physicalCores = 4; // guessy defaults because the code below can fail
		*logicalCores  = 4;

		int mib[4];
		int numCPU;
		size_t len = sizeof(numCPU);

		mib[0] = CTL_HW;
		mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

		// get the number of CPUs from the system
		sysctl( mib, 2, &numCPU, &len, NULL, 0 );

		if( numCPU < 1 )
		{
			mib[1] = HW_NCPU;
			sysctl(mib, 2, &numCPU, &len, NULL, 0);
			if( numCPU < 1 ) { numCPU = 1; }
		}
		
		*physicalCores = numCPU;
		*logicalCores = numCPU;
	#else
		#error No core count implementation
		ycFatal();
	#endif
}

ycSize_t ycSystem::GetEnv( char* dstBuf, const ycSize_t bufSize, const char* name )
{
	#if defined YC_MSVC
		size_t retLen = 0;
		errno_t ret = getenv_s( &retLen, dstBuf, bufSize, name );
		if( ret == ERANGE && bufSize ) { dstBuf[ 0 ] = '\0'; }
		return ycSize_t(retLen);
	#elif defined YC_NIX || defined YC_OSX
		const char* val = getenv( name );
		if( val == nullptr ) { return 0; }
		const ycSize_t len = strlen( val ) + 1 /*terminating NUL*/ ;
		if( len <= bufSize )
		{
			memcpy( dstBuf, val, len );
		}
		else if( bufSize )
		{
			dstBuf[ 0 ] = '\0';
		}
		return len;
	#elif YC_NDA
	#else
		#error ycSystem::GetEnv() not implemented on this platform!
		return 0;
	#endif
}

bool ycSystem::GetEnv( const char* name )
{
	char buf[64];
	if( GetEnv( buf, sizeof(buf), name ) )
	{
		if( ycStringUtils::EqualsI( buf, "0") || ycStringUtils::EqualsI( buf, "false") )
		{
			return false;
		}
		return true;
	}
	return false;
}

void ycSystem::SetEnv( const char* name, const char* val )
{
	if( val == nullptr ) { val = ""; }
	#if defined YC_MSVC
		_putenv_s( name, val );
	#elif defined YC_NIX || defined YC_OSX
		setenv( name, val, 1 );
	#else
		YC_UNUSED( name );
		YC_UNUSED( val );
		ycFatal();
	#endif
}

void ycSystem::UnsetEnv( const char* name )
{
	#if defined YC_MSVC
		_putenv_s( name, "" );
	#elif defined YC_NIX || defined YC_OSX
		unsetenv( name );
		YC_UNUSED( name );
		ycFatal();
	#endif
}

bool ycSystem::IsBuildServer()
{
#if defined YC_MSFT || defined YC_NIX || defined YC_OSX
	char buf[16];
	return ycSystem::GetEnv( buf, sizeof(buf), "NOGUI" ) && ycSystem::GetEnv( buf, sizeof(buf), "NOPAUSE" );
#else
	return false;
#endif
}

bool ycSystem::IsRunningAsAService()
{
#if defined YC_WIN32
	DWORD session = 1;
	ProcessIdToSessionId( GetCurrentProcessId(), &session );
	return session == 0;
#else
	return false;
#endif
}

#if !defined YC_OSX
ycSize_t ycSystem::GetSystemName( char* dstBuf, const ycSize_t bufSize )
{
#ifdef YC_MSFT
	const ycSize_t rs = GetEnv( dstBuf, bufSize, "COMPUTERNAME" );
	if( rs > 0 ) { return rs - 1/*NUL*/; }
#elif defined YC_NIX 
	const ycSize_t rs = GetEnv( dstBuf, bufSize, "HOSTNAME" );
	if( rs > 0 ) { return rs - 1/*NUL*/; }
	const int ret = gethostname( dstBuf, bufSize );
	if( ret == 0 ) { return ycStringUtils::Length( dstBuf ); }
#elif YC_NDA
#endif

	return ycStringUtils::Copy( dstBuf, "UNKNOWN", bufSize );
}
#endif

#if YC_NDA
#else
bool ycSystem::IsHandHeld()
{
	return false;
}
#endif

#if defined YC_STEAM
bool g_isSteamOverlayActive = false;
bool ycSystem::IsOverlay()
{
	return g_isSteamOverlayActive;
}
#elif YC_NDA
#elif YC_NDA
#else
bool ycSystem::IsOverlay()
{
	return false;
}
#endif

#ifdef YC_NDA
#endif


#ifdef YC_NIX
	bool g_isWayland = false;
	bool ycSystem::IsWayland()
	{
		return g_isWayland;
	}
#endif

void ycSystem::Exit()
{
#if YC_NDA
#else
	// this has a pointless branch around it,
	// the compiler is awfully good at detecting unused code after the _Exit call
	// (even outside of this function), so let's wrap it in a branch to hide from
	// the optimizer!
	if( g_defaultMem ) { _Exit( 0 ); }
#endif
}