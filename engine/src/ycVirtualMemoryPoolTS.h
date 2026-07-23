#pragma once

#include "ycAllocator.h"
#include "ycMutex.h"

class ycVirtualMemoryPoolTS : public ycAllocatorTS
{
public:

	static constexpr ycSize_t kDefaultCommitIncrement = 64 * 1024;
	static constexpr ycSize_t kDefaultMaxSize = 16 * 1024*1024; // low default so large amounts must be explicit

	ycVirtualMemoryPoolTS( const char* debugName );
	ycVirtualMemoryPoolTS( const char* debugName, const ycSize_t blockSize, const ycSize_t commitIncrement = kDefaultCommitIncrement, const ycSize_t maxBytes = kDefaultMaxSize );
	void Init( const ycSize_t blockSize, const ycSize_t commitIncrement = kDefaultCommitIncrement, const ycSize_t maxBytes = kDefaultMaxSize );
	virtual ~ycVirtualMemoryPoolTS();
	void Clear(); // not a thread-safe call

	YC_ALLOCATOR_DECLARATIONS

protected:

	ycSize_t m_blockSize = 0;
	ycSize_t m_commitIncrement = 0;
	u8* m_base = nullptr;
	ycSize_t m_committed = 0;
	ycSize_t m_maxSize = 0;
	ycMutex m_growGuard;

	union TaggedIndex
	{
		struct
		{
			u32 tag;
			u32 idx;
		};
		u64 asU64;
	};
	static constexpr TaggedIndex kInvalidTaggedIndex = { { 0, u32(-1) } };

	// some gross shenanigans to avoid leaking the header :(
	// TODO: fix up ycAtomic (note: C++20 adds the C atomics to C++! this'll be less gross to fix up!)
	//       P0943R6 Supporting C Atomics In C++	VS 2022 17.1 ( https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170 )
	u64 m_freeHead = kInvalidTaggedIndex.asU64;
	u32 m_tag = kInvalidTaggedIndex.tag;

	void FreeList( u8* head, u8* tail ); // a pre-linked list
	void Expand();
	void Link( const ycSize_t idx, const ycSize_t count );
	u32 IndexOf( u8* block );

#ifdef YC_NDA
#endif
};
