#pragma once

/*! \class ycCPP

A lightweight C preprocessor that lets you query/change the processor environment. For example you can parse a file
then inspect the definitions that were encountered.

It should not be able to crash Parse(), but syntax checking is lax. This preprocessor does not adhere to the full c
preprocessing specification. Some advanced features may not work, and non-standard things may parse.

*/

#include "ycCommon.h"

// core
#include "ycChainedHash.h"
#include "ycStringRef.h"
#include "ycVector.h"
class ycAllocator;
class ycByteBuffer;

// error handling, forgive me
#include <setjmp.h>

class ycCPP
{
public:

	ycCPP( ycAllocator* mem = nullptr );
	void Reset();
	~ycCPP();

	/*! Parse input and emit converted output
	- Returns false if an error occured, use GetError() to retrieve the error message
	*/
	bool Parse( const char* in, ycByteBuffer* out );
	bool Parse( const char* in, const ycSize_t len, ycByteBuffer* out );
	bool Parse( ycStringRef in, ycByteBuffer* out );
	inline const char* GetError() const { return m_errorMsg; }

	/*! Define a macro
	- 'def' can be complex, for example Define( "FOO(x) (x+1)" )
	*/
	void Define( const char* def );
	void Define( const char* def, const ycSize_t len );
	void Define( ycStringRef def );
	void Define( ycStringRef* def ); //!< advances the 'def' start ptr as it parses

	void DefineNoValue( const ycStringRef key ); // for simple assignment *with no value only*! 'key' should be an identifier!
	void DefineSimple( const ycStringRef key, const ycStringRef value ); // for simple assignment only! #define key value

	void Undef( const ycStringRef key );

	/*! Evaluate a condition
	- This is the clause that occurs after an #if or #elif
	- If you're going to be using this a lot, you should use the scratch buffer variant, and re-use the same buffer.
	*/
	bool EvaluateCondition( const char* expr );
	bool EvaluateCondition( const char* expr, const ycSize_t len );
	bool EvaluateCondition( ycStringRef expr );
	bool EvaluateCondition( ycStringRef expr, ycByteBuffer* scratch );
	
	s32 EvaluateIntExpression( ycStringRef expr );
	s32 EvaluateIntExpression( ycStringRef expr, ycByteBuffer* scratch );

	//! Interpolates macro values into plain text; does not parse directives
	void Interpolate( ycStringRef line, ycByteBuffer* out );

	void SetIncludePath( const ycStringRef path ); //!< path is not copied!

protected:

	ycAllocator* m_mem;
	ureg         m_ignoreDepth; // how many levels deep into a negative condition are we?
	bool         m_trueSeen;    // for tracking elif
	ycStringRef  m_includePath;
	struct Include
	{
		ycStringRef path; // points at the start of the #include "path, may not be a usable file path!
		const char* fileData;
		ycSize_t    fileSize;
		bool		isPragmaOnce;
	};
	ycVector< Include > m_includes;

	/*
	Macros are broken into a list of into a list of interpolants; an interpolant can either be copied text, or the value
	of a positional argument.
	*/

	struct Interpolant
	{
		const char*  start; // if start is nullptr, then length indicates the index of a positional argument
		union { ureg length, argIdx; };
		Interpolant* next;
	};
	Interpolant* AllocInterpolant();
	void FreeInterpolant( Interpolant* ptr );

	struct Macro
	{
		uint32_t    numArgs;
		bool        isFunctional;
		bool        hasValue;
		bool        unused[2];
		Interpolant firstInterpolant; // if hasValue == false, this is *not initialized*
	};
	void FreeInterpolants( Macro* macro );
	ycChainedHash< ycHashKey_t, Macro > m_macros;

	// set the error state and either halt the Parse(), or assert
	#ifdef YC_MSVC // structure was padded due to alignment specifier
		__pragma( warning( push ) )
		__pragma( warning( disable: 4324 ) )
	#endif
	jmp_buf m_jmpBuf;
	#ifdef YC_MSVC
		__pragma( warning( pop ) )
	#endif
	char*   m_errorMsg;
	ureg    m_inParse; // some functions can occur outside of Parse(), let Error() know to just assert instead
	enum { kErrBufSize = 4096 };
	void Error( const char* showCurPos, const char* fmt, ... );

	// the internal algorithm that evaluates conditional expressions
	enum
	{
		kOpStackSize     = 128, // to meet standards we may want this much bigger, and to put these on the heap; you'd have to craft one ugly expression to break out of this though
		kOutputQueueSize = kOpStackSize // doesn't have to match, but w/e
	};
	struct EvalCtx
	{
		uint32_t stack[     kOpStackSize ]; // smaller type might be cache advantageous
		int32_t  queue[ kOutputQueueSize ];
		ureg     stackSize;
		ureg     queueSize;
		bool     expectUnary; // used to distinguish between unary and infix + and -
	};
	void ApplyOperator( const ycStringRef& pos, const uint32_t op, EvalCtx* ctx ); // pos just used for error reporting
	void PushOperator( const ycStringRef& pos, const uint32_t op, EvalCtx* ctx ); // pos just used for error reporting
	void EvalCondParseLoop( ycStringRef* in, void* pCtx, ycByteBuffer* scratch );
	void ParseShortCircuit( ycStringRef* in );
	
	// interpolates the value of a macro; includes parsing (args)
	void InterpolateMacro( Macro* macro, ycStringRef* in, ycByteBuffer* out ); // 'in' is right after the macro's name

	// skips tabs, spaces, \r, \n, both single and multi-line comments
	bool SkipWhiteSpaceAndComments( ycStringRef* src, const bool required = false );
	
	// skips tabs, spaces, both single and multi-line comments
	// this DOES skip over newlines in multi-line comments!
	bool SkipHorizWhiteSpaceAndComments( ycStringRef* src, const bool required = false );

	// skip "string literals"
	void SkipQuotedString( ycStringRef* src );

	// skip 'c' haracter literals
	void SkipQuotedChar( ycStringRef* src );
};
