#pragma once

#include "ycCompilerConfig.h"

#define YC_FILEMANAGER_ENABLE_LOOSE_PAKS
#define YC_FILEMANAGER_ENABLE_LOOSE_FILES
#define YC_ENABLE_ASSERT
#define YC_ALLOW_GAME_DEBUG
#define YC_TOOL_NETWORKING
#define YC_ENABLE_CONSOLE
#define YC_ENABLE_BUILDERS
#define YC_ENABLE_META_STRINGS

#if defined YC_ENABLE_ASSERT
	// defines that arent directly tied to asserts, but generally are enabled together
	#define YC_ENABLE_LOG
	#define YC_ENABLE_DEBUG_STRINGS
#endif
