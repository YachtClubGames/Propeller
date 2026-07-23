#include "ycCPP.h"

/* TODO
- Still probably some asserts to replace with Error() calls
- Error/warn when there is useless junk after a directive
- pragma
- pseudo 'ffi', hook c functions into macros

- LOTS OF CLEAN UP!
*/

// core
#include "ycAllocator.h"
#include "ycBuilder.h"
#include "ycByteBuffer.h"
#include "ycFile.h"
#include "ycStackString.h"
#include "ycStd.h"
#include "ycStringUtils.h"

// std
#include <stdarg.h>
#include <stdio.h>

namespace
{
	enum
	{
		kMaxMacroArgs = 16 // far below std spec; if we raise this some stuff may need to be moved off of the stack
	};
}

ycCPP::ycCPP( ycAllocator* mem )
	: m_mem( mem ? mem : g_defaultMem )
	, m_ignoreDepth( 0 )
	, m_trueSeen( false )
	, m_includePath( nullptr )
	, m_includes( m_mem ) //!< no initial reserve; assume most files do not use #include, reserve later if we find that we'll need it
	, m_macros( "CPP Macros", m_mem )
	, m_errorMsg( nullptr )
	, m_inParse( 0 )
{
}

void ycCPP::Reset()
{
	m_ignoreDepth = 0;
	m_trueSeen = false;
	for( ycChainedHash< ycHashKey_t, Macro >::Iter iter = m_macros.Begin(); iter.IsValid(); ++iter )
	{
		FreeInterpolants( &iter.GetValue() );
	}
	m_macros.Clear();
	if( m_errorMsg )
	{
		YC_FREE( m_mem, m_errorMsg );
		m_errorMsg = nullptr;
	}
	m_inParse = 0;

	while( m_includes.Length() )
	{
		Include& inc = m_includes.Last();
		YC_FREE( g_defaultMem, (void*)inc.fileData );
		m_includes.TakeLast();
	}
}

ycCPP::~ycCPP()
{
	Reset();
	for( ycChainedHash< ycHashKey_t, Macro >::Iter iter = m_macros.Begin(); iter.IsValid(); ++iter )
	{
		FreeInterpolants( &iter.GetValue() );
	}
}

bool ycCPP::Parse( const char* in, ycByteBuffer* out )
{
	return Parse( ycStringRef( in ), out );
}

bool ycCPP::Parse( const char* in, const ycSize_t len, ycByteBuffer* out )
{
	return Parse( ycStringRef( in, len ), out );
}

bool ycCPP::Parse( ycStringRef in, ycByteBuffer* out )
{
	// warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
	// i know and have designed around it!
	#ifdef _MSC_VER
		#pragma warning( push )
		#pragma warning( disable : 4611 )
	#endif
	if( m_inParse || setjmp( m_jmpBuf ) == 0 )
	#ifdef _MSC_VER
		#pragma warning( pop )
	#endif
	{
		++m_inParse;
		while( in.Length() )
		{
			ycStringRef lineStart = in;

			SkipHorizWhiteSpaceAndComments( &in );

			// preprocessor directives
			if( in.First() == '#' )
			{
				in.Advance( 1 ); // #
				SkipHorizWhiteSpaceAndComments( &in );
				if( in.Take( "ifdef", 5 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth > 0 ) // if we're already in a negative condition no need to eval, act like this 'if' failed too
					{
						++m_ignoreDepth;
					}
					else
					{
						ycStringRef identifier = in.TakeIdentifier();
						if( identifier.IsNull() ) { Error( in.GetPtr(), "Expected identifier for 'ifdef'." ); }
						const ycHashKey_t identifierHash = identifier.Hash();
						const bool cond = m_macros.Contains( identifierHash );
						if( cond == false ) { ++m_ignoreDepth; }
						m_trueSeen = cond;
					}
					const ycSize_t lineLength = in.LineLengthWithComments( true );
					in.Advance( lineLength );
				}
				else if( in.Take( "ifndef", 6 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth > 0 ) // if we're already in a negative condition no need to eval, act like this 'if' failed too
					{
						++m_ignoreDepth;
					}
					else
					{
						ycStringRef identifier = in.TakeIdentifier();
						if( identifier.IsNull() ) { Error( in.GetPtr(), "Expected identifier for 'ifndef'." ); }
						const ycHashKey_t identifierHash = identifier.Hash();
						const bool cond = !m_macros.Contains( identifierHash );
						if( cond == false ) { ++m_ignoreDepth; }
						m_trueSeen = cond;
					}
					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "if", 2 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth > 0 ) // if we're already in a negative condition no need to eval, act like this 'if' failed too
					{
						++m_ignoreDepth;
					}
					else
					{
						const ycSize_t lineLength = in.LineLengthWithComments( false );
						bool cond = EvaluateCondition( in.Left( lineLength ), out );
						if( cond == false ) { ++m_ignoreDepth; }
						m_trueSeen = cond;
					}
					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "elif", 4 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth == 0 ) { m_ignoreDepth = 1; m_trueSeen = true; } // if currently in 'all is well, emit lines you see' mode, flip it -- we must be inside a true 'if', and this elif cannot trigger; also ensure m_trueSeen is correct, conditionals inside the true block above may have reset it
					else if( m_ignoreDepth == 1 && !m_trueSeen ) // if this could possibly knock us out of 'do not output anything' mode, it needs to be evaluated
					{
						const ycSize_t lineLength = in.LineLengthWithComments( false );
						bool cond = EvaluateCondition( in.Left( lineLength ), out );
						if( cond ) // if the condition evaluated to true, swap to output mode
						{
							m_ignoreDepth = 0;
							m_trueSeen    = true;
						}
					}
					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "else", 4 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, false );

					if(      m_ignoreDepth == 0                ) { m_ignoreDepth = 1; } // if currently in 'all is well, emit lines you see' mode, flip it -- as if we just encountered a false #if
					else if( m_ignoreDepth == 1 && !m_trueSeen ) { m_ignoreDepth = 0; } // otherwise, only flip it if this is the #else to pull out of a false #if -- as if we just encountered #endif

					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "endif", 5 ) )
				{
					if( m_ignoreDepth > 0 ) { --m_ignoreDepth; }

					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "define", 6 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth == 0 )
					{
						Define( &in );
					}
				
					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "undef", 5 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth == 0 )
					{
						ycStringRef identifier = in.TakeIdentifier();
						if( identifier.IsNull() ) { Error( in.GetPtr(), "Expected identifier for 'undef'." ); }
						const ycHashKey_t identifierHash = identifier.Hash();
						ycChainedHash< ycHashKey_t, Macro >::Iter find = m_macros.Find( identifierHash );
						if( find.IsValid() ) // fail silently
						{
							FreeInterpolants( &find.GetValue() );
							m_macros.Remove( identifierHash ); // wasted work :( searching the hash twice... don't have a way to remove by iterator atm
						}
					}

					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "include", 7 ) )
				{
					SkipHorizWhiteSpaceAndComments( &in, true );

					if( m_ignoreDepth == 0 )
					{
						if( m_includePath.IsNull() ) { Error( in.GetPtr(), "#include directive encountered but SetIncludePath() has not been called." ); }

						// parse filename
						ycStackString<> filePath( m_includePath );
						ycStringRef pathStart = in;
						{
							if( in.TakeFirst() != '"' ) { Error( pathStart.GetPtr(), "Expected quoted string." ); }
							bool escape = false;
							for(;;)
							{
								if( in.Length() == 0 ) { Error( pathStart.GetPtr(), "Unterminated string literal." ); }
								const char ch = in.TakeFirst();
								if( ch == '\r' || ch == '\n' ) { Error( pathStart.GetPtr(), "Unterminated string literal." ); }
								if( ch == '\\' && !escape )
								{
									escape = true;
								}
								else if( ch == '"' && !escape )
								{
									break;
								}
								else
								{
									escape = false;
									filePath.Append( ch );
								}
							}
						}

						pathStart.Set( pathStart.Get(), in.Get() - pathStart.Get() );

						// If this has already been included and is a #pragma once, then don't re-include it.
						bool shouldInclude = true;
						for( ureg i = 0; i < m_includes.Length(); i++ )
						{
							if( m_includes[ i ].isPragmaOnce && m_includes[ i ].path.Equals( pathStart ) )
							{
								shouldInclude = false;
								break;
							}
						}

						// read included file
						if( shouldInclude )
						{
							ycSize_t fileDataSize;
							const char* fileData;
							filePath.NullTerminate();
							fileData = (const char *)ycBuilder::ReadFile( filePath, m_mem, &fileDataSize, ycBuilder::kIOFlag_AutoFallback );
							if( fileData == nullptr )
							{
								Error( pathStart.GetPtr(), "Could not open #include file." );
							}

							// add to m_includes
							if( m_includes.GetCapacity() == 0 ) { m_includes.Reserve( 16 ); } // reserve some include space on the first include we encounter
							Include* inc = m_includes.Append();
							inc->path = pathStart;
							inc->fileData = fileData;
							inc->fileSize = fileDataSize;
							inc->isPragmaOnce = false;

							// parse include
							Parse( fileData, fileDataSize, out );
						}
					}

					in.Advance( in.LineLengthWithComments( true ) );
				}
				else if( in.Take( "pragma once", ycStringUtils::Length( "pragma once" ) ) )
				{
					if( m_includes.Length() > 0 )
					{
						// This is a bit hacky, and assumes that the #pragma once comes at the top of files before any includes.
						// Which should hopefully be good enough in practice!
						m_includes.Last().isPragmaOnce = true;
					}

					in.Advance( in.LineLengthWithComments( true ) );
				}
			}
			// plain text
			else
			{
				const ycSize_t lineLength = lineStart.LineLengthWithComments( true );
				if( lineLength == ycSize_t(-1) )
				{
					Error( lineStart.GetPtr(), "Unterminated multi-line comment." );
				}
				if( m_ignoreDepth == 0 )
				{
					Interpolate( lineStart.Left( lineLength ), out );
				}
				in = lineStart;
				in.Advance( lineLength );
			}
		}
		--m_inParse;
		return true;
	}
	else
	{
		return false;
	}
}

void ycCPP::Define( const char* def )
{
	Define( ycStringRef( def ) );
}

void ycCPP::Define( const char* def, const ycSize_t len )
{
	Define( ycStringRef( def, len ) );
}

void ycCPP::Define( ycStringRef def )
{
	Define( &def );
}

void ycCPP::Define( ycStringRef* def )
{
	SkipHorizWhiteSpaceAndComments( def );

	ycStringRef identifier = def->TakeIdentifier();
	if( identifier.IsNull() ) { Error( def->GetPtr(), "Expected macro name in 'define'." ); }
	const ycHashKey_t identifierHash = identifier.Hash();

	Macro macro;
	ycHashKey_t argHashes[ kMaxMacroArgs ];

	// functional macros
	if( def->Length() && def->First() == '(' ) // must come directly after the identifier! no whitespace/comments!
	{
		def->Advance( 1 ); // (
		macro.numArgs      = 0;
		macro.isFunctional = true;
		
		SkipHorizWhiteSpaceAndComments( def );

		if( def->Length() == 0 ) { Error( def->GetPtr(), "Expected macro argument list in 'define'." ); }

		// parse argument list and hash argument names
		if( def->First() == ')' ) { def->Advance( 1 ); } // treat this like a non-functional macro, except requiring parens
		else
		{
			for(;;)
			{
				ycStringRef argIdentifier = def->TakeIdentifier();
				if( argIdentifier.IsNull() ) { Error( def->GetPtr(), "Expected macro argument identifier 'define'." ); }
				const ycHashKey_t argIdentifierHash = argIdentifier.Hash();

				if( macro.numArgs >= kMaxMacroArgs ) { Error( def->GetPtr(), "Internal buffer overflow, too many macro arguments." ); }
				argHashes[ macro.numArgs ] = argIdentifierHash;
				++macro.numArgs;
			
				SkipHorizWhiteSpaceAndComments( def );

				if( def->Length() == 0 ) { Error( def->GetPtr(), "Malformed argument list in 'define'." ); }

				if( def->First() == ')' ) { def->Advance( 1 ); break; } // done w/ args
				if( def->First() == ',' ) // next arg
				{
					def->Advance( 1 );
					SkipHorizWhiteSpaceAndComments( def );
				}
				else
				{
					Error( def->GetPtr(), "Malformed argument list in 'define'." );
				}
			}
		}
	}
	// plain macros
	else
	{
		macro.numArgs      = 0;
		macro.isFunctional = false;
	}

	// parse value
	SkipHorizWhiteSpaceAndComments( def );
	if( def->Length() == 0 || def->First() == '\r' || def->First() == '\n' )
	{
		macro.hasValue = false;
	}
	else
	{
		macro.hasValue = true;
		Interpolant* prevInterpolant = nullptr;
		Interpolant* interpolant = &macro.firstInterpolant;
		interpolant->start  = def->GetPtr();
		interpolant->length = 0;
		interpolant->next   = nullptr;
		for(;;)
		{
			const char ch = def->First();
			const char ch2 = def->Length() ? def->GetChar(1) : '\0' ; // no rules will match \0
			if( ch == '/' && ch2 == '/' )
			{
				def->Advance( 2 ); // '//'
				while( def->Length() && def->First() != '\r' && def->First() != '\n'  ) { def->Advance( 1 ); }
				break;
			}
			else if( ch == '/' && ch2 == '*' )
			{
				def->Advance( 2 ); // '/*'
				for(;;)
				{
					if( def->Length() < 2 ) { Error( def->GetPtr(), "Unterminated multi-line comment." ); }
					if( def->First() == '*' && def->GetChar(1) == '/' ) { def->Advance( 2 ); break; }
					def->Advance( 1 );
				}

				prevInterpolant = interpolant;

				interpolant = AllocInterpolant();
				prevInterpolant->next = interpolant;

				interpolant->start  = def->GetPtr();
				interpolant->length = 0;
				interpolant->next   = nullptr;
			}
			else if( ch == '\r' || ch == '\n'  )
			{
				break;
			}
			else if( macro.numArgs > 0 && ycStringUtils::IsIdentifierFirstChar( ch ) )
			{
				ycStringRef argIdentifier = def->TakeIdentifier();
				ycAssert( !argIdentifier.IsNull() ); // shouldn't be possible since we checked the first character
				const ycHashKey_t argIdentifierHash = argIdentifier.Hash();

				ureg argIdx = kMaxMacroArgs;
				for( uint32_t i = 0; i != macro.numArgs; ++i )
				{
					if( argIdentifierHash == argHashes[i] ) { argIdx = i; break; }
				}

				//
				if( argIdx == kMaxMacroArgs ) { interpolant->length += argIdentifier.Length(); }
				else
				{
					Interpolant* argInterpolant;

					// if the prev interpolant is unused (has not accumulated any characters, and is not a macro argument), just use it
					if( interpolant->length == 0 && interpolant->start != nullptr ) { argInterpolant = interpolant; }

					// otherwise, create a new one
					else
					{
						prevInterpolant = interpolant;
						argInterpolant = AllocInterpolant();
						prevInterpolant->next = argInterpolant;
					}

					argInterpolant->start  = nullptr;
					argInterpolant->argIdx = argIdx;

					prevInterpolant = argInterpolant;

					interpolant = AllocInterpolant();
					prevInterpolant->next = interpolant;

					interpolant->start  = def->GetPtr();
					interpolant->length = 0;
					interpolant->next   = nullptr;
				}
			}
			else if( ch == '\\' )
			{
				def->Advance( 1 );
				ycStringRef lookAhead = *def;
				lookAhead.TrimLeadingHorizWhitespace();
				if( lookAhead.Length() && ( lookAhead.First() == '\r' || lookAhead.First() == '\n' ) )
				{
					if( lookAhead.Take( "\r\n", 2 ) ) { }
					else { lookAhead.Advance( 1 ); }
					lookAhead.TrimLeadingHorizWhitespace();
					*def = lookAhead;

					prevInterpolant = interpolant;

					interpolant = AllocInterpolant();
					prevInterpolant->next = interpolant;

					interpolant->start  = def->GetPtr();
					interpolant->length = 0;
					interpolant->next   = nullptr;
				}
				else
				{
					++interpolant->length;
				}
			}
			else
			{
				def->Advance( 1 );
				++interpolant->length;
			}
		}
		if( interpolant->length == 0 && interpolant->start != nullptr ) // if the final interpolant is empty, remove it (would be nice not to create it in the first place...)
		{
			ycAssert( interpolant != &macro.firstInterpolant ); // shouldn't be possible, we should've detected a value-less macro if this were the case
			prevInterpolant->next = nullptr;
			FreeInterpolant( interpolant );
		}
	}

	ycChainedHash< ycHashKey_t, Macro >::Iter find = m_macros.Find( identifierHash );
	if( find.IsNull() )
	{
		m_macros.Insert( identifierHash, macro );
	}
	else
	{
		FreeInterpolants( &find.GetValue() );
		find.GetValue() = macro;
	}
}

void ycCPP::DefineNoValue( const ycStringRef key )
{
	Macro macro;
	macro.numArgs      = 0;
	macro.isFunctional = false;
	macro.hasValue     = false;

	const ycHashKey_t identifierHash = key.Hash();
	ycChainedHash< ycHashKey_t, Macro >::Iter find = m_macros.Find( identifierHash );
	if( find.IsNull() )
	{
		m_macros.Insert( identifierHash, macro );
	}
	else
	{
		FreeInterpolants( &find.GetValue() );
		find.GetValue() = macro;
	}
}

void ycCPP::DefineSimple( const ycStringRef key, const ycStringRef value )
{
	Macro macro;
	macro.numArgs      = 0;
	macro.isFunctional = false;
	macro.hasValue     = true;
	macro.firstInterpolant.start  = value.GetPtr();
	macro.firstInterpolant.length = value.Length();
	macro.firstInterpolant.next   = nullptr;

	const ycHashKey_t identifierHash = key.Hash();
	ycChainedHash< ycHashKey_t, Macro >::Iter find = m_macros.Find( identifierHash );
	if( find.IsNull() )
	{
		m_macros.Insert( identifierHash, macro );
	}
	else
	{
		FreeInterpolants( &find.GetValue() );
		find.GetValue() = macro;
	}
}

void ycCPP::Undef( const ycStringRef key )
{
	m_macros.Remove( key.Hash() );
}

ycCPP::Interpolant* ycCPP::AllocInterpolant()
{
	return ycnew( m_mem, "ycCPP Interpolant" )Interpolant;
}

void ycCPP::FreeInterpolant( Interpolant* ptr )
{
	ycdelete( m_mem, ptr );
}

void ycCPP::FreeInterpolants( Macro* macro )
{
	if( macro->hasValue == false ) { return; }
	Interpolant* interpolant = macro->firstInterpolant.next;
	while( interpolant )
	{
		Interpolant* next = interpolant->next;
		FreeInterpolant( interpolant );
		interpolant = next;
	}
}

bool ycCPP::EvaluateCondition( const char* expr )
{
	return EvaluateCondition( ycStringRef( expr ) );
}

bool ycCPP::EvaluateCondition( const char* expr, const ycSize_t len )
{
	return EvaluateCondition( ycStringRef( expr, len ) );
}

bool ycCPP::EvaluateCondition( ycStringRef expr )
{
	ycByteBuffer buf( m_mem );
	return EvaluateIntExpression( expr, &buf ) != 0;
}

bool ycCPP::EvaluateCondition( ycStringRef expr, ycByteBuffer* scratch )
{
	return EvaluateIntExpression(expr, scratch) != 0;
}

void ycCPP::Interpolate( ycStringRef line, ycByteBuffer* out )
{
	// copy literals in batches instead of one at a time
	const char* literalsStart = line.GetPtr();
	ycSize_t literalsLen = 0;

	for(;;)
	{
		if( line.Length() == 0 ) { break; }

		ycStringRef identifier = line.TakeIdentifier();
		if( !identifier.IsNull() )
		{
			const ycHashKey_t identifierHash = identifier.Hash();
			ycChainedHash< ycHashKey_t, Macro >::Iter find = m_macros.Find( identifierHash );
			if( find.IsNull() ) { literalsLen += identifier.Length(); } // if the identifier is not a define, add it to literals
			else
			{
				if( literalsLen ) // dump any pending literals to output, and move the next potential literals starting point to after the identifier
				{
					out->AppendRaw( literalsStart, literalsLen );
					literalsLen = 0;
				}
				InterpolateMacro( &find.GetValue(), &line, out );
				literalsStart = line.GetPtr();
			}
		}
		else if( line.Length() >= 2 && line.BeginsWith( "/*", 2 ) )
		{
			if( literalsLen ) // dump any pending literals to output, and move the next potential literals starting point to after the comment
			{
				out->AppendRaw( literalsStart, literalsLen );
				literalsLen = 0;
			}
			if( line.TrimLeadingComment() == ycSize_t(-1) ) { Error( line.GetPtr(), "Unterminated multi-line comment." ); }
			literalsStart = line.GetPtr();
		}
		else if( line.Length() >= 2 && line.BeginsWith( "//", 2 ) )
		{
			if( literalsLen ) // dump any pending literals to output, and move the next potential literals starting point to after the comment
			{
				out->AppendRaw( literalsStart, literalsLen );
				literalsLen = 0;
			}
			line.TrimLeadingComment();
			literalsStart = line.GetPtr();
		}
		else
		{
			line.Advance( 1 );
			++literalsLen;
		}
	}

	// dump any remaining literals to output
	if( literalsLen ) { out->AppendRaw( literalsStart, literalsLen ); }
}

void ycCPP::SetIncludePath( const ycStringRef path )
{
	ycAssert( path.IsNull() || path.Last() == '/' );
	m_includePath = path;
}

void ycCPP::Error( const char* showCurPos, const char* fmt, ... )
{
	if( m_errorMsg ) { YC_FREE( m_mem, m_errorMsg ); }
	m_errorMsg = (char*)YC_ALLOC( m_mem, kErrBufSize, YC_MEM_DEFAULT_ALIGN, "ycCPP Error Message" );
	va_list args;
	va_start( args, fmt );
	int bytesWritten = vsnprintf( m_errorMsg, kErrBufSize, fmt, args );
	if( showCurPos )
	{
		bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\nAt:\n%.*s", 64, showCurPos );
	}
	ycAssert( bytesWritten < kErrBufSize );
	va_end( args );
	if( m_inParse ) { longjmp( m_jmpBuf, 1 ); }
	else            { ycAssertMsg( false, m_errorMsg ); }
}

/*
If the EvaluateCondition() code is really confusing, check: https://en.wikipedia.org/wiki/Shunting-yard_algorithm
(it's probably still confusing, but that algo is the intent)
*/

namespace
{
	enum
	{
		// right-associative/unary operators come first so we can do a simple range check for them
		kOp_UnaryPlus  = 0,
		kOp_UnaryMinus = 1,
		kOp_LogicalNot = 2,
		kOp_BitwiseNot = 3,
		
		kLastRightAssociativeOp = kOp_BitwiseNot,
		kLastUnaryOp            = kLastRightAssociativeOp, // if these two become different one is gonna lose the range check, but keep it for now

		kOp_Add = 4,
		kOp_Sub = 5,
		kOp_Mul = 6,
		kOp_Div = 7,

		kOp_LeftShift = 8,
		kOp_RightShift = 9,
		
		kOp_LogicalAnd = 10,
		kOp_LogicalOr  = 11,

		kOp_BitwiseAnd = 12,
		kOp_BitwiseOr  = 13,

		kOp_Less    = 14,
		kOp_Greater = 15,

		kOp_Eq  = 16,
		kOp_Neq = 17,
		kOp_Leq = 18,
		kOp_Geq = 19,

		kOp_LeftParen = 20 // handled specially
	};
	const uint32_t s_opPrecedence[] = // smaller type might be cache advantageous
	{ // http://clang.llvm.org/doxygen/OperatorPrecedence_8h_source.html
		14, // kOp_UnaryPlus
		14, // kOp_UnaryMinus
		14, // kOp_LogicalNot
		14, // kOp_BitwiseNot
		12, // kOp_Add
		12, // kOp_Sub
		13, // kOp_Mul
		13, // kOp_Div
		12, // kOp_LeftShift
		12, // kOp_RightShift
		5,  // kOp_LogicalAnd
		4,  // kOp_LogicalOr
		8,  // kOp_BitwiseAnd
		6,  // kOp_BitwiseOr
		10, // kOp_Less
		10, // kOp_Greater
		9,  // kOp_Eq
		9,  // kOp_Neq
		10, // kOp_Leq
		10, // kOp_Geq
		0   // kOp_LeftParen; must have lower precedence than anything else for PushOperator to follow the rules
	};
}

void ycCPP::ApplyOperator( const ycStringRef& pos, const uint32_t op, EvalCtx* ctx )
{
	ureg queueSize = ctx->queueSize;
	if( op <= kLastUnaryOp )
	{
		if( queueSize < 1 ) { Error( pos.GetPtr(), "An error occured while parsing a unary operator in a conditional expression." ); }
		int32_t& arg = ctx->queue[ queueSize-1 ];
		switch( op )
		{
		case kOp_UnaryPlus:  /*nop*/     break;
		case kOp_UnaryMinus: arg = -arg; break;
		case kOp_LogicalNot: arg = !arg; break;
		case kOp_BitwiseNot: arg = ~arg; break;
		YC_NO_DEFAULT_ASSERT
		}
	}
	else
	{
		if( queueSize < 2 ) { Error( pos.GetPtr(), "An error occured while parsing an infix operator in a conditional expression." ); }
		--queueSize; // combine two arguments into the first one
		int32_t& lhs = ctx->queue[ queueSize-1 ];
		const int32_t& rhs = ctx->queue[ queueSize ];
		switch( op )
		{
		case kOp_Add:        lhs = lhs + rhs;  break;
		case kOp_Sub:        lhs = lhs - rhs;  break;
		case kOp_Mul:        lhs = lhs * rhs;  break;
		case kOp_Div:        lhs = lhs / rhs;  break;
		case kOp_LogicalAnd: lhs = lhs && rhs; break;
		case kOp_LogicalOr:  lhs = lhs || rhs; break;
		case kOp_BitwiseAnd: lhs = lhs & rhs;  break;
		case kOp_BitwiseOr:  lhs = lhs | rhs;  break;
		case kOp_Less:       lhs = lhs < rhs;  break;
		case kOp_Greater:    lhs = lhs > rhs;  break;
		case kOp_Eq:         lhs = lhs == rhs; break;
		case kOp_Neq:        lhs = lhs != rhs; break;
		case kOp_Leq:        lhs = lhs <= rhs; break;
		case kOp_Geq:        lhs = lhs >= rhs; break;
		case kOp_LeftShift:  lhs = lhs << rhs; break;
		case kOp_RightShift: lhs = lhs >> rhs; break;
		YC_NO_DEFAULT_ASSERT
		}
	}
	ctx->queueSize = queueSize;
}

void ycCPP::PushOperator( const ycStringRef& pos, const uint32_t op, EvalCtx* ctx )
{
	ureg opStackSize = ctx->stackSize;
	const uint32_t precedence = s_opPrecedence[ op ] + (op<=kLastRightAssociativeOp); // add one to precedence for right associative ops, to simulate < instead of <=
	while( opStackSize && precedence <= s_opPrecedence[ ctx->stack[ opStackSize-1 ] ] )
	{
		--opStackSize;
		ApplyOperator( pos, ctx->stack[ opStackSize ], ctx );
	}
	if( opStackSize >= kOpStackSize ) { Error( pos.GetPtr(), "Internal buffer overflow, conditional expression is too complex." ); }
	ctx->stack[ opStackSize ] = op;
	++opStackSize;
	ctx->stackSize = opStackSize;
}

s32 ycCPP::EvaluateIntExpression( ycStringRef expr, ycByteBuffer* scratch )
{
	EvalCtx ctx;
	ctx.stackSize   = 0;
	ctx.queueSize   = 0;
	ctx.expectUnary = true;

	const char* exprStart = expr.GetPtr();

	EvalCondParseLoop( &expr, &ctx, scratch );
	if( expr.Length() ) { Error( expr.GetPtr(), "Extra tokens after conditional expression." ); }

	// collapse remaining stack/queue
	while( ctx.stackSize )
	{
		--ctx.stackSize;
		const uint32_t op = ctx.stack[ ctx.stackSize ];
		if( op == kOp_LeftParen ) { Error( exprStart, "Mismatched parentheses." ); } // mismatched parens
		ApplyOperator( expr, op, &ctx );
	}

	if( ctx.queueSize != 1 ) { Error( exprStart, "Could not parse expression." ); }
	return ctx.queue[0];
}

s32 ycCPP::EvaluateIntExpression( ycStringRef expr )
{
	ycByteBuffer buf( m_mem );
	return EvaluateIntExpression( expr, &buf );
}

void ycCPP::EvalCondParseLoop( ycStringRef* in, void* pCtx, ycByteBuffer* scratch )
{
	EvalCtx* ctx = (EvalCtx*)pCtx;
	for(;;)
	{
		//const char* loopStart = in->Get();
		SkipHorizWhiteSpaceAndComments( in );
		if( in->Length() == 0 ) { break; }
		
		const char ch  = in->First();
		const char ch2 = in->Length() > 1 ? in->GetChar( 1 ) : '\0' ;

		// integer literal - TODO HACK FIXME: should have ycStringRef util for this
		if( ycStringUtils::IsDigit( ch ) )
		{
			const char* numberStart = in->GetPtr();
			do { in->Advance( 1 ); } while( in->Length() && ycStringUtils::IsDigit( in->First() ) );
			if( ctx->queueSize >= kOutputQueueSize ) { Error( in->GetPtr(), "Internal buffer overflow, could not parse expression." ); }
			ctx->queue[ ctx->queueSize ] = ycStringUtils::ToInt( numberStart, in->GetPtr()-numberStart );
            if( in->First() == 'u' ) { in->Advance( 1 ); } // throw out an unsigned 'u' suffix'
			++ctx->queueSize;
			ctx->expectUnary = false;
		}

		// operators
		else if( ch == '+' )
		{
			in->Advance( 1 ); // +
			if( ctx->expectUnary ) { PushOperator( *in, kOp_UnaryPlus, ctx ); } // allow chained unary operators, allows for stupid things like --1 but need it for !-1
			else                   { PushOperator( *in, kOp_Add,       ctx ); ctx->expectUnary = true; }
		}
		else if( ch == '-' )
		{
			in->Advance( 1 ); // -
			if( ctx->expectUnary ) { PushOperator( *in, kOp_UnaryMinus, ctx ); }
			else                   { PushOperator( *in, kOp_Sub,        ctx ); ctx->expectUnary = true; }
		}
		else if( ch == '!'               ) { in->Advance( 1 ); PushOperator( *in, kOp_LogicalNot, ctx ); }
		else if( ch == '~'               ) { in->Advance( 1 ); PushOperator( *in, kOp_BitwiseNot, ctx ); }
		else if( ch == '*'               ) { in->Advance( 1 ); PushOperator( *in, kOp_Mul,        ctx ); ctx->expectUnary = true; }
		else if( ch == '/'               ) { in->Advance( 1 ); PushOperator( *in, kOp_Div,        ctx ); ctx->expectUnary = true; }
		else if( ch == '=' && ch2 == '=' ) { in->Advance( 2 ); PushOperator( *in, kOp_Eq,         ctx ); ctx->expectUnary = true; }
		else if( ch == '!' && ch2 == '=' ) { in->Advance( 2 ); PushOperator( *in, kOp_Neq,        ctx ); ctx->expectUnary = true; }
		else if( ch == '<' && ch2 == '<' ) { in->Advance( 2 ); PushOperator( *in, kOp_LeftShift,  ctx ); ctx->expectUnary = true; }
		else if( ch == '>' && ch2 == '>' ) { in->Advance( 2 ); PushOperator( *in, kOp_RightShift, ctx ); ctx->expectUnary = true; }
		else if( ch == '&' )
		{
			if( ch2 == '&')
			{
				in->Advance( 2 );
				PushOperator( *in, kOp_LogicalAnd, ctx );

				// short circuiting
				if( ctx->queueSize == 0 ) { Error( in->GetPtr(), "An error occured while parsing an infix operator '&&' in a conditional expression." ); }
				if( ctx->queue[ ctx->queueSize-1 ] == 0 )
				{
					// remove the operator we just pushed (but we needed to PushOperator to collapse the queue) TODO HACK FIXME: maybe special PushOperator for this type of thing? PushBooleanOperator? PushLogicalOperator?
					if( ctx->queueSize == 0 ) { Error( in->GetPtr(), "An error occured while parsing an infix operator '&&' in a conditional expression." ); }
					--ctx->stackSize;

					// drop down to simpler parsing until we hit something important
					ParseShortCircuit( in );

					// unary expect? may not matter since we only break on certain things (which dont check that value anyway)
				}
			}
			else { in->Advance( 1 ); PushOperator( *in, kOp_BitwiseAnd, ctx ); }
			ctx->expectUnary = true;
		}
		else if( ch == '|' )
		{
			if( ch2 == '|')
			{
				in->Advance( 2 );
				PushOperator( *in, kOp_LogicalOr, ctx );

				// short circuiting
				if( ctx->queueSize == 0 ) { Error( in->GetPtr(), "An error occured while parsing an infix operator '||' in a conditional expression." ); }
				if( ctx->queue[ ctx->queueSize-1 ] == 1 )
				{
					// remove the operator we just pushed (but we needed to PushOperator to collapse the queue) TODO HACK FIXME: maybe special PushOperator for this type of thing? PushBooleanOperator? PushLogicalOperator?
					if( ctx->queueSize == 0 ) { Error( in->GetPtr(), "An error occured while parsing an infix operator '||' in a conditional expression." ); }
					--ctx->stackSize;

					// drop down to simpler parsing until we hit something important
					ParseShortCircuit( in );

					// unary expect? may not matter since we only break on certain things (which dont check that value anyway)
				}
			}
			else { in->Advance( 1 ); PushOperator( *in, kOp_BitwiseOr, ctx ); }
			ctx->expectUnary = true;
		}
		else if( ch == '<' )
		{
			if( ch2 == '=') { in->Advance( 2 ); PushOperator( *in, kOp_Leq,  ctx ); }
			else            { in->Advance( 1 ); PushOperator( *in, kOp_Less, ctx ); }
			ctx->expectUnary = true;
		}
		else if( ch == '>' )
		{
			if( ch2 == '=') { in->Advance( 2 ); PushOperator( *in, kOp_Geq,     ctx ); }
			else            { in->Advance( 1 ); PushOperator( *in, kOp_Greater, ctx ); }
			ctx->expectUnary = true;
		}

		// 'defined' operator
		else if( in->BeginsWith( "defined", 7 ) )
		{
			const char* opStart = in->GetPtr();

			in->Advance( 7 ); // defined
			bool foundSpacing = SkipHorizWhiteSpaceAndComments( in );
			if( in->Length() == 0 ) { Error( opStart, "Expected argument for 'defined' operator." ); }
			const bool parens = in->First() == '(';
			if( parens )
			{
				in->Advance( 1 ); // (
				SkipHorizWhiteSpaceAndComments( in );
			}
			else if( !foundSpacing )
			{
				Error( opStart, "Could not parse 'defined' operator." );
			}

			ycStringRef identifier = in->TakeIdentifier();
			if( identifier.IsNull() ) { Error( opStart, "Expected identifier argument for 'defined' operator." ); }

			if( parens )
			{
				SkipHorizWhiteSpaceAndComments( in );
				if( in->Length() == 0 || in->First() != ')' )
				{
					Error( opStart, "Unterminated parentheses for 'defined' operator." );
				}
				in->Advance( 1 ); // )
			}
			const ycHashKey_t identifierHash = identifier.Hash();
			if( ctx->queueSize >= kOutputQueueSize ) { Error( in->GetPtr(), "Internal buffer overflow, conditional expression is too complex." ); }
			ctx->queue[ ctx->queueSize ] = m_macros.Contains( identifierHash ) ? 1 : 0 ;
			++ctx->queueSize;
			ctx->expectUnary = false;
		}

		// identifiers (mostly macros)
		else if( ycStringUtils::IsIdentifierFirstChar( ch ) )
		{
			ycStringRef identifier = in->TakeIdentifier();
			const ycHashKey_t identifierHash = identifier.Hash();
			ycChainedHash< ycHashKey_t, Macro >::Iter find = m_macros.Find( identifierHash );

			if( find.IsValid() )
			{
				if( find.GetValue().hasValue )
				{
					const uintptr_t offset = scratch->GetWriteOffset();
					InterpolateMacro( &find.GetValue(), in, scratch );
					ycStringRef tmp( (char*)scratch->GetData() + offset, scratch->GetWriteOffset() - offset );
					EvalCondParseLoop( &tmp, ctx, scratch );
					scratch->SetWriteOffset( offset );
				}
				else
				{
					// this is nonstandard behavior and should maybe be a warning; clang cpp allows it though
					// just consume the macro and emit nothing
				}
			}
			else
			{
				if( ctx->queueSize >= kOutputQueueSize ) { Error( in->GetPtr(), "Internal buffer overflow, conditional expression is too complex." ); }
				if( identifier.Equals( "true", 4 ) ) // check macros first! you can re#define true/false!
				{
					ctx->queue[ ctx->queueSize ] = 1;
				}
				else if( identifier.Equals( "false", 5 ) )
				{
					ctx->queue[ ctx->queueSize ] = 0;
				}
				else // undefined identifier, treat as zero
				{
					ctx->queue[ ctx->queueSize ] = 0;
				}
				++ctx->queueSize;
				ctx->expectUnary = false;
			}
		}

		// parentheses
		else if( ch == '(' )
		{
			in->Advance( 1 ); // (
			if( ctx->stackSize >= kOpStackSize ) { Error( in->GetPtr(), "Internal buffer overflow, conditional expression is too complex." ); }
			ctx->stack[ ctx->stackSize ] = kOp_LeftParen;
			++ctx->stackSize;
			ctx->expectUnary = true;
		}
		else if( ch == ')' )
		{
			in->Advance( 1 ); // )
			for(;;)
			{
				if( ctx->stackSize == 0 ) { Error( in->GetPtr(), "Mismatched parentheses in conditional expression." ); }
				--ctx->stackSize;
				const uint32_t op = ctx->stack[ ctx->stackSize ];
				if( op == kOp_LeftParen ) { break; }
				ApplyOperator( *in, op, ctx );
			}
			ctx->expectUnary = false;
		}
		else
		{
			Error( in->GetPtr(), "An error occured while parsing a conditional expression." );
		}
	}
}

void ycCPP::ParseShortCircuit( ycStringRef* in ) // most preprocessors still do syntactical checking for this.. we... don't.
{
	ureg parenDepth = 0;
	for(;;)
	{
		if( in->Length() == 0 ) { break; }

		const char ch  = in->First();
		const char ch2 = in->Length() > 1 ? in->GetChar(1) : '\0' ;

		if(      ch == '\'' ) { SkipQuotedChar( in ); }
		else if( ch == '"'  ) { SkipQuotedString( in ); }
		//else if( ch == '/' && ch2 == '/' )
		//{
		//	while( *in != '\r' && *in != '\n' && *in != '\0'  ) { ++in; }
		//	break;
		//}
		//else if( in[0] == '/' && in[1] == '*' )
		//{
		//	while( !( in[0] == '*' && in[1] == '/' ) ) { ycAssert( *in != '\0' ); ++in; }
		//	in += 2;
		//	break;
		//}
		else if( ch == '/' && ( ch2 == '/' || ch2 == '*' ) )
		{
			SkipHorizWhiteSpaceAndComments( in, true );
			break; // TODO HACK FIXME: why is this breaking? shouldnt that only occur w/ single line comments?
		}
		else if( ch == '&' && ch2 == '&' ) { break; }
		else if( ch == '|' && ch2 == '|' ) { break; }
		else if( ch == '(' ) { in->Advance( 1 ); ++parenDepth; }
		else if( ch == ')' )
		{
			if( parenDepth == 0 ) { break; }
			else { --parenDepth; }
			in->Advance( 1 );
		}
		else { in->Advance( 1 ); }
	}
}

void ycCPP::InterpolateMacro( Macro* macro, ycStringRef* in, ycByteBuffer* out )
{
	ycStringRef argValues[ kMaxMacroArgs ];
	uint32_t    numArgs = 0;

	// TODO HACK FIXME: check argument count match!
	if( macro->isFunctional )
	{
		SkipWhiteSpaceAndComments( in ); // can have whitespace/comments between MACRONAME and (
		if( !in->Take( '(' ) ) { Error( in->GetPtr(), "Expected '('." ); }
		SkipWhiteSpaceAndComments( in );
		if( macro->numArgs == 0 )
		{
			if( !in->Take( ')' ) ) { Error( in->GetPtr(), "Expected ')'." ); }
		}
		else
		{
			ureg parenDepth = 0;
			const char* argStart = in->GetPtr();
			for(;;)
			{
				if( in->Length() == 0 ) { Error( in->GetPtr(), "Premature end of macro arguments." ); }
				const char ch = in->First();

				if( ch == ',' && parenDepth == 0 ) // done with current arg
				{
					if( numArgs >= kMaxMacroArgs ) { Error( argStart, "Internal buffer overflow, too many macro arguments!" ); }
					argValues[ numArgs ] = ycStringRef( argStart, in->GetPtr() - argStart );
					argValues[ numArgs ].TrimTrailingWhiteSpace();
					//if( argValues[ numArgs ].Length() == 0 ) { Error( argStart, "Expected macro argument." ); }
					++numArgs;
					in->Advance( 1 ); // ,

					SkipWhiteSpaceAndComments( in );
					argStart = in->GetPtr();
				}
				else if( ch == ')' )
				{
					if( parenDepth == 0 ) // done with all args
					{
						if( numArgs >= kMaxMacroArgs ) { Error( argStart, "Internal buffer overflow, too many macro arguments!" ); }
						argValues[ numArgs ] = ycStringRef( argStart, in->GetPtr() - argStart );
						argValues[ numArgs ].TrimTrailingWhiteSpace();
						if( argValues[ numArgs ].Length() == 0 ) { Error( argStart, "Expected macro argument." ); }
						++numArgs;
						in->Advance( 1 ); // )

						break;
					}
					else
					{
						in->Advance( 1 ); // )
						--parenDepth;
					}
				}
				else if( ch == '(' )
				{
					in->Advance( 1 ); // (
					++parenDepth;
				}
				else if( ch == '"' )
				{
					SkipQuotedString( in );
				}
				else if( ch == '\'' )
				{
					SkipQuotedChar( in );
				}
				else
				{
					in->Advance( 1 );
				}
			}
		}
	}

	// TODO HACK FIXME: check number of macro arguments
	if( macro->hasValue )
	{
		for( Interpolant* interpolant = &macro->firstInterpolant; interpolant != nullptr; interpolant = interpolant->next )
		{
			if( interpolant->start != nullptr )
			{
				Interpolate( ycStringRef( interpolant->start, interpolant->length ), out );
			}
			else
			{
				Interpolate( argValues[ interpolant->argIdx ], out );
			}
		}
	}
}

bool ycCPP::SkipWhiteSpaceAndComments( ycStringRef* src, const bool required )
{
	bool encountered = false;
	for(;;)
	{
		ycSize_t trimmed;
		if( src->TrimLeadingWhitespace() ) { encountered = true; }
		else if( (trimmed = src->TrimLeadingComment()) != 0 )
		{
			encountered = true;
			if( trimmed == ycSize_t(-1) ) { Error( src->GetPtr(), "Unterminated multi-line comment." ); }
		}
		else { break; }
	}
	if( !encountered && required ) { Error( src->GetPtr(), "Expected whitespace or comments." ); }
	return encountered;
}

bool ycCPP::SkipHorizWhiteSpaceAndComments( ycStringRef* src, const bool required )
{
	bool encountered = false;
	for(;;)
	{
		ycSize_t trimmed;
		if( src->TrimLeadingHorizWhitespace() ) { encountered = true; }
		else if( (trimmed = src->TrimLeadingComment()) != 0 )
		{
			encountered = true;
			if( trimmed == ycSize_t(-1) ) { Error( src->GetPtr(), "Unterminated multi-line comment." ); }
		}
		else { break; }
	}
	if( !encountered && required ) { Error( src->GetPtr(), "Expected whitespace or comments." ); }
	return encountered;
}

void ycCPP::SkipQuotedString( ycStringRef* src )
{
	ycAssert( src->Length() && src->First() == '"' );
	const char* stringStart = src->GetPtr();
	src->Advance( 1 ); // "
	bool escape = false;
	for(;;)
	{
		if( src->Length() == 0 ) { Error( stringStart, "Unterminated string literal." ); }
		const char ch = src->First();
		if( ch == '\r' || ch == '\n' ) { Error( stringStart, "Unterminated string literal." ); }
		if( ch == '\\' && !escape )
		{
			escape = true;
			src->Advance( 1 );
		}
		else if( ch == '"' && !escape )
		{
			src->Advance( 1 );
			break;
		}
		else
		{
			escape = false;
			src->Advance( 1 );
		}
	}
}

void ycCPP::SkipQuotedChar( ycStringRef* src ) // TODO HACK FIXME: poor handling of NUL \r and \n (literals, not escaped \r and \n)
{
	ycAssert( src->Length() && src->First() == '\'' );
	const char* charStart = src->GetPtr();
	src->Advance( 1 ); // '
	if( src->Length() == 0 ) { Error( charStart, "Unterminated character literal." ); }
	if( src->First() == '\'' ) { Error( charStart, "Empty character literal." ); }
	ycSize_t len = src->First() == '\\' ? 2 : 1 ; // escaped characters get 2
	if( src->Length() < len+1 /*+1 for closing '*/ ) { Error( charStart, "Unterminated character literal." ); }
	src->Advance( len );
	if( src->First() != '\'' ) { Error( charStart, "Invalid character literal." ); }
	src->Advance( 1 );
}

#ifdef YC_TEST
#include "ycTest.h"

#include "ycFile.h"
#include "ycStringRef.h"

#include <stdio.h>

#if defined YC_EDITOR
	#define PATH_PREFIX ""
#else
	#define PATH_PREFIX "../../"
#endif

YC_BEGIN_TEST( ycCPP_Basic )
{
	ycAllocator* mem = g_defaultMem;

	u32 numTestsRan = 0;
	for( u32 i = 1; /**/; ++i )
	{
		char path[ YC_MAX_PATH ];

		ycSize_t fileSize;
		snprintf( path, YC_MAX_PATH, PATH_PREFIX "engine/tests/cpp/%u.txt", i );
		u8* raw = ycFile::Read( path, mem, &fileSize );
		if( raw == nullptr )
		{
			YC_TEST_CHECK( i != 1 );
			break;
		}
		ycByteBuffer input;
		input.AppendRaw( raw, fileSize );
		YC_FREE( mem, raw );

		ycByteBuffer preprocessed;
		{
			ycCPP preprocessor;
			preprocessor.Parse( ycStringRef( ( const char* )input.GetData(), fileSize ), &preprocessed );
		}

		snprintf( path, YC_MAX_PATH, PATH_PREFIX "engine/tests/cpp/%u.txt.ycpp", i );
		ycFile::Write( path, preprocessed.GetData(), preprocessed.Length() );
		
		snprintf( path, YC_MAX_PATH, PATH_PREFIX "engine/tests/cpp/%u.txt.mcpp", i );
		u8* compare = ycFile::Read( path, mem, &fileSize );

		// compare, ignoring whitespace
		const char* src0 = ( const char* )preprocessed.GetData();
		ycSize_t    len0 = preprocessed.Length();
		const char* src1 = ( const char* )compare;
		ycSize_t    len1 = fileSize;
		for(;;)
		{
			while( len0 && ycStringUtils::IsWhiteSpace( *src0 ) ) { ++src0; --len0; }
			while( len1 && ycStringUtils::IsWhiteSpace( *src1 ) ) { ++src1; --len1; }
			if( len0 == 0 || len1 == 0 )
			{
				YC_TEST_CHECK( len0 == 0 );
				YC_TEST_CHECK( len1 == 0 );
				break;
			}
			YC_TEST_CHECK( *src0 == *src1 );
			++src0; --len0;
			++src1; --len1;
		}

		YC_FREE( mem, compare );

		++numTestsRan;
	}

	YC_TEST_PASS( "ycCPP, ran %u tests.", numTestsRan );
}

#endif // YC_TEST
