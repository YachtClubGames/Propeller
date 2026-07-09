#pragma once

#include "ycCommon.h"

namespace ycVirtualMem
{
#if defined YC_MSFT
	constexpr bool kHasLargePages = false; //!< whether the platform has separate large/small page sizes
#elif YC_NDA
#elif YC_NDA
#elif defined YC_NIX || defined YC_OSX
	constexpr bool kHasLargePages = false;
#endif

	ycSize_t GetPageSize();
	ycSize_t GetLargePageSize(); //!< if the platform does not have large pages, returns the same value as GetPageSize()

	void* Reserve( const ycSize_t size );
	void Commit( void* base, const ycSize_t size
		#if YC_NDA
		#endif
	);
	void Release( void* base, const ycSize_t size ); //!< this only supports freeing the entire reserved range, the size passed must match

#if YC_NDA
#endif

	ycSize_t RoundToPageSize( const ycSize_t size ); //!< round to a reasonable page size (arbitrary logic for whether to use small or large pages)
}
