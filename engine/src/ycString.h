#pragma once

/*! \class ycString

Encapsulates a copy of a string.

*/

#include "ycCommon.h"
#include "ycStringRef.h"
#include <stdarg.h> // va_list

class ycAllocator;

template< class t_type, const bool t_isPOD >
class ycVector;

struct ycEmptyBuf;

class ycString : public ycStringRef
{
public:

	static void SetDefaultAllocator( ycAllocator* mem );
	static ycAllocator* GetDefaultAllocator();

	ycString();
	explicit ycString( ycAllocator* mem );
	explicit ycString( ycAllocator* mem, ycSize_t capacity );
	explicit ycString( const char* str, ycAllocator* mem = nullptr );
	explicit ycString( const ycStringRef str, ycAllocator* mem = nullptr );
	~ycString();

	/*!
	If freeMem == true, the memory is freed and the string IsNull() && IsEmpty(), but the allocator is not forgotten, so the string can be re-used
	If freeMem == false, the memory is not freed and the string IsEmpty() but _NOT_ IsNull(), since it does not point to null.
	Setting freeMem to false can be useful to re-use a string but not force a re-allocation every time.
	*/
	void Clear( const bool freeMem = true );

	ycStringRef GetRef() const;

	operator const char*() const { return GetPtr(); }

	ycSize_t Capacity() const { return m_capacity; }

	void Set( const char* str );
	void Set( const char * str, ycSize_t len );
	void Set( ycStringRef str );
	void SetChar( u32 charIdx, char c );

	void SetLength( ycSize_t len );

	ycSize_t Insert( ycSize_t where, const char* str );
	ycSize_t Insert( ycSize_t where, ycSize_t length, const char * str );

	void LeftJustified( ycSize_t len, const char fill );//!< returns size string of len that is padded by fill character

	void Remove( ycSize_t where, ycSize_t len );
	void Remove( ycStringRef erase );
	void RemovePrefix( ycStringRef prefix );
	void RemoveWhitespace();
	void RemoveLeadingWhitespace();
	void RemoveTrailingWhitespace();

	void ToLower( ycString* dst ) const;
	const char* ToCStr() const;
	char* ToCStr();

	void SetLiteral( const char* str, const ycSize_t len = ycSize_t(-1), const ycSize_t capacity = ycSize_t(-1) ); //!< does not copy the string (like the kNoCopy ctor)

	void Reserve( const ycSize_t minCapacity );
	void ReserveMore( const ycSize_t add ); //!< same as Reserve( Length()+1 /*NUL*/ + additional ), use this before appending a bunch of bits!
	
	bool operator == ( const ycString& rhs ) const;
	bool operator != ( const ycString& rhs ) const;
	bool operator == ( const char* rhs ) const;
	bool operator != ( const char* rhs ) const;

	ycString& operator = ( const char* rhs );
	ycString& operator = ( const ycStringRef& rhs );

	ycString( const ycString& other ); //!< inherits the allocator of the other string
	ycString& operator = ( const ycString& rhs ); //!< inherits the allocator of the other string

	ycString( ycString&& other ) noexcept;
	ycString& operator = ( ycString&& rhs ) noexcept;

	// should we have it?
	//operator bool() const { return !IsEmpty(); }

	ycStringRef Ref() const;

	char First() const;
	char Last() const;

	ycString& Append( const char ch );
	ycString& Append( const char* str );
	ycString& Append( const char* str, const ycSize_t len );
	ycString& Append( const ycString& str );
	ycString& Append( const ycStringRef& str );
	ycString& AppendF( const char* msg, ... );
	ycString& AppendFList( const char* msg, va_list args );

	ycString& SprintF( const char* msg, ... );
	ycString& SprintF( const char* msg, va_list args );

	void operator +=( const char * );
	void operator +=( const ycString& v);
	void operator +=( const ycStringRef& v);
	//ycString operator+( const char * v ) const;
	//ycString operator+( const ycStringRef& v ) const;
	//ycString operator+( const ycString& v ) const;

	bool TakeEndI( const char* str, const ycSize_t len );
	bool TakeEndI( const char* str );

	//! replaces orig with replace and returns number of occurences.
	ycSize_t Replace( char orig, char replace );
	ycSize_t Replace( const char * orig, const char * replace );
	ycSize_t Replace( const ycStringRef& orig, const ycStringRef& replace );
	ycSize_t ReplaceSingle( const ycStringRef& orig, const ycStringRef& replace, s32 start = 0);

	ycVector< ycString, false > Split( const char * findStr );
	void Join( const ycVector< ycString, false > & join, const char * separator );

	void Prepend( ycStringRef prefix );

	void ToUpper();
	void ToLower();

	void NullTerminate();
	void DoubleNullTerminate();

	void PadToLength( char c, ycSize_t pos );

	void SetAllocator( ycAllocator* mem ); // this function is dangerous, if the allocator has an allocation already, it will _not_ re-allocate it into the new one, so when it tries to free, it will use the wrong allocator
	ycAllocator* GetAllocator();

	void SetGrowable( bool growable = true );
	bool IsGrowable();

protected:

	void SetMemoryManagedExternal( bool external = true );
	bool IsMemoryManagedExternal();


	char *		m_alloc; // string ref can change the original str pointer. probably should do something different to make ycStringRef and ycString play together better

	ycAllocator* m_mem; // i think the only time we really want a special allocator is *maybe* debug strings; would it be better to find another solution for this?
	ycSize_t     m_capacity; // but capacity includes terminating NULL which m_len does not
	u32			m_flags;
	enum
	{
		kFlag_MemoryExternal = 1 << 0,
		kFlag_GrowableDisabled = 1 << 1
	};
};

inline ycHashKey_t ycHashKey( const ycString& key ) { return key.Hash(); }