#include "ycShaderParser.h"

// core
#include "ycAllocator.h"
#include "ycStd.h"
#include "ycString.h"
#include "ycStringUtils.h"

// shader builder
#include "ycShaderAST.h"
#include "ycShaderLexer.h"

// std
#include <stdarg.h>
#include <stdio.h>

namespace ycShaderBuilder
{
	// TODO HACK FIXME: could use a "Peek() and Advance()", Consume() is mandatory like Expect()

	inline void Parser::Advance( const ureg numTokens /*= 1*/ ) { m_curToken += numTokens; }

	inline bool Parser::Peek( TokenType token0                                                       ) { return                                   m_curToken[0].type == token0; }
	inline bool Parser::Peek( TokenType token0, TokenType token1                                     ) { return Peek( token0 ) &&                 m_curToken[1].type == token1; }
	inline bool Parser::Peek( TokenType token0, TokenType token1, TokenType token2                   ) { return Peek( token0, token1 ) &&         m_curToken[2].type == token2; }
	inline bool Parser::Peek( TokenType token0, TokenType token1, TokenType token2, TokenType token3 ) { return Peek( token0, token1, token2 ) && m_curToken[3].type == token3; }

	inline void Parser::Expect( TokenType token0 )
	{
		if( Peek( token0 ) ) { Advance( 1 ); }
		else { Error( *m_curToken, *m_curToken, "Expected '%s'.", GetTokenName( token0 ) ); }
	}
	inline void Parser::Expect( TokenType token0, TokenType token1                                     ) { Expect( token0 ); Expect( token1 ); }
	inline void Parser::Expect( TokenType token0, TokenType token1, TokenType token2                   ) { Expect( token0 ); Expect( token1 ); Expect( token2 ); }
	inline void Parser::Expect( TokenType token0, TokenType token1, TokenType token2, TokenType token3 ) { Expect( token0 ); Expect( token1 ); Expect( token2 ); Expect( token3 ); }

	#if defined YC_MSVC
		__pragma( warning( push ) )
		__pragma( warning( disable : 4702 ) ) 
	#endif
	inline const TokenDesc* Parser::Consume( TokenType token )
	{
		if( Peek( token ) ) { const TokenDesc* tokenDesc = m_curToken; Advance( 1 ); return tokenDesc; }
		else { Error( *m_curToken, *m_curToken, "Expected '%s'.", GetTokenName( token ) ); return nullptr;  }
	}
	#if defined YC_MSVC
		__pragma( warning( pop ) )
	#endif

	const char* typeNames[kIntrinsicType_Count] = 
	{
		 "void",   

		 "float",  
		 "int",    
		 "uint",   
		 "bool",   

		 "float1", 
		 "float2", 
		 "float3", 
		 "float4", 

		 "int1", 
		 "int2", 
		 "int3", 
		 "int4", 

		 "uint1", 
		 "uint2", 
		 "uint3", 
		 "uint4", 

		 "bool1", 
		 "bool2", 
		 "bool3", 
		 "bool4", 

		 "float2x2", 
		 "float2x3", 
		 "float2x4", 

		 "float3x2", 
		 "float3x3", 
		 "float3x4", 

		 "float4x2", 
		 "float4x3", 
		 "float4x4", 
		
		 "Tex2D",   
		 "Tex3D",   
		 "TexCube", 
		 "Tex2DUint", 
		 "Tex2DMS", 
	};

	template< class t_nodeType > t_nodeType* Parser::CreateNode( const u32 nodeType )
	{
		t_nodeType* node = ycnew( m_nodeMem, "AST Node" )t_nodeType;
		node->nodeType = nodeType;
		node->flags    = 0;
		node->nextNode = nullptr;
		return node;
	}

	Parser::Parser( ycAllocator* mem ) :
		m_mem( mem ? mem : g_defaultMem ),
		m_nodeMem( "ycShaderParser AST Nodes", m_mem ),
		m_firstStatement( nullptr ),
		m_tokens( nullptr ),
		m_curToken( nullptr ),
		m_curFunction( nullptr ),
		m_isInFunc( false ),
		m_identifiers( m_mem ),
		m_types( m_mem ),
		m_errorMsg( nullptr )
	{
	}

	Parser::~Parser()
	{
		if( m_errorMsg ) { YC_FREE( m_mem, m_errorMsg ); }
	}

	bool Parser::Parse( Lexer* input )
	{
		ycAssert( m_tokens == nullptr ); // can't currently re-use a ycShaderParser object
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
			m_types.Resize( input->m_types.Length() );
			InitTypes();

			m_tokens = &input->m_tokens.First();
			m_curToken = m_tokens;

			m_lexer = input;

			AST::Node** nextStatementPP = &m_firstStatement;
			while( m_curToken->type != kToken_EOF )
			{
				AST::Node* node = nullptr;
				if(      ( node = ParseCBufferDefinition()            ) != nullptr ) {}
				else if( ( node = ParseTypeDeclaration()              ) != nullptr ) {}
				else if( ( node = ParseFunctionDeclaration()          ) != nullptr ) {}
				else if( ( node = ParseVariableDeclarationStatement() ) != nullptr ) {}
				else { Error( *m_curToken, *m_curToken, "No parsing rules matched." ); }
				*nextStatementPP = node;
				nextStatementPP = &node->nextNode;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	void Parser::Error( const TokenDesc& firstToken, const TokenDesc& lastToken, const char* fmt, ... )
	{
		ycAssert( m_errorMsg == nullptr );
		m_errorMsg = (char*)YC_ALLOC( m_mem, kErrBufSize, YC_MEM_DEFAULT_ALIGN, "ycShaderParser Error Message" );
		va_list args;
		va_start( args, fmt );
		int bytesWritten = 0;
		if( firstToken.str.IsNull() || lastToken.str.IsNull() )
		{
			bytesWritten += vsnprintf( m_errorMsg + bytesWritten, kErrBufSize, fmt, args );
		}
		else
		{
			ycStringRef line; int lineNo, colNo;
			if( m_lexer && m_lexer->GetLineAndColumn( firstToken.str.Get(), line, lineNo, colNo ) )
			{
				bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "%d:%d: error: ", lineNo+1, colNo+1 );
				bytesWritten += vsnprintf( m_errorMsg + bytesWritten, kErrBufSize, fmt, args );
				bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\n%.*s\n", (int)line.Length(), line.Get() );
				for( int i=0; i<colNo; ++i ) { bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, line[i] == '\t'? "\t" : " " ); }
				s64 numCarets = ycMin(  line.Length() - colNo, lastToken.str.Get() - firstToken.str.Get() + lastToken.str.Length() );
				for( int i=0; i<numCarets; ++i ) { bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "^" ); }
				bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\n" );
			}
			else
			{
				bytesWritten += vsnprintf( m_errorMsg + bytesWritten, kErrBufSize, fmt, args );
				bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\nAt:\n%.*s", 64, firstToken.str.GetPtr() );
			}
		}
		ycAssert( bytesWritten < kErrBufSize );
		va_end( args );
		//printf( m_errorMsg );
		longjmp( m_jmpBuf, 1 );
	}

	void Parser::ErrorIf( const bool failCond, const TokenDesc& firstToken, const TokenDesc& lastToken, const char* fmt, ... )
	{
		if( !failCond ) {}
		else
		{
			ycAssert( m_errorMsg == nullptr );
			m_errorMsg = (char*)YC_ALLOC( m_mem, kErrBufSize, YC_MEM_DEFAULT_ALIGN, "ycShaderParser Error Message" );
			va_list args;
			va_start( args, fmt );
			int bytesWritten = 0;
			if( !firstToken.str.IsNull() && !lastToken.str.IsNull() )
			{
				ycStringRef line; int lineNo, colNo;
				if( m_lexer && m_lexer->GetLineAndColumn( firstToken.str.Get(), line, lineNo, colNo ) ) {
					bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "%d:%d: error: ", lineNo+1, colNo+1 );
					bytesWritten += vsnprintf( m_errorMsg + bytesWritten, kErrBufSize, fmt, args );
					bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\n%.*s\n", (int)line.Length(), line.Get() );
					for( int i=0; i<colNo; ++i ) { bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, line[i] == '\t'? "\t" : " " ); }
					s64 numCarets = ycMin(  line.Length() - colNo, lastToken.str.Get() - firstToken.str.Get() + lastToken.str.Length() );
					for( int i=0; i<numCarets; ++i ) { bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "^" ); }
					bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\n" );
				}
				else {
					bytesWritten += vsnprintf( m_errorMsg + bytesWritten, kErrBufSize, fmt, args );
					bytesWritten += snprintf( m_errorMsg + bytesWritten, kErrBufSize - bytesWritten, "\nAt:\n%.*s", 64, firstToken.str.GetPtr() );
				}
			}
			ycAssert( bytesWritten < kErrBufSize );
			va_end( args );
			//printf( m_errorMsg );
			longjmp( m_jmpBuf, 1 );
		}
	}

	void Parser::RegisterIdentifier( AST::NodeVariableDecl* node )
	{
		// TODO HACK FIXME: ensure no duplicate declarations at same scope depth! (nor name clashes...)
		IdentifierDesc* identifierDesc = m_identifiers.Append();
		identifierDesc->index = uint32_t(node->nameToken->identifierIndex);
		identifierDesc->node  = node;
	}

	void Parser::RegisterIdentifier( AST::NodeFunctionDecl* node )
	{
		// TODO HACK FIXME: this can hash the name multiple times...

		// if a previous declaration exists
		AST::Node* prevDecl = FindIdentifier( node->nameToken );
		if( prevDecl != nullptr )
		{
			// ensure it's a function declaration
			ycAssert( prevDecl->nodeType == AST::kNode_FunctionDecl );

			// deal with overloading
			AST::NodeFunctionDecl* funcDecl = (AST::NodeFunctionDecl*)prevDecl;
			do
			{
				bool match = true;
				AST::Node* arg0 = node->firstArg;
				AST::Node* arg1 = funcDecl->firstArg;
				while( !( arg0 == nullptr && arg1 == nullptr ) ) // stop when we've simultaneously reached the end of both arg lists
				{
					if(
						( arg0 == nullptr && arg1 != nullptr ) ||
						( arg0 != nullptr && arg1 == nullptr ) ||
						GetNodeValueType( arg0 ) != GetNodeValueType( arg1 )
					)
					{
						match = false;
						break;
					}
					arg0 = arg0->nextNode;
					arg1 = arg1->nextNode;
				}

				if( match )
				{
					// if args matched, check that return type does as well
					ycAssert( GetNodeValueType( node ) == GetNodeValueType( funcDecl ) );

					// and stop looking
					break;
				}

				funcDecl = funcDecl->nextOverload;
			}
			while( funcDecl != nullptr );

			// if a matching declaration was found
			if( funcDecl )
			{
				// ensure it has no definition attached
				ycAssert( (funcDecl->flags&AST::kNodeFlag_ForwardDeclaration) != 0 );
				ycAssert( funcDecl->definition == nullptr );

				// and that we do now
				ycAssert( (node->flags&AST::kNodeFlag_ForwardDeclaration) == 0 );
				ycAssert( node->definition );

				// TODO HACK FIXME: handle excess fwd declarations (aside from match-checking, just discard them)

				// attach definition to original declaration
				funcDecl->definition = node->definition;
			}
			// no matching decl was found, add a new overload
			else
			{
				funcDecl = (AST::NodeFunctionDecl*)prevDecl;
				node->nextOverload = funcDecl->nextOverload;
				funcDecl->nextOverload = node;
			}
		}
		// register the declaration
		else
		{
			IdentifierDesc* identifierDesc = m_identifiers.Append();
			identifierDesc->index = uint32_t(node->nameToken->identifierIndex);
			identifierDesc->node  = node;
		}
	}

	AST::Node* Parser::FindIdentifier( const TokenDesc* token )
	{
		if( m_identifiers.Length() == 0 ) { return nullptr; }
		const ureg index = token->identifierIndex;
		ycVector< IdentifierDesc >::Iter iter = m_identifiers.End();
		do
		{
			--iter;
			if( ureg(iter->index) == index ) { return iter->node; }
		}
		while( iter != m_identifiers.Begin() );
		return nullptr;
	}

	ureg Parser::PushScope()
	{
		return m_identifiers.Length();
	}

	void Parser::PopScope( const ureg depth )
	{
		while( m_identifiers.Length() > depth )
		{
			m_identifiers.TakeLast();
		}
	}

	void Parser::RegisterType( const ureg typeIndex, AST::Node* typeNode )
	{
		ycAssert( typeIndex >= kIntrinsicType_Count );
		ycAssert( m_types[ typeIndex ] == nullptr );
		m_types[ typeIndex ] = typeNode;
	}

	AST::Node* Parser::ParseCBufferDefinition()
	{
		AST::Node* node = nullptr;
		if( Peek( kToken_Keyword_CBuffer ) )
		{
			const TokenDesc* firstToken = m_curToken;
			Advance( 1 ); // kToken_Keyword_CBuffer

			AST::NodeCBufferDef* cbDef = CreateNode< AST::NodeCBufferDef >( AST::kNode_CBufferDef );//ycnew( m_nodeMem, "AST CBuffer Definition" )AST::NodeCBufferDef;
			cbDef->firstAttr = nullptr;
			ParseAttributes( &cbDef->firstAttr );
			cbDef->firstMember = nullptr;
			//ParseAggregateDefinition( &cbDef->firstMember ); // dont want push/pop scope, these variables are global
			{
				AST::Node** ppNextMember = &cbDef->firstMember;
				Expect( kToken_BeginBlock );
				while( AST::NodeVariableDecl* member = (AST::NodeVariableDecl*)ParseVariableDeclarationStatement() )
				{
					ycAssert( member->definition == nullptr );
					*ppNextMember = member;
					ppNextMember = &member->nextNode;
				}
				Expect( kToken_EndBlock );
			}
			ErrorIf( cbDef->firstMember == nullptr, *firstToken, *m_curToken, "Empty constant buffer definition." );
			Expect( kToken_Op_Semicolon );
			node = cbDef;

			//RegisterIdentifier( cbDef );
		}
		return node;
	}

	AST::Node* Parser::ParseTypeDeclaration()
	{
		AST::Node* node = nullptr;
		if( Peek( kToken_Keyword_Typedef ) )
		{
			Advance( 1 ); // kToken_Keyword_Typedef

			AST::NodeTypeAlias* typeAlias = CreateNode< AST::NodeTypeAlias >( AST::kNode_TypeAlias );
			typeAlias->baseToken  = Consume( kToken_TypeName );
			typeAlias->aliasToken = Consume( kToken_TypeName );
			Expect( kToken_Op_Semicolon );
			node = typeAlias;

			RegisterType( typeAlias->aliasToken->typeIndex, node );
		}
		else if( Peek( kToken_Keyword_Struct ) )
		{
			const TokenDesc* firstToken = m_curToken;
			Advance( 1 ); // kToken_Keyword_Struct

			AST::NodeStructDef* structDef = CreateNode< AST::NodeStructDef >( AST::kNode_StructDef );
			structDef->nameToken  = Consume( kToken_TypeName );
			structDef->firstMember = nullptr;
			structDef->firstAttr   = nullptr;
			ParseAttributes( &structDef->firstAttr );
			ParseAggregateDefinition( &structDef->firstMember );
			ErrorIf( structDef->firstMember == nullptr, *firstToken, *m_curToken, "Empty struct declaration." );
			Expect( kToken_Op_Semicolon );
			node = structDef;

			RegisterType( structDef->nameToken->typeIndex, node );
		}
		return node;
	}

	AST::Node* Parser::ParseBlock()
	{
		AST::Node* node = nullptr;
		if( Peek( kToken_BeginBlock ) )
		{
			const ureg scope = PushScope();
			Advance( 1 ); // kToken_BeginBlock
			AST::NodeBlock* func = CreateNode< AST::NodeBlock >( AST::kNode_Block );
			func->firstStatement = nullptr;
			AST::Node** nextStatementPP = &func->firstStatement;
			for(;;)
			{
				AST::Node* childNode = nullptr;
				if(      ( childNode = ParseStatement()        ) != nullptr ) {}
				else if( ( childNode = ParseBlock()            ) != nullptr ) {}
				else if( ( childNode = ParseControlFlowBlock() ) != nullptr ) {}
				else if( ( childNode = ParseControlKeyword()   ) != nullptr ) {}
				else { break; }
				*nextStatementPP = childNode;
				nextStatementPP = &childNode->nextNode;
			}
			Expect( kToken_EndBlock );
			PopScope( scope );
			node = func;
		}
		return node;
	}

	AST::Node* Parser::ParseFunctionDeclaration()
	{
		m_isInFunc = true;
		AST::Node* node = nullptr;
		if( Peek( kToken_TypeName, kToken_Identifier, kToken_LeftParen ) )
		{
			const TokenDesc* typeToken = m_curToken; Advance( 1 );
			const TokenDesc* nameToken = m_curToken; Advance( 1 );

			AST::NodeFunctionDecl* decl = CreateNode< AST::NodeFunctionDecl >( AST::kNode_FunctionDecl );
			decl->typeToken    = typeToken;
			decl->nameToken    = nameToken;
			decl->firstArg     = nullptr;
			decl->nextOverload = nullptr;

			AST::SetNodeValueType( decl, typeToken->typeIndex );

			const ureg scope = PushScope();

			// argument list
			ureg numArgs = 0;
			Advance( 1 ); // kToken_LeftParen
			AST::Node** nextArgPP = &decl->firstArg;
			if( AST::NodeVariableDecl* arg = (AST::NodeVariableDecl*)ParseVariableDeclaration() )
			{
				++numArgs;
				ErrorIf( arg->definition != nullptr, *arg->typeToken, m_curToken[-1], "Function arguments do not support default arguments." );
				//ErrorIf( arg->arraySize != 0, true, "Function arguments cannot be arrays." );
				*nextArgPP = arg;
				nextArgPP = &arg->nextNode;
				while( Peek( kToken_Op_Comma ) )
				{
					Advance( 1 ); // kToken_Op_Comma
					arg = (AST::NodeVariableDecl*)ParseVariableDeclaration();
					++numArgs;
					ErrorIf( arg == nullptr, *m_curToken, *m_curToken, "Expected function argument declaration." );
					ErrorIf( arg->definition != nullptr, *arg->typeToken, m_curToken[-1], "Function arguments do not support default values." );
					//ErrorIf( arg->arraySize != 0, true, "Function arguments cannot be arrays." );
					*nextArgPP = arg;
					nextArgPP = &arg->nextNode;
				}
			}
			decl->numArgs = numArgs;
			Expect( kToken_RightParen );

			decl->firstAttr = nullptr;
			ParseAttributes( &decl->firstAttr );

			// includes definition
			if( Peek( kToken_BeginBlock ) )
			{
				m_curFunction = decl;
				decl->definition = (AST::NodeBlock*)ParseBlock();
				m_curFunction = nullptr;
				ErrorIf( decl->definition == nullptr || decl->definition->nodeType != AST::kNode_Block, *m_curToken, *m_curToken, "Expected function definition." );
			}
			else
			{
				decl->definition = nullptr;
				decl->flags |= AST::kNodeFlag_ForwardDeclaration;
				Expect( kToken_Op_Semicolon );
			}

			PopScope( scope );

			RegisterIdentifier( decl );
			node = decl;
		}
		m_isInFunc = false;
		return node;
	}

	AST::Node* Parser::ParseVariableDeclaration()
	{
		AST::Node* node = nullptr;
		ureg modifiers = 0;
		if( Peek( kToken_Keyword_inout ) )
		{
			modifiers |= AST::NodeVariableDecl::kModifier_InOut;
			Advance( 1 );
		}
		else if( Peek( kToken_Keyword_out ) )
		{
			modifiers |= AST::NodeVariableDecl::kModifier_Out;
			Advance( 1 );
		}
		else if( Peek( kToken_Keyword_const ) )
		{
			modifiers |= AST::NodeVariableDecl::kModifier_Const;
			Advance( 1 );
		}
		if( Peek( kToken_TypeName, kToken_Identifier ) )
		{
			const TokenDesc* typeToken = m_curToken; Advance( 1 );
			const TokenDesc* nameToken = m_curToken; Advance( 1 );
			ErrorIf( Peek( kToken_LeftParen ), *typeToken, *m_curToken, "Did not expect function declaration." );

			AST::NodeVariableDecl* decl = CreateNode< AST::NodeVariableDecl >( AST::kNode_VariableDecl );
			decl->flags     = m_isInFunc ? 0 : AST::kNodeFlag_GlobalDecl ;
			AST::SetNodeValueType( decl, typeToken->typeIndex );
			decl->typeToken = typeToken;
			decl->nameToken = nameToken;
			decl->modifiers = modifiers;
			decl->arraySize = 0;

			// array
			if( Peek( kToken_LeftBracket ) )
			{
				Advance( 1 ); // kToken_LeftBracket
				ErrorIf( !Peek( kToken_Literal ) || m_curToken->literalType != kIntrinsicType_Int, *m_curToken, *m_curToken, "Expected integer literal for array size." );
				const s32 arraySize = s32(m_curToken->str.ToInt());
				ErrorIf( arraySize <= 0, *m_curToken, *m_curToken, "Array size must be greater than zero." );
				decl->arraySize = ureg(arraySize);
				Advance( 1 ); // kToken_Literal
				Expect( kToken_RightBracket );
			}

			decl->firstAttr = nullptr;
			ParseAttributes( &decl->firstAttr );

			// includes definition
			if( Peek( kToken_Op_Assign ) )
			{
				Advance( 1 ); // kToken_Op_Assign
				decl->definition = ParseExpression(); // if the comma operator is implemented this should become parsesubexpr
				ErrorIf( decl->definition == nullptr, *nameToken, *m_curToken, "Expected variable definition." );
				ErrorIf( AST::GetNodeValueType( decl->definition ) != typeToken->typeIndex, *nameToken, *m_curToken, "Variable type/assignment rvalue mismatch, cannot convert %s to %s.", typeNames[AST::GetNodeValueType( decl->definition )], typeNames[typeToken->typeIndex] );
			}
			else
			{
				decl->definition = nullptr;
			}
			RegisterIdentifier( decl );
			node = decl;
		}
		else if( modifiers )
		{
			Error( *m_curToken, *m_curToken, "Expected variable declaration." );
		}
		return node;
	}

	AST::Node* Parser::ParseVariableDeclarationStatement()
	{
		// total hack, but occurs are more or less the same places as variable decl statements...
		if( Peek( kToken_Keyword_asm ) )
		{
			Advance( 1 );
			AST::NodeAsm* node = CreateNode< AST::NodeAsm >( AST::kNode_Asm );
			node->val = Consume( kToken_ConstantString );
			return node;
		}

		if( AST::Node* node = ParseVariableDeclaration() )
		{
			Expect( kToken_Op_Semicolon );
			return node;
		}
		return nullptr;
	}

	AST::Node* Parser::ParseControlFlowBlock()
	{
		AST::Node* node = nullptr;

		if( Peek( kToken_Keyword_If ) )
		{
			Advance( 1 ); // kToken_Keyword_If
			Expect( kToken_LeftParen );

			AST::NodeIfBlock* block = CreateNode< AST::NodeIfBlock >( AST::kNode_IfBlock );

			block->condition = ParseExpression();
			ErrorIf( block->condition == nullptr, *m_curToken, *m_curToken, "Expected conditional expression." );
			Expect( kToken_RightParen );

			block->body = (AST::NodeBlock*)ParseBlock();
			ErrorIf( block->body == nullptr || block->body->nodeType != AST::kNode_Block, *m_curToken, *m_curToken, "Expected statement block." );

			AST::Node** ppOtherwise = &block->otherwise;
			block->otherwise = nullptr;
			
			while( Peek( kToken_Keyword_Else, kToken_Keyword_If ) )
			{
				Advance( 2 ); // kToken_Keyword_Else, kToken_Keyword_If
				Expect( kToken_LeftParen );

				AST::NodeIfBlock* elseif = CreateNode< AST::NodeIfBlock >( AST::kNode_IfBlock );

				elseif->condition = ParseExpression();
				ErrorIf( elseif->condition == nullptr, *m_curToken, *m_curToken, "Expected conditional expression." );
				Expect( kToken_RightParen );

				elseif->body = (AST::NodeBlock*)ParseBlock();
				ErrorIf( elseif->body == nullptr || elseif->body->nodeType != AST::kNode_Block, *m_curToken, *m_curToken, "Expected statement block." );

				*ppOtherwise = elseif;
				ppOtherwise = &elseif->otherwise;
				elseif->otherwise = nullptr;
			}

			if( Peek( kToken_Keyword_Else ) )
			{
				Advance( 1 ); // kToken_Keyword_Else
				AST::NodeBlock* body = (AST::NodeBlock*)ParseBlock();
				ErrorIf( body == nullptr || body->nodeType != AST::kNode_Block, *m_curToken, *m_curToken, "Expected statement block." );

				*ppOtherwise = body;
			}

			node = block;
		}
		else if( Peek( kToken_Keyword_For ) )
		{
			Advance( 1 ); // kToken_Keyword_For
			Expect( kToken_LeftParen );

			AST::NodeForBlock* block = CreateNode< AST::NodeForBlock >( AST::kNode_ForBlock );

			block->expr1 = ParseExpression();
			ErrorIf( block->expr1 == nullptr, *m_curToken, *m_curToken, "Expected first expression." );
			Expect( kToken_Op_Semicolon );

			block->expr2 = ParseExpression();
			ErrorIf( block->expr2 == nullptr, *m_curToken, *m_curToken, "Expected second expression." );
			Expect( kToken_Op_Semicolon );

			block->expr3 = ParseExpression();
			ErrorIf( block->expr3 == nullptr, *m_curToken, *m_curToken, "Expected third expression." );
			Expect( kToken_RightParen );

			block->body = (AST::NodeBlock*)ParseBlock();
			ErrorIf( block->body == nullptr || block->body->nodeType != AST::kNode_Block, *m_curToken, *m_curToken, "Expected statement block." );

			node = block;
		}

		return node;
	}

	AST::Node* Parser::ParseControlKeyword()
	{
		AST::Node* node = nullptr;

		if( Peek( kToken_Keyword_Return ) )
		{
			const TokenDesc* firstToken = m_curToken;
			ErrorIf( m_curFunction == nullptr, *firstToken, *firstToken, "'return' encountered outside of a function." ); // should we allow in entry points?
			Advance( 1 ); // kToken_Keyword_Return

			AST::NodeReturn* block = CreateNode< AST::NodeReturn >( AST::kNode_Return );

			block->val = ParseExpression();
			Expect( kToken_Op_Semicolon );

			if( block->val )
			{
				ErrorIf( AST::GetNodeValueType( block->val ) != AST::GetNodeValueType( m_curFunction ), *firstToken, *m_curToken, "Mismatched return type." );
			}
			else
			{
				ErrorIf( m_curFunction->typeToken->typeIndex != kIntrinsicType_Void, *firstToken, *m_curToken, "Function expected return value." );
			}

			node = block;
		}

		return node;
	}

	AST::Node* Parser::ParseStatement()
	{
		AST::Node* node = nullptr;
		if(      ( node = ParseVariableDeclarationStatement() ) != nullptr ) {}
		else if( ( node = ParseExpression()                   ) != nullptr ) { Expect( kToken_Op_Semicolon ); }
		return node;
	}

	AST::Node* Parser::ParseExpression()
	{
		AST::Node* node = nullptr;
		if(      ( node = ParseVariableDeclaration() ) != nullptr ) {}
		else if( ( node = ParseSubExpression()       ) != nullptr ) {}
		return node;
	}
	
	AST::Node* Parser::ParseSubExpression( const ureg minPrecedence /*= 0*/ )
	{
		AST::Node* node = nullptr;

		if(      ( node = ParseLiteral()       ) != nullptr ) {}
		else if( ( node = ParseFunctionCall()  ) != nullptr ) {}
		else if( ( node = ParseVariable()      ) != nullptr ) {}
		else if( ( node = ParseUnaryOperator() ) != nullptr ) {}
		else if( ( node = ParseCastOperator()  ) != nullptr ) {}
		else if( Peek( kToken_LeftParen ) )
		{
			Advance( 1 ); // kToken_LeftParen
			node = ParseExpression();
			ErrorIf( node == nullptr, *m_curToken, *m_curToken, "Expected expression." );
			Expect( kToken_RightParen );
		}

		if( node )
		{
			if( AST::Node* infixOp = ParseInfixOperations( node, minPrecedence ) )
			{
				node = infixOp;
			}
		}

		return node;
	}
	
	AST::Node* Parser::ParseInfixOperations( AST::Node* lhs, const ureg minPrecedence /*= 0*/ )
	{
		AST::Node* node = lhs;

		// check for member access operator
		while( Peek( kToken_Op_Dot ) )
		{
			const TokenDesc* dotToken = m_curToken;
			Advance( 1 ); // kToken_Op_Dot

			const uint32_t lhsValueType = AST::GetNodeValueType( node );

			AST::NodeMemberAccess* op = CreateNode< AST::NodeMemberAccess >( AST::kNode_MemberAccessOp );
			op->lhs         = node;
			op->memberToken = Consume( kToken_Identifier );
			// store more than just token??? ptr directly into the struct member definition might be useful... (maybe union it, builtins get member token, user types get definition)

			const bool handled = IsAggregateType( lhsValueType ) ? ParseIntrinsicMemberAccess( op ) : ParseUserMemberAccess( op ) ;
			ErrorIf( !handled, *dotToken, *dotToken, "Could not parse member access (.) operator for this type for member '%.*s'.", int(op->memberToken->str.Length()), op->memberToken->str.GetPtr() );

			node = op;
		}

		// check for standard infix operations
		ureg precedence;
		while(
			( precedence = GetTokenInfixOpPrecedence( m_curToken->type ) ) != 0
			&&
			precedence >= minPrecedence
		)
		{
			AST::NodeInfixOp* op = CreateNode< AST::NodeInfixOp >( AST::kNode_InfixOp );
			op->opToken  = m_curToken; Advance( 1 );
			op->lhs      = node;
			op->rhs      = ParseSubExpression( precedence+1 );
			ErrorIf( op->rhs == nullptr, *op->opToken, *m_curToken, "Infix operator expected expression." );

			const uint32_t lhsValueType = AST::GetNodeValueType( op->lhs );
			const uint32_t rhsValueType = AST::GetNodeValueType( op->rhs );
			if(
				lhsValueType == kIntrinsicType_Void || lhsValueType > kIntrinsicType_End
				||
				rhsValueType == kIntrinsicType_Void || rhsValueType > kIntrinsicType_End
			)
			{
				// TODO HACK FIXME: show type name
				Error( *op->opToken, *op->opToken, "Cannot use infix operator '%.*s' on this type.", int(op->opToken->str.Length()), op->opToken->str.GetPtr() );
			}

			u32 retType = kIntrinsicType_Void;

			TokenType tokenType = op->opToken->type;
			if( tokenType == kToken_Op_Star || tokenType == kToken_Op_MulAssign )
			{
				// handle special cases for *

				if( IsMatrixType( lhsValueType ) && IsAggregateType( rhsValueType ) )
				{
					// matrix<M, N> * vector<N> returns vector<M>

					const u32 mtxScalarType = GetAggregateScalarType( GetMatrixAggregateType( lhsValueType ) );
					const u32 vecScalarType = GetAggregateScalarType( rhsValueType );
					ErrorIf( mtxScalarType != vecScalarType, *op->opToken, *op->opToken, "Matrix multiply of two different base types not allowed." );

					const ureg mtxNumColumns = GetIntrinsicTypeNumColumns( lhsValueType );
					const ureg mtxNumRows = GetIntrinsicTypeNumRows( lhsValueType );
					const ureg vecNumComponents = GetIntrinsicTypeNumComponents( rhsValueType );
					ErrorIf( mtxNumColumns != vecNumComponents, *op->opToken, *op->opToken, "Matrix multiply has incorrect geometry. matrix<%d,%d> * vector<%d>", mtxNumRows, mtxNumColumns, vecNumComponents );

					retType = GetAggregateTypeForNumComponents( mtxScalarType, mtxNumRows );
				}
				else if( IsAggregateType( lhsValueType ) && IsMatrixType( rhsValueType ) )
				{
					// vector<M> * matrix<M, N> returns vector<N>

					const u32 vecScalarType = GetAggregateScalarType( lhsValueType );
					const u32 mtxScalarType = GetAggregateScalarType( GetMatrixAggregateType( rhsValueType ) );
					ErrorIf( vecScalarType != mtxScalarType, *op->opToken, *op->opToken, "Matrix multiply of two different base types not allowed." );

					const ureg vecNumComponents = GetIntrinsicTypeNumComponents( lhsValueType );
					const ureg mtxNumColumns = GetIntrinsicTypeNumColumns( rhsValueType );
					const ureg mtxNumRows = GetIntrinsicTypeNumRows( rhsValueType );
					ErrorIf( vecNumComponents != mtxNumRows, *op->opToken, *op->opToken, "Matrix multiply has incorrect geometry. vector<%d> * matrix<%d,%d>", vecNumComponents, mtxNumRows, mtxNumColumns );

					retType = GetAggregateTypeForNumComponents( mtxScalarType, mtxNumColumns );
				}
			}
			
			// vec*scalar returns vec (also applies for +, -, /, etc.)
			if( IsAggregateType( lhsValueType ) && IsScalarType(    rhsValueType ) ) { retType = lhsValueType; }
			else if( IsScalarType(    lhsValueType ) && IsAggregateType( rhsValueType ) ) { retType = rhsValueType; }

			// mtx*scalar returns mtx (also applies for +, -, /, etc.)
			else if( IsMatrixType( lhsValueType ) && IsScalarType( rhsValueType ) ) { retType = lhsValueType; }
			else if( IsScalarType( lhsValueType ) && IsMatrixType( rhsValueType ) ) { retType = rhsValueType; }

			if ( retType == kIntrinsicType_Void )
			{
				// otherwise, types must match
				ErrorIf( lhsValueType != rhsValueType, *op->opToken, *m_curToken, "Variable type mismatch, operator '%.*s' does not allow %s %.*s %s.", (int)op->opToken->str.Length(), op->opToken->str.Get(), typeNames[lhsValueType], (int)op->opToken->str.Length(), op->opToken->str.Get(), typeNames[rhsValueType] );
				const u32 otherRetType = GetTokenInfixOpValType( op->opToken->type );
				retType = otherRetType ? otherRetType : lhsValueType;
			}

			AST::SetNodeValueType( op, retType );

			node = op;
		}
		return node;
	}

	AST::Node* Parser::ParseLiteral()
	{
		AST::NodeLiteral* node = nullptr;
		if( Peek( kToken_Literal ) )
		{
			node = CreateNode< AST::NodeLiteral >( AST::kNode_Literal );
			AST::SetNodeValueType( node, m_curToken->literalType );
			node->valueToken = m_curToken; Advance( 1 ); // kToken_Literal
		}
		return node;
	}

	AST::Node* Parser::ParseFunctionCall()
	{
		AST::NodeFunctionCall* node = nullptr;
		if( Peek( kToken_Identifier, kToken_LeftParen ) )
		{
			const TokenDesc* firstToken = m_curToken;
			AST::NodeFunctionDecl* func = (AST::NodeFunctionDecl*)FindIdentifier( m_curToken );
			ErrorIf( func == nullptr || func->nodeType != AST::kNode_FunctionDecl, *m_curToken, *m_curToken, "Unknown function '%.*s'", int(m_curToken->str.Length()), m_curToken->str.GetPtr() );

			// construct ast node
			node = CreateNode< AST::NodeFunctionCall >( AST::kNode_FunctionCall );
			Advance( 2 ); // kToken_Identifier, kToken_LeftParen

			// parse function arguments
			ureg numArgs = 0;
			node->firstArg = nullptr;
			AST::Node** nextArgPP = &node->firstArg;
			if( AST::Node* arg = ParseExpression() )
			{
				++numArgs;
				*nextArgPP = arg;
				nextArgPP = &arg->nextNode;
				while( Peek( kToken_Op_Comma ) )
				{
					Advance( 1 ); // kToken_Op_Comma
					const TokenDesc* afterComma = m_curToken;
					arg = ParseExpression();
					ErrorIf( arg == nullptr, *afterComma, *afterComma, "Expected expression." );
					++numArgs;
					*nextArgPP = arg;
					nextArgPP = &arg->nextNode;
				}
			}
			Expect( kToken_RightParen );

			// validate function arguments and identify which overload to use
			AST::NodeFunctionDecl* overload = func;
			do
			{
				bool match = true;
				AST::Node* arg0 = node->firstArg;
				AST::Node* arg1 = overload->firstArg;
				while( !( arg0 == nullptr && arg1 == nullptr ) ) // stop when we've simultaneously reached the end of both arg lists
				{
					if(
						( arg0 == nullptr && arg1 != nullptr ) ||
						( arg0 != nullptr && arg1 == nullptr ) ||
						GetNodeValueType( arg0 ) != GetNodeValueType( arg1 )
					)
					{
						match = false;
						break;
					}
					arg0 = arg0->nextNode;
					arg1 = arg1->nextNode;
				}

				if( match ) { break; }

				overload = overload->nextOverload;
			} while( overload );
			
			// associate the matching overload with the function call node
			if( overload )
			{
				node->func = overload;
				AST::SetNodeValueType( node, overload->typeToken->literalType );
			}
			// or flip out
			else
			{
				// TODO HACK FIXME: show provided arg types, and all available overloads
				Error( *firstToken, *m_curToken, "No overloads matched for function '%.*s'.", int(func->nameToken->str.Length()), func->nameToken->str.GetPtr() );
			}
		}
		return node;
	}

	AST::Node* Parser::ParseVariable()
	{
		AST::Node* pNode = nullptr;
		if( Peek( kToken_Identifier ) )
		{
			const TokenDesc* varnameToken = m_curToken;
			AST::NodeVariable* node = CreateNode< AST::NodeVariable >( AST::kNode_Variable );
			node->var = (AST::NodeVariableDecl*)FindIdentifier( m_curToken );
			ErrorIf( node->var == nullptr || node->var->nodeType != AST::kNode_VariableDecl, *m_curToken, *m_curToken, "Could not find variable '%.*s'.", int(m_curToken->str.Length()), m_curToken->str.GetPtr() );
			Advance( 1 ); // kToken_Identifier

			AST::SetNodeValueType( node, node->var->typeToken->literalType );
			pNode = node;

			while( YC_UNLIKELY( Peek( kToken_LeftBracket ) ))
			{
				Advance( 1 ); // kToken_LeftBracket

				AST::NodeArrayAccess* nodeArray = CreateNode< AST::NodeArrayAccess >( AST::kNode_ArrayAccessOp );
				nodeArray->lhs = pNode;

				u32 valueType = AST::GetNodeValueType( pNode );
				
				nodeArray->arrayIdx = ParseExpression();
				ErrorIf( nodeArray->arrayIdx == nullptr, *m_curToken, *m_curToken, "Expected array subscript accessing variable '%.*s'.", int(m_curToken->str.Length()), m_curToken->str.GetPtr() );
				ureg arrayIdxType = AST::GetNodeValueType( nodeArray->arrayIdx );
				ErrorIf( arrayIdxType != kIntrinsicType_Int && arrayIdxType != kIntrinsicType_UInt, *m_curToken, *m_curToken, "Array index is not an integer when accessing array variable '%.*s'.", int(m_curToken->str.Length()), m_curToken->str.GetPtr() );
				// TODO: bounds check!!!!
				Consume( kToken_RightBracket );

				if( node->var->arraySize > 0 ) { /*standard array index*/ }
				else if( IsAggregateType( valueType ) ) { valueType = u32( GetAggregateScalarType( valueType ) ); }
				else if( IsMatrixType( valueType ) ) { valueType = u32( GetMatrixAggregateType( valueType ) ); }
				else
				{
					Error( *varnameToken, *m_curToken, "Could not index into non-array variable '%.*s'.", int( m_curToken->str.Length() ), m_curToken->str.GetPtr() );
				}
				AST::SetNodeValueType( nodeArray, valueType );

				pNode = nodeArray;
			}
		}
		return pNode;
	}
	
	AST::Node* Parser::ParseUnaryOperator()
	{
		AST::NodeUnaryOp* node = nullptr;
		if( const ureg precedence = GetTokenUnaryOpPrecedence( m_curToken->type ) )
		{
			node = CreateNode< AST::NodeUnaryOp >( AST::kNode_UnaryOp );
			node->opToken  = m_curToken; Advance( 1 ); // kToken_Op_*
			node->operand  = ParseSubExpression( precedence );
			ErrorIf( node->operand == nullptr, node->opToken[1], node->opToken[1], "Unary operator expected expression." );

			const uint32_t operatorValueType = AST::GetNodeValueType( node->operand );
			// TODO HACK FIXME: show type name
			ErrorIf( operatorValueType == kIntrinsicType_Void || operatorValueType > kIntrinsicType_End, node->opToken[1], node->opToken[1], "Cannot use unary operator '%.*s' on this type.", int(node->opToken->str.Length()), node->opToken->str.GetPtr() );
			const u32 retType = GetTokenUnaryOpValType( node->opToken->type );
			AST::SetNodeValueType( node, retType ? retType : operatorValueType );
		}
		return node;
	}
	
	AST::Node* Parser::ParseCastOperator()
	{
		AST::NodeCastOp* node = nullptr;
		if( Peek( kToken_TypeName, kToken_LeftParen ) )
		{
			node = CreateNode< AST::NodeCastOp >( AST::kNode_CastOp );
			node->typeToken = m_curToken; 

			const u32 castType = node->typeToken->literalType;
			AST::SetNodeValueType( node, castType );
			const ureg numExpectedComponents = GetIntrinsicTypeNumComponents( castType );
			ycAssert( numExpectedComponents <= 16 );
			ErrorIf( numExpectedComponents == 0, node->typeToken[1], *m_curToken, "Cannot use casting operator on this type." );

			Advance( 2 ); // kToken_TypeName, kToken_LeftParen

			// parse arguments
			ureg numFoundComponents = 0;
			ureg typeNumFoundComponents = 0;
			node->firstArg = nullptr;
			AST::Node** nextArgPP = &node->firstArg;
			AST::Node* arg = ParseExpression();
			while( arg )
			{
				numFoundComponents++;

				ureg typeNumComponents = GetIntrinsicTypeNumComponents( AST::GetNodeValueType( arg ) );
				ErrorIf( typeNumComponents == 0, *node->typeToken, *node->typeToken, "Invalid type used in casting operator." );
				typeNumFoundComponents += typeNumComponents;

				*nextArgPP = arg;
				nextArgPP = &arg->nextNode;

				if( Peek( kToken_Op_Comma ) )
				{
					Advance( 1 );
					arg = ParseExpression();
					ErrorIf( arg == nullptr, *m_curToken, *m_curToken, "Expected expression." );
				}
				else
				{
					arg = nullptr;
				}
			}
			Expect( kToken_RightParen );

			ErrorIf( numFoundComponents != numExpectedComponents && typeNumFoundComponents != numExpectedComponents, *node->typeToken, *m_curToken, "Incorrect number of arguments for cast '%.*s', expected %u got %u.", int(node->typeToken->str.Length()), node->typeToken->str.GetPtr(), u32(numExpectedComponents), u32(numFoundComponents) );
		}
		return node;
	}
	
	void Parser::ParseAttributes( AST::Attribute** ppNextAttr )
	{
		if( Peek( kToken_Op_Colon ) )
		{
			do
			{
				Advance( 1 ); // kToken_Op_Colon || kToken_Op_Comma

				AST::Attribute* attr = ycnew( m_nodeMem, "AST Node Attribute" )AST::Attribute;
				attr->nameToken = Consume( kToken_Identifier );
				if( Peek( kToken_LeftParen ) )
				{
					Advance( 1 ); // kToken_LeftParen
					if( !Peek( kToken_RightParen ) )
					{
						ErrorIf(
							!Peek( kToken_Literal ) && !Peek( kToken_Identifier ) && !Peek( kToken_ConstantString ),
							*m_curToken, *m_curToken, "Could not parse attribute value."
						);
						attr->valueToken = m_curToken;
						Advance( 1 ); // kToken_Literal || kToken_Identifier || kToken_ConstantString
					}
					else
					{
						attr->valueToken = nullptr;
					}
					Expect( kToken_RightParen );
				}
				else
				{
					attr->valueToken = nullptr;
				}
				attr->nextAttr = nullptr;

				*ppNextAttr = attr;
				ppNextAttr = &attr->nextAttr;
			}
			while( Peek( kToken_Op_Comma ) );
		}
	}

	bool Parser::ParseIntrinsicMemberAccess( AST::NodeMemberAccess* op )
	{
		const uint32_t lhsValueType = AST::GetNodeValueType( op->lhs );
		if( !IsAggregateType( lhsValueType ) ) { return false; }
		ycAssert( op->memberToken->type == kToken_Identifier );

		const TokenDesc* identifierToken = op->memberToken;
		const ureg identifierLength = identifierToken->str.Length();
		const char* identifier = identifierToken->str.GetPtr();
		ycAssert( identifierLength );
		if( identifierLength > 4 ) { return false; } // short-circuit for 'definitely bad'

		ureg vecWidth;
		u32 vecBaseType;
		switch( lhsValueType )
		{
		case kIntrinsicType_Float1: vecWidth = 1; vecBaseType = kIntrinsicType_Float1; break;
		case kIntrinsicType_Float2: vecWidth = 2; vecBaseType = kIntrinsicType_Float1; break;
		case kIntrinsicType_Float3: vecWidth = 3; vecBaseType = kIntrinsicType_Float1; break;
		case kIntrinsicType_Float4: vecWidth = 4; vecBaseType = kIntrinsicType_Float1; break;
		case kIntrinsicType_Int1:   vecWidth = 1; vecBaseType = kIntrinsicType_Int1; break;
		case kIntrinsicType_Int2:   vecWidth = 2; vecBaseType = kIntrinsicType_Int1; break;
		case kIntrinsicType_Int3:   vecWidth = 3; vecBaseType = kIntrinsicType_Int1; break;
		case kIntrinsicType_Int4:   vecWidth = 4; vecBaseType = kIntrinsicType_Int1; break;
		case kIntrinsicType_UInt1:  vecWidth = 1; vecBaseType = kIntrinsicType_UInt1; break;
		case kIntrinsicType_UInt2:  vecWidth = 2; vecBaseType = kIntrinsicType_UInt1; break;
		case kIntrinsicType_UInt3:  vecWidth = 3; vecBaseType = kIntrinsicType_UInt1; break;
		case kIntrinsicType_UInt4:  vecWidth = 4; vecBaseType = kIntrinsicType_UInt1; break;
		case kIntrinsicType_Bool1:  vecWidth = 1; vecBaseType = kIntrinsicType_Bool1; break;
		case kIntrinsicType_Bool2:  vecWidth = 2; vecBaseType = kIntrinsicType_Bool1; break;
		case kIntrinsicType_Bool3:  vecWidth = 3; vecBaseType = kIntrinsicType_Bool1; break;
		case kIntrinsicType_Bool4:  vecWidth = 4; vecBaseType = kIntrinsicType_Bool1; break;
		YC_NO_DEFAULT_ASSERT
		}

		//const bool isFloatVec = lhsValueType >= kIntrinsicType_Float1 && lhsValueType <= kIntrinsicType_Float4; // assumes int if not! TODO HACK FIXME: bool?

		const char members[8] = { 'x', 'r', 'y', 'g', 'z', 'b', 'w', 'a' };
		//const ureg numValidMemberNames = ( (lhsValueType - (isFloatVec?kIntrinsicType_Float1:kIntrinsicType_Int1)) + 1 ) * 2;
		const ureg numValidMemberNames = vecWidth * 2;

		for( uint32_t swizzleIdx = 0; swizzleIdx != identifierLength; ++swizzleIdx )
		{
			const char member = identifier[ swizzleIdx ];
			bool valid = false;
			for( ureg aliasIdx = 0; aliasIdx != numValidMemberNames; ++aliasIdx )
			{
				if( member == members[ aliasIdx ] ) { valid = true; break; }
			}
			if( valid == false ) { return false; }
		}

		//if( isFloatVec )
		//{
		//	const uint32_t valueTypes[4] = { kIntrinsicType_Float, kIntrinsicType_Float2, kIntrinsicType_Float3, kIntrinsicType_Float4 };
		//	AST::SetNodeValueType( op, valueTypes[ identifierLength-1 ] );
		//}
		//else
		//{
		//	const uint32_t valueTypes[4] = { kIntrinsicType_Int, kIntrinsicType_Int2, kIntrinsicType_Int3, kIntrinsicType_Int4 };
		//	AST::SetNodeValueType( op, valueTypes[ identifierLength-1 ] );
		//}
		u32 valueTypes[4];
		switch( vecBaseType )
		{
		case kIntrinsicType_Float1: valueTypes[0] = kIntrinsicType_Float; valueTypes[1] = kIntrinsicType_Float2; valueTypes[2] = kIntrinsicType_Float3; valueTypes[3] = kIntrinsicType_Float4; break;
		case kIntrinsicType_Int1:   valueTypes[0] = kIntrinsicType_Int;   valueTypes[1] = kIntrinsicType_Int2;   valueTypes[2] = kIntrinsicType_Int3;   valueTypes[3] = kIntrinsicType_Int4;   break;
		case kIntrinsicType_UInt1:  valueTypes[0] = kIntrinsicType_UInt;  valueTypes[1] = kIntrinsicType_UInt2;  valueTypes[2] = kIntrinsicType_UInt3;  valueTypes[3] = kIntrinsicType_UInt4;  break;
		case kIntrinsicType_Bool1:  valueTypes[0] = kIntrinsicType_Bool;  valueTypes[1] = kIntrinsicType_Bool2;  valueTypes[2] = kIntrinsicType_Bool3;  valueTypes[3] = kIntrinsicType_Bool4;  break;
		YC_NO_DEFAULT_ASSERT
		}
		AST::SetNodeValueType( op, valueTypes[ identifierLength-1 ] );

		return true;
	}

	bool Parser::ParseUserMemberAccess( AST::NodeMemberAccess* op )
	{
		const uint32_t lhsValueType = AST::GetNodeValueType( op->lhs );
		ycAssert( lhsValueType < m_types.Length() );
		if( !IsUserType( lhsValueType ) ) { return false; }

		AST::Node* typeNode = m_types[ lhsValueType ];

		if( typeNode->nodeType != AST::kNode_StructDef ) { return false; } // TODO HACK FIXME: user types should always be structs?

		AST::NodeStructDef* structDef = (AST::NodeStructDef*)typeNode;

		ycAssert( op->memberToken->type == kToken_Identifier );
		ureg memberId = op->memberToken->identifierIndex;
		for(
			AST::NodeVariableDecl* member = (AST::NodeVariableDecl*)structDef->firstMember;
			member != nullptr;
			member = (AST::NodeVariableDecl*)member->nextNode
		)
		{
			ycAssert( member->nodeType == AST::kNode_VariableDecl );
			if( member->nameToken->identifierIndex == memberId )
			{
				AST::SetNodeValueType( op, AST::GetNodeValueType( member ) );
				return true;
			}
		}

		return false;
	}

	void Parser::ParseAggregateDefinition( AST::Node** ppNextMember )
	{
		const ureg scope = PushScope(); // variables in this scope don't matter, but we need to make sure to clean them because variable declaration parsing will add them
		Expect( kToken_BeginBlock );
		const TokenDesc* varStartToken = m_curToken;
		while( AST::NodeVariableDecl* member = (AST::NodeVariableDecl*)ParseVariableDeclarationStatement() )
		{
			if( member->definition != nullptr )
			{
				Error( *varStartToken, *m_curToken, "Default argument values are not supported in aggregate definitions." );
			}
			*ppNextMember = member;
			ppNextMember = &member->nextNode;
			varStartToken = m_curToken;
		}
		Expect( kToken_EndBlock );
		PopScope( scope );
	}

	void Parser::InitTypes()
	{
		ycMemSet( &m_types[0], 0, sizeof(AST::Node*)*m_types.Length() );
	}

} // namespace ycShaderBuilder
