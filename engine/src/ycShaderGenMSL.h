#pragma once
#ifndef YC_SHADERGENMSL_H
#define YC_SHADERGENMSL_H

#include "ycCommon.h"
#include "ycShaderGen.h"
#include "ycVector.h"

class ycAllocator;
class ycByteBuffer;

namespace ycShaderBuilder
{
	class GenMSL : public Gen
	{
	public:

		enum
		{
			kTarget_Metal1_0,
		};
		
		enum
		{
			kStage_Unknown,
			kStage_Vert,
			kStage_Frag
		};

		GenMSL( ycAllocator* mem = nullptr, const uint32_t stage = kStage_Unknown, const uint32_t target = kTarget_Metal1_0 );
		virtual ~GenMSL();

	protected:

		uint32_t m_target;

		virtual void Preamble() override;
		virtual void Epilogue() override;
		virtual void Process( AST::NodeTypeAlias*       node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeStructDef*       node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeLiteral*         node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeVariable*        node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeFunctionCall*    node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeUnaryOp*         node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeInfixOp*         node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeCastOp*          node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeArrayAccess*     node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeMemberAccess*    node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeVariableDecl*    node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeFunctionDecl*    node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeBlock*           node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeIfBlock*         node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeCBufferDef*      node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeReturn*          node, const ureg flags = 0 ) override;
		virtual void Process( AST::NodeForBlock*        node, const ureg flags = 0 ) override;

		AST::NodeStructDef* m_structDef;

		uint32_t m_stage; // only set while within an entry point

		ycVector< AST::NodeCBufferDef* > m_cbuffers;
		ycVector< AST::NodeVariableDecl* > m_samplers;
		AST::NodeStructDef* m_inputBufferStruct = nullptr;
		AST::NodeVariableDecl* m_inputBufferDecl = nullptr;

		void EmitTypeName( const ureg typeIndex );
		void ProcessArgList( AST::Node* node, const ureg flags = 0 );
	};
} // namespace ycShaderBuilder

#endif // !YC_SHADERGENGLSL_H
