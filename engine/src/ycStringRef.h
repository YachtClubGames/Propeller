#pragma once

/*! \class ycStringRef

  ycStringRef represents a lightweight non-mutable string that REFERENCES its data from elsewhere.

  This means the following restrictions:
  - ycStringRef's are read-only.
  - ycStringRef's are not NUL-terminated.
  - ycStringRef's are not directly convertible to a C-style char* string
  - ycStringRef is NOT meant to be a polymorphic class! Do not use ycStringRef pointers to
    manage the lifetimes of derived classes or to call Clear() or Set() on derived classes.

  Note, classes do inherit ycStringRef that DO mutate the underlying string.

*/

#include "ycCommon.h"

// core
#include "ycHash.h"
#include "ycIsPOD.h"

template< class t_type, const bool t_isPOD >
class ycVector;

#define STRINGREF_FMT "%.*s"
#define STRINGREF_ARG(s) (int)(s).Length(), (s).GetPtr()

// TODO:
// - the specific functionality of many methods is not clear, document them!
// - there is a handful of duplicate functionality from merging struse functionality in! de-dupe!

class ycStringRef
{
public:

	ycStringRef();
	inline ~ycStringRef() = default;
	explicit ycStringRef( const char* str );
	explicit ycStringRef( const char* str, const ycSize_t len );
	template< size_t t_len > ycStringRef( const char (&str)[ t_len ] ); //!< support implicit casting from "constant string"; note that in the presence of both ycStringRef and const char* overloads, the compiler will probably choose the const char* version
	template< size_t t_len > ycStringRef( char (&str)[ t_len ] ); // but try to catch char buf[64]
	ycStringRef( const ycStringRef& other );

	void Clear();
	
	const char* Get() const; //!< warning: there is no guarantee that this is NUL terminated
	const char* GetPtr( const ycSize_t offset ) const; //!< warning: there is no guarantee that this is NUL terminated
	const char* GetPtr() const { return m_str; } //!< warning: there is no guarantee that this is NUL terminated
	const u8* GetBytes() const { return (const u8*)GetPtr(); } //!< warning: there is no guarantee that this is NUL terminated
	char GetChar( const ycSize_t charIdx ) const;
	ycSize_t Length() const;

	ycStringRef& operator++() { if( m_len ) { m_str++; m_len--; } return *this; }
	ycStringRef operator+( ycSize_t skip ) { if( skip < m_len ) return ycStringRef( m_str + skip, m_len - skip); return ycStringRef(); }
	ycStringRef operator+( int skip ) { if( ycSize_t(skip) < m_len ) return ycStringRef( m_str + skip, m_len - skip); return ycStringRef(); }
	void operator+=( ycSize_t skip) { if( skip < m_len ) { m_str += skip; m_len -= skip; } else { Clear(); } }
	char operator[](ycSize_t pos) const { return pos < m_len ? m_str[pos] : 0; }
	explicit operator bool() const { return !!m_len; }

	bool operator== ( const ycStringRef& other ) const { return Equals( other ); }
	bool operator!= ( const ycStringRef& other ) const { return !Equals( other ); }

	void Set( const char* str );
	void Set( const char* str, const ycSize_t len );
	void Set( const ycStringRef& strRef );

	bool IsNull() const; //!< null ptr
	bool IsEmpty() const; //!< null ptr or zero length
	
	bool Equals( const char* str ) const;
	bool Equals( const char* str, const ycSize_t len ) const;
	bool Equals( const ycStringRef& other ) const;

	bool EqualsI( const char* str ) const;
	bool EqualsI( const char* str, const ycSize_t len ) const;
	bool EqualsI( const ycStringRef& other ) const;

	s32 Compare( const char* str ) const;	//! -1 if this is alphabetically prior to str, 0 if same str, 1 if successor to str
	s32 CompareLower( const char * str ) const;
	s32 CompareI( const char* str ) const { return CompareLower( str ); } // case-insentive compare -- CompareLower works for everything but turkish :P

	s32 Compare( const ycStringRef& other ) const;	//! -1 if this is alphabetically prior to str, 0 if same str, 1 if successor to str
	s32 CompareLower( const ycStringRef& other ) const;
	s32 CompareI( const ycStringRef& other ) const { return CompareLower( other ); } // case-insentive compare -- CompareLower works for everything but turkish :P

	bool IsLower() const;

	size_t CopyTo( char* dest, ycSize_t space ) const; //! copies string to dest, fits within space, returns number of bytes up to terminating 0
	void Advance( const ycSize_t len = 1 );

	char First() const;
	char At( const ycSize_t pos ) const; //!< returns '\0' if pos is beyond the string ref
	char Last() const;

	ycStringRef Left( const ycSize_t len ) const; //!< returns a string ref to the left (beginning) N characters of the string ref; asserts if too many characters are requested
	ycStringRef TakeLeft( const ycSize_t len );

	ycStringRef RightOf( const ycSize_t index ) const; //!< returns a string ref to the right of the first N characters of the string ref; asserts if too many characters are requested

	ycStringRef Right( const ycSize_t len ) const; //!< returns a string ref to the right (ending) N characters of the string ref; asserts if too many characters are requested
	ycStringRef TakeRight( const ycSize_t len );

	ycStringRef TakeWord();
	ycStringRef Word() const;

	bool BeginsWith( const char c ) const;
	bool BeginsWith( const char* str ) const;
	bool BeginsWith( const char* str, const ycSize_t len ) const;
	bool BeginsWith( const ycStringRef& str ) const;
	
	bool BeginsWithI( const char* str ) const;
	bool BeginsWithI( const char* str, const ycSize_t len ) const;
	bool BeginsWithI( const ycStringRef& str ) const;
	
	bool EndsWith( const char* str ) const;
	bool EndsWith( const char* str, const ycSize_t len ) const;
	bool EndsWith( const ycStringRef& str ) const;
	
	bool EndsWithI( const char* str ) const;
	bool EndsWithI( const char* str, const ycSize_t len ) const;
	bool EndsWithI( const ycStringRef& str ) const;
	
	bool Contains( const char* contains ) const; //! Returns true if string contains the substring 'contains; asserts if NULL
	bool Contains( const ycStringRef& contains ) const; //! Returns true if string contains the substring 'contains; 
	bool ContainsI( const char* contains ) const; //! Returns true if string contains the substring 'contains, ignoring case; asserts if NULL
	bool ContainsI( const ycStringRef& contains ) const; //! Returns true if string contains the substring 'contains, ignoring case; 
	
	//! Find in string
	s32 Find( char c ) const; //!< returns offset to first instance of character c in string or -1 if not found
	s32 Find( char c, char d ) const; //!< returns offset to first instance of character c or d in string or -1 if not found
	s32 FindLast( char c ) const; //!< returns offset to first instance of character c in string or -1 if not found
	s32 FindLast( char c, char d ) const; //!< returns offset to first instance of character c or d in string or -1 if not found
	s32 FindLast( const char* str ) const; //!< returns offset to last instance of str in string or -1 if not found
	s32 FindLast( ycStringRef str ) const; //!< returns offset to last instance of str in string or -1 if not found
	s32 FindAtI( const char* str, ycSize_t pos = 0 ) const;
	s32 FindAtI( ycStringRef str, ycSize_t pos = 0 ) const;
	s32 FindI( const char * str ) const { return FindAtI( str ); }
	s32 FindI( ycStringRef str ) const { return FindAtI( str ); }

	s32 FindAt( char c, ycSize_t pos ) const;
	s32 FindAt( const char* str, ycSize_t pos = 0 ) const;
	s32 FindAt( ycStringRef str, ycSize_t pos = 0 ) const;
	s32 Find( const char * str ) const { return FindAt( str ); }
	s32 Find( ycStringRef str ) const { return FindAt( str ); }
	s32 FindAnyOf( ycStringRef set, ycSize_t offset ) const;
	ycSize_t LineEndPos( ycSize_t pos ) const;
	ycSize_t LineStartPos( ycSize_t pos ) const;
	ycSize_t LinePrevPos( ycSize_t pos ) const;
	ycSize_t LineNextPos( ycSize_t pos ) const;
	ycSize_t LineNumber( ycSize_t pos ) const;
	bool SkipFirstIf( char first ); //!< if first character matches argument, take first and return true

	//! Range matching
	ycSize_t EscapeCode( u8& code ) const; //!< returns length of escape code ("\n", etc.) and the parsed code
	u8 TakeFirstEsc();
	bool CharInRange( char c ) const; //!< Is c in any range described by this string, regexp bracketed char ( [a-zA-Z] )? (excluding check for initial !)
	bool CharInRangeEx( char c ) const; //!< Is c in any range described by this string, regexp bracketed char ( [a-zA-Z] or [!0-9a-fA-F] )? (including check for initial !)
	s32 FindRangeChar( const ycStringRef range, bool include ) const;
	s32 FindRangeCharEx( ycStringRef range, ycSize_t pos ) const;
	s32 FindWithEsc( const ycStringRef str, ycSize_t pos ) const;
	s32 FindWithEscI( const ycStringRef str, ycSize_t pos ) const;
	s32 FindRangeWithEsc( const ycStringRef str, const ycStringRef range, ycSize_t pos ) const;
	s32 FindRangeWithEscI( const ycStringRef str, ycStringRef range, ycSize_t pos ) const;
	s32 FindRangeWithinRangeOf( ycStringRef range_find, ycStringRef range_within, ycSize_t pos ) const;
	ycSize_t LengthEsc(); //!< length of string with escape characters counting as one
	bool EqualsEsc( const ycStringRef str, ycSize_t pos ) const;
	ycSize_t MatchRangeLength( const ycStringRef range, ycSize_t pos = 0 ) const;
	bool MatchRange( const ycStringRef range, ycSize_t pos = 0 ) const { return MatchRangeLength( range, pos ) == m_len; }
	ycStringRef SplitRange( const ycStringRef range ) { return Split( MatchRangeLength( range ) ); }

	ycStringRef FindWildcard( const ycStringRef wild, ycSize_t start = 0, bool case_sensitive = true );
	////!< Give a string a score based on fuzzy matching of a filter https://medium.com/@Srekel/implementing-a-fuzzy-search-algorithm-for-the-debuginator-cacc349e6c55
	ycSize_t FuzzyScore( const ycStringRef filter );

	//! Divide string
	ycStringRef Split( ycSize_t pos ); //!< return the string up to pos, remove pos characters from this string
	ycStringRef Line(); //!< return the current line without line break and step the current string ahead to the next line

	//! Part of strings
	ycStringRef SubStr( ycSize_t offs, ycSize_t len ) const;
	ycStringRef ClipTrail( ycSize_t len ) const;
	ycStringRef Skip( ycSize_t skip ) const;
	ycStringRef SkipBOM() const;
	ycStringRef BetweenLast( char open, char close ) const;

	ycStringRef BetweenLast( char open, char openAlt, char close ) const;

	//! Whitespace
	ycSize_t WhiteLen( ycSize_t pos = 0 ) const;
	ycSize_t WhiteTrailLen() const;
	void SkipWS() { Advance( WhiteLen() ); }
	void SkipTrailWS() { m_len -= WhiteTrailLen(); }
	ycStringRef GetSkipWS() const { return Skip( WhiteLen() ); }
	ycStringRef GetClipTrailWS() const { return ClipTrail( WhiteTrailLen() ); }

	ycSize_t LengthSeparator( ycSize_t pos ) const;
	ycSize_t LengthNonSeparator( ycSize_t pos ) const;

	u32 HashIgnoreWhitespace() const; // TODO HACK FIXME: this should probably be renamed checksum or something; or changed to use our standard ycHash functions to avoid confusion

	//! Parsing magic
	ycSize_t CommentLength( ycSize_t pos = 0 ) const;
	ycSize_t ScopeLengthCommented( ycSize_t pos = 0 ) const;
	bool AdvanceIfFirst( char c ) { if( m_len && *m_str==c ) { m_str++; m_len--; return true; } return false; }
	ycStringRef SplitWordOrQuote();
	ycStringRef SplitLang(); //!< split string based on common programming tokens (words, quotes, scopes, numbers)
	ycSize_t LengthNumeric( ycSize_t pos = 0 ) const;
	ycSize_t LengthHex( ycSize_t pos = 0 ) const;
	ycSize_t LengthFloat( ycSize_t pos = 0 ) const;

	//! Remove strings from the beginning, if they match
	/*!
	These will perform a length check, so you may want to avoid them if your code is already checking length
	*/
	char TakeFirst();
	bool Take( const char ch );
	bool Take( const char* str );
	bool Take( const char* str, const ycSize_t len );
	bool Take( const ycStringRef str );
	bool TakeI( const ycStringRef str );

	//! Remove strings from the end, if they match
	/*!
	These will perform a length check, so you may want to avoid them if your code is already checking length.
	*/
	char TakeLast();
	bool TakeEnd( const char ch );
	bool TakeEnd( const char* str );
	bool TakeEnd( const char* str, const ycSize_t len );
	bool TakeEnd( const ycStringRef str );
	bool TakeEndI( const char* str );
	bool TakeEndI( const char* str, const ycSize_t len );
	bool TakeEndI( const ycStringRef str );

	//! Split this into two separated by the first token, remove whitespace from string
	ycStringRef TokenSplit( char token );

	ycVector< ycStringRef, true > SplitList( const char * findStr ) const;

	ycHashKey_t Hash() const; //!< A convenience function for ycHashBytes( Get(), Length() )
	u32 Hash32() const;       //!< A convenience function for ycHash32( Get(), Length() )
	u64 Hash64() const;       //!< A convenience function for ycHash64( Get(), Length() )
	
	u32 Hash32EmptyZero() const;       //!< A convenience function for ycHash32( Get(), Length() )
	u64 Hash64EmptyZero() const;       //!< A convenience function for ycHash64( Get(), Length() )

	bool ToBool() const; //!< returns true unless the value is equal to 'false' [case-insensitive], 0, 0.0 or -0.0 (note: things like 0000 or .0 will return true!)

	s64 ToInt( bool* converted = nullptr ) const;
	s64 ToIntExt() const;
	u64 ToUInt( bool* converted = nullptr ) const;
	u64 ToUIntExt() const;
	u64 ToHexUInt( bool* converted = nullptr ) const;

	f32 ToF32() const; // TODO HACK FIXME: add a way to indicate failure like the integral conversions!
	f64 ToF64() const;

	enum
	{
		kEvalResult_Valid,
		kEvalResult_Invalid,
		kEvalResult_ParentheticalMismatch,
		kEvalResult_UnexpectedChar,
		kEvalResult_Count
	};

	s32 Eval( f32* output ) const;

	ureg CleanFloat( ureg max_fraction_digits = 2 );

	bool AlphaLessThan( const ycStringRef& other ) const;
	bool AlphaLessThanI( const ycStringRef& other ) const;

	//! Get the string after the first token
	ycStringRef NextToken( char token );

	/*! Parsing Utilities */

	ycStringRef TakeIdentifier(); //!< parses a C identifier from the start of the string ref and advances past it; returns a null ycStringRef if there was no identifier
	bool TakeIdentifier( const ycStringRef& identifier ); //!< same as above, but only if it matches the passed identifier

	ycSize_t TrimLeadingWhitespace(); //!< returns the number of characters trimmed
	ycSize_t TrimLeadingHorizWhitespace(); //!< no newlines
	ycSize_t TrimTrailingWhiteSpace();
	ycSize_t TrimSurroundingWhiteSpace();
	ycSize_t TrimTrailingHorizWhiteSpace();
	ycSize_t TrimLeadingComment(); //!< returns the number of characters trimmed; returns ycSize_t(-1) if a comment was found but was not terminated, and no characters were trimmed; for single line comments, the trailing newline is NOT consumed

	ycSize_t LineLength( const bool includeLineEnd ) const;
	ycSize_t LineLengthWithComments( const bool includeLineEnd ) const; //!< parses c style comments and ignores newlines inside multi-line comments; returns ycSize_t(-1) if a comment was found but was not terminated

	//ycStringRef TakeLine(); // includes the terminating newline, the last line returned may not have a newline, returns a null ycStringRef if used on an empty string

	/*! File path utilities */

	ycStringRef FileBaseName() const; //!< removes preceding directories and trailing extensions
	ycStringRef PathName() const; //!< removes everything after the last \ or /
	ycStringRef FileName() const; //!< removes preceding directories
	ycStringRef Extension( bool onlyFirstExtension = false) const; //!< file extension (including '.'), empty if none
	ycStringRef NoExtension() const; //!< removes any trailing extensions
	ycStringRef CdUp() const; //!< removes everything after the last \ or / but does not leave name
	ycStringRef FunctionName() const;

protected:

	/*const*/ char* m_str; //note, this is actually const within this class, but we want other classes to inherit from string ref that are not const
	ycSize_t    m_len;
};

template< size_t t_len >
ycStringRef::ycStringRef( const char (&str)[t_len] )
{
	if( t_len )
	{
		ycAssert( ycStringRef( (const char*)(str) ).Length() == t_len - 1 );
		Set( str, t_len-1/*NUL*/ );
	}
	else
	{
		Set( nullptr, 0 );
	}
}

template< size_t t_len >
ycStringRef::ycStringRef( char (&str)[t_len] )
{
	Set( str );
}

inline ycHashKey_t ycHashKey( const ycStringRef& key ) { return key.Hash(); }

YC_IS_POD( ycStringRef );
