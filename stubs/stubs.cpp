
/*
Stubbed functions to get things compiling
*/

#include "ycCommon.h"

// ycDebug

extern "C"
void _ycDebugAssertMsg( const char* , const char* , const u32, const char* , ... )
{
}

extern "C"
void ycLog( const char* , ... )
{
}

#if defined YC_MSVC

#include <Windows.h>

extern "C"
int _ycIsDebuggerPresent()
{
	return ::IsDebuggerPresent() ? YC_TRUE : YC_FALSE;
}

extern "C"
void _ycDebugAbort()
{
	__debugbreak();
}

#endif // YC_MSVC

// ycData

struct ycStartupEntry* g_dataMetaStartup;

#include "ycDataSchema.h"

void ycData::Schema::SetConstructor( RecordDef*, Constructor* )
{
}

struct ycMtx4;
struct ycQuat;

namespace ycData
{
	RecordDef* GetRecordDef( ycMtx4* ) { return nullptr; }
	RecordDef* GetRecordDef( ycQuat* ) { return nullptr; }
}

// main

int main( int, char** )
{
	return 0;
}
