#pragma once

#include "ycCompilerConfig.h"

#define YC_COLLISION_USE_SKIPS

#define YC_COLLISION_INVALID_NODE		(0x7fffffffUL)

#define YC_COLLISION_INVALID_SORTCODE	(0xFFFFFFFFUL)

#ifdef YC_ENABLE_ASSERT
//#define YC_SKIP_LIST_STATS
#endif

class ycCollisionComponent;

namespace ycCollision
{
	enum ShapeType : u32 
	{
		kShape_Empty = 0,
		kShape_Box,
		kShape_Mesh,
		kShape_Sphere,
		kShape_Capsule,
		kShape_Cylinder
	};

	// must match order of debug menu items
	enum DrawMode : u8
	{
		kDrawMode_Off,
		kDrawMode_Wire,
		kDrawMode_Alpha,
		kDrawMode_Solid,
		kDrawMode_Checker,
		kDrawMode_WireChecker,

		kDrawMode_Count
	};

	enum FeatureType : s8
	{
		kFeature_Invalid = 0,
		kFeature_Vertex = 1,
		kFeature_Edge = 2,
		kFeature_FaceTri = 3,
		kFeature_FaceQuad = 4,
		kFeature_FaceDisc
	};
		
	enum Convexity : u8
	{
		kConvexity_Convex = 0,
		kConvexity_Concave = 1,
		kConvexity_Flat = 2,
		kConvexity_Multiple = 3   // a vertex may have adjacent tris that are both above and below by the normal
	};

	struct PrimInfo
	{
		// Primitive-specific contact information:
		union {
			struct { u32 triIndex; } mesh;
		};
	};
}

// Shapes can be excluded from a query by returning false from this callback:
typedef bool ycCollisionShapeFilterFn( ycCollisionComponent* coll, void* userdata );
struct ycCollisionShapeFilterInfo
{
	ycCollisionShapeFilterFn* m_filter;
	void* m_userdata;
};
