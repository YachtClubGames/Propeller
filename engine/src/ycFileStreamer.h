#pragma once

#include "ycCommon.h"

#include "ycFile.h"
#include "ycFileManagerChangeNotifier.h"
#include "ycStd.h"
#include "ycString.h"
#include "ycStringRef.h"

//class ycAtomic;
class ycFileManager;

class ycFileStreamer : public ycNoCopy // no copying, not ref counted like ycFileRef, could add if useful though
{
public:

	~ycFileStreamer();
	inline ycStringRef GetPath() const { return m_path.GetRef(); }

protected:
	friend class ycFileManager;

	ycFileStreamer( const char* fileName ); // created by ycFileManager to make lifetime mgmt easier

	ycFileReader m_reader;
	ycString     m_path; // TODO HACK FIXME: debug only?
	bool         m_tryCancelReads;
	#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
		ycFileStreamer* m_prev;
		ycFileStreamer* m_next;
	#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
};
