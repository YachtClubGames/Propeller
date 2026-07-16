#pragma once

/*! \namespace ycGeo
Helper functions for operating on shape data
*/

#include "ycVec2.h"
#include "ycVec3.h"

struct ycAABB;
struct ycBox;
struct ycCapsule;
struct ycCircle;
struct ycConeRay;
struct ycFrustum;
struct ycLine;
struct ycLineSeg;
struct ycPlane;
struct ycRay;
struct ycShape;
struct ycShapeContainer;
struct ycSphere;
struct ycTri;

struct ycGeoRayTriHit
{
	ycVec3 pos;    // intersection point
	ycVec3 normal; // triangle normal (unnormalized!)
	ycVec3 uvw;    // barycentric co-ordinates at hit
	f32 t;         // distance along ray
	u32 triIndex;  // index of triangle within owner
};

namespace ycGeo
{

	//////////////////////////
	// Closest points //
	//////////////////////////
	void ClosestPointAABB( const ycAABB& aabb, const ycVec3& pt, ycVec3* ptOnAABB );
	void ClosestPointSphere( const ycSphere& sphere, const ycVec3& pt, ycVec3* ptOnSphere );
	void ClosestPointPlane( const ycPlane& plane, const ycVec3& pt, ycVec3* ptOnPlane );
	void ClosestPointCircle( const ycCircle& circle, const ycVec3 & pt, ycVec3* ptOnCircle );
	void ClosestPointCapsule( const ycCapsule& capsule, const ycVec3& pt, ycVec3* ptOnCapsule );
	void ClosestPointTri( const ycTri& tri, const ycVec3& pt, ycVec3* ptOnTri );
	void ClosestPointTriLineSeg( const ycTri& tri, const ycLineSeg & lineSeg, ycVec3* ptOnTri, ycVec3 * closestOnLine = nullptr );
	void ClosestPointTriLineSegToEdge( const ycTri& tri, const ycLineSeg & lineSeg, ycVec3* ptOnTri, f32 * lineDist = nullptr );
	void ClosestPointTriRayToEdge( const ycTri& tri, const ycRay & ray, ycVec3* ptOnTri, f32 * rayDist = nullptr );
	void ClosestPointRay( const ycRay& ray, const ycVec3& pt, ycVec3* ptOnRay );
	void ClosestPointLine( const ycLine& line, const ycVec3& pt, ycVec3* ptOnLine );
	bool ClosestPointLineLine( const ycVec3& p0, const ycVec3& d0, const ycVec3& p1, const ycVec3& d1, ycVec2* hit );
	void ClosestPointLineSeg( const ycLineSeg & lineSeg, const ycVec3& pt, ycVec3 * potOnLineSeg );
	void ClosestPointLineSeg( const ycLineSeg & lineSeg, const ycVec3& pt, f32 * point );
	void ClosestPointPlaneLineSeg( const ycPlane& plane, const ycLineSeg & lineSeg, ycVec3 * potOnLineSeg );
	void ClosestPointBox( const ycMtx4& box, const ycVec3& pt, ycVec3* ptOnBox );
	
	void ClosestPointsLineLine( const ycLine& a, const ycLine& b, ycVec3* ptOnA, ycVec3* ptOnB, f32* distA = nullptr, f32* distB = nullptr );
	void ClosestPointsLineSegLineSeg( const ycLineSeg & lineSegA, const ycLineSeg & lineSegB, ycVec3 * pointOnLineSegA, ycVec3 * pointOnLineSegB, f32 * distA = nullptr, f32 * distB = nullptr );
	bool ClosestPointsLineRay( const ycLine& line, const ycRay& ray, ycVec3* ptOnLine, ycVec3* ptOnRay );
	void ClosestPointsLineSegRay( const ycLineSeg& seg, const ycRay& ray, ycVec3* ptOnSeg, ycVec3* ptOnRay );
	void ClosestPointsRayRay( const ycRay& a, const ycRay& b, ycVec3* ptOnA, ycVec3* ptOnB );
	ycVec2 ClosestPointsRayRay( const ycRay& a, const ycRay& b );
	bool ClosestPointsRayLine( const ycRay& ray, const ycLine& line, ycVec3* ptOnRay, ycVec3* ptOnLine );
	void ClosestPointsSphereSphere( const ycSphere& a, const ycSphere& b, ycVec3* cpOnA, ycVec3* cpOnB );

	/////////////////////////
	// Distances //
	/////////////////////////
	f32 DistSqLineSegLineSeg( const ycLineSeg&, const ycLineSeg& );
	f32 DistLineSegLineSeg( const ycLineSeg&, const ycLineSeg& );

	f32 DistSqLineLine( const ycLine&, const ycLine& );
	f32 DistLineLine( const ycLine&, const ycLine& );

	f32 DistSqLineRay( const ycLine&, const ycRay& );
	f32 DistLineRay( const ycLine&, const ycRay& );
	
	f32 DistSqLineSegPoint( const ycLineSeg&, const ycVec3& );
	f32 DistLineSegPoint( const ycLineSeg&, const ycVec3& );

	f32 DistSqLinePoint( const ycLine&, const ycVec3& );
	f32 DistLinePoint( const ycLine&, const ycVec3& );

	f32 DistSqRayLine( const ycRay&, const ycLine& );
	f32 DistRayLine( const ycRay&, const ycLine& );
	f32 DistSqRayRay( const ycRay&, const ycRay& );
	f32 DistRayRay( const ycRay&, const ycRay& );
	f32 DistSqRayPoint( const ycRay&, const ycVec3& );
	f32 DistRayPoint( const ycRay&, const ycVec3& );

	f32 DistSqSphereSphere( const ycSphere&, const ycSphere& );
	f32 DistSphereSphere( const ycSphere&, const ycSphere& );
	f32 DistSqSpherePoint( const ycSphere&, const ycVec3& );
	f32 DistSpherePoint( const ycSphere&, const ycVec3& );
	f32 DistSqSphereLineSeg( const ycSphere&, const ycLineSeg& );
	f32 DistSphereLineSeg( const ycSphere&, const ycLineSeg& );
	f32 DistSqSphereTri( const ycSphere&, const ycTri& );
	f32 DistSphereTri( const ycSphere&, const ycTri& );

	f32 DistSqAABBPoint( const ycAABB&, const ycVec3& );
	f32 DistAABBPoint( const ycAABB&, const ycVec3& );
	f32 DistSqAABBSphere( const ycAABB&, const ycSphere& );
	f32 DistAABBSphere( const ycAABB&, const ycSphere& );
	f32 DistAABBPlane( const ycAABB&, const ycPlane& );
	f32 DistSqTriPoint( const ycTri&, const ycVec3& );
	f32 DistTriPoint( const ycTri&, const ycVec3& );
	f32 DistSqCirclePoint( const ycCircle&, const ycVec3& );
	f32 DistCirclePoint( const ycCircle&, const ycVec3& );

	f32 DistPlanePoint( const ycPlane&, const ycVec3& );
	f32 DistPlaneLineSeg( const ycPlane&, const ycLineSeg& );
	f32 DistPlaneSphere( const ycPlane&, const ycSphere& );
	f32 DistPlaneCircle( const ycPlane&, const ycCircle& );

	f32	DistSqCapsulePoint( const ycCapsule& capsule, const ycVec3 & pt );
	f32	DistCapsulePoint( const ycCapsule& capsule, const ycVec3 & pt );
	f32	DistSqCapsuleLineSeg( const ycCapsule& capsule, const ycLineSeg & lineSeg );
	f32	DistCapsuleLineSeg( const ycCapsule& capsule, const ycLineSeg & lineSeg );
	f32	DistSqCapsuleCapsule( const ycCapsule& capsule, const ycCapsule & other );
	f32	DistCapsuleCapsule( const ycCapsule& capsule, const ycCapsule & other );
	f32	DistSqCapsuleSphere( const ycCapsule& capsule, const ycSphere & sphere );
	f32	DistCapsuleSphere( const ycCapsule& capsule, const ycSphere & sphere );
	f32	DistCapsulePlane( const ycCapsule& capsule, const ycPlane & plane );

	/////////////////////////////
	// Intersections //
	/////////////////////////////
	bool IntersectsPlaneAABB( const ycPlane&, const ycAABB& );
	bool IntersectsPlaneSphere( const ycPlane&, const ycSphere& );
	bool IntersectsPlaneTri( const ycPlane&, const ycTri& );
	bool IntersectsPlanePlane( const ycPlane&, const ycPlane& );

	bool IntersectsSphereAABB( const ycSphere&, const ycAABB& );
	bool IntersectsSphereSphere( const ycSphere&, const ycSphere& );
	bool IntersectsSpherePlane( const ycSphere&, const ycPlane& );
	bool IntersectsSphereTri( const ycSphere&, const ycTri& );
	bool IntersectsSphereFrustum( const ycSphere&, const ycFrustum& );

	bool IntersectsAABBAABB( const ycAABB&, const ycAABB& );
	bool IntersectsAABBSphere( const ycAABB&, const ycSphere& );
	bool IntersectsAABBPlane( const ycAABB&, const ycPlane& );
	bool IntersectsAABBTri( const ycAABB&, const ycTri& );
	bool IntersectsAABBLineSeg( const ycAABB&, const ycLineSeg & );
	bool IntersectsAABBRay( const ycAABB&, const ycRay &, f32 maxT );
	bool IntersectsAABBFrustum( const ycAABB&, const ycFrustum& );

	bool IntersectsBoxBox( const ycBox&, const ycBox& );
	bool IntersectionSweptBoxBox( f32* hitT, const ycBox& sweepBox, const ycVec3& sweepDir, f32 maxT, const ycBox& aabb );
	bool IntersectionInvSweptBoxBox( f32* hitT, const ycBox& sweepBox, const ycVec3& sweepInvDir, f32 maxT, const ycBox& aabb );

	bool IntersectsTriTri( const ycTri&, const ycTri& );
	bool IntersectsTriLineSeg( const ycTri&, const ycLineSeg & );

	u32	 IntersectsCirclePlane( const ycCircle & circle, const ycPlane & plane );
	u32  IntersectsCircleCircle( const ycCircle & circle, const ycCircle & other );
	u32  IntersectsCircleLineSeg( const ycCircle & circle, const ycLineSeg & lineSeg );
	u32  IntersectsCircleRay( const ycCircle & circle, const ycRay& ray );
	bool IntersectsCircleSphere( const ycCircle & circle, const ycSphere & sphere );
	
	bool IntersectsRaySphere( const ycRay&, const ycSphere& );
	bool IntersectsLineSegSphere( const ycLineSeg&, const ycSphere& );
	bool IntersectsLineSegLineSegXY( const ycLineSeg&, const ycLineSeg & );

	bool IntersectsCapsuleCapsule( const ycCapsule & capsule, const ycCapsule & other );
	bool IntersectsCapsuleSphere( const ycCapsule & capsule, const ycSphere & sphere );
	bool IntersectsCapsuleLineSeg( const ycCapsule & capsule, const ycLineSeg & lineSeg );
	bool IntersectsCapsulePlane( const ycCapsule & capsule, const ycPlane & plane );

	enum
	{
		kIntersectionFlag_2D = 1 << 0
	};
	bool Intersects( const ycShapeContainer&, const ycVec3& posA, const ycShapeContainer&, const ycVec3& posB, u32 flags = 0 );
	bool Intersects( u32 shapeAType, const ycShape& shapeA, const ycVec3& posA, u32 shapeBType, const ycShape& shapeB, const ycVec3& posB, u32 flags = 0 );

	// Intersects an infinite ray (capped at maxT) with a shape.
	// On hit, returns true and fills in 'hit/hitT'.
	// (pass maxT == 1.0f for segment, maxT == YC_F32_MAX for infinite ray)
	bool IntersectionInvRayBox( f32* hitT, const ycVec3& rayStart, const ycVec3& rayInvDir, f32 maxT, const ycBox& );
	bool IntersectionRayAABB( f32* hitT, const ycRay&, f32 maxT, const ycAABB& );
	bool IntersectionRaySphere( f32* hitT, const ycRay&, f32 maxT, const ycSphere& );
	bool IntersectionRaySphere( f32* hitT, f32* hitT2, const ycRay&, f32 maxT, const ycSphere& );
	bool IntersectionRayPlane( f32* hitT, const ycRay&, f32 maxT, const ycPlane& );
	bool IntersectionRayTri( ycGeoRayTriHit* hit, const ycRay&, f32 maxT, const ycTri& );
	bool IntersectionRayRect( const ycRay & ray, const ycVec3& ctr, const ycVec3& ext1, const ycVec3& ext2, f32* dist = nullptr, ycVec2* local = nullptr );
	bool IntersectionRayCuboid( const ycRay & ray, const ycMtx4& cuboid, f32* dist = nullptr, ycVec3* normal = nullptr );
	bool IntersectionRayPlane( const ycRay & ray, const ycVec3 center, const ycVec3 normal, ycVec3* point = nullptr, f32* dist = nullptr ); //! plane
	bool IntersectionRayDisc( const ycRay & ray, const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point = nullptr, f32* dist = nullptr ); //! disc
	//fixme: not an accurate torus intersection
	bool IntersectionRayTorus( const ycRay & ray, const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point = nullptr, f32* dist = nullptr ); //! disc
	bool IntersectionRayCone( const ycRay & ray, const ycVec3& bottom, const ycVec3& tip, f32 radiusBottom, f32 radiusTip, f32* dist = nullptr ); 
	bool IntersectionRayLineXZ( const ycRay & ray, const ycVec3& A, const ycVec3& B, ycVec2* dist = nullptr ); 

	bool IntersectionConeRayRing( const ycConeRay & ray, const ycVec3 center, const ycVec3 normal, f32 radius, ycVec3 *closest, f32* dist );
	bool IntersectionConeRayPoint( const ycConeRay & ray, const ycVec3 pos, f32* dist = nullptr );
	bool IntersectionConeRayLine( const ycConeRay & ray, const ycVec3 start, const ycVec3 end, f32* dist = nullptr );

	//! Returns false if the geometry created by the intersection is not a point.
	bool IntersectionPlanePlanePlane( ycVec3* pt_col, const ycPlane&, const ycPlane&, const ycPlane& );
	//! returns false and a garbage ln_out if the geometry created by the intersection is not a line
	bool IntersectionPlanePlane( ycLine* ln_out, const ycPlane&, const ycPlane& );

	bool IntersectionLinePlane( ycVec3* pt_col, const ycLine&, const ycPlane& );
	bool IntersectionLineAABB( ycVec3* pt_col, const ycLine&, const ycAABB& );
	bool IntersectionLineTri( ycVec3* pt_col, const ycLine&, const ycTri& );
	bool IntersectionLineSphere( ycVec3* pt_col, const ycLine&, const ycSphere& );
	bool IntersectionLineLine( ycVec3* pt_col, const ycLine&, const ycLine& );
	bool IntersectionLineRay( ycVec3* pt_col, const ycLine&, const ycRay& );

	bool IntersectionLineSegPlane( f32* hitT, const ycLineSeg&, const ycPlane& );
	bool IntersectionLineSegSphere( f32* hitT, const ycLineSeg&, const ycSphere& );
	bool IntersectionLineSegTri( ycGeoRayTriHit* hit, const ycLineSeg&, const ycTri& );
	bool IntersectionLineSegLineSegXY( ycVec3* pt_col, f32 * fraction, const ycLineSeg&, const ycLineSeg& );

	u32	IntersectionCirclePlane( const ycCircle & circle, const ycPlane & plane, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr );
	u32 IntersectionCircleLineSeg( const ycCircle & circle, const ycLineSeg & lineSeg, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr );
	u32 IntersectionCircleRay( const ycCircle & circle, const ycRay& ray, ycVec3 * intersection1 = nullptr, ycVec3 * intersection2 = nullptr );

	////////////////////
	// Sweeps //
	////////////////////
	bool SweepAABBAABB( f32 * fraction, ycVec3 * normal, const ycAABB & a, const ycLineSeg & sweepA, const ycAABB & b, const ycLineSeg & sweepB );
	bool SweepRayAABB( f32 * fraction, ycVec3 * normal, const ycRay & seg, const ycAABB & b, const ycLineSeg & sweepB );
	bool SweepLineSegAABB( f32 * fraction, ycVec3 * normal, const ycLineSeg & seg, const ycAABB & b, const ycLineSeg & sweepB );
	bool SweepLineSegSphere( f32 * fraction, ycVec3 * normal, const ycLineSeg & seg, const ycSphere & b, const ycLineSeg & sweepB );
	bool SweepLineSegTri( f32 * fraction, ycVec3 * normal, const ycLineSeg & seg, const ycTri & b, const ycLineSeg & sweepB );
	bool SweepAABBTri( f32 * fraction, ycVec3 * normal, const ycAABB & a, const ycLineSeg & sweepA, const ycTri & b, const ycLineSeg & sweepB );
	bool SweepLineSegLineSegXY( f32 * fraction, ycVec3 * normal, const ycLineSeg & a, const ycLineSeg & b );
	bool SweepSphereTri( ycGeoRayTriHit* hit, const ycSphere& sphere, const ycVec3& sweep, f32 maxT, const ycTri& tri );
	bool SweepAABBSphere( f32 * fraction, ycVec3 * normal, const ycAABB & a, const ycLineSeg & sweepA, const ycSphere & b, const ycLineSeg & sweepB );
	bool SweepSphereSphere( f32 * fraction, ycVec3 * normal, const ycSphere & a, const ycLineSeg & sweepA, const ycSphere & b, const ycLineSeg & sweepB );
	bool Sweep( f32 * fraction, ycVec3 * normal, const ycShapeContainer&, const ycLineSeg & sweepA, const ycShapeContainer&, const ycLineSeg & sweepB, u32 flags = 0  );
	bool Sweep( f32 * fraction, ycVec3 * normal, u32 shapeAType, const ycShape& shapeA, const ycLineSeg & sweepA, u32 shapeBType, const ycShape& shapeB, const ycLineSeg & sweepB, u32 flags = 0 );
	
	//////////////
	// CONTAINS //
	//////////////
	bool ContainsAABBPt( const ycAABB&, const ycVec3& );
	bool ContainsAABBAABB( const ycAABB&, const ycAABB& );
	bool ContainsLineSegPt( const ycLineSeg&, const ycVec3& );
	bool ContainsTriPt( const ycTri&, const ycVec3& );
	bool ContainsSpherePt( const ycSphere&, const ycVec3& );
	bool ContainsPlanePt( const ycPlane&, const ycVec3& );
	bool ContainsCirclePt( const ycCircle&, const ycVec3& );
	bool ContainsFrustumPt( const ycFrustum&, const ycVec3& );
	bool ContainsCapsulePt( const ycCapsule&, const ycVec3& );
	bool ContainsBoxPt( const ycMtx4& box, const ycVec3& );
	//even-odd rule for polygon
	bool ContainsEORPolygonPt( const ycVec2 * polyPoints, u32 ptCount, const ycVec3& );

	//////////////
	// CONTAINS //
	//////////////

	void ToAABB( u32 shapeAType, const ycShape& shapeA, ycAABB * out );

	//////////////
	// Local vertex //
	//////////////
	ycVec3 GetLocalVertex(  const u32 shapeAType, const ycShape& shapeA, const ycVec3 & dir, f32 margin );

	//////////////
	// Area //
	//////////////
	f32 SignedArea( const ycVec2* pts, u32 ptsCount );
	f32 Area( const ycVec2* pts, u32 ptsCount );

	void SetTriMatchAxis( bool match );
};
