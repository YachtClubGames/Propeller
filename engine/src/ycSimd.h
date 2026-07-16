#pragma once

// core
#include "ycCommon.h"
#include "ycPlatformConfig.h"
#include "ycStd.h"

// sdk
#if defined YC_SIMD_SSE
	// currently compiling with SSE 4.2 on Windows
	#if defined YC_MSVC
		//#include <intrin.h>
		#define _INC_MALLOC // prevent xmmintrin from pulling in malloc.. msft why?
	#elif defined YC_NIX
		#include <smmintrin.h>
	#elif YC_NDA
	#elif YC_NDA
	#endif
	#include <xmmintrin.h>
	typedef __m128 ycSimd_t;
	#if defined YC_MSVC
		#undef _INC_MALLOC
	#endif
#elif defined YC_SIMD_NEON
	#include <arm_neon.h>
	typedef float32x4_t ycSimd_t;
#elif defined YC_SIMD_SOFTWARE
	#include "ycVec4.h"
	typedef ycVec4 ycSimd_t;
#else
	#error ycPlatformConfig.h should define a SIMD implementation
#endif

#ifdef YC_MSVC
	#define YC_SIMD_INLINE static __forceinline // doesnt really force inlining :(
#else
	#define YC_SIMD_INLINE static inline __attribute__((__always_inline__, __nodebug__))
#endif

#if defined YC_SIMD_SOFTWARE
	#define ycSimd_Set( x, y, z, w ) ycVec4( x, y, z, w )
	#define ycSimd_Load( ptr )       ycVec4( (ptr)[0], (ptr)[1], (ptr)[2], (ptr)[3] )
	#define ycSimd_Store( ptr, val ) ( (ptr)[0] = (val).x, (ptr)[1] = (val).y, (ptr)[2] = (val).z, (ptr)[3] = (val).w )
	#define ycSimd_Splat( xyzw )     ycVec4( xyzw )
	YC_SIMD_INLINE ycSimd_t ycSimd_SplatBits( const u32 x ) { return ycVec4( *( (f32*)&x ) ); }
	#define ycSimd_SplatX( src )     ycVec4( (src).x )
	#define ycSimd_SplatY( src )     ycVec4( (src).y )
	#define ycSimd_SplatZ( src )     ycVec4( (src).z )
	#define ycSimd_SplatW( src )     ycVec4( (src).w )
	#define ycSimd_Add( a, b )       ( (a) + (b) )
	#define ycSimd_Sub( a, b )       ( (a) - (b) )
	#define ycSimd_Mul( a, b )       (a).Mul( b )
	#define ycSimd_Div( a, b )       ( (a) / (b) )
	#define ycSimd_Min( a, b )       ycVec4::Min( a, b )
	#define ycSimd_Max( a, b )       ycVec4::Max( a, b )
	#define ycSimd_BitAnd( a, b)     ycFatal()
	#define ycSimd_BitOr( a, b )     ycFatal()
	#define ycSimd_BitXor( a, b )    ycFatal()
	#define ycSimd_GetX( a )         (a).x
	#define ycSimd_GetY( a )         (a).y
	#define ycSimd_GetZ( a )         (a).z
	#define ycSimd_GetW( a )         (a).w
	#define ycSimd_Less( a, b )      ycFatal()
	#define ycSimd_Greater( a, b )   ycFatal()
	#define ycSimd_LessEq( a, b )    ycFatal()
	#define ycSimd_GreaterEq( a, b ) ycFatal()
	#define ycSimd_AllFalse( a )     ycFatal()
	#define ycSimd_AnyTrue( a )      ycFatal()
#elif defined YC_SIMD_SSE
	#define ycSimd_Set( x, y, z, w ) _mm_set_ps( w, z, y, x )
	#define ycSimd_Load( ptr )       _mm_loadu_ps( ptr )
	#define ycSimd_Store( ptr, val ) _mm_storeu_ps( ptr, val )
	#define ycSimd_Splat( xyzw )     _mm_set1_ps( xyzw )
	YC_SIMD_INLINE ycSimd_t ycSimd_SplatBits( const u32 x ) { return _mm_load1_ps( (f32*)&x ); }
	#define ycSimd_SplatX( src )     _mm_shuffle_ps( src, src, _MM_SHUFFLE(0,0,0,0) )
	#define ycSimd_SplatY( src )     _mm_shuffle_ps( src, src, _MM_SHUFFLE(1,1,1,1) )
	#define ycSimd_SplatZ( src )     _mm_shuffle_ps( src, src, _MM_SHUFFLE(2,2,2,2) )
	#define ycSimd_SplatW( src )     _mm_shuffle_ps( src, src, _MM_SHUFFLE(3,3,3,3) )
	#define ycSimd_Add( a, b )       _mm_add_ps( a, b )
	#define ycSimd_Sub( a, b )       _mm_sub_ps( a, b )
	#define ycSimd_Mul( a, b )       _mm_mul_ps( a, b )
	#define ycSimd_Div( a, b )       _mm_div_ps( a, b )
	#define ycSimd_Min( a, b )       _mm_min_ps( a, b )
	#define ycSimd_Max( a, b )       _mm_max_ps( a, b )
	#define ycSimd_BitAnd( a, b )    _mm_and_ps( a, b )
	#define ycSimd_BitOr( a, b )     _mm_or_ps( a, b )
	#define ycSimd_BitXor( a, b )    _mm_xor_ps( a, b )
	#define ycSimd_GetX( a )         _mm_cvtss_f32( _mm_shuffle_ps( a, a, _MM_SHUFFLE(0,0,0,0) ) )
	#define ycSimd_GetY( a )         _mm_cvtss_f32( _mm_shuffle_ps( a, a, _MM_SHUFFLE(0,0,0,1) ) )
	#define ycSimd_GetZ( a )         _mm_cvtss_f32( _mm_shuffle_ps( a, a, _MM_SHUFFLE(0,0,0,2) ) )
	#define ycSimd_GetW( a )         _mm_cvtss_f32( _mm_shuffle_ps( a, a, _MM_SHUFFLE(0,0,0,3) ) )
	#define ycSimd_Less( a, b )      _mm_cmplt_ps( a, b )
	#define ycSimd_Greater( a, b )   _mm_cmpgt_ps( a, b )
	#define ycSimd_LessEq( a, b )    _mm_cmple_ps( a, b )
	#define ycSimd_GreaterEq( a, b ) _mm_cmpge_ps( a, b )
	#define ycSimd_AllFalse( a )     (_mm_movemask_epi8(_mm_castps_si128(a)) == 0)
	#define ycSimd_AnyTrue( a )      (_mm_movemask_epi8(_mm_castps_si128(a)) != 0)
#elif defined YC_SIMD_NEON
	YC_SIMD_INLINE ycSimd_t ycSimd_Set( const f32 x, const f32 y, const f32 z, const f32 w ) { const f32 init[ 4 ] = { x, y, z, w }; return vld1q_f32( init ); }
	#define ycSimd_Splat( xyzw )     vdupq_n_f32( xyzw )
	#define ycSimd_Load( ptr )       vld1q_f32( ptr )
	#define ycSimd_Store( ptr, val ) vst1q_f32( ptr, val )
	#define ycSimd_SplatBits( x )    vreinterpretq_f32_u32( vdupq_n_u32( x ) )
	#define ycSimd_SplatX( src )     vdupq_laneq_f32( src, 0 )
	#define ycSimd_SplatY( src )     vdupq_laneq_f32( src, 1 )
	#define ycSimd_SplatZ( src )     vdupq_laneq_f32( src, 2 )
	#define ycSimd_SplatW( src )     vdupq_laneq_f32( src, 3 )
	#define ycSimd_Add( a, b )       vaddq_f32( a, b )
	#define ycSimd_Sub( a, b )       vsubq_f32( a, b )
	#define ycSimd_Mul( a, b )       vmulq_f32( a, b )
	#define ycSimd_Div( a, b )       vdivq_f32( a, b )
	#define ycSimd_Min( a, b )       vminq_f32( a, b )
	#define ycSimd_Max( a, b )       vmaxq_f32( a, b )
	#define ycSimd_BitAnd( a, b )    vreinterpretq_f32_u32( vandq_u32( vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b) ) )
	#define ycSimd_BitOr( a, b )     vreinterpretq_f32_u32( vorrq_u32( vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b) ) )
	#define ycSimd_BitXor( a, b )    vreinterpretq_f32_u32( veorq_u32( vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b) ) )
	#define ycSimd_GetX( a )         vgetq_lane_f32( a, 0 )
	#define ycSimd_GetY( a )         vgetq_lane_f32( a, 1 )
	#define ycSimd_GetZ( a )         vgetq_lane_f32( a, 2 )
	#define ycSimd_GetW( a )         vgetq_lane_f32( a, 3 )
	#define ycSimd_Less( a, b )      vcltq_f32( a, b )
	#define ycSimd_Greater( a, b )   vcgtq_f32( a, b )
	#define ycSimd_LessEq( a, b )    vcleq_f32( a, b )
	#define ycSimd_GreaterEq( a, b ) vcgeq_f32( a, b )
	#define ycSimd_AllFalse( a )     (vreinterpret_u64_u32(vqmovn_u64(vreinterpretq_u64_f32(a)))[0] == 0)
	#define ycSimd_AnyTrue( a )      (vreinterpret_u64_u32(vqmovn_u64(vreinterpretq_u64_f32(a)))[0] != 0)
#endif

#define ycSimd_Clamp( src, min, max ) ycSimd_Max( ycSimd_Min( src, max ), min )
