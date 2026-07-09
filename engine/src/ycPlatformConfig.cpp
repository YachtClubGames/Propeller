#include "ycPlatformConfig.h"

#if defined YC_WIN32
	#pragma comment( lib, "Winmm.lib" )
	#pragma comment( lib, "Version.lib" )
	#pragma comment( lib, "Imm32.lib" )
	#pragma comment( lib, "shlwapi.lib" )
#endif

#if YC_NDA
#endif

#if YC_NDA
#endif

#if defined YC_WIN32 && defined YC_RENDERER_GL
	#pragma comment( lib, "OpenGL32.lib" )
#endif

#if defined YC_WIN32 && defined YC_RENDERER_VK && !defined YC_USE_VOLK
	#ifdef YC_64
		#pragma comment( lib, "engine/3rdparty/vulkan/lib64/vulkan-1.lib" )
	#else
		#pragma comment( lib, "engine/3rdparty/vulkan/lib32/vulkan-1.lib" )
	#endif
#endif

#if defined YC_WIN32 && defined YC_RENDERER_DX11
	#pragma comment( lib, "d3d11.lib" )
	#pragma comment( lib, "dxguid.lib") // SetPrivateData( WKPDID_D3DDebugObjectName )
	#pragma comment( lib, "dxgi.lib")
#endif

#if defined YC_WIN32 && defined YC_RENDERER_DX12
	#pragma comment( lib, "d3d12.lib" )
	#pragma comment( lib, "dxguid.lib") // SetPrivateData( WKPDID_D3DDebugObjectName )
	#pragma comment( lib, "dxgi.lib")
#endif

#if defined YC_WIN32 && ( defined YC_SDL_WINDOWING || defined YC_INPUT_SDL )
	#pragma comment( lib, "Setupapi.lib" )
	#ifdef YC_64
		#if defined YC_DEBUG && !defined YC_DEBUG_OPT
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/x64/Debug/SDL2.lib" )
		#else
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/x64/Release/SDL2.lib" )
		#endif
	#else 
		#if defined YC_DEBUG && !defined YC_DEBUG_OPT
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/Win32/Debug/SDL2.lib" )
		#else
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/Win32/Release/SDL2.lib" )
		#endif
	#endif
#endif

#if defined YC_SDL_MAIN 
	#ifdef YC_64
		#if defined YC_DEBUG && !defined YC_DEBUG_OPT
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/x64/Debug/SDL2main.lib" )
		#else
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/x64/Release/SDL2main.lib" )
		#endif
	#else
		#if defined YC_DEBUG && !defined YC_DEBUG_OPT
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/Win32/Debug/SDL2main.lib" )
		#else
			#pragma comment( lib, "engine/3rdparty/SDL2/VisualC/Win32/Release/SDL2main.lib" )
		#endif
	#endif
#endif

#if YC_NDA
#endif

#if YC_NDA
#endif
