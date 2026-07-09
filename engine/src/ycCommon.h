#pragma once
#ifndef YC_COMMON_H
#define YC_COMMON_H

//////////
// Compiler Configuration
//////////

#include "ycCompilerConfig.h"
#include "ycEngineConfig.h"

//////////
// Engine Configuration
//////////

#ifdef YC_DEBUG
	#ifndef YC_NO_MEM_DEBUG
		#define YC_ENABLE_MEM_DEBUG // track detailed information about memory usage and try to catch errors
	#endif
#endif

// default alignment for allocators
// smaller could save some bytes here and there, but this is large enough for any builtin's alignment, and allows for simd
#define YC_MEM_DEFAULT_ALIGN 16

// many allocators will default to a common page size to reduce fragmentation
#define YC_MEM_DEFAULT_PAGE_SIZE (32*1024)

// enable running tests
#if defined YC_DEBUG && !defined YC_NO_TESTS
	#define YC_TEST
#endif

// run additional tests that may take a while
//#define YC_ENABLE_SLOW_TESTS

//////////
// Utilities
//////////

#define YC_TRUE 1
#define YC_FALSE 0

//////////
// Debug
//////////
// definitions for external functions are in ycDebug.cpp

#if defined YC_ENABLE_LOG
YC_BEGIN_EXTERN_C
	void ycLog( const char* fmt, ... );
YC_END_EXTERN_C
#else
	#define ycLog( ... )
#endif

// some funkiness attempting to keep YC_DEBUG_BREAK call stacks at the actual point of failure instead of inside of a handler function
#if defined YC_ENABLE_ASSERT
#if defined YC_MSVC
	YC_BEGIN_EXTERN_C
		int _ycIsDebuggerPresent(); // int so that it's c-compatible
		void _ycDebugAbort();
	YC_END_EXTERN_C
	#define YC_DEBUG_BREAK do { if( _ycIsDebuggerPresent() ) { __debugbreak(); } else { _ycDebugAbort(); } } while(0)
#elif defined YC_OSX
	// __builtin_debugtrap allows you to continue past in a debugger, BRK 0 doesnt
	#define YC_DEBUG_BREAK __builtin_debugtrap()
	//#ifdef __aarch64__
	//	#define YC_DEBUG_BREAK asm volatile("BRK 0")
	//#else
	//	#define YC_DEBUG_BREAK asm volatile("int3")
	//#endif
#elif defined __has_builtin && __has_builtin(__builtin_debugtrap)
	#define YC_DEBUG_BREAK __builtin_debugtrap()
#elif defined YC_CLANG || defined YC_GCC
	#if defined YC_ARM && defined YC_THUMB
		#define YC_DEBUG_BREAK __asm__ volatile(".inst 0xde01")
	#elif defined YC_ARM
		#define YC_DEBUG_BREAK __asm__ volatile(".inst 0xe7f001f0")
	#else
		#define YC_DEBUG_BREAK __asm__ volatile("int $0x03")
	#endif
#else
	#error you probably want a proper YC_DEBUG_BREAK on this platform
	#define YC_DEBUG_BREAK do {} while( 1 )
#endif
#else
#define YC_DEBUG_BREAK 
#endif

#ifdef YC_MSVC
	#ifndef __analysis_assume // avoid including sal.h
		#ifdef _PREFAST_
			#define __analysis_assume( expr ) __assume( expr )
		#else
			#define __analysis_assume( expr )
		#endif
	#endif
#endif

#if defined YC_ENABLE_DEBUG_STRINGS
	#define YC_FILE_LINE __FILE__, __LINE__
	#define YC_DEBUG_STRING( x ) x
#else
	#define YC_FILE_LINE nullptr, 0
	#define YC_DEBUG_STRING( x ) nullptr
#endif

#if defined YC_ENABLE_ASSERT
	#ifdef __clang_analyzer__
		#define ycFatal() __builtin_unreachable()
	#else
		#define ycFatal() YC_DEBUG_BREAK
	#endif
	#ifdef YC_MSVC
		#define ycAssert( x )         do { if(YC_UNLIKELY(!(x))) { _ycDebugAssertMsg( #x, __FILE__, __LINE__, nullptr     ); YC_DEBUG_BREAK; } __analysis_assume(x); } while(0)
		#define ycAssertMsg( x, ... ) do { if(YC_UNLIKELY(!(x))) { _ycDebugAssertMsg( #x, __FILE__, __LINE__, __VA_ARGS__ ); YC_DEBUG_BREAK; } __analysis_assume(x); } while(0)
	#elif defined __clang_analyzer__
		#define ycAssert( x )         __builtin_assume( x )
		#define ycAssertMsg( x, ... ) __builtin_assume( x )
	#else
		#define ycAssert( x )         do { if(YC_UNLIKELY(!(x))) { _ycDebugAssertMsg( #x, __FILE__, __LINE__, nullptr     ); YC_DEBUG_BREAK; } } while(0)
		#define ycAssertMsg( x, ... ) do { if(YC_UNLIKELY(!(x))) { _ycDebugAssertMsg( #x, __FILE__, __LINE__, __VA_ARGS__ ); YC_DEBUG_BREAK; } } while(0)
	#endif
	YC_BEGIN_EXTERN_C
		void _ycDebugAssertMsg( const char* cond, const char* fileName, const u32 line, const char* fmt, ... );
	YC_END_EXTERN_C
#else
	#define ycFatal()             YC_ASSUME( 0 )
	#define ycAssert( x )         do{ (void)sizeof(x); } while(0)
	#define ycAssertMsg( x, ... ) do{ (void)sizeof(x); } while(0)
#endif
#define ycReleaseAssertMsg( x, ... ) do { if(YC_UNLIKELY(!(x))) { _ycReleaseAssertMsg( __VA_ARGS__ ); } } while(0) // this embeds the abort() inside the assert call, unlike the debug assert which puts the YC_DEBUG_BREAK in the macro
YC_BEGIN_EXTERN_C
	void _ycReleaseAssertMsg( const char* fmt, ... );
	// exists, but i hate pulling vcruntime.h into this, extern it if you need it
	//void _ycReleaseAssertMsgV( const char* fmt, va_list args );
YC_END_EXTERN_C

// should we try YC_TEST as an 'enable even more asserts' ?
//#if defined YC_TEST
//	#define ycTestAssert()            ycAssert( x )
//	#define ycTestAssertMsg( x, ... ) ycAssertMsg( x, __VA_ARGS__ )
//#else
//	#define ycTestAssert()
//	#define ycTestAssertMsg( x, ... )
//#endif

#if defined YC_ENABLE_WARN
	#define ycWarn( ... ) ycLog( __VA_ARGS__ )
	#define ycWarnOnce( ... ) static bool warned_##__LINE__ = false; if( !warned_##__LINE__ ) { warned_##__LINE__ = true; ycLog( __VA_ARGS__ ); ycLog( "\n" ); }
#else
	#define ycWarn( ... )
	#define ycWarnOnce( ... )
#endif

//////////
// Precompiled Hash
//////////

/*
To use the PHASH macros, only fill in the first argument, eg:
PHASH32( "foo" )

Our code preprocessor will calculate and fill in the second argument for you:
PHASH32( "foo", 0xBA365A68 )

If the text is changed, the hex value will automatically be updated.
*/

//!YC_CODEGEN IGNORE don't confuse ycCodeGen when it sees the PHASH macros
#define PHASH32( string, hash ) ( hash )
#define PHASH64( string, hash ) ( hash )
//!YC_CODEGEN END_IGNORE

// Struct visibility labels for ycCodeGen reflection.
#define yceditable public


#endif // !YC_COMMON_H
