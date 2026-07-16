#pragma once

// some excess headers, but this is only included from cpp files
#include "ycCollisionTypes.h"
#include "ycCollisionBox.h"
#include "ycCollisionMesh.h"
#include "ycCollisionSphere.h"
#include "ycCollisionCapsule.h"
#include "ycCollisionCylinder.h"

class ycCollisionComponent;
class ycCollisionWorld;
struct ycColor;
class ycDrawUtil;

struct ycCollisionShape
{
	ycCollisionWorld* GetWorld() const;

	ycCollision::ShapeType type = ycCollision::kShape_Empty;
	ycCollisionComponent* component = nullptr;
	union
	{
		ycCollisionBox box;
		ycCollisionMesh mesh;
		ycCollisionSphere sphere;
		ycCollisionCapsule capsule;
		ycCollisionCylinder cylinder;
	};

	void AddOffset( const ycVec3& offset );        // world translation
	void AddRigidTransform( const ycMtx4& delta ); // world translation + rotation only
	void Draw( ycDrawUtil* draw, const ycColor& color );
};
