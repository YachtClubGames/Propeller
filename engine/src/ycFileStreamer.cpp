#include "ycFileStreamer.h"

ycFileStreamer::ycFileStreamer( const char* fileName )
	: m_path( fileName )
	, m_tryCancelReads( false )
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		, m_prev( nullptr )
		, m_next( nullptr )
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
{
}

ycFileStreamer::~ycFileStreamer()
{
}
