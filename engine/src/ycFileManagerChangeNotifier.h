#pragma once

#include "ycFileManager.h"

#ifdef YC_FILEMANAGER_ENABLE_FILE_VERSIONS
#include "ycHash.h"
#include "ycStackString.h"

typedef void( *ChangeCallBack )( u64 );
class ycFileManagerChangeNotifier
{
public:

	ycFileManagerChangeNotifier();
	ycFileManagerChangeNotifier( const ycStringRef& path, const ycFileManager::FileType fileType, ChangeCallBack callback, const u64 userData );
	void Init( const ycStringRef& path, const ycFileManager::FileType fileType, ChangeCallBack callback, const u64 userData );
	void Clear();
	inline bool IsNull() const { return m_path.IsEmpty(); }
	inline const ycString& GetPath() const { return m_path; }
	~ycFileManagerChangeNotifier();

protected:
	friend class ycFileManager;

	ycStackString<> m_path;
	ycHashKey_t     m_hashKey;
	ChangeCallBack  m_callback;
	u64             m_userData;
	ycFileManagerChangeNotifier* m_next;
	ycFileManagerChangeNotifier* m_prev;
};

#endif // YC_FILEMANAGER_ENABLE_FILE_VERSIONS
