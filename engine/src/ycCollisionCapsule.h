#pragma once

#include "ycMtx4.h"

struct ycCollisionCapsule
{
	ycMtx4 forward;
	ycMtx4 inverse;
	ycVec3 pts[2];
	f32 radius;
};
