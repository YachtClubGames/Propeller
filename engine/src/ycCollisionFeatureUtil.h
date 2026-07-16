#pragma once

/*
\file ycCollisionFeatureUtil

Utilities for building or analyzing the features (point, edge, face) on collision shapes

*/

#include "ycVec3.h"

struct ycCollisionFeature;
struct ycCollisionResult;
struct ycCollisionFaceResult;

namespace ycCollisionFeatureUtil
{
	ycVec3 GetFaceCenter( const ycCollisionFeature& face );

	bool PointInFace( const ycVec3& vv, const ycCollisionResult& face );
	bool EdgeCoplanarFace( const ycCollisionResult& edge, const ycCollisionResult& face );
	bool EdgesCollinear( const ycVec3& p0, const ycVec3& p1, const ycVec3& p2, const ycVec3& p3 );

	bool FacesCoplanar( const ycCollisionFaceResult& faceA, const ycCollisionFaceResult& faceB );
	bool FaceBelowFace( const ycCollisionResult& faceBelow, const ycCollisionResult& faceAbove );
	bool ResultBelowFace( const ycCollisionResult& faceBelow, const ycCollisionResult& faceAbove );
	bool FaceContainsVert( const ycCollisionFeature& face, const ycCollisionFeature& vert );
	bool FaceMatchesVert( const ycCollisionFeature& face, const ycCollisionFeature& vert );
	bool FaceCircumscribesPoint( const ycCollisionFeature& face, const ycVec3& faceNormal, const ycVec3& vv );

	u32 FaceContainsEdge( const ycCollisionFeature& face, const ycCollisionFeature& edge );
	u32 FaceMatchesEdge( const ycCollisionFeature& face, const ycCollisionFeature& edge );
	
	u32 EdgeIsSharedByFace( const ycVec3& e0, const ycVec3& e1, const ycCollisionFeature& face );
	bool FaceSharesEdgesWithFace( const ycCollisionFeature& faceA, const ycCollisionFeature& faceB );

	// based on features, chooses a good point for the collision to return
	void SetFinalPoint( ycCollisionResult& mtd );
};

