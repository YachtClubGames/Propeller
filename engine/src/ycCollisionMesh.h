#pragma once

#include "ycMtx4.h"

struct ycCollisionMeshInfo;

struct ycCollisionMesh
{
	ycMtx4 worldToLocal;
	ycMtx4 localToWorld;
	ycCollisionMeshInfo* mesh;
};
