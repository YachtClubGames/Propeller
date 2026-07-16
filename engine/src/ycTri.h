#pragma once

/*! \struct ycTri
Shape defined by 3 points
*/

#include "ycShape.h"
#include "ycVec3.h"

struct ycAABB;
struct ycFrustum;
struct ycGeoRayTriHit;
struct ycLineSeg;
struct ycPlane;
struct ycPlane;
struct ycSphere;
struct ycTri;

struct ycTri : public ycShape //%
{
yceditable:

	union
	{
		struct { ycVec3 a, b, c; };
		struct { ycVec3 pt1; ycVec3 pt2; ycVec3 pt3; };
		ycVec3 pts[ 3 ];
	};

public:

	//! default ctor
	ycTri() = default;

	constexpr inline ycTri( const ycVec3& a, const ycVec3& b, const ycVec3& c  );

	ycTri& Offset( const ycVec3 & v );
	ycTri& Set( const ycTri& );
	ycTri& Set( const ycVec3& a, const ycVec3& b, const ycVec3& c );
	ycTri& SetA( const ycVec3& );
	ycTri& SetB( const ycVec3& );
	ycTri& SetC( const ycVec3& );
	ycVec3 GetA( ) const;
	ycVec3 GetB( ) const;
	ycVec3 GetC( ) const;

	//! Calculates the center of the triangle
	ycVec3 GetCenter( ) const;

	//! return the triangle normal.
	ycVec3 GetNormal( ) const;
	ycVec3 GetNormalCCW() const;

	//! Returns the clockwise normal of the triangle
	ycVec3 GetNormalCW() const;

	//! Returns the edge normals
	ycLineSeg GetEdge( s32 index ) const;
	void GetEdgeNormals( ycVec3 * ab, ycVec3 * bc, ycVec3* ca ) const;

	f32 GetArea( ) const;
	f32 GetAreaSq( ) const;

	//! returns the vector to B from A
	inline ycVec3 GetAB() const;

	//! returns the vector to C from B
	inline ycVec3 GetBC() const;

	//! returns the vector to C from A
	inline ycVec3 GetAC() const;

	//! Determines the area of the triangle
	f32	Area() const;

	//! Determines the length of the permimeter of the triangle
	f32 Perimeter() const;
	f32 PerimeterSq() const;

	ycPlane ToPlane() const;
	void GetPlane( ycPlane * plane) const;
	void GetPlaneCCW( ycPlane * plane) const;
	void GetPlaneCW( ycPlane * plane) const;
	void GetEdgePlanes( ycPlane * ab, ycPlane * bc, ycPlane * ca ) const;

	ycVec3 BarycentricUVW( const ycVec3 & pt ) const;
	ycVec3 GetPointUVW( const ycVec3 & baryUVWPt ) const;

	//! Calculate axis projection
	void AxisProjection(const ycVec3 & dir, f32 * min, f32 * max ) const;

	ycVec3 GetLocalVertex( const ycVec3 & dir, f32 margin ) const;

	ycAABB ToAABB() const;

	//! same ideas as Contains but different method that will also allow for less than normal triangles
	bool ContainsSameSide( const ycVec3&  pt );

	bool IsSameSide( const ycVec3 & pt );

	//! Determines if the triangle is front facing given the look direction
	bool IsFrontFacing(const ycVec3 & lookDir) const;

	//! Determines if the triangle is degenerate (has no area)
	bool IsDegenerate( f32 episilon = YC_F32_EPSILON ) const;

	//! Determines the closest point inside the tri to the point
	ycVec3 ClosestPoint( const ycVec3& pt ) const;
	//! Determines the closest point on the tri to the line (optionally gets the closest point on the line to the tri)
	ycVec3 ClosestPoint( const ycLineSeg & lineSeg, ycVec3 * closestOnLine = nullptr ) const;
	//! Determines the closest point on the edge of the tri to the line, optionally gets the distance along the line to the closest point
	ycVec3 ClosestPointToEdge( const ycLineSeg & lineSeg, f32 * lineDist = nullptr ) const;
	ycVec3 ClosestPointToEdge( const ycRay & ray, f32 * rayDist = nullptr ) const;
	
	//! Determine distance from point
	f32 DistSq( const ycVec3& pt ) const;
	f32 Dist( const ycVec3& pt ) const;

	//! Detect if point is inside trie
	bool Contains( const ycVec3 & pt ) const;
	
	//! Detects if triangle touches any part of the shape
	bool Intersects( const ycAABB& aabb ) const;
	bool Intersects( const ycTri& tri ) const;
	bool Intersects( const ycLineSeg & lineSeg ) const;
	bool Intersects( const ycPlane & plane ) const;
	bool Intersects( const ycSphere & sphere ) const;

	//! Determines if a capsule intersects with this plane
	//bool IntersectsCapsule( const ycCapsule & capsule ) const;
	
	//! Detects if triangle touches any part of the shape and returns intersection
	bool Intersection( ycGeoRayTriHit* hit, const ycRay&, f32 maxT ) const;
	bool Intersection( ycGeoRayTriHit* hit, const ycLineSeg& ) const;
	bool Intersection( ycVec3* pt_col, const ycLine& ) const;
	
	//! Sweep triangle againt shape
	bool Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycLineSeg & other ) const;
	bool Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycAABB & other, const ycLineSeg & otherSweep ) const;
	bool Sweep( ycGeoRayTriHit* hit, const ycSphere& other, const ycVec3& otherSweep ) const;

	//! Operators
	inline bool	operator==( const ycTri & other ) const;
	inline bool	operator!=( const ycTri & other ) const;
	inline ycTri operator+( const ycVec3& rhs ) const;
	inline ycTri& operator += ( const ycVec3& rhs );

	constexpr inline static ycTri ZERO()  { return ycTri( ycVec3::ZERO(), ycVec3::ZERO(), ycVec3::ZERO() ); }
};

constexpr inline ycTri::ycTri( const ycVec3& a, const ycVec3& b, const ycVec3& c  )
	: a( a )
	, b( b )
	, c( c )
{
}

inline bool	ycTri::operator==( const ycTri & other ) const
{
	return a == other.a && b == other.b && c == other.c;
}

inline bool	ycTri::operator!=( const ycTri & other ) const
{
	return a != other.a || b != other.b || c != other.c;
}

inline ycTri ycTri::operator+( const ycVec3& rhs ) const
{
	ycTri tri;
	tri.a = a + rhs;
	tri.b = b + rhs;
	tri.c = c + rhs;
	return tri;
}

inline ycTri& ycTri::operator+=( const ycVec3& rhs )
{
	a += rhs;
	b += rhs;
	c += rhs;
	return *this;
}

inline ycVec3 ycTri::GetAB( ) const
{
	return ( b - a );
}

inline ycVec3 ycTri::GetBC( ) const
{
	return ( c - b );
}

inline ycVec3 ycTri::GetAC( ) const
{
	return ( c - a );
}
