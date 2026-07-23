#include "ycShaderAST.h"

#include "ycStd.h"
#include "ycStringUtils.h"

namespace
{
	const char s_indent[] = "                                ";
	void PrintIndent( const ureg depth )
	{
		ycAssert( depth*2 <= YC_ARRAY_SIZE(s_indent) );
		ycLog( "%.*s", depth*2, s_indent);
	}
}

namespace ycShaderBuilder
{
	namespace AST
	{
		void DumpNode( AST::Node* pNode, const ureg depth/* = 0*/ )
		{
			switch( pNode->nodeType )
			{
			case AST::kNode_Literal:
				{
					AST::NodeLiteral* node = (AST::NodeLiteral*)pNode;
					PrintIndent( depth ); ycLog( "Literal [%.*s]\n", int(node->valueToken->str.Length()), node->valueToken->str.GetPtr() );
				}
				break;
			case AST::kNode_FunctionCall:
				{
					AST::NodeFunctionCall* node = (AST::NodeFunctionCall*)pNode;
					PrintIndent( depth ); ycLog( "Function Call [%.*s]\n", int(node->func->nameToken->str.Length()), node->func->nameToken->str.GetPtr() );
					for( AST::Node* argNode = node->firstArg; argNode != nullptr; argNode = argNode->nextNode )
					{
						DumpNode( argNode, depth+1 );
					}
				}
				break;
			case AST::kNode_Variable:
				{
					AST::NodeVariable* node = (AST::NodeVariable*)pNode;
					PrintIndent( depth ); ycLog( "Variable [%.*s]\n", int(node->var->nameToken->str.Length()), node->var->nameToken->str.GetPtr() );
				}
				break;
			case AST::kNode_MemberAccessOp:
				{
					AST::NodeMemberAccess* node = (AST::NodeMemberAccess*)pNode;
					PrintIndent( depth ); ycLog( "Member Access [%.*s]\n", int(node->memberToken->str.Length()), node->memberToken->str.GetPtr() );
					DumpNode( node->lhs, depth+1 );
				}
				break;
			case AST::kNode_UnaryOp:
				{
					AST::NodeUnaryOp* node = (AST::NodeUnaryOp*)pNode;
					PrintIndent( depth ); ycLog( "Unary Op [%.*s]\n", int(node->opToken->str.Length()), node->opToken->str.GetPtr() );
					DumpNode( node->operand, depth+1 );
				}
				break;
			case AST::kNode_InfixOp:
				{
					AST::NodeInfixOp* node = (AST::NodeInfixOp*)pNode;
					PrintIndent( depth ); ycLog( "Infix Op [%.*s]\n", int(node->opToken->str.Length()), node->opToken->str.GetPtr() );
					DumpNode( node->lhs, depth+1 );
					DumpNode( node->rhs, depth+1 );
				}
				break;
			case AST::kNode_CastOp:
				{
					AST::NodeCastOp* node = (AST::NodeCastOp*)pNode;
					PrintIndent( depth ); ycLog( "Function Call [%.*s]\n", int(node->typeToken->str.Length()), node->typeToken->str.GetPtr() );
					for( AST::Node* argNode = node->firstArg; argNode != nullptr; argNode = argNode->nextNode )
					{
						DumpNode( argNode, depth+1 );
					}
				}
				break;
			case AST::kNode_VariableDecl:
				{
					AST::NodeVariableDecl* node = (AST::NodeVariableDecl*)pNode;
					PrintIndent( depth ); ycLog(
						"Variable Decl%s [%.*s] [%.*s] array size[%u]\n",
						node->definition ? " + Assignment" : "",
						int(node->typeToken->str.Length()), node->typeToken->str.GetPtr(),
						int(node->nameToken->str.Length()), node->nameToken->str.GetPtr(),
						u32( node->arraySize )
					);
					if( node->definition )
					{
						DumpNode( node->definition, depth + 1 );
					}
				}
				break;
			case AST::kNode_FunctionDecl:
				{
					AST::NodeFunctionDecl* node = (AST::NodeFunctionDecl*)pNode;
					PrintIndent( depth ); ycLog(
						"Function Decl%s [%.*s] [%.*s] ",
						(node->flags&AST::kNodeFlag_ForwardDeclaration)==0 ? " + Definition" : "",
						int(node->typeToken->str.Length()), node->typeToken->str.GetPtr(),
						int(node->nameToken->str.Length()), node->nameToken->str.GetPtr()
					);
					if( node->firstArg )
					{
						ycLog( "( " );
						ureg numArgs = 0;
						for( AST::Node* arg = node->firstArg; arg != nullptr; arg = arg->nextNode )
						{
							ycAssert( arg->nodeType == AST::kNode_VariableDecl );
							AST::NodeVariableDecl* argDecl = (AST::NodeVariableDecl*)arg;
							ycLog(
								"%.*s %.*s",
								int(argDecl->typeToken->str.Length()), argDecl->typeToken->str.GetPtr(),
								int(argDecl->nameToken->str.Length()), argDecl->nameToken->str.GetPtr()
							);
							if( arg->nextNode ) { ycLog( ", " ); }
							++numArgs;
						}
						ycAssert( numArgs == node->numArgs );
						ycLog( " )" );
					}
					else
					{
						ycLog( "(no arguments)" );
					}
					ycLog( "\n" );
					if( (node->flags&AST::kNodeFlag_ForwardDeclaration)==0 )
					{
						ycAssert( node->definition );
						DumpNode( node->definition, depth + 1 );
					}
				}
				break;
			case AST::kNode_Block:
				{
					AST::NodeBlock* node = (AST::NodeBlock*)pNode;
					for( AST::Node* childNode = node->firstStatement; childNode != nullptr; childNode = childNode->nextNode )
					{
						DumpNode( childNode, depth );
					}
				}
				break;
			case kNode_TypeAlias:
				{
					AST::NodeTypeAlias* node = (AST::NodeTypeAlias*)pNode;
					PrintIndent( depth ); ycLog( "Type Alias [%.*s] == [%.*s]\n",
						int(node->aliasToken->str.Length()), node->aliasToken->str.GetPtr(),
						int(node->baseToken->str.Length()), node->baseToken->str.GetPtr()
					);
				}
				break;
			case kNode_StructDef:
				{
					AST::NodeStructDef* node = (AST::NodeStructDef*)pNode;
					PrintIndent( depth ); ycLog( "Struct Definition [%.*s]\n", int(node->nameToken->str.Length()), node->nameToken->str.GetPtr() );
					for( AST::Node* arg = node->firstMember; arg != nullptr; arg = arg->nextNode )
					{
						ycAssert( arg->nodeType == AST::kNode_VariableDecl );
						DumpNode( arg, depth+1 );
					}
				}
				break;
			//case kNode_CBufferDef:
			//	{
			//		AST::NodeStructDef* node = (AST::NodeStructDef*)pNode;
			//		PrintIndent( depth ); ycLog( "Struct Definition [%.*s]\n", node->nameToken->str.Length(), node->nameToken->str.Get() );
			//		for( AST::Node* arg = node->firstMember; arg != nullptr; arg = arg->nextNode )
			//		{
			//			ycAssert( arg->nodeType == AST::kNode_VariableDecl );
			//			DumpNode( arg, depth+1 );
			//		}
			//	}
			//	break;
			YC_NO_DEFAULT_ASSERT
			}
		}

		Attribute* FindAttr( Attribute* attr, const char* attrName )
		{
			const ycSize_t len = ycStringUtils::Length( attrName );
			while( attr )
			{
				if( attr->nameToken->str.Length() == uint32_t(len) && ycStringUtils::BeginsWith( attr->nameToken->str.GetPtr(), attrName, len ) )
				{
					return attr;
				}
				attr = attr->nextAttr;
			}
			return nullptr;
		}

		bool GetAttrBool( Attribute* firstAttr, const char* attrName )
		{
			Attribute* attr = FindAttr( firstAttr, attrName );
			if( attr )
			{
				if( attr->valueToken )
				{
					if( attr->valueToken->str.Equals( "1", 1 ) || attr->valueToken->str.Equals( "true", 4 ) )
					{
						return true;
					}
					return false;
				}
				else
				{
					// attr axists but has no value
					return true;
				}
			}
			else
			{
				// attr doesnt exist
				return false;
			}
		}

	} // namespace AST
} // namespace ycShaderBuilder
