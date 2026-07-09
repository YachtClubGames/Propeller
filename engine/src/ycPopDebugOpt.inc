#ifdef YC_ENABLE_DEBUG_OPT
	#if defined YC_MSVC && !defined YC_CLANG
		#pragma optimize( "", on ) // revert to default optimizations specified by cmd line /O, see https://docs.microsoft.com/en-us/cpp/preprocessor/optimize?view=msvc-160
		#pragma runtime_checks( "", restore )
	#endif
#endif
