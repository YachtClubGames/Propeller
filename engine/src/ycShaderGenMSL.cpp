#include "ycShaderGenMSL.h"

#include "ycByteBuffer.h"
#include "ycMath.h"
#include "ycShaderAST.h"
#include "ycShaderMetaData.h"
#include "ycShaderParser.h"
#include "ycStd.h"
#include "ycStringUtils.h"
#include "ycStackString.h"
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
	
	static bool SearchListForVar( AST::Node* firstStatement, ycStringRef var );

	// MSL hates global vars, so sometimes we're searching function asts for a "global" to work out we have to pass it in
	static bool SearchForVar( AST::Node* node, ycStringRef var )
	{
		if( node == nullptr ) return false;

		switch( node->nodeType )
		{
			case AST::kNode_TypeAlias:       return false;
			case AST::kNode_StructDef:       return false;
			case AST::kNode_Literal:         return false;
			case AST::kNode_FunctionCall:    return SearchListForVar( ((AST::NodeFunctionCall*)node)->firstArg, var ) || SearchForVar( ((AST::NodeFunctionCall*)node)->func, var );
			case AST::kNode_VariableDecl:    return SearchForVar( ((AST::NodeVariableDecl*)node)->definition, var );
			case AST::kNode_FunctionDecl:    return SearchForVar( ((AST::NodeFunctionDecl*)node)->definition, var );
			case AST::kNode_CBufferDef:      return false; 
			case AST::kNode_UnaryOp:         return SearchForVar( ((AST::NodeUnaryOp*)node)->operand, var );
			case AST::kNode_InfixOp:         return SearchForVar( ((AST::NodeInfixOp*)node)->lhs, var ) || SearchForVar( ((AST::NodeInfixOp*)node)->rhs, var );
			case AST::kNode_CastOp:          return SearchListForVar( ((AST::NodeCastOp*)node)->firstArg, var );
			case AST::kNode_ArrayAccessOp:   return SearchForVar( ((AST::NodeArrayAccess*)node)->lhs, var ) || SearchForVar( ((AST::NodeArrayAccess*)node)->arrayIdx, var );
			case AST::kNode_MemberAccessOp:  return SearchForVar( ((AST::NodeMemberAccess*)node)->lhs, var );
			case AST::kNode_Block:           return SearchListForVar( ((AST::NodeBlock*)node)->firstStatement, var );
			case AST::kNode_IfBlock:         return SearchForVar( ((AST::NodeIfBlock*)node)->condition, var ) || SearchForVar( ((AST::NodeIfBlock*)node)->body, var ) || SearchForVar( ((AST::NodeIfBlock*)node)->otherwise, var );
			case AST::kNode_Return:          return SearchForVar( ((AST::NodeReturn*)node)->val, var );
			case AST::kNode_ForBlock:        return SearchForVar( ((AST::NodeForBlock*)node)->expr1, var ) || SearchForVar( ((AST::NodeForBlock*)node)->expr2, var ) || SearchForVar( ((AST::NodeForBlock*)node)->expr3, var ) || SearchForVar( ((AST::NodeForBlock*)node)->body, var );
			case AST::kNode_Variable:        return ((AST::NodeVariable*)node)->var->nameToken->str == var;
			case AST::kNode_Asm:             return false; 
			YC_NO_DEFAULT_ASSERT
		}

	}

	static bool SearchListForVar( AST::Node* firstStatement, ycStringRef var )
	{
		for( AST::Node* node = firstStatement; node != nullptr; node = node->nextNode )
		{
			if( SearchForVar( node, var ) )
			{
				return true;
			}
		}

		return false;
	}

	

	GenMSL::GenMSL( ycAllocator* mem /*= nullptr*/, const uint32_t stage, const uint32_t target /*= kTarget_Metal1_0*/ ) :
		Gen( mem ),
		m_target( target ),
		m_stage( stage )
	{
	}

	/*virtual*/ GenMSL::~GenMSL()
	{
	}

	/*virtual*/ void GenMSL::Preamble()
	{
		Emit( "#include <metal_stdlib>\n" );
		Emit( "#include <simd/simd.h>\n" );

		Emit( "using namespace metal;\n" );
		Emit( "\n" );
	}

	/*virtual*/ void GenMSL::Epilogue()
	{
		//m_dst->AppendRaw( "", 1 );
	}

	/*virtual*/ void GenMSL::Process( AST::NodeTypeAlias* node, const ureg flags )
	{
		//
		YC_UNUSED( node );
		YC_UNUSED( flags );
	}

	/*virtual*/ void GenMSL::Process( AST::NodeStructDef* node, const ureg flags )
	{
		if( node->nameToken->str.Equals( "In" ) )
		{
			m_inputBufferStruct = node;
			if( m_stage == kStage_Unknown ) { return; } // will process later when we have the stage
		}

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

	/*virtual*/ void GenMSL::Process( AST::NodeLiteral* node, const ureg flags )
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

	/*virtual*/ void GenMSL::Process( AST::NodeVariable* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }

		AST::NodeVariableDecl* var = node->var;

		// allow accessing cbuffers as if they were globals
		for( int ii=0; ii<m_cbuffers.Length(); ++ii )
		{
			for( AST::Node* subnode = m_cbuffers[ii]->firstMember; subnode != nullptr; subnode = subnode->nextNode )
			{
				if( subnode->nodeType == AST::kNode_VariableDecl )
				{
					AST::NodeVariableDecl* subvar = (AST::NodeVariableDecl*)subnode;
					if( subvar->nameToken->str == var->nameToken->str )
					{
						AST::Attribute* idxAttr = AST::FindAttr( m_cbuffers[ii]->firstAttr, "index" );
						ycAssert( idxAttr );
						Emit( "g_cbuffer_" );
						Emit( idxAttr->valueToken );
						Emit( "." );
					}
				}
			}
		}

		Emit( var->nameToken );

		if( IsTextureType( var->typeToken->typeIndex ) && !!( var->flags & AST::kNodeFlag_GlobalDecl ) )
		{
			Emit( "_view, " );
			Emit( var->nameToken );
			Emit( "_sampler " );
		}

		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeFunctionCall* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }

		const struct TextureFunc
		{
			const char* funcName;
			const char* methodName;
			enum { kNone = 0, kWantsSampler = 1 } flags;
		} textureFuncs[] = {
			{ "texture2D",           	   "sample",          TextureFunc::kWantsSampler, },
			{ "texture2DLOD",              "sample",          TextureFunc::kWantsSampler, },
			{ "texture3D",           	   "sample",          TextureFunc::kWantsSampler, },
			{ "textureCube",         	   "sample",          TextureFunc::kWantsSampler, },
			{ "texture2DOffs",       	   "sample",          TextureFunc::kWantsSampler, },
			{ "textureCubeLOD",      	   "sample",          TextureFunc::kWantsSampler, },
			{ "texture2DCmp",        	   "sample_compare",  TextureFunc::kWantsSampler, },
			{ "_texture2DUintLoad",        "read",            TextureFunc::kNone, },
			{ "_texture2DLoad",     	   "read",            TextureFunc::kNone, },
			{ "_texture2DMSLoad",     	   "read",            TextureFunc::kNone, },
			{ "_texture2DGetWidth",        "get_width",       TextureFunc::kNone, },
			{ "_texture2DGetHeight",       "get_height",      TextureFunc::kNone, },
			{ "_texture2DMSGetWidth",      "get_width",       TextureFunc::kNone, },
			{ "_texture2DMSGetHeight",     "get_height",      TextureFunc::kNone, },
			{ "texture2DMSSampleCount",    "get_num_samples", TextureFunc::kNone, },
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
				//if( !!( texVar->var->flags & AST::kNodeFlag_GlobalDecl ) )
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
			Emit( "discard_fragment()" );
		}
		else
		{
			Emit( node->func->nameToken );
			Emit( "( " );
				ProcessArgList( node->firstArg );

				// must now look for "globals" that need to be passed in
				for( AST::NodeCBufferDef* cbuf : m_cbuffers )
				{
					bool foundReference = false;
					for( AST::Node* gvar = cbuf->firstMember; gvar != nullptr; gvar = gvar->nextNode )
					{
						if( gvar->nodeType == AST::kNode_VariableDecl )
						{
							AST::NodeVariableDecl* gvarTyped = (AST::NodeVariableDecl*)gvar;
							foundReference = foundReference || SearchListForVar( node->func->definition, gvarTyped->nameToken->str );
						}
					}

					if( foundReference )
					{
						Emit( ", g_cbuffer_" );
						AST::Attribute* idxAttr = AST::FindAttr( cbuf->firstAttr, "index" );
						ycAssert( idxAttr );
						Emit( idxAttr->valueToken );
					}
				}
				for( AST::NodeVariableDecl* samp : m_samplers )
				{
					if( SearchListForVar( node->func->definition, samp->nameToken->str ) )
					{ 
						Emit( ", " );
						Emit( samp->nameToken );
						Emit( "_view, " );
						Emit( samp->nameToken );
						Emit( "_sampler" );
					}
				}
			Emit( " )" );
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeUnaryOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		else if( (flags&kFlag_NoParens) == 0 ) { Emit( "(" ); }
		Emit( node->opToken );
		ProcessNode( node->operand );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
		else if( (flags&kFlag_NoParens) == 0 ) { Emit( ")" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeInfixOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }
		else if( (flags&kFlag_NoParens) == 0 ){ Emit( "(" ); }
		if( node->opToken->type == ycShaderBuilder::kToken_Op_Star )
		{
			const uint32_t lhsType = GetNodeValueType( node->lhs );
			const uint32_t rhsType = GetNodeValueType( node->rhs );
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
		else if( (flags&kFlag_NoParens) == 0 ) { Emit( ")" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeCastOp* node, const ureg flags )
	{
		if( flags&kFlag_Statement ) { EmitIndent(); }

		u32 transposedType = 0;
		if( IsMatrixType( node->typeToken->typeIndex ) )
		{
			transposedType = TransposeMatrixType( node->typeToken->typeIndex );
			// if( transposedType == node->typeToken->typeIndex ) { transposedType = 0; }
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
		
		if( transposedType != 0 )
		{
			Emit( ")" );
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeArrayAccess* node, const ureg flags )
	{
		if( flags & kFlag_Statement ) { EmitIndent(); }
		ProcessNode( node->lhs, flags );
		Emit( "[" );
		ProcessNode( node->arrayIdx, flags );
		Emit( "]" );
		if( flags & kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeMemberAccess* node, const ureg flags )
	{
		// see if these are vars with semantic types that can't be in the input struct
		bool isInStruct = true;
		if( node->lhs->nodeType == AST::kNode_Variable )
		{
			AST::NodeVariable* classVar = (AST::NodeVariable*)node->lhs;
			if( m_inputBufferStruct && classVar->var == m_inputBufferDecl )
			{
				for( AST::Node* subnode = m_inputBufferStruct->firstMember; subnode != nullptr; subnode = subnode->nextNode )
				{
					if( subnode->nodeType == AST::kNode_VariableDecl )
					{
						AST::NodeVariableDecl* subVar = (AST::NodeVariableDecl*)subnode;

						if( subVar->nameToken->str == node->memberToken->str )
						{
							AST::Attribute* attr = AST::FindAttr( subVar->firstAttr, "semantic" );
							if( attr && attr->valueToken && attr->valueToken->str.BeginsWith( "VERTEX_ID" ) )
							{
								isInStruct = false;
							}
							if( attr && attr->valueToken && attr->valueToken->str.BeginsWith( "INSTANCE_ID" ) )
							{
								isInStruct = false;
							}
						}
					}
				}
			}
		}

		if( flags&kFlag_Statement ) { EmitIndent(); }
		if( isInStruct )
		{
			ProcessNode( node->lhs );
			Emit( "." );
		}
		Emit( node->memberToken );
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeVariableDecl* node, const ureg flags )
	{
		AST::Attribute* attr = AST::FindAttr( node->firstAttr, "semantic" );
		if( attr && attr->valueToken && attr->valueToken->str.BeginsWith( "VERTEX_ID" ) )
		{
			return;
		}
		if( attr && attr->valueToken && attr->valueToken->str.BeginsWith( "INSTANCE_ID" ) )
		{
			return;
		}

		if( flags&kFlag_Statement ) { EmitIndent(); }
		if( IsTextureType( node->typeToken->typeIndex ) )
		{
			bool alreadyProcessed = false;
			for( AST::NodeVariableDecl* s : m_samplers )
			{
				if( s == node )
				{
					alreadyProcessed = true;
					break;
				}
			}

			if( !alreadyProcessed && node->flags&AST::kNodeFlag_GlobalDecl )
			{
				m_samplers.Append( node );

				AST::Attribute* idxAttr = AST::FindAttr( node->firstAttr, "index" );
				ycAssert( idxAttr );
				bool converted;
				const uint32_t samplerIndex = uint32_t( idxAttr->valueToken->str.ToUInt( &converted ) );
				ycAssert( converted );
				AddMetaSampler( samplerIndex, 0xffffffffu, 0xffffffffu, node->nameToken->str ); // TODO HACK FIXME: name is a lie! need separate view/samplers!!!

				ycAssert( node->arraySize == 0 ); // NOT SUPPORTED ATM!!
			}
			else
			{
				ycStringRef typeName;
				switch( node->typeToken->typeIndex )
				{
					case kIntrinsicType_Tex2D:
						if( AST::GetAttrBool( node->firstAttr, "shadow" ) )
						{
							typeName.Set( "depth2d<float>" );
						}
						else
						{
							typeName.Set( "texture2d<float>" );
						}
						break;
					case kIntrinsicType_Tex3D:   typeName.Set( "texture3d<float>" ); break;
					case kIntrinsicType_TexCube: typeName.Set( "texturecube<float>" ); break;
					case kIntrinsicType_Tex2DMS: typeName.Set( "texture2d_ms<float>" ); break;
					case kIntrinsicType_Tex2DUint: typeName.Set( "texture2d<uint>" ); break;
					YC_NO_DEFAULT_ASSERT
				}

				Emit( typeName );
				Emit( " " );
				Emit( node->nameToken );
				Emit( "_view, " );
				Emit( "sampler " );
				Emit( node->nameToken );
				Emit( "_sampler" );
			}
		}
		else
		{
			const ureg modifiers = node->modifiers;
			if(      modifiers&AST::NodeVariableDecl::kModifier_InOut ) { Emit( "thread " ); }
			else if( modifiers&AST::NodeVariableDecl::kModifier_Out   ) { Emit( "thread " ); }
			if( modifiers&AST::NodeVariableDecl::kModifier_Const ) { Emit( "const ", 6 ); }
			EmitTypeName( node->typeToken->typeIndex );
			if(      modifiers&AST::NodeVariableDecl::kModifier_InOut ) { Emit( "&" ); }
			else if( modifiers&AST::NodeVariableDecl::kModifier_Out   ) { Emit( "&" ); }
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
			if( attr )
			{
				Emit( " " );

				if( m_structDef->nameToken->str.Equals( "In" ) && m_stage == kStage_Vert )
				{
					if( attr->valueToken == nullptr )
					{
						// Emit an auto TEXCOORDn.
						u32 texcoord = FindStructDefAutoTEXCOORDn( m_structDef, node );
						char semanticName[ 32 ] = {};
						ycStringUtils::SprintF( semanticName, sizeof( semanticName ), "[[attribute(%d)]]", texcoord+1 );
						Emit( semanticName );
					}
					else if( attr->valueToken->str.Equals( "POSITION" ) )
					{
						Emit( "[[attribute(0)]]" );
					}
					else if( attr->valueToken->str.BeginsWith( "TEXCOORD" ) )
					{
						ycStringRef num = attr->valueToken->str;
						num.Take("TEXCOORD");
						ureg index = ycStringUtils::ToInt( num.Get(), num.Length() );

						char semanticName[ 32 ] = {};
						ycStringUtils::SprintF( semanticName, sizeof( semanticName ), "[[attribute(%d)]]", index+1 );
						Emit( semanticName );
					}
					else
					{
						ycAssert( 0 );
					}
				}
				else 
				{
					if( attr->valueToken == nullptr )
					{
						// Emit an auto TEXCOORDn.
						ycAssert( m_structDef != nullptr );
						u32 texcoord = FindStructDefAutoTEXCOORDn( m_structDef, node );
						char semanticName[ 32 ] = {};
						ycStringUtils::SprintF( semanticName, sizeof( semanticName ), "[[user(TEXCOORD%d)]]", texcoord );
						Emit( semanticName );
					}
					else if( attr->valueToken->str.Equals( "POSITION", 8 ) )
					{
						Emit( "[[position]]" );
					}
					else if( attr->valueToken->str.BeginsWith( "COLOR" ) )
					{
						s64 colorNum = attr->valueToken->str.Skip( 5 ).Word().ToInt();

						char semanticName[ 32 ] = {};
						ycStringUtils::SprintF( semanticName, sizeof( semanticName ), "[[color(%d)]]", colorNum );
						Emit( semanticName );
					}
					else if( attr->valueToken->str.Equals( "VERTEX_ID", 9 ) )
					{
						ycAssert( 0 ); // should never emit these
					}
					else if( attr->valueToken->str.Equals( "INSTANCE_ID", 11 ) )
					{
						ycAssert( 0 ); // should never emit these
					}
					else if( attr->valueToken->str.Equals( "DEPTH", 5 ) )
					{
						Emit( "[[depth(any)]]" );
					}
					else if( attr->valueToken->str.Equals( "DEPTH_LE", 8 ) )
					{
						// if this matches the ARB_conservative_raster gl ext, less/greater implicitely include 'or equal'
						Emit( "[[depth(less)]]" );
					}
					else if( attr->valueToken->str.Equals( "DEPTH_GE", 8 ) )
					{
						Emit( "[[depth(greater)]]" );
					}
					else
					{
						ycStackString<> semanticName;
						semanticName.Append( "[[user(" );
						semanticName.Append( attr->valueToken->str );
						semanticName.Append( ")]]" );
						Emit( semanticName );
					}
				}
			}
		}
		if( flags&kFlag_Statement ) { Emit( ";\n" ); }
	}

	/*virtual*/ void GenMSL::Process( AST::NodeFunctionDecl* node, const ureg flags )
	{
		YC_UNUSED( flags );
		if( AST::FindAttr( node->firstAttr, "intrinsic" ) ) { return; }

		bool isEntryPoint = false;
		AST::Attribute* stage = AST::FindAttr( node->firstAttr, "stage" );
		if( stage != nullptr )
		{
			isEntryPoint = true;

			u32 givenStage = m_stage;
			if( stage->valueToken->str.Equals( "vertex", 6 ) )
			{
				m_stage = kStage_Vert;
			}
			else if( stage->valueToken->str.Equals( "fragment", 8 ) )
			{
				m_stage = kStage_Frag;
			}
			else { ycFatal(); }
			ycAssert( givenStage == kStage_Unknown || givenStage == m_stage );

			if( givenStage == kStage_Unknown && m_inputBufferStruct ) { ProcessNode( m_inputBufferStruct, kFlag_Statement ); }
		}
		
		if( isEntryPoint && m_stage == kStage_Vert ) { Emit( "vertex " ); }
		if( isEntryPoint && m_stage == kStage_Frag ) { Emit( "fragment " ); }

		EmitTypeName( node->typeToken->typeIndex );
		Emit( " " );
		if( isEntryPoint && m_stage == kStage_Vert ) { Emit( "vertexMain " ); }
		else if( isEntryPoint && m_stage == kStage_Frag ) { Emit( "fragmentMain " ); }
		else { Emit( node->nameToken ); }

		
		bool hasArgs = false;
		Emit( "( " );
		if( node->firstArg )
		{
			for( AST::Node* arg = node->firstArg; arg != nullptr; arg = arg->nextNode )
			{
				if( isEntryPoint ) 
				{
					if( arg->nodeType == AST::kNode_VariableDecl )
					{
						AST::NodeVariableDecl* argVar = (AST::NodeVariableDecl*)arg;
						if( argVar->nameToken->str == "IN" )
						{
							m_inputBufferDecl = argVar;
						}
					}
				}

				if( hasArgs ) { Emit( ", " ); }
				ProcessNode( arg );
				if( m_inputBufferDecl == arg )
				{
					Emit( " [[stage_in]]" );
				}
				hasArgs = true;
			}
			if( isEntryPoint ) 
			{
				for( int ii=0; ii<m_cbuffers.Length(); ++ii )
				{
					AST::Attribute* idxAttr = AST::FindAttr( m_cbuffers[ii]->firstAttr, "index" );
					ycAssert( idxAttr );
					// TODO HACK FIXME: sanity check index attr value?

					// check for any references to the cbuffer, don't emit if unused
					bool foundReference = false;
					for( AST::Node* gvar = m_cbuffers[ii]->firstMember; gvar != nullptr; gvar = gvar->nextNode )
					{
						if( gvar->nodeType == AST::kNode_VariableDecl )
						{
							AST::NodeVariableDecl* gvarTyped = (AST::NodeVariableDecl*)gvar;
							foundReference = foundReference || SearchListForVar( node->definition->firstStatement, gvarTyped->nameToken->str );
						}
					}

					if( !foundReference ) { continue; }

					if( hasArgs ) { Emit( ", \n\t" ); }
					Emit( "constant Cbuffer_" );
					Emit( idxAttr->valueToken );
					Emit( "& g_cbuffer_" );
					Emit( idxAttr->valueToken );
					Emit( " [[buffer(" );
					Emit( idxAttr->valueToken );
					Emit( ")]]" );
					
					hasArgs = true;
				}

				for( AST::NodeVariableDecl* subNode : m_samplers )
				{
					AST::Attribute* idxAttr = AST::FindAttr( subNode->firstAttr, "index" );
					ycAssert( idxAttr );
					bool converted;
					const uint32_t samplerIndex = uint32_t( idxAttr->valueToken->str.ToUInt( &converted ) );
					ycAssert( converted );
					// TODO HACK FIXME: sanity check index attr value?

					// don't emit unused sampler, as some materials don't set unused samplers
					if( !SearchListForVar( node->definition->firstStatement, subNode->nameToken->str ) )
					{ 
						continue;
					}
					
					ycStringRef typeName;
					switch( subNode->typeToken->typeIndex )
					{
					case kIntrinsicType_Tex2D: 
						if( AST::GetAttrBool( subNode->firstAttr, "shadow" ) )
						{
							typeName.Set( "depth2d<float>" );
						}
						else
						{
							typeName.Set( "texture2d<float>" );
						}
						break;
					case kIntrinsicType_Tex3D:   typeName.Set( "texture3d<float>" ); break;
					case kIntrinsicType_TexCube: typeName.Set( "texturecube<float>" ); break;
					case kIntrinsicType_Tex2DMS: typeName.Set( "texture2d_ms<float>" ); break;
					case kIntrinsicType_Tex2DUint: typeName.Set( "texture2d<uint>" ); break;
					YC_NO_DEFAULT_ASSERT
					}
					
					if( hasArgs ) { Emit( ", \n\t" ); }
					Emit( typeName );
					Emit( " " );
					Emit( subNode->nameToken );
					Emit( "_view [[texture(" );
					Emit( idxAttr->valueToken );
					Emit( ")]], " );

					Emit( "\n\tsampler " );
					Emit( subNode->nameToken );
					Emit( "_sampler [[sampler(" );
					Emit( idxAttr->valueToken );
					Emit( ")]]" );
					ycAssert( subNode->definition == nullptr );
					
					hasArgs = true;
				}
				
				// some attribs can only be passed as args so we pull them from the cbuffer and put them here by themselves
				if( m_inputBufferStruct && m_stage == kStage_Vert )
				{
					for( AST::Node* subnode = m_inputBufferStruct->firstMember; subnode != nullptr; subnode = subnode->nextNode )
					{
						if( subnode->nodeType == AST::kNode_VariableDecl )
						{
							AST::NodeVariableDecl* subVar = (AST::NodeVariableDecl*)subnode;

							AST::Attribute* attr = AST::FindAttr( subVar->firstAttr, "semantic" );
							if( attr && attr->valueToken )
							{
								if( attr->valueToken->str.Equals( "VERTEX_ID" ) )
								{
									Emit( ", \n\t" );
									EmitTypeName( subVar->typeToken->typeIndex );
									Emit( " " );
									Emit( subVar->nameToken );
									Emit( " [[vertex_id]]" );
								}
								else if( attr->valueToken->str.Equals( "INSTANCE_ID" ) )
								{
									Emit( ", \n\t" );
									EmitTypeName( subVar->typeToken->typeIndex );
									Emit( " " );
									Emit( subVar->nameToken );
									Emit( " [[instance_id]]" );
								}
							}
						}
					}
				}
			}
		}

		if( !isEntryPoint ) 
		{
			for( AST::NodeCBufferDef* cbuf : m_cbuffers )
			{
				bool foundReference = false;
				for( AST::Node* gvar = cbuf->firstMember; gvar != nullptr; gvar = gvar->nextNode )
				{
					if( gvar->nodeType == AST::kNode_VariableDecl )
					{
						AST::NodeVariableDecl* gvarTyped = (AST::NodeVariableDecl*)gvar;
						foundReference = foundReference || SearchListForVar( node->definition->firstStatement, gvarTyped->nameToken->str );
					}
				}

				if( foundReference )
				{
					if( hasArgs )
					{
						Emit( ", " );
					}
					AST::Attribute* idxAttr = AST::FindAttr( cbuf->firstAttr, "index" );
					ycAssert( idxAttr );
					Emit( "constant Cbuffer_" );
					Emit( idxAttr->valueToken );
					Emit( "& g_cbuffer_" );
					Emit( idxAttr->valueToken );
					hasArgs = true;
				}
			}
			for( AST::NodeVariableDecl* samp : m_samplers )
			{
				if( SearchListForVar( node->definition->firstStatement, samp->nameToken->str ) )
				{ 
					Emit( ", " );
					ProcessNode( samp );
				}
			}
		}
		
		if( hasArgs )
		{
			Emit( " " );
		}
		Emit( ")" );
		
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

	/*virtual*/ void GenMSL::Process( AST::NodeBlock* node, const ureg flags )
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

	/*virtual*/ void GenMSL::Process( AST::NodeIfBlock* node, const ureg flags )
	{
		YC_UNUSED( flags );
		EmitIndent();
		Emit( "if( " );
		ProcessNode( node->condition, kFlag_NoParens );
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
				ProcessNode( elseif->condition, kFlag_NoParens );
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

	/*virtual*/ void GenMSL::Process( AST::NodeCBufferDef* node, const ureg flags )
	{
		m_cbuffers.Append( node );

		YC_UNUSED( flags );
		AST::Attribute* idxAttr = AST::FindAttr( node->firstAttr, "index" );
		ycAssert( idxAttr );
		// TODO HACK FIXME: sanity check index attr value?

		Emit( "struct Cbuffer_" );
		Emit( idxAttr->valueToken );
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

	/*virtual*/ void GenMSL::Process( AST::NodeForBlock* node, const ureg flags )
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

	/*virtual*/ void GenMSL::Process( AST::NodeReturn* node, const ureg /*flags*/ )
	{
		EmitIndent(); Emit( "return " );
		ProcessNode( node->val, kFlag_Statement );
	}
	
	void GenMSL::EmitTypeName( const ureg typeIndex )
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

	void GenMSL::ProcessArgList( AST::Node* node, const ureg flags )
	{
		for( ; node != nullptr; node = node->nextNode )
		{
			ProcessNode( node, flags );
			if( node->nextNode ) { Emit( ", " ); }
		}
	}

} // namespace ycShaderBuilder
