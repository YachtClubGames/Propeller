#pragma once

/*! \class ycCmdList

*/

#include "ycCommon.h"
#include "ycAllocator.h"

class ycCmdList
{
public:

	ycCmdList(); //!< cannot be used until Init() is called
	void Init( ycAllocator* allocator, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE );

	ycCmdList( ycAllocator* allocator, const ycSize_t pageSize = YC_MEM_DEFAULT_PAGE_SIZE );
	~ycCmdList(); //!< warning! non-virtual dtor!

	void Finalize(); //!< Call this after the commmand list has been built, before being Executed; adds 'end' command and frees unused memory
	
	void Reset( const bool freeMemory = false ); //!< If a command list is to be re-used you should probably leave its memory allocated

	virtual void Execute() = 0;

	void Trim(); //!< Free unused memory

	//void Jump( ycCmdList* cmdList ); //!< execute another command list; does not implicitely return! you probably want a->jump(b); b->jump(a); to come back

	//! insert a jump command which will be filled out later
	void* InsertJump();
	static void SetJump( void* pJump, u8* dst );

	//! mainly for use with setting jumps
	u8* GetStart() const;
	u8* GetCurPos() const;

protected:

	//! Returns a pointer to write the next command at
	uint8_t* GetDst( const ycSize_t size );
	template< class t_cmdType > YC_INLINE t_cmdType* GetDst() { return (t_cmdType*)GetDst( sizeof(t_cmdType) ); }

	ycAllocator* m_mem;
	uint8_t*     m_firstPage;
	uint8_t*     m_curPage; //!< current page may be the first page
	uint8_t*     m_curPos;  //!< position in the current page
	ycSize_t     m_cmdBytesLeft; //!< number of usable command bytes left in the current page
	ycSize_t     m_pageSize;
	ycSize_t     m_usablePageSize;

	enum
	{
		kCommand_Jump = 0,
		kCommand_End,
		kCommand_Start //!< marks the start index of command ids on derived classes
	};

	struct Command
	{
		YC_INLINE Command( const uint32_t _type ) : type( _type ), flags( 0 ) {}
		uint32_t type;
		uint32_t flags; // currently unused.... but keep this 8 byte aligned
		template< class t_type > t_type* As() { return static_cast< t_type* >( this ); }
	};

	struct Command_Jump : Command
	{
		YC_INLINE Command_Jump( uint8_t* _dst ) : Command( kCommand_Jump ), dst( _dst ) {}
		uint8_t* dst;
	};

	uint8_t* ExecuteCommand( Command* cmd );

	uint8_t* GetPageNext( uint8_t* page );
	void SetPageNext( uint8_t* page, uint8_t* next );
};
