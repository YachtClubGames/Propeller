#pragma once

//////////
// Implementation switches based on which platform we're building for
//////////

#include "ycCommon.h"

// Threading, Atomics, SIMD

#if defined( YC_NIX ) || defined( YC_OSX ) // should probably default to this?
	#define YC_PTHREAD
#endif

#if defined YC_X86
	#define YC_SIMD_SSE
#elif defined YC_ARM
	#define YC_SIMD_NEON
#else
	#define YC_SIMD_SOFTWARE
#endif

// Rendering

#ifndef YC_HEADLESS
	#if YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX
		#define YC_RENDERER_VK
	#elif defined YC_OSX
		//#ifndef YC_RENDERER_METAL
		//	#if defined YC_EDITOR
				#define YC_RENDERER_METAL
		//	#else
		//		#define YC_RENDERER_VK,
		//		//#define YC_RENDERER_METAL
		//	#endif
		//#endif
	#elif YC_NDA
	#else
		//#define YC_RENDERER_VK
		//#define YC_RENDERER_DX11
		#define YC_RENDERER_DX12
	#endif
	#if defined YC_DEBUG || defined YC_EDITOR
		#define YC_ENABLE_SHADER_RELOAD
	#endif
#endif // !YC_HEADLESS

#ifdef YC_RENDERER_VK
	// dynamically load vulkan via volk
	#define YC_USE_VOLK
#endif

// Window Management and Entry Point

#ifndef YC_HEADLESS
	#if YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#else
		#define YC_SDL_WINDOWING
		#if !defined YC_NIX && !defined YC_MAC
			#define YC_SDL_MAIN // YC_HEADLESS is not directly relevant... for the moment let's assume YC_HEADLESS means 'no sdl'
		#endif
	#endif
#endif // !YC_HEADLESS

#if YC_NDA
#else
	#define YC_MAIN_EXIT( code ) return code
#endif

// Input

#ifndef YC_HEADLESS
	#if YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#else
		#define YC_INPUT_SDL
		#define YC_INPUT_SDL_USE_EVENT_CONTROLLERS // enable new wip event-driven controller stuff
		#if defined YC_WIN32
			#define YC_RGB_RAZER
			#define YC_RGB_LOGITECH
		#endif
		#ifdef YC_OSX
			#define YC_SWAP_COMMAND_AND_CTRL
		#endif
	#endif
#endif // !YC_HEADLESS

// File System

#if defined YC_MSFT
	#define YC_FILESYSTEM_WIN32
#elif YC_NDA
#elif YC_NDA
#elif YC_NDA
#else
	#define YC_FILESYSTEM_CSTD
#endif

// Sound

#ifndef YC_HEADLESS
	#if YC_NDA
	#elif YC_NDA
	#elif YC_NDA
	#else
		#define YC_AUDIO_SDL
	#endif
#endif // !YC_HEADLESS
