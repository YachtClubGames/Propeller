#include "ycShaderGenHLSL.h"

#include "ycByteBuffer.h"
#include "ycMath.h"
#include "ycShaderAST.h"
#include "ycShaderMetaData.h"
#include "ycShaderParser.h"
#include "ycStd.h"
#include "ycStringUtils.h"
#include "ycSysAllocator.h"

namespace ycShaderBuilder
{
	static u32 FindStructDefAutoTEXCOORDn( AST::NodeStructDef* structDef, AST::NodeVariableDecl* decl )
	{
		s32 texcoord = 0;
		for( AST::Node* node = structDef->firstMember; node != nullptr; node = node->nextNode )
		{
			if( node == decl ) { return texcoord; }

			if( node->nodeType == AST::kNode_VariableDecl )
			{
				AST::NodeVariableDecl* nodeDecl = (AST::NodeVariableDecl*)node;
				AST::Attribute* attr = AST::FindAttr( nodeDecl->firstAttr, "semantic" );
				if( attr->valueToken == nullptr ) { texcoord++; }
			}
		}
		ycAssert( false );
		return 0;
	}

	static ycStringRef GetTextureTypeName( AST::NodeVariableDecl* decl )//const u32 typeIndex )
	{
		switch( decl->typeToken->typeIndex )
		{
		case kIntrinsicType_Tex2D:     return ycStringRef( "Texture2D",          9 ); break;
		case kIntrinsicType_Tex3D:     return ycStringRef( "Texture3D",          9 ); break;
		case kIntrinsicType_TexCube:   return ycStringRef( "TextureCube",        11 ); break;
		case kIntrinsicType_Tex2DMS:   return ycStringRef( "Texture2DMS<float>", 18 ); break;
		case kIntrinsicType_Tex2DUint: return ycStringRef( "Texture2D<uint>",    15 ); break;
		YC_NO_DEFAULT_ASSERT_RETURN( ycStringRef() )
		}
	}

	static ycStringRef GetSamplerTypeName( AST::NodeVariableDecl* decl )
	{
		if( AST::GetAttrBool( decl->firstAttr, "shadow" ) )
		{
			return "SamplerComparisonState";
		}
		else
		{
			return "SamplerState";
		}
	}

	GenHLSL::GenHLSL( ycAllocator* mem /*= nullptr*/, const uint32_t target /*= kTarget_DX11*/ ) :
		Gen( mem ),
		m_target( target ),
		m_stage( kStage_Unknown )
	{
	}

	/*virtual*/ GenHLSL::~GenHLSL()
	{
	}

	/*virtual*/ void GenHLSL::Preamble()
	{
		Emit( "#pragma pack_matrix (column_major)\n" );
		if( m_target >= kTarget_DX12 )
		{
			Emit( "#pragma dxc diagnostic ignored \"-Wparentheses-equality\"\n" );
		}
		Emit( "\n" );
		if( UseShaderModel66DynamicResources() )
		{
			Emit( "cbuffer g_push : register( b0, space1 )\n" );
			Emit( "{\n" );
			Emit( "	uint texBase : packoffset( c0.x );\n" );
			Emit( "	uint samplerBase : packoffset( c0.y );\n" );
			Emit( "};\n" );
		}
	}

	/*virtual*/ void GenHLSL::Epilogue()
	{
		//m_dst->AppendRaw( "", 1 );
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeTypeAlias* node, const ureg flags )
	{
		//
		YC_UNUSED( node );
		YC_UNUSED( flags );
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeStructDef* node, const ureg flags )
	{
		m_structDef = node;
		YC_UNUSED( flags );
		Emit( "struct " );
		Emit( node->nameToken );
		Emit( "\n{\n" );
		PushIndent();
		ProcessNodeList( node->firstMember, kFlag_Statement );
		PopIndent();
		Emit( "};\n\n" );
		m_structDef = nullptr;
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeLiteral* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		switch( node->valueToken->literalType )
		{
		case kIntrinsicType_Float:
			Emit( node->valueToken );
			break;
		case kIntrinsicType_Int:
		case kIntrinsicType_UInt:
			Emit( node->valueToken );
			break;
		YC_NO_DEFAULT_ASSERT
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeVariable* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }

		AST::NodeVariableDecl* var = node->var;
		Emit( var->nameToken );

		// TODO(jstpierre): Also allow passing sampler?
		if( IsTextureType( var->typeToken->typeIndex ) && !!( var->flags & AST::kNodeFlag_GlobalDecl ) )
		{
			Emit( "_view" );
		}

		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeFunctionCall* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }

		const struct TextureFunc
		{
			const char* funcName;
			const char* methodName;
			enum { kNone = 0, kWantsSampler = 1 } flags;
		} textureFuncs[] = {
			{ "texture2D",           	   "Sample",        TextureFunc::kWantsSampler, },
			{ "texture2DLOD",              "SampleLevel",   TextureFunc::kWantsSampler, },
			{ "texture3D",           	   "Sample",        TextureFunc::kWantsSampler, },
			{ "textureCube",         	   "Sample",        TextureFunc::kWantsSampler, },
			{ "texture2DOffs",       	   "Sample",        TextureFunc::kWantsSampler, },
			{ "textureCubeLOD",      	   "SampleLevel",   TextureFunc::kWantsSampler, },
			{ "texture2DCmp",        	   "SampleCmp",     TextureFunc::kWantsSampler, },
			{ "_texture2DLoad",       	   "Load",          TextureFunc::kNone, },
			{ "texture2DMSLoad",     	   "Load",          TextureFunc::kNone, },
			{ "texture2DMSLoadOffs", 	   "Load",          TextureFunc::kNone, },
			{ "_texture2DLoadOffs",	       "Load",          TextureFunc::kNone, },
			{ "texture2DUintLoad", 	       "Load",          TextureFunc::kNone, },
			{ "_texture2DGetDimensions",   "GetDimensions", TextureFunc::kNone, },
			{ "_texture2DMSGetDimensions", "GetDimensions", TextureFunc::kNone, },
		};

		const ycStringRef& funcName = node->func->nameToken->str;

		bool needsEmit = true;

		for( const auto& func : textureFuncs )
		{
			if( funcName.Equals( func.funcName, ycStringUtils::Length( func.funcName ) ) )
			{
				// first arg must be texture variable
				ycAssert( node->firstArg->nodeType == AST::kNode_Variable );
				AST::NodeVariable* texVar = (AST::NodeVariable*)node->firstArg;

				Emit( texVar->var->nameToken );
				if( !!( texVar->var->flags & AST::kNodeFlag_GlobalDecl ) )
				{
					Emit( "_view" );
				}
				Emit( "." );
				Emit( func.methodName );
				Emit( "( " );
				if( !!( func.flags & TextureFunc::kWantsSampler ) )
				{
					Emit( texVar->var->nameToken );
					Emit( "_sampler, " );
				}
				ProcessArgList( texVar->nextNode );
				Emit( " )" );

				needsEmit = false;
				break;
			}
		}

		if( !needsEmit )
		{
			// Nothing to do. Just here to make the else flow nicer to read below.
		}
		else if( funcName.Equals( "discard", 7 ) )
		{
			Emit( "discard" );
		}
		else
		{
			Emit( node->func->nameToken );
			Emit( "( " );
				ProcessArgList( node->firstArg );
			Emit( " )" );
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeUnaryOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		else { Emit( "(" ); }
		Emit( node->opToken );
		ProcessNode( node->operand );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
		else { Emit( ")" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeInfixOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		else { Emit( "(" ); }
		if( node->opToken->type == ycShaderBuilder::kToken_Op_Star )
		{
			const uint32_t lhsType = GetNodeValueType( node->lhs );
			const uint32_t rhsType = GetNodeValueType( node->rhs );
			if(
				( IsMatrixType( lhsType) && IsAggregateType( rhsType ) )
				||
				( IsAggregateType( lhsType ) && IsMatrixType( lhsType ) )
				||
				( IsMatrixType( lhsType) && IsMatrixType( rhsType ) )
			)
			{
				Emit( "mul(" );
				ProcessNode( node->lhs );
				Emit( "," );
				ProcessNode( node->rhs );
				Emit( ")" );
			}
			else
			{
				ProcessNode( node->lhs );
				Emit( " " );
				Emit( node->opToken );
				Emit( " " );
				ProcessNode( node->rhs );
			}
		}
		else
		{
			ProcessNode( node->lhs );
			Emit( " " );
			Emit( node->opToken );
			Emit( " " );
			ProcessNode( node->rhs );
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
		else { Emit( ")" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeCastOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		Emit( node->typeToken );
		Emit( "(" );
		for( AST::Node* arg = node->firstArg; arg != nullptr; arg = arg->nextNode )
		{
			ProcessNode( arg, 0 );
			if( arg->nextNode ) { Emit( ", " ); }
		}
		Emit( ")" );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeArrayAccess* node, const ureg flags )
	{
		if( flags & kFlag_Statement ) { EmitIndent(); }
		ProcessNode( node->lhs, flags );
		Emit( "[" );
		ProcessNode( node->arrayIdx, flags );
		Emit( "]" );
		if( flags & kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeMemberAccess* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		ProcessNode( node->lhs );
		Emit( "." );
		Emit( node->memberToken );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeVariableDecl* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		if( IsTextureType( node->typeToken->typeIndex ) )
		{
			ycStringRef typeName = GetTextureTypeName( node );
			if( node->flags&AST::kNodeFlag_GlobalDecl )
			{
				AST::Attribute* idxAttr = AST::FindAttr( node->firstAttr, "index" );
				ycAssert( idxAttr );
				bool converted;
				const uint32_t samplerIndex = uint32_t( idxAttr->valueToken->str.ToUInt( &converted ) );
				ycAssert( converted );
				// TODO HACK FIXME: sanity check index attr value?

				if( UseShaderModel66DynamicResources() )
				{
					Emit( "static " );
				}
				Emit( GetSamplerTypeName( node ) );
				Emit( " " );
				Emit( node->nameToken );
				Emit( "_sampler" );
				if( UseShaderModel66DynamicResources() )
				{
					Emit( " = SamplerDescriptorHeap[ samplerBase + " );
					Emit( idxAttr->valueToken );
					Emit( " ];\n" );
				}
				else
				{
					Emit( " : register( s" );
					Emit( idxAttr->valueToken );
					Emit( " );\n" );
				}
				if( UseShaderModel66DynamicResources() )
				{
					Emit( "static " );
				}
				Emit( typeName );
				Emit( " ", 1 );
				Emit( node->nameToken );
				Emit( "_view" );
				if( UseShaderModel66DynamicResources() )
				{
					Emit( " = ResourceDescriptorHeap[ texBase + " );
					Emit( idxAttr->valueToken );
					Emit( " ];\n" );
				}
				else
				{
					Emit( " : register( t" );
					Emit( idxAttr->valueToken );
					Emit( " )" );
				}
				ycAssert( node->definition == nullptr );

				AddMetaSampler( samplerIndex, 0xffffffffu, 0xffffffffu, node->nameToken->str ); // TODO HACK FIXME: name is a lie! need separate view/samplers!!!

				ycAssert( node->arraySize == 0 ); // NOT SUPPORTED ATM!!
			}
			else
			{
				Emit( typeName );
				Emit( " " );
				Emit( node->nameToken );
			}
		}
		else
		{
			const ureg modifiers = node->modifiers;
			if(      modifiers&AST::NodeVariableDecl::kModifier_InOut ) { Emit( "inout ", 6 ); }
			else if( modifiers&AST::NodeVariableDecl::kModifier_Out   ) { Emit( "out ",   4 ); }
			else if( modifiers&AST::NodeVariableDecl::kModifier_Const ) { Emit( "const ", 6 ); }
			EmitTypeName( node->typeToken->typeIndex );
			Emit( " " );
			Emit( node->nameToken );
			if( node->arraySize )
			{
				Emit( "[" );
				Emit( node->arraySize );
				Emit( "]" );
			}
		}
		if( node->definition )
		{
			Emit( " = " );
			ProcessNode( node->definition );
		}
		else
		{
			AST::Attribute* attr = AST::FindAttr( node->firstAttr, "semantic" );
			if( attr )
			{
				Emit( " : " );
				if( attr->valueToken == nullptr )
				{
					// Emit an auto TEXCOORDn.
					ycAssert( m_structDef != nullptr );
					u32 texcoord = FindStructDefAutoTEXCOORDn( m_structDef, node );
					char semanticName[ 16 ] = {};
					ycStringUtils::SprintF( semanticName, sizeof( semanticName ), "TEXCOORD%d", texcoord );
					Emit( semanticName );
				}
				else if( attr->valueToken->str.Equals( "POSITION", 8 ) )
				{
					Emit( "SV_Position" );
				}
				else if( attr->valueToken->str.BeginsWith( "COLOR" ) )
				{
					Emit( "SV_Target" );
					Emit( attr->valueToken->str.Skip( 5 ) );
				}
				else if( attr->valueToken->str.Equals( "VERTEX_ID", 9 ) )
				{
					Emit( "SV_VertexID" );
				}
				else if( attr->valueToken->str.Equals( "INSTANCE_ID", 11 ) )
				{
					Emit( "SV_InstanceID" );
				}
				else if( attr->valueToken->str.Equals( "DEPTH", 5 ) )
				{
					Emit( "SV_Depth" );
				}
				else if( attr->valueToken->str.Equals( "DEPTH_LE", 8 ) )
				{
					Emit( "SV_DepthLessEqual" );
				}
				else if( attr->valueToken->str.Equals( "DEPTH_GE", 8 ) )
				{
					Emit( "SV_DepthGreaterEqual" );
				}
				else
				{
					Emit( attr->valueToken );
				}
			}
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeFunctionDecl* node, const ureg flags )
	{
		YC_UNUSED( flags );
		if( AST::FindAttr( node->firstAttr, "intrinsic" ) ) { return; }

		bool isEntryPoint = false;
		AST::Attribute* stage = AST::FindAttr( node->firstAttr, "stage" );
		if( stage != nullptr )
		{
			isEntryPoint = true;
			if( stage->valueToken->str.Equals( "vertex", 6 ) )
			{
				m_stage = kStage_Vert;
			}
			else if( stage->valueToken->str.Equals( "fragment", 8 ) )
			{
				m_stage = kStage_Frag;
			}
			else { ycFatal(); }
		}

		EmitTypeName( node->typeToken->typeIndex );
		Emit( " " );
		if( isEntryPoint ) { Emit( "main" ); }
		else { Emit( node->nameToken ); }
		if( node->firstArg )
		{
			Emit( "( " );
			for( AST::Node* arg = node->firstArg; arg != nullptr; arg = arg->nextNode )
			{
				ProcessNode( arg );
				if( arg->nextNode )
				{
					Emit( ", " );
				}
			}
			Emit( " )" );
		}
		else
		{
			Emit( "()" );
		}
		if( node->flags&AST::kNodeFlag_ForwardDeclaration ) { Emit( ";\n\n" ); }
		else
		{
			Emit( "\n" );
			Emit( "{\n" );
			PushIndent();
			ProcessNodeList( node->definition->firstStatement, kFlag_Statement );
			PopIndent();
			Emit( "}\n\n" );
		}
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeBlock* node, const ureg flags )
	{
		YC_UNUSED( flags );
		EmitIndent();
		Emit( "{\n" );
		PushIndent();
		ProcessNodeList( node->firstStatement, kFlag_Statement );
		PopIndent();
		EmitIndent();
		Emit( "}\n" );
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeIfBlock* node, const ureg flags )
	{
		YC_UNUSED( flags );
		EmitIndent();
		Emit( "if( " );
		ProcessNode( node->condition );
		Emit( " )\n" );
		Process( node->body );

		AST::Node* otherwise = node->otherwise;
		if( otherwise )
		{
			while( otherwise->nodeType == AST::kNode_IfBlock ) // else if
			{
				AST::NodeIfBlock* elseif = (AST::NodeIfBlock*)otherwise;
				EmitIndent();
				Emit( "else if( " );
				ProcessNode( elseif->condition );
				Emit( " )\n" );
				Process( elseif->body );
				otherwise = elseif->otherwise;
				if( otherwise == nullptr ) { break; }
			}
			if( otherwise )
			{
				ycAssert( otherwise->nodeType == AST::kNode_Block );
				AST::NodeBlock* body = (AST::NodeBlock*)otherwise;
				EmitIndent();
				Emit( "else\n" );
				Process( body );
			}
		}
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeCBufferDef* node, const ureg flags )
	{
		YC_UNUSED( flags );
		AST::Attribute* idxAttr = AST::FindAttr( node->firstAttr, "index" );
		ycAssert( idxAttr );
		// TODO HACK FIXME: sanity check index attr value?

		Emit( "cbuffer g_cbuffer_" );
		Emit( idxAttr->valueToken );
		Emit( " : register( b" );
		Emit( idxAttr->valueToken );
		Emit( " )\n" );
		Emit( "\n{\n" );
		PushIndent();
		ProcessNodeList( node->firstMember, kFlag_Statement );
		PopIndent();
		Emit( "};\n\n" );

		ycShaderMetaData::UniformBufferMapping* meta = m_dstMetaData->AppendUninitialized< ycShaderMetaData::UniformBufferMapping >();
		new( meta )ycShaderMetaData::UniformBufferMapping;
		bool converted;
		meta->index             = uint32_t( idxAttr->valueToken->str.ToUInt( &converted ) );
		ycAssert( converted );
		meta->descriptorSet     = 0xffffffffu;
		meta->descriptorBinding = 0xffffffffu;
		const u32 requiredLen = 10 + u32(idxAttr->valueToken->str.Length()) + 1; // g_cbuffer_ + idx + NUL
		const u32 nameSize = ycRoundUpPow2( requiredLen, u32(ycShaderMetaData::kPadNamesToAlign) );
		meta->nameSize = nameSize;
		m_dstMetaData->AppendRaw( "g_cbuffer_", 10 );
		m_dstMetaData->AppendRaw( idxAttr->valueToken->str.GetPtr(), idxAttr->valueToken->str.Length() );
		m_dstMetaData->AppendRaw( "", 1 );
		m_dstMetaData->AppendZero( nameSize - requiredLen );
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeForBlock* node, const ureg flags )
	{
		YC_UNUSED( flags );
		EmitIndent();
		Emit( "for( " );
		ProcessNode( node->expr1 );
		Emit( "; " );
		ProcessNode( node->expr2 );
		Emit( "; " );
		ProcessNode( node->expr3 );
		Emit( " )\n" );
		Process( node->body );
	}

	/*virtual*/ void GenHLSL::Process( AST::NodeReturn* node, const ureg /*flags*/ )
	{
		EmitIndent(); Emit( "return " );
		ProcessNode( node->val, kFlag_Statement );
	}
	
	void GenHLSL::EmitTypeName( const ureg typeIndex )
	{
		if( typeIndex < kIntrinsicType_Count )
		{
			const char* typeNames[] =
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
				"float4x4"

				// textures are handled separately, because they must generate separate sampler/view variables
			};
			ycAssert( typeIndex < YC_ARRAY_SIZE(typeNames) );
			Emit( typeNames[ typeIndex ] );
		}
		else
		{
			AST::Node* typeNode = m_parser->GetTypeDesc( typeIndex );
			ycAssert( typeNode );
			if( typeNode->nodeType == AST::kNode_StructDef )
			{
				AST::NodeStructDef* structDef = (AST::NodeStructDef*)typeNode;
				Emit( structDef->nameToken );
			}
			else
			{
				ycAssert( typeNode->nodeType == AST::kNode_TypeAlias );
				AST::NodeTypeAlias* aliasDef = (AST::NodeTypeAlias*)typeNode;
				Emit( aliasDef->aliasToken );
			}
		}
	}

	void GenHLSL::ProcessArgList( AST::Node* node, const ureg flags )
	{
		for( ; node != nullptr; node = node->nextNode )
		{
			ProcessNode( node, flags );
			if( node->nextNode ) { Emit( ", " ); }
		}
	}

	bool GenHLSL::UseShaderModel66DynamicResources() const
	{
		return m_target == kTarget_DX12_SM66;
	}

} // namespace ycShaderBuilder
