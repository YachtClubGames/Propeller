#include "ycCylinder.h"
#include "ycAABB.h"
#include "ycQuat.h"

ycVec3 ycCylinder::GetPoint( const ycVec3 & _normal ) const
{
	ycVec3 newNormal = ycQuat( ycVec3( 0, 1.0f, 0 ), normal ).RotateInverse( _normal );
	return center + newNormal * radius;
}

ycAABB ycCylinder::ToAABB() const
{
	ycAABB aabb;
	aabb.center = center;
	ycVec3 extents = ycQuat( ycVec3( 0, 1.0f, 0 ), normal ).Rotate( ycVec3( radius.x, radius.y, radius.z ) );
	aabb.extents.x = ycAbs( extents.x );
	aabb.extents.y = ycAbs( extents.y );
	aabb.extents.z = ycAbs( extents.z );
	return aabb;
}

void ycCylinder::ToVertCount( u32 * ptsCount, u32* faceCount, const u32 segments, bool capCenterVert, bool capTop, bool capBot, bool centerFaces ) const
{
	const u32 kMaxSegments = 64;
	u32 segs = ycClamp( segments, (u32)3, kMaxSegments );
	*faceCount = 0;
	*ptsCount = 0;
	if( capCenterVert )
	{
		if( capBot )
		{
			*ptsCount += 1;
		}
		if( capTop )
		{
			*ptsCount += 1;
		}
	}
	for( u32 i = 0; i < segs; i++ )
	{
		*ptsCount += 2;
		if( centerFaces )
		{
			*faceCount += 2;
		}
		if( capCenterVert )
		{
			if( capBot )
			{
				*faceCount += 1;
			}

			if( capTop )
			{
				*faceCount += 1;
			}
		}
		else if( i >= 2 )
		{
			if( capBot )
			{
				*faceCount += 1;
			}

			if( capTop )
			{
				*faceCount += 1;
			}
		}
	}
}

void ycCylinder::ToVerts( ycVec3 * pts, u32 * ptsCount, u32 * faces, u32* faceCount, const u32 segments, bool capCenterVert, bool capTop, bool capBot, f32 radiusPerc, bool centerFaces ) const
{
	*ptsCount = 0;
	*faceCount = 0;

	const u32 kMaxSegments = 64;
	u32 segs = ycClamp( segments, (u32)3, kMaxSegments );
	ycVec3 bottom = ycVec3( center.x, center.y, center.z ) - normal * radius.y;
	ycVec3 top = ycVec3( center.x, center.y, center.z ) + normal * radius.y;

	s32 capCenterBot = 0;
	s32 capCenterTop = 0;
	s32 ptOffset = 0;
	if( capCenterVert )
	{
		if( capBot )
		{
			capCenterBot = ptOffset;
			pts[capCenterBot] = bottom;
			(*ptsCount)++;
			ptOffset++;
		}
		if( capTop )
		{
			capCenterTop = ptOffset;
			pts[capCenterTop] = top;
			(*ptsCount)++;
			ptOffset++;
		}
	}
	
	f32 angle = ( YC_TWO_PI * radiusPerc ) / (f32)segs;
	u32 lastSeg = segs - 1;
	for( u32 i = 0; i < segs; i++ )
	{
		f32 theta = i * angle;
		f32 thetaCos = ycCos( theta );
		f32 thetaSin = ycSin( theta );

		pts[ptOffset+i].x = bottom.x + (thetaCos * radius.x);
		pts[ptOffset+i].y = bottom.y;
		pts[ptOffset+i].z = bottom.z + (thetaSin * radius.z);
		(*ptsCount)++;
		pts[ptOffset+(segs+i)].x = top.x + (thetaCos * radius.x);
		pts[ptOffset+(segs+i)].y = top.y;
		pts[ptOffset+(segs+i)].z = top.z + (thetaSin * radius.z);
		(*ptsCount)++;
		u32 vertIndex0 = ptOffset + (i);
		u32 vertIndex1 = ptOffset + (lastSeg);
		u32 vertIndex2 = ptOffset + (lastSeg + segs);
		u32 vertIndex3 = ptOffset + (i + segs);

		if( centerFaces )
		{
			faces[*faceCount*3] = vertIndex0;
			faces[*faceCount*3+1] = vertIndex1;
			faces[*faceCount*3+2] = vertIndex2;
			(*faceCount)++;
			faces[*faceCount*3] = vertIndex0;
			faces[*faceCount*3+1] = vertIndex2;
			faces[*faceCount*3+2] = vertIndex3;
			(*faceCount)++;
		}

		if( capCenterVert )
		{
			if( capBot )
			{
				faces[*faceCount*3] = capCenterBot;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex0;
				(*faceCount)++;
			}

			if( capTop )
			{
				faces[*faceCount*3] = capCenterTop;
				faces[*faceCount*3+1] = vertIndex3;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;
			}
		}
		else if( i >= 2 )
		{
			if( capBot )
			{
				faces[*faceCount*3] = ptOffset;
				faces[*faceCount*3+1] = vertIndex1;
				faces[*faceCount*3+2] = vertIndex0;
				(*faceCount)++;
			}

			if( capTop )
			{
				faces[*faceCount*3] = ptOffset+segs;
				faces[*faceCount*3+1] = vertIndex3;
				faces[*faceCount*3+2] = vertIndex2;
				(*faceCount)++;
			}
		}
		lastSeg = i;
	}
}