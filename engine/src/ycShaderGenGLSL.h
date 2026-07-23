#pragma once
#ifndef YC_SHADERGENGLSL_H
#define YC_SHADERGENGLSL_H

#include "ycCommon.h"
#include "ycShaderGen.h"

class ycAllocator;
class ycByteBuffer;

namespace ycShaderBuilder
{
	class GenGLSL : public Gen
	{
	public:

		enum
		{
			kTarget_330,
			kTarget_Vk,
			#ifdef YC_NDA
			#endif 
			#ifdef YC_NDA
			#endif 
		};

		GenGLSL( ycAllocator* mem = nullptr, const uint32_t target = kTarget_330 );
		virtual ~GenGLSL();

	protected:

		uint32_t m_target;

		enum
		{
			kStage_Unknown,
			kStage_Vert,
			kStage_Frag
		};

		uint32_t m_stage; // only set while within an entry point

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

		void EmitTypeName( const ureg typeIndex );

		const char * GetBuiltinInput( AST::NodeVariableDecl * node );
		void GenerateEntryPoint( AST::NodeFunctionDecl* node );
	};
} // namespace ycShaderBuilder

#endif // !YC_SHADERGENGLSL_H
