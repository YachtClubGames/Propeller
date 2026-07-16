#pragma once

#include "ycMtx4.h"

struct ycCollisionBox
{
	// 32 floats
	ycMtx4 inverse;
	ycMtx4 forward;
	ycVec3 invExtents; // the half extent size of the unrotated box
};
