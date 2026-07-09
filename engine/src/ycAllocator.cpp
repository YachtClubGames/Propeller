#include "ycAllocator.h"

#include "ycMemTrack.h"

ycAllocatorTS* g_defaultMem = nullptr;

YC_ALLOCATOR_DECLARATION_ALLOC( ycAllocator )
{
	YC_UNUSED( bytes );
	YC_UNUSED( align );
	YC_UNUSED( debugName );
	YC_UNUSED( file );
	YC_UNUSED( line );
	ycAssert( (flags&kAllocFlag_NoAssert) );
	YC_UNUSED( flags );
	return nullptr;
}

YC_ALLOCATOR_DECLARATION_FREE( ycAllocator )
{
	YC_UNUSED( ptr );
	YC_UNUSED( flags );
	ycFatal();
}

YC_ALLOCATOR_DECLARATION_SIZE( ycAllocator )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	return 0;
}

ycAllocator::~ycAllocator()
{
	ycMemTrack::ClearAllocator( this );
}

void* operator new( size_t size, ycAllocator* mem, const char* debugName, const char* file, uint32_t line )
{
	size_t align = (size_t)YC_MEM_DEFAULT_ALIGN;
	#if YC_CPP_VER >= 201703L
		// the compiler may use the non-aligned new for any values equal to or smaller than
		// __STDCPP_DEFAULT_NEW_ALIGNMENT__, so we need to guarantee that we're giving at least that much
		//if( __STDCPP_DEFAULT_NEW_ALIGNMENT__ > align ) { align = __STDCPP_DEFAULT_NEW_ALIGNMENT__; }
		/*
		We currently have a problem with this. It effectively increases our minimum alignment globally.
		For example if you create a slab allocator for ints, and set the blockAlign to 4, then use
		new() against that allocator, this function will raise the alignment higher than 4, and the
		Alloc will assert.
		I'm not sure what to do about this, we could:
		- globally increase our minimum alignment, and require allocators to max(YC_MIN_ALIGN, requested alignment)
		- allow allocators to exempt themselves from this check, leaving it enabled for general heap style allocators,
		  but disabled for specialized things like slab allocators
		- leave new() with the extra alignment requirements, but allow allocators to still support lower if Alloc()
		  is used
		- do nothing, it hasn't caused any problems yet!
		*/
	#endif
	return mem->Alloc( size, align, debugName, file, line );
}

void* operator new( size_t size, std::align_val_t align, ycAllocator* mem, const char* debugName, const char* file, uint32_t line )
{
	return mem->Alloc( size, static_cast<size_t>( align ), debugName, file, line );
}

void* operator new( size_t size, ycAllocator& mem, const char* debugName, const char* file, uint32_t line )
{
	size_t align = (size_t)YC_MEM_DEFAULT_ALIGN;
	#if YC_CPP_VER >= 201703L
		// the compiler may use the non-aligned new for any values equal to or smaller than
		// __STDCPP_DEFAULT_NEW_ALIGNMENT__, so we need to gaurantee that we're giving at least that much
		//if( __STDCPP_DEFAULT_NEW_ALIGNMENT__ > align ) { align = __STDCPP_DEFAULT_NEW_ALIGNMENT__; }
	#endif
	return mem.Alloc( size, align, debugName, file, line );
}

void* operator new( size_t size, std::align_val_t align, ycAllocator& mem, const char* debugName, const char* file, uint32_t line )
{
	return mem.Alloc( size, static_cast<size_t>( align ), debugName, file, line );
}

#ifdef YC_CATCH_UNSUPPORTED_NEW
	void* operator new( size_t ) YC_NEW_EXCEPT           { ycFatal(); return nullptr; }
	void* operator new( size_t, uint32_t ) YC_NEW_EXCEPT { ycFatal(); return nullptr; }
	void operator delete( void* ) YC_NEW_EXCEPT          { ycFatal(); }

	void* operator new[]( size_t ) YC_NEW_EXCEPT           { ycFatal(); return nullptr; }
	void* operator new[]( size_t, uint32_t ) YC_NEW_EXCEPT { ycFatal(); return nullptr; }
	void operator delete[]( void* ) YC_NEW_EXCEPT          { ycFatal(); }

	void* operator new[]( size_t, ycAllocator*, const char*, const char*, uint32_t ) YC_NEW_EXCEPT         { ycFatal(); return nullptr; }
	void* operator new[]( size_t, size_t, ycAllocator*, const char*, const char*, uint32_t ) YC_NEW_EXCEPT { ycFatal(); return nullptr; }

	void* operator new[]( size_t, ycAllocator&, const char*, const char*, uint32_t ) YC_NEW_EXCEPT         { ycFatal(); return nullptr; }
	void* operator new[]( size_t, size_t, ycAllocator&, const char*, const char*, uint32_t ) YC_NEW_EXCEPT { ycFatal(); return nullptr; }
#endif
