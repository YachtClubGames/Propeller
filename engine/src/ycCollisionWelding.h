#pragma once

/*
Welding runs after main collision and removes spurious edges so that a mesh behaves like a single surface rather than a bunch of triangles
*/

#include "ycCollision.h"
#include "ycCollisionResult.h"

namespace ycCollisionWelding
{
	// Sorts and converts a list of the internal Result structs to an output ycCollisionContactList.
	// Optionally also runs welding, which is here because it goes in between the sorting and the output and can possibly save time by short-circuiting
	ureg WeldAndOutput( ycCollisionWeldableResults& toWeld, const ycVec3& shapeCenter, const ycCollisionOptions& opt, const ycVec3& sweepMove, ycCollisionSweepOptions::SweepResultsBehavior reportAllContacts, ycCollisionContactListBase& output );
};
