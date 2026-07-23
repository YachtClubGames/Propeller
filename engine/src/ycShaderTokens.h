#pragma once
#ifndef YC_SHADERTOKENS_H
#define YC_SHADERTOKENS_H

#include "ycCommon.h"

// core
#include "ycStringRef.h"

namespace ycShaderBuilder
{

	enum
	{
		kIntrinsicType_Void = 0,

		// singular types are grouped (no member access on this range!!!)
		kIntrinsicType_Float,
		kIntrinsicType_Int,
		kIntrinsicType_UInt,
		kIntrinsicType_Bool,

		kIntrinsicType_Begin_Scalars = kIntrinsicType_Float,
		kIntrinsicType_End_Scalars   = kIntrinsicType_Bool,

		// aggregate types are grouped (custom member access logic, swizzling, etc)
		// update GetIntrinsicTypeNumComponents() if adding here!
		kIntrinsicType_Float1,
		kIntrinsicType_Float2,
		kIntrinsicType_Float3,
		kIntrinsicType_Float4,

		kIntrinsicType_Int1,
		kIntrinsicType_Int2,
		kIntrinsicType_Int3,
		kIntrinsicType_Int4,

		kIntrinsicType_UInt1,
		kIntrinsicType_UInt2,
		kIntrinsicType_UInt3,
		kIntrinsicType_UInt4,

		kIntrinsicType_Bool1,
		kIntrinsicType_Bool2,
		kIntrinsicType_Bool3,
		kIntrinsicType_Bool4,

		kIntrinsicType_Begin_Aggregates = kIntrinsicType_Float1,
		kIntrinsicType_End_Aggregates   = kIntrinsicType_Bool4,

		// matrix types are grouped (array access)
		kIntrinsicType_Float2x2,
		kIntrinsicType_Float2x3,
		kIntrinsicType_Float2x4,

		kIntrinsicType_Float3x2,
		kIntrinsicType_Float3x3,
		kIntrinsicType_Float3x4,

		kIntrinsicType_Float4x2,
		kIntrinsicType_Float4x3,
		kIntrinsicType_Float4x4,

		kIntrinsicType_Begin_Matrices = kIntrinsicType_Float2x2,
		kIntrinsicType_End_Matrices   = kIntrinsicType_Float4x4,
		
		kIntrinsicType_Tex2D,
		kIntrinsicType_Tex3D,
		kIntrinsicType_TexCube,
		kIntrinsicType_Tex2DUint,
		kIntrinsicType_Tex2DMS,

		kIntrinsicType_Begin_Textures = kIntrinsicType_Tex2D,
		kIntrinsicType_End_Textures	  = kIntrinsicType_Tex2DMS,

		kIntrinsicType_Begin_Opaque = kIntrinsicType_Begin_Textures,
		kIntrinsicType_End_Opaque   = kIntrinsicType_End_Textures,

		kIntrinsicType_End = kIntrinsicType_End_Opaque,
		kIntrinsicType_Count
/* for now just gonna use 'void', we don't currently need a separate 'invalid' indicator yet (although we may need invalid/unknown later)
		,
		kIntrinsicType_Invalid = 0xffff // used in the parser if something has no valid type in a given context (not the same as void, indicates an operation is completely invalid, rather than just being nothing)
		                                // not -1, this is packed into a bit field so let's not worry about sign extension
*/
	};

	// 0:8      token (base) type
	// 8:16     (optional) token sub-type
	// 16:20    (optional) infix operator precedence
	// 20:24    (optional) unary operator precedence
	// 24:28    (optional) return type (primarily used to indicate operators that return bool)
	// stored directly in the type to avoid the need for LUTS/indirection during parsing (they might be faster though, so try 'em later)
	enum TokenType : u32
	{	//                          Token Type        sub-type  infix precedence  unary precedence  return type
		kToken_Keyword             = 1,  
			kToken_Keyword_Typedef = kToken_Keyword | ( 1<<8),
			kToken_Keyword_Struct  = kToken_Keyword | ( 2<<8),
			kToken_Keyword_If      = kToken_Keyword | ( 3<<8),
			kToken_Keyword_Else    = kToken_Keyword | ( 4<<8),
			kToken_Keyword_For     = kToken_Keyword | ( 5<<8),
			kToken_Keyword_Return  = kToken_Keyword | ( 6<<8),
			kToken_Keyword_CBuffer = kToken_Keyword | (10<<8),
			kToken_Keyword_const   = kToken_Keyword | (11<<8),
			//kToken_Keyword_in      = kToken_Keyword | (12<<8),
			kToken_Keyword_out     = kToken_Keyword | (12<<8),
			kToken_Keyword_inout   = kToken_Keyword | (13<<8),
			kToken_Keyword_asm     = kToken_Keyword | (14<<8),
		kToken_Literal             = 2,
		kToken_ConstantString      = 3,
		kToken_TypeName            = 4,
		kToken_Identifier          = 5,
		kToken_Op                  = 6,
			kToken_Op_Semicolon    = kToken_Op      | ( 1<<8),
			kToken_Op_Comma        = kToken_Op      | ( 2<<8),
			kToken_Op_Dot          = kToken_Op      | ( 3<<8),
			kToken_Op_Assign       = kToken_Op      | ( 4<<8) | ( 2<<16),
			kToken_Op_AddAssign    = kToken_Op      | ( 5<<8) | ( 2<<16),
			kToken_Op_SubAssign    = kToken_Op      | ( 6<<8) | ( 2<<16),
			kToken_Op_MulAssign    = kToken_Op      | ( 7<<8) | ( 2<<16),
			kToken_Op_DivAssign    = kToken_Op      | ( 8<<8) | ( 2<<16),
			kToken_Op_ModAssign    = kToken_Op      | ( 9<<8) | ( 2<<16),
			kToken_Op_LShiftAssign = kToken_Op      | (10<<8) | ( 2<<16),
			kToken_Op_RShiftAssign = kToken_Op      | (11<<8) | ( 2<<16),
			kToken_Op_AndAssign    = kToken_Op      | (12<<8) | ( 2<<16),
			kToken_Op_XorAssign    = kToken_Op      | (13<<8) | ( 2<<16),
			kToken_Op_OrAssign     = kToken_Op      | (14<<8) | ( 2<<16),
			kToken_Op_LogicalOr    = kToken_Op      | (15<<8) | ( 4<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_LogicalAnd   = kToken_Op      | (16<<8) | ( 5<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_BitwiseOr    = kToken_Op      | (17<<8) | ( 6<<16),
			kToken_Op_BitwiseXor   = kToken_Op      | (18<<8) | ( 7<<16),
			kToken_Op_BitwiseAnd   = kToken_Op      | (19<<8) | ( 8<<16),
			kToken_Op_Equals       = kToken_Op      | (20<<8) | ( 9<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_NEquals      = kToken_Op      | (21<<8) | ( 9<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_LEquals      = kToken_Op      | (22<<8) | (10<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_GEquals      = kToken_Op      | (23<<8) | (10<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_Less         = kToken_Op      | (24<<8) | (10<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_Greater      = kToken_Op      | (25<<8) | (10<<16)            | (kIntrinsicType_Bool<<24),
			kToken_Op_LeftShift    = kToken_Op      | (26<<8) | (11<<16),
			kToken_Op_RightShift   = kToken_Op      | (27<<8) | (11<<16),
			kToken_Op_Plus         = kToken_Op      | (28<<8) | (12<<16) | (14<<20),
			kToken_Op_Dash         = kToken_Op      | (29<<8) | (12<<16) | (14<<20),
			kToken_Op_Not          = kToken_Op      | (30<<8) |        0 | (14<<20) | (kIntrinsicType_Bool<<24),
			kToken_Op_BitwiseNot   = kToken_Op      | (31<<8) |        0 | (14<<20),
			kToken_Op_Inc          = kToken_Op      | (32<<8) |        0 | (14<<20),
			kToken_Op_Dec          = kToken_Op      | (33<<8) |        0 | (14<<20),
			kToken_Op_Star         = kToken_Op      | (34<<8) | (13<<16),
			kToken_Op_Slash        = kToken_Op      | (35<<8) | (13<<16),
			kToken_Op_Mod          = kToken_Op      | (36<<8) | (13<<16),
			kToken_Op_Colon        = kToken_Op      | (37<<8),
		kToken_BeginBlock          = 7,  // {
		kToken_EndBlock            = 8,  // }
		kToken_LeftParen           = 9,  // (
		kToken_RightParen          = 10, // )
		kToken_LeftBracket         = 11, // [
		kToken_RightBracket        = 12, // ]

		kToken_EOF = 0
	};

	const char* GetTokenName( TokenType token );
	
	inline ureg GetTokenInfixOpPrecedence( ureg token ) { return ((token&0x0f0000)>>16); }
	inline ureg GetTokenUnaryOpPrecedence( ureg token ) { return ((token&0xf00000)>>20); }

	inline u32 GetTokenInfixOpValType( u32 token ) { return ((token&0xf000000)>>24); }
	inline u32 GetTokenUnaryOpValType( u32 token ) { return ((token&0xf000000)>>24); }

	struct TokenDesc
	{
		TokenType   type; // would like to uint32_t this but atm it's helpful in the debugger if it knows the type so it can show the enum value names
		union // extra data depending on token type
		{
			u32 extData;
			u32 identifierIndex; // for identifiers and type names; note: identifiers and type names do *not* share an index-space
			u32 typeIndex;
			u32 literalType; // uses the kIntrinsicType_ stuff below, note that things like kIntrinsicType_Float4 will not be used, only singular types
		};
		ycStringRef str;
	};

	inline bool IsScalarType(    const u32 type ) { return type >= kIntrinsicType_Begin_Scalars    && type <= kIntrinsicType_End_Scalars; }
	inline bool IsAggregateType( const u32 type ) { return type >= kIntrinsicType_Begin_Aggregates && type <= kIntrinsicType_End_Aggregates; }
	inline bool IsMatrixType(    const u32 type ) { return type >= kIntrinsicType_Begin_Matrices   && type <= kIntrinsicType_End_Matrices; }
	inline bool IsTextureType(   const u32 type ) { return type >= kIntrinsicType_Begin_Textures   && type <= kIntrinsicType_End_Textures;}
	inline bool IsOpaqueType(    const u32 type ) { return type >= kIntrinsicType_Begin_Opaque     && type <= kIntrinsicType_End_Opaque; }
	inline bool IsUserType(      const u32 type ) { return type >  kIntrinsicType_End; }

	ureg GetIntrinsicTypeNumComponents( const u32 type ); //!< only valid for kIntrinsicType_Begin_Scalars <= x <= kIntrinsicType_End_Scalars || kIntrinsicType_Begin_Aggregates <= x <= kIntrinsicType_End_Aggregates; returns 0 otherwise
	ureg GetIntrinsicTypeNumColumns( const u32 type ); //!< only valid for matric types, returns 0 otherwise
	ureg GetIntrinsicTypeNumRows( const u32 type ); //!< only valid for matric types, returns 0 otherwise
	u32 GetAggregateScalarType( const u32 type ); //!< returns kIntrinsicType_Float for kIntrinsicType_Float3, etc
	u32 GetMatrixAggregateType( const u32 type ); //!< returns kIntrinsicType_Float3 for kIntrinsicType_Float3x3, etc
	u32 GetAggregateTypeForNumComponents( const u32 scalarType, const ureg numComponents ); //!< returns kIntrinsicType_Float3 given scalarType = kIntrinsicType_Float and numComponents = 3, etc
	u32 GetMatrixTypeForAggregateTypeAndNumColumns( const u32 aggregateType, const ureg numColumns ); //!< returns kIntrinsicType_Float3x2 given aggregateType = kIntrinsicType_Float3 and numColumns = 2, etc
	u32 TransposeMatrixType( const u32 type );

} // namespace ycShaderBuilder

#endif // !YC_SHADERTOKENS_H