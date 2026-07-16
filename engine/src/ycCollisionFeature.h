#pragma once

#include "ycVec3.h"
#include "ycLineSeg.h"
#include "ycCollisionTypes.h"

struct ycCollisionFeature
{
	ycCollision::FeatureType type = ycCollision::kFeature_Invalid;

	f32 radius = 0.0f;
	ycVec3 pts[ 4 ];

	bool IsFace() const { return type >= ycCollision::kFeature_FaceTri; }
	static ycCollisionFeature Vertex( const ycVec3& vert );
	static ycCollisionFeature Edge( const ycVec3& a, const ycVec3& b, f32 radius = 0.0f );
	static ycCollisionFeature FaceTri( const ycVec3& a, const ycVec3& b, const ycVec3& c );
	static ycCollisionFeature FaceQuad( const ycVec3& a, const ycVec3& b, const ycVec3& c, const ycVec3& d );
	static ycCollisionFeature FaceDisc( const ycVec3& center, f32 radius );

	static inline u32 GetNumVertsForType( ycCollision::FeatureType t )
	{
		static_assert( ycCollision::kFeature_Vertex == 1, "this function depends on type being the num of verts" );
		static_assert( ycCollision::kFeature_Edge == 2, "this function depends on type being the num of verts" );
		static_assert( ycCollision::kFeature_FaceTri == 3, "this function depends on type being the num of verts" );
		static_assert( ycCollision::kFeature_FaceQuad == 4, "this function depends on type being the num of verts" );
		return t;
	}

	ycLineSeg GetEdge( u32 ind ) { return ycLineSeg( pts[ind], pts[( ind + 1 ) % GetNumVertsForType(type)] ); }
};
