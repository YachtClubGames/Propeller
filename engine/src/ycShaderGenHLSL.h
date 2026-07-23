#pragma once

#include "ycCommon.h"

#include "ycShaderGen.h"

class ycAllocator;
class ycByteBuffer;

namespace ycShaderBuilder
{
	class GenHLSL : public Gen
	{
	public:

		enum
		{
			kTarget_DX11,
			kTarget_DX12,
			kTarget_DX12_SM66
		};

		GenHLSL( ycAllocator* mem = nullptr, const uint32_t target = kTarget_DX11 );
		virtual ~GenHLSL();

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

		enum
		{
			kStage_Unknown,
			kStage_Vert,
			kStage_Frag
		};

		uint32_t m_stage; // only set while within an entry point

		void EmitTypeName( const ureg typeIndex );
		void ProcessArgList( AST::Node* node, const ureg flags = 0 );

		bool UseShaderModel66DynamicResources() const;
	};
} // namespace ycShaderBuilder
