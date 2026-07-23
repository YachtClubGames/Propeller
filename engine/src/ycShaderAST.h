#pragma once
#ifndef YC_SHADERAST_H
#define YC_SHADERAST_H

#include "ycCommon.h"
#include "ycShaderTokens.h"
//#include "ycVector.h"

namespace ycShaderBuilder
{
	namespace AST
	{
		enum
		{
			kNode_Invalid = 0,

			kNode_TypeAlias,
			kNode_StructDef,
			kNode_CBufferDef,

			kNode_Literal,
			kNode_Variable,
			kNode_FunctionCall,
			kNode_UnaryOp,
			kNode_InfixOp,
			kNode_CastOp,
			kNode_MemberAccessOp,
			kNode_ArrayAccessOp,

			kNode_VariableDecl,
			kNode_FunctionDecl,

			kNode_Block,
			kNode_IfBlock,
			kNode_ForBlock,
			kNode_Return,

			kNode_Asm,

			kNode_Count,
		};

		enum
		{
			kNodeFlag_ForwardDeclaration = 1<<0, // used on kNode_FunctionDecl
			kNodeFlag_GlobalDecl         = 1<<1, // used on kNode_VariableDecl to indicate a variable declared at global scope
		};

		struct Node
		{
			uint32_t nodeType;
			uint32_t flags;    //!< bits 0:7 kNodeFlag_* value 8:11 type dim x 12:15 value type dim y 16:31 value type index
			Node*    nextNode; //!< context dependent; for example at statement level this points to the next statement in program flow, or in an argument list this points to the next argument
			#ifdef YC_DEBUG
				virtual ~Node() {} // so the watch window can show us the full types
			#endif
		};
		
		inline void SetNodeValueType( Node* node, const uint32_t typeIndex ) { node->flags = (node->flags&0xffff) | ( typeIndex << 16 ); }
		inline uint32_t GetNodeValueType( Node* node ) { return (node->flags&0xffff0000) >> 16; }

		struct NodeLiteral : Node
		{
			const TokenDesc* valueToken;
		};

		struct NodeUnaryOp : Node
		{
			const TokenDesc* opToken;
			Node*            operand;
		};

		struct NodeInfixOp : Node
		{
			const TokenDesc* opToken;
			Node*            lhs;
			Node*            rhs;
		};

		struct NodeCastOp : Node
		{
			const TokenDesc* typeToken;
			Node*            firstArg;
		};

		struct NodeMemberAccess : Node
		{
			Node*            lhs;
			const TokenDesc* memberToken;
		};

		struct NodeArrayAccess : Node
		{
			Node* lhs;
			Node* arrayIdx;
		};

		struct NodeBlock : Node
		{
			AST::Node* firstStatement;
		};

		struct Attribute
		{
			const TokenDesc* nameToken;
			const TokenDesc* valueToken;
			Attribute*       nextAttr;
		};

		struct NodeIfBlock : Node
		{
			Node*           condition;
			AST::NodeBlock* body;
			Node*           otherwise; // can either be another NodeIf for else-if, or a plain NodeBlock for else
		};

		struct NodeForBlock : Node
		{
			Node* expr1;
			Node* expr2;
			Node* expr3;
			AST::NodeBlock* body;
		};

		struct NodeVariableDecl : Node
		{
			const TokenDesc* typeToken;
			const TokenDesc* nameToken;
			ureg             modifiers; // inout, const, etc
			ureg             arraySize; // 0 for non-array
			Node*            definition;
			Attribute*       firstAttr;
			enum
			{
				kModifier_InOut = 1<<0,
				kModifier_Out   = 1<<1,
				kModifier_Const = 1<<2
			};
		};

		struct NodeVariable : Node
		{
			NodeVariableDecl* var;
		};

		struct NodeFunctionDecl : Node
		{
			const TokenDesc*  typeToken;
			const TokenDesc*  nameToken;
			Node*             firstArg;
			NodeBlock*        definition;
			NodeFunctionDecl* nextOverload;
			ureg              numArgs;
			Attribute*        firstAttr;
		};

		struct NodeFunctionCall : Node
		{
			NodeFunctionDecl* func;
			Node*             firstArg;
		};

		struct NodeTypeAlias : Node
		{
			const TokenDesc* baseToken;
			const TokenDesc* aliasToken;
		};

		struct NodeStructDef : Node
		{
			const TokenDesc* nameToken;
			Node*            firstMember;
			Attribute*       firstAttr;
		};

		struct NodeCBufferDef : Node
		{
			Attribute* firstAttr;
			Node*      firstMember;
		};

		struct NodeReturn : Node
		{
			Node* val;
		};

		struct NodeAsm : Node
		{
			const TokenDesc* val;
		};

		void DumpNode( AST::Node* pNode, const ureg depth = 0 );

		Attribute* FindAttr( Attribute* firstAttr, const char* attrName );
		bool GetAttrBool( Attribute* firstAttr, const char* attrName ); //!< convenience function for 'bool' type arguments; eg return true if an attr exists and has no value, returns false if an attr exists with a value of zero, and so on

	} // namespace AST
} // namespace ycShaderBuilder

#endif // !YC_SHADERAST_H
