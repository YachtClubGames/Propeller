#pragma once

#include "ycCollision.h"
#include "ycString.h"
#include "ycCollisionShape.h"

namespace ycCollisionTest
{
	enum Capture 
	{
		kCapture_EndAfterOne = 1 << 0,

		kCapture_SweepHits = 1 << 1,
		kCapture_Contact = 1 << 2,
		kCapture_Entry = 1 << 3,
	};

	void StartCapture( u32 bits );
	void EndCapture( u32 bits );

	bool ShouldCapture( Capture bits );

	ycString MakeLiteral( const f32& ff, ycStringRef name );
	ycString MakeLiteral( const ycVec3& vec, ycStringRef name );
	ycString MakeLiteral( const ycTri& tri, ycStringRef name );
	ycString MakeLiteral( const ycMtx4& box, ycStringRef name );
	ycString MakeLiteral( const ycAABB& box, ycStringRef name );
	ycString MakeLiteral( const ycSphere& tri, ycStringRef name );
	ycString MakeLiteral( const ycCollisionBox& box, ycStringRef name );
	ycString MakeLiteral( const ycCollisionSphere& sphere, ycStringRef name );
	ycString MakeLiteral( const ycCapsule& capsule, ycStringRef name );
	ycString MakeLiteral( const ycCollisionCapsule& capsule, ycStringRef name );
	ycString MakeLiteral( const ycCollisionCylinder& cylinder, ycStringRef name );

	ycString MakeLiteral( const ycCollisionOptions& opt, ycStringRef name );
	ycString MakeLiteral( const ycCollisionSweepOptions&, ycStringRef name );

	ycString TestMtd( const ycCollisionResult& mtd, ycStringRef mtdName );
};
