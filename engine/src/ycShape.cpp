#include "ycShape.h"

#include "ycGeo.h"
#include "ycAABB.h"

ycVec3 ycShapeContainer::GetLocalVertex( const ycVec3 & dir, f32 margin ) const
{
	return ycGeo::GetLocalVertex( m_shapeType, *m_shape, dir, margin );
}

ycAABB ycShapeContainer::GetBound() const
{
	ycAABB out;
	ycGeo::ToAABB( m_shapeType, *m_shape, &out );
	return out;
}
