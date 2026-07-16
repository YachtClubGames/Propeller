#pragma once

#include "ycCollisionFeature.h"
#include "ycCollisionMaterialIndex.h"
#include "ycCollisionTypes.h"
#include "ycVector.h"

class ycCollisionComponent;
struct ycCollisionResult;
struct ycCollisionContactPair;
enum ycCollisionMask : u64;

struct ycCollisionFaceResult  // smaller struct that only keeps info about a Face hit
{
	bool valid = false;
	bool isBackface = false;
	ycCollisionFeature featureB;
	ycVec3 supportingA;
	ycVec3 supportingB;
	ycVec3 dir;
	f32 mag;

	ycCollisionFaceResult() {}
	ycCollisionFaceResult( const ycCollisionResult& r );
};
	
// Result of a SAT check, heavyweight container with enough info to run welding
struct ycCollisionResult
{
	explicit operator bool() const { return valid; }

	bool valid = false;
	bool canWeld = true;
	bool welded = false;
	bool isBackface = false;
	bool matchAxisOkay = false;
	ycCollision::Convexity convexity = ycCollision::kConvexity_Convex;
	ycCollisionFeature featureA;
	ycCollisionFeature featureB;
	ycCollision::PrimInfo primB;
	ycVec3 pointB;

	ycVec3 supportingA;
	ycVec3 supportingB;

	ycCollisionFaceResult faceB;   // extra info about face to be used by welding

	// separate dir and mag to help with precision (in a lot of cases dir is known to high
	// precision but mag is small - multiplying them yields uncertainty on dir)
	ycVec3 dir;
	f32 mag;

	// for edges and verts, the most "upright" (closest to a supplied up vector) dir that's
	// within the bounds defined by the adjacent face normals
	ycVec3 uprightDir;

	// for edges/verts, we can provide angle info (generated in welding, which has a step that
	// gathers adjacent normals for the edge
	//f32 adjacentEdgeNormalsDotA = 1.0f;
	f32 adjacentEdgeNormalsDotB = 1.0f;

	f32 sweepDist = 0.0f;
	f32 fullSweepDist = 1.0f;

	ycCollision::ShapeType typeB = ycCollision::kShape_Empty;
	ycCollisionMask contactMaskB;
	ycCollisionMaterialIndex matIndexB;
	ycCollisionComponent* ownerB = nullptr;

	void SetPointA( const ycVec3 ptA ) { pointB = ptA + dir * mag; }
	void SetPointB( const ycVec3 ptB ) { pointB = ptB; }
	ycVec3 GetPointA() const { return pointB - dir * mag; }
	ycVec3 GetPointB() const { return pointB; }

	void WriteToContact( ycCollisionContactPair* contact );
};

inline ycCollisionFaceResult::ycCollisionFaceResult( const ycCollisionResult& r )
	: valid( r.valid )
	, isBackface( r.isBackface )
	, featureB( r.featureB )
	, supportingA( r.supportingA )
	, supportingB( r.supportingB )
	, dir( r.dir )
	, mag( r.mag )
{}

typedef ycStackVector< ycCollisionResult, 24 > ycCollisionWeldableResults;
