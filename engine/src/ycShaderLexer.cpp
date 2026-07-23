#include "ycShaderLexer.h"

// core
#include "ycAllocator.h"
#include "ycStd.h"
#include "ycStringUtils.h"

// std
#include <stdarg.h>
#include <stdio.h>

namespace ycShaderBuilder
{

	Lexer::Lexer( ycAllocator* mem, const ycSize_t tokenReserve ) :
		m_mem( mem ? mem : g_defaultMem ),
		m_tokens( m_mem ),
		m_types( m_mem ),
		m_identifiers( m_mem ),
		m_errorMsg( nullptr ),
		m_inParse( false )
	{
		m_tokens.Reserve( tokenReserve );
		InitIdentifiers();
		InitTypes();
	}

	Lexer::~Lexer()
	{
		if( m_errorMsg ) { YC_FREE( m_mem, m_errorMsg ); }
	}

	bool Lexer::Parse( const char* input, const ycSize_t len )
	{
		// warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
		// i know and have designed around it!
		#ifdef _MSC_VER
			#pragma warning( push )
			#pragma warning( disable : 4611 )
		#endif
		if( setjmp( m_jmpBuf ) == 0 )
		#ifdef _MSC_VER
			#pragma warning( pop )
		#endif
		{
			m_inParse = true;
			ycStringRef src = len == 0 ? ycStringRef( input ) : ycStringRef( input, len );
			m_sourceText = src;
			while( src.Length() )
			{
				// whitespace & comments
				SkipWhiteSpaceAndComments( &src );
				if( src.Length() == 0 ) { break; }

				// kToken_Literal
				if( ParseNumericLiteral( &src ) ) {}

				// kToken_ConstantString
				else if( ParseStringLiteral( &src ) ) {}

				// kToken_Identifier or kToken_TypeName or kToken_Keyword_*
				else if( ParseIdentifier( &src ) ) {}

				// kToken_Op_*
				else if( ParseOperator( &src ) ) {}

				// uh oh
				else
				{
					Error( src.GetPtr(), "No lexer rules matched." );
				}
			}

			// append EOF
			AppendToken( kToken_EOF, ycStringRef() );
			
			m_inParse = false;
			return true;
		}
		else
		{
			m_inParse = false;
			return false;
		}
	}
	
	void Lexer::AppendToken( TokenType type, const ycStringRef& str, const u32 extData )
	{
		TokenDesc* token = m_tokens.Append();
		token->type    = type;
		token->str     = str;
		token->extData = extData;
	}

	void Lexer::Error( const char* showCurPos, const char* fmt, ... )
	{
		ycAssert( m_errorMsg == nullptr );
		m_errorMsg = (char*)YC_ALLOC( m_mem, kErrBufSize, YC_MEM_DEFAULT_ALIGN, "ycShaderLexer Error Message" );
		va_list args;
		va_start( args, fmt );
		#ifdef YC_MSFT
			int bytesWritten = vsprintf_s( m_errorMsg, kErrBufSize, fmt, args );
			if( showCurPos )
			{
				bytesWritten += sprintf_s( m_errorMsg+bytesWritten, kErrBufSize-bytesWritten, "\nAt:\n%.*s", 64, showCurPos );
			}
		#else
			int bytesWritten = vsnprintf( m_errorMsg, kErrBufSize, fmt, args );
			if( showCurPos )
			{
				bytesWritten += snprintf( m_errorMsg+bytesWritten, kErrBufSize-bytesWritten, "\nAt:\n%.*s", 64, showCurPos );
			}
		#endif
		ycAssert( bytesWritten < kErrBufSize );
		va_end( args );
		if( m_inParse ) { longjmp( m_jmpBuf, 1 ); }
		else            { ycAssertMsg( false, m_errorMsg ); }
	}

	void Lexer::Clear()
	{
		m_tokens.Clear();
	}

	// TODO HACK FIXME: assuming 64 bits we should be pretty safe... but should probably still have hash collision checks :P

	void Lexer::InitTypes()
	{
		m_types.Reserve( 64 );
		ureg typeId;

		typeId = RegisterTypeName( ycStringRef( "void",   4 ) ); ycAssert( typeId == kIntrinsicType_Void );

		typeId = RegisterTypeName( ycStringRef( "float",  5 ) ); ycAssert( typeId == kIntrinsicType_Float );
		typeId = RegisterTypeName( ycStringRef( "int",    3 ) ); ycAssert( typeId == kIntrinsicType_Int );
		typeId = RegisterTypeName( ycStringRef( "uint",   4 ) ); ycAssert( typeId == kIntrinsicType_UInt );
		typeId = RegisterTypeName( ycStringRef( "bool",   4 ) ); ycAssert( typeId == kIntrinsicType_Bool );

		typeId = RegisterTypeName( ycStringRef( "float1", 6 ) ); ycAssert( typeId == kIntrinsicType_Float1 );
		typeId = RegisterTypeName( ycStringRef( "float2", 6 ) ); ycAssert( typeId == kIntrinsicType_Float2 );
		typeId = RegisterTypeName( ycStringRef( "float3", 6 ) ); ycAssert( typeId == kIntrinsicType_Float3 );
		typeId = RegisterTypeName( ycStringRef( "float4", 6 ) ); ycAssert( typeId == kIntrinsicType_Float4 );

		typeId = RegisterTypeName( ycStringRef( "int1", 4 ) ); ycAssert( typeId == kIntrinsicType_Int1 );
		typeId = RegisterTypeName( ycStringRef( "int2", 4 ) ); ycAssert( typeId == kIntrinsicType_Int2 );
		typeId = RegisterTypeName( ycStringRef( "int3", 4 ) ); ycAssert( typeId == kIntrinsicType_Int3 );
		typeId = RegisterTypeName( ycStringRef( "int4", 4 ) ); ycAssert( typeId == kIntrinsicType_Int4 );

		typeId = RegisterTypeName( ycStringRef( "uint1", 5 ) ); ycAssert( typeId == kIntrinsicType_UInt1 );
		typeId = RegisterTypeName( ycStringRef( "uint2", 5 ) ); ycAssert( typeId == kIntrinsicType_UInt2 );
		typeId = RegisterTypeName( ycStringRef( "uint3", 5 ) ); ycAssert( typeId == kIntrinsicType_UInt3 );
		typeId = RegisterTypeName( ycStringRef( "uint4", 5 ) ); ycAssert( typeId == kIntrinsicType_UInt4 );

		typeId = RegisterTypeName( ycStringRef( "bool1", 5 ) ); ycAssert( typeId == kIntrinsicType_Bool1 );
		typeId = RegisterTypeName( ycStringRef( "bool2", 5 ) ); ycAssert( typeId == kIntrinsicType_Bool2 );
		typeId = RegisterTypeName( ycStringRef( "bool3", 5 ) ); ycAssert( typeId == kIntrinsicType_Bool3 );
		typeId = RegisterTypeName( ycStringRef( "bool4", 5 ) ); ycAssert( typeId == kIntrinsicType_Bool4 );

		typeId = RegisterTypeName( ycStringRef( "float2x2", 8 ) ); ycAssert( typeId == kIntrinsicType_Float2x2 );
		typeId = RegisterTypeName( ycStringRef( "float2x3", 8 ) ); ycAssert( typeId == kIntrinsicType_Float2x3 );
		typeId = RegisterTypeName( ycStringRef( "float2x4", 8 ) ); ycAssert( typeId == kIntrinsicType_Float2x4 );

		typeId = RegisterTypeName( ycStringRef( "float3x2", 8 ) ); ycAssert( typeId == kIntrinsicType_Float3x2 );
		typeId = RegisterTypeName( ycStringRef( "float3x3", 8 ) ); ycAssert( typeId == kIntrinsicType_Float3x3 );
		typeId = RegisterTypeName( ycStringRef( "float3x4", 8 ) ); ycAssert( typeId == kIntrinsicType_Float3x4 );

		typeId = RegisterTypeName( ycStringRef( "float4x2", 8 ) ); ycAssert( typeId == kIntrinsicType_Float4x2 );
		typeId = RegisterTypeName( ycStringRef( "float4x3", 8 ) ); ycAssert( typeId == kIntrinsicType_Float4x3 );
		typeId = RegisterTypeName( ycStringRef( "float4x4", 8 ) ); ycAssert( typeId == kIntrinsicType_Float4x4 );
		
		typeId = RegisterTypeName( ycStringRef( "Tex2D",   5 ) ); ycAssert( typeId == kIntrinsicType_Tex2D );
		typeId = RegisterTypeName( ycStringRef( "Tex3D",   5 ) ); ycAssert( typeId == kIntrinsicType_Tex3D );
		typeId = RegisterTypeName( ycStringRef( "TexCube", 7 ) ); ycAssert( typeId == kIntrinsicType_TexCube );
		typeId = RegisterTypeName( ycStringRef( "Tex2DUint", 9 ) ); ycAssert( typeId == kIntrinsicType_Tex2DUint );
		typeId = RegisterTypeName( ycStringRef( "Tex2DMS", 7 ) ); ycAssert( typeId == kIntrinsicType_Tex2DMS );

		ycAssert( typeId+1 == kIntrinsicType_Count );
	}

	u32 Lexer::RegisterTypeName( const ycStringRef& typeName )
	{
		const ycHashKey_t hash      = ycHashBytes( typeName.GetPtr(), typeName.Length() );
		const ureg        typeIndex = m_types.Length();
		if( m_types.IndexOf( hash ) != -1 )
		{
			Error( typeName.GetPtr(), "Duplicate type name encountered. This can possibly be a hash collision, but it is probably a type being multiply-defined." );
		}
		m_types.Append( hash );
		return u32(typeIndex);
	}

	bool Lexer::IsTypeName( const ycStringRef& typeName, u32* typeIndex )
	{
		const ycHashKey_t hash = ycHashBytes( typeName.GetPtr(), typeName.Length() );
		return IsTypeName( hash, typeIndex );
	}

	bool Lexer::IsTypeName( const ycHashKey_t hash, u32* typeIndex )
	{
		const u32 numTypes = u32(m_types.Length());
		const ycHashKey_t* typeHashes = m_types.Begin();
		for( u32 i = 0; i != numTypes; ++i )
		{
			if( typeHashes[ i ] == hash )
			{
				*typeIndex = i;
				return true;
			}
		}
		return false;
	}

	void Lexer::InitIdentifiers()
	{
		m_identifiers.Reserve( 64 );
	}

	u32 Lexer::GetIdentifierIndex( const ycHashKey_t hash )
	{
		const ycSize_t numTypes = m_identifiers.Length();
		const ycHashKey_t* typeHashes = m_identifiers.Begin();
		ycSize_t i = 0;
		for( /**/; i != numTypes; ++i )
		{
			if( typeHashes[ i ] == hash ) { return u32(i); }
		}
		m_identifiers.Append( hash );
		return u32(i);
	}

	bool Lexer::ParseNumericLiteral( ycStringRef* input )
	{
		// TODO HACK FIXME: see comments below
		const char* src = input->GetPtr();
		ycSize_t    len = input->Length();
		//if( len && *src == '-' ) { ++src; --len; } // unary op, not part of literal -- should probably be???
		while( len && ycStringUtils::IsDigit( *src ) ) { ++src; --len; }
		
		if( src == input->GetPtr() )
		{
			return false;
		}

		if( src[0] == '.' && ycStringUtils::IsDigit( src[1] ) )
		{
			++src; --len; // .
			while( len && ycStringUtils::IsDigit( *src ) ) { ++src; --len; }
			if( len && *src == 'f' ) // should we include this in the token? or let the codegen add postfixes based on kIntrinsicType_ + context (or maybe better yet, should we have postfixes at all?)
			{
				++src;
				--len;
			}
			const ycSize_t literalLen = src - input->GetPtr();
			AppendToken( kToken_Literal, ycStringRef( input->GetPtr(), literalLen ), kIntrinsicType_Float );
			input->Advance( literalLen );
			return true;
		}
		else
		{
			u32 intrinsicType = kIntrinsicType_Int;

			if ( ycStringUtils::ToLower( src[0] ) == 'u' )
			{
				++src;
				--len;
				intrinsicType = kIntrinsicType_UInt;
			}

			const ycSize_t literalLen = src - input->GetPtr();
			AppendToken( kToken_Literal, ycStringRef( input->GetPtr(), literalLen ), intrinsicType );
			input->Advance( literalLen );
			return true;
		}
	}

	bool Lexer::ParseStringLiteral( ycStringRef* input )
	{
		// TODO HACK FIXME: escapes!
		const char* src = input->GetPtr();
		ycSize_t    len = input->Length();
		if( len && *src == '"' )
		{
			++src; --len; // "
			while( *src != '"' )
			{
				if( len == 0 ) { Error( input->GetPtr(), "Unterminated string literal." ); }
				++src; --len;
			}
			++src; --len; // "
			const ycSize_t literalLen = src - input->GetPtr();
			AppendToken( kToken_ConstantString, ycStringRef( input->GetPtr(), literalLen ) );
			input->Advance( literalLen );
			return true;
		}
		return false;
	}

	bool Lexer::ParseIdentifier( ycStringRef* input, const ureg expect )
	{
		ycStringRef identifier = input->TakeIdentifier();
		if( identifier.IsNull() ) { return false; }

		if( expect == kExpect_DontCare )
		{
			if( identifier.Equals( "typedef", 7 ) )
			{
				AppendToken( kToken_Keyword_Typedef, identifier );
				SkipWhiteSpaceAndComments( input );
				if( !ParseIdentifier( input, kExpect_TypeName    ) ) { Error( input->GetPtr(), "Expected type name identifier." ); }
				SkipWhiteSpaceAndComments( input );
				if( !ParseIdentifier( input, kExpect_NewTypeName ) ) { Error( input->GetPtr(), "Expected type name identifier." ); }
				return true;
			}
			if( identifier.Equals( "struct",  6 ) )
			{
				AppendToken( kToken_Keyword_Struct, identifier );
				SkipWhiteSpaceAndComments( input );
				if( !ParseIdentifier( input, kExpect_NewTypeName ) ) { Error( input->GetPtr(), "Expected type name identifier." ); }
				return true;
			}
			if( identifier.Equals( "if",      2 ) ) { AppendToken( kToken_Keyword_If,      identifier ); return true; }
			if( identifier.Equals( "else",    4 ) ) { AppendToken( kToken_Keyword_Else,    identifier ); return true; }
			if( identifier.Equals( "cbuffer", 7 ) ) { AppendToken( kToken_Keyword_CBuffer, identifier ); return true; }
			if( identifier.Equals( "return",  6 ) ) { AppendToken( kToken_Keyword_Return,  identifier ); return true; }
			if( identifier.Equals( "for",     3 ) ) { AppendToken( kToken_Keyword_For,     identifier ); return true; }
			if( identifier.Equals( "const",   5 ) ) { AppendToken( kToken_Keyword_const,   identifier ); return true; }
			if( identifier.Equals( "out",     3 ) ) { AppendToken( kToken_Keyword_out,     identifier ); return true; }
			if( identifier.Equals( "inout",   5 ) ) { AppendToken( kToken_Keyword_inout,   identifier ); return true; }
			if( identifier.Equals( "asm",     3 ) ) { AppendToken( kToken_Keyword_asm,     identifier ); return true; }
		}

		const ycHashKey_t hash = identifier.Hash();

		TokenType tokenType = {};
		u32 extData;
		if( IsTypeName( hash, &extData ) )
		{
			if( expect == kExpect_NewTypeName ) { Error( input->GetPtr(), "Expected new type name, but type has already been defined." ); }
			tokenType = kToken_TypeName;
		}
		else
		{
			if( expect == kExpect_NewTypeName )
			{
				extData = RegisterTypeName( identifier );
				tokenType = kToken_TypeName;
			}
			else if( expect == kExpect_TypeName )
			{
				Error( input->GetPtr(), "Expected type name." );
			}
			else
			{
				extData = GetIdentifierIndex( hash );
				tokenType = kToken_Identifier;
			}
		}

		AppendToken( tokenType, identifier, extData );

		return true;
	}

	bool Lexer::ParseOperator( ycStringRef* input )
	{
		const char* src = input->GetPtr();
		ycSize_t    len = input->Length();
		ycAssert( len ); // this shouldn't be able to be called with zero length; although if the lexer structure changes and this becomes possible just add an early-out
		
		TokenType tokenType = kToken_EOF;
		ycSize_t tokenLen = 1;

		const char src0 = src[0];
		const char src1 = len >= 2 ? src[1] : '\0';
		const char src2 = len >= 3 ? src[2] : '\0';
		if(      src0 == '+' )
		{
			if(      src1 == '=' ) { tokenType = kToken_Op_AddAssign; ++tokenLen; }
			else if( src1 == '+' ) { tokenType = kToken_Op_Inc;       ++tokenLen; }
			else                   { tokenType = kToken_Op_Plus; }
		}
		else if( src0 == '-' )
		{
			if(      src1 == '=' ) { tokenType = kToken_Op_SubAssign; ++tokenLen; }
			else if( src1 == '-' ) { tokenType = kToken_Op_Dec;       ++tokenLen; }
			else                   { tokenType = kToken_Op_Dash; }
		}
		else if( src0 == '*' )
		{
			if( src1 == '=' ) { tokenType = kToken_Op_MulAssign; ++tokenLen; }
			else              { tokenType = kToken_Op_Star; }
		}
		else if( src0 == '/' )
		{
			if( src1 == '=' ) { tokenType = kToken_Op_DivAssign; ++tokenLen; }
			else              { tokenType = kToken_Op_Slash; }
		}
		else if( src0 == '%' )
		{
			if( src1 == '=' ) { tokenType = kToken_Op_ModAssign; ++tokenLen; }
			else              { tokenType = kToken_Op_Mod;; }
		}
		else if( src0 == '=' )
		{
			if( src1 == '=' ) { tokenType = kToken_Op_Equals; ++tokenLen; }
			else              { tokenType = kToken_Op_Assign; }
		}
		else if( src0 == '&' )
		{
			if(      src1 == '&' ) { tokenType = kToken_Op_LogicalAnd; ++tokenLen; }
			else if( src1 == '=' ) { tokenType = kToken_Op_AndAssign;  ++tokenLen; }
			else                   { tokenType = kToken_Op_BitwiseAnd; }
		}
		else if( src0 == '|' )
		{
			if(      src1 == '|' ) { tokenType = kToken_Op_LogicalOr; ++tokenLen; }
			else if( src1 == '=' ) { tokenType = kToken_Op_OrAssign;  ++tokenLen; }
			else                   { tokenType = kToken_Op_BitwiseOr; }
		}
		else if( src0 == '^' )
		{
			if( src1 == '=' ) { tokenType = kToken_Op_XorAssign; ++tokenLen; }
			else              { tokenType = kToken_Op_BitwiseXor; }
		}
		else if( src0 == ';' ) { tokenType = kToken_Op_Semicolon; }
		else if( src0 == ',' ) { tokenType = kToken_Op_Comma; }
		else if( src0 == '{' ) { tokenType = kToken_BeginBlock; }
		else if( src0 == '}' ) { tokenType = kToken_EndBlock; }
		else if( src0 == '(' ) { tokenType = kToken_LeftParen; }
		else if( src0 == ')' ) { tokenType = kToken_RightParen; }
		else if( src0 == '[' ) { tokenType = kToken_LeftBracket; }
		else if( src0 == ']' ) { tokenType = kToken_RightBracket; }
		else if( src0 == ':' ) { tokenType = kToken_Op_Colon; }
		else if( src0 == '.' ) { tokenType = kToken_Op_Dot; }
		else if( src0 == '~' ) { tokenType = kToken_Op_BitwiseNot; }
		else if( src0 == '!' )
		{
			if( src1 == '=' ) { tokenType = kToken_Op_NEquals; ++tokenLen; }
			else              { tokenType = kToken_Op_Not;  }
		}
		else if( src0 == '<' )
		{
			if(      src1 == '=' ) { tokenType = kToken_Op_LEquals; ++tokenLen; }
			else if( src1 == '<' )
			{
				if( src2 == '=' ) { tokenType = kToken_Op_LShiftAssign; tokenLen = 3; }
				else              { tokenType = kToken_Op_LeftShift;    ++tokenLen; }
			}
			else { tokenType = kToken_Op_Less; }
		}
		else if( src0 == '>' )
		{
			if(      src1 == '=' ) { tokenType = kToken_Op_GEquals; ++tokenLen; }
			else if( src1 == '>' )
			{
				if( src2 == '=' ) { tokenType = kToken_Op_RShiftAssign; tokenLen = 3; }
				else              { tokenType = kToken_Op_RightShift;   ++tokenLen; }
			}
			else { tokenType = kToken_Op_Greater; }
		}

		if( tokenType != kToken_EOF )
		{
			AppendToken( tokenType, input->TakeLeft( tokenLen ) );
			return true;
		}
		else
		{
			return false;
		}
	}

	void Lexer::SkipWhiteSpaceAndComments( ycStringRef* src )
	{
		for(;;)
		{
			ycSize_t trimmed;
			if( src->TrimLeadingWhitespace() ) {}
			else if( (trimmed = src->TrimLeadingComment()) != 0 )
			{
				if( trimmed == ycSize_t(-1) ) { Error( src->GetPtr(), "Unterminated multi-line comment." ); }
			}
			else { break; }
		}
	}
	
	bool Lexer::GetLineAndColumn( const char* token, ycStringRef& line, int& lineNo, int& colNo ) const
	{
		if( m_sourceText.IsNull() || token < m_sourceText.Get() || token >= m_sourceText.Get() + m_sourceText.Length() )
		{ return false; }

		lineNo = 0;
		colNo = 0;
		const char* begin = m_sourceText.Get();
		const char* lineBegin = begin;
		while( begin < token && begin - m_sourceText.Get() < (s64)m_sourceText.Length() ) {
			colNo++;
			if( *begin == '\n' ) {
				lineNo++;
				colNo = 0;
				lineBegin = begin+1;
			}
			begin++;
		}

		if( begin - m_sourceText.Get() >= (s64)m_sourceText.Length() ) {
			return false;
		}
		else {
			const char* lineEnd = ycStringUtils::Find(lineBegin, '\n');
			if( lineEnd ) {
				line = ycStringRef( lineBegin, lineEnd - lineBegin );
			}
			else {
				line = ycStringRef( lineBegin );
			}

			return true;
		}
	}

} // namespace ycShaderBuilder
