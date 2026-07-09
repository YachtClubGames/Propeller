#include "ycVirtualMem.h"

#include "ycMath.h"

#if defined YC_MSFT
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#elif YC_NDA
#elif YC_NDA
#elif defined YC_NIX || defined YC_OSX
	#include <sys/mman.h>
	#include <unistd.h>
#else
	#error no virtual memory implementation for the current platform
#endif

namespace ycVirtualMem
{
#if defined YC_MSFT || defined YC_NIX || defined YC_OSX
	ycSize_t s_pageSize;
#endif
}

ycSize_t ycVirtualMem::GetPageSize()
{
#if defined YC_MSFT
	if( s_pageSize == 0 )
	{
		SYSTEM_INFO info;
		GetSystemInfo( &info );
		s_pageSize = info.dwAllocationGranularity;
	}
	return s_pageSize;
#elif YC_NDA
#elif YC_NDA
#elif defined YC_NIX || defined YC_OSX
	if( s_pageSize == 0 )
	{
		s_pageSize = getpagesize();
	}
	return s_pageSize;
#endif
}

ycSize_t ycVirtualMem::GetLargePageSize()
{
#if defined YC_MSFT
	// windows supports large pages, but requires admin + has other limitations
	// still might be wise to advertise 2MB pages and maybe the OS will optimize under the hood
	return GetPageSize();
#elif YC_NDA
#elif YC_NDA
#elif defined YC_NIX || defined YC_OSX
	// TODO: large pages (MAP_HUGETLB, MAP_HUGE_2MB)
	// https://www.kernel.org/doc/Documentation/vm/transhuge.txt
	// allocate blocks on a 2MB boundary with posix_memalign and then do a madvise(MADV_HUGEPAGE)
	// mac: VM_FLAGS_SUPERPAGE_SIZE_2MB
	return GetPageSize();
#endif
}

void* ycVirtualMem::Reserve( const ycSize_t size )
{
	void* mem = nullptr;
	#ifdef YC_MSFT
		mem = VirtualAlloc( nullptr, size, MEM_RESERVE, PAGE_READWRITE  );
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX || defined YC_OSX
		mem = mmap( 0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
	#endif
	ycAssert( mem );
	return mem;
}

void ycVirtualMem::Commit( void* base, const ycSize_t size
	#if YC_NDA
	#endif
)
{
	#ifdef YC_MSFT
		void* ret = VirtualAlloc( base, size, MEM_COMMIT, PAGE_READWRITE );
		ycAssert( ret == base );
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX || defined YC_OSX
		const int ret = mprotect( base, size, PROT_READ | PROT_WRITE );
		ycAssert( ret == 0 );
	#endif
}

void ycVirtualMem::Release( void* base, const ycSize_t size )
{
	#ifdef YC_MSFT
		YC_UNUSED( size ); // could pass into VirtualFree, but other platforms can't use it, so may as well not
		VirtualFree( base, 0, MEM_RELEASE );
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX || defined YC_OSX
		munmap( base, size );
	#endif
}

#if YC_NDA
#endif 

ycSize_t ycVirtualMem::RoundToPageSize( const ycSize_t size )
{
	const ycSize_t largePageSize = ycVirtualMem::GetLargePageSize();
	const ycSize_t pageSize = size >= largePageSize ? largePageSize : ycVirtualMem::GetPageSize() ;
	return ycRoundUpPow2( size, pageSize );
}
