#include "ycShaderGen.h"

#include "ycByteBuffer.h"
#include "ycMath.h"
#include "ycShaderAST.h"
#include "ycShaderMetaData.h"
#include "ycShaderParser.h"
#include "ycStd.h"
#include "ycStringUtils.h"
//#include "ycSysAllocator.h"

// TODO HACK FIXME: temp? should have our own, at least basic, formatter
#include <stdio.h>

namespace ycShaderBuilder
{
	Gen::Gen( ycAllocator* mem ) :
		m_mem( mem ? mem : g_defaultMem ),
		m_dst( nullptr ),
		m_parser( nullptr ),
		m_indent( 0 )
	{
	}

	/*virtual*/ Gen::~Gen()
	{
	}

	void Gen::Generate( ycByteBuffer* dst, ycByteBuffer* dstMetaData, Parser* parser )
	{
		ycAssert( m_dst == nullptr );
		m_dst         = dst;
		m_dstMetaData = dstMetaData;
		m_parser      = parser;

		Preamble();
		ProcessNodeList( parser->GetFirstStatement(), kFlag_Statement );
		Epilogue();

		ycShaderMetaData::Item metaEnd( ycShaderMetaData::kEnd );
		m_dstMetaData->AppendRaw( &metaEnd, sizeof(metaEnd) );
	}

	void Gen::ProcessNode( AST::Node* node, const ureg flags )
	{
		switch( node->nodeType )
		{
		case AST::kNode_TypeAlias:       Process( (AST::NodeTypeAlias*      )node, flags ); break;
		case AST::kNode_StructDef:       Process( (AST::NodeStructDef*      )node, flags ); break;
		case AST::kNode_Literal:         Process( (AST::NodeLiteral*        )node, flags ); break;
		case AST::kNode_Variable:        Process( (AST::NodeVariable*       )node, flags ); break;
		case AST::kNode_FunctionCall:    Process( (AST::NodeFunctionCall*   )node, flags ); break;
		case AST::kNode_UnaryOp:         Process( (AST::NodeUnaryOp*        )node, flags ); break;
		case AST::kNode_InfixOp:         Process( (AST::NodeInfixOp*        )node, flags ); break;
		case AST::kNode_CastOp:          Process( (AST::NodeCastOp*         )node, flags ); break;
		case AST::kNode_ArrayAccessOp:   Process( (AST::NodeArrayAccess*    )node, flags ); break;
		case AST::kNode_MemberAccessOp:  Process( (AST::NodeMemberAccess*   )node, flags ); break;
		case AST::kNode_VariableDecl:    Process( (AST::NodeVariableDecl*   )node, flags ); break;
		case AST::kNode_FunctionDecl:    Process( (AST::NodeFunctionDecl*   )node, flags ); break;
		case AST::kNode_Block:           Process( (AST::NodeBlock*          )node, flags ); break;
		case AST::kNode_IfBlock:         Process( (AST::NodeIfBlock*        )node, flags ); break;
		case AST::kNode_CBufferDef:      Process( (AST::NodeCBufferDef*     )node, flags ); break;
		case AST::kNode_Return:          Process( (AST::NodeReturn*         )node, flags ); break;
		case AST::kNode_ForBlock:        Process( (AST::NodeForBlock*       )node, flags ); break;
		case AST::kNode_Asm:
			{
				ycStringRef val = ((AST::NodeAsm*)node)->val->str;
				val.TakeFirst();
				val.TakeLast();
				Emit( val );
				Emit( "\n", 1 );
			}
			break;
		YC_NO_DEFAULT_ASSERT
		}
	}

	void Gen::ProcessNodeList( AST::Node* firstNode, const ureg flags )
	{
		for( AST::Node* node = firstNode; node != nullptr; node = node->nextNode )
		{
			ProcessNode( node, flags );
		}
	}

	const char s_indent[] = "                                ";
	void Gen::PushIndent()
	{
		++m_indent;
		ycAssert( m_indent*2 <= YC_ARRAY_SIZE(s_indent) );
	}

	void Gen::PopIndent()
	{
		ycAssert( m_indent > 0 );
		--m_indent;
	}

	void Gen::EmitIndent()
	{
		m_dst->AppendRaw( s_indent, m_indent*2 );
		//ycDebug::Printf( "%.*s", m_indent*2, s_indent);
	}

	void Gen::Emit( const char* str )
	{
		m_dst->AppendRaw( str, ycStringUtils::Length( str ) );
		//ycDebug::Print( str );
	}

	void Gen::Emit( const char* str, const ureg strLen )
	{
		m_dst->AppendRaw( str, strLen );
		//ycDebug::Printf( "%.*s", strLen, str);
	}

	void Gen::Emit( const ycStringRef& str )
	{
		m_dst->AppendRaw( str.GetPtr(), str.Length() );
	}

	void Gen::Emit( const TokenDesc* token )
	{
		Emit( token->str );
	}

	void Gen::Emit( const float num )
	{
		char buf[64];
		snprintf( buf, sizeof(buf), "%f", num );
		m_dst->AppendRaw( buf, ycStringUtils::Length( buf ) );
		//ycDebug::Printf( "%f", num );
	}

	void Gen::Emit( const int32_t num )
	{
		char buf[64];
		snprintf( buf, sizeof(buf), "%d", num );
		m_dst->AppendRaw( buf, ycStringUtils::Length( buf ) );
		//ycDebug::Printf( "%d", num );
	}

	void Gen::Emit( const uint32_t num )
	{
		char buf[64];
		snprintf( buf, sizeof(buf), "%u", num );
		m_dst->AppendRaw( buf, ycStringUtils::Length( buf ) );
		//ycDebug::Printf( "%u", num );
	}
		
	void Gen::AddMetaAttribute( const uint32_t index, const char* name )
	{
		AddMetaAttribute( index, name, ycStringUtils::Length( name ) );
	}

	void Gen::AddMetaAttribute( const uint32_t index, const char* name, const ureg nameLen )
	{
		AddMetaAttribute( index, ycStringRef( name, nameLen ) );
	}

	void Gen::AddMetaAttribute( const uint32_t index, const ycStringRef& name )
	{
		const u32 requiredLen = u32( name.Length() + 1 /*NUL terminator*/ );
		const u32 nameSize = ycRoundUpPow2( requiredLen, u32(ycShaderMetaData::kPadNamesToAlign) );
		ycShaderMetaData::AttributeMapping* attrib = m_dstMetaData->AppendUninitialized< ycShaderMetaData::AttributeMapping >();
		new( attrib )ycShaderMetaData::AttributeMapping;
		attrib->index    = index;
		attrib->nameSize = nameSize;
		m_dstMetaData->AppendRaw( name.GetPtr(), name.Length() );
		m_dstMetaData->AppendRaw( "", 1 ); // NUL byte
		m_dstMetaData->AppendZero( nameSize - requiredLen );
	}

	void Gen::AddMetaUniformBuffer( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name )
	{
		AddMetaUniformBuffer( index, descriptorSet, descriptorBinding, name, ycStringUtils::Length( name ) );
	}

	void Gen::AddMetaUniformBuffer( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name, const ureg nameLen )
	{
		AddMetaUniformBuffer( index, descriptorSet, descriptorBinding, ycStringRef( name, nameLen ) );
	}
	
	void Gen::AddMetaUniformBuffer( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const ycStringRef& name )
	{
		const u32 requiredLen = u32( name.Length() + 1 /*NUL terminator*/ );
		const u32 nameSize = ycRoundUpPow2( requiredLen, u32(ycShaderMetaData::kPadNamesToAlign) );
		ycShaderMetaData::UniformBufferMapping* ubo = m_dstMetaData->AppendUninitialized< ycShaderMetaData::UniformBufferMapping >();
		new( ubo )ycShaderMetaData::UniformBufferMapping;
		ubo->index             = index;
		ubo->descriptorSet     = descriptorSet;
		ubo->descriptorBinding = descriptorBinding;
		ubo->nameSize          = nameSize;
		m_dstMetaData->AppendRaw( name.GetPtr(), name.Length() );
		m_dstMetaData->AppendRaw( "", 1 ); // NUL byte
		m_dstMetaData->AppendZero( nameSize - requiredLen );
	}

	void Gen::AddMetaSampler( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name )
	{
		AddMetaSampler( index, descriptorSet, descriptorBinding, name, ycStringUtils::Length( name ) );
	}

	void Gen::AddMetaSampler( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const char* name, const ureg nameLen )
	{
		AddMetaSampler( index, descriptorSet, descriptorBinding, ycStringRef( name, nameLen ) );
	}

	void Gen::AddMetaSampler( const uint32_t index, const uint32_t descriptorSet, const uint32_t descriptorBinding, const ycStringRef& name )
	{
		const u32 requiredLen = u32( name.Length() + 1 /*NUL terminator*/ );
		const u32 nameSize = ycRoundUpPow2( requiredLen, u32(ycShaderMetaData::kPadNamesToAlign) );
		ycShaderMetaData::SamplerMapping* sampler = m_dstMetaData->AppendUninitialized< ycShaderMetaData::SamplerMapping >();
		new( sampler )ycShaderMetaData::SamplerMapping;
		sampler->index             = index;
		sampler->descriptorSet     = descriptorSet;
		sampler->descriptorBinding = descriptorBinding;
		sampler->nameSize          = nameSize;
		m_dstMetaData->AppendRaw( name.GetPtr(), name.Length() );
		m_dstMetaData->AppendRaw( "", 1 ); // NUL byte
		m_dstMetaData->AppendZero( nameSize - requiredLen );
	}

} // namespace ycShaderBuilder
