#include "ycCollisionFeatureUtil.h"

#include "ycCollisionWelding.h"
#include "ycGeo.h"

bool ycCollisionFeatureUtil::EdgesCollinear( const ycVec3& p0, const ycVec3& p1, const ycVec3& p2, const ycVec3& p3 )
{
	if( ( p3 - p1 ).Cross( p0 - p1 ).LengthSq() > 0.0001f ) { return false; }
	if( ( p3 - p1 ).Cross( p2 - p1 ).LengthSq() > 0.0001f ) { return false; }

	// the four points are collinear, search for line overlap
	if( ( p3 - p1 ).Dot( p2 - p1 ) > 0.0f && ( p3 - p1 ).Dot( p2 - p1 ) <= ( p3 - p1 ).LengthSq() ) { return true; }
	if( ( p3 - p1 ).Dot( p0 - p1 ) > 0.0f && ( p3 - p1 ).Dot( p0 - p1 ) <= ( p3 - p1 ).LengthSq() ) { return true; }
	if( ( p2 - p0 ).Dot( p3 - p0 ) > 0.0f && ( p2 - p0 ).Dot( p3 - p0 ) <= ( p2 - p0 ).LengthSq() ) { return true; }
	if( ( p2 - p0 ).Dot( p1 - p0 ) > 0.0f && ( p2 - p0 ).Dot( p1 - p0 ) <= ( p2 - p0 ).LengthSq() ) { return true; }

	// no overlap found
	return false;
}

u32 ycCollisionFeatureUtil::EdgeIsSharedByFace( const ycVec3& e0, const ycVec3& e1, const ycCollisionFeature& face )
{
	auto Eq = []( const ycVec3& aa, const ycVec3 bb ) { return aa.Dist( bb ) < 0.0001f; };

	if( face.type == ycCollision::kFeature_FaceTri )
	{
		if( EdgesCollinear( face.pts[ 0 ], e1, face.pts[ 1 ], e0 ) ) { return 1; }
		if( EdgesCollinear( face.pts[ 1 ], e1, face.pts[ 2 ], e0 ) ) { return 2; }
		if( EdgesCollinear( face.pts[ 2 ], e1, face.pts[ 0 ], e0 ) ) { return 3; }
	}
	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		if( EdgesCollinear( face.pts[ 0 ], e1, face.pts[ 1 ], e0 ) ) { return 1; }
		if( EdgesCollinear( face.pts[ 1 ], e1, face.pts[ 2 ], e0 ) ) { return 2; }
		if( EdgesCollinear( face.pts[ 2 ], e1, face.pts[ 3 ], e0 ) ) { return 3; }
		if( EdgesCollinear( face.pts[ 3 ], e1, face.pts[ 0 ], e0 ) ) { return 4; }
	}
	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		// disc has no edges so they can't match
	}
	else
	{
		// face is not a Face
		ycAssert( 0 );
	}

	return 0;
}

bool ycCollisionFeatureUtil::FaceSharesEdgesWithFace( const ycCollisionFeature& faceA, const ycCollisionFeature& faceB )
{
	if( !faceA.IsFace() || !faceB.IsFace() ) { return false; }
	if( faceA.type == ycCollision::kFeature_FaceDisc || faceB.type == ycCollision::kFeature_FaceDisc ) { return false; }

	if( faceA.type == ycCollision::kFeature_FaceTri )
	{
		if( EdgeIsSharedByFace( faceA.pts[0], faceA.pts[1], faceB ) ) { return true; }
		if( EdgeIsSharedByFace( faceA.pts[1], faceA.pts[2], faceB ) ) { return true; }
		if( EdgeIsSharedByFace( faceA.pts[2], faceA.pts[0], faceB ) ) { return true; }
	}
	else if( faceA.type == ycCollision::kFeature_FaceQuad )
	{
		if( EdgeIsSharedByFace( faceA.pts[0], faceA.pts[1], faceB ) ) { return true; }
		if( EdgeIsSharedByFace( faceA.pts[1], faceA.pts[2], faceB ) ) { return true; }
		if( EdgeIsSharedByFace( faceA.pts[2], faceA.pts[3], faceB ) ) { return true; }
		if( EdgeIsSharedByFace( faceA.pts[3], faceA.pts[0], faceB ) ) { return true; }
	}
	else
	{
		// face is not a Face
		ycAssert( 0 );
	}

	return false;
}

u32 ycCollisionFeatureUtil::FaceMatchesEdge( const ycCollisionFeature& face, const ycCollisionFeature& edge )
{
	ycAssert( edge.type == ycCollision::kFeature_Edge );
	auto Eq = []( const ycVec3& aa, const ycVec3 bb ) { return aa.Dist( bb ) < 0.0001f; };

	if( face.type == ycCollision::kFeature_FaceTri )
	{
		if( Eq( face.pts[ 0 ], edge.pts[ 0 ] ) && Eq( face.pts[ 1 ], edge.pts[ 1 ] ) ) { return 1; }
		if( Eq( face.pts[ 0 ], edge.pts[ 1 ] ) && Eq( face.pts[ 1 ], edge.pts[ 0 ] ) ) { return 1; }
		if( Eq( face.pts[ 1 ], edge.pts[ 0 ] ) && Eq( face.pts[ 2 ], edge.pts[ 1 ] ) ) { return 2; }
		if( Eq( face.pts[ 1 ], edge.pts[ 1 ] ) && Eq( face.pts[ 2 ], edge.pts[ 0 ] ) ) { return 2; }
		if( Eq( face.pts[ 2 ], edge.pts[ 0 ] ) && Eq( face.pts[ 0 ], edge.pts[ 1 ] ) ) { return 3; }
		if( Eq( face.pts[ 2 ], edge.pts[ 1 ] ) && Eq( face.pts[ 0 ], edge.pts[ 0 ] ) ) { return 3; }
	}
	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		if( Eq( face.pts[ 0 ], edge.pts[ 0 ] ) && Eq( face.pts[ 1 ], edge.pts[ 1 ] ) ) { return 1; }
		if( Eq( face.pts[ 0 ], edge.pts[ 1 ] ) && Eq( face.pts[ 1 ], edge.pts[ 0 ] ) ) { return 1; }
		if( Eq( face.pts[ 1 ], edge.pts[ 0 ] ) && Eq( face.pts[ 2 ], edge.pts[ 1 ] ) ) { return 2; }
		if( Eq( face.pts[ 1 ], edge.pts[ 1 ] ) && Eq( face.pts[ 2 ], edge.pts[ 0 ] ) ) { return 2; }
		if( Eq( face.pts[ 2 ], edge.pts[ 0 ] ) && Eq( face.pts[ 3 ], edge.pts[ 1 ] ) ) { return 3; }
		if( Eq( face.pts[ 2 ], edge.pts[ 1 ] ) && Eq( face.pts[ 3 ], edge.pts[ 0 ] ) ) { return 3; }
		if( Eq( face.pts[ 3 ], edge.pts[ 0 ] ) && Eq( face.pts[ 0 ], edge.pts[ 1 ] ) ) { return 4; }
		if( Eq( face.pts[ 3 ], edge.pts[ 1 ] ) && Eq( face.pts[ 0 ], edge.pts[ 0 ] ) ) { return 4; }
	}
	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		// disc has no edges so they can't match
	}
	else
	{
		// face is not a Face
		ycAssert( 0 );
	}

	return 0;
}

u32 ycCollisionFeatureUtil::FaceContainsEdge( const ycCollisionFeature& face, const ycCollisionFeature& edge )
{
	ycAssert( edge.type == ycCollision::kFeature_Edge );

	if( face.type == ycCollision::kFeature_FaceTri )
	{
		if( EdgesCollinear( face.pts[ 0 ], edge.pts[ 0 ], face.pts[ 1 ], edge.pts[ 1 ] ) ) { return 1; }
		if( EdgesCollinear( face.pts[ 1 ], edge.pts[ 0 ], face.pts[ 2 ], edge.pts[ 1 ] ) ) { return 2; }
		if( EdgesCollinear( face.pts[ 2 ], edge.pts[ 0 ], face.pts[ 0 ], edge.pts[ 1 ] ) ) { return 3; }
	}
	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		if( EdgesCollinear( face.pts[ 0 ], edge.pts[ 0 ], face.pts[ 1 ], edge.pts[ 1 ] ) ) { return 1; }
		if( EdgesCollinear( face.pts[ 1 ], edge.pts[ 0 ], face.pts[ 2 ], edge.pts[ 1 ] ) ) { return 2; }
		if( EdgesCollinear( face.pts[ 2 ], edge.pts[ 0 ], face.pts[ 3 ], edge.pts[ 1 ] ) ) { return 3; }
		if( EdgesCollinear( face.pts[ 3 ], edge.pts[ 0 ], face.pts[ 0 ], edge.pts[ 1 ] ) ) { return 4; }
	}
	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		// disc has no edges so they can't match
	}
	else
	{
		// face is not a Face
		ycAssert( 0 );
	}

	return 0;
}

bool ycCollisionFeatureUtil::FaceMatchesVert( const ycCollisionFeature& face, const ycCollisionFeature& vert )
{
	ycAssert( vert.type == ycCollision::kFeature_Vertex );

	auto Eq = []( const ycVec3& aa, const ycVec3 bb ) { return aa.Dist( bb ) < 0.0001f; };

	if( face.type == ycCollision::kFeature_FaceTri )
	{

		if( Eq( face.pts[ 0 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 1 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 2 ], vert.pts[ 0 ] ) ) { return true; }
	}
	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		if( Eq( face.pts[0], vert.pts[0] ) ) { return true; }
		if( Eq( face.pts[1], vert.pts[0] ) ) { return true; }
		if( Eq( face.pts[2], vert.pts[0] ) ) { return true; }
		if( Eq( face.pts[3], vert.pts[0] ) ) { return true; }
	}
	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		// disc has no verts so they can't match
	}
	else
	{
		// face is not a Face
		ycAssert( 0 );
	}

	return false;
}

bool ycCollisionFeatureUtil::FaceContainsVert( const ycCollisionFeature& face, const ycCollisionFeature& vert )
{
	ycAssert( vert.type == ycCollision::kFeature_Vertex );

	auto Eq = []( const ycVec3& e0, const ycVec3& e1, const ycVec3 pt ) 
	{
		return ycLineSeg( e0, e1 ).Dist( pt ) < 0.0001f; 
	};

	if( face.type == ycCollision::kFeature_FaceTri )
	{
		if( Eq( face.pts[ 0 ], face.pts[ 1 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 1 ], face.pts[ 2 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 2 ], face.pts[ 0 ], vert.pts[ 0 ] ) ) { return true; }
	}
	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		if( Eq( face.pts[ 0 ], face.pts[ 1 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 1 ], face.pts[ 2 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 2 ], face.pts[ 3 ], vert.pts[ 0 ] ) ) { return true; }
		if( Eq( face.pts[ 3 ], face.pts[ 0 ], vert.pts[ 0 ] ) ) { return true; }
	}
	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		// disc has no verts so they can't match
	}
	else
	{
		// face is not a Face
		ycAssert( 0 );
	}

	return false;
}

bool ycCollisionFeatureUtil::FacesCoplanar( const ycCollisionFaceResult& faceA, const ycCollisionFaceResult& faceB )
{
	if( ycAbs( faceA.dir.Dot( faceB.featureB.pts[ 0 ] - faceA.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }
	if( ycAbs( faceA.dir.Dot( faceB.featureB.pts[ 1 ] - faceA.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }
	if( ycAbs( faceA.dir.Dot( faceB.featureB.pts[ 2 ] - faceA.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }
	if( ycAbs( faceA.dir.Dot( faceB.featureB.pts[ 3 ] - faceA.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }

	return true;
}

bool ycCollisionFeatureUtil::EdgeCoplanarFace( const ycCollisionResult& edge, const ycCollisionResult& face )
{
	if( ycAbs( face.dir.Dot( edge.featureB.pts[ 0 ] - face.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }
	if( ycAbs( face.dir.Dot( edge.featureB.pts[ 1 ] - face.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }

	return true;
}

bool ycCollisionFeatureUtil::PointInFace( const ycVec3& vv, const ycCollisionResult& face )
{
	if( !face.featureB.IsFace() ) { return false; }

	if( ycAbs( face.dir.Dot( vv - face.featureB.pts[ 0 ] ) ) > 0.01f ) { return false; }

	return FaceCircumscribesPoint( face.featureB, face.dir, vv );
}

bool ycCollisionFeatureUtil::FaceBelowFace( const ycCollisionResult& rBelow, const ycCollisionResult& rAbove )
{
	ycAssert( rAbove.faceB.valid || rAbove.featureB.IsFace() );
	ycAssert( rBelow.faceB.valid || rBelow.featureB.IsFace() );

	ycCollisionFaceResult faceBelow( rBelow ), faceAbove( rAbove );
	if( rBelow.faceB.valid ) { faceBelow = rBelow.faceB; } 
	if( rAbove.faceB.valid ) { faceAbove = rAbove.faceB; } 

	f32 elevation0 = faceAbove.dir.Dot( faceBelow.featureB.pts[ 0 ] - faceAbove.featureB.pts[ 0 ] );
	f32 elevation1 = faceAbove.dir.Dot( faceBelow.featureB.pts[ 1 ] - faceAbove.featureB.pts[ 0 ] );
	f32 elevation2 = faceAbove.dir.Dot( faceBelow.featureB.pts[ 2 ] - faceAbove.featureB.pts[ 0 ] );
	f32 elevation3 = faceAbove.dir.Dot( faceBelow.featureB.pts[ 3 ] - faceAbove.featureB.pts[ 0 ] );

	f32 tolerance = 0.00005f;

	if( elevation0 > tolerance ) { return false; }
	if( elevation1 > tolerance ) { return false; }
	if( elevation2 > tolerance ) { return false; }
	if( faceAbove.featureB.type == ycCollision::kFeature_FaceQuad && elevation3 > tolerance ) { return false; }

	// either the test face is below or it is perfectly parallel. If any vertex is below the face is not perfectly parallel thus it is below
	if( elevation0 < -tolerance ) { return true; }
	if( elevation1 < -tolerance ) { return true; }
	if( elevation2 < -tolerance ) { return true; }
	if( faceAbove.featureB.type == ycCollision::kFeature_FaceQuad && elevation3 < -tolerance ) { return true; }

	// perfect parallel case
	return false;
}
	
bool ycCollisionFeatureUtil::FaceCircumscribesPoint( const ycCollisionFeature& face, const ycVec3& faceNormal, const ycVec3& vv )
{
	ycVec3 PQ = faceNormal;

	if( face.type == ycCollision::kFeature_FaceTri )
	{
		ycVec3 PA = face.pts[ 0 ] - vv;
		ycVec3 PB = face.pts[ 1 ] - vv;
		ycVec3 PC = face.pts[ 2 ] - vv;

		f32 u = PQ.Dot( PC.Cross( PB ) );
		f32 v = PQ.Dot( PA.Cross( PC ) );
		f32 w = PQ.Dot( PB.Cross( PA ) );

		return ( ycSign( u ) == ycSign( v ) && ycSign( v ) == ycSign( w ) );
	}

	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		ycVec3 PA = face.pts[ 0 ] - vv;
		ycVec3 PB = face.pts[ 1 ] - vv;
		ycVec3 PC = face.pts[ 2 ] - vv;
		ycVec3 PD = face.pts[ 3 ] - vv;

		f32 u = PQ.Dot( PC.Cross( PB ) );
		f32 v = PQ.Dot( PA.Cross( PD ) );
		f32 w = PQ.Dot( PB.Cross( PA ) );
		f32 x = PQ.Dot( PD.Cross( PC ) );

		return ( ycSign( u ) == ycSign( v ) && ycSign( v ) == ycSign( w ) && ycSign( w ) == ycSign( x ) );
	}

	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		ycVec3 PA = face.pts[ 0 ] - vv;

		return( PA.Without( PQ ).Length() <= face.radius );
	}

	else
	{
		ycAssert( !"implement me" );
		return false;
	}
}

bool ycCollisionFeatureUtil::ResultBelowFace( const ycCollisionResult& faceBelow, const ycCollisionResult& faceAbove )
{
	if( faceAbove.faceB.dir.Dot( faceBelow.faceB.dir ) < 0.0f ) { return false; }

	if( !FaceBelowFace(faceBelow, faceAbove) ) { return false; }

	return FaceCircumscribesPoint( faceAbove.faceB.featureB, faceAbove.faceB.dir, faceBelow.supportingB );
}
	

ycVec3 ycCollisionFeatureUtil::GetFaceCenter( const ycCollisionFeature& face )
{
	if( face.type == ycCollision::kFeature_FaceTri )
	{
		ycVec3 PB = face.pts[ 1 ] - face.pts[ 0 ];
		ycVec3 PC = face.pts[ 2 ] - face.pts[ 0 ];

		return face.pts[ 0 ] + ( PB + PC ) / 3.0f;
	}

	else if( face.type == ycCollision::kFeature_FaceQuad )
	{
		ycVec3 PB = face.pts[ 1 ] - face.pts[ 0 ];
		ycVec3 PC = face.pts[ 2 ] - face.pts[ 0 ];
		ycVec3 PD = face.pts[ 3 ] - face.pts[ 0 ];

		return face.pts[ 0 ] + ( PB + PC + PD ) * 0.25f;
	}

	else if( face.type == ycCollision::kFeature_FaceDisc )
	{
		return face.pts[ 0 ];
	}

	else
	{
		ycAssert( !"implement me" );
		return face.pts[ 0 ];
	}
}

void ycCollisionFeatureUtil::SetFinalPoint( ycCollisionResult& mtd )
{
	mtd.dir.Normalize();

	switch( mtd.featureA.type )
	{
	case ycCollision::kFeature_Vertex:
		switch( mtd.featureB.type )
		{
			case ycCollision::kFeature_Vertex:
				{
					mtd.SetPointA( mtd.supportingA );
				}
				break;
			case ycCollision::kFeature_Edge:
				{
				// surely I did this for some good reason, but I can't figure out what
					//ycLineSeg lineB( mtd.featureB.pts[ 0 ], mtd.featureB.pts[ 1 ] );
					//if( mtd.featureB.radius > 0.0f )
					//{
					//	lineB.pos += ( mtd.supportingB - mtd.featureB.pts[ 0 ] ).Without( lineB.dir );
					//}
					//mtd.SetPointB( lineB.ClosestPoint( mtd.supportingA ) );
					mtd.SetPointA( mtd.supportingA );
				}
				break;
			case ycCollision::kFeature_FaceTri:
			case ycCollision::kFeature_FaceQuad:
			case ycCollision::kFeature_FaceDisc:   // vert v face
				{
					mtd.SetPointA( mtd.supportingA );
				}
				break;
			default: ycAssert( 0 );
		};
		break;
	case ycCollision::kFeature_Edge:
		switch( mtd.featureB.type )
		{
			case ycCollision::kFeature_Vertex:
				{
					//ycLineSeg lineA( mtd.featureA.pts[ 0 ], mtd.featureA.pts[ 1 ] );
					//if( mtd.featureA.radius > 0.0f )
					//{
					//	lineA.pos += ( mtd.supportingA - mtd.featureA.pts[ 0 ] ).Without( lineA.dir );
					//}
					//mtd.SetPointA( lineA.ClosestPoint( mtd.supportingB ) );
					mtd.SetPointB( mtd.supportingB );
				}
				break;
			case ycCollision::kFeature_Edge:  // edge v edge
				{
					ycLineSeg lineA( mtd.featureA.pts[ 0 ], mtd.featureA.pts[ 1 ] );
					ycLineSeg lineB( mtd.featureB.pts[ 0 ], mtd.featureB.pts[ 1 ] );

					if( mtd.featureA.radius > 0.0f )
					{
						lineA.pos += ( mtd.supportingA - mtd.featureA.pts[ 0 ] ).Without( lineA.dir );
					}
					if( mtd.featureB.radius > 0.0f )
					{
						lineB.pos += ( mtd.supportingB - mtd.featureB.pts[ 0 ] ).Without( lineB.dir );
					}

					ycVec3 ptA, ptB;
					ycGeo::ClosestPointsLineSegLineSeg( lineA, lineB, &ptA, &ptB );
					mtd.SetPointB( ptB );
				}
				break;
			case ycCollision::kFeature_FaceTri:
			case ycCollision::kFeature_FaceQuad:
			case ycCollision::kFeature_FaceDisc:   // edge v face
				{
					ycLineSeg lineA( mtd.featureA.pts[ 0 ], mtd.featureA.pts[ 1 ] );
					if( mtd.featureA.radius > 0.0f )
					{
						lineA.pos += ( mtd.supportingA - mtd.featureA.pts[ 0 ] ).Without( lineA.dir );
					}
					mtd.SetPointA( lineA.ClosestPoint( GetFaceCenter( mtd.featureB ) ) );
				}
				break;
			default: ycAssert( 0 );
		};
		break;
	case ycCollision::kFeature_FaceTri:
	case ycCollision::kFeature_FaceQuad:
	case ycCollision::kFeature_FaceDisc:   // a face
		switch( mtd.featureB.type )
		{
			case ycCollision::kFeature_Vertex:  // face v vert
				{
					mtd.SetPointB( mtd.supportingB );
				}
				break;
			case ycCollision::kFeature_Edge:  // face v edge
				{
					ycLineSeg lineB( mtd.featureB.pts[ 0 ], mtd.featureB.pts[ 1 ] );
					if( mtd.featureB.radius > 0.0f )
					{
						lineB.pos += ( mtd.supportingB - mtd.featureB.pts[ 0 ] ).Without( lineB.dir );
					}
					mtd.SetPointB( lineB.ClosestPoint( GetFaceCenter( mtd.featureA ) ) );
				}
				break;
			case ycCollision::kFeature_FaceTri:
			case ycCollision::kFeature_FaceQuad:
			case ycCollision::kFeature_FaceDisc:   // face v face
				{
					ycVec3 centerA = GetFaceCenter( mtd.featureA );
					ycVec3 centerB = GetFaceCenter( mtd.featureB );

					if( FaceCircumscribesPoint( mtd.featureB, mtd.dir, centerA ) )
					{
						mtd.SetPointA( centerA );
					}
					else if( FaceCircumscribesPoint( mtd.featureB, mtd.dir, mtd.supportingA ) )
					{
						mtd.SetPointA( mtd.supportingA );
					}
					else if( FaceCircumscribesPoint( mtd.featureA, mtd.dir, centerB ) )
					{
						mtd.SetPointB( centerB );
					}
					else if( FaceCircumscribesPoint( mtd.featureA, mtd.dir, mtd.supportingB ) )
					{
						mtd.SetPointB( mtd.supportingB );
					}
					else if( (mtd.featureA.type == ycCollision::kFeature_FaceTri || mtd.featureA.type == ycCollision::kFeature_FaceQuad )
						&& FaceCircumscribesPoint( mtd.featureB, mtd.dir, mtd.featureA.pts[ 0 ] ) )
					{
						mtd.SetPointA( mtd.featureA.pts[ 0 ] );
					}
					else if( (mtd.featureA.type == ycCollision::kFeature_FaceTri || mtd.featureA.type == ycCollision::kFeature_FaceQuad )
						&& FaceCircumscribesPoint( mtd.featureB, mtd.dir, mtd.featureA.pts[ 1 ] ) )
					{
						mtd.SetPointA( mtd.featureA.pts[ 1 ] );
					}
					else if( (mtd.featureA.type == ycCollision::kFeature_FaceTri || mtd.featureA.type == ycCollision::kFeature_FaceQuad )
						&& FaceCircumscribesPoint( mtd.featureB, mtd.dir, mtd.featureA.pts[ 2 ] ) )
					{
						mtd.SetPointA( mtd.featureA.pts[ 2 ] );
					}
					else if( mtd.featureA.type == ycCollision::kFeature_FaceQuad 
						&& FaceCircumscribesPoint( mtd.featureB, mtd.dir, mtd.featureA.pts[ 3 ] ) )
					{
						mtd.SetPointA( mtd.featureA.pts[ 3 ] );
					}
					else if( (mtd.featureB.type == ycCollision::kFeature_FaceTri || mtd.featureB.type == ycCollision::kFeature_FaceQuad )
						&& FaceCircumscribesPoint( mtd.featureA, mtd.dir, mtd.featureB.pts[ 0 ] ) )
					{
						mtd.SetPointB( mtd.featureB.pts[ 0 ] );
					}
					else if( (mtd.featureB.type == ycCollision::kFeature_FaceTri || mtd.featureB.type == ycCollision::kFeature_FaceQuad )
						&& FaceCircumscribesPoint( mtd.featureA, mtd.dir, mtd.featureB.pts[ 1 ] ) )
					{
						mtd.SetPointB( mtd.featureB.pts[ 1 ] );
					}
					else if( (mtd.featureB.type == ycCollision::kFeature_FaceTri || mtd.featureB.type == ycCollision::kFeature_FaceQuad )
						&& FaceCircumscribesPoint( mtd.featureA, mtd.dir, mtd.featureB.pts[ 2 ] ) )
					{
						mtd.SetPointB( mtd.featureB.pts[ 2 ] );
					}
					else if( mtd.featureB.type == ycCollision::kFeature_FaceQuad 
						&& FaceCircumscribesPoint( mtd.featureA, mtd.dir, mtd.featureB.pts[ 3 ] ) )
					{
						mtd.SetPointB( mtd.featureB.pts[ 3 ] );
					}
					else if( mtd.featureA.type == ycCollision::kFeature_FaceDisc &&
						     mtd.featureB.type == ycCollision::kFeature_FaceDisc )
					{
						f32 lerp = mtd.featureA.radius / ( mtd.featureA.radius + mtd.featureB.radius );
						ycVec3 pt = ycLerp( mtd.featureA.pts[ 0 ], mtd.featureB.pts[ 1 ], lerp );
						pt -= mtd.featureB.pts[ 1 ];
						pt = pt.Without( mtd.dir );
						pt += mtd.featureB.pts[ 1 ];
						mtd.SetPointB( pt );
					}
					else if( ( mtd.featureA.type == ycCollision::kFeature_FaceTri || mtd.featureA.type == ycCollision::kFeature_FaceQuad ) &&
						     ( mtd.featureB.type == ycCollision::kFeature_FaceTri || mtd.featureB.type == ycCollision::kFeature_FaceQuad ) )
					{
						mtd.SetPointA( mtd.supportingA );

						s32 numSidesA = 3;
						s32 numSidesB = 3;
						if( mtd.featureA.type == ycCollision::kFeature_FaceQuad ) { numSidesA++; }
						if( mtd.featureB.type == ycCollision::kFeature_FaceQuad ) { numSidesB++; }

						for( int iSideA = 0; iSideA < numSidesA; ++iSideA )
						{
							ycVec3 sideApt0 = mtd.featureA.pts[ iSideA ];
							ycVec3 sideApt1 = mtd.featureA.pts[ (iSideA + 1) % numSidesA ];
							ycLineSeg sideA( sideApt0, sideApt1 );

							for( int iSideB = 0; iSideB < numSidesB; ++iSideB )
							{
								ycVec3 sideBpt0 = mtd.featureB.pts[ iSideB ];
								ycVec3 sideBpt1 = mtd.featureB.pts[ (iSideB + 1) % numSidesB ];
								ycLineSeg sideB( sideBpt0, sideBpt1 );

								ycVec3 pointOnLineSegA, pointOnLineSegB; f32 distA, distB;
								ycGeo::ClosestPointsLineSegLineSeg( sideA, sideB, &pointOnLineSegA, &pointOnLineSegB, &distA, &distB );

								if( distA > 0.0 && distA < 1.0 && distB > 0.0 && distB < 1.0 )
								{
									mtd.SetPointA( pointOnLineSegA );
									break;
								}
							}
						}
					}
					else if( mtd.featureA.type == ycCollision::kFeature_FaceDisc &&
						     ( mtd.featureB.type == ycCollision::kFeature_FaceTri || mtd.featureB.type == ycCollision::kFeature_FaceQuad ) )
					{
						mtd.SetPointB( mtd.supportingB );
						f32 distSq = mtd.supportingB.DistSq( mtd.featureA.pts[ 0 ] );

						s32 numSidesB = 3;
						if( mtd.featureB.type == ycCollision::kFeature_FaceQuad ) { numSidesB++; }

						for( int iSideB = 0; iSideB < numSidesB; ++iSideB )
						{
							ycVec3 sideBpt0 = mtd.featureB.pts[ iSideB ];
							ycVec3 sideBpt1 = mtd.featureB.pts[ ( iSideB + 1 ) % numSidesB ];
							ycLineSeg sideB( sideBpt0, sideBpt1 );

							f32 distB;

							ycGeo::ClosestPointLineSeg( sideB, mtd.featureA.pts[ 0 ], &distB );
							if( distB > 0.0f && distB < 1.0f )
							{
								ycVec3 newPt = sideB.GetPoint( distB );
								f32 newDistSq = newPt.DistSq( mtd.featureA.pts[ 0 ] );

								if( newDistSq < distSq )
								{
									mtd.SetPointB( newPt );
									distSq = newDistSq;
								}
							}
						}
					}
					else if( mtd.featureB.type == ycCollision::kFeature_FaceDisc &&
						     ( mtd.featureA.type == ycCollision::kFeature_FaceTri || mtd.featureA.type == ycCollision::kFeature_FaceQuad ) )
					{
						mtd.SetPointA( mtd.supportingA );
						f32 distSq = mtd.supportingA.DistSq( mtd.featureB.pts[ 0 ] );

						s32 numSidesA = 3;
						if( mtd.featureA.type == ycCollision::kFeature_FaceQuad ) { numSidesA++; }

						for( int iSideA = 0; iSideA < numSidesA; ++iSideA )
						{
							ycVec3 sideApt0 = mtd.featureA.pts[ iSideA ];
							ycVec3 sideApt1 = mtd.featureA.pts[ ( iSideA + 1 ) % numSidesA ];
							ycLineSeg sideA( sideApt0, sideApt1 );

							f32 distA;

							ycGeo::ClosestPointLineSeg( sideA, mtd.featureB.pts[ 0 ], &distA );
							if( distA > 0.0f && distA < 1.0f )
							{
								ycVec3 newPt = sideA.GetPoint( distA );
								f32 newDistSq = newPt.DistSq( mtd.featureB.pts[ 0 ] );

								if( newDistSq < distSq )
								{
									mtd.SetPointA( newPt );
									distSq = newDistSq;
								}
							}
						}
					}
					else
					{
						mtd.SetPointA( mtd.supportingA );
					}
				}
				break;
			default: ycAssert( 0 );
		};
		break;
	default: ycAssert( 0 );
	};
}

