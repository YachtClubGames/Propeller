#pragma once

/*
\class ycCollision

Core collision stuff

*/

#include "ycVec3.h"
#include "ycVec4.h"
#include "ycMtx4.h"
#include "ycBox.h"
#include "ycCollisionTypes.h"
#include "ycCollisionResult.h"
#include "ycCollisionOptions.h"

class ycCollisionComponent;
struct ycCollisionContact;
struct ycCollisionContactPair;
struct ycCollisionMeshInfo;
struct ycCollisionShape;
struct ycCollisionBox;
struct ycCollisionMesh;
struct ycCollisionSphere;
struct ycCollisionCapsule;
struct ycCollisionCylinder;
struct ycSphere;
struct ycLineSeg;
struct ycCylinder;
struct ycCollisionMaterialType;
struct ycCollisionMaterialTypes;
struct ycCollisionContactListBase;
class ycCollisionWorld;
enum ycCollisionMask : u64;

struct ycCollisionQueryIterator
{
	u32 m_idx;
	ycCollisionMask m_mask;
	ycBox m_bounds;
	const ycCollisionWorld* m_world;
	ycCollisionShapeFilterInfo* m_filter;
	bool m_queryRegion;
	u32 hits[4] = {};
	ycCollisionQueryIterator() : m_idx( 0 ), m_mask( (ycCollisionMask)0xFFFFFFFFFFFFFFFF ), m_bounds(), m_world( nullptr ), m_filter( nullptr ), m_queryRegion( true ) {}
	ycCollisionQueryIterator( u32 idx ) : m_idx( idx ), m_mask( (ycCollisionMask)0xFFFFFFFFFFFFFFFF ), m_bounds(), m_world( nullptr ), m_filter( nullptr ), m_queryRegion( true ) {}
	ycCollisionQueryIterator( const ycCollisionWorld* world, ycCollisionMask mask, const ycBox& bounds, ycCollisionShapeFilterInfo* filter );

	bool operator!= ( const ycCollisionQueryIterator& other ) const { return m_idx != other.m_idx; }
	ycCollisionShape* operator* () const;

	void operator++ ();

	static u32 FindStart( const ycCollisionWorld* world );
	static u32 FindFirst( const ycCollisionWorld* world, u32 idx );
	static u32 FindNext( const ycCollisionWorld* world, u32 idx );

private:
	static u32 FindStart( const ycCollisionWorld* world, ycCollisionMask mask, const ycBox& bounds, ycCollisionShapeFilterInfo* filter, u32 hits[4] );
	static u32 FindFirst( const ycCollisionWorld* world, u32 idx, ycCollisionMask mask, const ycBox& bounds, ycCollisionShapeFilterInfo* filter, u32 hits[4] );
	static u32 FindNext( const ycCollisionWorld* world, u32 idx, ycCollisionMask mask, const ycBox& bounds, ycCollisionShapeFilterInfo* filter, u32 hits[4] );
};

struct ycCollisionQueryData
{
	ycCollisionQueryIterator m_start;

	ycCollisionQueryIterator begin() const { return m_start; };
	ycCollisionQueryIterator end() const { return { 0x7fffffff }; }
};

namespace ycCollision
{
	void Init();
	void Shutdown();

	void InitUnsortedMask( ycCollisionMask mask );

	// Adds a new collision shape to the world. (fill in yourself afterwards)
	ycCollisionShape* Add( ycCollisionWorld* world, const ycBox& bounds, ycCollisionMask flags );

	// "Moves" a valid existing-in-a-world shape to a new world (swapping the passed-in pointer, since it's a new alloc)
	void ChangeWorld( ycCollisionShape** inout_shape, ycCollisionWorld* newWorld );

	// Removes a collision shape from the world.
	void Remove( ycCollisionShape* shape );

	// Moves an object to a new location.  if world is null, ycCollisionComponent on ycCollisionShape will be used to find world.
	void Move( ycCollisionShape* shape, const ycBox& bounds, ycCollisionWorld* world = nullptr );

	// Returns the bounds associated with a shape.
	ycBox GetBounds( const ycCollisionShape* shape, ycCollisionWorld* worldP = nullptr );

	// Moves a shape into a different category. (use 0 to effectively disable all collisions). if world is null, ycCollisionComponent on ycCollisionShape will be used to find world.
	// Returns the previous flags.
	ycCollisionMask ChangeMask( ycCollisionShape* shape, ycCollisionMask flags, ycCollisionWorld* world = nullptr );

	// Returns the flags associated with a shape.
	ycCollisionMask GetMask( const ycCollisionShape* shape, ycCollisionWorld* worldP = nullptr );

	ycCollisionMask GetCollideMask( ycCollisionMeshInfo * data, u32 idx );

	// Enumerates all shapes in the world that match the given flags.
	ycCollisionQueryData ForEach( const ycCollisionWorld* world, ycCollisionMask mask );

	// Enumerates all shapes in the world 
	ycCollisionQueryData ForEach( const ycCollisionWorld* world );

	// Enumerates all shapes the overlap a region.
	ycCollisionQueryData QueryRegion( const ycCollisionWorld* world, ycCollisionMask mask, const ycBox& bounds, ycCollisionShapeFilterInfo* filter = nullptr );

	// Helper for constructing box meshes.
	ycCollisionMesh GetMeshForCubePrimitive( const ycCollisionBox& box );

	// Helper for resolving multiple contacts into one. Use `stride` to handle ycCollisionContactPair subclasses.  (stride == 0 means sizeof(ycCollisionContactPair))
	ycVec3 ResolvePush( const ycCollisionContactPair* hits, ureg numHits, ureg stride = 0 );

	ycCollisionMaterialTypes* GetCollisionMaterialTypes();
	void EnableCollisionMaterialListener( ycCollisionWorld* world, bool enable );

	bool DefaultFilter( ycCollisionContactPair* contact, void* userdata );

	// Computes world-space AABB of a shape (without retrieving it from the world)
	ycBox ShapeComputeBounds( const ycCollisionShape& shape );
	void AxisProjection( const ycCollisionShape& shape, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );

	// ???-vs-???. Returns number of contacts generated.
	ureg ShapeCollide       ( const ycCollisionWorld* world, const ycCollisionShape& shape, const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg ShapeCollide       ( const ycCollisionShape& shape, const ycCollisionShape& shape2, const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	// Swept shape test.
	f32 ShapeSweep          ( const ycCollisionWorld* world, const ycCollisionShape& shape, const ycVec3& move,	const ycCollisionSweepOptions& opt, ycCollisionContactListBase& results );
	f32 ShapeSweep          ( const ycCollisionShape& shape, const ycCollisionShape& shape2, const ycVec3& move,	const ycCollisionSweepOptions& opt, ycCollisionContactListBase& results );

	// Swept sphere test.
	// Returns -1 if no hit, otherwise time of first intersection along sphere's path.
	f32 SphereSweep          ( const ycCollisionWorld* world, const ycSphere& sphere, const ycVec3& move,		const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	void SphereSweepShape    ( const ycSphere& sphere, const ycVec3& move, const ycCollisionShape& shape,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepBox      ( const ycSphere& sphere, const ycVec3& move, const ycCollisionShape& box,			const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepMesh     ( const ycSphere& sphere, const ycVec3& move, const ycCollisionShape& mesh,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepSphere   ( const ycSphere& sphere, const ycVec3& move, const ycCollisionShape& sphShape,	const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepCapsule  ( const ycSphere& sphere, const ycVec3& move, const ycCollisionShape& cap,			const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepCylinder ( const ycSphere& sphere, const ycVec3& move, const ycCollisionShape& cyl,			const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	
	// Swept sphere test (non-uniform scale)
	f32 SphereSweep          ( const ycCollisionWorld* world, const ycCollisionSphere& sphere, const ycVec3& move,		const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	void SphereSweepShape    ( const ycCollisionSphere& sphere, const ycVec3& move, const ycCollisionShape& shape,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepBox      ( const ycCollisionSphere& sphere, const ycVec3& move, const ycCollisionShape& box,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepMesh     ( const ycCollisionSphere& sphere, const ycVec3& move, const ycCollisionShape& mesh,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepSphere   ( const ycCollisionSphere& sphere, const ycVec3& move, const ycCollisionShape& sphShape,	const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepCapsule  ( const ycCollisionSphere& sphere, const ycVec3& move, const ycCollisionShape& cap,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void SphereSweepCylinder ( const ycCollisionSphere& sphere, const ycVec3& move, const ycCollisionShape& cyl,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );

	// Sphere-vs-???. Returns number of contacts generated.
	ureg SphereCollide         ( const ycCollisionWorld* world, const ycSphere& sphere,		const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg SphereCollideShape    ( const ycSphere& sphere, const ycCollisionShape& shape,     const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideBox      ( const ycSphere& sphere, const ycCollisionShape& box,       const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideMesh     ( const ycSphere& sphere, const ycCollisionShape& mesh,      const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideSphere   ( const ycSphere& sphere, const ycCollisionShape& sphShape,  const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideCapsule  ( const ycSphere& sphere, const ycCollisionShape& cylShape,  const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideCylinder ( const ycSphere& sphere, const ycCollisionShape& capShape,  const ycCollisionOptions& opt, ycCollisionWeldableResults& results );

	// Ellipsoid-vs-???. Returns number of contacts generated.
	ureg SphereCollide         ( const ycCollisionWorld* world, const ycCollisionSphere& sphere,	const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg SphereCollideShape    ( const ycCollisionSphere& sphere, const ycCollisionShape& shape,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideBox      ( const ycCollisionSphere& sphere, const ycCollisionShape& box,      const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideMesh     ( const ycCollisionSphere& sphere, const ycCollisionShape& mesh,     const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideSphere   ( const ycCollisionSphere& sphere, const ycCollisionShape& sphShape, const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideCapsule  ( const ycCollisionSphere& sphere, const ycCollisionShape& cylShape, const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg SphereCollideCylinder ( const ycCollisionSphere& sphere, const ycCollisionShape& capShape, const ycCollisionOptions& opt, ycCollisionWeldableResults& results );

	// Swept box test.
	f32 BoxSweep           ( const ycCollisionWorld* world, const ycMtx4& box, const ycVec3& move,		const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	void BoxSweepShape     ( const ycMtx4& box, const ycVec3& move, const ycCollisionShape& shape,      const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void BoxSweepBox       ( const ycMtx4& box, const ycVec3& move, const ycCollisionShape& boxB,       const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void BoxSweepMesh      ( const ycMtx4& box, const ycVec3& move, const ycCollisionShape& mesh,       const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void BoxSweepSphere    ( const ycMtx4& box, const ycVec3& move, const ycCollisionShape& sphShape,	const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void BoxSweepCapsule   ( const ycMtx4& box, const ycVec3& move, const ycCollisionShape& capShape,	const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void BoxSweepCylinder  ( const ycMtx4& box, const ycVec3& move, const ycCollisionShape& cylShape,	const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );

	// Box-vs-???. Returns number of contacts generated.
	ureg BoxCollide          ( const ycCollisionWorld* world, const ycMtx4& box,  const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg BoxCollideShape     ( const ycMtx4& box, const ycCollisionShape& shape,  const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg BoxCollideBox       ( const ycMtx4& box, const ycCollisionShape& boxB,   const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg BoxCollideMesh      ( const ycMtx4& box, const ycCollisionShape& mesh,   const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg BoxCollideSphere    ( const ycMtx4& box, const ycCollisionShape& sphere, const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg BoxCollideCapsule   ( const ycMtx4& box, const ycCollisionShape& cap,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg BoxCollideCylinder  ( const ycMtx4& box, const ycCollisionShape& cyl,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );

	// Swept capsule test.
	f32 CapsuleSweep          ( const ycCollisionWorld* world, const ycCollisionCapsule& cylinder, const ycVec3& move,	const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	void CapsuleSweepShape    ( const ycCollisionCapsule& cylinder, const ycVec3& move, const ycCollisionShape& shape,  const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CapsuleSweepBox      ( const ycCollisionCapsule& cylinder, const ycVec3& move, const ycCollisionShape& box,      const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CapsuleSweepMesh     ( const ycCollisionCapsule& cylinder, const ycVec3& move, const ycCollisionShape& mesh,    const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CapsuleSweepSphere   ( const ycCollisionCapsule& cylinder, const ycVec3& move, const ycCollisionShape& sph,   const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CapsuleSweepCapsule  ( const ycCollisionCapsule& cylinder, const ycVec3& move, const ycCollisionShape& cap,  const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CapsuleSweepCylinder ( const ycCollisionCapsule& cylinder, const ycVec3& move, const ycCollisionShape& cyl, const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );

	// Capsule-vs-???. Returns number of contacts generated.
	ureg CapsuleCollide         ( const ycCollisionWorld* world, const ycCollisionCapsule& capsule,	 const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg CapsuleCollideShape    ( const ycCollisionCapsule& capsule, const ycCollisionShape& b,      const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CapsuleCollideBox      ( const ycCollisionCapsule& capsule, const ycCollisionShape& box,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CapsuleCollideMesh     ( const ycCollisionCapsule& capsule, const ycCollisionShape& mesh,   const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CapsuleCollideSphere   ( const ycCollisionCapsule& capsule, const ycCollisionShape& sphere, const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CapsuleCollideCapsule  ( const ycCollisionCapsule& capsule, const ycCollisionShape& cap,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CapsuleCollideCylinder ( const ycCollisionCapsule& capsule, const ycCollisionShape& cyl,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );

	// Swept cylinder test.
	f32 CylinderSweep          ( const ycCollisionWorld* world, const ycCollisionCylinder& cylinder, const ycVec3& move,									const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	void CylinderSweepShape    ( const ycCollisionCylinder& cylinder, const ycVec3& move, const ycCollisionShape& shape,	const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CylinderSweepBox      ( const ycCollisionCylinder& cylinder, const ycVec3& move, const ycCollisionShape& box,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CylinderSweepMesh     ( const ycCollisionCylinder& cylinder, const ycVec3& move, const ycCollisionShape& mesh,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CylinderSweepSphere   ( const ycCollisionCylinder& cylinder, const ycVec3& move, const ycCollisionShape& sph,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CylinderSweepCapsule  ( const ycCollisionCylinder& cylinder, const ycVec3& move, const ycCollisionShape& cap,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );
	void CylinderSweepCylinder ( const ycCollisionCylinder& cylinder, const ycVec3& move, const ycCollisionShape& cyl,		const ycCollisionSweepOptions& opt, ycCollisionWeldableResults& results );

	// Cylinder-vs-???. Returns number of contacts generated.
	ureg CylinderCollide         ( const ycCollisionWorld* world, const ycCollisionCylinder& cylinder,	const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg CylinderCollideShape    ( const ycCollisionCylinder& cylinder, const ycCollisionShape& b,      const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CylinderCollideBox      ( const ycCollisionCylinder& cylinder, const ycCollisionShape& box,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CylinderCollideMesh     ( const ycCollisionCylinder& cylinder, const ycCollisionShape& mesh,   const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CylinderCollideSphere   ( const ycCollisionCylinder& cylinder, const ycCollisionShape& sphere, const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CylinderCollideCapsule  ( const ycCollisionCylinder& cylinder, const ycCollisionShape& cap,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );
	ureg CylinderCollideCylinder ( const ycCollisionCylinder& cylinder, const ycCollisionShape& cyl,    const ycCollisionOptions& opt, ycCollisionWeldableResults& results );

	// Mesh-vs-???. Returns number of contacts generated.
	ureg MeshCollide          ( const ycCollisionWorld* world, const ycCollisionMesh& mesh,				const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg MeshCollideShape     ( const ycCollisionMesh& mesh, const ycCollisionShape& shape,             const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg MeshCollideBox       ( const ycCollisionMesh& mesh, const ycCollisionShape& box,               const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg MeshCollideSphere    ( const ycCollisionMesh& mesh, const ycCollisionShape& sphere,            const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg MeshCollideCapsule   ( const ycCollisionMesh& mesh, const ycCollisionShape& capsule,           const ycCollisionOptions& opt, ycCollisionContactListBase& output );
	ureg MeshCollideCylinder  ( const ycCollisionMesh& mesh, const ycCollisionShape& cylinder,          const ycCollisionOptions& opt, ycCollisionContactListBase& output );

	// Returns -1 if no hit, otherwise distance of closest intersection along ray.
	f32 RayIntersect         ( const ycCollisionWorld* world, const ycLineSeg& segment,		const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	f32 RayIntersectShape    ( const ycLineSeg& segment, const ycCollisionShape& shape,     const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	f32 RayIntersectBox      ( const ycLineSeg& segment, const ycCollisionShape& box,       const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	f32 RayIntersectMesh     ( const ycLineSeg& segment, const ycCollisionShape& mesh,      const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	f32 RayIntersectSphere   ( const ycLineSeg& segment, const ycCollisionShape& sphere,	const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	f32 RayIntersectCapsule  ( const ycLineSeg& segment, const ycCollisionShape& cap,       const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
	f32 RayIntersectCylinder ( const ycLineSeg& segment, const ycCollisionShape& cyl,       const ycCollisionSweepOptions& opt, ycCollisionContactListBase& output );
}
