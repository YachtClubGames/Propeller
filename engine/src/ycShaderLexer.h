#pragma once
#ifndef YC_SHADERLEXER_H
#define YC_SHADERLEXER_H

#include "ycCommon.h"

// core
#include "ycHash.h"
#include "ycStringRef.h"
#include "ycVector.h"

// shader builder
#include "ycShaderTokens.h"

// error handling, forgive me
#include <setjmp.h>

namespace ycShaderBuilder
{
	class Parser;
	class Lexer
	{
	public:

		Lexer( ycAllocator* mem = nullptr, const ycSize_t tokenReserve = 8192 );
		~Lexer();

		/*!
		* if len == 0, assumes input is NUL terminated
		* returns false if parsing failed, use GetError() to retrieve the error message
		*/
		bool Parse( const char* input, const ycSize_t len = 0 );
		inline const char* GetError() const { return m_errorMsg; }

		void Clear();

	protected:
		friend class ycShaderBuilder::Parser;

		ycAllocator*            m_mem;
		ycVector< TokenDesc >   m_tokens;
		ycVector< ycHashKey_t > m_types;
		ycVector< ycHashKey_t > m_identifiers;

		ycStringRef m_sourceText;

		void AppendToken( TokenType type, const ycStringRef& str, const u32 extData = 0 );

		// error handling
		#ifdef YC_MSVC // structure was padded due to alignment specifier
			__pragma( warning( push ) )
			__pragma( warning( disable: 4324 ) )
		#endif
		jmp_buf m_jmpBuf;
		#ifdef YC_MSVC
			__pragma( warning( pop ) )
		#endif
		char*   m_errorMsg;
		bool    m_inParse; //!< some functions can occur outside of Parse(), let Error() know to just assert instead
		enum { kErrBufSize = 4096 };
		void Error( const char* showCurPos, const char* fmt, ... );

		// types and identifiers
		void InitTypes();
		u32 RegisterTypeName( const ycStringRef& typeName );
		bool IsTypeName( const ycStringRef& typeName, u32* typeIndex );
		bool IsTypeName( const ycHashKey_t hash, u32* typeIndex );
		void InitIdentifiers();
		u32 GetIdentifierIndex( const ycHashKey_t hash ); //!< registers the identifier if it does not exist, unlike the typename functions

		// tokens
		bool ParseNumericLiteral( ycStringRef* input );
		bool ParseStringLiteral( ycStringRef* input );
		enum
		{
			kExpect_DontCare = 0,
			kExpect_NewTypeName,
			kExpect_TypeName
		};
		bool ParseIdentifier( ycStringRef* input, const ureg expect = kExpect_DontCare );
		bool ParseOperator( ycStringRef* input );

		// utils
		void SkipWhiteSpaceAndComments( ycStringRef* src );

		bool GetLineAndColumn( const char* token, ycStringRef& line, int& lineNo, int& colNo ) const;
	};

} // namespace ycShaderBuilder

#endif // !YC_SHADERLEXER_H
