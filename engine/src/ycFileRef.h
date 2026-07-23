#pragma once
#ifndef YC_FILEREF_H
#define YC_FILEREF_H

#include "ycCommon.h"
#include "ycFileManager.h"

class ycAllocatorTS;
class ycManagedFile;
class ycStringRef;

#define ycFileBlockingLog ycLog

class ycFileRefBase
{
public:

	explicit ycFileRefBase( const char* basePath, ycFileManager::FileType fileType = ycFileManager::kFile_Data, const ureg fileLoadFlags = 0 );
	ycFileRefBase( const ycStringRef& basePath, ycFileManager::FileType fileType = ycFileManager::kFile_Data, const ureg fileLoadFlags = 0 );
	ycFileRefBase(); //! constructs a null file ref, cannot be used until assigned
	ycFileRefBase( const ycFileRefBase& other ); //!< copy ctor
	ycFileRefBase( ycFileRefBase&& other ) noexcept; //!< move ctor
	~ycFileRefBase();

	void Clear(); //!< the destructor automatically releases the file ref, but there are circumstances where it is useful to call this manually (re-use a file handle, release a file ref that persists beyond the existence of the manager)

	const u8* GetData() const; //!< asserts if the file is not loaded
	ycSize_t GetSize() const; //!< asserts if the file is not loaded

	bool IsNull() const;
	bool IsLoading() const; //!< asserts if IsNull()
	bool IsLoaded() const; //!< asserts if IsNull()
	bool IsNotFound() const; //!< asserts if IsNull()

	ycStringRef Path() const; //!< asserts if IsNull()
	ycStringRef BasePath() const { return Path(); }
	ycStringRef Data() const;
	ycFileManager::FileType GetFileType() const;

	ycFileRefBase& operator = ( const ycFileRefBase& rhs ); //!< copy assignment

protected:

	ycManagedFile* m_managedFile = nullptr;
};

template< class t_fileType >
class ycFileRef : public ycFileRefBase
{
public:

	explicit ycFileRef( const char* basePath, ycFileManager::FileType fileType = ycFileManager::kFile_Data ) : ycFileRefBase( basePath, fileType ) {}
	ycFileRef( const ycStringRef& basePath, ycFileManager::FileType fileType = ycFileManager::kFile_Data ) : ycFileRefBase( basePath, fileType ) {}
	ycFileRef() {}
	ycFileRef( const ycFileRefBase& other ) : ycFileRefBase( other ) {}
	ycFileRef( const ycFileRef& other ) : ycFileRefBase( other ) {}

	const t_fileType* GetData() const { return (const t_fileType*)ycFileRefBase::GetData(); }
	t_fileType* GetData() { return (t_fileType*)ycFileRefBase::GetData(); }
	operator const t_fileType*() const { return (const t_fileType*)ycFileRefBase::GetData(); }
	operator t_fileType*() { return (t_fileType*)ycFileRefBase::GetData(); }

	const t_fileType* operator->() const { return (t_fileType*)ycFileRefBase::GetData(); }
	const t_fileType* operator*() const { return (t_fileType*)ycFileRefBase::GetData(); }

	t_fileType* operator->() { return (t_fileType*)ycFileRefBase::GetData(); }
	t_fileType* operator*() { return (t_fileType*)ycFileRefBase::GetData(); }

	ycFileRefBase& operator = ( const ycFileRefBase& rhs )
	{
		return ycFileRefBase::operator=(rhs);
	}

	ycFileRef& operator=( const ycFileRef& other )
	{
		new( this ) ycFileRefBase( other );
		return *this;
	}
};

#endif // !YC_FILEREF_H
