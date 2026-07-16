#include "ycTri.h"

#include "ycPlane.h"
#include "ycAABB.h"
#include "ycGeo.h"
#include "ycLineSeg.h"

ycTri& ycTri::Offset( const ycVec3 & v )
{
	a += v;
	b += v;
	c += v;
	return *this;
}

ycTri& ycTri::Set( const ycTri& tri)
{
	a = tri.a;
	b = tri.b;
	c = tri.c;
	return *this;
}

ycTri& ycTri::Set( const ycVec3& _a, const ycVec3& _b, const ycVec3& _c )
{
	a = _a;
	b = _b;
	c = _c;
	return *this;
}

ycTri& ycTri::SetA( const ycVec3& _a )
{
	a = _a;
	return *this;
}

ycTri& ycTri::SetB( const ycVec3& _b )
{
	b = _b;
	return *this;
}

ycTri& ycTri::SetC( const ycVec3& _c )
{
	c = _c;
	return *this;
}

ycVec3 ycTri::GetA( ) const
{
	return a;
}

ycVec3 ycTri::GetB( ) const
{
	return b;
}

ycVec3 ycTri::GetC( ) const
{
	return c;
}

ycVec3 ycTri::GetCenter( ) const
{
	return ( a + b + c ) / 3.f;
}

f32 ycTri::GetArea( ) const
{
	ycVec3 cProd( ( a - b ).Dot( c - a ) );
	return  cProd.Mag()/ 2.f;
}

f32 ycTri::GetAreaSq( ) const
{
	ycVec3 cProd( ( a - b ).Dot( c - a ) );
	return  cProd.MagSq()/ 2.f;
}

ycVec3 ycTri::GetNormal( ) const
{
	return GetNormalCCW();
}

ycVec3 ycTri::GetNormalCCW() const
{
	return ( b - a ).Cross( c - a ).Normalized();
}

ycVec3 ycTri::GetNormalCW() const
{
	return ( c - a ).Cross( b - a ).Normalized();
}

f32 ycTri::Area() const
{
	ycVec3 normal = (pt2 - pt1).Cross( pt3 - pt1);
	return normal.Mag() * 0.5f;
}

f32 ycTri::Perimeter() const
{
	return pt1.Dist( pt2 ) + pt2.Dist( pt3 ) + pt3.Dist( pt1 );
}

f32 ycTri::PerimeterSq() const
{
	return pt1.DistSq( pt2 ) + pt2.DistSq( pt3 ) + pt3.DistSq( pt1 );
}

ycLineSeg ycTri::GetEdge( s32 index ) const
{
	switch( index )
	{
	case 1: return ycLineSeg( GetB(), GetC() ); break;
	case 2: return ycLineSeg( GetC(), GetA() ); break;
	}
	return ycLineSeg( GetA(), GetB() );
}

void ycTri::GetEdgeNormals( ycVec3 * ab, ycVec3 * bc, ycVec3* ca ) const
{
	ycVec3 n = GetNormal();
	*ab = GetAB().Cross( n ).Normalized();
	*bc = GetBC().Cross( n ).Normalized();
	*ca = -GetAC().Cross( n ).Normalized();
}

ycPlane ycTri::ToPlane() const
{
	return ycPlane( a, b, c );
}

void ycTri::GetPlane( ycPlane * plane) const
{
	GetPlaneCCW( plane );
}

void ycTri::GetPlaneCCW( ycPlane * plane) const
{
	*plane = ycPlane( a, b, c );
}

void ycTri::GetPlaneCW( ycPlane * plane) const
{
	*plane = ycPlane( a, c, b );
}

void ycTri::GetEdgePlanes( ycPlane * ab, ycPlane * bc, ycPlane * ca ) const
{
	ycVec3 n = GetNormal();
	*ab = ycPlane( -n.Cross( GetAB().Normalized() ), a );
	*bc = ycPlane( -n.Cross( GetBC().Normalized() ), b );
	*ca = ycPlane( n.Cross( GetAC().Normalized() ), c );
}

ycVec3 ycTri::BarycentricUVW( const ycVec3 & pt ) const
{
	ycVec3 v0 = GetB() - GetA();
	ycVec3 v1 = GetC() - GetA();
	ycVec3 v2 = pt - GetA();
	f32 d00 = v0.Dot( v0 );
	f32 d01 = v0.Dot( v1 );
	f32 d11 = v1.Dot( v1 );
	f32 d20 = v2.Dot( v0 );
	f32 d21 = v2.Dot( v1 );
	f32 denom = 1.0f / (d00 * d11 - d01 * d01 );
	f32 v = (d11 * d20 - d01 * d21) * denom;
	f32 w = (d00 * d21 - d01 * d20) * denom;
	f32 u = 1.0f - v - w;
	return ycVec3(u, v, w);
}

ycVec3 ycTri::GetPointUVW( const ycVec3 & baryUVWPt ) const
{
	return pt1 * baryUVWPt.x + pt2 * baryUVWPt.y + pt3 * baryUVWPt.z;
}

void ycTri::AxisProjection(const ycVec3 & dir, f32 * min, f32 * max ) const
{
	*min = *max = dir.Dot( a );
	f32 t = dir.Dot( b );
	*min = ycMin( t, *min );
	*max = ycMax( t, *max );
	t = dir.Dot( c );
	*min = ycMin( t, *min );
	*max = ycMax( t, *max );
}

ycVec3 ycTri::GetLocalVertex( const ycVec3 & dir, f32 margin ) const
{
	ycVec3 dots = ycVec3( dir.Dot( a ), dir.Dot( b ), dir.Dot( c ) );
	ycVec3 pos = pts[dots.a[0] < dots.a[1] ? (dots.a[1] <dots.a[2] ? 2 : 1) : (dots.a[0] <dots.a[2] ? 2 : 0)];
	if( margin )
	{
		ycVec3 vecnorm = dir;
		if( dir.MagSq() < (YC_F32_EPSILON*YC_F32_EPSILON))
		{
			vecnorm = ycVec3( -1.0f, -1.0f, -1.0f );
		} 
		vecnorm.Normalize();
		pos+= vecnorm * margin;
	}
	return pos;
}

ycAABB ycTri::ToAABB() const
{
	ycVec3 min = pts[2];
	ycVec3 max = pts[2];
	for( s32 i = 0; i < 2; ++i )
	{
		min.x = ycMin( pts[i].x, min.x );
		min.y = ycMin( pts[i].y, min.y );
		min.z = ycMin( pts[i].z, min.z );

		max.x = ycMax( pts[i].x, max.x );
		max.y = ycMax( pts[i].y, max.y );
		max.z = ycMax( pts[i].z, max.z );
	}
	return ycAABB( min, max );
}

bool ycTri::ContainsSameSide( const ycVec3&  pt )
{
	return ycTri(GetA(), GetB(), GetC()).IsSameSide( pt )
		&&  ycTri(GetB(), GetA(), GetC()).IsSameSide( pt )
		&&	ycTri(GetC(), GetA(), GetB()).IsSameSide( pt );
}

bool ycTri::IsSameSide( const ycVec3 & pt )
{
	ycVec3 threeMTwo, cp1, cp2;
	threeMTwo = GetC() - GetB();
	cp1 = threeMTwo.Cross( pt - GetB() );
	cp2 = threeMTwo.Cross( GetA() - GetB() );
	return cp1.Dot( cp2 ) >= 0.0f;
}

bool ycTri::IsFrontFacing( const ycVec3 & lookDir ) const
{
	return GetNormal().Dot( lookDir ) <= 0.0f;
}

bool ycTri::IsDegenerate( f32 episilon ) const
{
	return Area() <= episilon;
}

ycVec3 ycTri::ClosestPoint( const ycVec3& pt ) const
{
	ycVec3 ptOnTri;
	ycGeo::ClosestPointTri( *this, pt, &ptOnTri );
	return ptOnTri;
}

ycVec3 ycTri::ClosestPoint( const ycLineSeg & lineSeg, ycVec3 * closestOnLine ) const
{
	ycVec3 ptOnTri;
	ycGeo::ClosestPointTriLineSeg( *this, lineSeg, &ptOnTri, closestOnLine );
	return ptOnTri;
}

ycVec3 ycTri::ClosestPointToEdge( const ycLineSeg & lineSeg, f32 * lineDist ) const
{
	ycVec3 ptOnTri;
	ycGeo::ClosestPointTriLineSegToEdge( *this, lineSeg, &ptOnTri, lineDist );
	return ptOnTri;
}
	
ycVec3 ycTri::ClosestPointToEdge( const ycRay & ray, f32 * rayDist ) const
{
	ycVec3 ptOnTri;
	ycGeo::ClosestPointTriRayToEdge( *this, ray, &ptOnTri, rayDist );
	return ptOnTri;
}

f32 ycTri::DistSq( const ycVec3& pt ) const
{
	return ycGeo::DistSqTriPoint( *this, pt );
}

f32 ycTri::Dist( const ycVec3& pt ) const
{
	return ycGeo::DistTriPoint( *this, pt ); 
}

bool ycTri::Contains( const ycVec3 & pt ) const
{
	return ycGeo::ContainsTriPt( *this, pt );
}
	
bool ycTri::Intersects( const ycAABB& aabb ) const
{
	return ycGeo::IntersectsAABBTri( aabb, *this );
}

bool ycTri::Intersects( const ycTri& tri ) const
{
	return ycGeo::IntersectsTriTri( *this, tri );
}

bool ycTri::Intersects( const ycLineSeg & lineSeg ) const
{
	return ycGeo::IntersectsTriLineSeg( *this, lineSeg );
}

bool ycTri::Intersects( const ycPlane & plane ) const
{
	return ycGeo::IntersectsPlaneTri( plane, *this );
}

bool ycTri::Intersects( const ycSphere & sphere ) const
{
	return ycGeo::IntersectsSphereTri( sphere, *this );
}

bool ycTri::Intersection( ycGeoRayTriHit* hit, const ycRay& ray, f32 maxT ) const
{
	return ycGeo::IntersectionRayTri( hit, ray, maxT, *this );
}

bool ycTri::Intersection( ycGeoRayTriHit* hit, const ycLineSeg& lineSeg ) const
{
	return ycGeo::IntersectionLineSegTri( hit, lineSeg, *this );
}

bool ycTri::Intersection( ycVec3* pt_col, const ycLine& line ) const
{
	return ycGeo::IntersectionLineTri( pt_col, line, *this );
}

bool ycTri::Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycLineSeg & lineSeg ) const
{
	return ycGeo::SweepLineSegTri( fraction, normal, lineSeg, *this, mySweep );
}

bool ycTri::Sweep( f32 * fraction, ycVec3 * normal, const ycLineSeg & mySweep, const ycAABB & other, const ycLineSeg & otherSweep ) const
{
	return ycGeo::SweepAABBTri( fraction, normal, other, otherSweep, *this, mySweep );
}

bool ycTri::Sweep( ycGeoRayTriHit* hit, const ycSphere& other, const ycVec3& otherSweep ) const
{
	return ycGeo::SweepSphereTri( hit, other, otherSweep, 1.0f, *this );
}
