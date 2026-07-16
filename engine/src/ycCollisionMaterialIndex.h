#pragma once

#include "ycCommon.h"

struct ycCollisionMaterialIndex //%
{
yceditable:
	u32 index = 0;

public:
	bool operator==( const ycCollisionMaterialIndex& rhs ) { return index == rhs.index; }
};

//mainly used for ui purposes
struct ycCollisionMaterialIndexHolder //%
{
yceditable:
	ycCollisionMaterialIndex index;
};
