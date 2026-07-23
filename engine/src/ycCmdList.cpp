#include "ycCmdList.h"

ycCmdList::ycCmdList()
{
	m_mem            = nullptr ;
	m_firstPage      = nullptr;
	m_curPage        = nullptr;
	m_curPos         = nullptr;
	m_cmdBytesLeft   = 0;
	m_pageSize       = 0;
	m_usablePageSize = 0;
}

void ycCmdList::Init( ycAllocator* allocator, const ycSize_t pageSize /*= YC_MEM_DEFAULT_PAGE_SIZE*/ )
{
	ycAssert( allocator );
	m_mem            = allocator ;
	m_firstPage      = nullptr;
	m_curPage        = nullptr;
	m_curPos         = nullptr;
	m_cmdBytesLeft   = 0;
	m_pageSize       = pageSize;
	m_usablePageSize = pageSize - sizeof(void*) - sizeof(Command_Jump);
}

ycCmdList::ycCmdList( ycAllocator* allocator, const ycSize_t pageSize /*= YC_MEM_DEFAULT_PAGE_SIZE*/ )
{
	ycAssert( allocator );
	m_mem            = allocator ;
	m_firstPage      = nullptr;
	m_curPage        = nullptr;
	m_curPos         = nullptr;
	m_cmdBytesLeft   = 0;
	m_pageSize       = pageSize;
	m_usablePageSize = pageSize - sizeof(void*) - sizeof(Command_Jump);
}

ycCmdList::~ycCmdList()
{
	if( m_firstPage )
	{
		uint8_t* cur = m_firstPage;
		while( cur )
		{
			uint8_t* next = GetPageNext( cur );
			m_mem->Free( cur );
			cur = next;
		}
	}
}

uint8_t* ycCmdList::GetDst( const ycSize_t size )
{
	ycAssert( size <= m_usablePageSize ); //!< can't currently have a single data blob span multiple allocs

	// if the current page has enough space left, use it
	if( m_cmdBytesLeft >= size )
	{
		uint8_t* pos = m_curPos;
		m_curPos       += size;
		m_cmdBytesLeft -= size;
		return pos;
	}

	// othewrise, make a new page (or use an existing unused page)
	else
	{
		uint8_t* newPage;
		
		if( m_curPage && GetPageNext( m_curPage ) )
		{
			newPage = GetPageNext( m_curPage ); // TODO HACK FIXME: when would/should this happen?
		}
		else
		{
			newPage = (uint8_t*)YC_ALLOC( m_mem, m_pageSize, YC_CACHE_LINE_BYTES, "Render Cmds" );

			// null out next page ptr on new page
			SetPageNext( newPage, nullptr );
		}

		if( m_curPage )
		{
			// insert jump command in current page to the new page
			new( m_curPos )Command_Jump( newPage );

			// also add a ptr to next page in a predictable spot (so we can iterage pages without parsing all of the commands)
			SetPageNext( m_curPage, newPage );
		}
		else
		{
			m_firstPage = newPage;
		}
		m_curPage      = newPage;
		m_curPos       = newPage + size;
		m_cmdBytesLeft = m_usablePageSize - size;
		return newPage;
	}
}

void ycCmdList::Finalize()
{
	new( GetDst<Command>() )Command( kCommand_End );
	Trim();
}

uint8_t* ycCmdList::ExecuteCommand( Command* cmd )
{
	switch( cmd->type )
	{
	case kCommand_Jump:
		return (uint8_t*)( static_cast< Command_Jump* >( cmd )->dst );
	case kCommand_End:
		return nullptr;
	}
	ycFatal();
	return nullptr;
}

void ycCmdList::Reset( const bool freeMemory /*= false*/ )
{
	if( freeMemory == false )
	{
		// TODO HACK FIXME: if debug, zero out pages? (except the next ptrs!!!)
		m_curPage      = m_firstPage;
		m_curPos       = m_firstPage;
		m_cmdBytesLeft = m_firstPage ? m_usablePageSize : 0 ;
	}
	else
	{
		uint8_t* cur = m_firstPage;
		while( cur )
		{
			uint8_t* next = GetPageNext( cur );
			YC_FREE( m_mem, cur );
			cur = next;
		}
		m_firstPage      = nullptr;
		m_curPage        = nullptr;
		m_curPos         = nullptr;
		m_cmdBytesLeft   = 0;
	}
}

void ycCmdList::Trim()
{
	if( m_curPage == nullptr )
	{
		ycAssert( m_firstPage == nullptr );
		ycAssert( m_curPos    == nullptr );
		return;
	}

	uint8_t* cur = GetPageNext( m_curPage );
	while( cur )
	{
		uint8_t* next = GetPageNext( cur );
		m_mem->Free( cur );
		cur = next;
	}

	SetPageNext( m_curPage, nullptr );
}

uint8_t* ycCmdList::GetPageNext( uint8_t* page )
{
	uint8_t** pNextPage = reinterpret_cast< uint8_t** >( page+m_pageSize-sizeof(void*) );
	return *pNextPage;
}

void ycCmdList::SetPageNext( uint8_t* page, uint8_t* next )
{
	void** pNextPage = reinterpret_cast< void** >( page+m_pageSize-sizeof(void*) );
	*pNextPage = next;
}

//void ycCmdList::Jump( ycCmdList* cmdList )
//{
//	new( GetDst<Command_Jump>() )Command_Jump( cmdList->m_firstPage );
//}

void* ycCmdList::InsertJump()
{
	return new( GetDst<Command_Jump>() )Command_Jump( nullptr );
}

/*static*/ void ycCmdList::SetJump( void* pJump, u8* dst )
{
	Command_Jump* jump = (Command_Jump*)pJump;
	jump->dst = dst;
}

u8* ycCmdList::GetStart() const
{
	return m_firstPage;
}

u8* ycCmdList::GetCurPos() const
{
	return m_curPos;
}
