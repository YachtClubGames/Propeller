#pragma once
#ifndef YC_SHADERPARSER_H
#define YC_SHADERPARSER_H

#include "ycCommon.h"

// core
#include "ycPagedLinearAllocator.h"
#include "ycVector.h"

// shader builder
#include "ycShaderAST.h"
#include "ycShaderTokens.h"

// error handling, forgive me
#include <setjmp.h>

namespace ycShaderBuilder
{
	namespace AST
	{
		struct Node;
		struct NodeFunctionDecl;
		struct NodeVariableDecl;
		struct Attribute;
		struct NodeMemberAccess;
	};
	class Lexer;
	class Parser
	{
	public:

		Parser( ycAllocator* mem = nullptr );
		~Parser();

		bool Parse( Lexer* input ); //!< returns false if parsing failed, use GetError() to retrieve the error message
		inline const char* GetError() const { return m_errorMsg; }

		inline AST::Node* GetFirstStatement() { return m_firstStatement; }
		inline AST::Node* GetTypeDesc( const ureg index ) { return m_types[ index ]; }

	protected:

		ycAllocator*           m_mem;
		ycPagedLinearAllocator m_nodeMem;
		AST::Node*             m_firstStatement;
		const TokenDesc*       m_tokens;
		const TokenDesc*       m_curToken;
		AST::NodeFunctionDecl* m_curFunction; // set while inside of a function body (used by return to type check)
		bool                   m_isInFunc;   // to tell whether variable declarations are global or not

		//! list of known identifiers at the current point in parsing (eg, does not contain references to nodes in scopes we have exited!)
		// TODO HACK FIXME: SoA would be better.
		struct IdentifierDesc
		{
			uint32_t    index;   // more like a hash key really: the index in an array of all identifiers post-name-hashing
			uint32_t    _unused; // scope stuff?
			AST::Node*  node;    // variable or function *declarations*
		};
		ycVector< IdentifierDesc >   m_identifiers; // TODO HACK FIXME: stack data structure?
		ycVector< AST::Node* > m_types; // NULL for intrinsic types or types that have not yet been encountered; NodeTypeAlias or NodeStructDef for user types

		// error handling
		#ifdef YC_MSVC // structure was padded due to alignment specifier
			__pragma( warning( push ) )
			__pragma( warning( disable: 4324 ) )
		#endif
		jmp_buf m_jmpBuf;
		#ifdef YC_MSVC
			__pragma( warning( pop ) )
		#endif
		const Lexer* m_lexer;
		char*   m_errorMsg;
		enum { kErrBufSize = 4096 };
		void Error( const TokenDesc& firstToken, const TokenDesc& lastToken, const char* fmt, ... );
		void ErrorIf( const bool failCond, const TokenDesc& firstToken, const TokenDesc& lastToken, const char* fmt, ... );

		void RegisterIdentifier( AST::NodeVariableDecl* node );
		void RegisterIdentifier( AST::NodeFunctionDecl* node );
		AST::Node* FindIdentifier( const TokenDesc* token );

		ureg PushScope();
		void PopScope( const ureg depth );

		void RegisterType( const ureg typeIndex, AST::Node* typeNode );

		AST::Node* ParseCBufferDefinition();
		AST::Node* ParseTypeDeclaration();
		AST::Node* ParseBlock();
		AST::Node* ParseFunctionDeclaration();
		AST::Node* ParseVariableDeclaration();
		AST::Node* ParseVariableDeclarationStatement();
		AST::Node* ParseControlFlowBlock(); // handles things like if( 1 ) {}
		AST::Node* ParseControlKeyword(); // handles things like return, continue, break (..discard?)
		AST::Node* ParseStatement();
		AST::Node* ParseExpression();
		AST::Node* ParseSubExpression( const ureg minPrecedence = 0 );
		AST::Node* ParseInfixOperations( AST::Node* lhs, const ureg minPrecedence );
		AST::Node* ParseLiteral();
		AST::Node* ParseFunctionCall();
		AST::Node* ParseVariable();
		AST::Node* ParseUnaryOperator();
		AST::Node* ParseCastOperator();
		void ParseAttributes( AST::Attribute** firstAttr );
		bool ParseIntrinsicMemberAccess( AST::NodeMemberAccess* op );
		bool ParseUserMemberAccess( AST::NodeMemberAccess* op );
		void ParseAggregateDefinition( AST::Node** firstMember );

		void InitTypes();

		//
		// parse utils
		//
		
		inline void Advance( const ureg numTokens = 1 );

		inline bool Peek( TokenType token0 );
		inline bool Peek( TokenType token0, TokenType token1 );
		inline bool Peek( TokenType token0, TokenType token1, TokenType token2 );
		inline bool Peek( TokenType token0, TokenType token1, TokenType token2, TokenType token3 );

		inline void Expect( TokenType token0 );
		inline void Expect( TokenType token0, TokenType token1 );
		inline void Expect( TokenType token0, TokenType token1, TokenType token2 );
		inline void Expect( TokenType token0, TokenType token1, TokenType token2, TokenType token3 );

		inline const TokenDesc* Consume( TokenType token );

		template< class t_nodeType > t_nodeType* CreateNode( const u32 nodeType = AST::kNode_Invalid );
	};
} // namespace ycShaderBuilder


#endif // !YC_SHADERPARSER_H
