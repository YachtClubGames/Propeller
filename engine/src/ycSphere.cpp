#include "ycSphere.h"

#include "ycGeo.h"
#include "ycAABB.h"
#include "ycVector.h"
#include "ycOpenHash.h"

namespace
{
	typedef ycOpenHash< u64, u32 > MidPointHash;
	u32 FindMidPoint( ycVector<ycVec3>& verts, MidPointHash& table, u32 p1, u32 p2 )
	{
		u64 hashKey;
		if( p1 < p2 )
		{
			hashKey = u64((u64)p1 << (u64)32) + p2;
		}
		else
		{
			hashKey = u64((u64)p2 << (u64)32) + p1;
		}
		u32* index = table.Get( hashKey );
		if( index )
		{
			return *index;
		}
		ycVec3 pos = (verts[p1] + verts[p2]) * 0.5f;
		verts.Append( pos );
		table.Insert( hashKey, u32(verts.Length()-1) );
		return u32(verts.Length()-1);
	}
}

ycAABB ycSphere::ToAABB() const
{
	return ycAABB( center, radius, radius, radius );
}

ycVec3 ycSphere::GetLocalVertex( const ycVec3 & dir, f32 margin ) const
{
	ycVec3 normalized = dir;
	if( normalized.MagSq() < (YC_F32_EPSILON*YC_F32_EPSILON))
	{
		normalized = ycVec3( -1.0f, -1.0f, -1.0f );
	} 
	normalized.Normalize();
	return normalized * (radius+margin);
}

ycVec3 ycSphere::ClosestPoint( const ycVec3 & pt ) const
{
	ycVec3 point;
	ycGeo::ClosestPointSphere( *this, pt, &point );
	return point;
}

ycVec3 ycSphere::ClosestPoint( const ycSphere & sphere ) const
{
	ycVec3 point, point2;
	ycGeo::ClosestPointsSphereSphere( *this, sphere, &point, &point2 );
	return point;
}

f32 ycSphere::DistSq( const ycVec3 & pt ) const
{
	return ycGeo::DistSqSpherePoint( *this, pt );
}

f32 ycSphere::Dist( const ycVec3 & pt ) const
{
	return ycGeo::DistSpherePoint( *this, pt );
}

f32 ycSphere::DistSq( const ycLineSeg & lineSeg ) const
{
	return ycGeo::DistSqSphereLineSeg( *this, lineSeg );
}

f32 ycSphere::Dist( const ycLineSeg & lineSeg ) const
{
	return ycGeo::DistSphereLineSeg( *this, lineSeg );
}

f32 ycSphere::DistSq( const ycSphere & other ) const
{
	return ycGeo::DistSqSphereSphere( *this, other );
}

f32 ycSphere::Dist( const ycSphere & other ) const
{
	return ycGeo::DistSphereSphere( *this, other );
}

f32 ycSphere::DistSq( const ycCapsule & capsule ) const
{
	return ycGeo::DistSqCapsuleSphere( capsule, *this );
}

f32 ycSphere::Dist( const ycCapsule & capsule ) const
{
	return ycGeo::DistCapsuleSphere( capsule, *this );
}

f32	ycSphere::DistSq( const ycAABB & box ) const
{
	return ycGeo::DistSqAABBSphere( box, *this );
}

f32	ycSphere::Dist( const ycAABB & box ) const
{
	return ycGeo::DistAABBSphere( box, *this );
}

f32	ycSphere::Dist( const ycPlane & plane ) const
{
	return ycGeo::DistPlaneSphere( plane, *this );
}

f32	ycSphere::DistSq( const ycTri & tri ) const
{
	return ycGeo::DistSqSphereTri( *this, tri );
}

f32	ycSphere::Dist( const ycTri & tri ) const
{
	return ycGeo::DistSphereTri( *this, tri );
}
	
bool ycSphere::Contains( const ycVec3 & pt ) const
{
	return ycGeo::ContainsSpherePt( *this, pt );
}

bool ycSphere::Intersect( const ycSphere & other ) const
{
	return ycGeo::IntersectsSphereSphere( *this, other );
}

u32	 ycSphere::Intersects( const ycLineSeg & seg ) const
{
	return ycGeo::IntersectsLineSegSphere( seg, *this );
}

bool ycSphere::Intersects( const ycRay & ray ) const
{
	return ycGeo::IntersectsRaySphere( ray, *this );
}

bool ycSphere::Intersects( const ycPlane & plane ) const
{
	return ycGeo::IntersectsSpherePlane( *this, plane );
}

bool ycSphere::Intersects( const ycAABB &aabb ) const
{
	return ycGeo::IntersectsAABBSphere( aabb, *this );
}

bool ycSphere::Intersects( const ycTri & tri ) const
{
	return ycGeo::IntersectsSphereTri( *this, tri );
}

bool ycSphere::Intersects( const ycFrustum & frustum ) const
{
	return ycGeo::IntersectsSphereFrustum( *this, frustum );
}

bool ycSphere::Intersects( const ycCircle & other ) const
{
	return ycGeo::IntersectsCircleSphere( other, *this );
}

bool ycSphere::Intersects( const ycCapsule & capsule ) const
{
	return ycGeo::IntersectsCapsuleSphere( capsule, *this );
}

bool ycSphere::Intersection( f32* hit, const ycLineSeg & seg ) const
{
	return ycGeo::IntersectionLineSegSphere( hit, seg, *this );
}

bool ycSphere::Intersection( f32* hit, const ycRay & ray, f32 maxT ) const
{
	return ycGeo::IntersectionRaySphere( hit, ray, maxT, *this );
}

bool ycSphere::Sweep( ycGeoRayTriHit* hit, const ycVec3& mySweep, const ycTri& tri ) const
{
	return ycGeo::SweepSphereTri( hit, *this, mySweep, 1.0f, tri );
}

//http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
void ycSphere::ToVertCount( u32 * ptsCount, u32* faceCount, u32 subdivide ) const
{
	ycVector<ycVec3> vertices( g_defaultMem );
	vertices.Resize( 12 );

	f32 t = (1.0f + ycSqrt( 5.0f )) / 2.0f;
	vertices[0] = ycVec3( -1.0f, t, 0.0f );
	vertices[1] = ycVec3( 1.0f, t, 0.0f );
	vertices[2] = ycVec3( -1.0f, -t, 0.0f );
	vertices[3] = ycVec3( 1.0f, -t, 0.0f );
	vertices[4] = ycVec3( 0.0f, -1.0f, t );
	vertices[5] = ycVec3( 0.0f, 1.0f, t );
	vertices[6] = ycVec3( 0.0f, -1.0f, -t );
	vertices[7] = ycVec3( 0.0f, 1.0f, -t );
	vertices[8] = ycVec3( t, 0.0f, -1.0f );
	vertices[9] = ycVec3( t, 0.0f, 1.0f );
	vertices[10] = ycVec3( -t, 0.0f, -1.0f );
	vertices[11] = ycVec3( -t, 0.0f, 1.0f );
	for( s32 i = 0; i < 12; ++i )
	{
		vertices[i] += center;
		vertices[i] *= radius;
	}

	struct SphereFace
	{
		u32 ind[3];
	};
	ycVector<SphereFace> faces( g_defaultMem );
	faces.Resize( 20 );
	faces[0] = { 0, 11, 5 };
	faces[1] = { 0, 5, 1 };
	faces[2] = { 0, 1, 7 };
	faces[3] = { 0, 7, 10 };
	faces[4] = { 0, 10, 11 };

	faces[5] = { 1, 5, 9 };
	faces[6] = { 5, 11, 4 };
	faces[7] = { 11, 10, 2 };
	faces[8] = { 10, 7, 6 };
	faces[9] = { 7, 1, 8 };

	faces[10] = { 3, 9, 4 };
	faces[11] = { 3, 4, 2 };
	faces[12] = { 3, 2, 6 };
	faces[13] = { 3, 6, 8 };
	faces[14] = { 3, 8, 9 };

	faces[15] = { 4, 9, 5 };
	faces[16] = { 2, 4, 11 };
	faces[17] = { 6, 2, 10 };
	faces[18] = { 8, 6, 7 };
	faces[19] = { 9, 8, 1 };

	MidPointHash hashTable( g_defaultMem );
	for( u32 i = 0; i < subdivide; ++i )
	{
		ycVector<SphereFace> faces2( g_defaultMem );
		for( const SphereFace& tri : faces )
		{
			u32 a = FindMidPoint( vertices, hashTable, tri.ind[0], tri.ind[1] );
			u32 b = FindMidPoint( vertices, hashTable, tri.ind[1], tri.ind[2] );
			u32 c = FindMidPoint( vertices, hashTable, tri.ind[2], tri.ind[0] );
			faces2.Append( { tri.ind[0], a, c } );
			faces2.Append( { tri.ind[1], b, a } );
			faces2.Append( { tri.ind[2], c, b } );
			faces2.Append( { a, b, c } );
		}
		faces = faces2;
	}
	*faceCount = (u32)faces.Length();
	*ptsCount = (u32)vertices.Length();
}

void ycSphere::ToVerts( ycVec3 * pts, u32 * ptsCount, u32 * facesI, u32* faceCount, u32 subdivide ) const
{
	ycVector<ycVec3> vertices( g_defaultMem );
	vertices.Resize( 12 );

	f32 s = 1.0f;
	f32 t = (1.0f + ycSqrt( 5.0f )) / 2.0f;
	vertices[0] = ycVec3( -s, t, 0.0f );
	vertices[1] = ycVec3( s, t, 0.0f );
	vertices[2] = ycVec3( -s, -t, 0.0f );
	vertices[3] = ycVec3( s, -t, 0.0f );
	vertices[4] = ycVec3( 0.0f, -s, t );
	vertices[5] = ycVec3( 0.0f, s, t );
	vertices[6] = ycVec3( 0.0f, -s, -t );
	vertices[7] = ycVec3( 0.0f, s, -t );
	vertices[8] = ycVec3( t, 0.0f, -s );
	vertices[9] = ycVec3( t, 0.0f, s );
	vertices[10] = ycVec3( -t, 0.0f, -s );
	vertices[11] = ycVec3( -t, 0.0f, s );

	struct SphereFace
	{
		u32 ind[3];
	};
	ycVector<SphereFace> faces( g_defaultMem );
	faces.Resize( 20 );
	faces[0] = { 0, 11, 5 };
	faces[1] = { 0, 5, 1 };
	faces[2] = { 0, 1, 7 };
	faces[3] = { 0, 7, 10 };
	faces[4] = { 0, 10, 11 };

	faces[5] = { 1, 5, 9 };
	faces[6] = { 5, 11, 4 };
	faces[7] = { 11, 10, 2 };
	faces[8] = { 10, 7, 6 };
	faces[9] = { 7, 1, 8 };

	faces[10] = { 3, 9, 4 };
	faces[11] = { 3, 4, 2 };
	faces[12] = { 3, 2, 6 };
	faces[13] = { 3, 6, 8 };
	faces[14] = { 3, 8, 9 };

	faces[15] = { 4, 9, 5 };
	faces[16] = { 2, 4, 11 };
	faces[17] = { 6, 2, 10 };
	faces[18] = { 8, 6, 7 };
	faces[19] = { 9, 8, 1 };

	MidPointHash hashTable( g_defaultMem );
	for( u32 i = 0; i < subdivide; ++i )
	{
		ycVector<SphereFace> faces2( g_defaultMem );
		for( const SphereFace& tri : faces )
		{
			u32 a = FindMidPoint( vertices, hashTable, tri.ind[0], tri.ind[1] );
			u32 b = FindMidPoint( vertices, hashTable, tri.ind[1], tri.ind[2] );
			u32 c = FindMidPoint( vertices, hashTable, tri.ind[2], tri.ind[0] );
			faces2.Append( { tri.ind[0], a, c } );
			faces2.Append( { tri.ind[1], b, a } );
			faces2.Append( { tri.ind[2], c, b } );
			faces2.Append( { a, b, c } );
		}
		faces = faces2;
	}

	*faceCount = (u32)faces.Length();
	*ptsCount = (u32)vertices.Length();
	for( ycSize_t i = 0; i < faces.Length(); ++i )
	{
		facesI[i*3+0] = faces[i].ind[0];
		facesI[i*3+1] = faces[i].ind[1];
		facesI[i*3+2] = faces[i].ind[2];
	}
	for( ycSize_t i = 0; i < vertices.Length(); ++i )
	{
		pts[i] = vertices[i].Normalized() * radius;
	}
}
