#include "ycShaderGenGLSL.h"

// core
#include "ycByteBuffer.h"
#include "ycMath.h"
#include "ycStd.h"
#include "ycStringUtils.h"
//#include "ycSysAllocator.h"

// tools
#include "ycShaderAST.h"
#include "ycShaderMetaData.h"
#include "ycShaderParser.h"

// std
#include <stdio.h> // sprintf ugggghh

namespace
{
	#if YC_NDA
	#endif 
}

namespace ycShaderBuilder
{
	GenGLSL::GenGLSL( ycAllocator* mem, const uint32_t target /*= kTarget_330*/ ) :
		Gen( mem ),
		m_target( target ),
		m_stage( kStage_Unknown )
	{
	}

	/*virtual*/ GenGLSL::~GenGLSL()
	{
	}
	
	/*virtual*/ void GenGLSL::Preamble()
	{
		switch( m_target )
		{
			// precision declarations shouldn't actually do anything outside of GLES, we should probably just not have these
			case kTarget_330: Emit( "#version 330\n\n" "precision highp float;\n\n" "precision highp int;\n\n" ); break;
			case kTarget_Vk:  Emit( "#version 450\n\n" "precision highp float;\n\n" "precision highp int;\n\n" ); break;
			#ifdef YC_NDA
			#endif 
			#ifdef YC_NDA
			#endif 
			YC_NO_DEFAULT_ASSERT
		}
	}

	/*virtual*/ void GenGLSL::Epilogue()
	{
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeTypeAlias* node, const ureg flags )
	{
		//
		YC_UNUSED( node );
		YC_UNUSED( flags );
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeStructDef* node, const ureg flags )
	{
		YC_UNUSED( flags );
		Emit( "struct " );
		Emit( node->nameToken );
		Emit( "\n{\n" );
		PushIndent();
		ProcessNodeList( node->firstMember, kFlag_Statement );
		PopIndent();
		Emit( "};\n\n" );
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeLiteral* node, const ureg flags )
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

	/*virtual*/ void GenGLSL::Process( AST::NodeVariable* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		Emit( node->var->nameToken );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeFunctionCall* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		bool emitArgs = true;
		if( node->func->nameToken->str.Equals( "texture2D", 9 ) || node->func->nameToken->str.Equals( "textureCube", 11 ) )
		{
			Emit( "texture" );
		}
		else if( node->func->nameToken->str.Equals( "textureCubeLOD", 14 ) )
		{
			Emit( "textureLod" );
		}
		else if( node->func->nameToken->str.Equals( "texture2DLOD", 12 ) )
		{
			Emit( "textureLod" );
		}
		else if( node->func->nameToken->str.Equals( "texture2DOffs", 13 ) )
		{
			Emit( "textureOffset" );
		}
		else if( node->func->nameToken->str.Equals( "texture2DCmp", 12 ) )
		{
			// exactly thee args
			ycAssert( node->firstArg );
			ycAssert( node->firstArg->nextNode );
			ycAssert( node->firstArg->nextNode->nextNode );
			ycAssert( node->firstArg->nextNode->nextNode->nextNode == nullptr );

			// first arg must be texture variable
			ycAssert( node->firstArg->nodeType == AST::kNode_Variable );
			//AST::NodeVariable* texVar = (AST::NodeVariable*)node->firstArg;

			Emit( "texture( " );
			ProcessNode( node->firstArg );
			Emit( ", vec3( " );
				ProcessNode( node->firstArg->nextNode );
				Emit( ", " );
				ProcessNode( node->firstArg->nextNode->nextNode );
			Emit( " ) )" );
			emitArgs = false;
		}
		else if( node->func->nameToken->str.Equals( "texture2DMSLoad", 15 ) )
		{
			Emit( "texelFetch" );
		}
		else if( node->func->nameToken->str.Equals( "texture2DLoad", 13 ) )
		{
			Emit( "texelFetch" );
		}
		else if( node->func->nameToken->str.Equals( "_texture2DUintLoad", 18 ) )
		{
			Emit( "texelFetch" );
		}
		else if( node->func->nameToken->str.Equals( "texture2DMSLoadOffs", 19 ) )
		{
			Emit( "texelFetchOffset" );
		}
		else if( node->func->nameToken->str.Equals( "texture2DLoadOffs", 17 ) )
		{
			Emit( "texelFetchOffset" );
		}
		else if( node->func->nameToken->str.Equals( "discard", 7 ) )
		{
			Emit( "discard" );
			emitArgs = false;
		}
		else
		{
			Emit( node->func->nameToken );
		}
		if( emitArgs )
		{
			Emit( "(" );
			for( AST::Node* arg = node->firstArg; arg != nullptr; arg = arg->nextNode )
			{
				ProcessNode( arg, 0 );
				if( arg->nextNode ) { Emit( ", " ); }
			}
			Emit( ")" );
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeUnaryOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		else { Emit( "(" ); }
		Emit( node->opToken );
		ProcessNode( node->operand );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
		else { Emit( ")" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeInfixOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		else { Emit( "(" ); }
		ProcessNode( node->lhs );
		Emit( " " );
		Emit( node->opToken );
		Emit( " " );
		ProcessNode( node->rhs );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
		else { Emit( ")" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeCastOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }

		u32 transposedType = 0;
		if( IsMatrixType( node->typeToken->typeIndex ) )
		{
			transposedType = TransposeMatrixType( node->typeToken->typeIndex );
		}

		if( transposedType != 0 )
		{
			Emit( "transpose(" );
			EmitTypeName( transposedType );
		}
		else
		{
			EmitTypeName( node->typeToken->typeIndex );
		}

		Emit( "(" );
		for( AST::Node* arg = node->firstArg; arg != nullptr; arg = arg->nextNode )
		{
			ProcessNode( arg, 0 );
			if( arg->nextNode ) { Emit( ", " ); }
		}
		Emit( ")" );
		if( transposedType != 0 ) { Emit( ")" ); }

		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeArrayAccess* node, const ureg flags )
	{
		if( flags & kFlag_Statement ) { EmitIndent(); }
		ProcessNode( node->lhs, flags );
		Emit( "[" );
		ProcessNode( node->arrayIdx, flags );
		Emit( "]" );
		if( flags & kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeMemberAccess* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		ProcessNode( node->lhs );
		Emit( "." );
		Emit( node->memberToken );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeVariableDecl* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		if( node->typeToken->typeIndex == kIntrinsicType_Tex2D || node->typeToken->typeIndex == kIntrinsicType_Tex3D || node->typeToken->typeIndex == kIntrinsicType_Tex2DUint || node->typeToken->typeIndex == kIntrinsicType_TexCube )
		{
			ycStringRef typeName;
			switch( node->typeToken->typeIndex )
			{
			case kIntrinsicType_Tex2D:
				if( AST::GetAttrBool( node->firstAttr, "shadow" ) )
				{
					typeName.Set( "sampler2DShadow", 15 );
				}
				else
				{
					typeName.Set( "sampler2D", 9 );
				}
				break;
			case kIntrinsicType_Tex3D:   typeName.Set( "sampler3D",   9 ); break;
			case kIntrinsicType_TexCube: typeName.Set( "samplerCube", 11 ); break;
			case kIntrinsicType_Tex2DUint: typeName.Set( "usampler2D", 10 ); break;
			case kIntrinsicType_Tex2DMS: typeName.Set( "sampler2DMS", 11 ); break;
			YC_NO_DEFAULT_ASSERT
			}
			if( node->flags&AST::kNodeFlag_GlobalDecl )
			{
				AST::Attribute* idxAttr = AST::FindAttr( node->firstAttr, "index" );
				ycAssert( idxAttr );
				bool converted;
				uint32_t samplerIndex = uint32_t( idxAttr->valueToken->str.ToUInt( &converted ) );
				ycAssert( converted );
				// TODO HACK FIXME: sanity check index attr value?

				uint32_t samplerDescriptorSet     = 0xffffffffu;
				uint32_t samplerDescriptorBinding = 0xffffffffu;
				if( m_target == kTarget_Vk )
				{
					AST::Attribute* set     = AST::FindAttr( node->firstAttr, "descriptor_set" );
					AST::Attribute* binding = AST::FindAttr( node->firstAttr, "descriptor_binding" );
					ycAssert( set && binding );
					Emit( "layout(set=" );
					Emit( set->valueToken );
					Emit( ",binding=" );
					Emit( binding->valueToken );
					Emit( ") " );
					samplerDescriptorSet     = uint32_t( set->valueToken->str.ToUInt( &converted ) );
					ycAssert( converted );
					samplerDescriptorBinding = uint32_t( binding->valueToken->str.ToUInt( &converted ) );
					ycAssert( converted );
				}
				#if YC_NDA
				#endif
				AddMetaSampler( samplerIndex, samplerDescriptorSet, samplerDescriptorBinding, node->nameToken->str );
				Emit( "uniform " );
			}
			Emit( typeName );
		}
		else
		{
			const ureg modifiers = node->modifiers;
			if(      modifiers&AST::NodeVariableDecl::kModifier_InOut ) { Emit( "inout ", 6 ); }
			else if( modifiers&AST::NodeVariableDecl::kModifier_Out   ) { Emit( "out ",   4 ); }
			else if( modifiers&AST::NodeVariableDecl::kModifier_Const ) { Emit( "const ", 6 ); }
			EmitTypeName( node->typeToken->typeIndex );
		}
		//Emit( node->typeToken );
		Emit( " " );
		Emit( node->nameToken );
		if( node->arraySize )
		{
			Emit( "[" );
			Emit( node->arraySize );
			Emit( "]" );
		}
		if( node->definition )
		{
			Emit( " = " );
			ProcessNode( node->definition );
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeFunctionDecl* node, const ureg flags )
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
		Emit( node->nameToken );
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

			if( isEntryPoint ) { GenerateEntryPoint( node ); }
		}
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeBlock* node, const ureg flags )
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

	const char * GenGLSL::GetBuiltinInput( AST::NodeVariableDecl * node )
	{
		AST::Attribute * semantic = AST::FindAttr( node->firstAttr, "semantic" );
		ycAssert( semantic );

		if( m_stage == kStage_Frag )
		{
			if( semantic->valueToken != nullptr && semantic->valueToken->str.Equals( "POSITION", 8 ) )
			{
				return "gl_FragCoord";
			}
		}
		else if( m_stage == kStage_Vert )
		{
			if( semantic->valueToken != nullptr && semantic->valueToken->str.Equals( "VERTEX_ID", 9 ) )
			{
				if( m_target == kTarget_Vk )
				{
					return "gl_VertexIndex";
				}
				else
				{
					return "gl_VertexID";
				}
			}
			else if( semantic->valueToken != nullptr && semantic->valueToken->str.Equals( "INSTANCE_ID", 11 ) )
			{
				if( m_target == kTarget_Vk )
				{
					return "gl_InstanceIndex";
				}
				else
				{
					return "gl_InstanceID";
				}
			}
		}

		return nullptr;
	}

	void GenGLSL::GenerateEntryPoint( AST::NodeFunctionDecl* node )
	{
		ycAssert( m_stage != kStage_Unknown );

		ycAssert( node->numArgs == 0 || ( node->numArgs == 1 && node->firstArg->nodeType == AST::kNode_VariableDecl ) );
		AST::NodeVariableDecl* inToken = node->numArgs == 1 ? (AST::NodeVariableDecl*)node->firstArg : nullptr ;

		if( inToken && inToken->typeToken->typeIndex != kIntrinsicType_Void )
		{
			AST::NodeStructDef* inDef = (AST::NodeStructDef*)m_parser->GetTypeDesc( inToken->typeToken->typeIndex );
			ycAssert( inDef->nodeType == AST::kNode_StructDef );
			ureg inputIdx = 0;
			for( AST::NodeVariableDecl* varDef = (AST::NodeVariableDecl*)inDef->firstMember; varDef != nullptr; varDef = (AST::NodeVariableDecl*)varDef->nextNode )
			{
				ycAssert( varDef->nodeType == AST::kNode_VariableDecl );

				const char * builtin = GetBuiltinInput( varDef );
				if( builtin == nullptr )
				{
					bool isAttr = ( m_stage == kStage_Vert );
					if( m_target == kTarget_Vk || isAttr )
					{
						Emit( "layout(location=" );
						Emit( inputIdx );
						Emit( ") " );
						Emit( "in highp " );
						EmitTypeName( varDef->typeToken->typeIndex );
						Emit( " IN_" );
						Emit( inputIdx );
						Emit( "_" );
						if( isAttr )
						{
							ycShaderMetaData::AttributeMapping* meta = m_dstMetaData->AppendUninitialized< ycShaderMetaData::AttributeMapping >();
							new( meta )ycShaderMetaData::AttributeMapping;
							meta->index = uint32_t( inputIdx );
							char nameBuf[128]; // uuuuggggggghh using sprintf for this because we dont know the inputIdx length
							const u32 requiredLen = snprintf( nameBuf, sizeof(nameBuf), "IN_%u_%.*s", uint32_t(inputIdx), int(varDef->nameToken->str.Length()), varDef->nameToken->str.GetPtr() ) + 1; // +NUL
							const u32 nameSize = ycRoundUpPow2( requiredLen, u32(ycShaderMetaData::kPadNamesToAlign) );
							meta->nameSize = nameSize;
							m_dstMetaData->AppendRaw( nameBuf, requiredLen );
							m_dstMetaData->AppendZero( nameSize - requiredLen );
						}
					}
					else
					{ // 'in' is GL 4.2, our target is 3.3!; also need do to shader stage linkage differently for 3.3!!!! TODO HACK FIXME
						Emit( "in highp " );
						EmitTypeName( varDef->typeToken->typeIndex );
						Emit( " VARYING_" );
					}
					Emit( varDef->nameToken );
					Emit( ";\n" );
				}
				++inputIdx;
			}
			Emit( "\n" );
		}

		{
			AST::NodeStructDef* outDef = (AST::NodeStructDef*)m_parser->GetTypeDesc( node->typeToken->typeIndex );
			ycAssert( outDef->nodeType == AST::kNode_StructDef );
			ureg outputIdx = 0;
			for( AST::NodeVariableDecl* varDef = (AST::NodeVariableDecl*)outDef->firstMember; varDef != nullptr; varDef = (AST::NodeVariableDecl*)varDef->nextNode )
			{
				ycAssert( varDef->nodeType == AST::kNode_VariableDecl );
				bool dontEmit = false;
				AST::Attribute* semantic = AST::FindAttr( varDef->firstAttr, "semantic" );
				if( semantic && m_stage == kStage_Vert && semantic->valueToken && semantic->valueToken->str.Equals( "POSITION", 8 ) )
				{
					dontEmit = true;
				}
				if( semantic && m_stage == kStage_Frag && semantic->valueToken )
				{
					if( semantic->valueToken->str.Equals( "DEPTH", 5 ) )
					{
						Emit( "layout (depth_any) out float gl_FragDepth;\n" );
						dontEmit = true;
					}
					else if( semantic->valueToken->str.Equals( "DEPTH_LE", 8 ) )
					{
						Emit( "layout (depth_less) out float gl_FragDepth;\n" );
						dontEmit = true;
					}
					else if( semantic->valueToken->str.Equals( "DEPTH_GE", 8 ) )
					{
						Emit( "layout (depth_greater) out float gl_FragDepth;\n" );
						dontEmit = true;
					}
				}
				if( !dontEmit )
				{
					if( m_target == kTarget_Vk )
					{
						Emit( "layout(location=" );
						Emit( outputIdx );
						Emit( ") " );
						Emit( "out highp " );
						EmitTypeName( varDef->typeToken->typeIndex );
						Emit( " OUT_" );
						Emit( outputIdx );
						Emit( "_" );
					}
					else
					{ // 'in' is GL 4.2, our target is 3.3!; also need do to shader stage linkage differently for 3.3!!!!
						Emit( "out highp " );
						EmitTypeName( varDef->typeToken->typeIndex );
						if( m_stage == kStage_Frag )
						{
							Emit( " OUT_" );
							Emit( outputIdx );
							Emit( "_" );
						}
						else
						{
							Emit( " VARYING_" );
						}
					}
					Emit( varDef->nameToken );
					Emit( ";\n" );
				}
				++outputIdx; // always increment even if not emitted, to make calculating idxes if quickly iterating simpler
			}
			Emit( "\n" );
		}
		#if YC_NDA
		#endif 
		Emit( "void main()\n" );
		Emit( "{\n" );
		PushIndent();

		const char* inArgStr = "";
		if( inToken && inToken->typeToken->typeIndex != kIntrinsicType_Void )
		{
			inArgStr = " _in ";

			EmitIndent();
			EmitTypeName( inToken->typeToken->typeIndex );
			Emit( " _in;\n" );

			AST::NodeStructDef* inDef = (AST::NodeStructDef*)m_parser->GetTypeDesc( inToken->typeToken->typeIndex );
			ycAssert( inDef->nodeType == AST::kNode_StructDef );
			ureg inputIdx = 0;
			for( AST::NodeVariableDecl* varDef = (AST::NodeVariableDecl*)inDef->firstMember; varDef != nullptr; varDef = (AST::NodeVariableDecl*)varDef->nextNode )
			{
				ycAssert( varDef->nodeType == AST::kNode_VariableDecl );

				EmitIndent();
				Emit( "_in." );
				Emit( varDef->nameToken );
				Emit( " = " );

				const char * builtin = GetBuiltinInput( varDef );

				if( builtin != nullptr )
				{
					Emit( builtin );
				}
				else
				{
					if( m_target == kTarget_Vk || m_stage == kStage_Vert )
					{
						Emit( "IN_" );
						Emit( inputIdx );
						Emit( "_" );
					}
					else
					{
						Emit( "VARYING_" );
					}
					Emit( varDef->nameToken );
				}

				Emit( ";\n" );
				inputIdx++;
			}
		}

		// Call into real main function.
		EmitIndent();
		EmitTypeName( node->typeToken->typeIndex );
		Emit( " _out = " );
		Emit( node->nameToken );
		Emit( "(" );
		Emit( inArgStr );
		Emit( ");\n" );

		// Process output.
		{
			AST::NodeStructDef* outDef = (AST::NodeStructDef*)m_parser->GetTypeDesc( node->typeToken->typeIndex );
			ycAssert( outDef->nodeType == AST::kNode_StructDef );

			ureg outputIdx = 0;
			for( AST::NodeVariableDecl* varDef = (AST::NodeVariableDecl*)outDef->firstMember; varDef != nullptr; varDef = (AST::NodeVariableDecl*)varDef->nextNode )
			{
				EmitIndent();

				AST::Attribute* semantic = AST::FindAttr( varDef->firstAttr, "semantic" );
				ycAssert( semantic );
				
				if( semantic->valueToken && semantic->valueToken->str.Equals( "POSITION" ) )
				{
					ycAssert( varDef->typeToken->typeIndex == kIntrinsicType_Float4 );
					Emit( "gl_Position" );
				}
				else if( semantic->valueToken && ( semantic->valueToken->str.Equals( "DEPTH" ) || semantic->valueToken->str.Equals( "DEPTH_LE" ) || semantic->valueToken->str.Equals( "DEPTH_GE" ) ) )
				{
					ycAssert( varDef->typeToken->typeIndex == kIntrinsicType_Float );
					Emit( "gl_FragDepth" );
				}
				else
				{
					if( m_target == kTarget_Vk || m_stage == kStage_Frag )
					{
						Emit( "OUT_" );
						Emit( outputIdx );
						Emit( "_" );
					}
					else
					{
						Emit( "VARYING_" );
					}
					Emit( varDef->nameToken );
				}

				Emit( " = _out." );
				Emit( varDef->nameToken );
				Emit( ";\n" );

				++outputIdx;
			}
		}

		PopIndent();
		Emit( "}\n" );
		Emit( "\n" );
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeIfBlock* node, const ureg flags )
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

	/*virtual*/ void GenGLSL::Process( AST::NodeForBlock* node, const ureg flags )
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

	/*virtual*/ void GenGLSL::Process( AST::NodeCBufferDef* node, const ureg flags )
	{
		YC_UNUSED( flags );
		AST::Attribute* idxAttr = AST::FindAttr( node->firstAttr, "index" );
		ycAssert( idxAttr );
		// TODO HACK FIXME: sanity check index attr value?
		
		ycShaderMetaData::UniformBufferMapping* meta = m_dstMetaData->AppendUninitialized< ycShaderMetaData::UniformBufferMapping >();
		new( meta )ycShaderMetaData::UniformBufferMapping;
		bool converted;
		meta->index = uint32_t( idxAttr->valueToken->str.ToUInt( &converted ) );
		ycAssert( converted );

		if( m_target == kTarget_Vk )
		{
			AST::Attribute* set     = AST::FindAttr( node->firstAttr, "descriptor_set" );
			AST::Attribute* binding = AST::FindAttr( node->firstAttr, "descriptor_binding" );
			ycAssert( set && binding );
			Emit( "layout(set=" );
			Emit( set->valueToken );
			Emit( ",binding=" );
			Emit( binding->valueToken );
			Emit( ",std140) uniform g_cbuffer_" );
			meta->descriptorSet     = uint32_t( set->valueToken->str.ToUInt( &converted ) );
			ycAssert( converted );
			meta->descriptorBinding = uint32_t( binding->valueToken->str.ToUInt( &converted ) );
			ycAssert( converted );
		}
		#if YC_NDA
		#endif
		else
		{
			Emit( "layout(std140) uniform g_cbuffer_" );
			meta->descriptorSet     = 0xffffffffu;
			meta->descriptorBinding = 0xffffffffu;
		}
		Emit( idxAttr->valueToken );
		Emit( "\n{\n" );
		PushIndent();
		ProcessNodeList( node->firstMember, kFlag_Statement );
		PopIndent();
		Emit( "};\n\n" );
		const u32 requiredLen = 10 + u32(idxAttr->valueToken->str.Length()) + 1; // g_cbuffer_ + idx + NUL
		const u32 nameSize = ycRoundUpPow2( requiredLen, u32(ycShaderMetaData::kPadNamesToAlign) );
		meta->nameSize = nameSize;
		m_dstMetaData->AppendRaw( "g_cbuffer_", 10 );
		m_dstMetaData->AppendRaw( idxAttr->valueToken->str.GetPtr(), idxAttr->valueToken->str.Length() );
		m_dstMetaData->AppendRaw( "", 1 );
		m_dstMetaData->AppendZero( nameSize - requiredLen );
	}

	/*virtual*/ void GenGLSL::Process( AST::NodeReturn* node, const ureg /*flags*/ )
	{
		EmitIndent(); Emit( "return " );
		ProcessNode( node->val, kFlag_Statement );
	}

	void GenGLSL::EmitTypeName( const ureg typeIndex )
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

				"vec1",
				"vec2",
				"vec3",
				"vec4",

				"ivec1",
				"ivec2",
				"ivec3",
				"ivec4",

				"uvec1",
				"uvec2",
				"uvec3",
				"uvec4",

				"bvec1",
				"bvec2",
				"bvec3",
				"bvec4",

				"mat2x2",
				"mat3x2",
				"mat4x2",

				"mat2x3",
				"mat3x3",
				"mat4x3",

				"mat2x4",
				"mat3x4",
				"mat4x4",

				"sampler2D",
				"sampler3D",
				"samplerCube",
				"usampler2D"
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

} // namespace ycShaderBuilder
