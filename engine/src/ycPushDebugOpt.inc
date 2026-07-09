#ifdef YC_ENABLE_DEBUG_OPT
	#if defined YC_MSVC && !defined YC_CLANG
		#pragma optimize( "t", on )
		#pragma optimize( "g", on )
		#pragma runtime_checks( "", off )
	#endif
#endif
