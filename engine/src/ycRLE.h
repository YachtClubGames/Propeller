#pragma once

#include "ycCommon.h"

/*! \class ycRLE
A very basic compression/decompression algorithm that may suit a handful of spots that prefer simplicity.
*/

ycSize_t ycRLE_encode( const u8* in, const ycSize_t inSize, u8* out );

ycSize_t ycRLE_decode( const u8* in, u8* out );
