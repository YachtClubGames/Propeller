#include "ycSysAllocator.h"
#include "ycMath.h"
#include "ycMemTrack.h"
#include "ycStd.h"

#if defined YC_ALLOC_RAND_FILL
	#include "ycRand.h"
#endif // YC_ALLOC_RAND_FILL

#ifdef YC_ALLOC_PADDING_GUARD
	#ifdef YC_MSFT
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
		#endif
		#include <Windows.h>
		namespace
		{
			ycSize_t s_pageSize = 0;
			ycSize_t s_allocGranularity = 0;
			bool s_vmemGuardAllocEnd = true;
		}
	#endif // YC_MSFT
	#ifdef YC_NDA
	#endif
#endif // YC_ALLOC_PADDING_GUARD

#if defined YC_ALLOC_PADDING_CHECK && defined YC_ALLOC_PADDING_GUARD
	#error YC_ALLOC_PADDING_CHECK and YC_ALLOC_PADDING_GUARD cannot be used together
	/* is technically possible, but header would need to store some extra info */
#endif

#ifdef YC_ALLOC_PADDING_CHECK
#define YC_ALLOC_PADDING_CHECK_SIZE 16
namespace
{
	static_assert( ( YC_ALLOC_PADDING_CHECK_SIZE % sizeof(ureg) ) == 0, "" );
	constexpr ureg s_sentinel = 0xdeadbeefdeadbeefllu;
	void Sentinel_Fill( void* pDst )
	{
		ureg* dst = (ureg*)pDst;
		constexpr ureg count = YC_ALLOC_PADDING_CHECK_SIZE / sizeof(ureg);
		for( ureg i = 0; i != count; ++i )
		{
			YC_UNALIGNED_WRITE( ureg, dst + i, &s_sentinel );
		}
	}
	void Sentinel_Check( const void* pSrc )
	{
		const ureg* src = (const ureg*)pSrc;
		constexpr ureg count = YC_ALLOC_PADDING_CHECK_SIZE / sizeof(ureg);
		for( ureg i = 0; i != count; ++i )
		{
			ureg chk;
			YC_UNALIGNED_READ( ureg, &chk, src + i );
			ycAssert( chk == s_sentinel );
		}
	}

}
#endif // YC_ALLOC_PADDING_CHECK

#if defined YC_ALLOC_RAND_FILL
namespace
{
	ycRand s_allocFillRand;
	void RandFill( void* dst, const size_t numBytes )
	{
		u32 rand = s_allocFillRand.Rand(); // this isn't thread safe -- and the RNG can potentially get in a stuck state -- but good enough for debugging?
		u32* data = (u32*)dst;
		const size_t numQuadWords = numBytes / 16;
		for( size_t i = 0; i != numQuadWords; ++i, data += 4 )
		{
			data[0] = rand++;
			data[1] = rand++;
			data[2] = rand++;
			data[3] = rand++;
		}

		const size_t numWords = (numBytes-(numQuadWords*16)) / 4;
		for( size_t i = 0; i != numWords; ++i, ++data )
		{
			*data = rand++;
		}
	}
}
#endif // YC_ALLOC_RAND_FILL

#if defined YC_ALLOC_NAN_FILL
namespace
{
	void NanFill( void * dst, const size_t numBytes )
	{
		u32 nan = 0x7fc12345; // feel free to replace this bit pattern with something else
		u32* data = (u32*)dst;
		const size_t numWords = numBytes / 4;
		for( size_t i = 0; i != numWords; ++i, ++data )
		{
			*data = nan++;
		}
	}
}
#endif // YC_ALLOC_NAN_FILL

#if defined YC_ANDROID || defined YC_NIX || defined YC_APPLE
	#define YC_SYS_ALLOC_USE_MALLOC_IMPL
	#include <stdlib.h>
#endif
#ifndef YC_SYS_ALLOC_USE_MALLOC_IMPL
	#if defined YC_MSVC
		#include <malloc.h>
		struct Header
		{
			void* base;
			ycSize_t size;
		};
	#elif YC_NDA
	#elif YC_NDA
	#elif ( defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L ) || ( defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600 ) || defined YC_CLANG
		#include <stdlib.h>
	#elif defined _ISOC11_SOURCE
		#include <stdlib.h>
	#elif defined __linux__
		#include <malloc.h>
	#else
		#error ycSysAllocator is not implemented on the current platform, or the current platform cannot be detected
	#endif
#endif

#ifdef YC_NDA
#endif 

void* ycSysAlloc( ycSize_t bytes, ycSize_t align, const char* /*debugName*/, const char* /*file*/, const uint32_t /*line*/, const ureg flags )
{
	void* ptr = nullptr;
	if( align < YC_MEM_DEFAULT_ALIGN ) { align = YC_MEM_DEFAULT_ALIGN; }
	#if defined YC_SYS_ALLOC_USE_MALLOC_IMPL
		YC_UNUSED( flags );
		void* base = malloc( bytes + align + sizeof(uintptr_t)*2 );
		if( base != nullptr )
		{
			const uintptr_t headerEnd = uintptr_t(base) + align + sizeof(uintptr_t)*2; // can waste a lot of space to add to alignment!
			ptr = (void*)( headerEnd - (headerEnd%align) );
			*((uintptr_t*)ptr-1) = uintptr_t( base );
			*((uintptr_t*)ptr-2) = uintptr_t( bytes );
		}
	#elif defined YC_MSVC
		YC_UNUSED( flags );

		ycSize_t headerSize = sizeof(Header);
		ycSize_t footerSize = 0;
		#ifdef YC_ALLOC_PADDING_CHECK
			headerSize += YC_ALLOC_PADDING_CHECK_SIZE;
			footerSize += YC_ALLOC_PADDING_CHECK_SIZE;
		#endif // YC_ALLOC_PADDING_CHECK
		headerSize = ycRoundUpPow2( headerSize, align );

		const ycSize_t allocSize = headerSize + bytes + footerSize;
		#ifdef YC_ALLOC_PADDING_GUARD
			if(YC_UNLIKELY( s_pageSize == 0 ))
			{
				SYSTEM_INFO info;
				GetSystemInfo( &info );
				s_pageSize = info.dwPageSize;
				s_allocGranularity = info.dwAllocationGranularity;
			}
			const ycSize_t sizedToPage = ycRoundUpPow2( allocSize, s_pageSize );
			void* sysBasePtr = VirtualAlloc( 0, ycRoundUpPow2( sizedToPage + s_pageSize*2, s_allocGranularity ), MEM_RESERVE, PAGE_NOACCESS );
			void* rwBase = VirtualAlloc( (u8*)sysBasePtr + s_pageSize, sizedToPage, MEM_COMMIT, PAGE_READWRITE );
			ycAssert( rwBase ); YC_UNUSED( rwBase );
			//DWORD prevProtect = 0;
			//VirtualProtect( sysBasePtr, s_pageSize, PAGE_NOACCESS, &prevProtect );
			//VirtualProtect( (void*)( uintptr_t(sysBasePtr)+s_pageSize+sizedToPage ), s_pageSize, PAGE_NOACCESS, &prevProtect );
			void* sysPtr = (void*)( uintptr_t(sysBasePtr) + s_pageSize );
			if( s_vmemGuardAllocEnd )
			{
				// place the allocation toward the end of the page, so the guard is tighter around the end rather than the start
				uintptr_t tmp = uintptr_t(sysBasePtr)+s_pageSize+sizedToPage - allocSize + headerSize;
				uintptr_t rounded = ycRoundUpPow2( tmp, align );
				if( rounded != tmp ) { rounded -= align; } // round down
				ptr = (u8*)rounded;
			}
			else
			{
				ptr = (u8*)sysPtr + headerSize;
			}
			s_vmemGuardAllocEnd = !s_vmemGuardAllocEnd;
		#else
			void* sysPtr = _aligned_malloc( allocSize, align );
			ptr = (u8*)sysPtr + headerSize;
		#endif

		Header* header = ( Header* )( (u8*)ptr - sizeof(Header) );
		header->base = sysPtr;
		header->size = allocSize;

		#ifdef YC_ALLOC_PADDING_CHECK
			Sentinel_Fill( (u8*)ptr - sizeof(Header) - YC_ALLOC_PADDING_CHECK_SIZE ); // this is 'before the header' not 'at the base ptr', if things were padded out for alignment these are different!
			Sentinel_Fill( (u8*)sysPtr + allocSize - YC_ALLOC_PADDING_CHECK_SIZE ); 
		#endif // YC_ALLOC_PADDING_CHECK

		#ifdef YC_ALLOC_RAND_FILL
			RandFill( ptr, bytes );
		#endif
		#ifdef YC_ALLOC_NAN_FILL
			NanFill( ptr, bytes );
		#endif
	#elif YC_NDA
	#elif YC_NDA
	#elif ( defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L ) || ( defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600 ) || defined YC_CLANG
		YC_UNUSED( flags );
		const int ret = posix_memalign( &ptr, align, bytes );
		ycAssert( ret == 0 );
	#elif defined _ISOC11_SOURCE
		YC_UNUSED( flags );
		ptr = aligned_alloc( align, bytes );
	#elif defined __linux__
		YC_UNUSED( flags );
		ptr = memalign( align, bytes );
	#endif
	return ptr;
}

void ycSysFree( void* ptr )
{
	if( ptr == nullptr ) { return; }
	#if defined YC_SYS_ALLOC_USE_MALLOC_IMPL
		free( (void*)(*((uintptr_t*)ptr-1)) );
	#elif defined YC_MSVC
		Header* header = ( Header* )( (u8*)ptr - sizeof(Header) );
		void* allocBase = header->base;

		#ifdef YC_ALLOC_PADDING_CHECK
			Sentinel_Check( (u8*)header - YC_ALLOC_PADDING_CHECK_SIZE );
			Sentinel_Check( (u8*)header->base + header->size - YC_ALLOC_PADDING_CHECK_SIZE ); 
		#endif // YC_ALLOC_PADDING_CHECK

		#ifdef YC_ALLOC_RAND_FILL
			RandFill( allocBase, header->size );
		#endif // YC_ALLOC_RAND_FILL

		#ifdef YC_ALLOC_PADDING_GUARD
			VirtualFree( (void*)( uintptr_t(allocBase)-s_pageSize ), 0, MEM_RELEASE );
		#else
			_aligned_free( allocBase );
		#endif

	#elif YC_NDA
	#elif YC_NDA
	#elif ( defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L ) || ( defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600 ) || defined YC_CLANG
		free( ptr );
	#elif defined _ISOC11_SOURCE
		free( ptr );
	#elif defined __linux__
		free( ptr );
	#endif
}

ycSize_t ycSysMemSize( void* ptr, ycSize_t align = YC_MEM_DEFAULT_ALIGN )
{
#ifdef YC_ALLOC_PADDING_CHECK
	YC_UNUSED( align );
	ycSize_t size = 0;
	u8* buf = ( u8* )ptr;
	for( ycSize_t s = 0; s < ( sizeof( ycSize_t ) ); s++ ) { size <<= 8; size += *--buf; }
	return size;
#else
	#if defined YC_SYS_ALLOC_USE_MALLOC_IMPL
		YC_UNUSED( align );
		return *( (uintptr_t*)(*((uintptr_t*)ptr-2)) );
	#elif defined YC_MSVC
		if( align < YC_MEM_DEFAULT_ALIGN ) { align = YC_MEM_DEFAULT_ALIGN; }
		return _aligned_msize( ptr, align, 0 );
	#elif YC_NDA
	#elif YC_NDA
	#elif ( defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L ) || ( defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600 ) || defined YC_CLANG
		YC_UNUSED( align );
		return malloc_usable_size( ptr );
	#elif defined _ISOC11_SOURCE
		YC_UNUSED( align );
		return malloc_usable_size( ptr );
	#elif defined __linux__
		YC_UNUSED( align );
		return malloc_usable_size( ptr );
	#else
		return 0;
	#endif
#endif
}

// Construct it ourselves using a static buffer.
// This is to prevent the destructor triggering upon exit, which
// causes problems when the lifespan of our users outlasts
// our own lifespan (order of destruction is not guaranteed!)
namespace
{
	alignas(ycSysAllocator) char s_defaultSysAllocMem[ sizeof(ycSysAllocator) ];
	static ycSysAllocator* s_defaultSysAllocPtr = nullptr;
}
/*static*/ ycSysAllocator* ycSysAllocator::GetDefault()
{
	if( !s_defaultSysAllocPtr ) 
	{
		s_defaultSysAllocPtr = new( s_defaultSysAllocMem )ycSysAllocator( "Default System Allocator" );
	}
	return s_defaultSysAllocPtr;
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycSysAllocator )
{
	ycAssert( bytes != 0 );

	void* ptr = ycSysAlloc( bytes, align, YC_DEBUG_STRING(debugName.m_str), file, line, flags );
	ycAssertMsg( (flags & kAllocFlag_NoAssert) || ptr, "alloc size %" PRIu64 ", align %" PRIu64, u64(bytes), u64(align) );

	if( ( flags & kAllocFlag_NoTrack ) == 0 )
	{
		ycMemTrack::TrackAlloc( this, ptr, bytes, debugName, file, line );
	}

	return ptr;
}

YC_ALLOCATOR_DECLARATION_FREE( ycSysAllocator )
{
	if( (flags&kAllocFlag_NoTrack) == 0 )
	{
		ycMemTrack::TrackFree( this, ptr );
	}

	ycSysFree( ptr );
}

YC_ALLOCATOR_DECLARATION_SIZE( ycSysAllocator )
{
	return ycSysMemSize( ptr, align );
}
