#include "ycString.h"

// core
#include "ycAllocator.h"
#include "ycStd.h"
#include "ycStringUtils.h"
#include "ycVector.h"

namespace
{
	ycAllocator* s_defaultAllocator = nullptr;
}

/*static*/ void ycString::SetDefaultAllocator( ycAllocator* mem )
{
	s_defaultAllocator = mem;
}

/*static*/ ycAllocator* ycString::GetDefaultAllocator()
{
	return s_defaultAllocator;
}

ycString::ycString()
{
	m_mem = s_defaultAllocator;
	m_str = nullptr;
	m_alloc = nullptr;
	m_len = 0;
	m_capacity = 0;
	m_flags = 0;
}

ycString::ycString( ycAllocator* mem )
{
	m_mem = mem == nullptr ? s_defaultAllocator : mem ;
	m_str = nullptr;
	m_alloc = nullptr;
	m_len = 0;
	m_capacity = 0;
	m_flags = 0;
}

ycString::ycString( ycAllocator* mem, ycSize_t capacity )
{
	ycAssert( capacity );
	m_mem = mem == nullptr ? s_defaultAllocator : mem ;
	m_str = (char*)YC_ALLOC( m_mem, capacity, YC_PTR_SIZE, "ycString" );
	m_alloc = m_str;
	m_str[0] = '\0';
	m_len = 0;
	m_capacity = capacity;
	m_flags = 0;
}

ycString::ycString( const char* str, ycAllocator* mem )
{
	m_mem = mem ? mem : s_defaultAllocator ;
	m_str = nullptr;
	m_alloc = nullptr;
	m_len = 0;
	m_capacity = 0;
	m_flags = 0;
	Set( str );
}

ycString::ycString( const ycStringRef str, ycAllocator* mem )
{
	m_mem = mem ? mem : s_defaultAllocator ;
	m_str = nullptr;
	m_alloc = nullptr;
	m_len = 0;
	m_capacity = 0;
	m_flags = 0;
	Set( str );
}

ycString::~ycString()
{
	Clear();
}

void ycString::Clear( const bool freeMem )
{
	if( m_str )
	{
		// so IsEmpty() is true and Get() will always return an empty string
		// no matter what the string memory is backed with (like ycStackString)
		m_str[ 0 ] = '\0';
	}
	if( freeMem )
	{
		if( !IsMemoryManagedExternal() )
		{
			if( m_alloc && m_mem )
			{
				YC_FREE( m_mem, m_alloc );
			}
			m_alloc = nullptr;
			m_str = nullptr;
			m_capacity = 0;
		}
	}
	m_len = 0;
}

ycStringRef ycString::GetRef() const
{
	return ycStringRef( m_str, m_len );
}

void ycString::SetLiteral( const char* str, const ycSize_t len, const ycSize_t capacity )
{
	Clear();
	m_str      = const_cast<char*>(str); // Note: not nice, may need to rethink constness here.
	m_alloc    = m_str;
	m_len      = len      == ycSize_t(-1) ? ycStringUtils::Length( str ) : len ;
	m_capacity = capacity == ycSize_t(-1) ? m_len+1                   : capacity ;
	m_mem      = nullptr;
}

void ycString::Reserve( const ycSize_t minCapacity )
{
	if( m_capacity >= minCapacity ) { return; }

	ycAssert( m_mem );
	ycAssert( IsGrowable() );

	char* str = (char*)YC_ALLOC( m_mem, minCapacity, YC_PTR_SIZE, "ycString" ); // could use the string itself, but until debug string storing is sorted use this
	str[0] = '\0'; // for safety, always shove a NUL byte in there (same as the ctor does)
	if( m_str )
	{
		ycMemCpy( str, m_str, m_capacity );
		if( !IsMemoryManagedExternal() )
		{
			YC_FREE( m_mem, m_alloc );
		}
	}
	else
	{
		ycMemSet( str, 0, minCapacity );
	}
	m_alloc = str;
	m_str = str;
	m_capacity = minCapacity;
	if( IsMemoryManagedExternal() )
	{
		SetMemoryManagedExternal( false );
	}
}

void ycString::ReserveMore( const ycSize_t add )
{
	Reserve( Length() + 1 + add );
}

void ycString::Set( const char* str )
{
	if( str != nullptr )
	{
		Set( str, ycStringUtils::Length( str ) );
	}
	else if( m_str )
	{
		ycAssert( str == nullptr );
		Clear();
	}
}

void ycString::Set( const char* str, ycSize_t len )
{
	const ycSize_t size = str && len ? ( len + 1 ) : 1 ;
	if( size > m_capacity )
	{
		ycSize_t pad_size = (size + 16) & ycSize_t(-16);
		Reserve( pad_size );
	}
	if( size )
	{
		if( len ) { ycMemMove( m_str, str, len ); }
		m_str[ len ] = '\0';
	}
	m_len = len;
}

void ycString::Set( ycStringRef str )
{
	Set( str.GetPtr(), str.Length() );
}

void ycString::SetChar( u32 charIdx, char c )
{
	ycAssertMsg( charIdx >= 0 && charIdx < m_len, "Out of index" );
	m_str[charIdx] = c;
}

void ycString::SetLength( ycSize_t len ) 
{
	ycAssert( m_len <= m_capacity );
	m_len = len; 
}

ycSize_t ycString::Insert( ycSize_t where, ycSize_t length, const char* str )
{
	if( !str || !*str ) { return 0; }

	ycSize_t orgLen = m_len;
	if( where > orgLen ) { where = orgLen; }

	ycSize_t capacityOffset = (m_str-m_alloc);
	if( ( orgLen + length + 1 ) >= (m_capacity-capacityOffset) ) { Reserve( orgLen + length + 1 + 16 ); }
	m_len += length;

	char* dst = m_str + orgLen + length + 1;	// end of new string
	const char* src = dst - length;		// end of orig string
	const char* end = m_str + where;	// insert point
	*dst = 0;
	while( src > end ) { *--dst = *--src; }

	src = str + length;
	while( src > str ) { *--dst = *--src; }
	return length;
}

ycSize_t ycString::Insert( ycSize_t where, const char* str )
{
	if( !str || !*str ) { return 0; }
	return Insert( where, ycStringUtils::Length( str ), str );
}

void ycString::LeftJustified( ycSize_t len, const char fill )
{
	Reserve( len );
	while( m_len < len )
	{
		Append( fill );
	}
}

void ycString::Remove( ycSize_t where, ycSize_t len )
{
	ycSize_t orig = ycStringUtils::Length( m_str );
	if( where >= orig ) { return; }
	if( (where + len) > orig ) { len = orig - where; }
	m_len -= len;

	char* dst = m_str + where;
	const char* src = dst + len;
	while( *src ) { *dst++ = *src++; }
	*dst = 0;
}

void ycString::Remove( ycStringRef erase )
{
	s32 offs = ycStringRef( m_str, m_len ).FindAt( erase, 0 );
	if( offs >=0 ) { m_len = ycStringUtils::RemoveSubStr( m_str, m_len, m_str + offs, erase.Length() ); }
}

void ycString::RemovePrefix( ycStringRef prefix )
{
	if( BeginsWith( prefix ) )
	{
		m_len = ycStringUtils::RemoveSubStr( m_str, m_len, m_str, prefix.Length() );
	}
}

void ycString::RemoveWhitespace()
{
	u32 wsLen = 0;
	while( wsLen < m_len && m_str[ wsLen ] == ' ' ) wsLen++;
	if( wsLen > 0 ) Remove( 0, wsLen );
}

void ycString::RemoveLeadingWhitespace()
{
	if( m_len == 0 ) return;
	ycSize_t idx = 0;
	while( idx < m_len && m_str[ idx ] == ' ' ) idx++;
	if( idx > 0 ) Remove( 0, idx );
}

void ycString::RemoveTrailingWhitespace()
{
	if( m_len == 0 ) return;
	s32 idx = (s32)(m_len - 1);
	while( idx >= 0 && m_str[ idx ] == ' ' ) idx--;
	u32 len = (u32)(m_len - u32( idx + 1 ));
	if( len > 0 ) Remove( idx + 1, len );
}

void ycString::ToLower( ycString* dst ) const
{
	dst->ReserveMore( m_len+1 );
	for( ycSize_t i = 0; i != m_len; ++i )
	{
		dst->Append( ycStringUtils::ToLower( m_str[ i ] ) );
	}
}

const char* ycString::ToCStr() const
{
	if( m_str == nullptr ) { return nullptr; }
	m_str[ m_len ] = 0; 
	return m_str; 
}

char* ycString::ToCStr()
{
	if( m_str == nullptr ) { return nullptr; }
	m_str[ m_len ] = 0; 
	return m_str; 
}

bool ycString::operator == ( const ycString& rhs ) const
{
	return Length() == rhs.Length() && ycStringUtils::Equals( Get(), rhs.Get() );
}

bool ycString::operator != ( const ycString& rhs ) const
{
	return !( operator==(rhs) );
}

bool ycString::operator == ( const char* rhs ) const
{
	return ycStringUtils::Equals( Get(), rhs );
}

bool ycString::operator != ( const char* rhs ) const
{
	return !( operator==(rhs) );
}

ycString& ycString::operator = ( const char* rhs )
{
	Set( rhs );
	return *this;
}

ycString& ycString::operator = ( const ycStringRef& rhs )
{
	Set( rhs );
	return *this;
}

ycString::ycString( const ycString& other )
{
	new( this )ycString( other.GetRef(), other.m_mem );
}

ycString& ycString::operator = ( const ycString& rhs )
{
	Clear();
	m_mem = rhs.m_mem;
	if( rhs.m_len )
	{
		Set( rhs );
	}
	m_len = rhs.m_len;
	return *this;
}

ycString::ycString( ycString && other ) noexcept
{
	m_str = other.m_str;
	m_alloc = other.m_alloc;
	m_mem = other.m_mem;
	m_len = other.m_len;
	m_capacity = other.m_capacity;
	m_flags = other.m_flags;
	new( &other )ycString;
}

ycString& ycString::operator = ( ycString&& rhs ) noexcept
{
	Clear();
	m_str = rhs.m_str;
	m_alloc = rhs.m_alloc;
	m_mem = rhs.m_mem;
	m_len = rhs.m_len;
	m_capacity = rhs.m_capacity;
	m_flags = rhs.m_flags;
	new( &rhs )ycString;
	return *this;
}
	
ycStringRef ycString::Ref() const
{
	return ycStringRef( Get(), Length() );
}

char ycString::First() const
{
	return Ref().First();
}

char ycString::Last() const
{
	return Ref().Last();
}

ycString& ycString::Append( const char ch )
{
	ReserveMore( 1 );
	m_str[ m_len ] = ch;
	++m_len;
	m_str[ m_len ] = '\0';
	return *this;
}

ycString& ycString::Append( const char* str )
{
	return Append( str, ycStringUtils::Length( str ) );
}

ycString& ycString::Append( const char* str, const ycSize_t len )
{
	const ycSize_t needCapacity = Length() + 1 + len;
	ycSize_t capacityOffset = (m_str-m_alloc);
	if( (m_capacity-capacityOffset) < needCapacity ) { Reserve( needCapacity ); }
	const ycSize_t curLen = Length();
	ycMemCpy( m_str + curLen, str, len );
	m_str[ curLen + len ] = '\0';
	m_len += len;
	return *this;
}

ycString& ycString::Append( const ycString& str )
{
	return Append( str.Get(), str.Length() );
}

ycString& ycString::Append( const ycStringRef& str )
{
	return Append( str.GetPtr(), str.Length() );
}

ycString& ycString::AppendFList( const char* msg, va_list args )
{
	va_list args2; // must copy to iterate a second time (on some platforms/compilers)
	va_copy( args2, args );

	ycSize_t capacityOffset = (m_str-m_alloc);
	ycSize_t capacity = m_capacity - capacityOffset;
	ycSize_t required = ycStringUtils::VSprintF_NoCheck( m_str + m_len, capacity - m_len, msg, args );

	ycSize_t neededCapacity = m_len + required + 1;
	if( neededCapacity > capacity )
	{
		Reserve( neededCapacity );
		capacity = m_capacity - capacityOffset;
		ycAssert( capacity >= neededCapacity );
		ycSize_t required2 = ycStringUtils::VSprintF_NoCheck( m_str + m_len, capacity - m_len, msg, args2 );
		ycAssert( required == required2 );
		YC_UNUSED( required2 );
	}

	m_len += required;
	return *this;
}

ycString& ycString::AppendF( const char* msg, ... )
{
	va_list args;
	va_start( args, msg );
	AppendFList( msg, args );
	va_end( args );
	return *this;
}

ycString& ycString::SprintF( const char* msg, ... )
{
	m_len = 0;
	va_list args;
	va_start( args, msg );
	SprintF( msg, args );
	va_end( args );
	return *this;
}

ycString& ycString::SprintF( const char* msg, va_list args )
{
	m_len = 0;
	AppendFList( msg, args );
	return *this;
}

void ycString::operator +=( const char * v )
{
	this->Append( v );
}

void ycString::operator +=( const ycString& v )
{
	this->Append( v );
}

void ycString::operator +=( const ycStringRef& v )
{
	this->Append( v );
}

/*
ycString ycString::operator+( const char * v ) const
{
	ycString str( this );
	str.Append( v );
	reutrn str;
}

ycString ycString::operator+( const ycStringRef& v ) const
{
	ycString str( this );
	str.Append( v );
	reutrn str;
}

ycString ycString::operator+( const ycString& v ) const
{
	ycString str( this );
	str.Append( v );
	reutrn str;
}
*/

bool ycString::TakeEndI( const char* str, const ycSize_t len )
{
	if( EndsWithI( str, len ) )
	{
		m_len -= len;
		NullTerminate();
		return true;
	}
	return false;
}

bool ycString::TakeEndI( const char* str )
{
	if( str )
	{
		return TakeEndI( str, ycStringUtils::Length( str ) );
	}
	return false;
}

ycSize_t ycString::Replace( char orig, char replace )
{
	ycSize_t count = 0;
	for( ycSize_t o=0; o<m_len; ++o )
	{
		if( m_str[o] == orig )
		{
			m_str[o] = replace;
			++count;
		}
	}
	return count;
}

ycSize_t ycString::Replace( const char * orig, const char * replace )
{
	if( !orig || !*orig ) { return 0; }
	return Replace( ycStringRef( orig ), ycStringRef( replace ) );
}

ycSize_t ycString::Replace( const ycStringRef& orig, const ycStringRef& replace )
{
	if( orig.IsEmpty() ) { return 0; }
	ycSize_t count = 0;
	s32 copyIndex = 0;
	s32 findIndex;
	//this could be faster depending on the size comparsions of orig/replace and not using erase/insert
	while( copyIndex < (s32)Length() && (findIndex = FindAt( orig, copyIndex )) >= 0 )
	{
		Remove( findIndex, orig.Length() );
		Insert( findIndex, replace.Length(), replace.GetPtr() );
		copyIndex = findIndex + (s32)replace.Length();
		count++;
	}
	return count;
}

ycSize_t ycString::ReplaceSingle( const ycStringRef& orig, const ycStringRef& replace, s32 start )
{
	if( orig.IsEmpty() ) { return 0; }
	ycSize_t count = 0;
	s32 copyIndex = start;
	s32 findIndex;
	//this could be faster depending on the size comparsions of orig/replace and not using erase/insert
	while( copyIndex < (s32)Length() && (findIndex = FindAt( orig, copyIndex )) >= 0 && count < 1 )
	{
		Remove( findIndex, orig.Length() );
		Insert( findIndex, replace.Length(), replace.GetPtr() );
		copyIndex = findIndex + (s32)replace.Length();
		count++;
	}
	return count;
}

ycVector< ycString > ycString::Split( const char * findStr )
{
	ycVector< ycString > list;
	s32 findIndex;
	ycSize_t copyIndex = 0;
	ycSize_t length = ycStringUtils::Length( findStr );
	while( (findIndex = FindAt( findStr, copyIndex )) >= 0 )
	{
		ycString str;
		str.Set( &Get()[copyIndex], findIndex-copyIndex );
		list.Append( str );
		copyIndex = findIndex + length;
	}
	if( copyIndex < Length() )
	{
		ycString str;
		str.Set( &Get()[copyIndex] );
		list.Append( str );
	}
	return list;
}

void ycString::Join( const ycVector< ycString, false >& join, const char * separator )
{
	for( const ycString& str : join )
	{
		Append( str );

		if( str.Compare( join.Last() ) != 0 )
		{
			Append( separator );
		}
	}
}

void ycString::Prepend( ycStringRef prefix )
{
	Reserve( prefix.Length() + m_len );
	m_len = ycStringUtils::InsertSubStr( m_str, m_len, m_capacity, 0, prefix.GetPtr(), prefix.Length() );
}

void ycString::ToUpper()
{
	for( ycSize_t o=0; o<m_len; ++o )
	{
		m_str[o] = ycStringUtils::ToUpper( m_str[o] );
	}
}

void ycString::ToLower()
{
	for( ycSize_t o=0; o<m_len; ++o )
	{
		m_str[o] = ycStringUtils::ToLower( m_str[o] );
	}
}

void ycString::NullTerminate() 
{
	m_str[ m_len ] = '\0';
}

void ycString::DoubleNullTerminate() 
{
	NullTerminate();
	Reserve( m_len+1 );
	m_str[ m_len+1 ] = '\0';
}

void ycString::PadToLength( char c, ycSize_t pos )
{
	if( m_len < pos )
	{
		if( pos > m_capacity ) { Reserve( pos + 1 + 16 ); }
		for( ycSize_t pad = m_len; pad < pos; ++pad ) { m_str[ pad ] = c; }
	}
	m_len = pos;
}

void ycString::SetAllocator( ycAllocator* mem )
{
	m_mem = mem;
}

ycAllocator* ycString::GetAllocator()
{
	return m_mem;
}

void ycString::SetMemoryManagedExternal( bool external )
{
	if( external )
	{
		m_flags |= kFlag_MemoryExternal;
	}
	else
	{
		m_flags &= ~kFlag_MemoryExternal;
	}
}

bool ycString::IsMemoryManagedExternal()
{
	return (m_flags & kFlag_MemoryExternal) != 0;
}

void ycString::SetGrowable( bool growable )
{
	if( !growable )
	{
		m_flags |= kFlag_GrowableDisabled;
	}
	else
	{
		m_flags &= ~kFlag_GrowableDisabled;
	}
}

bool ycString::IsGrowable()
{
	return (m_flags & kFlag_GrowableDisabled) == 0;
}