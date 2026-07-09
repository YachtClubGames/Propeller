#include "ycTLSFHeap.h"

#include "ycBitUtils.h"
#include "ycMath.h"
#include "ycMemTrack.h"
#include "ycSysAllocator.h"
#include "ycVirtualMem.h"

#define TLFS_LOG( ... ) //ycLog( __VA_ARGS__ )

ycTLSFHeap::ycTLSFHeap( const char* debugName )
	: ycAllocator( debugName )
{
	m_flMaxIndex = 32;
	m_slBitmap = nullptr;
	m_free = nullptr;
	m_regionHead = nullptr;
	m_externalHead = nullptr;
	#ifdef YC_TLSFHEAP_DEBUG
		m_blockCount = 0;
		m_totalBytes = 0;
		m_externalCount = 0;
	#endif // YC_TLSFHEAP_DEBUG
	#ifdef YC_NDA
	#endif 
}

void ycTLSFHeap::Init( ycAllocator* parent, const ycSize_t maxAllocSize, const ycSize_t backingAllocSize )
{
	ycAssert( parent && maxAllocSize && backingAllocSize && backingAllocSize > maxAllocSize );
	m_parent = parent;
	m_maxAllocSize = maxAllocSize;
	m_backingAllocSize = backingAllocSize;

	ycAssert( backingAllocSize > sizeof(Block)*3 );
	const ycSize_t maxFreeSize = backingAllocSize - sizeof(Block)*3; // head, tail, and the free block header
	ycAssert( maxFreeSize >= maxAllocSize );
	const ureg flMaxIndex = 64 - ycBitUtils::CountLeadingZeros64( ycRoundUpPow2( maxFreeSize ) );
	m_flMaxIndex = u8(flMaxIndex);
	ycAssert( kSlIndexCountLog2 + kGranularityLog2 <= 255 );
	m_flIndexShift   = kSlIndexCountLog2 + kGranularityLog2;
	ycAssert( m_flMaxIndex >= m_flIndexShift );
	m_flIndexCount   = m_flMaxIndex - m_flIndexShift + 1;
	m_smallBlockSize = 1 << m_flIndexShift;

	InitBitmaps();
}

void ycTLSFHeap::Init( const ycSize_t _virtualReserve, const ycSize_t _commitChunk, const ycSize_t maxAllocSize )
{
	m_parent = nullptr;
	m_maxAllocSize = maxAllocSize;

	const ycSize_t commitChunk = ycVirtualMem::RoundToPageSize( _commitChunk );
	const ycSize_t virtualReserve = ycRoundUpPow2( _virtualReserve, commitChunk );
	m_commitChunk = commitChunk;

	const ycSize_t maxFreeSize = virtualReserve - sizeof(Block)*3; // head, tail, and the free block header
	ycAssert( maxFreeSize >= maxAllocSize );
	const ureg flMaxIndex = 64 - ycBitUtils::CountLeadingZeros64( ycRoundUpPow2( maxFreeSize ) );
	m_flMaxIndex = u8(flMaxIndex);
	ycAssert( kSlIndexCountLog2 + kGranularityLog2 <= 255 );
	m_flIndexShift   = kSlIndexCountLog2 + kGranularityLog2;
	ycAssert( m_flMaxIndex >= m_flIndexShift );
	m_flIndexCount   = m_flMaxIndex - m_flIndexShift + 1;
	m_smallBlockSize = 1 << m_flIndexShift;

	InitBitmaps();

	const ycSize_t initialCommit = ycRoundUpPow2( sizeof(Block)*2 + kMinBlockSize, commitChunk );
	ycAssert( initialCommit <= virtualReserve );

	u8* base = ( u8* )ycVirtualMem::Reserve( virtualReserve );
	#ifdef YC_NDA
	#else
		ycVirtualMem::Commit( base, initialCommit );
	#endif

	AddRegion( base, virtualReserve, initialCommit );
}

void ycTLSFHeap::InitBitmaps()
{
	ycAllocator* mem = m_parent;
	if( mem == nullptr ) { mem = ycSysAllocator::GetDefault(); }

	m_flBitmap = 0;
	m_slBitmap = (ureg*)YC_ALLOC( mem, sizeof(ureg)*ycSize_t(m_flIndexCount), alignof(ureg), GetDebugName() );
	for( u8 fl = 0; fl != m_flIndexCount; ++fl )
	{
		m_slBitmap[ fl ] = 0;
	}
	const ureg freeListCount = ureg(m_flIndexCount) * kSlIndexCount;
	m_free = (Block**)YC_ALLOC( mem, sizeof(Block*)*freeListCount, alignof(Block*), GetDebugName() );
	for( ureg i = 0; i != freeListCount; ++i )
	{
		m_free[ i ] = nullptr;
	}
}

/*virtual*/ ycTLSFHeap::~ycTLSFHeap()
{
	ycAllocator* mem = m_parent;
	if( mem == nullptr ) { mem = ycSysAllocator::GetDefault(); }
	if( m_slBitmap ) { YC_FREE( mem, m_slBitmap ); }
	if( m_free ) { YC_FREE( mem, m_free ); }

	if( m_parent )
	{
		Block* region = m_regionHead;
		while( region )
		{
			Block* nextRegion = region->region.next;
			YC_FREE( mem, region );
			region = nextRegion;
		}
	}
	else
	{
		ycAssert( m_regionHead && m_regionHead->region.next == nullptr ); // exactly one region
		Block* region = m_regionHead;
		ycVirtualMem::Release( region, region->region.size );
	}

	Block* ext = m_externalHead;
	while( ext )
	{
		Block* nextExt = ext->external.next;
		YC_FREE( mem, ext );
		ext = nextExt;
	}

	#ifdef YC_NDA
	#endif 
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycTLSFHeap )
{
	YC_UNUSED( flags );
	YC_UNUSED( line );
	YC_UNUSED( file );
	YC_UNUSED( debugName );

	void* ret = nullptr;

	ycSize_t size = ycRoundUpPow2( bytes, kGranularity );

	Check();
	if( align <= kGranularity )
	{
		if(YC_UNLIKELY( size > m_maxAllocSize ))
		{
			return ExternalAlloc( bytes, align, debugName, file, line, flags );
		}
		Block* block = FindFree( size );
		if( block == nullptr )
		{
			return ExternalAlloc( bytes, align, debugName, file, line, flags );
		}
		PrepareBlock( block, size );
		ycAssert( BlockSize(block) >= bytes );
		ret = BlockGetPtr( block );
	}
	else
	{
		/*
		this requires extra space both for the alignment, but also because the
		offset we make for the alignment must be returned to the free pool --
		it must be large enough to be a free block

		there are other ways we could do it, eg use a single bit in the 'size'
		field to indicate that it's actually an offset to the real block start
		*/
		ycAssert( (align%kGranularity) == 0 );
		const ycSize_t paddedSize = size + align + kMinBlockSize;
		ycAssert( (paddedSize%kGranularity) == 0 );
		if(YC_UNLIKELY( paddedSize > m_maxAllocSize ))
		{
			return ExternalAlloc( bytes, align, debugName, file, line, flags );
		}
		Block* block = FindFree( paddedSize );
		ret = PrepareBlockAligned( &block, size, align );
		ycAssert( BlockFromPtr(ret) == block );
		ycAssert( !BlockIsFree(block) );
		ycAssert( BlockSize(block) >= bytes );
		ycAssert( (uintptr_t(ret)%align) == 0 );
	}
	Check();

	ycAssert( ( uintptr_t(ret) % kGranularity ) == 0 );
	ycMemTrack::TrackAlloc( this, ret, bytes, debugName, file, line );
	return ret;
}

void* ycTLSFHeap::ExternalAlloc( const ycSize_t bytes, const ycSize_t align, ycAllocInfo debugName, const char* file, const uint32_t line, const ureg flags )
{
	const ureg pad = ycMax< ureg >( sizeof(Block), align ); // could be smaller, only a couple of the fields will be accessed
	ycAllocator* mem = m_parent;
	if( mem == nullptr ) { mem = ycSysAllocator::GetDefault(); }
	const ycSize_t size = bytes + pad;

	u8* alloc = (u8*)mem->Alloc( size, align, debugName, file, line, flags ); // shouldnt this use min alignment, or should it just always request +align?

	const uintptr_t allocStart = uintptr_t(alloc);
	uintptr_t aligned = ycRoundUpPow2( allocStart + sizeof(Block), align );
	if( aligned - allocStart < sizeof(Block) ) { aligned += align; }
	const uintptr_t allocEnd = allocStart + size;
	ycAssert( aligned + bytes <= allocEnd );

	Block* block = (Block*)( aligned - sizeof(Block) );
	#ifdef YC_DEBUG // make it less gross to look at in a debugger
		block->prevPhys = nullptr;
		block->nextFree = nullptr;
		block->prevFree = nullptr;
	#endif // YC_DEBUG

	block->external.next = m_externalHead;
	block->external.prev = nullptr;

	if( m_externalHead )
	{
		m_externalHead->external.prev = block;
	}
	m_externalHead = block;

	block->flags = kFlag_External;
	block->size = allocEnd - allocStart; // doesnt include the block header
	block->external.base = alloc;

	#ifdef YC_TLSFHEAP_DEBUG
		++m_externalCount;
	#endif // YC_TLSFHEAP_DEBUG

	return (void*)aligned;
}

YC_ALLOCATOR_DECLARATION_FREE( ycTLSFHeap )
{
	if( ptr == nullptr ) { return; }

	Block* block = BlockFromPtr( ptr );
	if(YC_UNLIKELY( block->flags & kFlag_External ))
	{
		ycAllocator* mem = m_parent;
		if( mem == nullptr ) { mem = ycSysAllocator::GetDefault(); }
		if( m_externalHead == block )
		{
			ycAssert( block->external.prev == nullptr );
			m_externalHead = block->external.next;
		}
		else
		{
			ycAssert( block->external.prev );
			block->external.prev->external.next = block->external.next;
		}
		if( block->external.next )
		{
			block->external.next->external.prev = block->external.prev;
		}
		mem->Free( block->external.base, flags );
		#ifdef YC_TLSFHEAP_DEBUG
			ycAssert( m_externalCount );
			--m_externalCount;
		#endif // YC_TLSFHEAP_DEBUG
		return;
	}
	Check();
	ycAssert( !BlockIsFree(block) );
	ycMemTrack::TrackFree( this, ptr );

	/*
	when merging, it's possible that removal is unnecessary, the merged free block
	may wind up in the same fl/sl as the old one, but it's probably not likely enough
	to be worth making this code more complex

	maybe merging a very tiny allocation into a very large free area isnt that uncommon?
	(would be nice to get some numbers!)
	*/

	Block* prev = block->prevPhys;
	if( BlockIsFree(prev) )
	{
TLFS_LOG( "merge left\n" );
		RemoveFreeBlock( prev ); // we'll add it back later, after the size is finalized
		prev->size += BlockSize(block) + sizeof(Block);

		Block* next = BlockNextPhys(prev);
		next->prevPhys = prev;

		block = prev;

		#ifdef YC_TLSFHEAP_DEBUG
			ycAssert( m_blockCount );
			--m_blockCount;
		#endif // YC_TLFSGPUHEAP_DEBUG
	}
	Block* next = BlockNextPhys( block );
	if( BlockIsFree( next ) )
	{
TLFS_LOG( "merge right\n" );
		ycAssert( BlockSize(next) );

		RemoveFreeBlock( next );
		block->size += BlockSize(next) + sizeof(Block);

		next = BlockNextPhys( block ); // grab the _new_ next
		next->prevPhys = block;

		#ifdef YC_TLSFHEAP_DEBUG
			ycAssert( m_blockCount );
			--m_blockCount;
		#endif // YC_TLFSGPUHEAP_DEBUG
	}
	else
	{
		//ycAssert( !BlockIsPrevFree(next) );
		//next->size |= kFlag_PrevBlockFree;
		next->prevPhys = block; // TODO HACK FIXME: this is set on every path, move it
	}

	//ycAssert( !BlockIsFree(block) ); // merged blocks are already free
	BlockSetFree( block );

	AddToFreeList( block );

	Check();
}

YC_ALLOCATOR_DECLARATION_SIZE( ycTLSFHeap )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	ycAssert( false );
	return 0;
}

/*static*/ ureg ycTLSFHeap::GetBitIndex( const ureg x )
{
	return 64 - ycBitUtils::CountLeadingZeros64( x ) - 1;
}

ureg ycTLSFHeap::CalcFl( const ycSize_t size )
{
	if( size < ycSize_t(m_smallBlockSize) )
	{
		return 0;
	}
	else
	{
		return GetBitIndex( size ); // TODO HACK FIXME: doesnt this need to offset?
	}
}

void ycTLSFHeap::CalcFlSl( const ycSize_t size, ureg* pFl, ureg* pSl )
{
	ureg fl, sl;
	// can you do this without the branch by masking out the bottom bits before clz?
	if( size < ycSize_t(m_smallBlockSize) )
	{
		fl = 0;
		sl = size / ( ycSize_t(m_smallBlockSize) / kSlIndexCount ); // cant this just be a right shift?
	}
	else
	{
		fl = GetBitIndex( size );
		sl = size >> ( fl - kSlIndexCountLog2 ); // shift the bits we care about to the bottom
		sl ^= 1ull << kSlIndexCountLog2; // and mask off the top bit (the one we used to figure out fl)
		fl -= ycSize_t(m_flIndexShift)-1;
	}
	*pFl = fl;
	*pSl = sl;
}

ycTLSFHeap::Block* ycTLSFHeap::FindFree( ycSize_t size )
{
	ycAssert( size );

	ureg fl, sl;
	Block* block = nullptr;
	if( size >= ycSize_t(m_smallBlockSize) ) // adjust size cuz we want the bin that contains blocks >= our size, not just includes our size (and potentially less)
	{
		size += ( 1ull << ( CalcFl(size) - kSlIndexCountLog2 ) ) - 1;
	}
	CalcFlSl( size, &fl, &sl );

	// TODO HACK FIXME: assert? external alloc?
	if( fl >= ureg(m_flIndexCount) ) { return nullptr; } // adjusted size may go out of bounds; could check kMaxAllocSize earlier

	ureg slMap = m_slBitmap[ fl ] & ( ureg(-1) << sl ); // check sl bins equal to or larger than what we need
	if( !slMap )
	{
		// search up the first level list
		ureg flMap = m_flBitmap & ( ureg(-1) << (fl+1) );
		if( !flMap )
		{
			if( m_parent )
			{
				u8* newRegion = (u8*)YC_ALLOC( m_parent, m_backingAllocSize, kGranularity, GetDebugName() );
				if( newRegion )
				{
					AddRegion( newRegion, m_backingAllocSize, m_backingAllocSize );
				}
				else
				{
					return nullptr;
				}
			}
			else
			{
				if( !ExpandRegion( size ) )
				{
					return nullptr;
				}
			}
			flMap = m_flBitmap & ( ureg(-1) << (fl/*+1*/) );
			ycAssert( flMap );
		}
		fl = GetBitIndex( flMap );
		slMap = m_slBitmap[ fl ];
	}
	ycAssert( slMap );
	sl = GetBitIndex( slMap );

	block = RemoveFreeBlock( fl, sl );

	return block;
}

void ycTLSFHeap::PrepareBlock( Block* block, const ycSize_t size )
{
	ycAssert( block && !BlockIsFree(block) && size && BlockSize(block) >= size );

	// split the block, returning the excess at the end back to the free lists
	const ycSize_t remain = BlockSize(block) - size;
	if( remain >= kMinBlockSize )
	{
		SplitBack( block, remain );
	}
}

void* ycTLSFHeap::PrepareBlockAligned( Block** ppBlock, const ycSize_t size, const ycSize_t align )
{
//ycLog( "PrepareBlockAligned\n" );
	Block* block = *ppBlock;
	const ycSize_t blockSize = BlockSize(block);
	ycAssert( block && !BlockIsFree(block) && size && blockSize >= size );

	const uintptr_t blockStart = uintptr_t(BlockGetPtr(block));
	const uintptr_t blockEnd = blockStart + blockSize;
	uintptr_t aligned = ycRoundUpPow2( blockStart, align );
	uintptr_t remainFront = aligned - blockStart;
	if( remainFront )
	{
		if( remainFront < kMinBlockSize )
		{
			aligned += ycMax( align, kMinBlockSize - remainFront );
			aligned = ycRoundUpPow2( aligned, align );
			ycAssert( (aligned%align) == 0 );
			remainFront = aligned - blockStart;
			ycAssert( remainFront >= kMinBlockSize );
		}
	}
	//ycAssert( (aligned%align) == 0 );
	const uintptr_t alignedEnd = aligned + size;
	ycAssert( blockEnd >= alignedEnd );
	const uintptr_t remainBack = blockEnd - alignedEnd;

//ycLog( "%u bytes split %u | %u | %u [%u]\n", u32(blockSize), u32(remainFront), u32(size), u32(remainBack), u32(remainFront+size+remainBack) );

	// split a block off the front
	if( remainFront /*>= uintptr_t(m_minBlockSize)*/ )
	{
		ycAssert( remainFront >= sizeof(Block::size) );
		block = SplitFront( block, remainFront - sizeof(Block) /* leave a gap for our new header */ );
		ycAssert( uintptr_t(BlockGetPtr(block)) == aligned );
		int a = 1; YC_UNUSED( a );
	}

	Check();

	// split a block off the end
	if( remainBack >= kMinBlockSize )
	{
		SplitBack( block, remainBack );
	}

	*ppBlock = block;

	return (void*)aligned;
}

ycTLSFHeap::Block* ycTLSFHeap::SplitFront( Block* block, const ycSize_t size )
{
	ycAssert( (size%kGranularity) == 0 );
	ycAssert( BlockSize(block) > size );
	ycAssert( size >= kGranularity );
	ycAssert( !BlockIsFree(block) );

	const ycSize_t backSize = BlockSize( block ) - size;
	ycAssert( (backSize%kGranularity) == 0 );
	ycAssert( backSize > kMinBlockSize );
	block->size = size;

	BlockSetFree( block );
	AddToFreeList( block );

	Block* newBlock = BlockNextPhys( block );
	newBlock->prevPhys = block;
	newBlock->size = backSize - sizeof(Block);
	newBlock->flags = 0;

	Block* next = BlockNextPhys( newBlock );
	next->prevPhys = newBlock;

#ifdef YC_TLSFHEAP_DEBUG
	++m_blockCount;
#endif // YC_TLSFHEAP_DEBUG

	return newBlock;
}

void ycTLSFHeap::SplitBack( Block* block, const ycSize_t size )
{
	ycAssert( (size%kGranularity) == 0 );
	ycAssert( BlockSize(block) > size );
	ycAssert( size >= kMinBlockSize );
	ycAssert( !BlockIsFree(block) );

	block->size -= size;

	Block* newBlock = BlockNextPhys( block );
	newBlock->prevPhys = block; // this is only valid if prev is free, which it's about not to be
	newBlock->size     = size - sizeof(Block);
	newBlock->flags    = kFlag_BlockFree;
	AddToFreeList( newBlock );

	Block* next = BlockNextPhys( newBlock );
	next->prevPhys = newBlock;

#ifdef YC_TLSFHEAP_DEBUG
	++m_blockCount;
#endif // YC_TLSFHEAP_DEBUG
}

void ycTLSFHeap::AddRegion( u8* base, const ycSize_t size, const ycSize_t commit )
{
	//ycLog( "AddRegion %p %u\n", base, u32(size) );
	Check();

	ycAssert( ( uintptr_t(base) % kGranularity ) == 0 );
	ycAssert( ( size % kGranularity ) == 0 );
	ycAssert( ( commit % kGranularity ) == 0 );

	// head/tail blocks, and an allocation of minimum size
	ycAssert( commit >= sizeof(Block)*2 + kMinBlockSize );

	const ycSize_t usable = commit - sizeof(Block)*2;

	Block* head = ( Block* )base;
	head->prevPhys = nullptr;
	head->nextFree = nullptr;
	head->prevFree = nullptr;
	head->region.next = m_regionHead;
	head->region.size = size;
	head->region.commit = commit;
	head->flags = kFlag_Region;
	head->size = 0;

	m_regionHead = head;

	Block* free = BlockNextPhys( head );
	free->prevPhys = head;
	free->nextFree = nullptr;
	free->prevFree = nullptr;
	free->region.next = nullptr;
	free->region.size = 0;
	free->region.commit = 0;
	free->flags = kFlag_BlockFree;
	free->size = usable - sizeof(Block);

	AddToFreeList( free );

	Block* tail = BlockNextPhys( free );
	tail->prevPhys = free;
	tail->nextFree = nullptr;
	tail->prevFree = nullptr;
	tail->region.next = nullptr;
	tail->region.size = 0;
	tail->region.commit = 0;
	tail->flags = 0;
	tail->size = 0;

#ifdef YC_TLSFHEAP_DEBUG
	m_totalBytes += commit;
	m_blockCount += 3; // incl sentinels
#endif // YC_TLSFHEAP_DEBUG

	Check();
}

bool ycTLSFHeap::ExpandRegion( const ycSize_t allocSize )
{
	Block* region = m_regionHead;
	Block* tail = GetRegionTail( region );
	Block* prev = tail->prevPhys;
	ycSize_t need = allocSize;
	const bool prevIsFree = BlockIsFree( prev );
	if( prevIsFree )
	{
		ycAssert( need > prev->size ); // shouldnt have exhausted the heap?
		need -= prev->size;
	}
	else
	{
		need += sizeof(Block); // the free space is gonna have to have a free header prepended
	}
	const ycSize_t commit = ycRoundUpPow2( need, m_commitChunk );

	if( region->region.commit + commit > region->region.size )
	{
		return false;
	}

	// commit
	#ifdef YC_NDA
	#else
		ycVirtualMem::Commit( (u8*)region + region->region.commit, commit );
	#endif
	region->region.commit += commit;

	// move tail
	Block* newTail = GetRegionTail( region );
	newTail->prevPhys = prevIsFree ? prev : tail ;
	newTail->nextFree = nullptr;
	newTail->prevFree = nullptr;
	newTail->region.next = nullptr;
	newTail->region.size = 0;
	newTail->region.commit = 0;
	newTail->flags = 0;
	newTail->size = 0;

	// adjust prev (or insert a new free block)
	if( BlockIsFree( prev ) )
	{
		RemoveFreeBlock( prev );
		prev->size += commit;
	}
	else
	{
		prev = BlockNextPhys( prev );
		ycAssert( prev == tail );
		prev->prevPhys = prev;
		prev->nextFree = nullptr;
		prev->prevFree = nullptr;
		prev->region.next = nullptr;
		prev->region.size = 0;
		prev->region.commit = 0;
		prev->flags = kFlag_BlockFree;
		prev->size = commit - sizeof(Block);

		#ifdef YC_TLSFHEAP_DEBUG
			++m_totalBytes;
		#endif // YC_TLSFHEAP_DEBUG
	}
	AddToFreeList( prev );
	#ifdef YC_TLSFHEAP_DEBUG
		m_totalBytes += commit;
	#endif // YC_TLSFHEAP_DEBUG

	return true;
}

void ycTLSFHeap::AddToFreeList( Block* block )
{
	ycAssert( block );
	ureg fl, sl;
	CalcFlSl( BlockSize(block), &fl, &sl );
	ycAssert( BlockIsFree(block) );
	Block** freeList = GetFreeList( fl, sl );
	Block* oldHead = *freeList;
	block->nextFree = oldHead;
	block->prevFree = nullptr;
	if( oldHead )
	{
		oldHead->prevFree = block;
	}
	*freeList = block;
	m_flBitmap |= 1ull << fl;
	m_slBitmap[ fl ] |= 1ull << sl;
}

ycTLSFHeap::Block* ycTLSFHeap::RemoveFreeBlock( const ureg fl, const ureg sl )
{
	Block** freeList = GetFreeList( fl, sl );
	Block* block = *freeList;
	ycAssert( block );
	ycAssert( BlockIsFree( block ) );
	ycAssert( block->prevFree == nullptr );

	Block* nextFree = block->nextFree;

	*freeList = nextFree;
	if( nextFree )
	{
		nextFree->prevFree = nullptr;
	}
	else
	{
		m_slBitmap[ fl ] ^= 1ull << sl;
		if( m_slBitmap[ fl ] == 0 )
		{
			m_flBitmap ^= 1ull << fl;
		}
	}

	block->flags &= ~kFlag_BlockFree;

	return block;
}

void ycTLSFHeap::RemoveFreeBlock( Block* block )
{
	ureg fl, sl;
	CalcFlSl( BlockSize(block), &fl, &sl );

	Block** freeList = GetFreeList( fl, sl );
	Block* prevFree = block->prevFree;
	Block* nextFree = block->nextFree;
	if( nextFree )
	{
		block->nextFree->prevFree = prevFree;
	}
	if( prevFree )
	{
		block->prevFree->nextFree = nextFree;
	}
	else
	{
		ycAssert( *freeList == block );
		*freeList = nextFree;
		if( nextFree == nullptr )
		{
			m_slBitmap[ fl ] ^= 1ull << sl;
			if( m_slBitmap[ fl ] == 0 )
			{
				m_flBitmap ^= 1ull << fl;
			}
		}
	}
}

ycTLSFHeap::Block** ycTLSFHeap::GetFreeList( const ureg fl, const ureg sl )
{
	return m_free + kSlIndexCount*fl + sl;
}

/*static*/ ycSize_t ycTLSFHeap::BlockSize( Block* block )
{
	return block->size;
}

/*static*/ bool ycTLSFHeap::BlockIsFree( Block* block )
{
	return ( block->flags & kFlag_BlockFree ) != 0;
}

/*static*/ void ycTLSFHeap::BlockSetFree( Block* block )
{
	block->flags |= kFlag_BlockFree;
}

/*static*/ ycTLSFHeap::Block* ycTLSFHeap::BlockNextPhys( Block* block )
{
	return (Block*)( (u8*)( block + 1 ) + block->size );
}

/*static*/ void* ycTLSFHeap::BlockGetPtr( Block* block )
{
	return block + 1;
}

/*static*/ ycTLSFHeap::Block* ycTLSFHeap::BlockFromPtr( void* ptr )
{
	return (Block*)( (u8*)ptr - sizeof(Block) );
}

/*static*/ ycTLSFHeap::Block* ycTLSFHeap::GetRegionTail( Block* region )
{
	ycAssert( region->flags & kFlag_Region );
	return (Block*)( (u8*)region + region->region.commit - sizeof(Block) );
}

#ifdef YC_NDA
#endif 

void ycTLSFHeap::Check()
{
#ifdef YC_TLSFHEAP_DEBUG
	ureg blockCount = 0;
	ureg bytes = 0;
TLFS_LOG( "----------\n" );
	for( Block* region = m_regionHead; region != nullptr; region = region->region.next )
	{
		ycAssert( region->flags == kFlag_Region && region->size == 0 );
		Block* prev = region;
		++blockCount; // region header
		bytes += sizeof(Block);
		Block* block = BlockNextPhys( region );
		for(;;)
		{
TLFS_LOG( "%p  %u  %s\n", BlockGetPtr(block), BlockSize(block), BlockIsFree(block) ? "free" : "used" );
			ycAssert( block->prevPhys == prev );
			ycAssert( BlockSize(block) );
			ycAssert( (BlockSize(block)%kGranularity) == 0 );
			ycAssert( (uintptr_t(BlockGetPtr(block))%kGranularity) == 0 );
			++blockCount;
			ycAssert( blockCount <= m_blockCount );
			bytes += BlockSize(block) + sizeof(Block);
			if( BlockIsFree(block) )
			{
				ycAssert( !BlockIsFree(prev) ); // should have been merged
				// could sanity check free links, but could also do that later
			}
			prev = block;
			block = BlockNextPhys( block );
			if( BlockSize(block) == 0 )
			{
				++blockCount;
				bytes += sizeof(Block);
				ycAssert( block->flags == 0 );
				break;
			}
		}
	}
TLFS_LOG( "----------\n" );
	ycAssert( blockCount == m_blockCount );
	ycAssert( bytes == m_totalBytes );
	const ureg flIndexCount = m_flIndexCount;
	const ureg slIndexCount = kSlIndexCount;
	for( ureg fl = 0; fl != flIndexCount; ++fl )
	{
		for( ureg sl = 0; sl != slIndexCount; ++sl )
		{
			Block** freeList = GetFreeList( fl, sl );
			ycAssert( *freeList == nullptr || (*freeList)->prevFree == nullptr );
			ureg seen = 0;
			ureg seenBytes = 0;
			for( Block* block = *freeList; block != nullptr; block = block->nextFree )
			{
				++seen;
				seenBytes += BlockSize(block) + sizeof(Block::size);
				ycAssert( seen <= m_blockCount ); // cycle detection
				ycAssert( seenBytes <= m_totalBytes );
				ycAssert( BlockIsFree( block ) );
				ureg flCheck, slCheck;
				CalcFlSl( BlockSize(block), &flCheck, &slCheck );
				ycAssert( fl == flCheck && sl == slCheck );
				if( block->prevFree != nullptr )
				{
					ycAssert( block->prevFree->nextFree == block );
				}
				if( block->nextFree != nullptr )
				{
					ycAssert( block->nextFree->prevFree == block );
				}
			}
		}
	}
	ureg externalCount = 0;
	for( Block* ext = m_externalHead; ext; ext = ext->external.next )
	{
		++externalCount;
	}
	ycAssert( externalCount == m_externalCount );
#endif // YC_TLSFHEAP_DEBUG
}

#ifdef YC_TLSFHEAP_DEBUG
#include "ycBitArray.h"
#include "ycRand.h"
/*static*/ void ycTLSFHeap::Test()
{
#if 0 // check binning
	struct MinMax
	{
		u32 min, max;
	};
	MinMax bins[ kFlIndexCount ][ kSlIndexCount ];
	for( ureg fl = 0; fl != kFlIndexCount; ++fl )
	{
		for( ureg sl = 0; sl != kSlIndexCount; ++sl )
		{
			bins[ fl ][ sl ].min = u32(-1);
			bins[ fl ][ sl ].max = u32(0);
		}
	}
	for( ureg size = 0; size != 0xffffffu; ++size )
	{
		ureg fl, sl;
		CalcFlSl( size, &fl, &sl );
		if( size < bins[fl][sl].min ) { bins[fl][sl].min = u32(size); }
		if( size > bins[fl][sl].max ) { bins[fl][sl].max = u32(size); }
	}
	for( ureg _size = 0; _size != 0xffffffu; ++_size )
	{
		ureg size = ycRoundUpPow2( _size, kGranularity );
		const ureg sizeRef = size;
		if( size >= kSmallBlockSize ) // adjust size cuz we want the bin that contains blocks >= our size, not just includes our size (and potentially less)
		{
			size += ( 1ull << ( CalcFl(size) - kSlIndexCountLog2 ) ) - 1;
		}
		ureg fl, sl;
		CalcFlSl( size, &fl, &sl );
		ycAssert( bins[fl][sl].min >= sizeRef );
		ycLog( "%u(%u) -> [%u, %u]\n", u32(_size), u32(sizeRef), u32(bins[fl][sl].min), u32(bins[fl][sl].max) );
	}
	__debugbreak();
#endif

	ycTLSFHeap heap( "ycTLSFHeap::Test" );
	enum
	{
		kMaxAllocs = 1024,
		kAllocSizeMin = 1,
		kAllocSizeMax = 2*1024,//*1024,
		kAllocAlignMin = 1,
		kAllocAlignMax = 1024,//8,

		kBigAllocChance = 100, // 1/kBigAllocChance
		kBigAllocSizeMin = 1024*1024,
		kBigAllocSizeMax = 4*1024*1024,//*1024,

		kSuperIter = 100,
		kSubIter = 1024*1024,
	};
	void* allocs[ kMaxAllocs ];
	ycStaticBitArray< kMaxAllocs > allocsUsed;
	ycRand rng;
	//u32 seed = 418158837;
	u32 seed = rng.Rand();//2439554704;
	ycLog( "seed %u\n", seed );
	rng.Seed( seed );

	//heap.Init( g_defaultMem, 512*1024, 1024*1024 );
	heap.Init( 1024*1024*1024, 256*1024, 1024*1024 );

	//heap.Init( ycRoundUpPow2( ureg(rng.RangeFast( 0, 1024 )), kGranularity ), 1024*1024 );

	for( ureg super = 0; super != kSuperIter; ++super )
	{
		for( ureg sub = 0; sub != kSubIter; ++sub )
		{
//if( super == 0 && sub == 65 )
//{
//	__debugbreak();
//}
			const ureg idx = rng.Rand() % kMaxAllocs;
			if( allocsUsed.Get(idx) )
			{
TLFS_LOG( "free %p\n", allocs[idx] );
				heap.Free( allocs[ idx ] );
				allocsUsed.Clear( idx );
			}
			else
			{
				ycSize_t size = rng.RangeFast( kAllocSizeMin, kAllocSizeMax );
				ycSize_t align = ycRoundUpPow2( rng.RangeFast( kAllocAlignMin, kAllocAlignMax ) );
				if( bool(kBigAllocChance) && rng.RangeFast( 1, u32(kBigAllocChance) ) == 1 )
				{
					size = rng.RangeFast( kBigAllocSizeMin, kBigAllocSizeMax );
				}
				allocs[ idx ] = heap.Alloc( size, align );
				allocsUsed.Set( idx );
TLFS_LOG( "alloc %p [%u bytes]\n", allocs[idx], u32(size) );
			}
		}
		for( ureg i = 0; i != kMaxAllocs; ++i )
		{
			if( allocsUsed.Get(i) )
			{
TLFS_LOG( "free %p\n", allocs[i] );
				heap.Free( allocs[ i ] );
				allocsUsed.Clear( i );
			}
		}
	}
}
#endif // YC_TLSFHEAP_DEBUG

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

ycTLSFHeapTS::ycTLSFHeapTS( const char* debugName )
	: ycAllocatorTS(debugName)
	, m_guard( debugName )
	, m_heap( debugName )
{}

void ycTLSFHeapTS::Init( const ycSize_t virtualReserve, const ycSize_t commitChunk, const ycSize_t maxAllocSize )
{
	m_heap.Init( virtualReserve, commitChunk, maxAllocSize );
}

void ycTLSFHeapTS::Init( ycAllocator* parent, const ycSize_t maxAllocSize, const ycSize_t backingAllocSize )
{
	m_heap.Init( parent, maxAllocSize, backingAllocSize );
}

YC_ALLOCATOR_DECLARATION_ALLOC( ycTLSFHeapTS )
{
	m_guard.Lock();
	void* ret = m_heap.ycTLSFHeap::Alloc( bytes, align, debugName, file, line, flags );
	m_guard.Unlock();
	return ret;
}

YC_ALLOCATOR_DECLARATION_FREE( ycTLSFHeapTS )
{
	m_guard.Lock();
	m_heap.ycTLSFHeap::Free( ptr, flags );
	m_guard.Unlock();
}

YC_ALLOCATOR_DECLARATION_SIZE( ycTLSFHeapTS )
{
	YC_UNUSED( ptr );
	YC_UNUSED( align );
	ycAssert( false );
	return 0;
}