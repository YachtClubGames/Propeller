#pragma once

#include "ycCommon.h"
#include "ycAllocator.h"

class ycPagedLinearAllocator : public ycAllocator
{
public:

	void Init( const char* debugName, ycAllocator* mem = nullptr, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE, const ycSize_t pageAlign = YC_MEM_DEFAULT_ALIGN, const bool allowOversize = false );
	ycPagedLinearAllocator( const char* debugName, ycAllocator* mem = nullptr, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE, const ycSize_t pageAlign = YC_MEM_DEFAULT_ALIGN, bool allowOversize = false );
	void Clear();
	~ycPagedLinearAllocator();

	YC_ALLOCATOR_DECLARATIONS

	u8* Peek( ycSize_t* pAvailBytes, const ycSize_t minimumRequired = 0 );
	void Advance( const ycSize_t size );

	u8* Save();
	void Restore( u8* ptr );

protected:

	u8*          m_curPos; // within m_tail
	u8*          m_head;
	u8*          m_tail;
	ycSize_t     m_pageSize;
	ycSize_t     m_pageAlign;
	ycAllocator* m_mem;
	#ifdef YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif
	bool m_allowOversize;

	// these occur at the *end* of each page
	struct PageDesc
	{
		u8* next;
	};

	struct PageHeader
	{
		ycSize_t size;
	};

	inline void SetPageNextPtr( u8* page, u8* next );
	inline u8* GetPageNextPtr( u8* page );
	inline void SetPageSize(u8* page, ycSize_t size);
	inline ycSize_t GetPageSize(u8* page);
};
