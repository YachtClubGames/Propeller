#include "ycStringUtils.h"

#include <stdlib.h> // atof
#include "ycStd.h"
#include "ycVec2.h"
#include "ycVec3.h"
#include "ycVec4.h"
#include "ycMtx4.h"
#include <stdarg.h>	// va_args
#include <stdio.h> // vsnprintf

// TODO HACK FIXME: shitty impl
ycSize_t ycStringUtils::Copy( char* YC_RESTRICT dst, const char* YC_RESTRICT pSrc )
{
	ycAssert( dst );
	ycAssert( pSrc );
	const char* YC_RESTRICT src = pSrc;
	for(;;)
	{
		*dst = *src;
		if( *src == '\0' ) { return ycSize_t(src-pSrc); }
		++dst;
		++src;
	}
}

ycSize_t ycStringUtils::Copy( char *YC_RESTRICT dst, const char *YC_RESTRICT src, ycSize_t space )
{
	ycAssert( dst );
	ycAssert( src );
	ycAssert( space );
	char* _dst = dst;
	const char* YC_RESTRICT _src = src;
	ycSize_t left = space;
	while( left )
	{
		*_dst = *_src;
		if( *_src == '\0' ) { return ycSize_t(_src-src); }
		++_dst;
		++_src;
		--left;
	}
	dst[ space - 1 ] = 0;
	return ycSize_t(_src-src);
}

void ycStringUtils::Append( char* dst, const char* str )
{
	ycAssert( dst );
	ycAssert( str );
	Append( dst, Length( dst ), str );
}

void ycStringUtils::Append( char* dst, const ycSize_t start, const char* str )
{
	ycAssert( dst );
	ycAssert( str );
	const char* YC_RESTRICT src = str;
	for(;;)
	{
		dst[start] = *src;
		if( *src == '\0' ) { return; }
		++dst;
		++src;
	}
}

ycSize_t ycStringUtils::Length( const char* str )
{
	ycAssert( str != nullptr );
	const char* s = str;
	for( /**/ ; *str != '\0'; ++str ) { /* nop */ }
	return str - s;
}

ycSize_t ycStringUtils::Length( const char* str, u32 maxLength, bool * foundEnd )
{
	ycAssert( str != nullptr );
	const char* s = str;
	for( /**/ ; *str != '\0' && maxLength; ++str ) { --maxLength; }
	if( *str != '\0' )
	{
		*foundEnd = false;
	}
	else
	{
		*foundEnd = true;
	}
	return str - s;
}

// implementation by Dale Weiler
ycSize_t ycStringUtils::LengthAligned( const char* str )
{
	ycAssert( str != nullptr );
	#define YToCStrLEN_ALIGN (sizeof(ycSize_t))
	#define YToCStrLEN_ONES ((ycSize_t)-1/0xffu)
	#define YToCStrLEN_HIGHS (YToCStrLEN_ONES * (0xffu/2+1))
	#define YToCStrLEN_HASZERO(X) (((X)-YToCStrLEN_ONES) & ~(X) & YToCStrLEN_HIGHS)
	const char* s = str;
	const char* a = str;
	const ycSize_t* w;
	for( /**/ ; (uintptr_t)s % YToCStrLEN_ALIGN; ++s )
	{
		if( *s ) { continue; }
		return ycSize_t( s-a );
	}
	for( w = (const ycSize_t*)s; !YToCStrLEN_HASZERO(*w); ++w ) { /* Make the next word legal to access */ }
	for( s = (const char *)w; *s; s++) { /* nop */ } /* It's not legal to access this area anymore */
	return ycSize_t( s-a );
}

bool ycStringUtils::Equals( const char* a, const char* b )
{
	ycAssert( a != nullptr );
	ycAssert( b != nullptr );
	for(;;)
	{
		if( *a != *b ) return false;
		if( *a == '\0' && *b == '\0' ) return true;
		if( *a == '\0' || *b == '\0' ) return false;
		++a;
		++b;
	}
}

bool ycStringUtils::EqualsI( const char* a, const char* b )
{
	ycAssert( a != nullptr );
	ycAssert( b != nullptr );
	for(;;)
	{
		if( ToLower(*a) != ToLower(*b) ) return false;
		if( *a == '\0' && *b == '\0' )   return true;
		if( *a == '\0' || *b == '\0' )   return false;
		++a;
		++b;
	}
}

bool ycStringUtils::BeginsWith( const char* string, const char* beginsWith )
{
	ycAssert( string     != nullptr );
	ycAssert( beginsWith != nullptr );
	for(;;)
	{
		if( *beginsWith == '\0' )    return true;
		if( *string != *beginsWith ) return false;
		++string;
		++beginsWith;
	}
}

bool ycStringUtils::BeginsWithI( const char* string, const char* beginsWith )
{
	ycAssert( string     != nullptr );
	ycAssert( beginsWith != nullptr );
	for(;;)
	{
		if( *beginsWith == '\0' )                      return true;
		if( ToLower(*string) != ToLower(*beginsWith) ) return false;
		++string;
		++beginsWith;
	}
}

bool ycStringUtils::BeginsWith( const char* string, const char* beginsWith, const ycSize_t length )
{
	if( length == 0 )
		return true;
	ycAssert( string     != nullptr );
	ycAssert( beginsWith != nullptr );
	for( ycSize_t i = 0; i != length; ++i )
	{
		if( *string != *beginsWith ) return false;
		++string;
		++beginsWith;
	}
	return true;
}

bool ycStringUtils::BeginsWithI( const char* string, const char* beginsWith, const ycSize_t length )
{
	if( length == 0 )
		return true;
	ycAssert( string     != nullptr );
	ycAssert( beginsWith != nullptr );
	for( ycSize_t i = 0; i != length; ++i )
	{
		if( ToLower(*string) != ToLower(*beginsWith) ) return false;
		++string;
		++beginsWith;
	}
	return true;
}

bool ycStringUtils::Contains( const char* string, const char* contains )
{
	ycAssert( string   != nullptr );
	ycAssert( contains != nullptr );
	for(;;)
	{
		if( *string == '\0' )	                       return false;
		if( ycStringUtils::BeginsWith( string, contains ) ) return true;
		++string;
	}
}

bool ycStringUtils::ContainsI( const char* string, const char* contains )
{
	ycAssert( string   != nullptr );
	ycAssert( contains != nullptr );
	for(;;)
	{
		if( *string == '\0' )                           return false;
		if( ycStringUtils::BeginsWithI( string, contains ) ) return true; // TODO HACK FIXME: this is calling ToLower a lot more than necessary
		++string;
	}
}

bool ycStringUtils::Contains( const char* string, const char contains )
{
	for(;;)
	{
		if( *string == '\0' )     return false;
		if( *string == contains ) return true;
		++string;
	}
}

bool ycStringUtils::ContainsI( const char* string, const char contains )
{
	const char lc = ToLower( contains );
	for(;;)
	{
		if( *string == '\0' )          return false;
		if( ToLower( *string ) == lc ) return true;
		++string;
	}
}

char ycStringUtils::ToLower( const char ascii )
{
	return ( ascii >= 'A' && ascii <= 'Z' ) ? ascii + 32 : ascii ;
}

bool ycStringUtils::IsLower( const char ascii )
{
	return ( ascii >= 'A' && ascii <= 'Z' );
}

char ycStringUtils::ToUpper( const char ascii )
{
	return ( ascii >= 'a' && ascii <= 'z' ) ? ascii - 32 : ascii ;
}

float ycStringUtils::ToFloat( const char* string )
{
	return (float)atof( string );
}

double ycStringUtils::ToDouble( const char* string )
{
	return atof( string );
}

int32_t ycStringUtils::ToInt( const char* string )
{
	int32_t val = 0;
	bool neg = false; // im sure this could be handled better, would prolly need to bench different approaches (eg pure math would work, put shader thinking cap on)
	if( *string == '-' ) { neg = true; ++string; }
	while( IsDigit( *string ) )
	{
		const char digit = *string;
		++string;
		val *= 10;
		val += digit - '0';
	}
	if( neg ) { val *= -1; }
	return val;
}

int32_t ycStringUtils::ToInt( const char* string, const ycSize_t len )
{
	ycAssert( len );
	int32_t val = 0;
	bool neg = false; // im sure this could be handled better, would prolly need to bench different approaches (eg pure math would work, put shader thinking cap on)
	ycSize_t i = 0;
	if( *string == '-' ) { neg = true; ++string; ++i; }
	for( /**/; i != len; ++i )
	{
		const char digit = *string;
		ycAssert( digit >= '0' && digit <= '9' );
		++string;
		val *= 10;
		val += digit - '0';
	}
	if( neg ) { val *= -1; }
	return val;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const u32 val )
{
	if( val == 0)
	{
		ycAssertMsg( buffSize >= 2, "Buffer is not big enough" );
		buff[0] = '0';
		buff[1] = '\0';
		return 1;
	}

	// Check how much space we need for the buffer
	u32 numChar = 1;
	u32 tempVal = val;
	while( tempVal > 0 )
	{
		tempVal /= 10;
		numChar++;
	}
	ycAssertMsg( numChar <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );

	// Start iterating backwards to convert int to string
	tempVal = val;
	buff[ --numChar ] = '\0';

	u32 retNumChar = numChar;
	while( numChar > 0 )
	{
		buff[ --numChar ] = ( tempVal % 10 + '0' );
		tempVal /= 10;
	}
	return retNumChar;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const u64 val )
{
	if( val == 0 )
	{
		ycAssertMsg( buffSize >= 2, "Buffer is not big enough" );
		buff[0] = '0';
		buff[1] = '\0';
		return 1;
	}

	// Check how much space we need for the buffer
	u32 numChar = 1;
	u64 tempVal = val;
	while( tempVal > 0 )
	{
		tempVal /= 10;
		numChar++;
	}
	ycAssertMsg( numChar <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );

	// Start iterating backwards to convert int to string
	tempVal = val;
	buff[ --numChar ] = '\0';

	u32 retNumChar = numChar;
	while( numChar > 0 )
	{
		buff[ --numChar ] = ( tempVal % 10 + '0' );
		tempVal /= 10;
	}
	return retNumChar;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const s32 val )
{
	if( val == 0 )
	{
		ycAssertMsg( buffSize >= 2, "Buffer is not big enough" );
		buff[0] = '0';
		buff[1] = '\0';
		return 1;
	}

	// Check how much space we need for the buffer
	bool neg = ( val < 0 ) ? true : false;
	u32 numChar = ( neg ) ? 2 : 1;
	s32 tempVal = val;
	while( tempVal != 0 )
	{
		tempVal /= 10;
		numChar++;
	}
	ycAssertMsg( numChar <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );

	// Start iterating backwards to convert int to string
	buff[ --numChar ] = '\0';
	u32 retNumChar = numChar;

	// handle case where val == INT_MIN
	if( !( val ^ 0x80000000 ) )
	{
		buff[ --numChar ] = '8';
		tempVal = val / 10;
		tempVal = ~( tempVal ) + 1;	// convert it to positive
	}
	else
	{
		tempVal = ( neg ) ? ~( val ) + 1 : val;
	}

	u32 end = ( neg ) ? 1 : 0;
	while( numChar > end )
	{
		buff[ --numChar ] = ( tempVal % 10 + '0' );
		tempVal /= 10;
	}
	buff[0] = ( numChar == 1 ) ? '-' : buff[0];

	return retNumChar;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const s64 val )
{
	if( val == 0 )
	{
		ycAssertMsg( 2 < buffSize, "Buffer is not big enough" );
		buff[0] = '0';
		buff[1] = '\0';
		return 1;
	}

	// Check how much space we need for the buffer
	bool neg = ( val < 0 ) ? true : false;
	u32 numChar = ( neg ) ? 2 : 1;
	s64 tempVal = val;
	while( tempVal != 0 )
	{
		tempVal /= 10;
		numChar++;
	}
	ycAssertMsg( numChar <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );

	// Start iterating backwards to convert int to string
	buff[ --numChar ] = '\0';
	u32 retNumChar = numChar;

	// handle case where val == INT_MIN
	if( !( val ^ 0x8000000000000000llu ) )
	{
		buff[ --numChar ] = '8';
		tempVal = val / 10;
		tempVal = ~( tempVal ) + 1;
	}
	else
	{
		tempVal = ( neg ) ? ~( val ) + 1 : val;
	}

	u32 end = ( neg ) ? 1 : 0;
	while( numChar > end )
	{
		buff[ --numChar ] = ( tempVal % 10 + '0' );
		tempVal /= 10;
	}
	buff[0] = ( numChar == 1 ) ? '-' : buff[0];
	return retNumChar;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const f32 val )
{
	char tempValBuff[16];	// should only need 15, but might as well use 16
	ReadableFloatF( tempValBuff, sizeof( tempValBuff ), "%.9g", val );

	ycSize_t strLen = Length( tempValBuff ) + 1;
	ycAssertMsg( strLen <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );
	ycMemCpy( buff, tempValBuff, strLen );
	return u32(strLen) - 1;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const f64 val )
{
	char tempValBuff[310];	// Max amount of digits for f64
	ReadableFloatF( tempValBuff, sizeof( tempValBuff ), "%f", val );

	ycSize_t strLen = Length( tempValBuff ) + 1;
	ycAssertMsg( strLen <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );
	ycMemCpy( buff, tempValBuff, strLen );
	return u32(strLen) - 1;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const ycVec2& vec )
{
	char strVecX[ 41 ];
	char strVecY[ 41 ];
	ycStringUtils::ReadableFloatF( strVecX, sizeof( strVecX ), "%f", vec.x );
	ycStringUtils::ReadableFloatF( strVecY, sizeof( strVecY ), "%f", vec.y );

	ycSize_t strVecXLen = Length( strVecX ) + 1;
	ycSize_t strVecYLen = Length( strVecY ) + 1;
	ycAssertMsg( strVecXLen + strVecYLen <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );
	ycSize_t size = 0;
	buff[size++] = '(';
	size += Copy( &buff[size], strVecX, strVecXLen );
	size += Copy( &buff[size], ", " );
	size += Copy( &buff[size], strVecY, strVecYLen );
	buff[size++] = ')';
	buff[size] = '\0';
	return u32(size);
}
u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const ycVec3& vec )
{
	char strVecX[ 41 ];
	char strVecY[ 41 ];
	char strVecZ[ 41 ];
	ycStringUtils::ReadableFloatF( strVecX, sizeof( strVecX ), "%f", vec.x );
	ycStringUtils::ReadableFloatF( strVecY, sizeof( strVecY ), "%f", vec.y );
	ycStringUtils::ReadableFloatF( strVecZ, sizeof( strVecZ ), "%f", vec.z );

	ycSize_t strVecXLen = Length( strVecX ) + 1;
	ycSize_t strVecYLen = Length( strVecY ) + 1;
	ycSize_t strVecZLen = Length( strVecZ ) + 1;
	ycAssertMsg( strVecXLen + strVecYLen + strVecZLen <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );
	ycSize_t size = 0;
	buff[size++] = '(';
	size += Copy( &buff[size], strVecX, strVecXLen );
	size += Copy( &buff[size], ", " );
	size += Copy( &buff[size], strVecY, strVecYLen );
	size += Copy( &buff[size], ", " );
	size += Copy( &buff[size], strVecZ, strVecZLen );
	buff[size++] = ')';
	buff[size] = '\0';
	return u32(size);
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const ycVec4& vec )
{
	char strVecX[ 41 ];
	char strVecY[ 41 ];
	char strVecZ[ 41 ];
	char strVecW[ 41 ];
	ycStringUtils::ReadableFloatF( strVecX, sizeof( strVecX ), "%f", vec.x );
	ycStringUtils::ReadableFloatF( strVecY, sizeof( strVecY ), "%f", vec.y );
	ycStringUtils::ReadableFloatF( strVecZ, sizeof( strVecZ ), "%f", vec.z );
	ycStringUtils::ReadableFloatF( strVecW, sizeof( strVecW ), "%f", vec.w );

	ycSize_t strVecXLen = Length( strVecX ) + 1;
	ycSize_t strVecYLen = Length( strVecY ) + 1;
	ycSize_t strVecZLen = Length( strVecZ ) + 1;
	ycSize_t strVecWLen = Length( strVecW ) + 1;
	ycAssertMsg( strVecXLen + strVecYLen + strVecZLen + strVecWLen <= buffSize, "Buffer is not big enough" );
	YC_UNUSED( buffSize );
	ycSize_t size = 0;
	buff[size++] = '(';
	size += Copy( &buff[size], strVecX, strVecXLen );
	size += Copy( &buff[size], ", " );
	size += Copy( &buff[size], strVecY, strVecYLen );
	size += Copy( &buff[size], ", " );
	size += Copy( &buff[size], strVecZ, strVecZLen );
	size += Copy( &buff[size], ", " );
	size += Copy( &buff[size], strVecW, strVecWLen );
	buff[size++] = ')';
	buff[size] = '\0';
	return u32(size);
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const ycMtx4& mat )
{
	char* cursor = buff;
	u32 remaining = (u32)buffSize;

#define PRINT( x_ ) do { \
	u32 ret = ToString( cursor, remaining, x_ ); \
	if( remaining < ret ) { ret = remaining; } \
	cursor += ret;  remaining -= ret; \
} while( 0 );

	PRINT( "(" );
	PRINT( mat.x );
	PRINT( ", " );
	PRINT( mat.y );
	PRINT( ", " );
	PRINT( mat.z );
	PRINT( ", " );
	PRINT( mat.t );
	PRINT( ")" );
	PRINT( "\0" );

	return (u32)buffSize - remaining;
#undef PRINT
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const char* val )
{
	int result = snprintf( buff, buffSize, "%s", val );
	ycAssert( result >= 0 );
	return result;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, const void* ptr )
{
	int result = snprintf( buff, buffSize, "%p", ptr );
	ycAssert( result >= 0 );
	return result;
}

u32 ycStringUtils::ToString( char* buff, const ycSize_t buffSize, bool val )
{
	int result = snprintf( buff, buffSize, "%d", val );
	ycAssert( result >= 0 );
	return result;
}

bool ycStringUtils::IsWhiteSpace( const char c )
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool ycStringUtils::IsSeparator( const char c )
{
	return c!='\'' && !(c>='0' && c<='9') && !(c>='A' && c<='Z') && !(c>='a' && c<='z');
}

bool ycStringUtils::IsHorizWhiteSpace( const char c )
{
	return c == ' ' || c == '\t';
}

bool ycStringUtils::IsIdentifier( const char c )
{
	//[a-zA-Z_0-9]*
	return IsDigit( c ) || IsAlpha( c ) || c == '_';
}

bool ycStringUtils::IsIdentifierFirstChar( const char c )
{
	//[a-zA-Z_]*
	return IsAlpha( c ) || c == '_';
}

bool ycStringUtils::IsDigit( const char c )
{
	return ( c >= '0' && c <= '9' );
}

bool ycStringUtils::IsAlpha( const char c )
{
	return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
}

ycSize_t ycStringUtils::CountTrailingWhiteSpace( const char* start, const ycSize_t length )
{
	const char* pos = start + length - 1; // don't access first char after string
	const char* last = pos;
	while( pos > start && IsWhiteSpace( *pos ) ) { --pos; } // ends loop when character at pos is not whitespace
	return last - pos;
}

ycSize_t ycStringUtils::RemoveSubStr( const char* string, ycSize_t length, char* subStr, ycSize_t subStrLen )
{
	const ycSize_t offset = ycSize_t( subStr - string );
	if( offset < length )
	{
		const ycSize_t left = length - offset;
		const ycSize_t remain = left < subStrLen ? 0 : (left - subStrLen);
		const char* read = string + length - remain;
		for( ycSize_t i = 0; i < remain; ++i ) { *subStr++ = *read++; }
		return offset + remain;
	}
	return length;
}

ycSize_t ycStringUtils::InsertSubStr( char* string, ycSize_t length, ycSize_t capacity, ycSize_t insert, const char* subStr, ycSize_t subStrLen )
{
	ycAssert( length <= capacity );
	if( insert > length ) { insert = length; }
	ycSize_t subEnd = insert + subStrLen;
	if( subEnd > capacity ) { subEnd = capacity; }
	ycSize_t remain = length - insert;
	if( ( remain + subEnd ) > capacity ) { remain = capacity - subEnd; }

	const char* read = string + insert + remain;
	char* write = string + subEnd + remain;
	for( ycSize_t i = 0; i < remain; ++i ) { *--write = *--read; }
	read = subStr + subEnd - insert;
	for( ycSize_t i = subEnd - insert; i; --i ) { *--write = *--read; }
	return subEnd + remain;
}

ycSize_t ycStringUtils::SprintF( char* buf, ycSize_t bufSize, const char* format, ... )
{
	va_list args;
	va_start( args, format );
	int ret = vsnprintf( buf, bufSize, format, args );
	va_end( args );
	ycAssert( ret >= 0 );
	ycAssert( (ycSize_t)ret < bufSize );
	return (ret < 0) ? 0 : (ycSize_t)ret;
}

ycSize_t ycStringUtils::VSprintF( char * buf, ycSize_t bufSize, const char * format, va_list argList )
{
	int ret = vsnprintf( buf, bufSize, format, argList );
	ycAssert( ret >= 0 );
	ycAssert( (ycSize_t)ret < bufSize );
	return (ret < 0) ? 0 : (ycSize_t)ret;
}

ycSize_t ycStringUtils::SprintF_NoCheck( char* buf, ycSize_t bufSize, const char* format, ... )
{
	va_list args;
	va_start( args, format );
	int ret = vsnprintf( buf, bufSize, format, args );
	va_end( args );
	ycAssert( ret >= 0 );
	return (ret < 0) ? 0 : (ycSize_t)ret;
}

ycSize_t ycStringUtils::VSprintF_NoCheck( char * buf, ycSize_t bufSize, const char * format, va_list argList )
{
	int ret = vsnprintf( buf, bufSize, format, argList );
	ycAssert( ret >= 0 );
	return (ret < 0) ? 0 : (ycSize_t)ret;
}

s32 ycStringUtils::ScanF( const char * string, const char * format, ... )
{
	va_list args;
	va_start( args, format );
	s32 ret = vsscanf( string, format, args );
	va_end( args );
	return ret;
}

ycSize_t ycStringUtils::ReadableFloatF( char* buf, ycSize_t bufSize, const char* format, f32 value )
{
	ycSize_t l = SprintF( buf, bufSize, format, value );
	return l;
}

ycSize_t ycStringUtils::ReadableFloatF( char* buf, ycSize_t bufSize, const char* format, f64 value )
{
	ycSize_t l = SprintF( buf, bufSize, format, value );
	if( l )
	{
		while( l && buf[l - 1] == '0' )
		{
			if( l > 1 && buf[l - 2] == '.' ) { break; }
			--l;
		}
		buf[l] = 0;
	}
	return l;
}

ycSize_t ycStringUtils::CleanupFloats( char* buf )
{
	char* src = buf;
	char* out = buf;
	char* dec = nullptr;
	while( *src )
	{
		if( dec && *src != '0' )
		{
			if( !( *src >= '0' && *src <= '9' ) && out > dec ) { out = dec; }
			dec = nullptr;
		}
		if( !dec && src[ 1 ] == '.' && *src >= '0' && *src <= '9' )
		{
			*out++ = *src++;
			dec = out+2;
		}
		*out++ = *src++;
	}
	if( dec ) { *dec = 0; }
	else { *out = 0; }
	return out-buf;
}

bool ycStringUtils::CompareIgnoringWhitspace( const char* a, const char* b )
{
	ycAssert( a && b );
	for(;;)
	{
		while( IsWhiteSpace( *a ) ) { ++a; }
		while( IsWhiteSpace( *b ) ) { ++b; }
		const char c0 = *a;
		const char c1 = *b;
		if( c0 == '\0' && c1 == '\0' ) { return true; }
		if( c0 != c1 ) { return false; }
	}
	// unreachable
}

bool ycStringUtils::CompareIgnoringWhitspace( const char* a, ycSize_t aLen, const char* b, ycSize_t bLen )
{
	ycAssert( a && b );
	for(;;)
	{
		while( aLen && ycStringUtils::IsWhiteSpace( *a ) ) { ++a; --aLen; }
		while( bLen && ycStringUtils::IsWhiteSpace( *b ) ) { ++b; --bLen; }
		if( aLen == 0 || bLen == 0 )
		{
			return aLen == 0 && bLen == 0;
		}
		if( *a != *b )
		{
			return false;
		}
		++a; --aLen;
		++b; --bLen;
	}
	// unreachable
}

const char* ycStringUtils::Find( const char* start, char target )
{
	ycAssert( start );
	for( ; *start != '\0'; start++ ) { if( *start == target ) { return start; } }
	return nullptr;
}

const char* ycStringUtils::Find( const char* start, ycSize_t length, char target )
{
	ycAssert( start );
	for( u32 i = 0; i < length; i++ ) { if( start[ i ] == target ) { return start + i; } }
	return nullptr;
}


#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycStringUtils_ToLower )
{
	YC_TEST_CHECK( '0' == ycStringUtils::ToLower( '0' ) );
	YC_TEST_CHECK( '1' == ycStringUtils::ToLower( '1' ) );
	YC_TEST_CHECK( '2' == ycStringUtils::ToLower( '2' ) );
	YC_TEST_CHECK( '3' == ycStringUtils::ToLower( '3' ) );
	YC_TEST_CHECK( '4' == ycStringUtils::ToLower( '4' ) );
	YC_TEST_CHECK( '5' == ycStringUtils::ToLower( '5' ) );
	YC_TEST_CHECK( '6' == ycStringUtils::ToLower( '6' ) );
	YC_TEST_CHECK( '7' == ycStringUtils::ToLower( '7' ) );
	YC_TEST_CHECK( '8' == ycStringUtils::ToLower( '8' ) );
	YC_TEST_CHECK( '9' == ycStringUtils::ToLower( '9' ) );
	YC_TEST_CHECK( 'a' == ycStringUtils::ToLower( 'a' ) );
	YC_TEST_CHECK( 'b' == ycStringUtils::ToLower( 'b' ) );
	YC_TEST_CHECK( 'c' == ycStringUtils::ToLower( 'c' ) );
	YC_TEST_CHECK( 'd' == ycStringUtils::ToLower( 'd' ) );
	YC_TEST_CHECK( 'e' == ycStringUtils::ToLower( 'e' ) );
	YC_TEST_CHECK( 'f' == ycStringUtils::ToLower( 'f' ) );
	YC_TEST_CHECK( 'g' == ycStringUtils::ToLower( 'g' ) );
	YC_TEST_CHECK( 'h' == ycStringUtils::ToLower( 'h' ) );
	YC_TEST_CHECK( 'i' == ycStringUtils::ToLower( 'i' ) );
	YC_TEST_CHECK( 'j' == ycStringUtils::ToLower( 'j' ) );
	YC_TEST_CHECK( 'k' == ycStringUtils::ToLower( 'k' ) );
	YC_TEST_CHECK( 'l' == ycStringUtils::ToLower( 'l' ) );
	YC_TEST_CHECK( 'm' == ycStringUtils::ToLower( 'm' ) );
	YC_TEST_CHECK( 'n' == ycStringUtils::ToLower( 'n' ) );
	YC_TEST_CHECK( 'o' == ycStringUtils::ToLower( 'o' ) );
	YC_TEST_CHECK( 'p' == ycStringUtils::ToLower( 'p' ) );
	YC_TEST_CHECK( 'q' == ycStringUtils::ToLower( 'q' ) );
	YC_TEST_CHECK( 'r' == ycStringUtils::ToLower( 'r' ) );
	YC_TEST_CHECK( 's' == ycStringUtils::ToLower( 's' ) );
	YC_TEST_CHECK( 't' == ycStringUtils::ToLower( 't' ) );
	YC_TEST_CHECK( 'u' == ycStringUtils::ToLower( 'u' ) );
	YC_TEST_CHECK( 'v' == ycStringUtils::ToLower( 'v' ) );
	YC_TEST_CHECK( 'w' == ycStringUtils::ToLower( 'w' ) );
	YC_TEST_CHECK( 'x' == ycStringUtils::ToLower( 'x' ) );
	YC_TEST_CHECK( 'y' == ycStringUtils::ToLower( 'y' ) );
	YC_TEST_CHECK( 'z' == ycStringUtils::ToLower( 'z' ) );
	YC_TEST_CHECK( 'a' == ycStringUtils::ToLower( 'A' ) );
	YC_TEST_CHECK( 'b' == ycStringUtils::ToLower( 'B' ) );
	YC_TEST_CHECK( 'c' == ycStringUtils::ToLower( 'C' ) );
	YC_TEST_CHECK( 'd' == ycStringUtils::ToLower( 'D' ) );
	YC_TEST_CHECK( 'e' == ycStringUtils::ToLower( 'E' ) );
	YC_TEST_CHECK( 'f' == ycStringUtils::ToLower( 'F' ) );
	YC_TEST_CHECK( 'g' == ycStringUtils::ToLower( 'G' ) );
	YC_TEST_CHECK( 'h' == ycStringUtils::ToLower( 'H' ) );
	YC_TEST_CHECK( 'i' == ycStringUtils::ToLower( 'I' ) );
	YC_TEST_CHECK( 'j' == ycStringUtils::ToLower( 'J' ) );
	YC_TEST_CHECK( 'k' == ycStringUtils::ToLower( 'K' ) );
	YC_TEST_CHECK( 'l' == ycStringUtils::ToLower( 'L' ) );
	YC_TEST_CHECK( 'm' == ycStringUtils::ToLower( 'M' ) );
	YC_TEST_CHECK( 'n' == ycStringUtils::ToLower( 'N' ) );
	YC_TEST_CHECK( 'o' == ycStringUtils::ToLower( 'O' ) );
	YC_TEST_CHECK( 'p' == ycStringUtils::ToLower( 'P' ) );
	YC_TEST_CHECK( 'q' == ycStringUtils::ToLower( 'Q' ) );
	YC_TEST_CHECK( 'r' == ycStringUtils::ToLower( 'R' ) );
	YC_TEST_CHECK( 's' == ycStringUtils::ToLower( 'S' ) );
	YC_TEST_CHECK( 't' == ycStringUtils::ToLower( 'T' ) );
	YC_TEST_CHECK( 'u' == ycStringUtils::ToLower( 'U' ) );
	YC_TEST_CHECK( 'v' == ycStringUtils::ToLower( 'V' ) );
	YC_TEST_CHECK( 'w' == ycStringUtils::ToLower( 'W' ) );
	YC_TEST_CHECK( 'x' == ycStringUtils::ToLower( 'X' ) );
	YC_TEST_CHECK( 'y' == ycStringUtils::ToLower( 'Y' ) );
	YC_TEST_CHECK( 'z' == ycStringUtils::ToLower( 'Z' ) );
	YC_TEST_PASS( "ycStringUtils::ToLower" );
}

YC_BEGIN_TEST( ycStringUtils_ToUpper )
{
	YC_TEST_CHECK( '0' == ycStringUtils::ToUpper( '0' ) );
	YC_TEST_CHECK( '1' == ycStringUtils::ToUpper( '1' ) );
	YC_TEST_CHECK( '2' == ycStringUtils::ToUpper( '2' ) );
	YC_TEST_CHECK( '3' == ycStringUtils::ToUpper( '3' ) );
	YC_TEST_CHECK( '4' == ycStringUtils::ToUpper( '4' ) );
	YC_TEST_CHECK( '5' == ycStringUtils::ToUpper( '5' ) );
	YC_TEST_CHECK( '6' == ycStringUtils::ToUpper( '6' ) );
	YC_TEST_CHECK( '7' == ycStringUtils::ToUpper( '7' ) );
	YC_TEST_CHECK( '8' == ycStringUtils::ToUpper( '8' ) );
	YC_TEST_CHECK( '9' == ycStringUtils::ToUpper( '9' ) );
	YC_TEST_CHECK( 'A' == ycStringUtils::ToUpper( 'a' ) );
	YC_TEST_CHECK( 'B' == ycStringUtils::ToUpper( 'b' ) );
	YC_TEST_CHECK( 'C' == ycStringUtils::ToUpper( 'c' ) );
	YC_TEST_CHECK( 'D' == ycStringUtils::ToUpper( 'd' ) );
	YC_TEST_CHECK( 'E' == ycStringUtils::ToUpper( 'e' ) );
	YC_TEST_CHECK( 'F' == ycStringUtils::ToUpper( 'f' ) );
	YC_TEST_CHECK( 'G' == ycStringUtils::ToUpper( 'g' ) );
	YC_TEST_CHECK( 'H' == ycStringUtils::ToUpper( 'h' ) );
	YC_TEST_CHECK( 'I' == ycStringUtils::ToUpper( 'i' ) );
	YC_TEST_CHECK( 'J' == ycStringUtils::ToUpper( 'j' ) );
	YC_TEST_CHECK( 'K' == ycStringUtils::ToUpper( 'k' ) );
	YC_TEST_CHECK( 'L' == ycStringUtils::ToUpper( 'l' ) );
	YC_TEST_CHECK( 'M' == ycStringUtils::ToUpper( 'm' ) );
	YC_TEST_CHECK( 'N' == ycStringUtils::ToUpper( 'n' ) );
	YC_TEST_CHECK( 'O' == ycStringUtils::ToUpper( 'o' ) );
	YC_TEST_CHECK( 'P' == ycStringUtils::ToUpper( 'p' ) );
	YC_TEST_CHECK( 'Q' == ycStringUtils::ToUpper( 'q' ) );
	YC_TEST_CHECK( 'R' == ycStringUtils::ToUpper( 'r' ) );
	YC_TEST_CHECK( 'S' == ycStringUtils::ToUpper( 's' ) );
	YC_TEST_CHECK( 'T' == ycStringUtils::ToUpper( 't' ) );
	YC_TEST_CHECK( 'U' == ycStringUtils::ToUpper( 'u' ) );
	YC_TEST_CHECK( 'V' == ycStringUtils::ToUpper( 'v' ) );
	YC_TEST_CHECK( 'W' == ycStringUtils::ToUpper( 'w' ) );
	YC_TEST_CHECK( 'X' == ycStringUtils::ToUpper( 'x' ) );
	YC_TEST_CHECK( 'Y' == ycStringUtils::ToUpper( 'y' ) );
	YC_TEST_CHECK( 'Z' == ycStringUtils::ToUpper( 'z' ) );
	YC_TEST_CHECK( 'A' == ycStringUtils::ToUpper( 'A' ) );
	YC_TEST_CHECK( 'B' == ycStringUtils::ToUpper( 'B' ) );
	YC_TEST_CHECK( 'C' == ycStringUtils::ToUpper( 'C' ) );
	YC_TEST_CHECK( 'D' == ycStringUtils::ToUpper( 'D' ) );
	YC_TEST_CHECK( 'E' == ycStringUtils::ToUpper( 'E' ) );
	YC_TEST_CHECK( 'F' == ycStringUtils::ToUpper( 'F' ) );
	YC_TEST_CHECK( 'G' == ycStringUtils::ToUpper( 'G' ) );
	YC_TEST_CHECK( 'H' == ycStringUtils::ToUpper( 'H' ) );
	YC_TEST_CHECK( 'I' == ycStringUtils::ToUpper( 'I' ) );
	YC_TEST_CHECK( 'J' == ycStringUtils::ToUpper( 'J' ) );
	YC_TEST_CHECK( 'K' == ycStringUtils::ToUpper( 'K' ) );
	YC_TEST_CHECK( 'L' == ycStringUtils::ToUpper( 'L' ) );
	YC_TEST_CHECK( 'M' == ycStringUtils::ToUpper( 'M' ) );
	YC_TEST_CHECK( 'N' == ycStringUtils::ToUpper( 'N' ) );
	YC_TEST_CHECK( 'O' == ycStringUtils::ToUpper( 'O' ) );
	YC_TEST_CHECK( 'P' == ycStringUtils::ToUpper( 'P' ) );
	YC_TEST_CHECK( 'Q' == ycStringUtils::ToUpper( 'Q' ) );
	YC_TEST_CHECK( 'R' == ycStringUtils::ToUpper( 'R' ) );
	YC_TEST_CHECK( 'S' == ycStringUtils::ToUpper( 'S' ) );
	YC_TEST_CHECK( 'T' == ycStringUtils::ToUpper( 'T' ) );
	YC_TEST_CHECK( 'U' == ycStringUtils::ToUpper( 'U' ) );
	YC_TEST_CHECK( 'V' == ycStringUtils::ToUpper( 'V' ) );
	YC_TEST_CHECK( 'W' == ycStringUtils::ToUpper( 'W' ) );
	YC_TEST_CHECK( 'X' == ycStringUtils::ToUpper( 'X' ) );
	YC_TEST_CHECK( 'Y' == ycStringUtils::ToUpper( 'Y' ) );
	YC_TEST_CHECK( 'Z' == ycStringUtils::ToUpper( 'Z' ) );
	YC_TEST_PASS( "ycStringUtils::ToUpper" );
}

#include <string.h>
YC_BEGIN_TEST( ycStringUtils_Length )
{
	const char* strings[] = { "Adult", "Aeroplane", "Air", "Aircraft Carrier", "Airforce", "Airport", "Album", "Alphabet", "Apple", "Arm", "Army", "Baby", "Baby", "Backpack", "Balloon", "Banana", "Bank", "Barbecue", "Bathroom", "Bathtub", "Bed", "Bed", "Bee", "Bible", "Bible", "Bird", "Bomb", "Book", "Boss", "Bottle", "Bowl", "Box", "Boy", "Brain", "Bridge", "Butterfly", "Button", "Cappuccino", "Car", "Car-race", "Carpet", "Carrot", "Cave", "Chair", "Chess Board", "Chief", "Child", "Chisel", "Chocolates", "Church", "Church", "Circle", "Circus", "Circus", "Clock", "Clown", "Coffee", "Coffee-shop", "Comet", "Compact Disc", "Compass", "Computer", "Crystal", "Cup", "Cycle", "Data Base", "Desk", "Diamond", "Dress", "Drill", "Drink", "Drum", "Dung", "Ears", "Earth", "Egg", "Electricity", "Elephant", "Eraser", "Explosive", "Eyes", "Family", "Fan", "Feather", "Festival", "Film", "Finger", "Fire", "Floodlight", "Flower", "Foot", "Fork", "Freeway", "Fruit", "Fungus", "Game", "Garden", "Gas", "Gate", "Gemstone", "Girl", "Gloves", "God", "Grapes", "Guitar", "Hammer", "Hat", "Hieroglyph", "Highway", "Horoscope", "Horse", "Hose", "Ice", "Ice-cream", "Insect", "Jet fighter", "Junk", "Kaleidoscope", "Kitchen", "Knife", "Leather jacket", "Leg", "Library", "Liquid", "Magnet", "Man", "Map", "Maze", "Meat", "Meteor", "Microscope", "Milk", "Milkshake", "Mist", "Money $$$$", "Monster", "Mosquito", "Mouth", "Nail", "Navy", "Necklace", "Needle", "Onion", "PaintBrush", "Pants", "Parachute", "Passport", "Pebble", "Pendulum", "Pepper", "Perfume", "Pillow", "Plane", "Planet", "Pocket", "Post-office", "Potato", "Printer", "Prison", "Pyramid", "Radar", "Rainbow", "Record", "Restaurant", "Rifle", "Ring", "Robot", "Rock", "Rocket", "Roof", "Room", "Rope", "Saddle", "Salt", "Sandpaper", "Sandwich", "Satellite", "School", "Sex", "Ship", "Shoes", "Shop", "Shower", "Signature", "Skeleton", "Slave", "Snail", "Software", "Solid", "Space Shuttle", "Spectrum", "Sphere", "Spice", "Spiral", "Spoon", "Sports-car", "Spot Light", "Square", "Staircase", "Star", "Stomach", "Sun", "Sunglasses", "Surveyor", "Swimming Pool", "Sword", "Table", "Tapestry", "Teeth", "Telescope", "Television", "Tennis racquet", "Thermometer", "Tiger", "Toilet", "Tongue", "Torch", "Torpedo", "Train", "Treadmill", "Triangle", "Tunnel", "Typewriter", "Umbrella", "Vacuum", "Vampire", "Videotape", "Vulture", "Water", "Weapon", "Web", "Wheelchair", "Window", "Woman", "Worm", "X-ray" };
	for( ycSize_t i = 0; i != YC_ARRAY_SIZE(strings); ++i )
	{
		const char* str = strings[i];
		YC_TEST_CHECK( ycStringUtils::Length(str) == strlen(str) );
	}
	YC_TEST_PASS( "ycStringUtils::Length" );
}

#endif // YC_TEST
