#pragma once

#include "ycCommon.h"
#include "ycPlatformConfig.h"

/* Memory Debugging Switches
These are not supported by all allocators; they may be defined externally
(eg Propeller.props, YC_ENV_DEFINES, etc)
*/

/* Pad allocations with known values
Checking these values when the allocation is freed can help find out-of-bounds
writes, but not reads.
*/
//#define YC_ALLOC_PADDING_CHECK

/* Pad OS-protected guard pages around allocations
This can help catch out of bounds reads and writes, but can take substantial
memory overhead. Due to the high alignment requirements of pages, the guard
pages are not always tightly packed around each allocation.
*/
#if defined YC_DEBUG && defined YC_GOTHIC && !defined YC_EDITOR && !defined YC_ALLOC_PADDING_CHECK && !defined YC_ALLOC_PADDING_GUARD
	#define YC_ALLOC_PADDING_GUARD
#endif

/* Fill allocations with f32 NaNs
This can help find uninitialized variables, especially issues with random NaNs.
*/
//#define YC_ALLOC_NAN_FILL

/* Fill allocations with random data
This can also help find uninitialized variables, possibly tripping bugs more readily.
*/
//#define YC_ALLOC_RAND_FILL

/* Allocation description
You generally just pass plain strings to Alloc(), but you can use this to
indicate that a string should be copied by ycMemTrack.
*/
struct ycAllocInfo
{
public:
#ifdef YC_ENABLE_DEBUG_STRINGS
	const char* m_str;
	bool m_temp;
	inline ycAllocInfo( const char* info ) : m_str( info ), m_temp( false ) {}
	inline ycAllocInfo( const char* info, bool temp ) : m_str( info ), m_temp( temp ) {}
#else
	inline ycAllocInfo( const char* ) {}
	inline ycAllocInfo( const char*, bool ) {}
#endif
};

// Global default allocator
extern class ycAllocatorTS* g_defaultMem;

/*! \class ycAllocator
Defines a base interface for allocators. Methods are not abstract, the base
class implements them, but they all assert. Not all derived allocators
require all methods.
*/
class ycAllocator
{
public:

	//! Some flags are allocator type specific (ycSysAllocator in particular may support platform specific allocation options)
	enum
	{
		kAllocFlag_None     = 0,
		kAllocFlag_NoAssert = 1 << 0, //!< do not assert if the allocation fails, just return nullptr
		kAllocFlag_NoTrack  = 1 << 1, //!< do not track this allocation
		// derived classes should begin their flags at 1<<16
	};

	// macro to shove all the relevant virtuals into sub classes
	#define _YC_ALLOCATOR_DECLARATIONS(Override) \
		virtual void* Alloc( const ycSize_t bytes, const ycSize_t align = YC_MEM_DEFAULT_ALIGN, ycAllocInfo debugName = "<UNKNOWN>", const char* file = "<UNKNOWN>", const uint32_t line = 0, const ureg flags = kAllocFlag_None ) Override; \
		virtual void Free( void* ptr, const ureg flags = kAllocFlag_None ) Override; \
		virtual ycSize_t Size( void* ptr, ycSize_t align = YC_MEM_DEFAULT_ALIGN ) Override; // returns 0 if unsupported for allocator

	#define YC_ALLOCATOR_DECLARATIONS _YC_ALLOCATOR_DECLARATIONS(override)

	#define NOTHING
	_YC_ALLOCATOR_DECLARATIONS(NOTHING)
	#undef NOTHING

	virtual ~ycAllocator(); //!< enforce virtual dtor on derived classes

#ifdef YC_ENABLE_DEBUG_STRINGS
	inline ycAllocator( const char* allocatorName ) : m_name( allocatorName ) {}
	const char* m_name;
	inline const char* GetDebugName() const { return m_name; }
#else
	inline ycAllocator( const char* ) {}
	inline const char* GetDebugName() const { return nullptr; }
#endif

#ifdef YC_ENABLE_MEM_DEBUG
	virtual bool IsDumpSummarized() const { return false; } // return true to make ycMemTrack::Output show a short summary instead of listing every alloc
#endif

	virtual bool IsStorageEmbedded() { return false; } // used for StackVector to determine if a ptr is safe to take ownership of
	virtual bool IsPtrEmbedded( void* ) { return false; } // used for StackVector to determine if a ptr is safe to take ownership of
	virtual void SetFallbackAllocator( ycAllocator* ) { ycAssert( 0 ); } // used to set fallback allocator for StackVector

	ycAllocator( const ycAllocator& ) = delete;
	ycAllocator( ycAllocator&& ) = delete;
	ycAllocator& operator=( const ycAllocator& ) = delete;
	ycAllocator& operator=( ycAllocator&& ) = delete;

protected:

	// macros to define the overridden virtuals in sub classes

	#define YC_ALLOCATOR_DECLARATION_ALLOC( subclass ) \
		void* subclass::Alloc( const ycSize_t bytes, const ycSize_t align, ycAllocInfo debugName, const char* file, const uint32_t line, const ureg flags )

	#define YC_ALLOCATOR_DECLARATION_FREE( subclass ) \
		void subclass::Free( void* ptr, const ureg flags )

	#define YC_ALLOCATOR_DECLARATION_SIZE( subclass ) \
		ycSize_t subclass::Size( void* ptr, ycSize_t align )
};

//! Exists only to act as a base class for thread-safe allocators; this allows code to type-check when requiring thread safe allocators
class ycAllocatorTS : public ycAllocator
{
public:
	ycAllocatorTS( const char *debugName ) : ycAllocator( debugName ) {}
};

//! ycAllocator new and delete
/*
Really don't like overloading global new... but couldn't come up with a better solution :(

Important Note: We do not have array new/delete!
Their implementations are compiler specific and messing with them can be problematic.
Maybe a newer C++ std improves the situation?
*/

namespace std
{
#if YC_CPP_VER >= 201703L
	enum class align_val_t : size_t;
#else
	typedef size_t align_val_t;
#endif
}

void* operator new( size_t size, ycAllocator* mem, const char* debugName, const char* file, uint32_t line );
void* operator new( size_t size, std::align_val_t align, ycAllocator* mem, const char* debugName, const char* file, uint32_t line );
void* operator new( size_t size, ycAllocator& mem, const char* debugName, const char* file, uint32_t line );
void* operator new( size_t size, std::align_val_t align, ycAllocator& mem, const char* debugName, const char* file, uint32_t line );

#define ycnew( ... ) new( __VA_ARGS__, YC_FILE_LINE )

template< class t_type >
inline
void ycdelete( class ycAllocator* mem, t_type* ptr )
{
	ptr->~t_type();
	mem->Free( ptr );
}

template<> inline void ycdelete( class ycAllocator* /*mem*/, void* /*ptr*/ ) { ycFatal(); }

template< class t_type >
inline
void ycdelete( class ycAllocator& mem, t_type* ptr )
{
	ptr->~t_type();
	mem.Free( ptr );
}

template<> inline void ycdelete( class ycAllocator& /*mem*/, void* /*ptr*/ ) { ycFatal(); }

#define YC_ALLOC( allocator, size, align, debugName ) (allocator)->Alloc( size, align, YC_DEBUG_STRING(debugName), YC_FILE_LINE ) //!< most of the time you probably want alignof(T) or YC_MEM_DEFAULT_ALIGN for alignment
#define YC_ALLOCT( allocator, type, debugName ) ( (type*)((allocator)->Alloc( sizeof(type), alignof(type), YC_DEBUG_STRING(debugName), YC_FILE_LINE )) )
#define YC_ALLOCARRAY( allocator, type, count, debugName ) ( (type*)((allocator)->Alloc( sizeof(type)*(count), alignof(type), YC_DEBUG_STRING(debugName), YC_FILE_LINE )) )
#define YC_FREE( allocator, ptr ) (allocator)->Free( ptr ) // TODO HACK FIXME: this should probably (void*) cast for you, so it doesnt whine about const ptrs

//#if defined YC_CLANG && !defined YC_OSX
//	#define YC_NEW_EXCEPT noexcept
//#else
	#define YC_NEW_EXCEPT
//#endif

//! Catch use of default new/delete

#ifdef YC_ENABLE_ASSERT
	#if !defined YC_NIX /* linux .so loading uses new */ && !defined YC_OSX /* osx app setup uses STL */ && !defined YC_RGB_RAZER
		#define YC_CATCH_UNSUPPORTED_NEW // trigger asserts when default new/delete are used
	#endif
#endif
#ifdef YC_CATCH_UNSUPPORTED_NEW
	void* operator new( size_t ) YC_NEW_EXCEPT;
	void* operator new( size_t, uint32_t ) YC_NEW_EXCEPT;
	void operator delete( void* ) YC_NEW_EXCEPT;

	void* operator new[]( size_t ) YC_NEW_EXCEPT;
	void* operator new[]( size_t, uint32_t ) YC_NEW_EXCEPT;
	void operator delete[]( void* ) YC_NEW_EXCEPT;

	void* operator new[]( size_t, ycAllocator*, const char*, const char*, uint32_t ) YC_NEW_EXCEPT;
	void* operator new[]( size_t, size_t, ycAllocator*, const char*, const char*, uint32_t ) YC_NEW_EXCEPT;

	void* operator new[]( size_t, ycAllocator&, const char*, const char*, uint32_t ) YC_NEW_EXCEPT;
	void* operator new[]( size_t, size_t, ycAllocator&, const char*, const char*, uint32_t ) YC_NEW_EXCEPT;
#endif

//! Placement new and delete... if you are using <new>, remove these!

#if defined __clang__ && __clang_major__ >= 17
	/*
	broke upgrading sdk 15->17
	i'm not sure how to work around this
	- perfectly matching the <new> definitions still errors about redefinition
	- define-ing _LIBCPP_NEW to prevent <new> misses other things; we could try to represent those other things ourselves
	  (define noexcept, std::launder)
	it doesn't matter too much, we mainly do this on msvc to avoid the include chain, other std libs aren't as bad
	*/
	#include <new>
#else
	#if !defined YC_DONT_DEFINE_PLACEMENT_NEW && !defined _NEW_  && !defined _NEW && !defined _NEW_CXXVERS_V1_ // try to guard against these defines if <new> was included before us
		inline void* operator new( size_t, void* ptr ) YC_NEW_EXCEPT { return ptr; }
		inline void* operator new[]( size_t, void* ptr ) YC_NEW_EXCEPT { return ptr; }
		inline void  operator delete( void*, void* ) YC_NEW_EXCEPT { /* nop */ }
		inline void  operator delete[]( void*, void* ) YC_NEW_EXCEPT { /* nop */ }
		#ifdef _MSC_VER // try to prevent msvc from defining placement new if it's included after us
			#define __PLACEMENT_NEW_INLINE
			#define __PLACEMENT_VEC_NEW_INLINE
		#endif
	#endif
#endif
