#include "ycCapsule.h"

#include "ycQuat.h"
#include "ycCircle.h"
#include "ycGeo.h"

ycVec3 ycCapsule::GetEndTop() const
{
	return top + GetUp() * radius;
}

ycVec3 ycCapsule::GetEndBottom() const
{
	return bottom - GetUp() * radius;
}

ycVec3 ycCapsule::GetCenter() const
{
	return (top + bottom) * 0.5f;
}

f32 ycCapsule::GetCentersMag() const
{
	return top.Dist( bottom );
}

f32 ycCapsule::GetHeight() const
{
	return GetCentersMag() + radius*2.0f;
}

ycVec3 ycCapsule::GetUp() const
{
	ycVec3 diff = (top-bottom);
	if( diff == ycVec3::ZERO() )
	{
		return ycVec3::YAXIS();
	}
	return diff.Normalized();
}

f32 ycCapsule::Volume() const
{
	return YC_PI * radius * radius * GetCentersMag() + 4.0f * YC_PI * radius * radius * radius / 3.0f;
}

f32 ycCapsule::SurfaceArea() const
{
	return YC_TWO_PI * radius * GetCentersMag() + 4.0f * YC_PI * radius * radius;
}

ycCircle ycCapsule::CrossSection( const f32 height ) const
{
	f32 scale = height;
	scale = ycClamp( scale, 0.0f, 1.0f );
	scale *= GetHeight();
	ycVec3 up = GetUp();
	ycVec3 circleCenter = GetEndBottom() + up * scale;
	if( scale < radius )
	{
		return ycCircle( circleCenter, ycSqrt( (radius * radius) - (radius - scale) * (radius - scale ) ), up );
	}
	f32 centerLength = GetCentersMag();
	if( scale < (centerLength + radius) )
	{
		return ycCircle( circleCenter, radius, up );
	}
	else
	{
		f32 d = scale - radius - centerLength;
		return ycCircle( circleCenter, ycSqrt( radius*radius + d*d ), up );
	}
}

ycCapsule& ycCapsule::Scale( const f32 scale )
{
	ycVec3 dir = (top - bottom) * 0.5f;
	ycVec3 mid = bottom + dir;
	top = mid + dir * scale;
	bottom = mid - dir * scale;
	radius *= scale;
	return *this;
}

ycCapsule& ycCapsule::Rotate( const ycQuat & rotation )
{
	ycVec3 dir = (top - bottom) * 0.5f;
	ycVec3 mid = bottom + dir;
	ycVec3 rotDir = rotation.Rotate( dir );
	top = mid + rotDir;
	bottom = mid - rotDir;
	return *this;
}

ycVec3 ycCapsule::Closest( const ycVec3 & pt ) const
{
	ycVec3 closest;
	ycGeo::ClosestPointCapsule( *this, pt, &closest );
	return closest;
}

f32 ycCapsule::DistSq( const ycVec3 & pt ) const
{
	return ycGeo::DistSqCapsulePoint( *this, pt );
}

f32 ycCapsule::Dist( const ycVec3 & pt ) const
{
	return ycGeo::DistCapsulePoint( *this, pt );
}

f32 ycCapsule::DistSq( const ycLineSeg & lineSeg ) const
{
	return ycGeo::DistSqCapsuleLineSeg( *this, lineSeg );
}

f32 ycCapsule::Dist( const ycLineSeg & lineSeg ) const
{
	return ycGeo::DistSqCapsuleLineSeg( *this, lineSeg );
}

f32 ycCapsule::DistSq( const ycCapsule & other ) const
{
	return ycGeo::DistSqCapsuleCapsule( *this, other );
}

f32 ycCapsule::Dist( const ycCapsule & other ) const
{
	return ycGeo::DistCapsuleCapsule( *this, other );
}

f32 ycCapsule::DistSq( const ycSphere & sphere ) const
{
	return ycGeo::DistSqCapsuleSphere( *this, sphere );
}

f32 ycCapsule::Dist( const ycSphere & sphere ) const
{
	return ycGeo::DistCapsuleSphere( *this, sphere );
}

f32 ycCapsule::Dist( const ycPlane & plane ) const
{
	return ycGeo::DistCapsulePlane( *this, plane );
}

bool ycCapsule::Contains( const ycVec3 & pt ) const
{
	return ycGeo::ContainsCapsulePt( *this, pt );
}

bool ycCapsule::Intersects( const ycCapsule & other ) const
{
	return ycGeo::IntersectsCapsuleCapsule( *this, other );
}

bool ycCapsule::Intersects( const ycSphere & sphere ) const
{
	return ycGeo::IntersectsCapsuleSphere( *this, sphere );
}

bool ycCapsule::Intersects( const ycLineSeg & lineSeg ) const
{
	return ycGeo::IntersectsCapsuleLineSeg( *this, lineSeg );
}

bool ycCapsule::Intersects( const ycPlane & plane ) const
{
	return ycGeo::IntersectsCapsulePlane( *this, plane );
}

void ycCapsule::ToVertCount( u32 * ptsCount, u32* faceCount, const u32 segments/* = 32*/ ) const
{
	const u32 kMaxSegments = 64;
	u32 segs = ycClamp( segments, (u32)3, kMaxSegments );
	*faceCount = 0;
	*ptsCount = 0;
	for( u32 i = 0; i < segs; i++ )
	{
		*ptsCount += 2;
		*faceCount += 2;
#if 1
		for( u32 longitude = 0; longitude < segs; longitude++ )
		{
			bool last = longitude == (segs-1);
			if( !last )
			{
				*ptsCount += 2;
			}
			if( longitude == 0 )
			{
				*faceCount += 4;
			}
			else if( last )
			{
				*faceCount += 2;
			}
			else
			{
				*faceCount += 4;
			}
		}
#endif
	}
	//add the sphere points
	*ptsCount += 2;
}

void ycCapsule::ToVerts( ycVec3 * pts, u32 * ptsCount, u32 * faces, u32* faceCount, const u32 segments/* = 32*/ ) const
{
	const u32 kMaxSegments = 64;
	u32 segs = ycClamp( segments, (u32)3, kMaxSegments );
	*ptsCount = 0;
	*faceCount = 0;
	ycVec3 up = GetUp();
	ycVec3 forward = up.Perpendicular();
	ycVec3 right = up.Cross( forward ).Normalized();
	ycVec3 bottomEnd = GetEndBottom();
	ycVec3 topEnd = GetEndTop();
	*faceCount = 0;
	f32 angle = (YC_TWO_PI) / (f32) segs;
	u32 lastSeg = segs - 1;
	s32 cylinderEnd = segs*2;
	u32 longitudeSegs = segs-1;
	s32 topStartV = cylinderEnd + (longitudeSegs*segs);
	s32 bottomI = topStartV + (longitudeSegs*segs);
	s32 topI = bottomI+1;
	for( u32 i = 0; i < segs; i++ )
	{
		f32 theta = i * angle;
		f32 thetaCos = ycCos( theta );
		f32 thetaSin = ycSin( theta );

		pts[i] = top + (thetaCos * radius * forward) + (thetaSin * radius * right);
		(*ptsCount)++;
		pts[segs+i] = bottom + (thetaCos * radius * forward) + (thetaSin * radius * right);
		(*ptsCount)++;
		u32 vertIndex0 = i;
		u32 vertIndex1 = lastSeg;
		u32 vertIndex2 = lastSeg + segs;
		u32 vertIndex3 = i + segs;
		faces[*faceCount*3] = vertIndex0;
		faces[*faceCount*3+1] = vertIndex1;
		faces[*faceCount*3+2] = vertIndex2;
		(*faceCount)++;

		faces[*faceCount*3] = vertIndex0;
		faces[*faceCount*3+1] = vertIndex2;
		faces[*faceCount*3+2] = vertIndex3;
		(*faceCount)++;
#if 1
		for( u32 longitude = 0; longitude < segs; longitude++ )
		{
			bool last = longitude == longitudeSegs;
			f32 longitudeRad = YC_PI_OVER_2 * ((((f32)longitude+1)) / ((f32) segs));
			f32 longitudeSin = ycCos( longitudeRad );
			f32 longitudeCos = ycSin( longitudeRad );
			//vert order is cylinders then bottom semisphere then top semisphere
			if( !last )
			{
				s32 ptIndex = cylinderEnd + (longitudeSegs*i)+longitude;
				pts[ptIndex] = bottom + (radius * -up) * longitudeCos + (thetaCos * radius * forward) * longitudeSin + (thetaSin * radius * right) * longitudeSin;
				(*ptsCount)++;

				ptIndex = topStartV + (longitudeSegs*i)+longitude;
				pts[ptIndex] = top + (radius * up) * longitudeCos + (thetaCos * radius * forward) * longitudeSin + (thetaSin * radius * right) * longitudeSin;
				(*ptsCount)++;
			}
			//connect to the cylinder
			if( longitude == 0 )
			{
				//bottom
				vertIndex0 = cylinderEnd + (longitudeSegs*lastSeg)+longitude;
				vertIndex1 = cylinderEnd + (longitudeSegs*i)+longitude;
				vertIndex2 = segs+i;
				vertIndex3 = segs+lastSeg;
				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;

				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex2;
				faces[*faceCount*3+2] = vertIndex3;
				(*faceCount)++;

				//top
				vertIndex0 = i;
				vertIndex1 = topStartV + (longitudeSegs*i)+longitude;
				vertIndex2 = topStartV + (longitudeSegs*lastSeg)+longitude;
				vertIndex3 = lastSeg;
				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;

				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex2;
				faces[*faceCount*3+2] = vertIndex3;
				(*faceCount)++;
			}
			//connect to the top point
			else if( last )
			{
				//bottom
				vertIndex0 = cylinderEnd + (longitudeSegs*i)+longitude-1;
				vertIndex1 = cylinderEnd + (longitudeSegs*lastSeg)+longitude-1;
				vertIndex2 = bottomI;
				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;

				//top
				vertIndex0 = topI;
				vertIndex1 = topStartV + (longitudeSegs*lastSeg)+longitude-1;
				vertIndex2 = topStartV + (longitudeSegs*i)+longitude-1;
				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;
			}
			else
			{
				//bottom
				vertIndex0 = cylinderEnd + (longitudeSegs*lastSeg)+longitude-1;
				vertIndex1 = cylinderEnd + (longitudeSegs*lastSeg)+longitude;
				vertIndex2 = cylinderEnd + (longitudeSegs*i)+longitude;
				vertIndex3 = cylinderEnd + (longitudeSegs*i)+longitude-1;
				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;

				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex2;
				faces[*faceCount*3+2] = vertIndex3;
				(*faceCount)++;

				//top
				vertIndex0 = topStartV + (longitudeSegs*i)+longitude;
				vertIndex1 = topStartV + (longitudeSegs*lastSeg)+longitude;
				vertIndex2 = topStartV + (longitudeSegs*lastSeg)+longitude-1;
				vertIndex3 = topStartV + (longitudeSegs*i)+longitude-1;
				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;

				faces[*faceCount*3] = vertIndex0;
				faces[*faceCount*3+1] = vertIndex2;
				faces[*faceCount*3+2] = vertIndex3;
				(*faceCount)++;
			}
		}
#endif

		lastSeg = i;
	}

	//add the sphere points
	pts[bottomI] = bottomEnd;
	pts[topI] = topEnd;
	(*ptsCount)++;
	(*ptsCount)++;
}
