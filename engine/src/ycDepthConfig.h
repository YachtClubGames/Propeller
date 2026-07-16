#pragma once

#include "ycCompilerConfig.h"

namespace ycDepthConfig
{
	// Whether to put the near plane at 1.0, aka "reversed depth"
	constexpr bool IS_DEPTH_REVERSED = true;

	constexpr f32 ReverseDepthValue( f32 depthValue )
	{
		if( IS_DEPTH_REVERSED )
		{
			return 1.0f - depthValue;
		}
		else
		{
			return depthValue;
		}
	}

	constexpr f32 CLIP_SPACE_Z_NEAR = ReverseDepthValue( 0.0f );
	constexpr f32 CLIP_SPACE_Z_FAR = ReverseDepthValue( 1.0f );
};
