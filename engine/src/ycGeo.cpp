// References used:
// -"Real-Time Collision Detection by Christer Ericson, published by Morgan Kaufmann Publishers
// copyright 2005 Elsevier Inc"
// -"Essential Mathematics for Games and Interactive Applications by James M. Van Verth and Lars M. Bishop, published by CRC Press, copyright 2016
#include "ycGeo.h"

#include "ycAABB.h"
#include "ycBox.h"
#include "ycCapsule.h"
#include "ycCircle.h"
#include "ycConeRay.h"
#include "ycFrustum.h"
#include "ycLine.h"
#include "ycLineSeg.h"
#include "ycMtx4.h"
#include "ycPlane.h"
#include "ycRay.h"
#include "ycShape.h"
#include "ycSphere.h"
#include "ycTri.h"
#include "ycVec3.h"

#include "ycPushDebugOpt.inc"

namespace
{
	float Dmnop( const ycVec3 *v, int m, int n, int o, int p )
	{
		return (v[m].x - v[n].x) * (v[o].x - v[p].x) + (v[m].y - v[n].y) * (v[o].y - v[p].y) + (v[m].z - v[n].z) * (v[o].z - v[p].z);
	}
}

static bool s_triMatchAxis = false;

void ycGeo::ClosestPointAABB( const ycAABB& aabb, const ycVec3& pt, ycVec3* ptOnAABB )
{
	ycVec3 min = aabb.GetMin();
	ycVec3 max = aabb.GetMax();	
	ptOnAABB->x = ycClamp( pt.x, min.x, max.x );
	ptOnAABB->y = ycClamp( pt.y, min.y, max.y );
	ptOnAABB->z = ycClamp( pt.z, min.z, max.z );
}

void ycGeo::ClosestPointSphere( const ycSphere& sphere, const ycVec3& pt, ycVec3* ptOnSphere )
{
	ycVec3 c = sphere.center;
	ycVec3 v_to_pt = pt - c;
	f32 r = sphere.radius;
	if( v_to_pt.MagSq() <= (r*r) )
	{
		( *ptOnSphere ) = pt;
	}
	else
	{
		v_to_pt.Normalize( );
		( *ptOnSphere ) = c + r*v_to_pt;
	}
}

void ycGeo::ClosestPointPlane( const ycPlane& pl, const ycVec3& pt, ycVec3* ptOnPlane )
{
	ycVec3 n = pl.normal;
	f32 d = pl.d;
	f32 t = ( ( d-n.Dot(pt)) ) / n.MagSq( );
	( *ptOnPlane ) = pt + t*n;
}

void ycGeo::ClosestPointCircle( const ycCircle& circle, const ycVec3 & pt, ycVec3* ptOnCircle )
{
	ycVec3 planePt = circle.ToPlane().Project( pt );
	ycVec3 diff = planePt - circle.center;
	if( diff == ycVec3::ZERO() )
	{
		*ptOnCircle = circle.center;
	}
	else
	{
		*ptOnCircle = circle.center + diff.ScaledToMag( circle.radius );
	}
}

void ycGeo::ClosestPointCapsule( const ycCapsule& capsule, const ycVec3& pt, ycVec3* ptOnCapsule )
{
	ycVec3 cpLine;
	ClosestPointLineSeg( ycLineSeg( capsule.bottom, capsule.top ), pt, &cpLine );
	if( pt.DistSq( cpLine ) <= (capsule.radius * capsule.radius) )
	{
		*ptOnCapsule = pt;
	}
	else
	{
		*ptOnCapsule = cpLine + (pt - cpLine).ScaledToMag( capsule.radius );
	}
}

void ycGeo::ClosestPointTri( const ycTri& tri, const ycVec3& pt, ycVec3* ptOnTri )
{
	// Special thanks to "Real-Time Collision Detection" by Christer Ericson
	// for it's guidance on pages 136 to 142, section 5.1.5
	ycVec3 a = tri.GetA( );
	ycVec3 b = tri.GetB( );
	ycVec3 c = tri.GetC( );
	ycVec3 ab = tri.GetAB( );
	ycVec3 ac = tri.GetAC( );
	ycVec3 ap = pt - a;
	ycVec3 bp = pt - b;
	ycVec3 cp = pt - c;
	// checking vertex regions
	// checking if outsize vertex region A
	f32 d1 = ab.Dot( ap );
	f32 d2 = ac.Dot( ap );
	if( d1 <= 0.f && d2 <= 0.f )
	{
		( *ptOnTri ) = a;
		return;
	}
	// checking if outsize vertex region B
	f32 d3 = ab.Dot( bp );
	f32 d4 = ac.Dot( bp );
	if( d3 >= 0.f && d4 <= d3 )
	{
		( *ptOnTri ) = b;
		return;
	}
	// checking if outsize vertex region C
	f32 d5 = ab.Dot( cp );
	f32 d6 = ac.Dot( cp );
	if( d6 >= 0.f && d5 <= d6 )
	{
		( *ptOnTri ) = c;
		return;
	}
	// check if on region outside AB
	f32 vc = d1*d4 - d3*d2;
	if( vc <= 0.f && d1 >= 0.f && d3 <= 0.f )
	{
		f32 t = d1 / ( d1 - d3 );
		( *ptOnTri ) = a + t * ab;
		return;
	}
	// check if on region outside AC
	f32 vb = d5*d2 - d1*d6;
	if( vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f )
	{
		f32 t = d2 / ( d2 - d6 );
		( *ptOnTri ) = a + t * ac;
		return;
	}
	;
	// check if on region outside BC
	f32 va = d3*d6 - d5*d4;
	if( va <= 0.0f && ( d4 - d3 ) >= 0.0f && ( d5 - d6 ) >= 0.0f )
	{
		f32 t = ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) );
		( *ptOnTri ) = b + t * ( c - b );
		return;
	}
	// else p is inside face region of the triangle.
	f32 denom = 1.0f / ( va + vb + vc );
	f32 t = vb * denom;
	f32 v = vc * denom;
	( *ptOnTri ) = a + t * ab + v * ac;
	return;
}

void ycGeo::ClosestPointTriLineSeg( const ycTri& tri, const ycLineSeg & lineSeg, ycVec3* ptOnTri, ycVec3 * closestOnLine/* = nullptr*/ )
{
	ycVec3 start = lineSeg.GetStart();
	ycVec3 end = lineSeg.GetEnd();
	f32 dist1, dist2, dist3, lineDist;
	ycVec3 closest1, closest2, closest3, linePt;

	ClosestPointTri( tri, start, &closest1 );
	ClosestPointTri( tri, end, &closest2 );
	ClosestPointTriLineSegToEdge( tri, lineSeg, &closest3, &lineDist );
	linePt = start.Lerp( end, lineDist );
	dist1 = closest1.DistSq( start );
	dist2 = closest2.DistSq( end );
	dist3 = closest3.DistSq( linePt );
	if( dist1 <= dist2 && dist1 <= dist3 )
	{
		if( closestOnLine )
		{
			*closestOnLine = start;
		}
		*ptOnTri = closest1;
	}
	else if( dist2 <= dist3 )
	{
		if( closestOnLine )
		{
			*closestOnLine = end;
		}
		*ptOnTri = closest2;
	}
	else
	{
		if( closestOnLine )
		{
			*closestOnLine = linePt;
		}
		*ptOnTri = closest3;
	}
}

void ycGeo::ClosestPointTriLineSegToEdge( const ycTri& tri, const ycLineSeg & lineSeg, ycVec3* ptOnTri, f32 * lineDist/* = nullptr*/ )
{
	ycVec3 _pt1, _pt2, _pt3;
	ycVec3 _lpt1, _lpt2, _lpt3;
	f32 dist1, dist2, dist3;
	ClosestPointsLineSegLineSeg( tri.GetEdge( 0 ), lineSeg, &_pt1, &_lpt1 );
	ClosestPointsLineSegLineSeg( tri.GetEdge( 1 ), lineSeg, &_pt2, &_lpt2 );
	ClosestPointsLineSegLineSeg( tri.GetEdge( 2 ), lineSeg, &_pt3, &_lpt3 );
	dist1 = _pt1.DistSq( _lpt1 );
	dist2 = _pt2.DistSq( _lpt2 );
	dist3 = _pt3.DistSq( _lpt3 );
	if( dist1 <= dist2 && dist1 <= dist3 )
	{
		if( lineDist )
		{
			*lineDist = dist1;
		}
		*ptOnTri = _pt1;
	}
	else if( dist2 <= dist3 )
	{
		if( lineDist )
		{
			*lineDist = dist2;
		}
		*ptOnTri = _pt2;
	}
	else
	{
		if( lineDist )
		{
			*lineDist = dist3;
		}
		*ptOnTri = _pt3;
	}
}

void ycGeo::ClosestPointTriRayToEdge( const ycTri& tri, const ycRay& ray, ycVec3* ptOnTri, f32* rayDist/* = nullptr*/ )
{
	ycVec3 _pt1, _pt2, _pt3;
	ycVec3 _lpt1, _lpt2, _lpt3;
	f32 dist1, dist2, dist3;
	ClosestPointsLineSegRay( tri.GetEdge( 0 ), ray, &_pt1, &_lpt1 );
	ClosestPointsLineSegRay( tri.GetEdge( 1 ), ray, &_pt2, &_lpt2 );
	ClosestPointsLineSegRay( tri.GetEdge( 2 ), ray, &_pt3, &_lpt3 );
	dist1 = _pt1.DistSq( _lpt1 );
	dist2 = _pt2.DistSq( _lpt2 );
	dist3 = _pt3.DistSq( _lpt3 );
	if( dist1 <= dist2 && dist1 <= dist3 )
	{
		if( rayDist )
		{
			*rayDist = dist1;
		}
		*ptOnTri = _pt1;
	}
	else if( dist2 <= dist3 )
	{
		if( rayDist )
		{
			*rayDist = dist2;
		}
		*ptOnTri = _pt2;
	}
	else
	{
		if( rayDist )
		{
			*rayDist = dist3;
		}
		*ptOnTri = _pt3;
	}
}

void ycGeo::ClosestPointRay( const ycRay& r, const ycVec3& p, ycVec3* pt_out )
{
	// naming taken from section 12.2.3 of Essential Mathematics for Games and Interactive Application
	// third edition by James M. Van Verth, and Lars M. Bishop.
	ycVec3 s0 = r.pos;
	ycVec3 v = r.dir;
	ycVec3 w = p - s0;
	f32    t = w.Dot( v ) / v.Dot( v );// if the Ray's v is gauranteed to be normalized this can be optimized
	if( t <= 0.f )
	{
		( *pt_out ) = s0;
	}
	else
	{
		( *pt_out ) = s0 + ( t * v );
	}
}

void ycGeo::ClosestPointLine( const ycLine& l, const ycVec3& p, ycVec3* pt_out )
{
	// naming taken from section 12.2.1 of Essential Mathematics for Games and Interactive Application
	// third edition by James M. Van Verth, and Lars M. Bishop. Using the formula for implementation
	ycVec3 s0 = l.GetOrigin( );
	ycVec3 v = l.GetSlope( );
	ycVec3 w = p - l.GetOrigin( );
	f32    t = w.Dot( v ) / v.Dot( v );
	(*pt_out) = s0 + ( t * v );
}

void ycGeo::ClosestPointLineSeg( const ycLineSeg & lineSeg, const ycVec3& pt, ycVec3 * potOnLineSeg )
{
	f32 length = lineSeg.offset.Mag();
	if( length == 0.0f )
	{
		// trap for segment of length 0
		*potOnLineSeg = lineSeg.start;
		return;
	}

	ycVec3 dir = lineSeg.offset / length;
	f32 dot = dir.Dot( pt - lineSeg.start );

	if( dot < 0.0f)
	{
		*potOnLineSeg = lineSeg.start;
	}
	else if(dot > length)
	{
		*potOnLineSeg = lineSeg.GetEnd();
	}
	else
	{
		*potOnLineSeg = lineSeg.start + dir * dot;
	}
}

void ycGeo::ClosestPointLineSeg( const ycLineSeg & lineSeg, const ycVec3& pt, f32 * point )
{
	f32 length = lineSeg.offset.Mag();
	if( length == 0.0f )
	{
		*point = 0.0f;
	}
	else
	{
		ycVec3 dir = lineSeg.offset / length;
		f32 dot = dir.Dot( pt - lineSeg.start );

		if( dot < 0.0f )
		{
			*point = 0.0f;
		}
		else if( dot > length )
		{
			*point = 1.0f;
		}
		else
		{
			*point = dot / length;
		}
	}
}

void ycGeo::ClosestPointPlaneLineSeg( const ycPlane& plane, const ycLineSeg & lineSeg, ycVec3 * potOnLineSeg )
{
	f32 hit;
	if( IntersectionLineSegPlane( &hit, lineSeg, plane ) )
	{
		*potOnLineSeg = lineSeg.Lerp( hit );
	}
	else
	{
		if( DistPlanePoint( plane, lineSeg.GetStart() ) < DistPlanePoint( plane, lineSeg.GetEnd() ) )
		{
			*potOnLineSeg = plane.Project( lineSeg.GetStart() );
		}
		else
		{
			*potOnLineSeg = plane.Project( lineSeg.GetEnd() );
		}
	}
}

bool ycGeo::ClosestPointLineLine( const ycVec3& p0, const ycVec3& d0, const ycVec3& p1, const ycVec3& d1, ycVec2* hit )
{
	ycVec3 p1p0 = p0 - p1;
	f32 a = ( p1p0 ).Dot( d0 );
	f32 b = ( p1p0 ).Dot( d1 );
	f32 c = d0.Dot( d0 );
	f32 d = d0.Dot( d1 );
	f32 e = d1.Dot( d1 );

	f32 dnm = d*d - c*e;
	if( ycAbs( dnm ) > YC_F32_MIN )
	{
		if( hit ) { *hit = ycVec2( ( a*e - b*d ) / dnm, ( a*d - c*b ) / dnm ); }
		return true;
	}

	// Lines are parallel, return a valid pair of closest points

	if( hit )
	{
		// both lines are paralell so find an overlapping area
		f32 s0 = 0.0f;
		f32 s1 = 1.0f;
		f32 t0 = -a / c;
		f32 t1 = t0 + d / c;
		if( t1 < t0 ) { f32 tmp = t0; t0 = t1; t1 = tmp; }
		// find the middle point m, either between the ranges or the middle of the range if overlapping
		f32 m;
		if( s0 >= t0 && s0 <= t1 ) { m = s0; }
		else if( t0 > s1 ) { m = 0.5f * ( s1 + t0 ); }
		else if( t1 < s0 ) { m = 0.5f * ( s0 + t1 ); }
		else { m = 0.5f * ( ycMax( s0, t0 ) + ycMin( s1, t1 ) ); } 	// there is overlap, return the center of the overlap
		hit->x = m;
		hit->y = ( p0 + m * d0 - p1 ).Dot( d1 ) / e;
	}
	return false;
}

void ycGeo::ClosestPointsLineLine( const ycLine& line_a, const ycLine& line_b, ycVec3* ptOnA, ycVec3* ptOnB, f32* dist1 /*= nullptr*/, f32* dist2 /*= nullptr*/ )
{
	// using naming from section 12.2.5 from 
	// "Essential Mathematics for Games and Interactive Applications third Edition"
	// by James M. Van Verth, and Lars M. Bishop.

	// special case of two parallel lines.
	ycVec3 a_origin = line_a.GetOrigin( );
	ycVec3 a_slope = line_a.GetSlope( );
	ycVec3 b_origin = line_b.GetOrigin( );
	ycVec3 b_slope = line_b.GetSlope( );
	ycVec3 w0 =  a_origin - b_origin;

	f32 a = a_slope.Dot( a_slope );
	f32 b = a_slope.Dot( b_slope );
	f32 c = b_slope.Dot( b_slope );
	f32 d = a_slope.Dot( w0 );
	f32 e = b_slope.Dot( w0 );
	f32 denom = ( a * c ) - ( b * b );
	if( ycIsZero( denom ) )
	{
		f32 t1 = -( d / a );
		f32 t2 =  ( e / c );

		if( t1 >= 0.0f && t1 <= 1.0f )
		{
			( *ptOnA ) = a_origin + a_slope * t1;
			( *ptOnB ) = b_origin;
			if( dist1 ) { *dist1 = t1; }
			if( dist2 ) { *dist2 = 0.0f; }
		}
		else
		{
			( *ptOnA ) = a_origin;
			( *ptOnB ) = b_origin + b_slope * t2;
			if( dist1 ) { *dist1 = 0.0f; }
			if( dist2 ) { *dist2 = t2; }
		}

	}
	else
	{
		f32 s = ( ( b*e ) - ( c*d) )/ denom;
		f32 t = ( ( a*e ) - ( b*d) ) / denom;
		( *ptOnA ) = a_origin + ( s * a_slope );
		( *ptOnB ) = b_origin + ( t * b_slope );
		if( dist1 ) { *dist1 = s; }
		if( dist2 ) { *dist2 = t; }
	}
}

void ycGeo::ClosestPointsLineSegLineSeg( const ycLineSeg & lineSegA, const ycLineSeg & lineSegB, ycVec3 * pointOnLineSegA, ycVec3 * pointOnLineSegB, f32 * dist1 /*= nullptr*/, f32 * dist2 /*= nullptr*/ )
{
	const ycVec3 v[4] = { lineSegA.GetStart(), lineSegA.GetEnd(), lineSegB.GetStart(), lineSegB.GetEnd() };
	f32 d0232 = Dmnop( v, 0, 2, 3, 2 );
	f32 d3210 = Dmnop( v, 3, 2, 1, 0 );
	f32 d3232 = Dmnop( v, 3, 2, 3, 2 );

	f32 returnDist, returnDist2;

	f32 denom = (Dmnop( v, 1, 0, 1, 0 ) * Dmnop( v, 3, 2, 3, 2 ) - Dmnop( v, 3, 2, 1, 0 ) * Dmnop( v, 3, 2, 1, 0 ));
	if( denom < 0.000001f )
	{
		// parallel - we probably have to solve in one dimension
		f32 tB0 = (lineSegB.GetStart() - lineSegA.GetStart()).Dot( lineSegA.GetOffset() );
		f32 tB1 = (lineSegB.GetEnd()   - lineSegA.GetStart()).Dot( lineSegA.GetOffset() );

		// i'm sure there's a smarter way to do this, but enumerating the cases is obvious
		if( (tB0 <= 0.0f && tB1 >= 1.0f) || (tB1 <= 0.0f && tB0 >= 1.0f) )
		{
			// b interval contains a - just use a0
			returnDist = 0.0f;
			returnDist2 = lineSegB.offset.Dot( lineSegA.pos - lineSegB.pos ) / lineSegB.offset.MagSq();
		}
		else if( tB0 >= 0.0f && tB0 <= 1.0f )
		{
			// there is overlap, and b start is in a segment
			returnDist = tB0;
			returnDist2 = 0.0f;
		}
		else if( tB1 >= 0.0f && tB1 <= 1.0f )
		{
			// there is overlap, and b end is in a segment
			returnDist = tB1;
			returnDist2 = 1.0f;
		}
		else
		{
			// no overlap - pick the closest points on the interval
			if( tB0 > 0.0f )
			{
				returnDist = 1.0f;
				returnDist2 = tB1 > tB0 ? 0.0f : 1.0f;
			}
			else
			{
				returnDist = 0.0f;
				returnDist2 = tB1 > tB0 ? 1.0f : 0.0f;
			}
		}
	}
	else
	{
		f32 mu = (d0232 * d3210 - Dmnop( v, 0, 2, 1, 0 ) * d3232) / denom;

		returnDist = mu;
		returnDist2 = (d0232 + mu * d3210) / d3232;

		if( returnDist < 0.0f || returnDist > 1.0f || returnDist2 < 0.0f || returnDist2 > 1.0f )
		{
			// closest point on the lines is off the segments - check the four seg ends and use the closest

			f32 bestDistSq = YC_F32_MAX;
			f32 t; ycVec3 closest;

			ycGeo::ClosestPointLineSeg( lineSegA, lineSegB.GetStart(), &t ); closest = lineSegA.GetPoint( t );
			if( lineSegB.GetStart().DistSq( closest ) < bestDistSq )
			{
				bestDistSq = lineSegB.GetStart().DistSq( closest );
				returnDist = t;
				returnDist2 = 0.0f;
			}
			ycGeo::ClosestPointLineSeg( lineSegA, lineSegB.GetEnd(), &t ); closest = lineSegA.GetPoint( t );
			if( lineSegB.GetEnd().DistSq( closest ) < bestDistSq )
			{
				bestDistSq = lineSegB.GetEnd().DistSq( closest );
				returnDist = t;
				returnDist2 = 1.0f;
			}

			ycGeo::ClosestPointLineSeg( lineSegB, lineSegA.GetStart(), &t ); closest = lineSegB.GetPoint( t );
			if( lineSegA.GetStart().DistSq( closest ) < bestDistSq )
			{
				bestDistSq = lineSegA.GetStart().DistSq( closest );
				returnDist = 0.0f;
				returnDist2 = t;
			}
			ycGeo::ClosestPointLineSeg( lineSegB, lineSegA.GetEnd(), &t ); closest = lineSegB.GetPoint( t );
			if( lineSegA.GetEnd().DistSq( closest ) < bestDistSq )
			{
				bestDistSq = lineSegA.GetEnd().DistSq( closest );
				returnDist = 1.0f;
				returnDist2 = t;
			}

		}

		returnDist2 = returnDist2 < 0.0f ? 0.0f : (returnDist2 > 1.0f ? 1.0f : returnDist2);
	}

	
	if( dist1 ) { *dist1 = returnDist; }
	if( dist2 ) { *dist2 = returnDist2; }
	*pointOnLineSegA = lineSegA.GetPoint( returnDist );
	*pointOnLineSegB = lineSegB.GetPoint( returnDist2 );
}

void ycGeo::ClosestPointsLineSegRay( const ycLineSeg& seg, const ycRay& ray, ycVec3* ptOnSeg, ycVec3* ptOnRay )
{
	ycVec3 a_origin = seg.pos;
	ycVec3 a_u = seg.offset;
	ycVec3 b_origin = ray.pos;
	ycVec3 b_v = ray.dir;
	ycVec3 w0 = a_origin - b_origin;

	f32 a = a_u.Dot( a_u );
	f32 b = a_u.Dot( b_v );
	f32 c = b_v.Dot( b_v );
	f32 d = a_u.Dot( w0 );
	f32 e = b_v.Dot( w0 );
	f32 denom = ( a * c ) - ( b * b );

	f32 s;
	f32 t;
	if( ycIsZero( denom ) )
	{
		s = 0.f;
		t = ( e / c );
	}
	else
	{
		s = ( ( b * e ) - ( c * d ) ) / denom;
		t = ( ( a * e ) - ( b * d ) ) / denom;
	}
	// check to see if s and t are between 0 and 1
	if( t < 0.f )
	{
		( *ptOnRay ) = b_origin;
		ClosestPointLineSeg( seg, b_origin, ptOnSeg );
	}
	else if( s < 0.f )
	{
		( *ptOnSeg ) = a_origin;
		ClosestPointRay( ray, a_origin, ptOnRay );
	}
	else if( s > 1.f )
	{
		( *ptOnSeg ) = a_origin + a_u;
		ClosestPointRay( ray, a_origin + a_u, ptOnRay );
	}
	else
	{
		( *ptOnSeg ) = a_origin + ( s * a_u );
		( *ptOnRay ) = b_origin + ( t * b_v );
	}
}

void ycGeo::ClosestPointsRayRay( const ycRay& ray_a, const ycRay& ray_b, ycVec3* ptOnA, ycVec3* ptOnB )
{
	// Used section 12.2.6 from 
	// "Essential Mathematics for Games and Interactive Applications third Edition"
	// by James M. Van Verth, and Lars M. Bishop.
	// for guidance
	ycVec3 a_origin = ray_a.pos;
	ycVec3 a_u = ray_a.dir;
	ycVec3 b_origin = ray_b.pos;
	ycVec3 b_v = ray_b.dir;
	ycVec3 w0 =  a_origin - b_origin;

	f32 a = a_u.Dot( a_u );
	f32 b = a_u.Dot( b_v );
	f32 c = b_v.Dot( b_v );
	f32 d = a_u.Dot( w0 );
	f32 e = b_v.Dot( w0 );
	f32 denom = ( a * c ) - ( b * b );
	f32 s;
	f32 t;
	if( ycIsZero( denom ) )
	{
		s = 0.f;
		t = ( e / c );
	}
	else
	{
		s = ( ( b*e ) - ( c*d) )/ denom;
		t = ( ( a*e ) - ( b*d) ) / denom;
	}
	// check to see if s and t are between 0 and 1
	if( s >= 0.f && t >= 0.f )
	{
		( *ptOnA ) = a_origin + ( s * a_u );
		( *ptOnB ) = b_origin + ( t * b_v );
	}
	else
	{
		if( s < 0.f )
		{
			( *ptOnA ) = a_origin;
			ClosestPointRay( ray_b, a_origin, ptOnB );
		}
		else if( t < 0.f )
		{
			( *ptOnB ) = b_origin;
			ClosestPointRay( ray_a, b_origin, ptOnA );
		}
	}
}

ycVec2 ycGeo::ClosestPointsRayRay( const ycRay& ray_a, const ycRay& ray_b )
{
	f32 a = ray_a.dir.Dot( ray_a.pos - ray_b.pos );
	f32 b = ray_b.dir.Dot( ray_a.pos - ray_b.pos );
	f32 c = ray_a.dir.Dot( ray_a.dir );
	f32 d = ray_b.dir.Dot( ray_a.dir );
	f32 e = ray_b.dir.Dot( ray_b.dir );

	f32 denom = d*d - c*e;
	if( denom > YC_F32_MIN || denom < -YC_F32_MIN )
	{
		f32 a0 = ( a * e - b * d ) / denom;
		f32 a1 = ( a * d - c * b ) / denom;
		return ycVec2( a0, a1 );
	}
	return ycVec2( 0, 0 );
}

bool ycGeo::ClosestPointsLineRay( const ycLine& line, const ycRay& ray, ycVec3* ptOnLine, ycVec3* ptOnRay )
{
	// Adapted from theory in
	// "Essential Mathematics for Games and Interactive Applications third Edition"
	// by James M. Van Verth, and Lars M. Bishop.
	// for guidance
	ycVec3 line_origin = line.GetOrigin( );
	ycVec3 line_slope = line.GetSlope( );
	ycVec3 ray_origin = ray.pos;
	ycVec3 ray_slope = ray.dir;
	ycVec3 w0 =  line_origin - ray_origin;

	f32 a = line_slope.Dot( line_slope );
	f32 b = line_slope.Dot( ray_slope );
	f32 c = ray_slope.Dot( ray_slope );
	f32 d = line_slope.Dot( w0 );
	f32 e = ray_slope.Dot( w0 );
	f32 denom = ( a * c ) - ( b * b );
	f32 t_line;
	f32 t_ray;
	if( ycIsZero( denom ) )
	{
		t_line = 0.f;
		t_ray = ( e / c );
	}
	else
	{
		t_line = ( ( b*e ) - ( c*d ) ) / denom;
		t_ray = ( ( a*e ) - ( b*d ) ) / denom;
	}
	// check to see if s and t are between 0 and 1
	if( t_ray >= 0.f  )
	{
		( *ptOnLine ) = line_origin + ( t_line * line_slope );
		( *ptOnRay ) = ray_origin + ( t_ray * ray_slope );
		return true;
	}
	else
	{
		( *ptOnRay ) = ray_origin;
		ClosestPointLine( line, ray_origin, ptOnLine );
		return false;
	}
}

bool ycGeo::ClosestPointsRayLine( const ycRay& ray, const ycLine& line, ycVec3* ptOnRay, ycVec3* ptOnLine )
{
	return ClosestPointsLineRay( line, ray, ptOnLine, ptOnRay );
}

void ycGeo::ClosestPointsSphereSphere( const ycSphere& a, const ycSphere& b, ycVec3* cpOnA, ycVec3* cpOnB )
{
	ycVec3 a_center = a.center;
	f32 a_r = a.radius;
	ycVec3 b_center = b.center;
	f32 b_r = b.radius;
	ycVec3 dirAtoB = b_center - a_center;
	dirAtoB.Normalize( );
	( *cpOnA ) = a_center + dirAtoB * a_r;
	( *cpOnB ) = b_center - dirAtoB * b_r;
}

void ycGeo::ClosestPointBox( const ycMtx4& box, const ycVec3& pt, ycVec3* ptOnBox )
{
	ycVec3 delta = pt - box.t.xyz();

	f32 xProj = box.x.xyz().Dot( delta );
	f32 yProj = box.y.xyz().Dot( delta );
	f32 zProj = box.z.xyz().Dot( delta );

	if( ycAbs( xProj ) > 0.0f ) { xProj /= box.x.xyz().LengthSq(); }
	if( ycAbs( yProj ) > 0.0f ) { yProj /= box.y.xyz().LengthSq(); }
	if( ycAbs( zProj ) > 0.0f ) { zProj /= box.z.xyz().LengthSq(); }

	xProj = ycClamp( xProj, -1.0f, 1.0f );
	yProj = ycClamp( yProj, -1.0f, 1.0f );
	zProj = ycClamp( zProj, -1.0f, 1.0f );
	
	*ptOnBox = box.t.xyz() + xProj * box.x.xyz() + yProj * box.y.xyz() + zProj * box.z.xyz();
}

/////////////////////////
// YC_GEO::MIN_DIST_SQ //
/////////////////////////
f32 ycGeo::DistSqRayPoint( const ycRay& ray, const ycVec3& pt)
{
	ycVec3 closestPoint;
	ClosestPointRay( ray, pt, &closestPoint );
	return closestPoint.DistSq( pt );
}

f32 ycGeo::DistRayPoint( const ycRay& ray, const ycVec3& pt)
{
	return ycSqrt( DistSqRayPoint( ray, pt ) );
}

f32 ycGeo::DistSqLineSegPoint( const ycLineSeg& lineSeg, const ycVec3& pt )
{
	ycVec3 closestPoint;
	ClosestPointLineSeg( lineSeg, pt, &closestPoint );
	return closestPoint.DistSq( pt );
}

f32 ycGeo::DistLineSegPoint( const ycLineSeg& lineSeg, const ycVec3& pt )
{
	ycVec3 closestPoint;
	ClosestPointLineSeg( lineSeg, pt, &closestPoint );
	return closestPoint.Dist( pt );
}

f32 ycGeo::DistSqLinePoint( const ycLine& line, const ycVec3& pt )
{
	ycVec3 closestPoint;
	ClosestPointLine( line, pt, &closestPoint );
	return closestPoint.DistSq( pt );
}

f32 ycGeo::DistLinePoint( const ycLine& line, const ycVec3& pt)
{
	return ycSqrt( DistSqLinePoint( line, pt ) );
}

f32 ycGeo::DistSqLineSegLineSeg( const ycLineSeg& a, const ycLineSeg& b )
{
	ycVec3 cp_a;
	ycVec3 cp_b;
	ClosestPointsLineSegLineSeg( a, b, &cp_a, &cp_b );
	return cp_a.DistSq( cp_b );
}

f32 ycGeo::DistLineSegLineSeg( const ycLineSeg& a, const ycLineSeg& b )
{
	return ycSqrt( DistSqLineSegLineSeg( a, b ) );
}

f32 ycGeo::DistSqLineLine( const ycLine& a, const ycLine& b )
{
	ycVec3 cp_a;
	ycVec3 cp_b;
	ClosestPointsLineLine( a, b, &cp_a, &cp_b );
	return cp_a.DistSq( cp_b );
}

f32 ycGeo::DistLineLine( const ycLine& a, const ycLine& b )
{
	return ycSqrt( DistSqLineLine( a, b ) );
}

f32 ycGeo::DistSqSphereSphere( const ycSphere& a, const ycSphere& b )
{
	ycVec3 cp_a; // cp read as closest point
	ycVec3 cp_b;
	ClosestPointsSphereSphere( a, b, &cp_a, &cp_b );
	return cp_a.DistSq( cp_b );
}

f32 ycGeo::DistSphereSphere( const ycSphere& a, const ycSphere& b )
{
	return ycSqrt( DistSqSphereSphere( a, b ) );
}

f32 ycGeo::DistSqRayRay( const ycRay& ray_a, const ycRay& ray_b)
{
	ycVec3 cp_a;
	ycVec3 cp_b;
	ClosestPointsRayRay( ray_a, ray_b, &cp_a, &cp_b );
	return cp_a.DistSq( cp_b );
}

f32 ycGeo::DistRayRay( const ycRay& ray_a, const ycRay& ray_b)
{
	return ycSqrt( DistSqRayRay( ray_a, ray_b ) );
}

f32 ycGeo::DistSqRayLine( const ycRay& ray, const ycLine& line )
{
	return DistSqLineRay( line, ray );
}

f32 ycGeo::DistRayLine( const ycRay& ray, const ycLine& line )
{
	return DistLineRay( line, ray );
}

f32 ycGeo::DistSqLineRay( const ycLine& line, const ycRay& ray )
{
	ycVec3 cp_line;
	ycVec3 cp_ray;
	ClosestPointsLineRay( line, ray, &cp_line, &cp_ray );
	return cp_line.DistSq( cp_ray );
}

f32 ycGeo::DistLineRay( const ycLine& line, const ycRay& ray )
{
	return ycSqrt( DistSqLineRay( line, ray ) );
}

f32 ycGeo::DistSqAABBPoint( const ycAABB& aabb, const ycVec3& pt )
{
	ycVec3 cp;
	ClosestPointAABB( aabb, pt, &cp );
	return pt.DistSq( cp );
}

f32 ycGeo::DistAABBPoint( const ycAABB& aabb, const ycVec3& pt )
{
	return ycSqrt( DistSqAABBPoint( aabb, pt ) );
}

f32 ycGeo::DistSqAABBSphere( const ycAABB& aabb, const ycSphere& sphere )
{
	ycVec3 cp;
	ClosestPointAABB( aabb, sphere.center, &cp );
	return sphere.center.DistSq( cp ) - sphere.radius*sphere.radius;
}

f32 ycGeo::DistAABBSphere( const ycAABB& aabb, const ycSphere& sphere )
{
	ycVec3 cp;
	ClosestPointAABB( aabb, sphere.center, &cp );
	return sphere.center.Dist( cp ) - sphere.radius;
}

f32 ycGeo::DistAABBPlane( const ycAABB& aabb, const ycPlane& plane )
{
	f32 radius = aabb.extents.x + ycAbs( plane.normal.x ) + aabb.extents.y + ycAbs( plane.normal.y ) + aabb.extents.z + ycAbs( plane.normal.z );
	return ycAbs( plane.SignedDistance( aabb.center ) ) <= radius;
}

f32 ycGeo::DistSqSpherePoint( const ycSphere& sphere, const ycVec3& pt)
{
	ycVec3 cp;
	ClosestPointSphere( sphere, pt, &cp );
	return pt.DistSq( cp );
}

f32 ycGeo::DistSpherePoint( const ycSphere& sphere, const ycVec3& pt)
{
	return ycSqrt( DistSqSpherePoint( sphere, pt ) );
}

f32 ycGeo::DistSqSphereLineSeg( const ycSphere& sphere, const ycLineSeg& lineSeg )
{
	ycVec3 closest;
	ClosestPointLineSeg( lineSeg, sphere.center, &closest );
	return closest.DistSq( sphere.center ) - (sphere.radius*sphere.radius);
}

f32 ycGeo::DistSphereLineSeg( const ycSphere& sphere, const ycLineSeg& lineSeg )
{
	ycVec3 closest;
	ClosestPointLineSeg( lineSeg, sphere.center, &closest );
	return closest.Dist( sphere.center ) - (sphere.radius);
}

f32 ycGeo::DistSqSphereTri( const ycSphere& sphere, const ycTri& tri )
{
	return DistSqTriPoint( tri, sphere.center ) - sphere.radius*sphere.radius;
}

f32 ycGeo::DistSphereTri( const ycSphere& sphere, const ycTri& tri )
{
	return DistTriPoint( tri, sphere.center ) - sphere.radius;
}

f32 ycGeo::DistSqTriPoint( const ycTri& tri, const ycVec3& pt )
{
	ycVec3 cp;
	ClosestPointTri( tri, pt, &cp );
	return cp.DistSq( cp );
}

f32 ycGeo::DistTriPoint( const ycTri& tri, const ycVec3& pt )
{
	return ycSqrt( DistSqTriPoint( tri, pt ) );
}

f32 ycGeo::DistSqCirclePoint( const ycCircle& circle, const ycVec3& pt )
{
	ycVec3 diff1 = pt - circle.center;
	f32 dist = diff1.Dot( circle.normal );

	ycVec3 diff2 = diff1 - circle.normal * dist;
	f32 magSq = diff2.MagSq();

	f32 distSq;
	if( magSq <= YC_F32_EPSILON )
	{
		distSq = circle.radius * circle.radius + dist * dist;
	}
	else
	{
		distSq = diff2.MagSq();
	}
	return distSq;	
}

f32 ycGeo::DistCirclePoint( const ycCircle& circle, const ycVec3& pt )
{
	return ycSqrt( DistSqCirclePoint( circle, pt ) );	
}

f32 ycGeo::DistPlanePoint( const ycPlane& plane, const ycVec3& pt )
{
	return ycAbs( plane.SignedDistance( pt ) );
}

f32 ycGeo::DistPlaneLineSeg( const ycPlane& plane, const ycLineSeg& lineSeg )
{
	f32 distStart = plane.SignedDistance( lineSeg.GetStart() );
	f32 distEnd = plane.SignedDistance( lineSeg.GetEnd() );
	if( distStart * distEnd < 0.0f )
	{
		return 0.0f;
	}
	return ycMin( ycAbs( distStart ), ycAbs( distEnd ) );
}

f32 ycGeo::DistPlaneSphere( const ycPlane& plane, const ycSphere& sphere )
{
	return DistPlanePoint( plane, sphere.center ) - sphere.radius;
}

f32 ycGeo::DistPlaneCircle( const ycPlane& plane, const ycCircle& circle )
{
	f32 dist = plane.SignedDistance( circle.center );
	if( dist < -circle.radius )
	{
		return dist + circle.radius;
	}
	else if( dist > circle.radius )
	{
		return dist - circle.radius;
	}
	return 0.0f;
}

f32	ycGeo::DistSqCapsulePoint( const ycCapsule& capsule, const ycVec3 & pt )
{
	return DistSqLineSegPoint( ycLineSeg( capsule.top, capsule.bottom ), pt ) - capsule.radius*capsule.radius;
}

f32	ycGeo::DistCapsulePoint( const ycCapsule& capsule, const ycVec3 & pt )
{
	return DistLineSegPoint( ycLineSeg( capsule.top, capsule.bottom ), pt ) - capsule.radius;
}

f32	ycGeo::DistSqCapsuleLineSeg( const ycCapsule& capsule, const ycLineSeg & lineSeg )
{
	return DistSqLineSegLineSeg( ycLineSeg( capsule.top, capsule.bottom ), lineSeg ) - capsule.radius*capsule.radius;
}

f32	ycGeo::DistCapsuleLineSeg( const ycCapsule& capsule, const ycLineSeg & lineSeg )
{
	return DistLineSegLineSeg( ycLineSeg( capsule.top, capsule.bottom ), lineSeg ) - capsule.radius;
}

f32	ycGeo::DistSqCapsuleCapsule( const ycCapsule& capsule, const ycCapsule & other )
{
	return DistSqLineSegLineSeg( ycLineSeg( capsule.top, capsule.bottom ), ycLineSeg( other.top, other.bottom ) ) - capsule.radius*capsule.radius - other.radius*other.radius;
}

f32	ycGeo::DistCapsuleCapsule( const ycCapsule& capsule, const ycCapsule & other )
{
	return DistLineSegLineSeg( ycLineSeg( capsule.top, capsule.bottom ), ycLineSeg( other.top, other.bottom ) ) - capsule.radius - other.radius;
}

f32	ycGeo::DistSqCapsuleSphere( const ycCapsule& capsule, const ycSphere & sphere )
{
	return DistSqCapsulePoint( capsule, sphere.center ) - sphere.radius*sphere.radius;
}

f32	ycGeo::DistCapsuleSphere( const ycCapsule& capsule, const ycSphere & sphere )
{
	return DistCapsulePoint( capsule, sphere.center ) - sphere.radius;
}

f32	ycGeo::DistCapsulePlane( const ycCapsule& capsule, const ycPlane & plane )
{
	return DistPlaneLineSeg( plane, ycLineSeg( capsule.bottom, capsule.top ) ) - capsule.radius;
}

/////////////////////////////
// YC_GEO::IS_INTERSECTING //
/////////////////////////////
bool ycGeo::IntersectsPlanePlane( const ycPlane& p1, const ycPlane& p2 )
{
	// Special thanks to "Real-Time Collision Detection" by Christer Ericson
	// for it's guidance on pages 207 to 209, whose equations this implements
	// Here are the equations used from the text
	// Line of intersection = P + t*d; where P is a point on the line, and d is the direction
	// solving this system of linear equations we get (see book for more info)
	// denom = ( n1.dot(n2) * n2.dot(n2) - (n1.dot(n2)^2) )
	// k1 = ( d1*(n1.dot(n2)) - d2*(n1.dot(n2)) ) / denom; //d1 and d2 are plane.distance
	// k2 = ( d2(n1.dot(n1)) -  d1*(n2.dot(n2)) ) / denom;
	ycVec3 n1 = p1.normal;
	f32 d1 = p1.d;
	ycVec3 n2 = p2.normal;
	f32 d2 = p2.d;
	ycVec3 slope = ( n1 ).Cross( n2 ); // the direction of the intersection line

	if( ycIsZero( slope.MagSq( ) ) ) // planes are parallel
	{
		if( ycIsZero( d1 - d2 ) ) //planes are coincident
		{
			return true;
		}
		else // planes do not overlap at all
		{
			return false;
		}
	}
	return true;
}

bool ycGeo::IntersectsSphereSphere( const ycSphere& a, const ycSphere& b )
{
	ycVec3 a_center = a.center;
	f32 a_r = a.radius;
	ycVec3 b_center = b.center;
	f32 b_r = b.radius;
	return ( a_center.DistSq( b_center ) ) <= ( ( a_r + b_r ) * ( a_r + b_r ) );
}

bool ycGeo::IntersectsAABBAABB( const ycAABB& a, const ycAABB& b )
{
	// separating axis test. If separated along any axis then seperated.
	ycVec3 a_min = a.GetMin( );
	ycVec3 a_max = a.GetMax( );
	ycVec3 b_min = b.GetMin( );
	ycVec3 b_max = b.GetMax( );
	if( a_min.x > b_max.x || b_min.x > a_max.x )
	{
		return false;
	}
	if( a_min.y > b_max.y || b_min.y > a_max.y )
	{
		return false;
	}
	if( a_min.z > b_max.z || b_min.z > a_max.z )
	{
		return false;
	}
	return true;
}

bool ycGeo::IntersectsBoxBox( const ycBox& a, const ycBox& b )
{
	if( a.GetMin().x > b.GetMax().x || b.GetMin().x > a.GetMax().x )
	{
		return false;
	}
	if( a.GetMin().y > b.GetMax().y || b.GetMin().y > a.GetMax().y )
	{
		return false;
	}
	if( a.GetMin().z > b.GetMax().z || b.GetMin().z > a.GetMax().z )
	{
		return false;
	}
	return true;
}

bool ycGeo::IntersectsSpherePlane( const ycSphere& sphere, const ycPlane& plane )
{
	return IntersectsPlaneSphere( plane, sphere );
}

bool ycGeo::IntersectsSphereTri( const ycSphere& sphere, const ycTri& tri )
{
	ycVec3 ptOnTri;
	ClosestPointTri( tri, sphere.center, &ptOnTri );
	return ptOnTri.DistSq( sphere.center ) <= (sphere.radius * sphere.radius);
}

bool ycGeo::IntersectsSphereFrustum( const ycSphere& sphere, const ycFrustum& frustum )
{
	for( s32 i = 0; i < ycFrustum::kPlane_Count; i++)
	{
		const ycPlane& p = frustum.planes[ i ];
		if( !p.IsFinite() ) { continue; }

		f32 dist = p.SignedDistance( sphere.center );
		if( dist < -sphere.radius )
		{
			return false;
		}
		if( ycAbs( dist ) < sphere.radius )
		{
			return true;
		}
	}
	return true;
}

bool ycGeo::IntersectsPlaneSphere( const ycPlane& plane, const ycSphere& sphere )
{
	ycVec3 sc = sphere.center;
	f32 r = sphere.radius;
	ycVec3 pn = plane.normal;
	f32 pd = plane.d;
	f32 signedDistance = sc.Dot( pn ) - pd;
	return ycAbs( signedDistance ) <= r;
}

bool ycGeo::IntersectsPlaneTri( const ycPlane& plane, const ycTri& tri )
{
	f32 dist1 = plane.SignedDistance( tri.pt1 );
	f32 dist2 = plane.SignedDistance( tri.pt2 );
	f32 dist3 = plane.SignedDistance( tri.pt3 );
	return (dist1*dist2 <= 0.0f || dist2*dist3 <= 0.0f);
}

bool ycGeo::IntersectsAABBPlane( const ycAABB& aabb, const ycPlane& plane )
{
	return IntersectsPlaneAABB( plane, aabb );
}

bool ycGeo::IntersectsAABBTri( const ycAABB& aabb, const ycTri& tri )
{
	ycVec3 min = ycVec3::Min( ycVec3::Min( tri.GetA(), tri.GetB() ), tri.GetC() );
	ycVec3 max = ycVec3::Max( ycVec3::Max( tri.GetA(), tri.GetB() ), tri.GetC() );
	ycVec3 aMax = aabb.GetMax();
	ycVec3 aMin = aabb.GetMin();

	if(	   min.x >= aMax.x 
		|| max.x <= aMin.x
		|| min.y >= aMax.y 
		|| max.y <= aMin.y
		|| min.z >= aMax.z 
		|| max.z <= aMin.z )
	{
		return false;
	}

	ycVec3 triDirs[3] = { tri.GetB() - tri.GetA(), tri.GetC() - tri.GetA(), tri.GetC() - tri.GetB() };
	ycVec3 center = aabb.GetCenter();
	ycVec3 normal = triDirs[0].Cross( triDirs[1] );
	f32 normalD = normal.Dot( tri.GetA() - center );
	f32 radiusD = ycAbs( aabb.GetExtents().Dot( normal.Abs() ) );
	if( ycAbs( normalD ) >= radiusD )
	{
		return false;
	}

	f32 t1, t2, a1, a2;
	f32 length;
	ycVec3 axis;
	ycVec3 directions[] = { ycVec3::XAXIS(), ycVec3::YAXIS(), ycVec3::ZAXIS() };
	for( s32 i = 0; i < 3; i++ )
	{
		for( s32 j = 0; j < 3; j++ )
		{
			axis = triDirs[i].Cross( directions[j] );
			length = axis.MagSq();
			if( length <= YC_F32_EPSILON )
			{
				continue;
			}

			tri.AxisProjection(axis, &t1, &t2);
			aabb.AxisProjection(axis, &a1, &a2);
			if( t2 <= a1 || t1 >= a2 )
			{
				return false;
			}
		}
	}
	return true;
	/*
	f32 t1, t2, a1, a2;

	ycVec3 directions[] = { ycVec3::XAXIS(), ycVec3::YAXIS(), ycVec3::ZAXIS() };
	for( s32 i = 0; i < 3; ++i )
	{
		tri.AxisProjection( directions[i], &t1, &t2 );
		aabb.AxisProjection( directions[i], &a1, &a2 );
		if( t2 < a1 || a2 < t1 )
		{
			return false;
		}
	}

	ycVec3 normal;
#if YC_LHC_UNTESTED && 0
	normal = (tri.GetC() - tri.GetA()).Cross( tri.GetB() - tri.GetA() );
#else
	normal = (tri.GetB() - tri.GetA()).Cross( tri.GetC() - tri.GetA() );
#endif
	tri.AxisProjection(normal, &t1, &t2);
	aabb.AxisProjection(normal, &a1, &a2);
	if( t2 < a1 || a2 < t1 )
	{
		return false;
	}

	f32 length;
	ycVec3 axis;
	ycVec3 triDirs[3] = { tri.GetB() - tri.GetA(), tri.GetC() - tri.GetA(), tri.GetC() - tri.GetB() };
	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
		#if YC_LHC_UNTESTED && 0
			axis = triDirs[j].Cross( directions[i] );
		#else
			axis = directions[i].Cross( triDirs[j] );
		#endif
			length = axis.MagSq();
			if( length <= YC_F32_EPSILON )
			{
				continue;
			}

			tri.AxisProjection(axis, &t1, &t2);
			aabb.AxisProjection(axis, &a1, &a2);
			if( t2 < a1 || a2 < t1 )
			{
				return false;
			}
		}
	}
	return true;*/
}

bool ycGeo::IntersectsAABBLineSeg( const ycAABB& aabb, const ycLineSeg & line )
{
	ycVec3 lineDirAxisDP, lineDir, lineCenter, dir, cross, extents, center;
	
	center = aabb.GetCenter();
	extents = aabb.GetExtents();
	lineDir = ( line.offset ) * 0.5f;
	lineCenter = line.start + lineDir;
	dir = lineCenter - center;
	
	lineDirAxisDP.x = ycAbs( lineDir.x );
	if( ycAbs( dir.x ) > extents.x + lineDirAxisDP.x )
	{
		return false;
	}

	lineDirAxisDP.y = ycAbs( lineDir.y );
	if( ycAbs( dir.y ) > extents.y + lineDirAxisDP.y )
	{
		return false;
	}

	lineDirAxisDP.z = ycAbs( lineDir.z );
	if( ycAbs( dir.z ) > extents.z + lineDirAxisDP.z )
	{
		return false;
	}

	cross = lineDir.Cross( dir );
	if( ycAbs( cross.x ) > extents.y * lineDirAxisDP.z + extents.z * lineDirAxisDP.y )
	{
		return false;
	}
	if( ycAbs( cross.y ) > extents.x * lineDirAxisDP.z + extents.z * lineDirAxisDP.x )
	{
		return false;
	}
	if( ycAbs( cross.z ) > extents.x * lineDirAxisDP.y + extents.y * lineDirAxisDP.x )
	{
		return false;
	}

	return true;
}

bool ycGeo::IntersectsAABBRay( const ycAABB& aabb, const ycRay & ray, f32 maxT )
{
	return IntersectionRayAABB( nullptr, ray, maxT, aabb );
}

bool ycGeo::IntersectsAABBFrustum( const ycAABB& aabb, const ycFrustum& frustum )
{
	for( ureg i = 0; i < ycFrustum::kPlane_Count; ++i )
	{
		const ycPlane& p = frustum.planes[ i ];
		if( !p.IsFinite() ) { continue; }

		const ycVec3 abs{ ycAbs( p.a ), ycAbs( p.b ), ycAbs( p.c ) };
		const f32 dp = aabb.GetCenter().Dot( p.normal ) + aabb.GetExtents().Dot( abs );
		if( dp < p.d ) { return false; }
	}
	return true;
}

bool ycGeo::IntersectsTriTri( const ycTri& a, const ycTri& b )
{
	f32 dist1 = ycSqrt( DistSqTriPoint( a, b.GetA() ) );
	f32 dist2 = ycSqrt( DistSqTriPoint( a, b.GetB() ) );
	f32 dist3 = ycSqrt( DistSqTriPoint( a, b.GetC() ) );
	return (dist1*dist2 <= 0.0f || dist2*dist3 <= 0.0f);
}

bool ycGeo::IntersectionRayTri( ycGeoRayTriHit* hit, const ycRay& seg, f32 maxT, const ycTri& tri )
{
	// Used Real-Time Collision Detection by Christer Ericson, section 5.3.6 for reference/guidance
	ycVec3 ab = tri.GetB() - tri.GetA();
	ycVec3 ac = tri.GetC() - tri.GetA();
	ycVec3 p = seg.pos;
	ycVec3 q = p + seg.dir;
	ycVec3 qp = p - q;

	// Compute triangle normal. Can be precalculated or cached if
	// intersecting multiple segments against the same triangle
	ycVec3 n = ab.Cross( ac );

	// Compute denominator d. If d <= 0, segment is parallel to or points
	// away from triangle, so exit early
	f32 d = qp.Dot( n );
	if( d <= 0.0f ) return false;
	// Compute intersection t value of pq with plane of triangle. A ray
	// intersects iff 0 <= t. Segment intersects iff 0 <= t <= maxT. Delay
	// dividing by d until intersection has been found to pierce triangle
	ycVec3 ap = p - tri.GetA();
	f32 t = ap.Dot( n );
	if( t < 0.0f ) return false;

	// Compute barycentric coordinate components and test if within bounds
	ycVec3 e = qp.Cross( ap );

	f32 v = ac.Dot( e );
	if( v < 0.0f || v > d ) return false;
	f32 w = -ab.Dot( e );
	if( w < 0.0f || v + w > d ) return false;
	
	// Infinite ray intersects triangle. Perform delayed division and
	// compute the last barycentric coordinate component
	f32 ood = 1.0f / d;
	t *= ood;
	if( t > maxT ) return false;
	
	if( hit ) 
	{
		v *= ood;
		w *= ood;
		hit->uvw = ycVec3( 1 - v - w, v, w );
		hit->t = t;
		hit->pos = seg.GetPoint( t );
		hit->normal = n;
		hit->triIndex = 0;
	}
	return true;
}

bool ycGeo::IntersectionRayRect( const ycRay & ray, const ycVec3& ctr, const ycVec3& ext1, const ycVec3& ext2, f32* dist/* = nullptr*/, ycVec2* local /*= nullptr*/ )
{
	ycVec3 n = ext1.Cross( ext2 );

	f32 dd = ray.dir.Dot( n );
	if( ycAbs( dd ) <= YC_F32_MIN ) { return false; }
	f32 along = ( ctr - ray.pos ).Dot( n ) / dd;
	if( along > 0.0f )
	{
		ycVec3 h = ray.pos + along * ray.dir;
		ycVec3 o = h - ctr;
		f32 lx = o.Dot( ext1 ) / ext1.LengthSq();
		f32 ly = o.Dot( ext2 ) / ext2.LengthSq();
		if( ycAbs( lx ) < 1.0f && ycAbs( ly ) < 1.0f )
		{
			if( dist ) { *dist = along; }
			if( local ) { local->x = lx; local->y = ly; }
			return true;
		}
	}
	return  false;
}

bool ycGeo::IntersectionRayCuboid( const ycRay & ray, const ycMtx4& cuboid, f32* dist /*= nullptr*/, ycVec3* normal /*= nullptr*/ )
{
	ycVec3 c = cuboid.t.xyz();
	for( ureg a = 0; a < 3; ++a )
	{
		ycVec3 n = cuboid.GetAxis( a );
		f32 dd = ray.dir.Dot( n ); // direct d
		if( dd > 0.0f ) // if face is away from ray, chose the opposite face
		{
			dd = -dd;
			n.Negate();
		}
		if( dd < -YC_F32_MIN ) // always negative, exclude 0.0
		{
			ycVec3 p = c + n;
			f32 along = (p - ray.pos).Dot( n ) / dd;
			if( along >= 0.0f )
			{
				ycVec3 h = ray.pos + along * ray.dir; // hit on box plane here
				// check intersecting with the square in the plane of the opposite directions of the normal
				bool hit = true;
				for( ureg b = 0; b < 3; ++b )
				{
					if( b != a )
					{
						ycVec3 e = cuboid.GetAxis( b );
						if( ycAbs( e.Dot( h - p ) ) > e.LengthSq() )
						{
							hit = false;
							break;
						}
					}
				}
				if( hit )
				{
					if( dist ) { *dist = along; }
					if( normal ) { *normal = n; }
					return true;
				}
			}
		}
	}
	return false;
}

bool ycGeo::IntersectionRayPlane( const ycRay & ray, const ycVec3 center, const ycVec3 normal, ycVec3* point/* = nullptr*/, f32* dist/* = nullptr*/ )
{
	// ( i - center ) dot normal = 0
	// p = d*dir + pos
	// ( d*dir+pos - center) dot normal = 0
	// ( d*dir ) dot normal + ( pos - center ) dot normal = 0
	// d = ( pos-center ) dot normal / dir dot normal
	// i = pos + d * dir

	float t = ( center - ray.pos ).Dot( normal );
	float denom = ray.dir.Dot( normal );
	if( ycIsZero( denom ) )
		return false;

	t /= denom;

	if( dist )
		*dist = t;
	if( point )
		*point = ray.pos + t * ray.dir;

	return true;
}

bool ycGeo::IntersectionRayDisc( const ycRay & ray, const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point /*= nullptr*/, f32* dist /*= nullptr*/ )
{
	f32 tmpdist;
	ycVec3 tmppos;
	if( !IntersectionRayPlane( ray, center, normal, &tmppos, &tmpdist ) )
	{
		return false;
	}

	f32 d = ( tmppos - center ).LengthSq();
	if( d < rInner*rInner || d > rOuter*rOuter )
	{
		return false;
	}

	if( point )
	{
		*point = tmppos;
	}
	if( dist )
	{
		*dist = tmpdist;
	}

	return true;
}

bool ycGeo::IntersectionRayTorus( const ycRay & ray, const ycVec3 center, const ycVec3 normal, f32 rInner, f32 rOuter, ycVec3* point /*= nullptr*/, f32* dist /*= nullptr*/ )
{
	f32 radius = (rOuter-rInner)*0.5f;
	ycCircle circle( center, rInner + radius, normal );
	ycPlane plane = circle.ToPlane();
	ycVec3 intersection1, closest;
	f32 hit;
	f32 dotNormal = ycAbs( ray.dir.Dot( normal ) );
	bool intersection = ycGeo::IntersectionRayPlane( &hit, ray, YC_F32_MAX, plane );
	if( dotNormal <= 0.01f )//once it's close enough, our hack algorithm starts losing its meaning
	{
		f32 hit1, hit2;
		if( dotNormal <= YC_F32_EPSILON )
		{
			ycVec3 extents = ycVec3( radius ) + normal.Cross( ray.dir ).Abs() * rOuter;
			if( IntersectionRayAABB( &hit1, ray, YC_F32_MAX, ycAABB( circle.center, extents.x, extents.y, extents.z ) ) )
			{
				if( dist ) { *dist = hit1; }
				if( point ) { *point = ray.GetPoint( hit1 ); }
				return true;
			}
		}
		else if( IntersectionRaySphere( &hit1, &hit2, ray, YC_F32_MAX, ycSphere( circle.center, rOuter ) ) )
		{
			if( dist ) { *dist = hit1; }
			if( point ) { *point = ray.GetPoint( hit1 ); }
			return true;
		}
		return false;
	}
	else if( intersection )
	{
		intersection1 = ray.GetPoint( hit );
		ycGeo::ClosestPointCircle( circle, intersection1, &closest );
		radius /= ycAbs( ray.dir.Dot( normal ) );
		bool collide = closest.Dist( intersection1 ) <= radius;
		if( collide )
		{
			if( dist ) { *dist = hit; }
			if( point ) { *point = intersection1; }
		}
		return collide;
	}
	return false;
}

bool ycGeo::IntersectionRayCone( const ycRay & ray, const ycVec3& bottom, const ycVec3& tip, f32 radiusBottom, f32 radiusTip, f32* dist /*= nullptr*/ )
{
	ycVec3 P = bottom;
	ycVec3 V = tip - P;
	ycVec3 S = ray.pos;
	ycVec3 O = S - P;
	ycVec3 D = ray.dir;
	f32 f = D.Dot( V );
	f32 j = V.Dot( V );
	f32 e = O.Dot( V );

	// check endcaps
	if( f > 0.0f )
	{
		if( e < 0.0f )
		{
			f32 t = -e / f;
			ycVec3 I = t * D + O;
			if( I.LengthSq() < ycSquare( radiusBottom ) )
			{
				if( dist ) { *dist = t; }
				return true;
			}
		}
	}
	else if( e > j )
	{
		f32 t = ( j - e ) / f;
		ycVec3 I = O - V + t * D;
		if( I.LengthSq() < ycSquare( radiusTip ) )
		{
			if( dist ) { *dist = t; }
			return true;
		}
	}

	// check cone part
	f32 g = O.Dot( O );
	f32 h = D.Dot( D );
	f32 i = D.Dot( O );
	f32 j2 = j*j;
	f32 q = radiusTip - radiusBottom; // difference between the radius from A to B
	f32 q2 = q*q;
	f32 r = radiusBottom; // inner cylinder radius padded with sphere radius

	f32 a = j2*h - f*f*( j + q2 );
	f32 b = 2 * i*j2 - 2 * e*f*j - 2 * e*f*q2 - 2 * f*j*r*q;
	f32 c = j2*( g - r*r ) - e*e*( j + q2 ) - 2 * e*j*r*q;

	// check if line intersects double cone
	f32 dis = b * b - 4 * a * c;
	if( dis >= 0.0f )	// if negative then no intersect
	{
		f32 d = ycSqrt( dis );
		f32 t = -( b + d ) / ( 2.0f * a );
		// check that cone is in front of ray
		if( t > 0.0f )
		{	// make sure cone part is inbetween end caps
			ycVec3 T = S + t * D;
			f32 TPdV = ( T - P ).Dot( V );
			f32 TQdV = TPdV - j;
			if( TPdV > 0.0f && TQdV < 0 )
			{
				if( dist ) { *dist = t; }
				return true;
			}
		}
	}
	return false;
}

bool ycGeo::IntersectionRayLineXZ( const ycRay & ray, const ycVec3& A, const ycVec3& B, ycVec2* dist /*= nullptr*/ )
{
	ycVec3 O = ray.pos - A;
	ycVec3 V = B - A;
	ycVec3 D = ray.dir;
	f32 d = D.z * V.x - D.x * V.z;
	if( d > YC_F32_MIN || d < -YC_F32_MIN )
	{
		f32 t = ( O.x * V.z - O.z * V.x ) / d;
		if( t > 0.0f )
		{
			f32 k = ycAbs( V.x ) > ycAbs( V.z ) ? ( O.x + t * D.x ) / V.x : ( O.z + t * D.z ) / V.z;
			if( dist ) { dist->x = t; dist->y = k; }
			return true;
		}
	}

	return false;
}

bool ycGeo::IntersectionConeRayRing( const ycConeRay & ray, const ycVec3 center, const ycVec3 normal, f32 radius, ycVec3 *closest, f32* dist )
{
	f32 ddn = ray.dir.Dot( normal );
	if( ycAbs( ddn ) <= YC_F32_MIN ) { return false; }

	f32 along = ( ( center - ray.pos ).Dot( normal ) / normal.Dot( normal ) / ray.dir.Dot( normal ) );
	ycVec3 linePlane = ray.pos + along * ray.dir;
	ycVec3 ringClose = radius * (linePlane - center).Normalized() + center;

	f32 d2 = linePlane.DistSq( ringClose );
	f32 lineRadius = ray.base + along * ray.slope;

	if( d2 < ycSquare( lineRadius ) )
	{
		if( closest ) { *closest = ringClose; }
		if( dist ) { *dist = ycSqrt( d2 ) / lineRadius; } // normalize the distance by cone radius
		return true;
	}
	return false;
}

bool ycGeo::IntersectionConeRayPoint( const ycConeRay & ray, const ycVec3 point, f32* dist /*= nullptr*/ )
{
	f32 a = ( ( point - ray.pos ).Dot( ray.dir ) / ray.dir.Dot( ray.dir ) );
	if( a > 0 )
	{
		ycVec3 cp = a * ray.dir + ray.pos;
		f32 d2 = cp.DistSq( point );
		f32 r = a * ray.slope + ray.base;
		if( d2 < ( r * r ) )
		{
			if( dist ) { *dist = ycSqrt( d2 ); }
			return true;
		}
	}
	return false;
}

bool ycGeo::IntersectionConeRayLine( const ycConeRay & ray, const ycVec3 _start, const ycVec3 end, f32* dist /*= nullptr*/ )
{
	ycVec2 hit;
	if( ClosestPointLineLine( ray.pos, ray.dir, _start, end-_start, &hit ) )
	{
		if( hit.y >= 0.0f && hit.y <= 1.0f )
		{
			f32 r = hit.x * ray.slope + ray.base;
			f32 d2 = ( ray.pos + hit.x * ray.dir ).DistSq( hit.y * ( end - _start ) + _start );
			if( d2 < ( r * r ) )
			{
				if( dist ) { *dist = ycSqrt( d2 ); }
				return true;
			}
		}
	}
	return false;
}

bool ycGeo::IntersectsRaySphere( const ycRay& ray, const ycSphere& sphere )
{
	return IntersectionRaySphere( nullptr, ray, YC_F32_MAX, sphere );
}

bool ycGeo::IntersectsLineSegSphere( const ycLineSeg& lineSeg, const ycSphere& sphere )
{
	return IntersectionLineSegSphere( nullptr, lineSeg, sphere );
}

bool ycGeo::IntersectsTriLineSeg( const ycTri& tri, const ycLineSeg & seg )
{
	ycRay ray( seg.start, seg.offset );
	return IntersectionRayTri( nullptr, ray, 1.0f, tri );
}

u32	 ycGeo::IntersectsCirclePlane( const ycCircle & circle, const ycPlane & plane )
{
	return IntersectionCirclePlane( circle, plane );
}

u32  ycGeo::IntersectsCircleCircle( const ycCircle & circle, const ycCircle & other )
{
	if( circle.normal == other.normal )
	{
		f32 dist = circle.center.DistSq( other.center );
		f32 rad = ( ( circle.radius + other.radius ) * ( circle.radius + other.radius ) );
		if( dist < rad )
		{
			return 2;
		}
		else if( dist == rad )
		{
			return 1;
		}
		return 0;
	}
	else
	{
		ycVec3 intersection1, intersection2;
		u32 hits = IntersectionCirclePlane( circle, other.ToPlane(), &intersection1, &intersection2 );	
		if( (hits >= 1 && circle.Contains( intersection1 ))
			|| (hits == 2 && circle.Contains( intersection2 )) )
		{
			return 1;
		}
		return 0;
	}
}

u32  ycGeo::IntersectsCircleLineSeg( const ycCircle & circle, const ycLineSeg & lineSeg )
{
	return IntersectionCircleLineSeg( circle, lineSeg );
}

u32  ycGeo::IntersectsCircleRay( const ycCircle & circle, const ycRay& ray )
{
	return IntersectionCircleRay( circle, ray );
}

bool ycGeo::IntersectsCircleSphere( const ycCircle & circle, const ycSphere & sphere )
{
	if( IntersectsSphereSphere( sphere, ycSphere( circle.center, circle.radius ) ) )
	{
		return IntersectsPlaneSphere( circle.ToPlane(), sphere );
	}
	return false;
}

bool ycGeo::IntersectsLineSegLineSegXY( const ycLineSeg& a, const ycLineSeg& b )
{
	ycVec3 start, start2, end, end2;
	start = a.start;
	start2 = b.start;
	end = start + a.offset;
	end2 = start2 + b.offset;
	f32 denom = ((end2.y - start2.y) * (end.x - start.x)) - ((end2.x - start2.x) * (end.y - start.y));
	if( denom != 0.0f )
	{
		f32 ua = (((end2.x - start2.x) * (start.y - start2.y)) - ((end2.y - start2.y) * (start.x - start2.x))) / denom;
		f32 ub = (((end.x - start.x) * (start.y - start2.y)) - ((end.y - start.y) * (start.x - start2.x))) / denom;
		if( (ua < 0) || (ua > 1) || (ub < 0) || (ub > 1) )
		{
			return false;
		}
		return true;
	}
	return false;
}

bool ycGeo::IntersectsCapsuleCapsule( const ycCapsule & capsule, const ycCapsule & other )
{
	return DistLineSegLineSeg( ycLineSeg( capsule.top, capsule.bottom ), ycLineSeg( other.top, other.bottom ) ) <= (capsule.radius + other.radius);
}

bool ycGeo::IntersectsCapsuleSphere( const ycCapsule & capsule, const ycSphere & sphere )
{
	return DistLineSegPoint( ycLineSeg( capsule.top, capsule.bottom ), sphere.center ) <= (capsule.radius + sphere.radius);
}

bool ycGeo::IntersectsCapsuleLineSeg( const ycCapsule & capsule, const ycLineSeg & lineSeg )
{
	return DistLineSegLineSeg( ycLineSeg( capsule.top, capsule.bottom ), lineSeg ) <= capsule.radius;
}

bool ycGeo::IntersectsCapsulePlane( const ycCapsule & capsule, const ycPlane & plane )
{
	 return DistPlaneLineSeg( plane, ycLineSeg( capsule.bottom, capsule.top ) ) <= capsule.radius;
}

bool ycGeo::IntersectsPlaneAABB( const ycPlane& plane, const ycAABB& aabb )
{
	ycVec3 extents = aabb.GetExtents( );
	ycVec3 pn = plane.normal;
	f32 r_eff; // the effective radius of the OBB
	r_eff  = extents.x * ycAbs( pn.x );
	r_eff += extents.y * ycAbs( pn.y );
	r_eff += extents.z * ycAbs( pn.z );
	f32 s = pn.Dot( aabb.GetCenter() ) - plane.d;
	return ycAbs( s ) <= r_eff;
}

bool ycGeo::IntersectsAABBSphere( const ycAABB& aabb, const ycSphere& sphere )
{
	return IntersectsSphereAABB( sphere, aabb );
}

bool ycGeo::IntersectsSphereAABB( const ycSphere& sphere, const ycAABB& aabb )
{
	ycVec3 cp;
	ycVec3 sphereCenter = sphere.center;
	f32 r = sphere.radius;
	ClosestPointAABB( aabb, sphereCenter, &cp );
	return ( cp.DistSq( sphereCenter ) ) <= ( r * r );
}

bool ycGeo::Intersects( const ycShapeContainer& a, const ycVec3& posA, const ycShapeContainer& b, const ycVec3& posB, u32 flags )
{
	return Intersects( a.GetShapeType(), *a.GetShape(), posA, b.GetShapeType(), *b.GetShape(), posB, flags );
}

bool ycGeo::Intersects( u32 shapeAType, const ycShape& shapeA, const ycVec3& posA, u32 shapeBType, const ycShape& shapeB, const ycVec3& posB, u32 flags )
{
	switch( shapeAType )
	{
		case kShapeType_LineSeg:
			switch( shapeBType )
			{
				case kShapeType_AABB:		{ ycAABB shapeB_O( (const ycAABB&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB );	ycLineSeg shapeA_O( (const ycLineSeg&)shapeA ); shapeA_O.start += posA; return IntersectsAABBLineSeg( shapeB_O, shapeA_O ); } break;
				case kShapeType_Tri:		{ ycTri shapeB_O( (const ycTri&)shapeB ); shapeB_O.Offset( posB );								ycLineSeg shapeA_O( (const ycLineSeg&)shapeA ); shapeA_O.start += posA; return IntersectsTriLineSeg( shapeB_O, shapeA_O ); } break;
				case kShapeType_LineSeg:	YC_UNUSED( flags ); ycAssert( (flags & kIntersectionFlag_2D) != 0 ); 
											{ ycLineSeg shapeB_O( (const ycLineSeg&)shapeB ); shapeB_O.start += posB;						ycLineSeg shapeA_O( (const ycLineSeg&)shapeA ); shapeA_O.start += posA; return IntersectsLineSegLineSegXY( shapeA_O, shapeB_O ); } break;
				case kShapeType_Sphere:		{ ycSphere shapeB_O( (const ycSphere&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB );	ycLineSeg shapeA_O( (const ycLineSeg&)shapeA ); shapeA_O.start += posA; return IntersectsLineSegSphere( shapeA_O, shapeB_O ); } break;
			}
			break;
		case kShapeType_AABB:
			switch( shapeBType )
			{
				case kShapeType_AABB:		{ ycAABB shapeB_O( (const ycAABB&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB );	ycAABB shapeA_O( (const ycAABB&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA ); return IntersectsAABBAABB( shapeA_O, shapeB_O ); } break;
				case kShapeType_Tri:		{ ycTri shapeB_O( (const ycTri&)shapeB ); shapeB_O.Offset( posB );								ycAABB shapeA_O( (const ycAABB&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA ); return IntersectsAABBTri( shapeA_O, shapeB_O ); } break;
				case kShapeType_LineSeg:	{ ycLineSeg shapeB_O( (const ycLineSeg&)shapeB ); shapeB_O.start += posB;						ycAABB shapeA_O( (const ycAABB&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA ); return IntersectsAABBLineSeg( shapeA_O, shapeB_O ); } break;
				case kShapeType_Sphere:		{ ycSphere shapeB_O( (const ycSphere&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB );	ycAABB shapeA_O( (const ycAABB&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA ); return IntersectsAABBSphere( shapeA_O, shapeB_O ); } break;
			}
			break;
		case kShapeType_Tri:
			switch( shapeBType )
			{
				case kShapeType_AABB:		{ ycTri shapeA_O( (const ycTri&)shapeA ); shapeA_O.Offset( posA );								ycAABB shapeB_O( (const ycAABB&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB ); return IntersectsAABBTri( shapeB_O, shapeA_O ); } break;
				case kShapeType_Tri:		{ ycTri shapeA_O( (const ycTri&)shapeA ); shapeA_O.Offset( posA );								ycTri shapeB_O( (const ycTri&)shapeB ); shapeB_O.Offset( posB );  return IntersectsTriTri( shapeA_O, shapeB_O ); } break;
				case kShapeType_LineSeg:	{ ycTri shapeA_O( (const ycTri&)shapeB ); shapeA_O.Offset( posA );								ycLineSeg shapeB_O( (const ycLineSeg&)shapeB ); shapeB_O.start += posB; return IntersectsTriLineSeg( shapeA_O, shapeB_O ); } break;
				case kShapeType_Sphere:		{ ycTri shapeA_O( (const ycTri&)shapeA ); shapeA_O.Offset( posA );								ycSphere shapeB_O( (const ycSphere&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB ); return IntersectsSphereTri( shapeB_O, shapeA_O ); } break;
			}
			break;

		case kShapeType_Sphere:
			switch( shapeBType )
			{
				case kShapeType_AABB:		{ ycSphere shapeA_O( (const ycSphere&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA );	ycAABB shapeB_O( (const ycAABB&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB ); return IntersectsAABBSphere( shapeB_O, shapeA_O ); } break;
				case kShapeType_Tri:		{ ycTri shapeB_O( (const ycTri&)shapeB ); shapeB_O.Offset( posB );									ycSphere shapeA_O( (const ycSphere&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA ); return IntersectsSphereTri( shapeA_O, shapeB_O ); } break;
				case kShapeType_LineSeg:	{ ycSphere shapeA_O( (const ycSphere&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA );	ycLineSeg shapeB_O( (const ycLineSeg&)shapeB ); shapeB_O.start += posB; return IntersectsLineSegSphere( shapeB_O, shapeA_O ); } break;
				case kShapeType_Sphere:		{ ycSphere shapeA_O( (const ycSphere&)shapeA ); shapeA_O.SetCenter( shapeA_O.GetCenter() + posA );	ycSphere shapeB_O( (const ycSphere&)shapeB ); shapeB_O.SetCenter( shapeB_O.GetCenter() + posB ); return IntersectsSphereSphere( shapeA_O, shapeB_O ); } break;
			}
			break;
	}
	ycAssertMsg( 0, "Not Supported" );
	return false;
}

bool ycGeo::IntersectionInvRayBox( f32* hitT, const ycVec3& rayStart, const ycVec3& rayInvDir, f32 maxT, const ycBox& aabb )
{
	ycVec3 d0 = ( aabb.GetMin() - rayStart ) * rayInvDir;
	ycVec3 d1 = ( aabb.GetMax() - rayStart ) * rayInvDir;

	ycVec3 v0 = ycVec3::Min( d0, d1 );
	ycVec3 v1 = ycVec3::Max( d0, d1 );

	f32 tmin = ycMax( ycMax( v0.x, v0.y ), v0.z );
	f32 tmax = ycMin( ycMin( v1.x, v1.y ), v1.z );

	if( ( tmax >= 0 ) && ( tmax >= tmin ) && ( tmin <= maxT ) )
	{
		if( hitT ) { *hitT = ycMax( tmin, 0.0f ); }
		return true;
	}
	return false;
}

bool ycGeo::IntersectionSweptBoxBox( f32* hitT, const ycBox& sweepBox, const ycVec3& sweepDir, f32 maxT, const ycBox& aabb )
{
	ycVec3 sweepInvDir = ycVec3::ONE() / sweepDir;
	return IntersectionInvSweptBoxBox( hitT, sweepBox, sweepInvDir, maxT, aabb );
}

bool ycGeo::IntersectionInvSweptBoxBox( f32* hitT, const ycBox& sweepBox, const ycVec3& sweepInvDir, f32 maxT, const ycBox& aabb )
{
	ycVec3 cen = sweepBox.GetCenter();
	ycVec3 ext = sweepBox.GetExtents();
	ycBox paddedBox( aabb.GetMin() - ext,
		aabb.GetMax() + ext );

	return IntersectionInvRayBox( hitT, cen, sweepInvDir, maxT, paddedBox );
}

bool ycGeo::IntersectionRayAABB( f32* hitT, const ycRay& ray, f32 maxT, const ycAABB& aabb )
{
	ycVec3 invDir = ycVec3::ONE() / ray.dir;
	ycBox box( aabb.GetMin(), aabb.GetMax() );
	return IntersectionInvRayBox( hitT, ray.pos, invDir, maxT, box );
}

bool ycGeo::IntersectionRaySphere( f32* hitT, const ycRay& ray, f32 maxT, const ycSphere& sph )
{
	return IntersectionRaySphere( hitT, nullptr, ray, maxT, sph );
}


bool ycGeo::IntersectionRaySphere( f32* hitT, f32* hitT2, const ycRay& ray, f32 maxT, const ycSphere& sph )
{
	// Originally used Real-Time Collision Detection by Christer Ericson, section 5.3.2 for reference/guidance
	// The quadratic sphere/ray test has precision issues if the ray length is much larger than the sphere radius
	// This method is from Haines, Gunther, & Akenine-M�ller, "Precision Improvements for Ray/Sphere Intersection", 2019

	ycVec3 f = ray.pos - sph.center;
	ycVec3 d = ray.dir;

	f32 r = sph.radius;
	f32 a = d.Dot( d );
	f32 b = -f.Dot( d );
	f32 c = f.LengthSq() - ( r * r ); // if(-) then origin inside sphere

	if( b < 0.0f && c > 0.0f )
	{
		return false; // origin outside of sphere, ray pointing away from sphere
	}

	f32 discriminant = ( sph.radius * sph.radius ) - ( f + ( b / a ) * d ).LengthSq();
	if( discriminant < 0.0f )
	{
		return false; // if disc is (-) then no real solution, i.e. no t for t of intersection & therefore no intersection
	}

	f32 q = b + ycSign( b ) * ycSqrt( a * discriminant );

	// Compute time of first intersection.
	f32 t = c / q;
	if( t >= maxT )
	{
		return false;
	}

	// Clamp to zero if the ray starts inside the sphere.
	if( t < 0.0f )
	{
		t = 0.0f;
	}

	if( hitT )
	{
		*hitT = t;
	}
	if( hitT2 )
	{
		*hitT2 = q / a;
	}

	return true;
}

bool ycGeo::IntersectionRayPlane( f32* hitT, const ycRay& ray, f32 maxT, const ycPlane& plane )
{
	/*f32 dirDP = plane.normal.Dot( ray.dir );
	if( ycAbs( dirDP ) < YC_F32_EPSILON )
	{
		//todo: return something than notifies we are on the same plane
		//pick the start point if we are on the same plane
		if( plane.Intersects( ray.start ) )
		{
			if( hitT ) { *hitT = 0.0f; }
			return true;
		}
		return false;
	}

	f32 startDP = -((plane.normal.Dot( ray.start ) - plane.d ) / dirDP );
	if((startDP < 0.0f && dirDP > 0.0f) || (startDP > 0.0f && dirDP > 0.0f) || startDP > maxT )
	{
		return false;
	}
	if( hitT ) { *hitT = startDP; }
	return true;*/
	// Referenced "Realtime Collision Detection" by Christer Ericson, section 5.3.1
	ycVec3 ray_origin = ray.pos;
	ycVec3 ray_slope = ray.dir;
	ycVec3 pl_normal = plane.normal;
	f32 pl_d = plane.d;
	// t = (D - n.Dot(origin))  /  (n.Dot(slope))
	f32 numerator = pl_d - pl_normal.Dot( ray_origin );
	f32 denominator = pl_normal.Dot( ray_slope );
	if( !ycIsZero( denominator ) )
	{
		f32 t = numerator / denominator;
		if( t >= 0.0f && t <= maxT )
		{
			if( hitT )
			{
				*hitT = t;
			}
			return true;
		}
	}
	// ray is on the plane
	if( ycIsZero(pl_d - ( ray_origin.Dot( pl_normal ) ) ) )
	{
		if( hitT )
		{
			*hitT = 0.0f;
		}
		return true;
	}
	//  ray is not on the plane
	return false;
}

bool ycGeo::IntersectionLineAABB( ycVec3* pt_col, const ycLine& line, const ycAABB& aabb )
{
	// Used Real-Time Collision Detection by Christer Ericson, section 5.3.2 for reference/guidance
	ycVec3 lineSlope = line.GetSlope( );
	ycVec3 lineOrigin = line.GetOrigin( );
	ycVec3 aabbMin = aabb.GetMin( );
	ycVec3 aabbMax = aabb.GetMax( );
	f32 tmin = YC_F32_MIN; // from the definition of a line
	f32 tmax = YC_F32_MAX;
	// getting tests lines that are parllel to a slab
	for( sreg i = 0; i < 3; ++i )
	{
		if( ycIsZero( lineSlope.a[i] ) )
		{
			if( lineOrigin.a[i] < aabbMin.a[i] || lineOrigin.a[i] > aabbMax.a[i] )
			{
				return false;
			}
		}
	}
	// testing that there is a region of line that overlaps all slabs
	for( sreg i = 0; i < 3; ++i )
	{
		f32 slopeInv = 1.f / lineSlope.a[i];
		f32 t1 = aabbMin.a[i] * slopeInv;
		f32 t2 = aabbMax.a[i] * slopeInv;
		if( t2 < t1 ) // conditional swap
		{
			f32 tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = ( tmin > t1 )?tmin:t1;
		tmax = ( tmax < t2 )?tmax:t2;
		if( tmin > tmax )
		{
			return false;
		}
	}
	( *pt_col ) = lineOrigin + tmin * lineSlope;
	return true;
}

bool ycGeo::IntersectionLineSphere( ycVec3* pt_col, const ycLine& line, const ycSphere& sph )
{
	// Used Real-Time Collision Detection by Christer Ericson, section 5.3.2 for reference/guidance
	ycVec3 lineOrigin = line.GetOrigin( );
	ycVec3 lineSlope = line.GetSlope( );
	ycVec3 sphCenter = sph.center;
	f32 sphRadius = sph.radius;

	ycVec3 m = lineOrigin - sphCenter;
	f32 b = m.Dot( lineSlope ); // if(-) then line heading towards sphere
	f32 c = lineOrigin.DistSq( sphCenter ) - ( sphRadius * sphRadius ); // if(-) then origin inside sphere
	// here we solve the equation  t = -b (+|-)sqrt( b^2 -c) as pointed out by Mr. Ericson
	f32 discriminant = b*b - c;
	if( c < 0.f )
	{
		return false; // if disc is (-) then no real solution, i.e. no t for t of intersection & therefore no intersection
	}
	f32 sqrDisc = ycSqrt( discriminant );
	f32 t = -b - sqrDisc; // - will always give the min
	( *pt_col ) = lineOrigin + t * lineSlope;
	return true;
}

bool ycGeo::IntersectionLineTri( ycVec3* pt_col, const ycLine& line, const ycTri& tri )
{
	// step one get intersection point of seg and plane triangle is on
	ycPlane trisPlane( tri.GetA( ), tri.GetB( ), tri.GetC( ) );
	bool segIntersectsTriPlane = IntersectionLinePlane( pt_col, line, trisPlane );
	if( !segIntersectsTriPlane )
	{
		return false;
	}
	// step two see if that point is within the triangle.
	return ContainsTriPt( tri, *pt_col );
}

bool ycGeo::IntersectionLinePlane( ycVec3* pt_col, const ycLine& line, const ycPlane& pl )
{
	// Referenced "Realtime Collision Detection" by Christer Ericson, section 5.3.1
	ycVec3 line_origin = line.GetOrigin( );
	ycVec3 line_slope = line.GetSlope( );
	ycVec3 pl_normal = pl.normal;
	f32 pl_d = pl.d;
	// t = (D - n.Dot(origin))  /  (n.Dot(slope))
	f32 numerator = pl_d - pl_normal.Dot( line_origin );
	f32 denominator = pl_normal.Dot( line_slope );
	if( !ycIsZero( denominator ) )
	{
		f32 t = numerator / denominator;
		( *pt_col ) = line_origin + t*line_slope;
		return true;
	}
	// line is on the plane
	if( ycIsZero(pl_d - ( line_origin.Dot( pl_normal ) ) ) )
	{
			return false;
	}
	//  line is not on the plane
	return false;
}

bool ycGeo::IntersectionLineLine( ycVec3* pt_col, const ycLine& a, const ycLine& b )
{
	ycVec3 cp_a;
	ycVec3 cp_b;
	ClosestPointsLineLine( a, b, &cp_a, &cp_b );
	( *pt_col ) = cp_a;
	return ycIsZero( cp_a.DistSq( cp_b) );
}

bool ycGeo::IntersectionLineRay( ycVec3* pt_col, const ycLine& line, const ycRay& ray )
{
	ycVec3 cp_line;
	ycVec3 cp_ray;
	ClosestPointsLineRay( line, ray, &cp_line, &cp_ray );
	( *pt_col ) = cp_line;
	return ycIsZero( cp_line.DistSq( cp_ray) );
}

bool ycGeo::IntersectionLineSegPlane( f32* hitT, const ycLineSeg& lineSeg, const ycPlane& plane )
{
	ycRay ray( lineSeg.start, lineSeg.offset );
	return IntersectionRayPlane( hitT, ray, 1.0f, plane );

	/*f32 mag = lineSeg.offset.Mag();
	ycVec3 normalized = lineSeg.offset / mag;
	ycRay ray( lineSeg.start, normalized );
	bool hit = IntersectionRayPlane( hitT, ray, mag, plane );
	if( hit && hitT )
	{
		*hitT /= mag;
	}
	return hit;*/
}

bool ycGeo::IntersectionLineSegSphere( f32* hitT, const ycLineSeg& lineSeg, const ycSphere& sphere )
{
	ycRay ray( lineSeg.start, lineSeg.offset );
	return IntersectionRaySphere( hitT, ray, 1.0f, sphere );
}

bool ycGeo::IntersectionLineSegTri( ycGeoRayTriHit* hit, const ycLineSeg& lineSeg, const ycTri& tri )
{
	ycRay ray( lineSeg.start, lineSeg.offset );
	return IntersectionRayTri( hit, ray, 1.0f, tri );
}

bool ycGeo::IntersectionLineSegLineSegXY( ycVec3* pt_col, f32 * fraction, const ycLineSeg& a, const ycLineSeg& b)
{
	ycVec3 start, start2, end, end2;
	start = a.start;
	start2 = b.start;
	end = start + a.offset;
	end2 = start2 + b.offset;
	f32 denom = ((end2.y - start2.y) * (end.x - start.x)) - ((end2.x - start2.x) * (end.y - start.y));
	if( denom != 0.0f )
	{
		f32 ua = (((end2.x - start2.x) * (start.y - start2.y)) - ((end2.y - start2.y) * (start.x - start2.x))) / denom;
		f32 ub = (((end.x - start.x) * (start.y - start2.y)) - ((end.y - start.y) * (start.x - start2.x))) / denom;
		if( (ua < 0) || (ua > 1) || (ub < 0) || (ub > 1) )
		{
			return false;
		}
		if( pt_col)
		{
			*pt_col = start + (end - start) * ua;
		}
		if( fraction )
		{
			*fraction = ua;
		}
		return true;
	}
	return false;
}

u32	ycGeo::IntersectionCirclePlane( const ycCircle & circle, const ycPlane & plane, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2 /* = nullptr*/ )
{
	ycLine line;
	if( IntersectionPlanePlane( &line, plane, circle.ToPlane() ) )
	{
		line.start -= circle.center;

		f32 a, b, c, r1, r2;
		a = 1.0f;
		b = 2.0f * line.start.Dot( line.dir );
		c = line.start.MagSq() - (circle.radius * circle.radius);
		s32 numRoots = ycQuadratic( a, b, c, &r1, &r2 );
		if( numRoots >= 1 && intersection1 )
		{
			*intersection1 = circle.center + line.start + line.dir * r1;
		}
		if( numRoots >= 2 && intersection2 )
		{
			*intersection2 = circle.center + line.start + line.dir * r2;
		}
		return numRoots;
	}
	return 0;
}

u32 ycGeo::IntersectionCircleLineSeg( const ycCircle & circle, const ycLineSeg & lineSeg, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2/* = nullptr*/ )
{
	ycVec3 inter1, inter2;
	u32 hits = IntersectionCircleRay( circle, ycRay( lineSeg.pos, lineSeg.dir ), &inter1, &inter2 );
	if( hits )
	{
		bool contains1 = ContainsLineSegPt( lineSeg, inter1 );
		bool contains2 = ContainsLineSegPt( lineSeg, inter2 );
		if( contains1 && contains2 )
		{
			if( intersection1 ) { *intersection1 = inter1; }
			if( intersection2 ) { *intersection2 = inter2; }
			return 2;
		}
		else if( contains1 )
		{
			if( intersection1 ) { *intersection1 = inter1; }
			return 1;
		}
		else if( contains2 )
		{
			if( intersection2 ) { *intersection2 = inter2; }
			return 1;
		}
	}
	return 0;
}

u32 ycGeo::IntersectionCircleRay( const ycCircle & circle, const ycRay& ray, ycVec3 * intersection1 /*= nullptr*/, ycVec3 * intersection2 /*= nullptr*/ )
{
	f32 hit1, hit2;
	ycPlane circlePlane = circle.ToPlane();
	if( circlePlane == ycPlane( ray.dir, ray.pos ) )
	{
		// case: ray perpendicular to circle
		if( IntersectionRaySphere( &hit1, &hit2, ray, YC_F32_MAX,ycSphere( circle.center, circle.radius ) ) )
		{
			if( intersection1 ) { *intersection1 = ray.GetPoint( hit1 ); }
			if( intersection2 ) { *intersection2 = ray.GetPoint( hit2 ); }
			return 2;
		}
	}
	else if( ycAbs( ray.dir.Dot( circlePlane.normal ) ) <= 1e-3f && circlePlane.Distance( ray.pos ) <= 1e-3f )
	{
		// case: ray coplanar with circle
		if( IntersectionRaySphere( &hit1, &hit2, ray, YC_F32_MAX, ycSphere( circle.center, circle.radius ) ) )
		{
			if( intersection1 ) { *intersection1 = ray.GetPoint( hit1 ); }
			if( intersection2 ) { *intersection2 = ray.GetPoint( hit2 ); }
			return 2;
		}
	}
	else if( IntersectionRayPlane( &hit1, ray, YC_F32_MAX, circlePlane ) )
	{
		// normal case
		ycVec3 pt = ray.GetPoint( hit1 );
		if( ContainsSpherePt( ycSphere( circle.center, circle.radius ), pt ) )
		{
			if( intersection1 ) { *intersection1 = pt; }
			return 1;
		}
	}
	return 0;
}

bool ycGeo::IntersectionPlanePlanePlane( ycVec3* pt_col, const ycPlane& p1, const ycPlane& p2, const ycPlane& p3 )
{
	ycVec3 u = p2.normal.Cross( p3.normal );

	const f32 denom = p1.normal.Dot( u );
	if( ycAbs( denom ) < 0.001f ) { return false; }
	*pt_col = ( p1.d * u + p1.normal.Cross( p3.d * p2.normal - p2.d * p3.normal ) ) / denom;

	return true;
}

bool ycGeo::IntersectionPlanePlane( ycLine* ln_out, const ycPlane& p1, const ycPlane& p2)
{

	// Special thanks to "Real-Time Collision Detection" by Christer Ericson
	// for it's guidance on pages 207 to 209, whose equations this implements
	// Here are the equations used from the text
	// Line of intersection = P + t*d; where P is a point on the line, and d is the direction
	// solving this system of linear equations we get (see book for more info)
	// denom = ( n1.dot(n2) * n2.dot(n2) - (n1.dot(n2)^2) )
	// k1 = ( d1*(n1.dot(n2)) - d2*(n1.dot(n2)) ) / denom; //d1 and d2 are plane.distance
	// k2 = ( d2(n1.dot(n1)) -  d1*(n2.dot(n2)) ) / denom;
	ycVec3 n1 = p1.normal;
	f32 d1 = p1.d;
	ycVec3 n2 = p2.normal;
	f32 d2 = p2.d;
	ycVec3 slope = ( n1 ).Cross( n2 ); // the direction of the intersection line

	if( ycIsZero( slope.MagSq() ) ) // intersection geometry will not be a line
	{
		return false;
	}
	slope.Normalize( );
	(*ln_out).SetSlope( slope );
	// computing dots here for quick acces	
	//@question should I separate memory acces from computation from saving so easier for
	// Get mem values first
	f32 a = n1.Dot( n1 );
	f32 b = n1.Dot( n2 );
	f32 c = n2.Dot( n2 );
	f32 d = ( a * c ) - ( b*b );
	f32 k1 = ( ( d1 * c ) - ( d2 * b ) ) / d;
	f32 k2 = ( ( d2 * a ) - ( d1 * b ) ) / d;
	ycVec3 newLineOrigin = n1*k1 + n2*k2;
	(*ln_out).SetOrigin( newLineOrigin );
	return true;
}


bool ycGeo::SweepAABBAABB( f32 * fraction, ycVec3 * normal, const ycAABB & boxA, const ycLineSeg & sweepA, const ycAABB & boxB, const ycLineSeg & sweepB )
{
	ycVec3 a_min = boxA.GetMin( );
	ycVec3 a_max = boxA.GetMax( );
	ycVec3 b_min = boxB.GetMin( );
	ycVec3 b_max = boxB.GetMax( );
	ycVec3 minA = sweepA.start + a_min;
	ycVec3 maxA = sweepA.start + a_max;
	ycVec3 minB = sweepB.start + b_min;
	ycVec3 maxB = sweepB.start + b_max;

	if( minA.x > maxB.x || minB.x > maxA.x )
	{
	}
	else if( minA.y > maxB.y || minB.y > maxA.y )
	{
	}
	else if( minA.z > maxB.z || minB.z > maxA.z )
	{
	}
	else
	{
		return false;
	}

	//if( IntersectsAABBAABB( ycAABB( minA, maxA ), ycAABB( minB, maxB ) ) )
	//{
	//	return false;
	//}

	f32 hitTime = 0.0f;
	f32 outTime = 1.0f;
	s32 hitAxis = 0;
	f32 hitTimeAxis[3] = { 0.0f, 0.0f, 0.0f };
	f32 outTimeAxis[3] = { 1.0f, 1.0f, 1.0f };
					
	ycVec3 vel = sweepB.offset-sweepA.offset;
	for( s32 i = 0 ; i < 3; i++ )
	{
		f32 oldHitTime = hitTime;
		if( vel.a[i] < 0 )
		{
			if( maxB.a[i] < minA.a[i] ) { return false; }
			if( maxA.a[i] < minB.a[i] ) { hitTimeAxis[i] = (maxA.a[i] - minB.a[i]) / vel.a[i]; hitTime = ycMax( hitTime, hitTimeAxis[i] ); }
			if( maxB.a[i] > minA.a[i] ) { outTimeAxis[i] = (minA.a[i] - maxB.a[i]) / vel.a[i]; outTime = ycMin( outTime, outTimeAxis[i] ); }
		}
		else if( vel.a[i] > 0 )
		{
			if( minB.a[i] > maxA.a[i] ) { return false; }
			if( maxB.a[i] < minA.a[i] ) { hitTimeAxis[i] = (minA.a[i] - maxB.a[i]) / vel.a[i]; hitTime = ycMax( hitTime, hitTimeAxis[i] ); }
			if( maxA.a[i] > minB.a[i] ) { outTimeAxis[i] = (maxA.a[i] - minB.a[i]) / vel.a[i]; outTime = ycMin( outTime, outTimeAxis[i] ); }
		}	
		else if(minB.a[i] > maxA.a[i] || maxB.a[i]< minA.a[i]) { return false; }
				
		if( hitTime > outTime ) { return false; }
		if( hitTime != oldHitTime ) { hitAxis = i; }
	}

	*normal = ycVec3::ZERO();
	normal->a[hitAxis] = (f32)ycSign( vel.a[hitAxis] );
	*fraction = hitTime;
	return true;
}

bool ycGeo::SweepRayAABB( f32 * fraction, ycVec3 * normal, const ycRay & ray, const ycAABB & aabb, const ycLineSeg & sweepB )
{
	// Used Real-Time Collision Detection by Christer Ericson, section 5.3.2 for reference/guidance
	ycVec3 raySlope = ray.dir;
	ycVec3 rayOrigin = ray.pos;
	ycVec3 aabbMin = aabb.GetMin( ) + sweepB.start;
	ycVec3 aabbMax = aabb.GetMax( ) + sweepB.start;
	f32 tmin = 0.f; // from the definition of a ray
	f32 tmax = YC_F32_MAX;
	// getting tests rays that are parllel to a slab
	for( sreg i = 0; i < 3; ++i )
	{
		if( ycIsZero( raySlope.a[i] ) )
		{
			if( rayOrigin.a[i] < aabbMin.a[i] || rayOrigin.a[i] > aabbMax.a[i] )
			{
				return false;
			}
		}
	}
	// testing that there is a region of ray that overlaps all slabs
	for( sreg i = 0; i < 3; ++i )
	{
		f32 slopeInv = 1.f / raySlope.a[i];
		f32 t1 = aabbMin.a[i] * slopeInv;
		f32 t2 = aabbMax.a[i] * slopeInv;
		if( t2 < t1 ) // conditional swap
		{
			f32 tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = ( tmin > t1 )? tmin : t1;
		tmax = ( tmax < t2 )? tmax : t2;
		if( tmin > tmax )
		{
			return false;
		}
	}
	f32 t = ( tmin > 0.f ) ? tmin : tmax;
	*fraction = t;
	//this could probably use the code above...
	*normal = aabb.GetNormal( ray.GetPoint( t ) );
	return true;
}

bool ycGeo::SweepLineSegAABB( f32 * fraction, ycVec3 * normal, const ycLineSeg & seg, const ycAABB & b, const ycLineSeg & sweepB )
{
	f32 timeMin = 0.0f;
	f32 timeMax = YC_F32_MAX;
	ycVec3 distance = seg.GetOffset();
	ycVec3 segStart = seg.GetStart();
	ycVec3 boxMin = b.GetMin() + sweepB.GetStart();
	ycVec3 boxMax = b.GetMax() + sweepB.GetStart();
	for( s32 i = 0; i < 3; ++i )
	{
		if( ycAbs( distance.a[i] ) < YC_F32_EPSILON )
		{
			// Line is parallel to slab. No collision unless origin is within shape.
			if( segStart.a[i] < boxMin.a[i] || segStart.a[i] > boxMax.a[i] )
			{
				return false;
			}
		}
		else
		{
			// Find intersection of ray with near and far plane.
			f32 ood = 1.0f / distance.a[i];
			f32 t1 = (boxMin.a[i] - segStart.a[i]) * ood;
			f32 t2 = (boxMax.a[i] - segStart.a[i]) * ood;
			// t1 near plane, t2 far plane.
			if( t1 > t2 )
			{
				f32 tmp = t1;
				t1 = t2;
				t2 = tmp;
			}
			// Find intersection of intersection intervals.
			timeMin = ycMax( timeMin, t1 );
			timeMax = ycMin( timeMax, t2 );
			// Exit with no collision as soon as slab intersection becomes empty.
			if( timeMin > timeMax )
			{
				return false;
			}
		}
	}
	// Ray intersects all 3 slabs.
	if( timeMin > 1.0f )
	{
		return false;
	}
	*fraction = timeMin;
	//this could probably use the code above...
	*normal = b.GetNormal( seg.GetPoint( timeMin ) );
	return true;
}

bool ycGeo::SweepLineSegSphere( f32 * fraction, ycVec3 * normal, const ycLineSeg & seg, const ycSphere & b, const ycLineSeg & sweepB )
{
	ycRay ray( seg.start, seg.offset );
	ycSphere sphereTest = b;
	sphereTest.center += sweepB.GetStart();
	bool intersection = IntersectionRaySphere( fraction, ray, 1.0f, sphereTest );
	if( intersection )
	{
		if( normal )
		{
			*normal = (ray.GetPoint( *fraction ) - sphereTest.center).Normalized();
		}
		return true;
	}
	return false;
}

bool ycGeo::SweepLineSegLineSegXY( f32 * fraction, ycVec3 * normal, const ycLineSeg & a, const ycLineSeg & b )
{
	ycVec3 start, start2, end, end2;
	start = a.start;
	start2 = b.start;
	end = start + a.offset;
	end2 = start2 + b.offset;
	f32 denom = ((end2.y - start2.y) * (end.x - start.x)) - ((end2.x - start2.x) * (end.y - start.y));
	if( denom != 0.0f )
	{
		f32 ua = (((end2.x - start2.x) * (start.y - start2.y)) - ((end2.y - start2.y) * (start.x - start2.x))) / denom;
		f32 ub = (((end.x - start.x) * (start.y - start2.y)) - ((end.y - start.y) * (start.x - start2.x))) / denom;
		if( (ua < 0) || (ua > 1) || (ub < 0) || (ub > 1) )
		{
			return false;
		}
		if( fraction )
		{
			*fraction = ua;
		}
		if( normal )
		{
			//fixme:
			*normal = (b.offset).Cross( a.GetPoint( ycMax( ua - 0.0001f, 0.0f ) ) );
		}
		return true;
	}
	return false;
}

bool ycGeo::SweepLineSegTri( f32 * fraction, ycVec3 * normal, const ycLineSeg & seg, const ycTri & a, const ycLineSeg & sweepB )
{
	// Used Real-Time Collision Detection by Christer Ericson, section 5.3.6 for reference/guidance
	ycTri tri = a + sweepB.GetStart();
	ycVec3 ab = tri.GetB() - tri.GetA();
	ycVec3 ca = tri.GetC() - tri.GetA();
	ycVec3 p = seg.start;
	ycVec3 q = p + seg.offset;
	ycVec3 qp = p - q;

	// Compute triangle normal. Can be precalculated or cached if
	// intersecting multiple segments against the same triangle
	ycVec3 n = ab.Cross( ca );

	// Compute denominator d. If d <= 0, segment is parallel to or points
	// away from triangle, so exit early
	f32 d = qp.Dot( n );
	if( d <= 0.0f ) return false;
	// Compute intersection t value of pq with plane of triangle. A ray
	// intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
	// dividing by d until intersection has been found to pierce triangle
	ycVec3 ap = p - tri.GetA();
	f32 t = ap.Dot( n );
	if( t < 0.0f ) return false;
	if( t > d ) return false; // For segment; exclude this code line for a ray test

	// Compute barycentric coordinate components and test if within bounds
	ycVec3 e = qp.Cross( ap );

	f32 v = ca.Dot( e );
	if(v < 0.0f || v > d) return false;
	f32 w = -ab.Dot( e );
	if(w < 0.0f || v + w > d) return false;
	
	// Segment/ray intersects triangle. Perform delayed division and
	// compute the last barycentric coordinate component
	f32 ood = 1.0f / d;
	//v *= ood;
	//w *= ood;
	//u = 1.0f - v - w;
	*fraction = t * ood;
	*normal = ycVec3::ZERO();
	return true;
}

#include "ycCollisionSAT.h"
bool ycGeo::SweepAABBTri( f32 * fraction, ycVec3 * normal, const ycAABB & a, const ycLineSeg & sweepA, const ycTri & b, const ycLineSeg & sweepB )
{
#if 1
	ycMtx4 box;  
	box.FromScaleRotPos( a.GetExtents(), ycVec3::ZERO(), a.GetCenter() + sweepA.GetStart() );
	ycTri bWorld( b.GetA()+sweepB.GetStart(), b.GetB()+sweepB.GetStart(), b.GetC()+sweepB.GetStart() );
	ycVec3 dir = sweepA.GetOffset()-sweepB.GetOffset();//think this function should be reversed
	f32 dist = dir.Normalize();
	ycCollisionResult contactPoint;
	contactPoint.matchAxisOkay = s_triMatchAxis;
	bool anyCollide = ycCollisionSAT::SatIterativeSweep( box, bWorld, dir, dist, &contactPoint, ycCollisionOptions::kBackface_Ignore, false );
	if( anyCollide )
	{
		*normal = contactPoint.dir;
		*fraction = contactPoint.sweepDist / contactPoint.fullSweepDist;
	}
	return anyCollide;

	//this could probably be a lot better...basically 3 line segment tests vs an AAABB
	//and technically 2d...not like against a poly
	//adapted from https://gamedev.stackexchange.com/questions/29479/swept-aabb-vs-line-segment-2d
	//assuming CCW winding
#else
	ycVec3 triNormal = b.GetNormal();
	ycVec3 edgePos[3] = { b.GetA(), b.GetB(), b.GetC() };
	ycVec3 edgeNormal[3];
	ycVec3 edgeMin[3];
	ycVec3 edgeMax[3];

	for( s32 i = 0; i < 3; ++i )
	{
		ycVec3 next = edgePos[(i+1)%3];
		edgeNormal[i] = (next - edgePos[i]).Cross( triNormal ).Normalized();
		edgeMin[i] = ycVec3::Min( edgePos[i], next );
		edgeMax[i] = ycVec3::Max( edgePos[i], next );
	}
	ycVec3 move = sweepA.GetOffset()-sweepB.GetOffset();//think this function should be reversed
	ycVec3 dir = move.Normalized();
	ycVec3 extents = a.GetExtents();
	ycVec3 p1 = a.GetCenter() + sweepA.GetStart();
	ycVec3 hitNormalAll = ycVec3::ZERO();
	bool anyCollide = false;
	f32 hitTimeAll = 1.1f;
	ycVec3 boxMax = p1 + extents;
	ycVec3 boxMin = p1 - extents;

	for( s32 i = 0; i < 3; ++i )
	{
		f32 dotP = edgeNormal[i].Dot( dir );//not sure this is necesary, maybe just do an intersection test at the start?
		if( dotP < 0.0f )
		{
			dotP = edgeNormal[i].Dot( move );
			ycVec3 lineStart = (sweepB.GetStart() + edgePos[i]);
			ycVec3 lineMin = sweepB.GetStart() + edgeMin[i];
			ycVec3 lineMax = sweepB.GetStart() + edgeMax[i];
			f32 radius = -(extents.x * ycAbs( edgeNormal[i].x ) + extents.y * ycAbs( edgeNormal[i].y) + extents.z * ycAbs( edgeNormal[i].z )); //radius to Line
			f32 boxProj = (lineStart - p1).Dot( edgeNormal[i] ); //projected Line distance to N			

			f32 hitTime = 0.0f;
			f32 outTime = 1.0f;
			hitTime = ycMax( ( boxProj - radius ) / dotP, hitTime);
			outTime = ycMin( ( boxProj + radius ) / dotP, outTime);

			bool collide = true;
			for( s32 k = 0; k < 3; ++k )
			{
				if( move.a[k] < 0 ) //Sweeping left
				{
					if( boxMax.a[k] < lineMin.a[k] )
					{
						collide = false; 
						break;
					}
					hitTime = ycMax( (lineMax.a[k] - boxMin.a[k]) / move.a[k], hitTime);
					outTime = ycMin( (lineMin.a[k] - boxMax.a[k]) / move.a[k], outTime);
				}
				else if( move.a[k] > 0 ) //Sweeping right
				{
					if( boxMin.a[k] > lineMax.a[k] ) 
					{
						collide = false; 
						break;
					}
					hitTime = ycMax( (lineMin.a[k] - boxMax.a[k]) / move.a[k], hitTime);
					outTime = ycMin( (lineMax.a[k] - boxMin.a[k]) / move.a[k], outTime);
				}
				else
				{
					if(lineMin.a[k] > boxMax.a[k] || lineMax.a[k] < boxMin.a[k]) 
					{
						collide = false; 
						break;
					}
				}

				if( hitTime > outTime ) { collide = false; break; }
			}

			if( collide )
			{
				anyCollide = true;
				if( hitTime < hitTimeAll )
				{
					hitTimeAll = hitTime;
					hitNormalAll = edgeNormal[i];
				}
			}
		}	
	}
	if( anyCollide )
	{
		//fix above so you don't need this bonkers check
		if( hitTimeAll <= 0.0f )
		{
			ycAABB test = a;
			test.SetCenter( test.GetCenter() + sweepA.GetStart() );
			ycTri testB = b;
			testB.Offset( sweepB.GetStart() );
			if( ( test.Contains( testB.GetA() ) || test.Contains( testB.GetB() ) || test.Contains( testB.GetC() ) ) )
			{
				return false;
			}
		}
		*normal = hitNormalAll;
		*fraction = hitTimeAll;
	}
	return anyCollide;
#endif
}

bool ycGeo::SweepSphereTri( ycGeoRayTriHit* hit, const ycSphere& sphere, const ycVec3& sweep, f32 maxT, const ycTri& tri )
{
	ycVec3 n = tri.GetNormal();
	if( n.Dot( sphere.center - tri.GetA() ) < 0.0f ) { n = -n;  }

	// NOTE: this uses Paul Nettle's sphere/ellipsoid algorithm.
	// Find first point on sphere to intersect triangle's plane.
	ycVec3 p = sphere.center - n * sphere.radius;

	// Find intersection R of the ray emitting from P and the triangle's plane.
	ycRay ray1( p, sweep );
	ycVec3 r;
	if( !IntersectionRayPlane( ray1, tri.GetA(), n, &r ) )
		return false;

	// Find point on tri closest to R.
	ycVec3 ptri;
	ClosestPointTri( tri, r, &ptri );

	// Fire a ray backwards and see if it hits the sphere.
	f32 t;
	ycRay ray2( ptri, -sweep );
	if( !IntersectionRaySphere( &t, ray2, maxT, sphere ) )
	{
		return false;
	}

	if( hit ) 
	{
		hit->pos = ptri;
		hit->normal = n;
		hit->t = t;
		hit->uvw = ycVec3::ZERO(); // TODO!
	}
	return true;
}

bool ycGeo::SweepAABBSphere( f32 * fraction, ycVec3 * normal, const ycAABB & a, const ycLineSeg & sweepA, const ycSphere & b, const ycLineSeg & sweepB )
{
	ycAABB aabb = a;
	aabb.center += sweepA.GetStart();
	ycSphere s = b;
	s.center += sweepB.start;
	if( aabb.Intersects( s ) )
	{
		return false;
	}
	ycMtx4 box;  
	box.FromScaleRotPos( a.GetExtents(), ycVec3::ZERO(), a.GetCenter() + sweepA.GetStart() );
	ycCollisionSphere sphere;
	ycMtx4 sphereBox = ycMtx4::IDENTITY();
	sphereBox.t.SetXYZ( b.center + sweepB.GetStart() );
	sphere.forward = sphereBox;
	sphere.inverse = sphereBox.Inverted();
	sphere.radius = b.radius;
	ycVec3 dir = sweepA.GetOffset()-sweepB.GetOffset();//think this function should be reversed
	f32 dist = dir.Normalize();
	ycCollisionResult contactPoint;
	bool anyCollide = ycCollisionSAT::SatIterativeSweep( box, sphere, dir, dist, &contactPoint, ycCollisionOptions::kBackface_Ignore, false );
	if( anyCollide )
	{
		*normal = contactPoint.dir;
		*fraction = contactPoint.sweepDist / contactPoint.fullSweepDist;
	}
	return anyCollide;
}

bool ycGeo::SweepSphereSphere( f32 * fraction, ycVec3 * normal, const ycSphere & a, const ycLineSeg & sweepA, const ycSphere & b, const ycLineSeg & sweepB )
{
	ycCollisionSphere sphereA;
	ycMtx4 sphereBox = ycMtx4::IDENTITY();
	sphereBox.t.SetXYZ( a.center + sweepA.GetStart() );
	sphereA.forward = sphereBox;
	sphereA.inverse = sphereBox.Inverted();
	sphereA.radius = a.radius;

	ycCollisionSphere sphereB;
	sphereBox = ycMtx4::IDENTITY();
	sphereBox.t.SetXYZ( b.center + sweepB.GetStart() );
	sphereB.forward = sphereBox;
	sphereB.inverse = sphereBox.Inverted();
	sphereB.radius = b.radius;
	ycVec3 dir = sweepA.GetOffset()-sweepB.GetOffset();//think this function should be reversed
	f32 dist = dir.Normalize();
	ycCollisionResult contactPoint;
	bool anyCollide = ycCollisionSAT::SatIterativeSweep( sphereBox, sphereB, dir, dist, &contactPoint, ycCollisionOptions::kBackface_Ignore, false );
	if( anyCollide )
	{
		*normal = contactPoint.dir;
		*fraction = contactPoint.sweepDist / contactPoint.fullSweepDist;
	}
	return anyCollide;
}

bool ycGeo::Sweep( f32 * fraction, ycVec3 * normal, const ycShapeContainer& a, const ycLineSeg & sweepA, const ycShapeContainer& b, const ycLineSeg & sweepB, u32 flags )
{
	return Sweep( fraction, normal, a.GetShapeType(), *a.GetShape(), sweepA, b.GetShapeType(), *b.GetShape(), sweepB, flags );
}

bool ycGeo::Sweep( f32 * fraction, ycVec3 * normal, u32 shapeAType, const ycShape& shapeA, const ycLineSeg & sweepA, u32 shapeBType, const ycShape& shapeB, const ycLineSeg & sweepB, u32 flags )
{
	switch( shapeAType )
	{
		case kShapeType_LineSeg:
			switch( shapeBType )
			{
				//shape b not expected to move for these..would be nice to fix
				case kShapeType_Sphere:		return SweepLineSegSphere( fraction, normal, ((const ycLineSeg&) shapeA) + sweepA, (const ycSphere&) shapeB, sweepB );
				case kShapeType_AABB:		return SweepLineSegAABB( fraction, normal, ((const ycLineSeg&) shapeA) + sweepA, (const ycAABB&) shapeB, sweepB );
				case kShapeType_Tri:		return SweepLineSegTri( fraction, normal, ((const ycLineSeg&) shapeA) + sweepA, (const ycTri&) shapeB, sweepB );
				case kShapeType_LineSeg:	YC_UNUSED( flags ); ycAssert( (flags & kIntersectionFlag_2D) != 0 );
											return SweepLineSegLineSegXY( fraction, normal, ((const ycLineSeg&) shapeA) + sweepA, ((const ycLineSeg&) shapeB) + sweepB );
			}
			break;
		case kShapeType_AABB:
			switch( shapeBType )
			{
				case kShapeType_AABB:		return SweepAABBAABB( fraction, normal, (const ycAABB&) shapeA, sweepA, (const ycAABB&) shapeB, sweepB );
				case kShapeType_Tri:		return SweepAABBTri( fraction, normal, (const ycAABB&) shapeA, sweepA, (const ycTri&) shapeB, sweepB );
				case kShapeType_LineSeg:	return SweepLineSegAABB( fraction, normal, ((const ycLineSeg&) shapeB) + sweepB, (const ycAABB&) shapeA, sweepA );
				case kShapeType_Sphere:		return SweepAABBSphere( fraction, normal, (const ycAABB&) shapeA, sweepA, (const ycSphere&) shapeB, sweepB );
			}
			break;
		case kShapeType_Tri:
			switch( shapeBType )
			{
				case kShapeType_AABB:		return SweepAABBTri( fraction, normal, (const ycAABB&) shapeB, sweepB, (const ycTri&) shapeA, sweepA );
				//case kShapeType_Tri:		return IntersectsTriTri( (const ycTri&) shapeA, (const ycTri&) shapeB );
				case kShapeType_LineSeg:	return SweepLineSegTri( fraction, normal, ((const ycLineSeg&) shapeA) + sweepA, (const ycTri&) shapeB, sweepB );
				case kShapeType_Sphere:		
					{
#if 1
						//2d 
						const ycTri& triA = (const ycTri&) shapeA ;
						ycTri triWorld( triA.GetA()+sweepA.GetStart(), triA.GetB()+sweepA.GetStart(), triA.GetC()+sweepA.GetStart() );
						ycPlane edgePlanes[3];
						triWorld.GetEdgePlanes( &edgePlanes[0], &edgePlanes[1], &edgePlanes[2] );
						ycSphere sphereB = (const ycSphere&) shapeB;
						sphereB.center += sweepB.GetStart();
						ycVec3 dir = sweepB.GetOffset()-sweepA.GetOffset();
						ycVec3 normalTest = ycVec3::ZAXIS();
						normalTest = normalTest.Cross( ycVec3( 1.0f, 0.0f ) );
						bool hit = false;
						for( s32 i = 0; i < 3; ++i )
						{
							f32 d0 = edgePlanes[i].SignedDistance( sphereB.center );
							if( ycAbs( d0 ) <= sphereB.radius )
							{
								continue;
							}
							ycLineSeg seg = triWorld.GetEdge( i );
							f32 d1 = edgePlanes[i].SignedDistance( sphereB.center + dir );
							if( d0 > sphereB.radius && d1 < sphereB.radius )
							{
								f32 time = (d0 - sphereB.radius ) / ( d0 - d1 );
								if( time < *fraction )
								{
									ycSphere newSphere( sphereB.center + dir * time, sphereB.radius );
									ycVec3 point = newSphere.center + -edgePlanes[i].normal * sphereB.radius;
									ycVec3 dirSeg = seg.offset.Normalized();
									f32 dot = dirSeg.Dot( point - seg.start );
									if( ( dot ) <= 1.05f && ( dot ) >= -0.05f ) //not sure why the epsilon here
									{
										*fraction = time;
										*normal = edgePlanes[i].normal;
										hit = true;
									}
								}
							}
						}
						return hit;
#elif 0
						ycGeoRayTriHit hit; 
						ycSphere sphereB = (const ycSphere&) shapeB;
						sphereB.center += sweepB.GetStart();
						const ycTri& triA = (const ycTri&) shapeA ;
						ycVec3 dir = sweepB.GetOffset()-sweepA.GetOffset();
						ycTri triWorld( triA.GetA()+sweepA.GetStart(), triA.GetB()+sweepA.GetStart(), triA.GetC()+sweepA.GetStart() );
						return SweepSphereTri( &hit, sphereB, dir, 1.0f, triWorld );
#else
						ycSphere sphereB = (const ycSphere&) shapeB;
						ycCollisionSphere sphere;
						ycMtx4 sphereBox = ycMtx4::IDENTITY();
						sphereBox.t.SetXYZ( sphereB.center + sweepB.GetStart() );
						sphere.forward = sphereBox;
						sphere.inverse = sphereBox.Inverted();
						sphere.radius = sphereB.radius;

						const ycTri& triA = (const ycTri&) shapeA ;
						ycTri triWorld( triA.GetA()+sweepB.GetStart(), triA.GetB()+sweepB.GetStart(), triA.GetC()+sweepB.GetStart() );
						ycVec3 dir = sweepA.GetOffset()-sweepB.GetOffset();//think this function should be reversed
						f32 dist = dir.Normalize();
						ycCollisionResult contactPoint;
						bool anyCollide = ycCollisionSAT::SatIterativeSweep( sphere, triWorld, dir, dist, &contactPoint, ycCollisionOptions::kBackface_Ignore, false );
						if( anyCollide )
						{
							*normal = contactPoint.dir;
							*fraction = contactPoint.sweepDist / contactPoint.fullSweepDist;
							return true;
						}
						return false;
#endif
					}
					break;
			}
			break;
		case kShapeType_Sphere:
			switch( shapeBType )
			{
				case kShapeType_AABB:		
				{
					bool contact = SweepAABBSphere( fraction, normal, (const ycAABB&) shapeB, sweepB, (const ycSphere&) shapeA, sweepA );
					if( contact )
					{
						*normal = -*normal;
						return true;
					}
				}
				case kShapeType_Tri:		return Sweep( fraction, normal, shapeBType, shapeB, sweepB, shapeAType, shapeA, sweepA, flags );
				case kShapeType_LineSeg:	return SweepLineSegSphere( fraction, normal, ((const ycLineSeg&) shapeB) + sweepB, (const ycSphere&) shapeA, sweepA );
				case kShapeType_Sphere:		return SweepSphereSphere( fraction, normal, ((const ycSphere&) shapeA), sweepA, (const ycSphere&) shapeB, sweepB );
			}
			break;
	}
	ycAssertMsg( 0, "Not Supported" );
	return false;
}

//////////////
// CONTAINS //
//////////////

bool ycGeo::ContainsAABBPt( const ycAABB& aabb, const ycVec3& pt )
{
	ycVec3 min = aabb.GetMin();
	ycVec3 max = aabb.GetMax();
	if( pt.x < min.x
		|| pt.y < min.y
		|| pt.z < min.z
		|| pt.x > max.x
		|| pt.y > max.y
		|| pt.z > max.z )
	{
		return false;
	}
	return true;
}

bool ycGeo::ContainsAABBAABB(  const ycAABB& a, const ycAABB& b )
{
	ycVec3 min = a.GetMin();
	ycVec3 max = a.GetMax();
	ycVec3 otherMin = b.GetMin();
	ycVec3 otherMax = b.GetMax();
	if( otherMin.x < min.x
		|| otherMin.y < min.y
		|| otherMin.z < min.z
		|| otherMax.x > max.x
		|| otherMax.y > max.y
		|| otherMax.z > max.z ) 
	{
		return false;
	}
	return true;
}

bool ycGeo::ContainsLineSegPt( const ycLineSeg& line, const ycVec3& pt )
{
	//f32 length = (line.m_dir).MagSq();
	//return pt.DistSq( line.m_start ) <= length && pt.DistSq( line.GetEnd() ) <= length;
	f32 dot = ( line.GetStart() - pt ).Dot( line.GetEnd() - pt );
	return dot <= 0.0f;
}

bool ycGeo::ContainsTriPt( const ycTri& tri, const ycVec3&  pt)
{
	// Special thanks to "Real-Time Collision Detection" by Christer Ericson
	// for it's guidance on pages 136 to 142, section 5.1.5
	ycVec3 cp;
	ClosestPointTri( tri, pt, &cp );
	return ycIsZero( pt.DistSq( cp ) );
}

bool ycGeo::ContainsSpherePt( const ycSphere& sphere, const ycVec3& pt )
{
	if( pt.DistSq( sphere.center ) > ( sphere.radius * sphere.radius ) )
	{
		return false;
	}
	return true;
}

bool ycGeo::ContainsPlanePt( const ycPlane& plane, const ycVec3& pt )
{
	f32 dist = plane.SignedDistance( pt );
	if( ycAbs( dist ) < YC_F32_EPSILON )
	{
		return true;
	}
	return false;
}

bool ycGeo::ContainsCirclePt( const ycCircle& circle, const ycVec3& pt )
{
	if( circle.ToPlane().Side( pt ) == ycPlane::kPlaneSide_Planar )
	{
		if( pt.DistSq( circle.center ) > ( circle.radius * circle.radius ) )
		{
			return false;
		}
		return true;
	}
	return false;
}

bool ycGeo::ContainsFrustumPt( const ycFrustum& frustum, const ycVec3& pt )
{
	for( s32 i = 0; i < ycFrustum::kPlane_Count; i++ )
	{
		const ycPlane& p = frustum.planes[ i ];
		if( !p.IsFinite() ) { continue; }

		if( p.SignedDistance( pt ) < 0 )
		{
			return false;
		}
	}
	return true;
}

bool ycGeo::ContainsCapsulePt( const ycCapsule& capsule, const ycVec3& point )
{
	return DistLineSegPoint( ycLineSeg( capsule.bottom, capsule.top ), point ) <= capsule.radius;
}

bool ycGeo::ContainsBoxPt( const ycMtx4& box, const ycVec3& pt )
{
	ycVec3 delta = pt - box.t.xyz();

	if( ycAbs( box.x.xyz().Dot( delta ) ) > box.x.xyz().LengthSq() ) { return false; }
	if( ycAbs( box.y.xyz().Dot( delta ) ) > box.y.xyz().LengthSq() ) { return false; }
	if( ycAbs( box.z.xyz().Dot( delta ) ) > box.z.xyz().LengthSq() ) { return false; }

	return true;
}

//http://alienryderflex.com/polygon/
bool ycGeo::ContainsEORPolygonPt( const ycVec2 * polyPoints, u32 ptCount, const ycVec3& point )
{
	bool contains = false;
	for(u32 i = 0, j = ptCount - 1; i < ptCount; j = i++) 
	{
		if( ((polyPoints[i].y > point.y) != (polyPoints[j].y > point.y)) 
			&& (point.x < (polyPoints[j].x - polyPoints[i].x) * (point.y - polyPoints[i].y) / (polyPoints[j].y - polyPoints[i].y) + polyPoints[i].x)) 
		{
			contains = !contains;
		}
	}
  return contains;
}

void ycGeo::ToAABB( u32 shapeAType, const ycShape& shapeA, ycAABB * out )
{
	switch( shapeAType )
	{
		case kShapeType_AABB: *out = (const ycAABB&)shapeA; return;
		case kShapeType_Tri: *out = ((const ycTri&)shapeA).ToAABB(); return;
		case kShapeType_Sphere: *out = ycAABB(  ((const ycSphere&)shapeA).center, ((const ycSphere&)shapeA).radius, ((const ycSphere&)shapeA).radius, ((const ycSphere&)shapeA).radius ); return;
		case kShapeType_LineSeg: *out = ycAABB( ((const ycLineSeg&)shapeA).GetStart(), 0.0f, 0.0f, 0.0f ); out->Expand( ((const ycLineSeg&)shapeA).GetEnd() );
	}
	ycAssertMsg( 0, "Not Supported" );
}

ycVec3 ycGeo::GetLocalVertex( const u32 shapeAType, const ycShape& shapeA, const ycVec3 & dir, f32 margin )
{
	switch( shapeAType )
	{
		case kShapeType_AABB: return ((const ycAABB&)shapeA).GetLocalVertex( dir, margin ); break;
		case kShapeType_Tri: return ((const ycTri&)shapeA).GetLocalVertex( dir, margin ); break;
		case kShapeType_Sphere: return ((const ycSphere&)shapeA).GetLocalVertex( dir, margin ); break;
		default: ycAssertMsg( 0, "Not Supported" );
	}
	return ycVec3::ZERO();
}

f32 ycGeo::SignedArea( const ycVec2* pts, u32 ptsCount )
{
	f32 area = 0.0f;
	u32 last = ptsCount - 1;
	for( u32 i = 0; i < ptsCount; ++i )
	{
		area += ( pts[ last ].x + pts[ i ].x ) * ( pts[ last ].y - pts[ i ].y );
		last = i;
	}
	area *= 0.5f;
	return area;
}

f32 ycGeo::Area( const ycVec2* pts, u32 ptsCount )
{
	return ycAbs( ycGeo::SignedArea( pts, ptsCount ) );
}

void ycGeo::SetTriMatchAxis( bool match )
{
	s_triMatchAxis = match;
}

/*
#ifdef YC_TEST
#include "ycTest.h"
YC_BEGIN_TEST( ycGeo_ClosestPointSeg_Vec3 )
{
	ycVec3 point;
	ycVec3 expectedClosestPoint;
	ycVec3 actualClosestPoint;
	f32 tolerance = 0.00005f;
	// test point is on line
	point.Set( 0.f, 0.f, 0.f );
	segment.SetStart( 1.f, 1.f, 1.f );
	segment.SetEnd( -1.f, -1.f, -1.f );
	expectedClosestPoint.Set( 0.f, 0.f, 0.f );
	ycGeo::ClosestPointSeg( &actualClosestPoint, segment, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	// point somewhere inbetween endpoints is closest
	point.Set( -1.f, 0.f, 1.f );
	segment.SetStart( 1.f, 1.f, 1.f );
	segment.SetEnd( -1.f, -1.f, -1.f );
	expectedClosestPoint.Set( 0.f, 0.f, 0.f );
	ycGeo::ClosestPointSeg( &actualClosestPoint, segment, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	// start point closest.
	point.Set( 2.f, 3.f, 3.f );
	segment.SetStart( 1.f, 1.f, 1.f );
	segment.SetEnd( -1.f, -1.f, -1.f );
	expectedClosestPoint.Set( 1.f, 1.f, 1.f );
	ycGeo::ClosestPointSeg( &actualClosestPoint, segment, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	// endpoint closest
	point.Set( -2.f, -3.f, -3.f );
	segment.SetStart( 1.f, 1.f, 1.f );
	segment.SetEnd( -1.f, -1.f, -1.f );
	expectedClosestPoint.Set( -1.f, -1.f, -1.f );
	ycGeo::ClosestPointSeg(  &actualClosestPoint, segment, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	YC_TEST_PASS( "ycGeo::ClosestPointSeg" );
}
YC_BEGIN_TEST( ycGeo_ClosestPointRay_Vec3 )
{
	ycVec3 point;
	ycRay ray;
	ycVec3 expectedClosestPoint;
	ycVec3 actualClosestPoint;
	f32 tolerance = 0.00005f;

	// point on ray
	point.Set( 1.5f, 1.5f, 1.5f );
	ray.SetOrigin( 0.f, 0.f, 0.f );
	ray.SetSlope( 1.f, 1.f, 1.f );
	expectedClosestPoint.Set( 1.5f, 1.5f, 1.5f );
	ycGeo::ClosestPointRay( &actualClosestPoint, ray, point);
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	// point behind ray
	point.Set( -2.f, -3.f, -3.f );
	ray.SetOrigin( 0.f, 0.f, 0.f );
	ray.SetSlope( 1.f, 1.f, 1.f );
	expectedClosestPoint.Set( 0.f, 0.f, 0.f );
	ycGeo::ClosestPointRay( &actualClosestPoint, ray, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	//  testing x-axis	
	point.Set( -0.5f, 3.f, 0.f );
	ray.SetOrigin( -1.f, 0.f, 0.f );
	ray.SetSlope( 1.f, 0.f, 0.f );
	expectedClosestPoint.Set( -0.5f, 0.f, 0.f );
	ycGeo::ClosestPointRay( &actualClosestPoint, ray, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	//  testing y-axis	
	point.Set( 2.f, 3.f, 3.f );
	ray.SetOrigin( 0.f, -1.f, 0.f );
	ray.SetSlope( 0.f, 1.f, 0.f );
	expectedClosestPoint.Set( 0.f, 3.f, 0.f );
	ycGeo::ClosestPointRay( &actualClosestPoint, ray, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	//  testing z-axis	
	point.Set( 0.f, 3.f, 5.f );
	ray.SetOrigin( 0.f, 0.f, -1.f );
	ray.SetSlope( 0.f, 0.f, 1.f );
	expectedClosestPoint.Set( 0.f, 0.f, 5.f );
	ycGeo::ClosestPointRay( &actualClosestPoint, ray, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	YC_TEST_PASS( "ycGeo::ClosestPointRay");
}
YC_BEGIN_TEST( ycGeo_ClosestPointLine_Vec3 )
{
	ycVec3 point;
	ycLine line;
	ycVec3 expectedClosestPoint;
	ycVec3 actualClosestPoint;
	f32 tolerance = 0.00005f;

	// point on ray
	point.Set( 1.5f, 1.5f, 1.5f );
	line.SetOrigin( 0.f, 0.f, 0.f );
	line.SetSlope( 1.f, 1.f, 1.f );
	expectedClosestPoint.Set( 1.5f, 1.5f, 1.5f );
	ycGeo::ClosestPointLine( &actualClosestPoint, line, point);
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	// point behind line
	point.Set( -2.f, -2.f, -2.f );
	line.SetOrigin( 0.f, 0.f, 0.f );
	line.SetSlope( 1.f, 1.f, 1.f );
	expectedClosestPoint.Set( -2.f, -2.f, -2.f );
	ycGeo::ClosestPointLine( &actualClosestPoint, line, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	//  testing x-axis	
	point.Set( -10.f, 3.f, 0.f );
	line.SetOrigin( -1.f, 0.f, 0.f );
	line.SetSlope( 1.f, 0.f, 0.f );
	expectedClosestPoint.Set( -10.f, 0.f, 0.f );
	ycGeo::ClosestPointLine( &actualClosestPoint, line, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	//  testing y-axis	
	point.Set( 2.f, 3.f, 3.f );
	line.SetOrigin( 0.f, -1.f, 0.f );
	line.SetSlope( 0.f, 1.f, 0.f );
	expectedClosestPoint.Set( 0.f, 3.f, 0.f );
	ycGeo::ClosestPointLine( &actualClosestPoint, line, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	//  testing z-axis	
	point.Set( 0.f, 3.f, 5.f );
	line.SetOrigin( 0.f, 0.f, -1.f );
	line.SetSlope( 0.f, 0.f, 1.f );
	expectedClosestPoint.Set( 0.f, 0.f, 5.f );
	ycGeo::ClosestPointLine( &actualClosestPoint, line, point );
	YC_TEST_CHECK( actualClosestPoint.IsEqual( expectedClosestPoint, tolerance ) );

	YC_TEST_PASS( "ycGeo::ClosestPointLine_Vec3");
} 
#endif//YC_TEST
*/
