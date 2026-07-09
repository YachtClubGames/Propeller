#include "ycTLSFGpuHeap.h"

#include "ycBitUtils.h"
#include "ycMath.h"

/*static*/ ycTLSFGpuHeap::Block ycTLSFGpuHeap::kNullBlock =
{
	&kNullBlock,
	&kNullBlock,
	0,
	0,
	0,
	&kNullBlock,
	&kNullBlock,
	nullptr
};

ycTLSFGpuHeap::ycTLSFGpuHeap()
	: m_blockMem( "ycTLSFGpuHeap", sizeof(Block) )
{
	m_flBitmap = 0;
	for( ureg fl = 0; fl != kFlIndexCount; ++fl )
	{
		m_slBitmap[ fl ] = 0;
		for( ureg sl = 0; sl != kSlIndexCount; ++sl )
		{
			m_free[ fl ][ sl ] = &kNullBlock;
		}
	}
	m_growFunc = nullptr;

#ifdef YC_TLSFGPUHEAP_DEBUG
	//m_physHead.Init( g_defaultMem, 16 ); // late init, in case g_defaultMem isnt set yet
	m_blockCount = 0;
	m_totalBytes = 0;
#endif // YC_TLSFGPUHEAP_DEBUG
}

void ycTLSFGpuHeap::AddRegion( const ycSize_t initialBase, const ycSize_t initialSize, void* userData )
{
	ycAssert( ( initialSize % kGranularity ) == 0 );
	ycAssert( ( initialBase % kGranularity ) == 0 );

	Block* block = AllocBlock();
	block->prevPhys = &kNullBlock;
	block->nextPhys = &kNullBlock;
	block->offset   = initialBase;
	block->size     = initialSize;
	block->flags    = kFlag_BlockFree;
	block->userData = userData;

	AddToFreeList( block );

#ifdef YC_TLSFGPUHEAP_DEBUG
	if( m_physHead.IsNull() ) { m_physHead.Init( g_defaultMem, 16 ); }
	m_physHead.Append( block );
	m_totalBytes += initialSize;
#endif // YC_TLSFGPUHEAP_DEBUG

	Check();
}

void ycTLSFGpuHeap::Init( GrowFunc growFunc )
{
	m_growFunc = growFunc;

#ifdef YC_TLSFGPUHEAP_DEBUG
	if( m_physHead.IsNull() ) { m_physHead.Init( g_defaultMem, 16 ); }
#endif // YC_TLSFGPUHEAP_DEBUG
}

ycTLSFGpuHeap::~ycTLSFGpuHeap()
{
}

ycTLSFGpuHeap::AllocDesc ycTLSFGpuHeap::Alloc( const ycSize_t _size )
{
	Check();
	const ycSize_t size = ycRoundUpPow2( _size, kGranularity );
	Block* block = FindFree( size );
	PrepareBlock( block, size );
	ycAssert( block->size >= _size );
	Check();
	return AllocDesc{ block->offset, block };
}

ycTLSFGpuHeap::AllocDesc ycTLSFGpuHeap::Alloc( const ycSize_t _size, const ycSize_t align )
{
	Check();
	if( align <= kGranularity ) { return Alloc( _size ); }
	ycAssert( (align%kGranularity) == 0 );
	const ycSize_t size = ycRoundUpPow2( _size, kGranularity );
	const ycSize_t paddedSize = size + align;
	Block* block = FindFree( paddedSize );
	const ycSize_t aligned = PrepareBlockAligned( block, size, align );
	ycAssert( (aligned%align) == 0 );
	ycAssert( block->offset + aligned + _size ); // tf is this checking? looks like an incomplete check which should be <= end
	Check();
	return AllocDesc{ aligned, block };
}

void ycTLSFGpuHeap::Free( const AllocDesc alloc )
{
	Free( alloc.block );
}

void ycTLSFGpuHeap::Free( Block* block )
{
	Check();
	ycAssert( !BlockIsFree(block) );

	Block* prev = block->prevPhys;
	if( BlockIsFree( prev ) )
	{
//ycLog( "merge left\n" );
		ycAssert( prev != &kNullBlock );
		const ycSize_t prevSize = prev->size;
		block->offset -= prevSize;
		block->size += prevSize;
		UnlinkPhys( prev );
		RemoveFreeBlock( prev );
		FreeBlock( prev );
	}
	Block* next = block->nextPhys;
	if( BlockIsFree( next ) )
	{
//ycLog( "merge right\n" );
		ycAssert( next != &kNullBlock );
		const ycSize_t nextSize = next->size;
		block->size += nextSize;
		UnlinkPhys( next );
		RemoveFreeBlock( next );
		FreeBlock( next );
	}

	BlockSetFree( block );
	AddToFreeList( block );

	Check();
}

/*static*/ void* ycTLSFGpuHeap::GetUserData( Block* block )
{
	return block->userData;
}

ycTLSFGpuHeap::Block* ycTLSFGpuHeap::AllocBlock()
{
	#ifdef YC_TLSFGPUHEAP_DEBUG
		++m_blockCount;
	#endif // YC_TLSFGPUHEAP_DEBUG
	return (Block*)YC_ALLOC( &m_blockMem, sizeof(Block), alignof(Block), "ycTLSFGpuHeap::Block" );
}

void ycTLSFGpuHeap::FreeBlock( Block* block )
{
	#ifdef YC_TLSFGPUHEAP_DEBUG
		ycAssert( m_blockCount );
		--m_blockCount;
	#endif // YC_TLSFGPUHEAP_DEBUG
	YC_FREE( &m_blockMem, block );
}

/*static*/ ureg ycTLSFGpuHeap::GetBitIndex( const ureg x )
{
	return 64 - ycBitUtils::CountLeadingZeros64( x ) - 1;
}

/*static*/ ureg ycTLSFGpuHeap::CalcFl( const ycSize_t size )
{
	if( size < kSmallBlockSize )
	{
		return 0;
	}
	else
	{
		return GetBitIndex( size ); // TODO HACK FIXME: shouldnt this -kFlIndexShift-1 ?
	}
}

/*static*/ void ycTLSFGpuHeap::CalcFlSl( const ycSize_t size, ureg* pFl, ureg* pSl )
{
	ureg fl, sl;
	// can you do this without the branch by masking out the bottom bits before clz?
	if( size < kSmallBlockSize )
	{
		fl = 0;
		sl = size / ( kSmallBlockSize / kSlIndexCount ); // cant this just be a right shift?
	}
	else
	{
		fl = GetBitIndex( size );
		sl = size >> ( fl - kSlIndexCountLog2 ); // shift the bits we care about to the bottom
		sl ^= 1ull << kSlIndexCountLog2; // and mask off the top bit (the one we used to figure out fl)
		fl -= kFlIndexShift-1;
	}
	*pFl = fl;
	*pSl = sl;
}

ycTLSFGpuHeap::Block* ycTLSFGpuHeap::FindFree( ycSize_t size )
{
	ycAssert( size );

	ureg fl, sl;
	Block* block = nullptr;
	if( size >= kSmallBlockSize ) // adjust size cuz we want the bin that contains blocks >= our size, not just includes our size (and potentially less)
	{
		size += ( 1ull << ( CalcFl(size) - kSlIndexCountLog2 ) ) - 1;
	}
	CalcFlSl( size, &fl, &sl );
	if( fl >= kFlIndexCount ) { return nullptr; } // adjusted size may go out of bounds; could check kMaxAllocSize earlier

	ureg slMap = m_slBitmap[ fl ] & ( ureg(-1) << sl ); // check sl bins equal to or larger than what we need
	if( !slMap )
	{
		// search up the first level list
		ureg flMap = m_flBitmap & ( ureg(-1) << (fl+1) );
		if( !flMap )
		{
			if( m_growFunc )
			{
				m_growFunc( this, size );
				flMap = m_flBitmap & ( ureg(-1) << (fl) );
				ycAssert( flMap );
			}
			else
			{
				ycAssert( false );
				return nullptr;
			}
		}
		fl = GetBitIndex( flMap );
		slMap = m_slBitmap[ fl ];
	}
	ycAssert( slMap );
	sl = GetBitIndex( slMap );
	block = m_free[ fl ][ sl ];

	ycAssert( block && block != &kNullBlock );
	RemoveFreeBlock( block, fl, sl );
	BlockSetUsed( block );

	return block;
}

void ycTLSFGpuHeap::PrepareBlock( Block* block, const ycSize_t size )
{
	ycAssert( block && !BlockIsFree(block) && size && block != &kNullBlock && block->size >= size );

	// split the block, returning the excess at the end back to the free lists
	const ycSize_t remain = block->size - size;
	if( remain >= kGranularity )
	{
		SplitBack( block, remain );
	}
}

ycSize_t ycTLSFGpuHeap::PrepareBlockAligned( Block* block, const ycSize_t size, const ycSize_t align )
{
	ycAssert( block && !BlockIsFree(block) && size && block != &kNullBlock && block->size >= size );

	const ycSize_t alignedOffset = ycRoundUpPow2( block->offset, align );
	const ycSize_t remainFront = alignedOffset - block->offset;
	const ycSize_t blockEnd = block->offset + block->size;
	const ycSize_t alignedEnd = alignedOffset + size;
	ycAssert( blockEnd >= alignedEnd );
	const ycSize_t remainBack = blockEnd - alignedEnd;

	// split a block off the front
	if( remainFront >= kGranularity )
	{
		SplitFront( block, remainFront );
	}

	// split a block off the end
	if( remainBack >= kGranularity )
	{
		SplitBack( block, remainBack );
	}

	return alignedOffset;
}

void ycTLSFGpuHeap::SplitFront( Block* block, const ycSize_t size )
{
	ycAssert( (size%kGranularity) == 0 );
	ycAssert( block->size > size );

	Block* newBlock = AllocBlock();
	newBlock->prevPhys = block->prevPhys;
	newBlock->nextPhys = block;
	newBlock->offset   = block->offset;
	newBlock->size     = size;
	newBlock->flags    = kFlag_BlockFree;
	newBlock->userData = block->userData;
	AddToFreeList( newBlock );
	
	#ifdef YC_TLSFGPUHEAP_DEBUG
	// TODO HACK FIXME: could get prev==kNullBlock instead; or swap block places and skip this check altogether
	for( ureg i = 0; i != m_physHead.Length(); ++i )
	{
		if( m_physHead.At(i) == block )
		{
			m_physHead.At(i) = newBlock;
			break;
		}
	}
	#endif

	block->prevPhys->nextPhys = newBlock; // maybe kNullBlock
	block->prevPhys = newBlock;
	block->size -= size;
	block->offset += size;
}

void ycTLSFGpuHeap::SplitBack( Block* block, const ycSize_t size )
{
	ycAssert( (size%kGranularity) == 0 );
	ycAssert( block->size > size );

	Block* newBlock = AllocBlock();
	newBlock->prevPhys = block;
	newBlock->nextPhys = block->nextPhys;
	newBlock->offset   = block->offset + block->size - size;
	newBlock->size     = size;
	newBlock->flags    = kFlag_BlockFree;
	newBlock->userData = block->userData;
	AddToFreeList( newBlock );

	block->nextPhys->prevPhys = newBlock; // maybe kNullBlock
	block->nextPhys = newBlock;
	block->size -= size;
}

void ycTLSFGpuHeap::AddToFreeList( Block* block )
{
	ureg fl, sl;
	CalcFlSl( block->size, &fl, &sl );
	ycAssert( block != &kNullBlock );
	ycAssert( BlockIsFree(block) );
	Block* prevFree = m_free[ fl ][ sl ];
	ycAssert( prevFree && block );
	block->nextFree = prevFree;
	block->prevFree = &kNullBlock;
	prevFree->prevFree = block; // potentially kNullBlock
	m_free[ fl ][ sl ] = block;
	m_flBitmap |= 1ull << fl;
	m_slBitmap[ fl ] |= 1ull << sl;
}

void ycTLSFGpuHeap::RemoveFreeBlock( Block* block, const ureg fl, const ureg sl )
{
	Block* prevFree = block->prevFree;
	Block* nextFree = block->nextFree;
	ycAssert( prevFree && nextFree );
	block->prevFree->nextFree = nextFree; // potentially kNullBlock
	block->nextFree->prevFree = prevFree;
	if( m_free[ fl ][ sl ] == block )
	{
		m_free[ fl ][ sl ] = nextFree;
		if( nextFree == &kNullBlock )
		{
			m_slBitmap[ fl ] ^= 1ull << sl;
			if( m_slBitmap[ fl ] == 0 )
			{
				m_flBitmap ^= 1ull << fl;
			}
		}
	}
}

void ycTLSFGpuHeap::RemoveFreeBlock( Block* block )
{
	ureg fl, sl;
	CalcFlSl( block->size, &fl, &sl );
	RemoveFreeBlock( block, fl, sl );
}

/*static*/ bool ycTLSFGpuHeap::BlockIsFree( Block* block )
{
	return ( block->flags & kFlag_BlockFree ) != 0;
}

/*static*/ void ycTLSFGpuHeap::BlockSetFree( Block* block )
{
	ycAssert( (block->flags&kFlag_BlockFree) == 0 );
	block->flags |= kFlag_BlockFree;
}

/*static*/ void ycTLSFGpuHeap::BlockSetUsed( Block* block )
{
	ycAssert( block->flags & kFlag_BlockFree );
	block->flags ^= kFlag_BlockFree;
}

void ycTLSFGpuHeap::UnlinkPhys( Block* block )
{
	ycAssert( block != &kNullBlock );
	block->prevPhys->nextPhys = block->nextPhys;
	block->nextPhys->prevPhys = block->prevPhys;
	#ifdef YC_TLSFGPUHEAP_DEBUG
		// TODO HACK FIXME: could get prev==kNullBlock instead
		for( ureg i = 0; i != m_physHead.Length(); ++i )
		{
			if( m_physHead.At(i) == block )
			{
				m_physHead.At(i) = block->nextPhys;
				break;
			}
		}
	#endif
}

void ycTLSFGpuHeap::Check()
{
#ifdef YC_TLSFGPUHEAP_DEBUG
	ycAssert( kNullBlock.size == 0 && kNullBlock.flags == 0 );
	ureg blockCount = 0;
	ureg bytes = 0;
	for( ureg physChunk = 0; physChunk != m_physHead.Length(); ++physChunk )
	{
		Block* block = m_physHead.At( physChunk );
		ycAssert( block->prevPhys == &kNullBlock );
		for(;;)
		{
			ycAssert( block->size );
			ycAssert( (block->size%kGranularity) == 0 );
			ycAssert( (block->offset%kGranularity) == 0 );
			if( block->prevPhys != &kNullBlock )
			{
				if( BlockIsFree(block) )
				{
					ycAssert( !BlockIsFree(block->prevPhys) ); // contiguous free blocks should have been merged
				}
				ycAssert( block->prevPhys->nextPhys == block );
				ycAssert( block->offset == block->prevPhys->offset + block->prevPhys->size );
			}
			++blockCount;
			bytes += block->size;
			if( block->nextPhys == &kNullBlock )
			{
				break;
			}
			if( BlockIsFree(block) )
			{
				ycAssert( !BlockIsFree(block->nextPhys) ); // contiguous free blocks should have been merged
			}
			block = block->nextPhys;
		}
	}
	ycAssert( blockCount == m_blockCount );
	ycAssert( bytes == m_totalBytes );
	for( ureg fl = 0; fl != kFlIndexCount; ++fl )
	{
		for( ureg sl = 0; sl != kSlIndexCount; ++sl )
		{
			// TODO: check that head has no prev
			ureg seen = 0;
			ureg seenBytes = 0;
			for( Block* block = m_free[ fl ][ sl ]; block != &kNullBlock; block = block->nextFree )
			{
				++seen;
				seenBytes += block->size;
				ycAssert( seen <= m_blockCount ); // cycle detection
				ycAssert( seenBytes <= m_totalBytes );
				ycAssert( BlockIsFree( block ) );
				ureg flCheck, slCheck;
				CalcFlSl( block->size, &flCheck, &slCheck );
				ycAssert( fl == flCheck && sl == slCheck );
				if( block->prevFree != &kNullBlock )
				{
					ycAssert( block->prevFree->nextFree == block );
				}
				if( block->nextFree != &kNullBlock )
				{
					ycAssert( block->nextFree->prevFree == block );
				}
			}
		}
	}
#endif // YC_TLSFGPUHEAP_DEBUG
}

#ifdef YC_TLSFGPUHEAP_DEBUG
#include "ycBitArray.h"
#include "ycRand.h"
/*static*/ void ycTLSFGpuHeap::Test()
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

	ycTLSFGpuHeap heap;
	enum
	{
		kMaxAllocs = 1024,
		kAllocSizeMin = 1,
		kAllocSizeMax = 1024*1024,
		kAllocAlignMin = 1,
		kAllocAlignMax = 256*1024,
		kSuperIter = 100,
		kSubIter = 1024*1024
	};
	ycTLSFGpuHeap::AllocDesc allocs[ kMaxAllocs ];
	ycStaticBitArray< kMaxAllocs > allocsUsed;
	ycRand rng;
#if 1
	heap.Init( []( ycTLSFGpuHeap* heap, const ycSize_t /*allocSize*/ )
		{
			static ycSize_t offset = 0;
			ycSize_t grow = 64*1024*1024;
			heap->AddRegion( offset, grow );
			offset += grow;
		});
#else
	heap.Init( ycRoundUpPow2( ureg(rng.RangeFast( 0, 1024 )), kGranularity ), 1024*1024 );
#endif
	for( ureg super = 0; super != kSuperIter; ++super )
	{
		for( ureg sub = 0; sub != kSubIter; ++sub )
		{
			const ureg idx = rng.Rand() % kMaxAllocs;
			if( allocsUsed.Get(idx) )
			{
				//ycLog( "free %p\n", allocs[idx].offset );
				heap.Free( allocs[ idx ] );
				allocsUsed.Clear( idx );
			}
			else
			{
				const ycSize_t size = rng.RangeFast( kAllocSizeMin, kAllocSizeMax );
				const ycSize_t align = ycRoundUpPow2( rng.RangeFast( kAllocAlignMin, kAllocAlignMax ) );
				if( align <= kGranularity )
				{
					allocs[ idx ] = heap.Alloc( size );
				}
				else
				{
					allocs[ idx ] = heap.Alloc( size, align );
				}
				allocsUsed.Set( idx );
				//ycLog( "alloc %p [%u bytes]\n", allocs[idx].offset, u32(size) );
			}
		}
		for( ureg i = 0; i != kMaxAllocs; ++i )
		{
			if( allocsUsed.Get(i) )
			{
				//ycLog( "free %p\n", allocs[i].offset );
				heap.Free( allocs[ i ] );
				allocsUsed.Clear( i );
			}
		}
	}
}
#endif // YC_TLSFGPUHEAP_DEBUG
