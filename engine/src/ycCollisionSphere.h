#pragma once

#include "ycMtx4.h"

struct ycCollisionSphere
{
	ycMtx4 forward;
	ycMtx4 inverse;
	f32 radius;
};
