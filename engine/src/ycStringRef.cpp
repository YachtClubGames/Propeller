#include "ycStringRef.h"

// core
#include "ycStringUtils.h"
#include "ycStd.h"
#include "ycBitField.h"
#include "ycMath.h"
#include "ycVector.h"

ycStringRef::ycStringRef()
{
	Clear();
}

ycStringRef::ycStringRef( const char* str )
{
	Set( str );
}

ycStringRef::ycStringRef( const char* str, const ycSize_t len )
{
	Set( str, len );
}

ycStringRef::ycStringRef( const ycStringRef& other )
{
	Set( other.GetPtr(), other.Length() );
}

void ycStringRef::Clear()
{
	m_str = nullptr;
	m_len = 0;
}

const char* ycStringRef::Get() const
{
	return m_str ? m_str : "";
}

const char* ycStringRef::GetPtr( const ycSize_t offset ) const
{
	ycAssertMsg( offset < m_len, "Out of index" );
	return m_str + offset;
}

char ycStringRef::GetChar( const ycSize_t charIdx ) const 
{ 
	ycAssertMsg( charIdx < m_len, "Out of index" );
	return m_str[charIdx]; 
}

ycSize_t ycStringRef::Length() const
{
	return m_len;
}

void ycStringRef::Set( const char* str )
{
	m_str = const_cast<char*>(str);
	m_len = str ? ycStringUtils::Length( str ) : 0;
}

void ycStringRef::Set( const char* str, const ycSize_t len )
{
	m_str = const_cast<char*>(str);
	m_len = len;
}

void ycStringRef::Set( const ycStringRef& strRef )
{
	m_str = strRef.m_str;
	m_len = strRef.m_len;
}

bool ycStringRef::IsNull() const
{
	return m_str == nullptr;
}

bool ycStringRef::IsEmpty() const
{
	return IsNull() || m_len == 0 || m_str[0] == '\0';
}

bool ycStringRef::Equals( const char* str ) const
{
	return Equals( str, ycStringUtils::Length( str ) );
}

bool ycStringRef::Equals( const char* str, const ycSize_t len ) const
{
	if( m_len != len ) { return false; }
	for( ureg i = 0; i != len; ++i )
	{
		if( m_str[ i ] != str[ i ] ) { return false; }
	}
	return true;
}

bool ycStringRef::Equals( const ycStringRef& other ) const
{
	return Equals( other.GetPtr(), other.Length() );
}

bool ycStringRef::EqualsI( const char* str ) const
{
	ureg len = m_len;
	if( len == 0 && ( !str || str[0] == 0 ) ) { return true; }
	if( !str || str[0] == 0 ) { return false; }
	for( ureg i = 0; i != len; ++i )
	{
		if( ycStringUtils::ToLower( m_str[ i ] ) != ycStringUtils::ToLower( str[ i ] ) ) { return false; }
	}
	if( str[ len ] != 0 ) { return false; }
	return true;
}

bool ycStringRef::EqualsI( const char* str, const ycSize_t len ) const
{
	if( m_len != len ) { return false; }
	for( ureg i = 0; i != len; ++i )
	{
		if( ycStringUtils::ToLower(m_str[ i ]) != ycStringUtils::ToLower(str[ i ]) ) { return false; }
	}
	return true;
}

bool ycStringRef::EqualsI( const ycStringRef& other ) const
{
	return EqualsI( other.GetPtr(), other.Length() );
}

s32 ycStringRef::Compare( const char * str ) const
{
	const char *s = m_str;
	if( !m_str ) { return str ? 1 : 0; }
	else if( !str ) { return -1; }
	ycSize_t l = m_len;
	while( l )
	{
		char c0 = *s++;
		char c1 = *str++;
		if( !c1 ) { return 1; }
		if( c0 < c1 ) { return -1; }
		else if( c0 > c1 ) { return 1; }
		--l;
	}
	return *str ? -1 : 0;
}

s32 ycStringRef::CompareLower( const char * str ) const
{
	const char *s = m_str;
	while( *s )
	{
		if( char c1 = *str++ )
		{
			char c0 = *s++;
			c0 = ycStringUtils::ToLower( c0 );
			c1 = ycStringUtils::ToLower( c1 );
			if( c0 < c1 ) { return -1; }
			else if( c0 > c1 ) { return 1; }
		}
		else
		{
			return 1;
		}
	}
	return *str ? -1 : 0;
}

s32 ycStringRef::Compare( const ycStringRef& other ) const
{
	const char *s = m_str;
	const char * str = other.m_str;
	if( IsEmpty() ) { return !other.IsEmpty() ? 1 : 0; }
	else if( other.IsEmpty() ) { return !IsEmpty() ? 1 : 0; }
	if( other.Length() > Length() ) { return -1; }
	if( other.Length() < Length() ) { return 1; }
	ycSize_t l = m_len;
	while( l )
	{
		char c0 = *s++;
		char c1 = *str++;
		if( c0 < c1 ) { return -1; }
		else if( c0 > c1 ) { return 1; }
		--l;
	}
	return 0;
}

s32 ycStringRef::CompareLower( const ycStringRef& other ) const
{
	const char *s = m_str;
	const char * str = other.m_str;
	if( IsEmpty() ) { return !other.IsEmpty() ? 1 : 0; }
	else if( other.IsEmpty() ) { return !IsEmpty() ? 1 : 0; }
	if( other.Length() > Length() ) { return -1; }
	if( other.Length() < Length() ) { return 1; }
	ycSize_t l = m_len;
	while( l )
	{
		char c0 = ycStringUtils::ToLower( *s++ );
		char c1 = ycStringUtils::ToLower( *str++ );
		if( c0 < c1 ) { return -1; }
		else if( c0 > c1 ) { return 1; }
		--l;
	}
	return 0;
}

bool ycStringRef::IsLower() const
{
	const char *s = m_str;
	ycSize_t l = m_len;
	while( l )
	{
		if( ycStringUtils::IsLower( *s++ ) )
		{
			return false;
		}
	}
	return true;
}

size_t ycStringRef::CopyTo( char* dest, ycSize_t space ) const
{
	ycAssert( dest && space );
	if( m_len )
	{
		ycSize_t use = m_len < space ? m_len : ( space - 1 );
		ycMemCpy( dest, m_str, use );
		dest[ use ] = 0;
		return use;
	}
	dest[ 0 ] = 0;
	return 0;
}

void ycStringRef::Advance( const ycSize_t len )
{
	ycAssert( len <= m_len );
	m_len -= len;
	m_str += len;
}

char ycStringRef::First() const
{
	// must handle empty strings
	return m_len ? m_str[ 0 ] : '\0' ;
}

char ycStringRef::At( const ycSize_t pos ) const
{
	// must handle empty strings
	return pos <= m_len ? m_str[ pos ] : '\0' ;
}

char ycStringRef::Last() const
{
	return m_len ? m_str[ m_len-1 ] : '\0' ;
}

ycStringRef ycStringRef::Left( const ycSize_t len ) const
{
	ycAssert( m_len >= len );
	return ycStringRef( m_str, len );
}

ycStringRef ycStringRef::RightOf( const ycSize_t index ) const
{
	ycSize_t len = m_len - index - 1;
	if( index >= m_len || m_len < len )
	{
		return ycStringRef();
	}
	return Right( len );
}

ycStringRef ycStringRef::TakeLeft( const ycSize_t len )
{
	ycAssert( m_len >= len );
	const char* str = m_str;
	m_str += len;
	m_len -= len;
	return ycStringRef( str, len );
}

ycStringRef ycStringRef::Right( const ycSize_t len ) const
{
	ycAssert( m_len >= len );
	const ycSize_t offset = Length() - len;
	return ycStringRef( m_str + offset, len );
}

ycStringRef ycStringRef::TakeRight( const ycSize_t len )
{
	ycAssert( m_len >= len );
	const ycSize_t offset = Length() - len;
	m_len -= len;
	return ycStringRef( m_str + offset, len );
}

static bool IsWordChar( char c )
{
	return ( c>='A' && c<='Z' ) || ( c>='a' && c<='z' ) || ( c>='0' && c<='9' ) || c=='_';
}

ycStringRef ycStringRef::TakeWord()
{
	ycSize_t s = 0;
	while( s < m_len && !IsWordChar( m_str[ s ] ) ) { ++s; }
	ycSize_t e = s;
	while( e < m_len && IsWordChar( m_str[ e ] ) ) { ++e; }
	ycSize_t c = e;
	while( c < m_len && ( ( u8 )m_str[ c ] )<=' ' ) { ++c; }
	const char *word = m_str + s;
	m_len -= c;
	m_str += c;
	return ycStringRef( word, e-s );
}

ycStringRef ycStringRef::Word() const
{
	ycSize_t s = 0;
	while( s < m_len && !IsWordChar( m_str[ s ] ) ) { ++s; }
	ycSize_t e = s;
	while( e < m_len && IsWordChar( m_str[ e ] ) ) { ++e; }
	ycSize_t c = e;
	while( c < m_len && ( ( u8 )m_str[ c ] ) <= ' ' ) { ++c; }
	const char *word = m_str + s;
	return ycStringRef( word, e - s );
}

// BeginsWith

bool ycStringRef::BeginsWith( const char c ) const
{
	return m_len && m_str[0] == c;
}

bool ycStringRef::BeginsWith( const char* str ) const
{
	return BeginsWith( ycStringRef( str ) );
}

bool ycStringRef::BeginsWith( const char* str, const ycSize_t len ) const
{
	return BeginsWith( ycStringRef( str, len ) );
}

bool ycStringRef::BeginsWith( const ycStringRef& str ) const
{
	if( m_len >= str.Length() ) { return Left( str.Length() ).Equals( str ); }
	return false;
}

// BeginsWithI

bool ycStringRef::BeginsWithI( const char* str ) const
{
	return BeginsWithI( ycStringRef( str ) );
}

bool ycStringRef::BeginsWithI( const char* str, const ycSize_t len ) const
{
	return BeginsWithI( ycStringRef( str, len ) );
}

bool ycStringRef::BeginsWithI( const ycStringRef& str ) const
{
	if( m_len >= str.Length() ) { return Left( str.Length() ).EqualsI( str ); }
	return false;
}

// EndsWith

bool ycStringRef::EndsWith( const char* str ) const
{
	return EndsWith( ycStringRef( str ) );
}

bool ycStringRef::EndsWith( const char* str, const ycSize_t len ) const
{
	return EndsWith( ycStringRef( str, len ) );
}

bool ycStringRef::EndsWith( const ycStringRef& str ) const
{
	const ycSize_t matchLen = str.Length();
	if( Length() >= matchLen ) { return Right( matchLen ).Equals( str ); }
	return false;
}

// EndsWithI

bool ycStringRef::EndsWithI( const char* str ) const
{
	return EndsWithI( ycStringRef( str ) );
}

bool ycStringRef::EndsWithI( const char* str, const ycSize_t len ) const
{
	return EndsWithI( ycStringRef( str, len ) );
}

bool ycStringRef::EndsWithI( const ycStringRef& str ) const
{
	const ycSize_t matchLen = str.Length();
	if( Length() >= matchLen ) { return Right( matchLen ).EqualsI( str ); }
	return false;
}

bool ycStringRef::Contains( const ycStringRef& contains ) const
{
	ycStringRef compare( *this );
	while( compare.m_len )
	{
		if( compare.BeginsWith( contains ) )
		{
			return true;
		}
		compare.Advance();
	}
	return false;
}

bool ycStringRef::Contains( const char* contains ) const
{
	ycAssert( contains != nullptr );
	return Contains( ycStringRef( contains ) );
}
	
bool ycStringRef::ContainsI( const ycStringRef& contains ) const
{
	ycStringRef compare( *this );
	while( compare.m_len )
	{
		if( compare.BeginsWithI( contains ) )
		{
			return true;
		}
		compare.Advance();
	}
	return false;
}
	
bool ycStringRef::ContainsI( const char* contains ) const
{
	ycAssert( contains != nullptr );
	return ContainsI( ycStringRef( contains ) );
}
	

s32 ycStringRef::Find( char c ) const
{
	const char *scan = m_str;
	ycSize_t left = m_len;
	while( left )
	{
		if( c == *scan++ ) { return ( s32 )( m_len-left ); }
		--left;
	}
	return -1;
}

s32 ycStringRef::FindLast( char c ) const
{
	const char *scan = m_str + m_len;
	ycSize_t left = m_len;
	while( left )
	{
		--left;
		if( c == *--scan ) { return ( s32 )( left ); }
	}
	return -1;
}

s32 ycStringRef::Find( char c, char d ) const
{
	const char *scan = m_str;
	ycSize_t left = m_len;
	while( left )
	{
		if( d == *scan || c == *scan ) { return ( s32 )( m_len-left ); }
		++scan;
		--left;
	}
	return -1;
}

s32 ycStringRef::FindLast( const char* str ) const
{
	const char *scan = m_str + m_len;
	ycSize_t left = m_len;
	while( left )
	{
		--left;
		--scan;
		if( ycStringUtils::BeginsWith( scan, str ) ) { return ( s32 )( left ); }
	}
	return -1;
}

s32 ycStringRef::FindLast( ycStringRef str ) const
{
	const char *scan = m_str + m_len;
	ycSize_t left = m_len;
	while( left )
	{
		--left;
		--scan;
		if( ycStringUtils::BeginsWith( scan, str.GetPtr() ) ) { return ( s32 )( left ); }
	}
	return -1;
}

s32 ycStringRef::FindLast( char c, char d ) const
{
	const char *scan = m_str + m_len;
	ycSize_t left = m_len;
	while( left )
	{
		--left;
		char e = *--scan;
		if( d == e || c == e ) { return ( s32 )( left ); }
	}
	return -1;
}

s32 ycStringRef::FindAt( char c, ycSize_t pos ) const
{
	if( pos < m_len )
	{
		const char *scan = m_str + pos;
		ycSize_t left = m_len - pos;
		while( left )
		{
			if( c == *scan ) { return ( s32 )( m_len-left ); }
			++scan;
			--left;
		}
	}
	return -1;
}

s32 ycStringRef::FindAt(const char *str, ycSize_t pos) const
{
	if( !m_len || pos >= m_len ) { return -1; }

	u8 c = /*ycStringUtils::ToLower*/( *str++ );
	if( !c ) { return 0; }

	const char *scan = m_str + pos;
	const char *compare = str;

	ycSize_t left = m_len - pos;
	while( left ) 
	{
		if( /*ycStringUtils::ToLower*/( *scan++ ) == c ) 
		{
			bool equal = true;
			const char *scan_chk = scan;
			while( u8 c2 = *compare++ ) 
			{
				if( /*ycStringUtils::ToLower*/( *scan_chk++ ) != /*ycStringUtils::ToLower*/( c2 ) )
				{
					compare = str;
					equal = false;
					break;
				}
			}
			if( equal ) { return s32( m_len - left ); }
		}
		left--;
	}
	return -1;
}


s32 ycStringRef::FindAtI(const char *str, ycSize_t pos) const
{
	if( !m_len || pos >= m_len ) { return -1; }

	u8 c = ycStringUtils::ToLower( *str++ );
	if( !c ) { return 0; }

	const char *scan = m_str + pos;
	const char *compare = str;

	ycSize_t left = m_len - pos;
	while( left ) 
	{
		if( ycStringUtils::ToLower( *scan++ ) == c ) 
		{
			bool equal = true;
			const char *scan_chk = scan;
			while( u8 c2 = *compare++ ) 
			{
				if( ycStringUtils::ToLower( *scan_chk++ ) != ycStringUtils::ToLower( c2 ) )
				{
					compare = str;
					equal = false;
					break;
				}
			}
			if( equal ) { return s32( m_len - left ); }
		}
		left--;
	}
	return -1;
}

s32 ycStringRef::FindAtI( ycStringRef str, ycSize_t pos ) const
{
	if( ( str.m_len + pos ) > m_len ) { return -1; }
	if( str.m_len == 0 ) { return 0; }
	
	u8 c = ycStringUtils::ToLower( str.m_str[0] );
	if( !c ) { return 0; }

	const char *scan = m_str + pos;

	ycSize_t left = m_len - pos;
	while( left >= str.m_len ) {
		if( ycStringUtils::ToLower( *scan++ ) == c ) {
			const char *scan_chk = scan;
			const char *compare = str.m_str + 1;
			ycSize_t cmplen = str.m_len-1;
			while( cmplen && ycStringUtils::ToLower( *scan_chk++ ) == ycStringUtils::ToLower( *compare++ ) )
			{
				--cmplen;
			}
			if(!cmplen) { return s32( m_len - left ); }
		}
		left--;
	}
	return -1;
}

s32 ycStringRef::FindAt( ycStringRef str, ycSize_t pos ) const
{
	if( ( str.m_len + pos ) > m_len ) { return -1; }
	if( str.m_len == 0 ) { return 0; }
	
	u8 c = /*ycStringUtils::ToLower*/( str.m_str[0] );
	if( !c ) { return 0; }

	const char *scan = m_str + pos;

	ycSize_t left = m_len - pos;
	while( left >= str.m_len ) {
		if( /*ycStringUtils::ToLower*/( *scan++ ) == c ) {
			const char *scan_chk = scan;
			const char *compare = str.m_str + 1;
			ycSize_t cmplen = str.m_len-1;
			while( cmplen && /*ycStringUtils::ToLower*/( *scan_chk++ ) == /*ycStringUtils::ToLower*/( *compare++ ) )
			{
				--cmplen;
			}
			if(!cmplen) { return s32( m_len - left ); }
		}
		left--;
	}
	return -1;
}

s32 ycStringRef::FindAnyOf( ycStringRef set, ycSize_t offset ) const
{
	if( offset >= m_len )
		return -1;

	const char *scan = m_str + offset;
	ycSize_t left = m_len - offset;

	const char *rng = set.m_str;
	ycSize_t count = set.m_len;

	while( left )
	{
		char c = *scan++;
		const char *rng_chk = rng;
		for( ycSize_t n = count; n; --n )
		{
			if( c == *rng_chk++ )
			{
				return s32( m_len - left );
			}
		}
		left--;
	}
	return -1;
}

ycSize_t ycStringRef::LineEndPos( ycSize_t pos ) const
{
	if( pos > m_len ) { return m_len; }
	const char* str = m_str + pos;
	ycSize_t left = m_len - pos;
	while( left )
	{
		if( *str == 0x0a || *str == 0x0d ) { return str - m_str; }
		++str;
		--left;
	}
	return m_len;
}

ycSize_t ycStringRef::LineStartPos( ycSize_t pos ) const
{
	if( pos > m_len ) { return m_len; }
	const char* str = m_str + pos;
	ycSize_t left = pos;
	while( left )
	{
		char c = str[-1];
		if( c == 0x0a || c == 0x0d ) { break; }
		++str;
		--left;
	}
	return left;

}

ycSize_t ycStringRef::LinePrevPos( ycSize_t pos ) const
{
	pos = LineStartPos( pos );
	if( pos )
	{
		--pos;
		if( pos )
		{
			char prevChar = m_str[pos-1];
			if( *m_str != prevChar && ( prevChar == 0x0a || prevChar == 0x0d ) ) { --pos; }
		}
	}
	return pos;
}

ycSize_t ycStringRef::LineNextPos( ycSize_t pos ) const
{
	pos = LineEndPos( pos );
	if( pos < m_len )
	{
		char r1 = m_str[pos];
		++pos;
		if( pos < m_len && r1 != m_str[pos] && ( m_str[pos] == 0x0a || m_str[pos] == 0x0d ) ) { ++pos; }
	}
	return pos;
}

ycSize_t ycStringRef::LineNumber( ycSize_t pos ) const
{
	ycSize_t left = pos < m_len ? pos : m_len;
	const char* str = m_str;
	ycSize_t lineNumber = 0;

	while( left )
	{
		char c = *str++;
		--left;
		if( c == 0x0a || c==0x0d )
		{
			++lineNumber;
			if( left && *str != c && ( *str == 0x0a || *str == 0x0d ) )
			{
				++str;
				--left;
			}
		}
	}
	return lineNumber;
}

bool ycStringRef::SkipFirstIf( char first )
{
	if( m_len && m_str[ 0 ] == first )
	{
		--m_len;
		++m_str;
		return true;
	}
	return false;
}



// convert escape codes to characters
// supports: \a, \b, \f, \n, \r, \t, \v, \000, \x00
// any other character is returned as same
ycSize_t ycStringRef::EscapeCode(u8 &code) const
{
	if( m_len==0 ) { return 0; }

	ycSize_t step = 0;
	const u8* buf = (const u8*)m_str;
	ycSize_t left = m_len;

	uint8_t c = *buf++;
	left--;
	step++;
	// \x??
	if( c=='x' && left && ( ( c>='0' && c<='9' ) || ( c>='a' && c<='f' ) || ( c>='A' && c<='F' ) ) )
	{
		// parse hex char code
		c = 0;
		for( s32 r = 0; r<2 && left; r++ )
		{
			u8 n = *buf++;
			if( ! ( ( c>='0' && c<='9' ) || ( c>='a' && c<='f' ) || ( c>='A' && c<='F' ) ) ) { break; }
			c = (uint8_t)((c<<4) + n - (n<='9' ? '0' : (n<='F'?('A'-0xA) : ('a'-0xa))));
			step++;
			left--;
		}
	}
	else if( c>='0' && c<='7' ) // \02
	{
		// parse octal char code
		c -= '0';
		for( s32 r = 0; r<2 && left; r++ )
		{
			u8 n = *buf++;
			if( n<'0' || n>'7' )
				break;
			c = (uint8_t)(c*8 + n-'0');
			step++;
			left--;
		}
	}
	else
	{	// check for custom escape code symbol
		switch (c)
		{
			case 'a': c = 7; break;		// \a
			case 'b': c = 8; break;		// \b
			case 'f': c = 12; break;	// \f
			case 'n': c = 10; break;	// \n
			case 'r': c = 13; break;	// \r
			case 't': c = 9; break;		// \t
			case 'v': c = 11; break;	// \v
		}
	}
	code = c;
	return step;
}

// The number of characters in a string with escape notification
ycSize_t ycStringRef::LengthEsc()
{
	ycSize_t l = 0, left = m_len;
	const char *s = m_str;
	while( left )
	{
		left--;
		if( *s=='\\' && left )
		{
			left--;
			s++;
		}
		s++;
		l++;
	}
	return l;
}

// allow escape codes in search string
bool ycStringRef::EqualsEsc(const ycStringRef str, ycSize_t pos) const
{
	if( pos >= m_len ) { return false; }

	const u8 *scan = GetBytes() + pos;
	ycStringRef compare = str;
	while( compare ) {
		char c = compare.TakeFirstEsc();
		if( *scan++ != c ) { return false; }
	}
	return true;
}



u8 ycStringRef::TakeFirstEsc()
{
	if( !m_len ) { return 0; }
	if( m_str[ 0 ] == '\\')
	{
		u8 r;
		ycSize_t skip = EscapeCode( r );
		m_len -= skip;
		m_str += skip;
		return r;
	}
	return TakeFirst();
}

// checks if character c matches range
// NOTE: does not check inverse range ([!...])
bool ycStringRef::CharInRange( char c ) const
{
	const u8* rng_chk = (const u8*)m_str;
	ycSize_t rng_lft = m_len;

	// no match yet, check skipped character against allowed range
	bool match = false;
	while( rng_lft ) 
	{
		u8 m = *rng_chk++;
		rng_lft--;
		// escape code?
		if( m == '\\' && rng_lft ) 
		{
			ycSize_t skip = ycStringRef( (const char*)rng_chk, rng_lft ).EscapeCode( m );
			rng_chk += skip;
			rng_lft -= skip;
		}
		// range?
		if( rng_lft>1 && *rng_chk == '-' )
		{
			rng_chk++;
			rng_lft--;
			uint8_t n = (uint8_t)*rng_chk++;
			rng_lft--;
			// escape code for range end?
			if( n == '\\' && rng_lft )
			{
				ycSize_t skip = ycStringRef( (const char*)rng_chk, rng_lft ).EscapeCode( n );
				rng_chk += skip;
				rng_lft -= skip;
			}
			if( c >= m && c <= n )
			{
				match = true;
				break;
			}
		}
		else if( c == m )
		{
			match = true;
			break;
		}
	}
	return match;
}

// checks if character c matches range
// NOTE: does check inverse range ([!...])
bool ycStringRef::CharInRangeEx( char c ) const
{
	if( First() == '!' )
	{
		return !ycStringRef( m_str+1, m_len-1 ).CharInRange( c );
	}
	return CharInRange( c );
}

// Find first instance of a ranged character in a string
s32 ycStringRef::FindRangeChar( const ycStringRef range, bool include ) const
{
	ycStringRef scan = *this;
	s32 index = 0;
	while( scan )
	{
		bool match = range.CharInRange( scan.TakeFirstEsc() );
		if( match == include ) { return index; }
		++index;
	}
	return -1;
}

// find a character matching a range, allow a range of characters using '-'
s32 ycStringRef::FindRangeCharEx( ycStringRef range, ycSize_t pos ) const
{
	if( pos >= m_len ) { return -1; }
	bool include = !range.SkipFirstIf( '!' );
	ycStringRef search( m_str + pos, m_len - pos );
	s32 found = search.FindRangeChar( range, include );
	return found < 0 ? found : ( found + s32( pos ) );
}

// find case sensitive allow escape codes (\x => x) in search string
s32 ycStringRef::FindWithEsc( const ycStringRef str, ycSize_t pos ) const
{
	if( !str.m_len || !m_len || pos>=m_len ) { return -1; }

	// start scan buffer pointers
	const u8 *scan = GetBytes() + pos;

	ycStringRef compare = str;

	// number of characters left in each buffer
	ycSize_t scan_left = m_len - pos;

	// get first character
	u8 c = compare.TakeFirstEsc();

	// sweep the scan buffer for the matching string
	while( scan_left )
	{
		if( *scan++ == c )
		{
			const u8 *chk_scan = scan;
			ycStringRef chk_compare = compare;
			while( chk_compare ) {
				if( *chk_scan++ != chk_compare.TakeFirstEsc() )
				{
					chk_compare.m_len = 1;
					break;
				}
			}
			if( !chk_compare )
			{
				return s32( m_len - scan_left );
			}
		}
		scan_left--;
	}
	return -1;
}

// find case sensitive allow escape codes (\x => x) in search string
s32 ycStringRef::FindWithEscI(const ycStringRef str, ycSize_t pos) const
{
	if( !str.m_len || !m_len || pos>=m_len ) { return -1; }

	// start scan buffer pointers
	const u8 *scan = GetBytes() + pos;

	ycStringRef compare = str;

	// number of characters left in each buffer
	ycSize_t scan_left = m_len - pos;

	// get first character
	u8 c = ycStringUtils::ToLower( compare.TakeFirstEsc() );

	// sweep the scan buffer for the matching string
	while( scan_left )
	{
		if( ycStringUtils::ToLower( *scan++ ) == c )
		{
			const u8 *chk_scan = scan;
			ycStringRef chk_compare = compare;
			while( chk_compare )
			{
				if( ycStringUtils::ToLower( *chk_scan++ ) != ycStringUtils::ToLower( chk_compare.TakeFirstEsc() ) )
				{
					chk_compare.m_len = 1;
					break;
				}
			}
			if( !chk_compare )
			{
				return s32( m_len - scan_left );
			}
		}
		scan_left--;
	}
	return -1;
}

// find case sensitive allow escape codes (\x => x) in search string
s32 ycStringRef::FindRangeWithEsc(const ycStringRef str, ycStringRef range, ycSize_t pos) const
{
	if( !str || !m_len || pos>=m_len || !range.m_len ) { return -1; }

	// start scan buffer pointers
	const u8 *scan = GetBytes() + pos;

	ycStringRef compare = str;

	// number of characters left in each buffer
	ycSize_t scan_left = m_len - pos;

	// get first character
	u8 c = compare.TakeFirstEsc();

	// check if range is inclusive or exclusive
	bool include = !range.SkipFirstIf( '!' );

	// sweep the scan buffer for the matching string
	while( scan_left ) 
	{
		u8 b = (u8)*scan++;
		// check for string match
		if( b == c ) 
		{
			ycStringRef chk( ( const char* )scan, scan_left );
			ycStringRef chk_compare = compare;
			while(chk_compare) 
			{
				u8 d = chk_compare.TakeFirstEsc();
				u8 e = chk.TakeFirst();
				if( e!=d )
				{
					chk_compare.m_len = 1;
					break;
				}
			}
			if( !chk_compare ) { return s32(m_len - scan_left); }
		}

		// no match yet, check character against range
		// check if character is allowed
		bool match = range.CharInRange( b );
		if( (match && !include) || (!match && include) )
			return -1;

		scan_left--;
	}
	return -1;
}

// find substring, allow escape codes (\x => x) in search string
s32 ycStringRef::FindRangeWithEscI(const ycStringRef str, ycStringRef range, ycSize_t pos) const
{
	if( !str || !m_len || pos>=m_len || !range.m_len ) { return -1; }

	// start scan buffer pointers
	const u8 *scan = GetBytes() + pos;

	ycStringRef compare = str;

	// number of characters left in each buffer
	ycSize_t scan_left = m_len - pos;

	// get first character
	u8 c = ycStringUtils::ToLower( compare.TakeFirstEsc() );

	// check if range is inclusive or exclusive
	bool include = !range.SkipFirstIf( '!' );

	// sweep the scan buffer for the matching string
	while( scan_left ) 
	{
		u8 b = (u8)ycStringUtils::ToLower( *scan++ );
		// check for string match
		if( b == c )
		{
			ycStringRef chk( ( const char* )scan, scan_left );
			ycStringRef chk_compare = compare;
			while( chk_compare ) 
			{
				u8 d = chk_compare.TakeFirstEsc();
				u8 e = chk.TakeFirst();
				if( ycStringUtils::ToLower( e ) != ycStringUtils::ToLower( d ) )
				{
					chk_compare.m_len = 1;
					break;
				}
			}
			if( !chk_compare ) { return s32( m_len - scan_left ); }
		}

		// no match yet, check character against range
		// check if character is allowed
		bool match = range.CharInRange( b );
		if((match && !include) || (!match && include))
			return -1;

		scan_left--;
	}
	return -1;
}

// search of a character in a given range while also checking that
// skipped characters are in another given range.
s32 ycStringRef::FindRangeWithinRangeOf( ycStringRef range_find, ycStringRef range_within, ycSize_t pos ) const
{
	if( pos >= m_len )
		return -1;

	const char *scan = GetPtr() + pos;
	ycSize_t left = m_len - pos;

	// check if range is inclusive or exclusive
	bool include_find = !range_find.SkipFirstIf( '!' );

	bool include_within = !range_within.SkipFirstIf( '!' );

	while( left )
	{
		char c = *scan++;
		bool match_find = range_find.CharInRange( c );
		if( (match_find && include_find) || (!match_find && !include_find) )
		{
			return s32( m_len - left );
		}

		// no match yet, check skipped character against allowed range
		bool match = range_within.CharInRange( c );

		// check if character is allowed
		if( (match && !include_within) || (!match && include_within) )
		{
			return -1;
		}
		--left;
	}
	return -1;
}


ycSize_t ycStringRef::MatchRangeLength( const ycStringRef range, ycSize_t pos ) const
{
	if( pos >= m_len ) { return 0; }

	bool match = range.m_len == 0 || range.m_str[0] != '!';
	const ycStringRef rng = match ? range : ycStringRef( range.m_str+1, range.m_len-1 );

	const char *scan = m_str + pos;
	ycSize_t left = m_len - pos;
	while( left )
	{
		if( rng.CharInRange( *scan++ ) != match ) { return m_len - left - pos; }
		--left;
	}
	return m_len;
}

#define MAX_FILTER_CHARS 128
ycSize_t ycStringRef::FuzzyScore( const ycStringRef filter )
{
	const ycStringRef & text = *this;
	ycBitField< MAX_FILTER_CHARS > used;
	used.Clear();

	ycSize_t totalScore = 0;
	ycSize_t filter_len = filter.Length();
	ycSize_t text_len = text.Length();
	const char* str = text.GetPtr();
	for( ;; )
	{
		bool has_filter = false;
		char filter_char = 0;
		ycSize_t filter_index = 0;
		for( ycSize_t f = 0; f<filter_len; ++f )
		{
			if( !used.Check( f ) )
			{
				filter_char = ycStringUtils::ToLower( filter[ f ] );
				filter_index = f;
				used.Set( f );
				has_filter = true;
				break;
			}
		}

		// no chars left in filter, done!
		if( !has_filter ) { break; }

		// scan through the string for best score using the filter_char
		ycSize_t bestIndex = ycSize_t(-1);
		ycSize_t bestScore = 0;
		ycSize_t bestCount = 0;

		ycSize_t wordStart = 1;
		ycSize_t textStart = 1;

		for( ycSize_t index = 0; index < text_len; ++index )
		{
			char string_char = ycStringUtils::ToLower( str[ index ] );
			if( string_char == filter_char )
			{
				// how many characters remain?
				ycSize_t remain = text_len - index;
				if( remain > ( filter_len - filter_index ) ) { remain = filter_len - filter_index; }

				ycSize_t match_length = 1;
				for( ycSize_t match = 1; match < remain; ++match )
				{
					ycSize_t filter_match = filter_index + match;
					ycSize_t text_match = index + match;
					if( used.Check( filter_match ) ||
						ycStringUtils::ToLower( filter[ filter_match ] ) != ycStringUtils::ToLower( str[ text_match ] ) )
					{
						break;
					}
					match_length++;
				}

				ycSize_t wordEnd = (index == (filter_len-1)) || ycStringUtils::IsSeparator( str[ index+1 ] );

				ycSize_t score = ( ( textStart + wordStart ) * 10 + wordEnd * 5 + match_length ) * match_length;
				if( score > bestScore )
				{
					bestIndex = index;
					bestScore = score;
					bestCount = match_length;
				}
				textStart = 0;
				wordStart = ycStringUtils::IsWhiteSpace( str[ index ] );
			}
		}

		if( bestIndex != ycSize_t(-1) && bestScore )
		{
			totalScore += bestScore;
			for( ycSize_t match = 1; match < bestCount; ++match ) { used.Set( match + filter_index ); }
		}
	}

	return totalScore;
}

ycStringRef ycStringRef::Split( ycSize_t pos )
{
	if( pos < m_len )
	{
		ycStringRef ret( m_str, pos );
		m_str += pos;
		m_len -= pos;
		return ret;
	}
	else
	{
		ycStringRef ret = *this;
		Clear();
		return ret;
	}
}

ycStringRef ycStringRef::Line()
{
	const char *scan = m_str;
	ycSize_t left = m_len;

	while( left && *scan!=0x0a && *scan!=0x0d )
	{
		scan++;
		left--;
	}
	ycStringRef ret( m_str, m_len - left );
	if( left )
	{
		char c = *scan++;
		left--;
		if( left && ((c==0x0a && *scan==0x0d) || (c==0x0d && *scan==0x0a)) )
		{
			scan++;
			left--;
		}
	}
	m_str = (char*)scan;
	m_len = left;

	return ret;
}

ycStringRef ycStringRef::SubStr( ycSize_t offs, ycSize_t len ) const
{
	if( offs < m_len )
	{
		const ycSize_t left = m_len - offs;
		return ycStringRef( m_str + offs, len < left ? len : left );
	}
	return ycStringRef();
}

ycStringRef ycStringRef::ClipTrail( ycSize_t len ) const
{
	if( len < m_len )
	{
		return ycStringRef( m_str, m_len - len );
	}
	return ycStringRef();
}

ycStringRef ycStringRef::Skip( ycSize_t skip ) const
{
	if( skip < m_len )
	{
		return ycStringRef( m_str + skip, m_len - skip );
	}
	return ycStringRef();
}

ycStringRef ycStringRef::SkipBOM() const
{
	const u8* buf = (const u8*)m_str;
	if( m_len >= 3 && buf[ 0 ] == 0xef && buf[ 1 ] == 0xbb && buf[ 2 ] == 0xbf )
	{
		return ycStringRef( m_str + 3, m_len - 3 );
	}
	return *this;
}

ycStringRef ycStringRef::BetweenLast( char open, char close ) const
{
	ycSize_t end = m_len;
	ycSize_t begin = 0;

	s32 lastClose = FindLast( close );
	if( lastClose >= 0 ) { end = (ycSize_t)lastClose; }
	s32 lastOpen = ycStringRef( m_str, end ).FindLast( open );
	if( lastOpen >= 0) { begin = (ycSize_t)lastOpen + 1; }

	return ycStringRef( m_str + begin, end - begin );
}

ycStringRef ycStringRef::BetweenLast( char open, char openAlt, char close ) const
{
	ycSize_t end = m_len;
	ycSize_t begin = 0;

	s32 lastClose = FindLast( close );
	if( lastClose >= 0 ) { end = (ycSize_t)lastClose; }
	ycStringRef left( m_str, end );
	s32 lastOpen = left.FindLast( open );
	s32 lastOpenAlt = left.FindLast( openAlt );
	lastOpen = lastOpen > lastOpenAlt ? lastOpen : lastOpenAlt;
	if( lastOpen >= 0) { begin = (ycSize_t)lastOpen + 1; }
	return ycStringRef( m_str + begin, end - begin );
}

ycSize_t ycStringRef::WhiteLen( ycSize_t pos ) const
{
	if( pos >= m_len ) { return 0; }
	ycSize_t left = m_len - pos;
	const char* scan = m_str + pos;
	while( left )
	{
		if( u8( *scan++ ) > ' ' ) { break; }
		--left;
	}
	return m_len - left - pos;
}

ycSize_t ycStringRef::WhiteTrailLen() const
{
	ycSize_t left = m_len;
	const char* scan = m_str + left;
	while( left )
	{
		if( u8( *--scan ) > ' ' ) { break; }
		--left;
	}
	return m_len - left;
}


ycSize_t ycStringRef::LengthSeparator( ycSize_t pos ) const
{
	ycSize_t origPos = pos;
	while( pos < m_len && ycStringUtils::IsSeparator( m_str[ pos ] ) ) { ++pos; }
	return pos - origPos;
}

ycSize_t ycStringRef::LengthNonSeparator( ycSize_t pos ) const
{
	ycSize_t origPos = pos;
	while( pos < m_len && !ycStringUtils::IsSeparator( m_str[ pos ] ) ) { ++pos; }
	return pos - origPos;
}

u32 ycStringRef::HashIgnoreWhitespace() const
{
	u32 hash = 2166136261;
	u8* scan = ( u8* )m_str;
	ycSize_t left = m_len;
	while( left-- ) 
	{
		uint8_t c = *scan++;
		if( c<' ' ) { c = ' '; }
		hash = (c ^ hash) * 16777619;
		if( c==' ' )
		{
			while( left && *scan<=0x20 )
			{
				left--;
				scan++;
			}
		}
	}
	return hash;
}

ycSize_t ycStringRef::CommentLength( ycSize_t pos ) const
{
	pos += WhiteLen( pos );
	if( pos >= m_len ) { return 0; }

	const char* scan = m_str + pos;
	ycSize_t left = m_len - pos;

	char c = *scan++;
	left--;
	if( c == '/' && left && (*scan == '/' || *scan == '*'))
	{	// comment
		c = *scan++;
		left--;
		if( c == '/' )
		{ // line comment
			while( left )
			{
				c = *scan++;
				--left;
				if( c == 0x0a || c==0x0d )
				{
					if( *scan != c && (*scan == 0x0a || *scan == 0x0d ) )
					{
						scan++;
						left--;
					}
					return m_len - left - pos;
				}
			}
		}
		else
		{ /* block comment */
			while( left )
			{
				c = *scan++;
				--left;
				if( left && c == '*' && *scan =='/' )
				{
					++scan;
					--left;
					return m_len - left - pos;
				}
			}
		}
	}
	return 0;
}

ycSize_t ycStringRef::ScopeLengthCommented( ycSize_t pos ) const
{
	pos += WhiteLen( pos );
	if( pos >= m_len ) { return 0; }

	const char* scan = m_str + pos;
	ycSize_t left = m_len - pos;
	if( left )
	{
		char open = *scan++, close = 0;
		left--;
		switch( open )
		{
			case '(' : close = ')'; break;
			case '[' : close = ']'; break;
			case '{' : close = '}'; break;
			case '<' : close = '>'; break;
			default: return 0; // invalid scope opening character
		}
		ycSize_t depth = 1;
		do
		{
			char c = *scan++;
			left--;
			if( left && c == '/' && left && ( *scan == '/' || *scan == '*' ) )
			{	// comment
				if( ycSize_t skip = ycStringRef( scan - 1, left + 1 ).CommentLength() )
				{
					scan += skip - 1;
					left -= skip - 1;
				}
			}
			else if( c == open ) { depth++; }
			else if( c == close ) { depth--; }
		} while( depth && left );
		if( !depth ) { return ycSize_t( scan - m_str - pos ); }
	}
	return 0;
}

ycStringRef ycStringRef::SplitWordOrQuote()
{
	SkipWS();
	if( m_len > 0 )
	{
		const u8* str = ( const u8* )m_str;
		ycSize_t left = m_len;
		if( *str == '"' )
		{
			++str;
			--left;
			while( left && *str != '"' )
			{
				++str;
				--left;
				if( left >= 2 && *str == '\\' && str[1] == '"' )
				{
					str += 2;
					left -= 2;
				}
			}
			if( left && *str == '"' ) { ++str; --left; }
			return Split( (const char*)str - m_str );
		}
		const u8* ret_str = str;
		while( left )
		{
			char c = *str;
			if( !( ( c>='0' && c<='9' ) || ( c>='A' && c<='Z' ) || ( c>='a' && c<='z' ) || c=='_' ) )
			{
				break;
			}
			++str;
			--left;
		}
		ycStringRef ret( (const char*)ret_str, ycSize_t( str - ret_str ) );
		m_str = (char*)str;
		m_len = left;
		return ret;
	}
	return ycStringRef();
}

// split string based on common programming tokens (words, quotes, scopes, numbers)
ycStringRef ycStringRef::SplitLang()
{
	SkipWS();
	char c = First();
	if( c == '"' )
	{
		return SplitWordOrQuote();
	}
	else if( c == '{' || c=='(' )
	{
		return Split( ScopeLengthCommented() );
	}
	else if( (c >= '0' && c <= '9' ) || c == '-' )
	{
		if( ( c == '0' && m_str[1] == 'x' ) || m_str[1] == 'X' )
		{
			return Split( LengthHex( 2 ) + 2 );
		}
		else
		{
			return Split( LengthFloat() );
		}
	}
	else if( c >='!' && c <='@' )
	{
		return Split( 1 );
	}
	return SplitWordOrQuote();
}


ycSize_t ycStringRef::LengthNumeric( ycSize_t pos ) const
{
	if( pos >= m_len ) { return 0; }
	const char* str = m_str + pos;
	ycSize_t left = m_len - pos;
	while( left )
	{
		u8 c = (u8)*str++;
		if( !( c >= '0' && c <= '9' ) ) { break; }
		--left;
	}
	return m_len - left - pos;
}

ycSize_t ycStringRef::LengthHex( ycSize_t pos ) const
{
	if( pos >= m_len ) { return 0; }
	const char* str = m_str + pos;
	ycSize_t left = m_len - pos;
	ycSize_t len = 0;
	while( left )
	{
		u8 c = (u8)*str++;
		if( !( ( c >= '0' && c <= '9' ) || ( c>= 'a' && c <='f' ) || ( c>='A' && c<='F' ) || (len == 1 && (c == 'X' || c =='x')) ) ) { break; }
		--left;
		len++;
	}
	return len;
}

// determine how many characters can be used for floating point
ycSize_t ycStringRef::LengthFloat( ycSize_t pos ) const
{
	if( pos >= m_len ) { return 0; }
	// skip whitespace
	const char *str = m_str + pos;
	ycSize_t left = m_len - pos;

	// valid check
	if( left==0 ) { return 0; }

	// not a floating point if just spaces and a dot
	bool has_value = false;

	// include whitespace
	ycSize_t ws = WhiteLen();
	str += ws;
	left -= ws;

	// include sign
	if( left && ( *str=='-' || *str=='+' ) )
	{
		str++;
		left--;
		if( !left || !( *str >= '0' && *str <= '9') ) { return 0; }
	}

	// integer portion
	while( left && ( *str>='0' && *str<='9' ) )
	{
		str++;
		left--;
		has_value = true;
	}

	// decimal
	if( left && *str == '.' )
	{
		str++;
		left--;
	}

	// fraction
	while( left && ( *str>='0' && *str<='9' ) )
	{
		str++;
		left--;
		has_value = true;
	}

	// exponent
	if( left && ( *str == 'e' || *str == 'E' ) )
	{
		ycSize_t e = left;
		str++;
		left--;
		if( left && ( *str == '-' || *str == '+' ) ) {
			str++;
			left--;
		}
		if( !left || !( *str >= '0' && *str <= '9' ) ) { return m_len - e - pos; }

		while( left && ( *str >= '0' && *str <= '9' ) )
		{
			str++;
			left--;
			has_value = true;		// e-10 is a fine floating point number
		}
	}
	// return size of floating point number
	return has_value ? m_len - left - pos : 0;
}

char ycStringRef::TakeFirst()
{
	if( m_len )
	{
		char c = *m_str;
		++m_str;
		--m_len;
		return c;
	}
	return 0;
}

bool ycStringRef::Take( const char ch )
{
	if( m_len && *m_str == ch ) { ++m_str; --m_len; return true; }
	return false;
}

bool ycStringRef::Take( const char* str ) { return Take( ycStringRef( str ) ); }
bool ycStringRef::Take( const char* str, const ycSize_t len ) { return Take( ycStringRef( str, len ) ); }
bool ycStringRef::Take( const ycStringRef str )
{
	if( BeginsWith( str ) ) { m_str += str.Length(); m_len -= str.Length(); return true; }
	return false;
}

bool ycStringRef::TakeI( const ycStringRef str )
{
	if( BeginsWithI( str ) ) { m_str += str.Length(); m_len -= str.Length(); return true; }
	return false;
}
char ycStringRef::TakeLast()
{
	if( m_len ) { return m_str[ --m_len ]; }
	return 0;
}

bool ycStringRef::TakeEnd( const char ch )
{
	if( m_len && m_str[ m_len-1 ] == ch ) { --m_len; return true; }
	return false;
}

bool ycStringRef::TakeEnd( const char* str ) { return TakeEnd( ycStringRef( str ) ); }
bool ycStringRef::TakeEnd( const char* str, const ycSize_t len ) { return TakeEnd( ycStringRef( str, len ) ); }
bool ycStringRef::TakeEnd( const ycStringRef str )
{
	if( EndsWith( str ) ) { m_len -= str.Length(); return true; }
	return false;
}

bool ycStringRef::TakeEndI( const char* str ) { return TakeEndI( ycStringRef( str ) ); }
bool ycStringRef::TakeEndI( const char* str, const ycSize_t len ) { return TakeEndI( ycStringRef( str, len ) ); }
bool ycStringRef::TakeEndI( const ycStringRef str )
{
	if( EndsWithI( str ) ) { m_len -= str.Length(); return true; }
	return false;
}

ycStringRef ycStringRef::TokenSplit( char token )
{
	const char *t = m_str;
	ycStringRef ret;
	bool found = false;
	for( ycSize_t s = 0; s < m_len; ++s )
	{
		if( *t++ == token )
		{
			ret.m_str = m_str;
			ret.m_len = s;
			m_str = (char*)t;
			m_len -= s+1;
			found = true;
			break;
		}
	}
	if( !found )
	{
		ret = *this;
		Clear();
	}
	while( ret.m_len && *(u8*)ret.m_str <= (u8)' ' )
	{
		ret.m_str++;
		ret.m_len--;
	}
	while( ret.m_len && ((u8*)ret.m_str)[ret.m_len-1] <= (u8)' ' )
	{
		ret.m_len--;
	}
	return ret;
}

ycVector< ycStringRef > ycStringRef::SplitList( const char * findStr ) const
{
	ycVector< ycStringRef > list;
	s32 findIndex;
	ycSize_t copyIndex = 0;
	ycSize_t length = ycStringUtils::Length( findStr );
	while( (findIndex = FindAt( findStr, copyIndex )) >= 0 )
	{
		ycStringRef str;
		str.Set( &Get()[copyIndex], findIndex-copyIndex );
		list.Append( str );
		copyIndex = findIndex + length;
	}
	if( copyIndex < Length() )
	{
		ycStringRef str;
		str.Set( &Get()[copyIndex], Length()-copyIndex );
		list.Append( str );
	}
	return list;
}

ycHashKey_t ycStringRef::Hash() const
{
	return ycHashBytes( m_str, m_len );
}

u32 ycStringRef::Hash32() const
{
	const u32 h = ycHash32( m_str, m_len );
	ycAssertMsg( h != 0, "The string '%.*s' is too dangerous to live.", m_len, m_str );
	return h;
}

u32 ycStringRef::Hash32EmptyZero() const
{
	return IsEmpty() ? 0 : ycHash32( m_str, m_len );
}

u64 ycStringRef::Hash64EmptyZero() const
{
	return IsEmpty() ? 0 : ycHash64( m_str, m_len );
}

u64 ycStringRef::Hash64() const
{
	return ycHash64( m_str, m_len );
}

bool ycStringRef::ToBool() const
{
	// TODO HACK FIXME: should probably just try integral/float conversions instead of trying to catch all values? but float conversions dont currently have a way to "attempt"!
	if( EqualsI( "false", 5 ) || Equals( "0" ) || Equals( "0.0" ) || Equals( "-0.0" ) )
	{
		return false;
	}
	return true;
}
	
s64 ycStringRef::ToInt( bool* converted ) const
{
	const ycSize_t len = Length();
	if( converted ) { *converted = false; }
	if( len )
	{
		const u8* s = GetBytes();
		const u8* e = s + len;
		while( s!=e && *s<=0x20 ) { ++s; }
		if( s < e )
		{
			s64 v = 0;
			const bool neg = (*s == '-');
			if( neg ) { ++s; }
			while( s != e )
			{
				const u8 c = *s++;
				if( c < '0' || c > '9' ) { break; }
				if( converted ) { *converted = true; }
				v = c-'0' + v*10;
			}
			return neg ? -v : v;
		}
	}
	return 0;
}

s64 ycStringRef::ToIntExt() const
{
	const u8* s = GetBytes();
	ycSize_t l = Length();
	while( l && *s <= 0x20 ) { ++s; --l; }
	if( l >=2 && s[0]=='0' && (s[1] == 'x' || s[1] == 'X') ) { return (s64)ToHexUInt(); }
	return ToInt();
}

u64 ycStringRef::ToUInt( bool* converted ) const
{
	const ycSize_t len = Length();
	if( len )
	{
		const u8* s = GetBytes();
		const u8* e = s + len;
		while( s!=e && *s<=0x20 ) { ++s; }
		if( s < e )
		{
			u64 v = 0;
			while( s!=e )
			{
				const u8 c = *s++;
				if( c < '0' || c > '9' ) { break; }
				v = c-'0' + v*10;
			}
			if( converted ) { *converted = true; }
			return v;
		}
	}
	if( converted ) { *converted = false; }
	return 0;
}

u64 ycStringRef::ToUIntExt() const
{
	const u8* s = GetBytes();
	ycSize_t l = Length();
	while( l && *s <= 0x20 ) { ++s; --l; }
	if( l >=2 && s[0]=='0' && (s[1] == 'x' || s[1] == 'X') ) { return (u64)ToHexUInt(); }
	return ToUInt();
}

u64 ycStringRef::ToHexUInt( bool * converted ) const
{
	const ycSize_t len = Length();
	if( len )
	{
		const u8* s = GetBytes();
		const u8* e = s + len;
		while( s!=e && *s<=0x20 ) { ++s; }
		if( (e-s)>2 && *s=='0' && ( s[1]=='x' || s[1] == 'X' ) ) { s += 2; }

		if( s < e )
		{
			u64 v = 0;
			while( s!=e )
			{
				const u8 c = *s++;
				if( c >= '0' && c <= '9' ) { v <<= 4; v += c-'0'; }
				else if( c >= 'a' && c<='f' ) { v <<= 4; v += c-'a'+10; }
				else if( c >= 'A' && c<='F' ) { v <<= 4; v += c-'A'+10; }
				else { break; }
			}
			if( converted ) { *converted = true; }
			return v;
		}
	}
	if( converted ) { *converted = false; }
	return 0;
}

#define F64_MAXEXP_EXP   308                     // max decimal exponent
static const u32 flt_max_exp = (~0U) / 20;
static const u64 flt_max_int_add_digit = (~0ULL) / 10;

f32 ycStringRef::ToF32() const
{
	return (f32)ToF64();
}

f64 ycStringRef::ToF64() const
{
	bool negative = false;
	bool overflow = false;
	u64 integer_part = 0;
	u64 fractional_part = 0;
	s64 fractional_divide = 1;
	ycSize_t left = m_len;
	u8* scan = ( u8* )m_str;

	while( left && *scan <= ' ' ) { left--; scan++; }	// skip initial spaces
	if( left && *scan == '-' ) { negative = true; left--; scan++; }	// check for negativeative numbers
	else if( left && *scan == '+') { left--; scan++; }			// check for explicitly positive numbers

	// read integer
	while( left && *scan >= '0' && *scan <= '9' )
	{	// for floating point values ignoring overflow digits doesn't matter
		if( integer_part < flt_max_int_add_digit ) { integer_part = integer_part * 10 + (*scan - '0'); }	
		else { overflow = true; }	// for integer values ignoring digits does matter
		left--;
		scan++;
	}

	// read fraction
	if( left && *scan == '.' ) 
	{
		left--;
		scan++;
		while( left && *scan >= '0' && *scan <= '9' )	// count fractional numbers
		{	
			if(fractional_part < ((~0ULL) / 10))		// stop reading more fraction numbers when no more room in word
			{
				fractional_part = fractional_part * 10 + (*scan - '0');
				fractional_divide *= 10;
			}
			left--;
			scan++;
		}
	}

	// check for exponent
	if( left && (*scan == 'e' || *scan == 'E') )
	{
		left--;
		scan++;
		s32 exponent = 0;
		bool negative_exp = false;
		if(left && *scan == '-') { negative_exp = true; left--; scan++; }	// check for +/- sign
		else if(left && *scan == '+') { left--; scan++; }
		while( left && *scan >= '0' && *scan <= '9' ) 
		{ // read numbers
			if((u32)exponent < flt_max_exp) {	exponent = exponent * 10 + (*scan - '0'); } // exp is signed so divide by 2 and 10
			else { overflow = true; }
			left--;
			scan++;
		}
		if( negative_exp ) { exponent = -exponent; } // negativeate exponent if needed

		ycAssert( exponent < F64_MAXEXP_EXP && !overflow );
		if( exponent > F64_MAXEXP_EXP || overflow ) { return 0.0; }

		double ret = ( double(integer_part) + double(fractional_part) / double(fractional_divide) ) * ycPowD( 10.0, exponent );
		return negative ? -ret : ret;
	}

	ycAssert( !overflow );

	// skip pow if no exponent parsed
	double ret = ( double(integer_part) + double(fractional_part) / double(fractional_divide) );
	return negative ? -ret : ret;
}

s32 ycStringRef::Eval( f32* output ) const
{
	struct ResultInfo
	{
		f32 value = 0.0f;
		s32 valid = kEvalResult_Valid;
		bool IsValid()
		{
			return valid == kEvalResult_Valid;
		}
	};
	struct EvalObj
	{
		ycStringRef str;
		ycStringRef strLast;
		s32 ch;
		void Next()
		{
			if( str.Length() > 0 )
			{
				strLast = str;
				ch = str.TakeFirst();
			}
			else
			{
				strLast = str;
				ch = -1;
			}
		};
		bool Consume( s32 _ch )
		{
			while( ycStringUtils::IsWhiteSpace( (char)ch ) ) 
			{
				Next();
			}
			if( ch == _ch )
			{
				Next();
				return true;
			}
			return false;
		};

		ResultInfo ParseFactor()
		{
			if( Consume( '+' ) ) { return ParseFactor(); }
			if( Consume( '-' ) ) 
			{
				ResultInfo negTerm = ParseFactor();
				negTerm.value = -negTerm.value;
				return negTerm;
			}
			ycStringRef start = strLast;
			ResultInfo result;
			if( Consume( '(' ) )
			{
				result = ParseExpression();
				if( !Consume( ')' ) )
				{
					result.valid = kEvalResult_ParentheticalMismatch;
					return result;
				}
			}
			else if( start.SubStr( 0, 2 ).Equals( "PI" ) || start.SubStr( 0, 5 ).Equals( "YC_PI" ) )
			{
				Next();
				result.value = YC_PI;
			}
			else if( (ch >= '0' && ch <= '9') || ch == '.' )
			{
				while(  (ch >= '0' && ch <= '9') || ch == '.' )
				{
					Next();
				}
				ycStringRef num = start.SubStr( 0, start.Length() - strLast.Length() );
				result.value = num.ToF32();
			}
			else if( ch >= 'a' && ch <= 'z' )
			{
				while( ch >= 'a' && ch <= 'z' )
				{
					Next();
				}
				ycStringRef func = start.SubStr( 0, start.Length() - strLast.Length() );
				if( Consume( '(' ) )
				{
					result = ParseExpression();
				}
				else
				{
					result = ParseFactor();
				}
				if( !result.IsValid() )
				{
					return result;
				}
				if( func.EqualsI( "sqrt" ) )
				{
					result.value = ycSqrt( result.value );
				}
				else if( func.EqualsI( "sin" ) )
				{
					result.value = ycSin( result.value );
				} 
				else if( func.EqualsI( "cos" ) )
				{
					result.value = ycCos( result.value );
				} 
				else if( func.EqualsI( "tan" ) )
				{
					result.value = ycTan( result.value );
				} 
				else if( func.EqualsI( "deg" ) )
				{
					result.value = ycDegToRad( result.value );
				} 
				else
				{
					result.valid = kEvalResult_UnexpectedChar;
				}
			}
			else
			{
				result.valid = kEvalResult_UnexpectedChar;
			}
			if( !result.IsValid() )
			{
				return result;
			}
			if( Consume( '^' ) )
			{
				ResultInfo parseFactor = ParseFactor();
				if( !parseFactor.IsValid() )
				{
					return parseFactor;
				}
				result.value = ycPow( result.value, parseFactor.value );
			}
			return result;
		}

		ResultInfo ParseTerm()
		{
			ResultInfo result = ParseFactor();
			if( !result.IsValid() ) 
			{ 
				return result; 
			}
			while( true )
			{
				if( Consume( '*' ) ) 
				{ 
					ResultInfo addTerm = ParseTerm();
					if( !addTerm.IsValid() ) 
					{ 
						return addTerm; 
					}
					result.value *= addTerm.value;
				}
				else if( Consume( '/' ) ) 
				{ 
					ResultInfo addTerm = ParseTerm();
					if( !addTerm.IsValid() ) 
					{ 
						return addTerm; 
					}
					result.value /= addTerm.value;
				}
				else
				{
					return result;
				}
			}
		}

		ResultInfo ParseExpression()
		{
			ResultInfo result = ParseTerm();
			if( !result.IsValid() ) 
			{ 
				return result; 
			}
			while( true )
			{
				if( Consume( '+' ) ) 
				{ 
					ResultInfo addTerm = ParseTerm();
					if( !addTerm.IsValid() ) 
					{ 
						return addTerm; 
					}
					result.value += addTerm.value;
				}
				else if( Consume( '-' ) ) 
				{ 
					ResultInfo addTerm = ParseTerm();
					if( !addTerm.IsValid() ) 
					{ 
						return addTerm; 
					}
					result.value -= addTerm.value;
				}
				else
				{
					return result;
				}
			}
		}

		ResultInfo Parse()
		{
			Next();
			ResultInfo result = ParseExpression();
			if( str.Length() > 0 )
			{
				result.valid = kEvalResult_UnexpectedChar;
			}
			return result;

		};
	};
	EvalObj obj;
	obj.str = *this;
	ResultInfo info = obj.Parse();
	*output = info.value;
	return info.valid;
}

ureg ycStringRef::CleanFloat( ureg max_fraction_digits )
{
	int decimal = Find( '.' );
	if( decimal >= 0 )
	{	// clamp length
		if( Length() > ( decimal + max_fraction_digits + 1 ) )
		{
			m_len = decimal + max_fraction_digits + 1;
		}
		while( Last() == '0' ) { TakeLast(); }
		if( Last() == '.' ) { TakeLast(); }
	}
	return Length();
}

bool ycStringRef::AlphaLessThan( const ycStringRef& other ) const
{
	const char* str1 = m_str;
	const char* str2 = other.m_str;
	ureg left = m_len < other.m_len ? m_len : other.m_len;
	for( ureg i = 0; i < left; ++i )
	{	// a character in this string is less than the corresponding character in the other
		char c1 = *str1++;
		char c2 = *str2++;
		if( c1 < c2 ) { return true; }
		else if( c1 > c2 ) { return false; }
	}	// this string is shorter than the other string
	if( left < other.m_len ) { return true; }
	return false;
}

bool ycStringRef::AlphaLessThanI( const ycStringRef& other ) const
{
	const char* str1 = m_str;
	const char* str2 = other.m_str;
	ureg left = m_len < other.m_len ? m_len : other.m_len;
	for( ureg i = 0; i < left; ++i )
	{	// a character in this string is less than the corresponding character in the other
		char c1 = ycStringUtils::ToLower( *str1++ );
		char c2 = ycStringUtils::ToLower( *str2++ );
		if( c1 < c2 ) { return true; }
		else if( c1 > c2 ) { return false; }
	}	// this string is shorter than the other string
	if( m_len < other.m_len ) { return true; }
	return false;
}

ycStringRef ycStringRef::NextToken( char token )
{
	ycSize_t left = m_len;
	const char* str = m_str;
	if( str )
	{
		while( left )
		{
			--left;
			char c = *str++;
			if( left && c == token )
			{
				ycStringRef ret = TakeLeft( m_len - left - 1);
				TakeFirst();
				return ret.GetClipTrailWS();
			}
		}
		return TakeLeft( m_len - left );
	}
	return ycStringRef();
}

ycStringRef ycStringRef::TakeIdentifier()
{
	const char* str = m_str;
	ycSize_t    len = m_len;

	if( len > 0 && ycStringUtils::IsIdentifierFirstChar( *str ) )
	{
		--len;
		++str;
		while( len && ycStringUtils::IsIdentifier( *str ) ) { ++str; --len; }
		const char* identifierStart = m_str;
		m_str = (char*)str;
		m_len = len;
		return ycStringRef( identifierStart, m_str - identifierStart );
	}
	else
	{
		return ycStringRef();
	}
}

bool ycStringRef::TakeIdentifier( const ycStringRef& identifier )
{
	const char* str = m_str;
	ycSize_t    len = m_len;

	if( len > 0 && ycStringUtils::IsIdentifierFirstChar( *str ) )
	{
		--len;
		++str;
		while( len && ycStringUtils::IsIdentifier( *str ) ) { ++str; --len; }
		const char* identifierStart = m_str;
		const ycStringRef found( identifierStart, str - identifierStart );
		if( found.Equals( identifier ) )
		{
			m_str = (char*)str;
			m_len = len;
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
}

ycSize_t ycStringRef::TrimLeadingWhitespace()
{
	const char* orig = m_str;
	while( m_len && ycStringUtils::IsWhiteSpace( *m_str ) )
	{
		--m_len;
		++m_str;
	}
	return m_str - orig;
}

ycSize_t ycStringRef::TrimLeadingHorizWhitespace()
{
	const char* orig = m_str;
	while( m_len && ycStringUtils::IsHorizWhiteSpace( *m_str ) )
	{
		--m_len;
		++m_str;
	}
	return m_str - orig;
}

ycSize_t ycStringRef::TrimTrailingWhiteSpace()
{
	const char* end = m_str + m_len - 1;
	const char* origEnd = end;
	while( m_len && ycStringUtils::IsWhiteSpace( *end ) )
	{
		--m_len;
		--end;
	}
	return origEnd - end;
}

ycSize_t ycStringRef::TrimSurroundingWhiteSpace()
{
	ycSize_t left = m_len;
	const char* str = m_str, *end = str + left;
	while( left && ycStringUtils::IsWhiteSpace( *str ) )
	{
		--left;
		++str;
	}

	while( left && ycStringUtils::IsWhiteSpace( *--end ) )
	{
		--left;
	}

	m_str = (char*)str;
	m_len = left;
	return m_len;
}


ycSize_t ycStringRef::TrimTrailingHorizWhiteSpace()
{
	const char* end = m_str + m_len - 1;
	const char* origEnd = end;
	while( m_len && ycStringUtils::IsHorizWhiteSpace( *end ) )
	{
		--m_len;
		--end;
	}
	return origEnd - end;
}

ycSize_t ycStringRef::TrimLeadingComment()
{
	const char* str = m_str;
	ycSize_t    len = m_len;
	ycSize_t    trimmed = 0;

	// trim leading whitespace and apply if there was a comment
	while( len && u8( *str ) <= ' ' )
	{
		--len;
		++str;
	}

	if( len >= 2 && str[0] == '/' )
	{
		if( str[1] == '/' )
		{
			str += 2;
			len -= 2;
			while( len )
			{
				const char ch = *str;
				if( ch == '\n' || ch == '\r' ) { break; }
				++str;
				--len;
			}
			trimmed = str - m_str;
			m_str = (char*)str;
			m_len = len;
		}
		else if( str[1] == '*' )
		{
			str += 2;
			len -= 2;
			do
			{
				if( len < 2 ) { return ycSize_t(-1); }
				if( str[0] == '*' && str[1] == '/' )
				{
					str += 2;
					len -= 2;
					break;
				}
				++str;
				--len;
			} while( len );
			while( len && u8(*str) < ' ' && *str != '\n' && *str != '\r' ) // also skip none-line-breaking white space after /* */ comment
			{
				++str;
				--len;
			}
			trimmed = str - m_str;
			m_str = (char*)str;
			m_len = len;
		}
	}

	return trimmed;
}

ycSize_t ycStringRef::LineLength( const bool includeLineEnd ) const
{
	const char* str = m_str;
	ycSize_t    len = m_len;
	while( len )
	{
		const char ch = *str;
		if( ch == '\r' )
		{
			if( includeLineEnd )
			{
				++str;
				--len;
				if( len && *str == '\n' )
				{
					++str;
					//--len;
				}
			}
			break;
		}
		else if( ch == '\n' )
		{
			if( includeLineEnd )
			{
				++str;
				//--len;
			}
			break;
		}
		++str;
		--len;
	}
	return str - m_str;
}

ycSize_t ycStringRef::LineLengthWithComments( const bool includeLineEnd ) const
{
	const char* str = m_str;
	ycSize_t    len = m_len;
	while( len )
	{
		const char ch = *str;
		if( ch == '\r' )
		{
			if( includeLineEnd )
			{
				++str;
				--len;
				if( len && *str == '\n' )
				{
					++str;
					//--len;
				}
			}
			break;
		}
		else if( ch == '\n' )
		{
			if( includeLineEnd )
			{
				++str;
				//--len;
			}
			break;
		}
		else if( ch == '/' && len > 1 && str[1] == '*' )
		{
			str += 2;
			len -= 2;
			do
			{
				if( len < 2 ) { return ycSize_t(-1); }
				if( str[0] == '*' && str[1] == '/' )
				{
					str += 2;
					len -= 2;
					break;
				}
				++str;
				--len;
			}
			while( len );
		}
		else
		{
			++str;
			--len;
		}
	}
	return str - m_str;
}

ycStringRef ycStringRef::FileBaseName() const
{
	ycSize_t left  = m_len;
	if( left )
	{
		const char* end = m_str + left;
		const char* scan = end;
		while( left )
		{
			--scan;
			const char ch = *scan;

			if( ch == '/' || ch == '\\' )
			{
				break;
			}
			else if( ch == '.' )
			{
				end = scan;
			}

			--left;
		}
		const char* start = m_str + left;
		ycAssert( end >= start ); // what about "foo/.svn" ?
		return ycStringRef( start, end - start );
	}
	else
	{
		return ycStringRef();
	}
}

ycStringRef ycStringRef::PathName() const
{
	const s32 pos = FindLast( '/', '\\' );
	if( pos != -1 )
	{
		return Left( pos+1 );
	}
	else
	{
		return ycStringRef();
	}
}


ycStringRef ycStringRef::FileName() const
{
	ycSize_t left = m_len;
	if( left )
	{
		const char* end = m_str + left;
		const char* scan = end;
		while( left )
		{
			--scan;
			const char ch = *scan;
		
			if( ch == '/' || ch == '\\' )
			{
				break;
			}
			--left;
		}
		const char* start = m_str + left;
		ycAssert( end >= start ); // what about "foo/.svn" ?
		return ycStringRef( start, end - start );
	}
	return ycStringRef( "" );
}

ycStringRef ycStringRef::Extension( bool onlyFirstExtension ) const
{
	ycSize_t left = m_len;
	ycSize_t bestLeft = 0;
	while( left > 0 )
	{
		char c = m_str[ left ];
		if( c == '.' ) 
		{
			if( onlyFirstExtension )
			{
				return Skip( left );
			}
			else
			{
				bestLeft = left;
			}
		}
		if( c == '/' || c == '\\' || c == '*' ) 
		{
			if( bestLeft )
			{
				return Skip( bestLeft );
			}
			break;
		}
		left--;
	}
	if( bestLeft )
	{
		return Skip( bestLeft );
	}
	return ycStringRef( "" );
}

ycStringRef ycStringRef::NoExtension() const
{
	ycSize_t left = m_len;
	ycSize_t ext = m_len;
	while( left > 0 )
	{
		char c = m_str[ left ];
		if( c == '.' ) {
			ext = left;
		}
		if( c == '/' || c == '\\' || c == '*' ) {
			break;
		}
		left--;
	}
	return Left( ext );
}

ycStringRef ycStringRef::CdUp() const
{
	s32 pos = FindLast( '/', '\\' );
	ycStringRef returnMe( *this );
	if( pos >= ((s32)Length()-1) )
	{
		returnMe.TakeLast();
		pos = returnMe.FindLast( '/', '\\' );
	}
	if( pos != -1 )
	{
		return returnMe.Left( pos+1 );
	}
	else
	{
		return ycStringRef();
	}
}

ycStringRef ycStringRef::FunctionName() const
{
	// example in MSVC: void __cdecl ShovelKnight::OnSpeedPadPresent(class SpeedPad *)
	// @TODO: get example and test in clang
	// 
	// plan:
	// (a) the beginning of the last paren group in the name will mark the end of the function name
	// (b) searching backward from the end of the function name, the first space found will mark the
	//		beginning (excluding any spaces within () or <> groups)

	const char* cursor = Get() + Length() - 1;

	// from the end, find beginning of the first () pair (the parameters)
	s32 parenStack = 0;
	bool parenStackHasBeenNonZero = false;
	for( ; cursor >= Get(); --cursor )
	{
		switch( *cursor )
		{
		case ')': ++parenStack; break;
		case '(': --parenStack; break;
		}

		parenStackHasBeenNonZero = parenStackHasBeenNonZero || parenStack > 0;
		if( parenStackHasBeenNonZero && parenStack == 0 )
		{
			break;
		}
	}

	const char* const end = cursor;
	--cursor;

	// now find the beginning of the function name (first space outside () or <> groups)
	s32 bracketStack = 0;
	for( ; cursor >= Get(); --cursor )
	{
		switch( *cursor )
		{
		case ')': ++parenStack; break;
		case '(': --parenStack; break;
		case '>': ++bracketStack; break;
		case '<': --bracketStack; break;
		}

		if( parenStack == 0 && bracketStack == 0 && *cursor == ' ' )
		{
			++cursor;
			return ycStringRef( cursor, end - cursor );
		}
	}

	return ycStringRef();
}

#ifdef YC_TEST
#include "ycTest.h"

#include <string.h>
YC_BEGIN_TEST( ycStringRef_ToInt )
{
	const struct {
		const char* str;
		bool converted;
		s64 num = 0;
	} testCases[] = {
		{ "1", true, 1 },
		{ "0", true, 0 },
		{ "-1", true, -1 },
		{ "1234", true, 1234 },
		{ "-1234", true, -1234 },
		{ "01234", true, 1234 },
		{ "1]", true, 1 },
		{ "1px", true, 1 },
		{ "123]", true, 123 },
		{ "123px", true, 123 },

		{ "abc", false },
		{ "abc123", false },
	};

	for( ycSize_t i = 0; i != YC_ARRAY_SIZE( testCases ); ++i )
	{
		const ycStringRef ref{ testCases[ i ].str };
		bool converted = false;
		s64 num = ref.ToInt( &converted );
		YC_TEST_CHECK( converted == testCases[ i ].converted );
		YC_TEST_CHECK( num == testCases[ i ].num );
	}

	YC_TEST_PASS( "ycStringRef::ToInt" );
}

#endif /* YC_TEST */
