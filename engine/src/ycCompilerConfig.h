#pragma once
#ifndef YC_COMPILERCONFIG_H
#define YC_COMPILERCONFIG_H

//////////
// Compiler Configuration
//////////

// platform
#if YC_NDA
#elif YC_NDA
#elif YC_NDA
#elif YC_NDA
#elif YC_NDA
#elif YC_NDA
#elif defined _WIN32
	#define YC_MSFT
	#define YC_WIN32 // TODO HACK FIXME: clarify this define.. is this 'windows desktop' or 'win32 api' ?
#elif defined ANDROID || defined __ANDROID__
	#define YC_ANDROID
#elif defined __APPLE__
	#define YC_APPLE
	#include "TargetConditionals.h"
	#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
		#error I have no idea if this works under the simulator  
	#elif TARGET_OS_IPHONE || TARGET_OS_IOS
		#define YC_IOS
		#define YC_OSX
	#elif TARGET_OS_TV
		#define YC_TVOS
		#define YC_OSX
	#else
		#define YC_MAC
		#define YC_OSX
	#endif
#elif !defined YC_NIX
	#define YC_NIX // (linux/unix/posix)-ish flavors
#endif

// architecture
#if defined __arm__ || defined __arm || defined _M_ARM || defined __ARM_ARCH
	#define YC_ARM
	#if defined __thumb__ || defined __thumb || defined _M_ARMT
		#define YC_THUMB
	#endif
	#if ( defined __LP64__ && __LP64__ ) || ( defined __ARM_64BIT_STATE && __ARM_64BIT_STATE )
		#define YC_64
	#else
		#define YC_32
	#endif
	// does anyone actually use arm big endian?
#else
	#define YC_X86
	#if defined __x86_64 || defined __x86_64__ || defined _M_X64
		#define YC_64
	#else
		#define YC_32
	#endif
#endif
#ifndef YC_BIG_ENDIAN
	#define YC_LITTLE_ENDIAN
#endif

// compiler
#if defined _MSC_VER
	#define YC_MSVC
	// msvc-clang!
	#if defined __clang__
		#define YC_CLANG
	#endif
#elif defined __clang__
	#define YC_CLANG
#elif defined __GNUC__ // clang can define this, must come after __clang__ check!
	#define YC_GCC
#endif

// portable types
#if defined _MSC_VER
	// avoid nasty include chain on MSFT
	typedef signed char        int8_t;
	typedef short              int16_t;
	typedef int                int32_t;
	typedef long long          int64_t;
	typedef unsigned char      uint8_t;
	typedef unsigned short     uint16_t;
	typedef unsigned int       uint32_t;
	typedef unsigned long long uint64_t;
	#ifdef _WIN64
		typedef unsigned __int64 size_t;
		typedef __int64          ptrdiff_t;
		typedef __int64          intptr_t;
		typedef unsigned __int64 uintptr_t;
	#else
		typedef unsigned int     size_t;
		typedef int              ptrdiff_t;
		typedef int              intptr_t;
		typedef unsigned int     uintptr_t;
	#endif
#else
	#include <stdint.h>
	#include <stddef.h> // size_t
#endif

typedef uint64_t u64;
typedef int64_t  s64;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint8_t  u8;
typedef int8_t   s8;
typedef float    f32;
typedef double   f64;

// used for memory management and file sizes
// size_t is not guaranteed to be large enough for file sizes, but i'm not aware of any relevant platforms it actually isn't
#ifdef YC_64
	typedef uint64_t ycSize_t;
#else
	typedef uint32_t ycSize_t;
#endif
typedef uint64_t ycFileSize_t; //!< file sizes are always 64 bit

// used in cases where we want a native word sized variable; makes intent more clear than using uintptr_t, etc
#ifdef YC_64
	typedef uint64_t ureg;
	typedef int64_t  sreg;
#else
	typedef uint32_t ureg;
	typedef int32_t  sreg;
#endif

#ifdef YC_64
	#define YC_PTR_SIZE 8
#else
	#define YC_PTR_SIZE 4
#endif

/*! A hint used by some data structures to improve performance.
* This is a little weird on some ppc variants: hw lines are technically 64, but most interactions are in 'half lines' of 32
* Setting this higher than necessary (as long as it's a multiple) is fine, and possibly beneficial.
*/
#define YC_CACHE_LINE_BYTES 64

// compiler hinting
//#if defined YC_MSVC
//	#define YC_ASSUME( x ) do { __assume( !!(x) ); __analysis_assume( !!(x) ); } while(0)
//#elif defined YC_CLANG
//	#define YC_ASSUME( x ) do { __builtin_assume( !!(x) ); } while(0)
//#elif defined YC_GCC
//	#define YC_ASSUME( x ) do { if(!(x)) { __builtin_unreachable(); } } while(0)
//#else
	#define YC_ASSUME( x ) do { } while(0)
//#endif

#if defined YC_GCC || defined YC_CLANG
	#define YC_UNLIKELY( x ) ( __builtin_expect(!!(x), 0) )
#else
	#define YC_UNLIKELY( x ) (x)
#endif

#if defined YC_MSVC || defined YC_GCC
	#define YC_RESTRICT __restrict
#elif defined YC_CLANG
	#define YC_RESTRICT __restrict__
#elif __STDC_VERSION__ >= 19901L
	#define YC_RESTRICT restrict
#else
	#define YC_RESTRICT
#endif

#define YC_RESTRICT_REF YC_RESTRICT

// inline
/* sometimes you just want 'inline' for syntactical reasons (define something
   in a header), but this is meant to be 'force inline' macro, as much as we
   can hint/force it on any given compiler
*/
#if defined YC_MSVC
	#define YC_INLINE __forceinline // despite the name this is still only a strongly worded request
#elif defined YC_GCC || defined YC_CLANG
	#define YC_INLINE __attribute__((always_inline))
#else
	#define YC_INLINE inline
#endif

#if defined YC_CLANG
	#define YC_NO_INLINE __attribute__ ((noinline)) 
#else
	#define YC_NO_INLINE 
#endif

#if defined YC_CLANG || defined YC_MSVC
	// the nullptr macro trick relies on undefined behavior, to keep ubsan from yelling we must use the builtin
	#define YC_OFFSETOF( STRUCT, FIELD ) __builtin_offsetof( STRUCT, FIELD )
#else
	#define YC_OFFSETOF( STRUCT, FIELD ) ((size_t)( &((STRUCT *)nullptr)->FIELD ))
#endif

// compatibility
#define YC_PRAGMA( x ) __pragma( x )

#if defined YC_GCC || defined YC_CLANG
	#define YC_ALIGN( x ) __attribute__ ((aligned ( x )))
#elif defined YC_MSVC
	#define YC_ALIGN( x ) __declspec( align( x ) )
#endif

#if defined __cplusplus
	#define YC_BEGIN_EXTERN_C extern "C" {
	#define YC_END_EXTERN_C   }
#else
	#define YC_BEGIN_EXTERN_C
	#define YC_END_EXTERN_C
#endif

#if defined _MSVC_LANG // https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-170
	#define YC_CPP_VER _MSVC_LANG
#else
	#define YC_CPP_VER __cplusplus
#endif

// warnings

#if defined YC_MSVC
	#define YC_UNUSED( x ) ( (void)(x) )
#else
	#define YC_UNUSED( x ) ( (void)(sizeof((x), 0)) ) // use this variant if possible, the above can potentially have side effects (particularly if the variable is volatile)
#endif

#if YC_CPP_VER >= 201703L
	#define YC_FALLTHROUGH [[ fallthrough ]]
#else
	#define YC_FALLTHROUGH // fallthrough
#endif

#ifdef YC_MSVC
	#define YC_UNINIT_SAFESTART( x )	      \
		__pragma( warning( push ) )           \
		__pragma( warning( disable : 4701 ) ) /* potentially uninitialized local variable used */

	#define YC_UNINIT_SAFEEND( x ) __pragma( warning( pop ) )

	// When working on things, we often want to disable a lot of the more annoying warnings
	// to help iterate faster. Shove YC_HACKABLE at the top of a file to make MSVC shut up.
	#define YC_HACKABLE							\
		__pragma( warning( disable : 4100 ) ) /* unused parameter */	\
		__pragma( warning( disable : 4702 ) ) /* unreachable code */	\
		__pragma( warning( disable : 4189 ) ) /* variable initialized but never used */ \
		__pragma( warning( disable : 4505 ) ) /* unused function */ \
		__pragma( warning( disable : 4060 ) ) /* switch statement contains no 'case' or 'default' labels */
#else
	#define YC_UNINIT_SAFESTART( x )
	#define YC_UNINIT_SAFEEND( x )

	#define YC_HACKABLE
#endif

#if defined YC_MSVC
	#pragma warning( disable: 4127 ) // conditional expression is constant
	#pragma warning( disable: 4201 ) // nonstandard extension used : nameless struct/union
#endif

// optimization

#if defined YC_DEBUG && !defined YC_DEBUG_OPT /* debug opt should already have good things set */
	// whether #include ycPush/PopDebugOpt does anything
	#define YC_ENABLE_DEBUG_OPT
#endif

#endif // !YC_COMPILERCONFIG_H
