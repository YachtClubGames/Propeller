#pragma once

#include "ycCompilerConfig.h"
#include "ycVec3.h"

struct ycAABB;
struct ycLine;
struct ycLineSeg;
struct ycPlane;
struct ycRay;
struct ycShape;
struct ycShapeContainer;
struct ycSphere;
struct ycTri;

enum ShapeType : s32
{
	kShapeType_None,
	kShapeType_LineSeg,
	kShapeType_Ray,
	kShapeType_Line,
	kShapeType_AABB,
	kShapeType_Box,
	kShapeType_Capsule,
	kShapeType_Circle,
	kShapeType_Sphere,
	kShapeType_Tri,
	kShapeType_Plane,
	kShapeType_Cone,
	kShapeType_Cylinder,
	kShapeType_Ellipsoid,
	kShapeType_Frustum,
	kShapeType_Rectangle,
	kShapeType_Bezier,
	kShapeType_Count
};

struct ycShape //%
{
	// has no members. just here for convenience in casting
};

// convenince struct
struct ycShapeContainer
{
public:

	ycShapeContainer() {}
	ycShapeContainer( ShapeType shapeType, ycShape * shape ) { m_shapeType = shapeType; m_shape = shape; }
	ShapeType GetShapeType() { return m_shapeType; }
	ycShape* GetShape() { return m_shape; }
	ShapeType GetShapeType() const { return m_shapeType; }
	ycShape* GetShape() const { return m_shape; }

	ycVec3 GetLocalVertex( const ycVec3 & dir, f32 margin ) const;
	ycAABB GetBound() const;

protected:

	ShapeType m_shapeType;
	ycShape * m_shape;
};
