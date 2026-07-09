#pragma once

#include "ycCommon.h"

#include <stdarg.h> // va_list

/*! \class ycStringUtils

Implements utility functions for C Strings.

*/

struct ycVec2;
struct ycVec3;
struct ycVec4;
struct ycMtx4;

namespace ycStringUtils
{
	//! Copies bytes from src into dst until a NUL byte is copied
	/*!
	* Asserts if either argument is NULL
	* Returns the length of the copied string
	*/
	ycSize_t Copy( char* YC_RESTRICT dst, const char* YC_RESTRICT src );

	ycSize_t Copy( char* YC_RESTRICT dst, const char* YC_RESTRICT src, ycSize_t space ); // TODO HACK FIXME DISCUSS: should this assert rather than truncating? implicitely truncating seems dangerous

	void Append( char* dst, const char* str );
	void Append( char* dst, const ycSize_t start, const char* str );

	//! Returns the length of a c-string; asserts if str is NULL
	ycSize_t Length( const char* str );

	//! Returns the length of a c-string; asserts if str is NULL
	ycSize_t Length( const char* str, u32 maxLength, bool * foundEnd );

	/*!
	* Assumes the string start and allocation sizes are word aligned; potentially reads slightly beyond the end of the string
	*/
	ycSize_t LengthAligned( const char* str );

	//! Returns true if 'string' starts with 'beginsWith'; asserts in either are NULL
	bool BeginsWith( const char* string, const char* beginsWith );

	//! Returns true if 'string' starts with 'beginsWith', ignoring case; asserts in either are NULL
	bool BeginsWithI( const char* string, const char* beginsWith );

	//! Returns true if 'string' starts with 'beginsWith'; asserts in either are NULL
	/*!
	This does not depend on NULL termination of 'beginsWith', but uses the length argument instead.
	*/
	bool BeginsWith( const char* string, const char* beginsWith, const ycSize_t length );
	bool BeginsWithI( const char* string, const char* beginsWith, const ycSize_t length );

	//! Case sensitive string equality check; asserts if a or b are NULL
	bool Equals( const char* a, const char* b );

	//! Case insensitive string equality check; asserts if a or b are NULL
	bool EqualsI( const char* a, const char* b );

	//! An alias for BeginsWith + length, since they are the same thing
	inline bool Equals( const char* a, const char* b, const ycSize_t length ) { return BeginsWith( a, b, length ); }
	inline bool EqualsI( const char* a, const char* b, const ycSize_t length ) { return BeginsWithI( a, b, length ); }

	//! Returns true if 'string' contains the substring 'contains; asserts if either are NULL
	bool Contains( const char* string, const char* contains );
	
	//! Returns true if 'string' contains the substring 'contains, ignoring case; asserts if either are NULL
	bool ContainsI( const char* string, const char* contains );
	
	//! Returns true if 'string' contains the character 'contains; asserts if 'string' is NULL
	bool Contains( const char* string, const char contains );
	
	//! Returns true if 'string' contains the character 'contains, ignoring case; asserts if 'string' is NULL
	bool ContainsI( const char* string, const char contains );

	//! Returns the lowercase version of a character, if it is within the range A-Z, otherwise returns the character unchanged.
	/*!
	This is ascii A-Z only, unlike the C++ locale-aware tolower
	*/
	char ToLower( const char ascii );

	//! Returns if a character is lowercase
	/*!
	This is ascii A-Z only, unlike the C++ locale-aware tolower
	*/
	bool IsLower( const char ascii );

	//! Returns the uppercase version of a character, if it is within the range a-z, otherwise returns the character unchanged.
	/*!
	This is ascii a-z only, unlike the C++ locale-aware tolower
	*/
	char ToUpper( const char ascii );

	//! Currently thin wrappers around atof()
	float ToFloat( const char* string ); // TODO HACK FIXME: add a way to check for conversion success
	double ToDouble( const char* string ); // TODO HACK FIXME: add a way to check for conversion success

	//! 
	int32_t ToInt( const char* string ); // TODO HACK FIXME: add a way to check for conversion success
	int32_t ToInt( const char* string, const ycSize_t len );

	//! Convert ints and floats to string
	// TODO: How should we handle the buffer size? Truncating might be dangerous
	u32 ToString( char* buff, const ycSize_t buffSize, const u32 val );
	u32 ToString( char* buff, const ycSize_t buffSize, const s32 val );
	u32 ToString( char* buff, const ycSize_t buffSize, const u64 val );
	u32 ToString( char* buff, const ycSize_t buffSize, const s64 val );
	u32 ToString( char* buff, const ycSize_t buffSize, const f32 val );
	u32 ToString( char* buff, const ycSize_t buffSize, const f64 val );
	u32 ToString( char* buff, const ycSize_t buffSize, const ycVec2& vec );
	u32 ToString( char* buff, const ycSize_t buffSize, const ycVec3& vec );
	u32 ToString( char* buff, const ycSize_t buffSize, const ycVec4& vec );
	u32 ToString( char* buff, const ycSize_t buffSize, const ycMtx4& mat );
	u32 ToString( char* buff, const ycSize_t buffSize, const char* val );
	u32 ToString( char* buff, const ycSize_t buffSize, const void* ptr );
	u32 ToString( char* buff, const ycSize_t buffSize, bool val );

	// move some of these to ycParseUtils or something?

	//! Returns true for spaces, tabs, newlines, and carriage returns
	/*!
	* This does not check for vertical tables or form feeds!
	*/
	bool IsWhiteSpace( const char c );

	//! Returns true for non alphanumeric and non apostrophes
	bool IsSeparator( const char c );

	//! Does not include newlines
	bool IsHorizWhiteSpace( const char c );

	//! Returns true the character is a valid C identifier
	/*!
	* if you wish to fully abide by C standard, the first character should be checked differently.
	*/
	bool IsIdentifier( const char c );
	bool IsIdentifierFirstChar( const char c );

	//! Returns true if the character represents [0-9]
	bool IsDigit( const char c );

	//! Returns true if the character represents [a-zA-Z]
	bool IsAlpha( const char c );

	ycSize_t CountTrailingWhiteSpace( const char* start, const ycSize_t length );
	ycSize_t RemoveSubStr( const char* string, ycSize_t length, char* subStr, ycSize_t subStrLen );
	ycSize_t InsertSubStr( char* string, ycSize_t length, ycSize_t capacity, ycSize_t insert, const char* subStr, ycSize_t subStrLen );
	ycSize_t SprintF( char * buf, ycSize_t bufSize, const char * format, ... );
	ycSize_t VSprintF( char * buf, ycSize_t bufSize, const char * format, va_list argList );
	ycSize_t SprintF_NoCheck( char * buf, ycSize_t bufSize, const char * format, ... );
	ycSize_t VSprintF_NoCheck( char * buf, ycSize_t bufSize, const char * format, va_list argList );
	s32 ScanF( const char * string, const char * format, ... );
	ycSize_t ReadableFloatF( char* buf, ycSize_t bufSize, const char* format, f32 value );
	ycSize_t ReadableFloatF( char* buf, ycSize_t bufSize, const char* format, f64 value );
	ycSize_t CleanupFloats( char* buf );

	/*!<
	mostly a testing/verification thing; can be useful when comparing human-editable files, but be cautious using this for
	'real' as _some_ whitespace is certainly important! eg this says 'asdf' and 'as df' are equal!
	*/
	bool CompareIgnoringWhitspace( const char* a, const char* b );
	bool CompareIgnoringWhitspace( const char* a, ycSize_t aLen, const char* b, ycSize_t bLen );

	//! Returns a pointer to the first instance of `target` within `start` (excluding null terminator), or nullptr if not found
	const char* Find( const char* start, char target );

	//! Returns a pointer to the first instance of `target` within `start` up to (but not including) `start + length`, or nullptr if not found
	const char* Find( const char* start, ycSize_t length, char target );
};
