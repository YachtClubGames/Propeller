#pragma once
#ifndef YC_SHADERGEN_H
#define YC_SHADERGEN_H

#include "ycCommon.h"

// core
#include "ycStringRef.h"

// shader builder
#include "ycShaderAST.h"

class ycAllocator;
class ycByteBuffer;

namespace ycShaderBuilder
{
	namespace AST { struct Node; };
	class Parser;
	class Gen
	{
	public:

		Gen( ycAllocator* mem = nullptr );
		virtual ~Gen();

		void Generate( ycByteBuffer* dst, ycByteBuffer* dstMetaData, Parser* parser );

	protected:

		ycAllocator*  m_mem;
		ycByteBuffer* m_dst;
		ycByteBuffer* m_dstMetaData;
		Parser*       m_parser;
		ureg          m_indent;

		/*
		Flags are kinda weird, may or may not be passed to child nodes... is there a better way to handle this? Guess see what flags we wind up needing first.
		*/
		enum
		{
			kFlag_Statement = 1<<0, //!< not always used/obeyed, generally only used by things that can be expr or statement (eg struct def is always stmt)
			kFlag_NoParens  = 1<<1, //!< used when putting parens around an expr would cause problems

			kFlags_End
		};

		void ProcessNode( AST::Node* node, const ureg flags = 0 );
		void ProcessNodeList( AST::Node* firstNode, const ureg flags = 0 );

		virtual void Preamble() {} //!< called before generation of nodes begins, used to insert target-specific stuff at the beginning of generated code
		virtual void Epilogue() {} //!< called after code gen ends
		virtual void Process( AST::NodeTypeAlias*       node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeStructDef*       node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeLiteral*         node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeVariable*        node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeFunctionCall*    node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeUnaryOp*         node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeInfixOp*         node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeCastOp*          node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeArrayAccess*     node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeMemberAccess*    node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeVariableDecl*    node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeFunctionDecl*    node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeBlock*           node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeIfBlock*         node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeCBufferDef*      node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeReturn*          node, const ureg flags = 0 ) = 0;
		virtual void Process( AST::NodeForBlock*        node, const ureg flags = 0 ) = 0;

		void PushIndent();
		void PopIndent();
		void EmitIndent();
		void Emit( const char* str );
		void Emit( const char* str, const ureg strLen );
		void Emit( const ycStringRef& str );
		void Emit( const TokenDesc* token );
		void Emit( const float num );
		void Emit( const double num ) { Emit( float(num) ); } // not great but we dont need to emit doubles atm, add later
		void Emit( const int32_t num );
		void Emit( const uint32_t num );
		void Emit( const int64_t num ) { Emit( int32_t(num) ); } // ditto
		void Emit( const uint64_t num ) { Emit( uint32_t(num) ); } // ditto
		
		void AddMetaAttribute( const uint32_t index, const char* name );
		void AddMetaAttribute( const uint32_t index, const char* name, const ureg nameLen );
		void AddMetaAttribute( const uint32_t index, const ycStringRef& name );
		void AddMetaUniformBuffer( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name );
		void AddMetaUniformBuffer( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name, const ureg nameLen );
		void AddMetaUniformBuffer( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const ycStringRef& name );
		void AddMetaSampler( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name );
		void AddMetaSampler( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name, const ureg nameLen );
		void AddMetaSampler( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const ycStringRef& name );
	};
} // namespace ycShaderBuilder

#endif // !YC_SHADERGEN_H
