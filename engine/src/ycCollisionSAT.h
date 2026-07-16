#pragma once

/*
\class ycCollisionSAT

Utilities for using the separating axis theorem to run collision tests
*/

// some excess headers, but this is only included from cpp files
#include "ycCollision.h"
#include "ycLine.h"
#include "ycGeo.h"
#include "ycSphere.h"
#include "ycCapsule.h"
#include "ycAABB.h"
#include "ycCollisionTest.h"
#include "ycCollisionFeature.h"
#include "ycCollisionShape.h"

struct ycCollisionResult;

namespace ycCollisionSAT
{
	// Simple replacement for an identity matrix, used to make sure the templated code doesn't do lots of matrix multiplies
	struct IdentityProxy
	{
		constexpr operator ycMtx4() const { return ycMtx4::IDENTITY(); }
		ycVec3 operator*( const ycVec3& rhs ) const { return rhs; }
		constexpr IdentityProxy Transposed() const { return IdentityProxy(); }
		ycVec3 TransformPos( const ycVec3& rhs ) const { return rhs; }
		ycVec3 TransformDir( const ycVec3& rhs ) const { return rhs; }

		friend constexpr IdentityProxy operator*( const IdentityProxy&, const IdentityProxy& ) { return IdentityProxy(); }
		friend ycMtx4 operator*( const IdentityProxy&, const ycMtx4& rhs ) { return rhs; }
		friend ycMtx4 operator*( const ycMtx4& lhs, const IdentityProxy& ) { return lhs; }

		u8 dummy = 0; // required so struct has size
	};

	// --------------------------------------------------
	// HOW TO ADD SUPPORT FOR A NEW SHAPE TO ycSAT
	// --------------------------------------------------
	//
	//  1) Write an AxisProjection function. This should take your shape and an axis, and return min/max supporting points, and their dot products with the axis, for the shape.
	//  2) Write a SepAxisTest() function for each shape pair you need. This should call various CheckAxis functions.
	//  3) If you want to use the SatIterativeSweep() test, you will also need to implement its utility MoveShape().

	template <typename Shape>
	struct ShapeTraits
	{
	};

	// ==================
	//    ycTri
	// ==================
	template<>
	struct ShapeTraits<ycTri>
	{
		static constexpr u32 kVertCount = 3;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 3;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 3;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 1;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = false; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycTri& tt, u32 ii );
		static ycCollisionFeature GetEdge( const ycTri& tt, u32 ii );
		static ycVec3 GetEdgeDir( const ycTri& tt, u32 ii );
		static ycVec3 GetFaceNorm( const ycTri& tt, u32 ii );

		static constexpr IdentityProxy GetForwardXform( const ycTri& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycTri& ) { return IdentityProxy(); }

		static constexpr f32 GetRadius( const ycTri& ) { return 0.0f; }
	};

	// ==================
	//    Box
	// ==================
	template<>
	struct ShapeTraits<ycMtx4>
	{
		static constexpr u32 kVertCount = 8;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 12;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 3;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 3;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycMtx4& tt, u32 ii );
		static ycCollisionFeature GetEdge( const ycMtx4& tt, u32 ii );
		static ycVec3 GetEdgeDir( const ycMtx4& tt, u32 ii );
		static ycVec3 GetFaceNorm( const ycMtx4& tt, u32 ii );

		static constexpr IdentityProxy GetForwardXform( const ycMtx4& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycMtx4& ) { return IdentityProxy(); }

		static constexpr f32 GetRadius( const ycMtx4& ) { return 0.0f; }

		static void MoveShape( ycMtx4& box, const ycVec3& move ) { box.t.SetXYZ( box.t.xyz() + move ); }
	};
	
	template<>
	struct ShapeTraits<ycAABB>
	{
		static constexpr u32 kVertCount = 8;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 12;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 3;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 3;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycAABB& tt, u32 ii );
		static ycCollisionFeature GetEdge( const ycAABB& tt, u32 ii );
		static ycVec3 GetEdgeDir( const ycAABB& tt, u32 ii );
		static ycVec3 GetFaceNorm( const ycAABB& tt, u32 ii );

		static constexpr IdentityProxy GetForwardXform( const ycAABB& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycAABB& ) { return IdentityProxy(); }

		static constexpr f32 GetRadius( const ycAABB& ) { return 0.0f; }

		static void MoveShape( ycAABB& box, const ycVec3& move ) { box.center += move; }
	};

	// =======================
	//    ycCollision::Cube
	// =======================
	template<>
	struct ShapeTraits<ycCollisionBox>
	{
		static constexpr u32 kVertCount = 8;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 12;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 3;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 3;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycCollisionBox& tt, u32 ii )
		{
			return ShapeTraits<ycMtx4>::GetVert( tt.forward, ii );
		}

		static ycCollisionFeature GetEdge( const ycCollisionBox& tt, u32 ii )
		{
			return ShapeTraits<ycMtx4>::GetEdge( tt.forward, ii );
		}

		static ycVec3 GetEdgeDir( const ycCollisionBox& tt, u32 ii )
		{
			return ShapeTraits<ycMtx4>::GetEdgeDir( tt.forward, ii );
		}

		static ycVec3 GetFaceNorm( const ycCollisionBox& tt, u32 ii )
		{
			return ShapeTraits<ycMtx4>::GetFaceNorm( tt.forward, ii );
		}

		static constexpr IdentityProxy GetForwardXform( const ycCollisionBox& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycCollisionBox& ) { return IdentityProxy(); }

		static constexpr f32 GetRadius( const ycCollisionBox& ) { return 0.0f; }
	};

	// ==================
	//    ycVec3 (point)
	// ==================
	template<>
	struct ShapeTraits<ycVec3>
	{
		static constexpr u32 kVertCount = 1;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 0;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 0;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 0;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycVec3& tt, u32 )
		{
			return ( tt );
		}

		static ycCollisionFeature GetEdge( const ycVec3& tt, u32 )
		{
			ycAssert( 0 );
			return ycCollisionFeature::Vertex( tt );
		}

		static ycVec3 GetEdgeDir( const ycVec3&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static ycVec3 GetFaceNorm( const ycVec3&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static constexpr IdentityProxy GetForwardXform( const ycVec3& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycVec3& ) { return IdentityProxy(); }

		static f32 GetRadius( const ycVec3& )
		{ 
			return 0.0f;
		}

		static void MoveShape( ycVec3& tt, const ycVec3& move )
		{ 
			tt += move; 
		}
	};

	// ==================
	//    ycSphere
	// ==================
	template<>
	struct ShapeTraits<ycSphere>
	{
		static constexpr u32 kVertCount = 1;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 0;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 0;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 0;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycSphere& tt, u32 )
		{
			return ( tt.center );
		}

		static ycCollisionFeature GetEdge( const ycSphere& tt, u32 )
		{
			ycAssert( 0 );
			return ycCollisionFeature::Vertex( tt.center );
		}

		static ycVec3 GetEdgeDir( const ycSphere&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static ycVec3 GetFaceNorm( const ycSphere&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static constexpr IdentityProxy GetForwardXform( const ycSphere& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycSphere& ) { return IdentityProxy(); }

		static f32 GetRadius( const ycSphere& tt ) 
		{ 
			return tt.radius;
		}

		static void MoveShape( ycSphere& sph, const ycVec3& move ) 
		{ 
			sph.center += move; 
		}
	};

	// =========================
	//    ycCollisionSphere
	// =========================
	template<>
	struct ShapeTraits<ycCollisionSphere>
	{
		static constexpr u32 kVertCount = 1;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 0;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 0;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 0;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycCollisionSphere&, u32 )
		{
			return ycVec3::ZERO();
		}

		static ycCollisionFeature GetEdge( const ycCollisionSphere&, u32 )
		{
			ycAssert( 0 );
			return ycCollisionFeature::Vertex( ycVec3::ZERO() );
		}

		static ycVec3 GetEdgeDir( const ycCollisionSphere&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static ycVec3 GetFaceNorm( const ycCollisionSphere&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static ycMtx4 GetForwardXform( const ycCollisionSphere& s )
		{
			return s.forward;
		}

		static ycMtx4 GetInverseXform( const ycCollisionSphere& s )
		{
			return s.inverse;
		}

		static f32 GetRadius( const ycCollisionSphere& tt )
		{
			return tt.radius;
		}

		static void MoveShape( ycCollisionSphere& sph, const ycVec3& move ) 
		{ 
			sph.forward.t += ycVec4( move, 0.0f ); 
			sph.inverse = sph.forward.Inverted();  // FIXME: there should be a simpler way to update the inverse
		}
	};

	// ==================
	//    ycCapsule
	// ==================
	template<>
	struct ShapeTraits<ycCapsule>
	{
		static constexpr u32 kVertCount = 2;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 1;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 1;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 0;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycCapsule& cc, u32 ii )
		{
			ycAssert( ii < 2 );
			return cc.center[ ii ];
		}

		static ycCollisionFeature GetEdge( const ycCapsule& cc, u32 ii )
		{
			ycAssert( ii == 0 );
			YC_UNUSED( ii );
			return ycCollisionFeature::Edge( cc.bottom, cc.top, cc.radius );
		}

		static ycVec3 GetEdgeDir( const ycCapsule& cc, u32 ii )
		{
			ycAssert( ii == 0 );
			YC_UNUSED( ii );
			return ( cc.top - cc.bottom );
		}

		static ycVec3 GetFaceNorm( const ycCapsule&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static constexpr IdentityProxy GetForwardXform( const ycCapsule& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycCapsule& ) { return IdentityProxy(); }

		static f32 GetRadius( const ycCapsule& tt )
		{ 
			return tt.radius;
		}

		static void MoveShape( ycCapsule& cc, const ycVec3& move )
		{ 
			cc.top += move;
			cc.bottom += move;
		}
	};

	// =========================
	//    ycCollisionCapsule
	// =========================
	template<>
	struct ShapeTraits<ycCollisionCapsule>
	{
		static constexpr u32 kVertCount = 2;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 1;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 1;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 0;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycCollisionCapsule& cc, u32 ii )
		{
			ycAssert( ii < 2 );
			return cc.pts[ ii ];
		}

		static ycCollisionFeature GetEdge( const ycCollisionCapsule& cc, u32 ii )
		{
			ycAssert( ii == 0 );
			YC_UNUSED( ii );
			return ycCollisionFeature::Edge( cc.pts[ 0 ], cc.pts[ 1 ], cc.radius );
		}

		static ycVec3 GetEdgeDir( const ycCollisionCapsule& cc, u32 ii )
		{
			ycAssert( ii == 0 );
			YC_UNUSED( ii );
			return ( cc.pts[ 1 ] - cc.pts[ 0 ] );
		}

		static ycVec3 GetFaceNorm( const ycCollisionCapsule&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::YAXIS();
		}

		static ycMtx4 GetForwardXform( const ycCollisionCapsule& s )
		{
			return s.forward;
		}

		static ycMtx4 GetInverseXform( const ycCollisionCapsule& s )
		{
			return s.inverse;
		}

		static f32 GetRadius( const ycCollisionCapsule& tt )
		{
			return tt.radius;
		}

		static void MoveShape( ycCollisionCapsule& cap, const ycVec3& move ) 
		{ 
			ycMtx4 fwd = ycMtx4::IDENTITY(); fwd.t.SetXYZ( move );
			ycMtx4 inv = ycMtx4::IDENTITY(); inv.t.SetXYZ( -move );
			cap.forward = fwd * cap.forward;
			cap.inverse = cap.inverse * inv;
		}
	};

	// ===========================
	//    ycCollisionCylinder
	// ===========================
	template<>
	struct ShapeTraits<ycCollisionCylinder>
	{
		static constexpr u32 kVertCount = 0;      // Number of distinct verts
		static constexpr u32 kEdgeCount = 1;      // Number of distinct edges
		static constexpr u32 kEdgeDirCount = 1;   // Number of distinct edge directions (can be less than edges)
		static constexpr u32 kFaceNormCount = 1;  // Number of distinct face normals (can be less than faces)

		static constexpr bool kAllowBackface = true; // Does this shape allow axis inversions (aka backfaces)

		static ycVec3 GetVert( const ycCollisionCylinder&, u32 )
		{
			ycAssert( 0 );
			return ycVec3::ZERO();
		}

		static ycCollisionFeature GetEdge( const ycCollisionCylinder& cc, u32 )
		{
			return ycCollisionFeature::Edge( cc.center + cc.upDir * (cc.height * 0.5f), cc.center - cc.upDir * (cc.height * 0.5f) );
		}

		static ycVec3 GetEdgeDir( const ycCollisionCylinder& cc, u32 )
		{
			return cc.upDir;
		}

		static ycVec3 GetFaceNorm( const ycCollisionCylinder& cc, u32 )
		{
			return cc.upDir;
		}

		static constexpr IdentityProxy GetForwardXform( const ycCollisionCylinder& ) { return IdentityProxy(); }
		static constexpr IdentityProxy GetInverseXform( const ycCollisionCylinder& ) { return IdentityProxy(); }

		static constexpr f32 GetRadius( const ycCollisionCylinder& cyl ) { return cyl.radius; }

		static void MoveShape( ycCollisionCylinder& cyl, const ycVec3& move ) { cyl.center += move; }
	};

	typedef ycCollisionOptions::BackfaceBehavior BackfaceBehavior;

	void AxisProjection( const ycVec3&              point, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycTri&                 tri, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycSphere&              sph, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycCollisionSphere&   sph, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycMtx4&                box, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycAABB&                box, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycCollisionBox&      box, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycCollisionCapsule&  cap, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );
	void AxisProjection( const ycCollisionCylinder& cyl, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint );

	ycCollisionResult SepAxisTest( const ycVec3& pt, const ycCollisionSphere& sphB,   f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycVec3& pt, const ycCollisionCapsule& sphB,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycVec3& pt, const ycCollisionCylinder& sphB, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycTri& tri,                 f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycMtx4& box,                f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycSphere& sph,              f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycCollisionBox& box,      f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycCollisionSphere& sph,   f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycCollisionCapsule& cap,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCylinder& cyl, const ycCollisionCylinder& cyB, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycMtx4& box, const ycTri& tri,                 f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycMtx4& box, const ycCollisionBox& boxB,     f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycMtx4& box, const ycCollisionSphere& sph,   f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycMtx4& box, const ycCollisionCapsule& cap,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycMtx4& box, const ycCollisionCylinder& cyl, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycCollisionSphere& sph, const ycCollisionSphere& sphB,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionSphere& sph, const ycCollisionBox& box,      f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionSphere& sph, const ycTri& tri,                 f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionSphere& sph, const ycCollisionCapsule& cap,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionSphere& sph, const ycCollisionCylinder& cyl, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycCollisionBox& box,   const ycCollisionSphere& sph, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycSphere& sph, const ycCollisionCylinder& cyl, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycSphere& sph, const ycCollisionSphere& sphB,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycSphere& sph, const ycSphere& sphB,             f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycSphere& sph, const ycTri& tri,                 f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycSphere& sph, const ycCollisionBox& box,      f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycSphere& sph, const ycCollisionCapsule& cap,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycCollisionCapsule& capA, const ycTri& triB,                 f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionSphere& sphB,   f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionBox& boxB,      f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionCapsule& capB,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionCylinder& cylB, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );

	ycCollisionResult SepAxisTest( const ycTri& tri, const ycMtx4& box,                f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycTri& tri, const ycCollisionSphere& sph,   f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycTri& tri, const ycCollisionCylinder& cyl, f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	ycCollisionResult SepAxisTest( const ycTri& tri, const ycCollisionCapsule& cap,  f32 sepPrecision, BackfaceBehavior allowBackface, bool weldingOn );
	
	bool SatIterativeSweep( const ycSphere& sph,     const ycSphere& sphB,           const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycSphere& sph,     const ycCollisionSphere& sphB,  const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycSphere& sph,     const ycCollisionCylinder& cyl, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycSphere& sph,     const ycTri& tri,               const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycSphere& sph,     const ycCollisionCapsule& cap,  const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycSphere& sph,     const ycCollisionBox& box,      const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );

	bool SatIterativeSweep( const ycMtx4& box,       const ycTri& tri,                 const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycMtx4& box,       const ycCollisionBox& boxB,     const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycMtx4& box,       const ycCollisionSphere& sph,   const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycMtx4& box,       const ycCollisionCapsule& cap,  const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycMtx4& box,       const ycCollisionCylinder& cyl, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	
	bool SatIterativeSweep( const ycAABB& box,       const ycTri& tri,                 const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );

	bool SatIterativeSweep( const ycCollisionSphere& sphA, const ycTri& tri,                const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionSphere& sphB,   const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionCapsule& capB,  const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionBox& boxB,      const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionCylinder& cylB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionSphere& sphA, const ycSphere& sphB,            const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );

	bool SatIterativeSweep( const ycCollisionCapsule& capA, const ycTri& tri,               const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionSphere& sphB,  const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionCapsule& capB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionBox& boxB,     const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionCylinder& cyl, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );

	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycTri& tri,                 const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycCollisionSphere& sph,   const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycCollisionCapsule& cap,  const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycSphere& sph,              const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycCollisionCylinder& cyB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycCollisionBox& boxB,     const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );
	bool SatIterativeSweep( const ycCollisionCylinder& cyl, const ycMtx4& box,                const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision = 0.0001f );

	template <typename ShapeA, typename ShapeB>
	void GetClosestPoints( const ShapeA& sA, const ShapeB& sB, ycVec3* pointOnA, ycVec3* pointOnB )
	{
		ycCollisionResult mtd = SepAxisTest( sA, sB, YC_F32_MAX );

		if( pointOnA ) { *pointOnA = mtd.GetPointA(); }
		if( pointOnB ) { *pointOnB = mtd.GetPointB(); }
	}

	// Use the AxisProject funtion to get a high-quality aabb
	template <typename Shape>
	ycBox GetBounds( const Shape& sA )
	{
		ycVec3 minDummy, maxDummy;
		ycVec3 boxMin, boxMax;
		AxisProjection( sA, ycVec3::XAXIS(), boxMin.x, minDummy, boxMax.x, maxDummy);
		AxisProjection( sA, ycVec3::YAXIS(), boxMin.y, minDummy, boxMax.y, maxDummy );
		AxisProjection( sA, ycVec3::ZAXIS(), boxMin.z, minDummy, boxMax.z, maxDummy );

		return ycBox( boxMin, boxMax );
	};

};
