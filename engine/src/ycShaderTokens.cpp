#include "ycShaderTokens.h"

#include "ycStd.h"

namespace ycShaderBuilder
{

	const char* GetTokenName( TokenType token )
	{
		switch( token )
		{
		case kToken_Keyword:         return "keyword";
		case kToken_Keyword_Typedef: return "typedef";
		case kToken_Keyword_Struct:  return "struct";
		case kToken_Keyword_If:      return "if";
		case kToken_Keyword_Else:    return "else";
		case kToken_Keyword_For:     return "for keyword";
		case kToken_Keyword_Return:  return "return keyword";
		case kToken_Keyword_CBuffer: return "cbuffer keyword";
		case kToken_Literal:         return "numeric literal";
		case kToken_ConstantString:  return "string literal";
		case kToken_TypeName:        return "type name";
		case kToken_Identifier:      return "identifier";
		case kToken_Op:              return "operator";
		case kToken_Op_Semicolon:    return "semicolon";
		case kToken_Op_Comma:        return "comma";
		case kToken_Op_Dot:          return "period";
		case kToken_Op_Assign:       return "assignment operator";
		case kToken_Op_Equals:       return "equality operator";
		case kToken_Op_NEquals:      return "not equals operator";
		case kToken_Op_LEquals:      return "less than or equals operator";
		case kToken_Op_GEquals:      return "greater than or equals operator";
		case kToken_Op_Less:         return "less than operator";
		case kToken_Op_Greater:      return "greater than operator";
		case kToken_Op_Plus:         return "plus operator";
		case kToken_Op_Dash:         return "dash operator";
		case kToken_Op_Star:         return "asterisk operator";
		case kToken_Op_Slash:        return "forward slash operator";
		case kToken_Op_Colon:        return "colon";
		case kToken_BeginBlock:      return "begin block {";
		case kToken_EndBlock:        return "end block }";
		case kToken_LeftParen:       return "left paren (";
		case kToken_RightParen:      return "right paren )";
		case kToken_LeftBracket:     return "left bracket [";
		case kToken_RightBracket:    return "right bracket ]";
		case kToken_EOF:             return "End of file marker";
		YC_NO_DEFAULT_ASSERT_RETURN( nullptr )
		}
	}

	ureg GetIntrinsicTypeNumComponents( const u32 type )
	{
		if( type >= kIntrinsicType_Begin_Scalars && type <= kIntrinsicType_End_Scalars )
		{
			return 1;
		}
		else if( type >= kIntrinsicType_Begin_Aggregates && type <= kIntrinsicType_End_Aggregates )
		{
			if( type <= kIntrinsicType_Float4 )
			{
				return type - kIntrinsicType_Float1 + 1;
			}
			else if( type <= kIntrinsicType_Int4 )
			{
				return type - kIntrinsicType_Int1 + 1;
			}
			else if( type <= kIntrinsicType_UInt4 )
			{
				return type - kIntrinsicType_UInt1 + 1;
			}
			else if( type <= kIntrinsicType_Bool4 )
			{
				return type - kIntrinsicType_Bool1 + 1;
			}
			else
			{
				ycAssert( false );
			}
			static_assert( kIntrinsicType_Bool4 == kIntrinsicType_End_Aggregates, "add your new intrinsic type above" );
		}
		else if( type >= kIntrinsicType_Begin_Matrices && type <= kIntrinsicType_End_Matrices )
		{
			return GetIntrinsicTypeNumRows( type );
		}
		return 0;
	}

	ureg GetIntrinsicTypeNumColumns( const u32 type )
	{
		if( type >= kIntrinsicType_Begin_Matrices && type <= kIntrinsicType_End_Matrices )
		{
			return ( (type-kIntrinsicType_Begin_Matrices ) % 3 ) + 2;
		}
		return 0;
	}

	ureg GetIntrinsicTypeNumRows( const u32 type )
	{
		if( type >= kIntrinsicType_Begin_Matrices && type <= kIntrinsicType_End_Matrices )
		{
			return ( (type-kIntrinsicType_Begin_Matrices ) / 3 ) + 2;
		}
		return 0;
	}

	u32 GetAggregateScalarType( const u32 type )
	{
		ycAssert( type >= kIntrinsicType_Begin_Aggregates && type <= kIntrinsicType_End_Aggregates );
		switch( type )
		{
		case kIntrinsicType_Float1: // fall through
		case kIntrinsicType_Float2: // fall through
		case kIntrinsicType_Float3: // fall through
		case kIntrinsicType_Float4: // fall through
			return kIntrinsicType_Float;
		case kIntrinsicType_Int1: // fall through
		case kIntrinsicType_Int2: // fall through
		case kIntrinsicType_Int3: // fall through
		case kIntrinsicType_Int4: // fall through
			return kIntrinsicType_Int;
		case kIntrinsicType_UInt1: // fall through
		case kIntrinsicType_UInt2: // fall through
		case kIntrinsicType_UInt3: // fall through
		case kIntrinsicType_UInt4: // fall through
			return kIntrinsicType_UInt;
		case kIntrinsicType_Bool1: // fall through
		case kIntrinsicType_Bool2: // fall through
		case kIntrinsicType_Bool3: // fall through
		case kIntrinsicType_Bool4: // fall through
			return kIntrinsicType_Bool;
		YC_NO_DEFAULT_ASSERT_RETURN( 0 )
		}
	}

	u32 GetMatrixAggregateType( const u32 type )
	{
		ycAssert( type >= kIntrinsicType_Begin_Matrices && type <= kIntrinsicType_End_Matrices );
		switch( type )
		{
		case kIntrinsicType_Float2x2: // fall through
		case kIntrinsicType_Float2x3: // fall through
		case kIntrinsicType_Float2x4: // fall through
			return kIntrinsicType_Float2;
		case kIntrinsicType_Float3x2: // fall through
		case kIntrinsicType_Float3x3: // fall through
		case kIntrinsicType_Float3x4: // fall through
			return kIntrinsicType_Float3;
		case kIntrinsicType_Float4x2: // fall through
		case kIntrinsicType_Float4x3: // fall through
		case kIntrinsicType_Float4x4: // fall through
			return kIntrinsicType_Float4;
		YC_NO_DEFAULT_ASSERT_RETURN( 0 )
		}
	}

	u32 GetAggregateTypeForNumComponents( const u32 scalarType, const ureg numComponents )
	{
		if( scalarType == kIntrinsicType_Float )
		{
			if( numComponents == 2 ) { return kIntrinsicType_Float2; }
			if( numComponents == 3 ) { return kIntrinsicType_Float3; }
			if( numComponents == 4 ) { return kIntrinsicType_Float4; }
		}
		else if( scalarType == kIntrinsicType_Int )
		{
			if( numComponents == 2 ) { return kIntrinsicType_Int2; }
			if( numComponents == 3 ) { return kIntrinsicType_Int3; }
			if( numComponents == 4 ) { return kIntrinsicType_Int4; }
		}
		else if( scalarType == kIntrinsicType_UInt )
		{
			if( numComponents == 2 ) { return kIntrinsicType_UInt2; }
			if( numComponents == 3 ) { return kIntrinsicType_UInt3; }
			if( numComponents == 4 ) { return kIntrinsicType_UInt4; }
		}
		else if( scalarType == kIntrinsicType_Bool )
		{
			if( numComponents == 2 ) { return kIntrinsicType_Bool2; }
			if( numComponents == 3 ) { return kIntrinsicType_Bool3; }
			if( numComponents == 4 ) { return kIntrinsicType_Bool4; }
		}
		ycAssert( false );
		return 0;
	}

	u32 GetMatrixTypeForAggregateTypeAndNumColumns( const u32 aggregateType, const ureg numColumns )
	{
		if( aggregateType == kIntrinsicType_Float2 )
		{
			if( numColumns == 2 ) { return kIntrinsicType_Float2x2; }
			if( numColumns == 3 ) { return kIntrinsicType_Float2x3; }
			if( numColumns == 4 ) { return kIntrinsicType_Float2x4; }
		}
		else if( aggregateType == kIntrinsicType_Float3 )
		{
			if( numColumns == 2 ) { return kIntrinsicType_Float3x2; }
			if( numColumns == 3 ) { return kIntrinsicType_Float3x3; }
			if( numColumns == 4 ) { return kIntrinsicType_Float3x4; }
		}
		else if( aggregateType == kIntrinsicType_Float4 )
		{
			if( numColumns == 2 ) { return kIntrinsicType_Float4x2; }
			if( numColumns == 3 ) { return kIntrinsicType_Float4x3; }
			if( numColumns == 4 ) { return kIntrinsicType_Float4x4; }
		}
		ycAssert( false );
		return 0;
	}

	u32 TransposeMatrixType( const u32 type )
	{
		ycAssert( IsMatrixType( type ) );
		switch( type )
		{
		case kIntrinsicType_Float2x3: return kIntrinsicType_Float3x2;
		case kIntrinsicType_Float2x4: return kIntrinsicType_Float4x2;
		case kIntrinsicType_Float3x2: return kIntrinsicType_Float2x3;
		case kIntrinsicType_Float3x4: return kIntrinsicType_Float4x3;
		case kIntrinsicType_Float4x2: return kIntrinsicType_Float2x4;
		case kIntrinsicType_Float4x3: return kIntrinsicType_Float3x4;
		default: return type;
		}
	}

} // namespace ycShaderBuilder
