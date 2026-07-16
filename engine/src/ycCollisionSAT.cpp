#include "ycCollisionSAT.h"

#include "ycTri.h"
#include "ycSphere.h"
#include "ycCollisionWelding.h"
#include "ycCollisionFeatureUtil.h"
#include "ycCollisionMtx3.h"

#ifndef YC_CHARLIE
#include "ycPushDebugOpt.inc"
#endif

namespace
{
	void ReverseResult( ycCollisionResult& rr )
	{
		rr.dir = -rr.dir; 
		rr.SetPointA( rr.pointB ); 
		ycSwap( rr.supportingA, rr.supportingB ); 
		ycSwap( rr.featureA, rr.featureB ); 
		rr.faceB.valid = false;
	}

	constexpr bool OrthonormalScaleMtx( const ycCollisionSAT::IdentityProxy& )
	{
		return true;
	}
	
	bool OrthonormalScaleMtx( const ycMtx4& xform )
	{
		f32 lenX = xform.x.xyz().Length();
		f32 lenY = xform.y.xyz().Length();
		f32 lenZ = xform.x.xyz().Length();

		if( !ycIsZero( lenX - lenY, 0.0001f ) ) { return false; }
		if( !ycIsZero( lenX - lenZ, 0.0001f ) ) { return false; }
		if( !ycIsZero( lenY - lenZ, 0.0001f ) ) { return false; }

		if( !ycIsZero( xform.x.xyz().Dot( xform.y.xyz() ), 0.0001f ) ) { return false; }
		if( !ycIsZero( xform.x.xyz().Dot( xform.z.xyz() ), 0.0001f ) ) { return false; }
		if( !ycIsZero( xform.y.xyz().Dot( xform.z.xyz() ), 0.0001f ) ) { return false; }

		return true;
	}

	template <typename Shape>
	bool OrthonormalScale( const Shape& sA )
	{
		typedef ycCollisionSAT::ShapeTraits<Shape> Traits;
		return OrthonormalScaleMtx( Traits::GetForwardXform( sA ) );
	}

	// 3x3 Jacobi eigendecomposition, requires that A is a square symmetric matrix.
	// The eigenvalues of the 3x3 upper corner of A are returned in the vector D
	// The eigenvectors of A are returned in the 3x3 upper corner of V

	// Multiplying (v.Transposed() * (d * (v * myVec))) is equivalent to the original matrix A
	void eigendecompose( ycCollisionMtx3 a, ycVec3& d, ycCollisionMtx3& v )
	{
		int j, iq, ip, i;
		float tresh, theta, tau, t, sm, s, h, g, c;
		ycVec3 b, z = ycVec3::ZERO();
		v = ycCollisionMtx3::IDENTITY();

		auto Rotate = [&tau, &s, &h, &g]( ycCollisionMtx3& a, int i, int j, int k, int l )
		{
			g = a.m[ i ][ j ];
			h = a.m[ k ][ l ];
			a.m[ i ][ j ] = g - s * ( h + g * tau );
			a.m[ k ][ l ] = h + s * ( g - h * tau );
		};

		for( ip = 0; ip < 3; ip++ ) 
		{
			//Initialize b and d to the diagonal of a.
			b.a[ ip ] = d.a[ ip ] = a.m[ ip ][ ip ];
		}

		for( i = 1; i <= 50; i++ ) 
		{
			sm = 0.0;

			for( ip = 0; ip < 3; ip++ ) 
			{
				for( iq = ip + 1; iq < 3; iq++ )
				{
					sm += ycAbs( a.m[ ip ][ iq ] );
				}
			}
			if( sm < 0.00001f )
			{
				return;
			}

			if( i < 4 )
			{
				tresh = 0.2f * sm / ( 9.0f ); 
			}
			else
			{
				tresh = 0.0f; 
			}

			for( ip = 0; ip < 3; ip++ ) 
			{
				for( iq = ip + 1; iq < 3; iq++ ) 
				{
					g = 100.0f * ycAbs( a.m[ ip ][ iq ] );
					if( i > 4 && ( ycAbs( d.a[ ip ] ) + g ) == ycAbs( d.a[ ip ] ) && ( ycAbs( d.a[ iq ] ) + g ) == ycAbs( d.a[ iq ] ) )
					{
						a.m[ ip ][ iq ] = 0.0f;
					}
					else if( ycAbs( a.m[ ip ][ iq ] ) > tresh ) 
					{
						h = d.a[ iq ] - d.a[ ip ];
						if( (float)( ycAbs( h ) + g ) == (float)ycAbs( h ) )
						{
							t = ( a.m[ ip ][ iq ] ) / h;
						}
						else 
						{
							theta = 0.5f * h / ( a.m[ ip ][ iq ] ); 
							t = 1.0f / ( ycAbs( theta ) + ycSqrt( 1.0f + theta * theta ) );
							if( theta < 0.0 ) { t = -t; }
						}

						c = 1.0f / ycSqrt( 1 + t * t );
						s = t * c;
						tau = s / ( 1.0f + c );
						h = t * a.m[ ip ][ iq ];
						z.a[ ip ] -= h;
						z.a[ iq ] += h;
						d.a[ ip ] -= h;
						d.a[ iq ] += h;
						a.m[ ip ][ iq ] = 0.0f;
						for( j = 0; j <= ip - 1; j++ ) 
						{
							Rotate( a, j, ip, j, iq );
						}
						for( j = ip + 1; j <= iq - 1; j++ ) 
						{
							Rotate( a, ip, j, j, iq );
						}
						for( j = iq + 1; j < 3; j++ ) 
						{
							Rotate( a, ip, j, iq, j );
						}
						for( j = 0; j < 3; j++ ) 
						{
							Rotate( v, j, ip, j, iq );
						}
					}
				}
			}
			for( ip = 0; ip < 3; ip++ ) 
			{
				b.a[ ip ] += z.a[ ip ];
				d.a[ ip ] = b.a[ ip ];
				z.a[ ip ] = 0.0;
			}
		}
	}

	// Solves by the method of Goldfeld, Quandt, and Trotter [1966] "Maximization by quadratic hill climbing"
	// as given in More & Sorensen [1981] "Computing a Trust Region Step"
	//
	// Minimizes a quadratic program F(p) = B^T * g^T * p + 0.5 * p^T * B^T * B * p given ||project * p|| <= radius
	// project should be identity or a rank-deficient matrix projecting vectors onto a relevant subspace
	ycVec3 GQT_solve( const ycCollisionMtx3& B, const ycCollisionMtx3& project, const ycVec3& g, const f32 radius, const u32 numDims = 3 )
	{
		ycAssert( radius != 0 );

		ycVec3 p;

		auto dmul = []( const ycVec3& p, ycVec3& x )
		{
			for( s32 i = 0; i < 3; i++ )
			{
				x.a[ i ] = ( ycAbs( p.a[ i ] ) < 0.0001f ? 0.0f : x.a[ i ] / p.a[ i ] );
			}
		};

		auto dmulsq = []( const ycVec3& p, ycVec3& x )
		{
			for( s32 i = 0; i < 3; i++ )
			{
				x.a[ i ] = ( ycAbs( p.a[ i ] ) < 0.0001f ? 0.0f : x.a[ i ] / ycSqrt( p.a[ i ] ) );
			}
		};

		auto clearLowest = []( ycVec3& vals )
		{
			const f32 absX = ycAbs( vals.x );
			const f32 absY = ycAbs( vals.y );
			const f32 absZ = ycAbs( vals.z );

			const f32 absMax = ycMax( absX, ycMax( absY, absZ ) );

			if( absX <= absY && absX <= absZ )
			{
				ycAssert( absX < absMax * 0.0001f );
				vals.x = 0.0f;
			}
			else if( absY <= absX && absY <= absZ )
			{
				ycAssert( absY < absMax * 0.0001f );
				vals.y = 0.0f;
			}
			else if( absZ <= absX && absZ <= absY )
			{
				ycAssert( absZ < absMax * 0.0001f );
				vals.z = 0.0f;
			}
		};

		const ycVec3 minG = - B.Transposed().Transform( g );
		f32 lambda = 0.0f;

		ycCollisionMtx3 BTB( B );
		BTB.MulWithTranspose();

		f32 offDiag = ycAbs( BTB.m[ 0 ][ 1 ] ) + ycAbs( BTB.m[ 0 ][ 2 ] ) + ycAbs( BTB.m[ 1 ][ 2 ] );
		f32 offDiag_n = ycAbs( project.m[ 0 ][ 1 ] ) + ycAbs( project.m[ 0 ][ 2 ] ) + ycAbs( project.m[ 1 ][ 2 ] );

		f32 bestDiff = YC_F32_MAX;
		ycVec3 bestP = ycVec3::YAXIS();
		f32 omega = 1.0f;

		if( offDiag < 0.0001f && offDiag_n < 0.0001f )
		{
			// diagonal matrix case, the eigendecomposition is trivial and can be represented as a vector
			f32 minBound = 0.0f;

			ycVec3 btb( BTB.m[ 0 ][ 0 ], BTB.m[ 1 ][ 1 ], BTB.m[ 2 ][ 2 ] );
			ycVec3 proj( project.m[ 0 ][ 0 ], project.m[ 1 ][ 1 ], project.m[ 2 ][ 2 ] );

			for( s32 iter = 0; iter < 20; ++iter )
			{
				// Factor B + lambda * I = R^T * R 
				ycVec3 eigenvals = btb + proj * lambda;
				if( numDims < 3 ) { clearLowest( eigenvals ); }

				// Solve R^T * R * p = -g
				p = minG;
				dmul( eigenvals, p );

				f32 pMag = ( proj * p ).Length();
				ycAssert( pMag >= 0 );
				if( lambda == 0.0f && pMag < radius )
				{
					// solution found inside the radius
					return p;
				}
				if( ycAbs( pMag - radius ) < 0.00001f )
				{
					// solution found
					return p;
				}
				if( ycAbs( pMag - radius ) < bestDiff )
				{
					bestDiff = ycAbs( pMag - radius );
					bestP = p;
				}

				if( lambda == 0.0f )
				{
					f32 minEigen = YC_F32_MAX;
					if( eigenvals.x != 0.0f ) { minEigen = ycMin( minEigen, eigenvals.x ); }
					if( eigenvals.y != 0.0f ) { minEigen = ycMin( minEigen, eigenvals.y ); }
					if( eigenvals.z != 0.0f ) { minEigen = ycMin( minEigen, eigenvals.z ); }

					minBound = -ycAbs( minEigen );
					lambda = ycAbs( minEigen );
				}
				else
				{
					// Solve R^T * q = p
					ycVec3 q = p;
					dmulsq( eigenvals, q );

					f32 qMag = ( proj * q ).Length();
					ycAssert( qMag > 0 );

					f32 newLambda = lambda + omega * ( pMag / qMag ) * ( pMag / qMag ) * ( pMag - radius ) / radius;

					s32 iterCount = 100; // added due to (newLambda <= minBound) precision issues
					while( (newLambda <= minBound) && iterCount > 0 )
					{
						newLambda = ycLerp( newLambda, lambda, 0.5f );
						omega *= 0.8f;
						iterCount--;
					}

					lambda = newLambda;
				}
			}
		}
		else
		{
			f32 minBound = 0.0f;

			for( int iter = 0; iter < 20; ++iter )
			{
				// Factor B + lambda * I = R^T * R 
				ycCollisionMtx3  blI = BTB + project * lambda;
				ycCollisionMtx3  eigenvecs; ycVec3 eigenvals;
				eigendecompose( blI, eigenvals, eigenvecs );
				if( numDims < 3 ) { clearLowest( eigenvals ); }

				// Solve R^T * R * p = -g
				p = eigenvecs * minG;
				dmul( eigenvals, p );
				p = eigenvecs.Transposed() * p;
				p = project.Transform( p );

				f32 pMag = p.Length();
				ycAssert( pMag >= 0 );
				if( lambda == 0.0f && pMag < radius )
				{
					// solution found inside the radius
					return p;
				}
				if( ycAbs( pMag - radius ) < 0.00001f )
				{
					// solution found
					return p;
				}
				if( ycAbs( pMag - radius ) < bestDiff )
				{
					bestDiff = ycAbs( pMag - radius );
					bestP = p;
				}

				if( lambda == 0.0f )
				{
					f32 minEigen = YC_F32_MAX;
					if( eigenvals.x != 0.0f ) { minEigen = ycMin( minEigen, eigenvals.x ); }
					if( eigenvals.y != 0.0f ) { minEigen = ycMin( minEigen, eigenvals.y ); }
					if( eigenvals.z != 0.0f ) { minEigen = ycMin( minEigen, eigenvals.z ); }

					minBound = -ycAbs( minEigen );
					lambda = ycAbs( minEigen );
				}
				else
				{
					// Solve R^T * q = p
					ycVec3 q = p;
					dmulsq( eigenvals, q );
					q = eigenvecs.Transposed() * q;
					f32 qMag = project.Transform( q ).Length();

					ycAssert( qMag >= 0 );
					f32 newLambda = lambda + omega * ( pMag / qMag ) * ( pMag / qMag ) * ( pMag - radius ) / radius;

					while( newLambda <= minBound )
					{
						newLambda = ycLerp( newLambda, lambda, 0.5f );
						omega *= 0.8f;
					}
					ycAssert( newLambda > minBound );

					lambda = newLambda;
				}
			}
		}


		return bestP;
	}

	// generic AxisProjection
	template< typename Shape >
	void AxisProjectionGeneric( const Shape& shape, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
	{
		typedef ycCollisionSAT::ShapeTraits<Shape> Traits; 
		const auto xform = Traits::GetForwardXform( shape );

		// the image of a transformed shape on an axis is the image of the untransformed shape
		// on the axis multiplied by the transpose of the forward
		ycVec3 projAxis = xform.Transposed().TransformDir( axis ).Normalized();

		ycVec3 minVert = Traits::GetVert( shape, 0 );
		ycVec3 maxVert = minVert;

		minDot = maxDot = projAxis.Dot( minVert );

		for( u32 ii = 1; ii < Traits::kVertCount; ++ii )
		{
			ycVec3 testVert = Traits::GetVert( shape, ii );
			f32 testDot = projAxis.Dot( testVert );
			if( testDot < minDot )
			{
				minDot = testDot;
				minVert = testVert;
			}
			if( testDot > maxDot )
			{
				maxDot = testDot;
				maxVert = testVert;
			}
		}

		minPoint = xform.TransformPos( minVert - projAxis * Traits::GetRadius( shape ) );
		maxPoint = xform.TransformPos( maxVert + projAxis * Traits::GetRadius( shape ) );

		minDot = axis.Dot( minPoint );
		maxDot = axis.Dot( maxPoint );
	}

	template <typename ShapeA>
	constexpr bool IsBackface( const ShapeA&, const ycVec3& )
	{
		return false;
	}

	bool IsBackface( const ycTri& tri, const ycVec3& normal )
	{
		return tri.GetAB().Cross( tri.GetAC() ).Dot( normal ) < 0.0f;
	}
	
	// returns true if this axis is better than the one stored in MTD
	template <typename ShapeA, typename ShapeB>
	bool CheckAxis( const ycVec3& axis, const ShapeA& sA, const ShapeB& sB, ycCollisionResult& mtd, bool allowInversion, f32 inversionRadius, const char*  )
	{
		ycAssert( !ycIsNan( axis ) );
		ycAssert( !ycIsInf( axis ) );
		if( axis.LengthSq() < (0.0001f * 0.0001f) ) { return false; }

		f32 minDotA, minDotB, maxDotA, maxDotB;
		ycVec3 minPtA, minPtB, maxPtA, maxPtB;

		ycVec3 axisNorm = axis.Normalized();

		ycAssert( !ycIsNan( axisNorm ) );
		ycAssert( !ycIsInf( axisNorm ) );
		ycCollisionSAT::AxisProjection( sA, axisNorm, minDotA, minPtA, maxDotA, maxPtA );
		ycCollisionSAT::AxisProjection( sB, axisNorm, minDotB, minPtB, maxDotB, maxPtB );

		// initial overlap - push shape A in the axis direction
		f32 overlap = maxDotB - minDotA;
		ycVec3 supportingA = minPtA;
		ycVec3 supportingB = maxPtB;
		f32 axisSign = 1.0f;

		// try opposite direction - if allowed
		if( ( allowInversion || ( maxDotA - minDotB ) < -inversionRadius )
			&& ( maxDotA - minDotB ) < overlap )
		{
			overlap = maxDotA - minDotB;
			axisSign = -1.0f;
			supportingA = maxPtA;
			supportingB = minPtB;
		}

		if( !mtd.valid || overlap < mtd.mag || (mtd.matchAxisOkay && mtd.mag == overlap) )
		{
			mtd.valid = true;
			mtd.mag = overlap;
			mtd.dir = axisNorm * axisSign;  // always points from shape B towards shape A
			mtd.supportingA = supportingA;
			mtd.supportingB = supportingB;
			mtd.isBackface = false;         // this will be filled in by caller

			return true;
		}

		return false;
	};

	ycCollisionFeature GetSupportingFeature( const ycVec3& vv, const ycVec3&, const ycVec3& )
	{
		return ycCollisionFeature::Vertex( vv );
	}

	ycCollisionFeature GetSupportingFeature( const ycSphere&, const ycVec3&, const ycVec3& supportingPt )
	{
		return ycCollisionFeature::Vertex( supportingPt );
	}

	ycCollisionFeature GetSupportingFeature( const ycCollisionSphere&, const ycVec3&, const ycVec3& supportingPt )
	{
		return ycCollisionFeature::Vertex( supportingPt );
	}

	ycCollisionFeature GetSupportingFeature( const ycCollisionCapsule& cap, const ycVec3& normal, const ycVec3& supportingPt )
	{
		const ycVec3 p0 = ( cap.pts[ 0 ] );
		const ycVec3 p1 = ( cap.pts[ 1 ] );
		const ycVec3 edge = p1 - p0;

		const ycVec3 nNorm = cap.forward.Transposed().TransformDir( normal ).Normalized();

		if( ycAbs( nNorm.Dot( edge ) ) < 0.001f * edge.Length() )
		{
			return ycCollisionFeature::Edge( cap.forward.TransformPos( p0 ), cap.forward.TransformPos( p1 ), cap.radius );
		}

		return ycCollisionFeature::Vertex( supportingPt );
	}

	ycCollisionFeature GetSupportingFeature( const ycTri& tri, const ycVec3& normal, const ycVec3& supportingPt )
	{
		typedef ycCollisionSAT::ShapeTraits<ycTri> Traits;

		ycVec3 triNorm = tri.GetNormal();
		ycVec3 nNorm = normal.Normalized();

		if( ycAbs( nNorm.Dot( triNorm ) ) > 0.99999f )
		{
			return ycCollisionFeature::FaceTri( tri.a, tri.b, tri.c );
		}

		if( nNorm.Dot( tri.c - tri.a ) < 0.0f && nNorm.Dot( tri.c - tri.b ) < 0.0f &&
			ycAbs( nNorm.Dot(tri.GetAB()) ) < 0.001f * tri.GetAB().Length() )
		{
			return Traits::GetEdge( tri, 0 );
		}
		if( nNorm.Dot( tri.a - tri.b ) < 0.0f && nNorm.Dot( tri.a - tri.c ) < 0.0f &&
			ycAbs( nNorm.Dot( tri.GetBC() ) ) < 0.001f * tri.GetBC().Length() )
		{
			return Traits::GetEdge( tri, 1 );
		}
		if( nNorm.Dot( tri.b - tri.a ) < 0.0f && nNorm.Dot( tri.b - tri.c ) < 0.0f &&
			ycAbs( nNorm.Dot( tri.GetAC() ) ) < 0.001f * tri.GetAC().Length() )
		{
			return Traits::GetEdge( tri, 2 );
		}

		return ycCollisionFeature::Vertex( supportingPt );
	}

	ycCollisionFeature GetBoxSupportingFeature( const ycMtx4& forward, const ycMtx4& inverse, const ycVec3& normal, const ycVec3& supportingPt )
	{
		typedef ycCollisionSAT::ShapeTraits<ycMtx4> Traits;

		auto GetFace = []( const ycMtx4& tt, u32 ii )
		{
			ycAssert( ii < 6 );
			const ycVec3 faces[ 6 ][ 4 ] = {
				{ ycVec3( -1,-1,-1 ), ycVec3( -1, 1,-1 ), ycVec3( -1, 1, 1 ), ycVec3( -1,-1, 1 ) },
				{ ycVec3( 1,-1,-1 ), ycVec3( 1, 1,-1 ), ycVec3( 1, 1, 1 ), ycVec3( 1,-1, 1 ) },

				{ ycVec3( -1,-1,-1 ), ycVec3( 1,-1,-1 ), ycVec3( 1,-1, 1 ), ycVec3( -1,-1, 1 ) },
				{ ycVec3( -1, 1,-1 ), ycVec3( 1, 1,-1 ), ycVec3( 1, 1, 1 ), ycVec3( -1, 1, 1 ) },

				{ ycVec3( -1,-1,-1 ), ycVec3( 1,-1,-1 ), ycVec3( 1, 1,-1 ), ycVec3( -1, 1,-1 ) },
				{ ycVec3( -1,-1, 1 ), ycVec3( 1,-1, 1 ), ycVec3( 1, 1, 1 ), ycVec3( -1, 1, 1 ) },
			};

			ycVec3 v0 = tt.TransformPos( faces[ ii ][ 0 ] );
			ycVec3 v1 = tt.TransformPos( faces[ ii ][ 1 ] );
			ycVec3 v2 = tt.TransformPos( faces[ ii ][ 2 ] );
			ycVec3 v3 = tt.TransformPos( faces[ ii ][ 3 ] );

			return ycCollisionFeature::FaceQuad( v0, v1, v2, v3 );
		};

		ycVec3 xfmXAxis = inverse.Transposed().TransformDir( ycVec3::XAXIS() ).Normalized();
		ycVec3 xfmYAxis = inverse.Transposed().TransformDir( ycVec3::YAXIS() ).Normalized();
		ycVec3 xfmZAxis = inverse.Transposed().TransformDir( ycVec3::ZAXIS() ).Normalized();
		ycVec3 nNorm = normal.Normalized();

		if( nNorm.Dot( xfmXAxis ) < -0.9999f )
		{
			return GetFace( forward, 0 );
		}
		if( nNorm.Dot( xfmXAxis ) > 0.9999f )
		{
			return GetFace( forward, 1 );
		}
		if( nNorm.Dot( xfmYAxis ) < -0.9999f )
		{
			return GetFace( forward, 2 );
		}
		if( nNorm.Dot( xfmYAxis ) > 0.9999f )
		{
			return GetFace( forward, 3 );
		}
		if( nNorm.Dot( xfmZAxis ) < -0.9999f )
		{
			return GetFace( forward, 4 );
		}
		if( nNorm.Dot( xfmZAxis ) > 0.9999f )
		{
			return GetFace( forward, 5 );
		}

		ycVec3 xEdge = forward.TransformDir( ycVec3::XAXIS() ).Normalized();
		ycVec3 yEdge = forward.TransformDir( ycVec3::YAXIS() ).Normalized();
		ycVec3 zEdge = forward.TransformDir( ycVec3::ZAXIS() ).Normalized();

		ycVec3 locNorm = forward.Transposed().TransformDir( normal ).Normalized();
		if( ycAbs( nNorm.Dot( xEdge ) ) < 0.0001f )
		{
			u32 bit1 = ( locNorm.y < 0.0f ) ? 0 : 2;
			u32 bit0 = ( locNorm.z < 0.0f ) ? 0 : 1;

			return Traits::GetEdge( forward, bit1 + bit0 );
		}
		if( ycAbs( nNorm.Dot( yEdge ) ) < 0.0001f )
		{
			u32 bit1 = ( locNorm.x < 0.0f ) ? 0 : 2;
			u32 bit0 = ( locNorm.z < 0.0f ) ? 0 : 1;

			return Traits::GetEdge( forward, 4 + bit1 + bit0 );
		}
		if( ycAbs( nNorm.Dot( zEdge ) ) < 0.0001f )
		{
			u32 bit1 = ( locNorm.x < 0.0f ) ? 0 : 2;
			u32 bit0 = ( locNorm.y < 0.0f ) ? 0 : 1;

			return Traits::GetEdge( forward, 8 + bit1 + bit0 );
		}

		return ycCollisionFeature::Vertex( supportingPt );
	}


	ycCollisionFeature GetSupportingFeature( const ycAABB& aabb, const ycVec3& normal, const ycVec3& supportingPt )
	{
		typedef ycCollisionSAT::ShapeTraits<ycAABB> Traits;

		auto GetFace = []( const ycAABB& tt, u32 ii )
		{
			ycAssert( ii < 6 );
			const ycVec3 faces[ 6 ][ 4 ] = {
				{ ycVec3( -1,-1,-1 ), ycVec3( -1, 1,-1 ), ycVec3( -1, 1, 1 ), ycVec3( -1,-1, 1 ) },
				{ ycVec3( 1,-1,-1 ), ycVec3( 1, 1,-1 ), ycVec3( 1, 1, 1 ), ycVec3( 1,-1, 1 ) },

				{ ycVec3( -1,-1,-1 ), ycVec3( 1,-1,-1 ), ycVec3( 1,-1, 1 ), ycVec3( -1,-1, 1 ) },
				{ ycVec3( -1, 1,-1 ), ycVec3( 1, 1,-1 ), ycVec3( 1, 1, 1 ), ycVec3( -1, 1, 1 ) },

				{ ycVec3( -1,-1,-1 ), ycVec3( 1,-1,-1 ), ycVec3( 1, 1,-1 ), ycVec3( -1, 1,-1 ) },
				{ ycVec3( -1,-1, 1 ), ycVec3( 1,-1, 1 ), ycVec3( 1, 1, 1 ), ycVec3( -1, 1, 1 ) },
			};

			ycVec3 v0 = tt.center + tt.extents * ( faces[ ii ][ 0 ] );
			ycVec3 v1 = tt.center + tt.extents * ( faces[ ii ][ 1 ] );
			ycVec3 v2 = tt.center + tt.extents * ( faces[ ii ][ 2 ] );
			ycVec3 v3 = tt.center + tt.extents * ( faces[ ii ][ 3 ] );

			return ycCollisionFeature::FaceQuad( v0, v1, v2, v3 );
		};

		ycVec3 locNorm = ( normal ).Normalized();
		if( locNorm.Dot( ycVec3::XAXIS() ) < -0.99f )
		{
			return GetFace( aabb, 0 );
		}
		if( locNorm.Dot( ycVec3::XAXIS() ) > 0.99f )
		{
			return GetFace( aabb, 1 );
		}
		if( locNorm.Dot( ycVec3::YAXIS() ) < -0.99f )
		{
			return GetFace( aabb, 2 );
		}
		if( locNorm.Dot( ycVec3::YAXIS() ) > 0.99f )
		{
			return GetFace( aabb, 3 );
		}
		if( locNorm.Dot( ycVec3::ZAXIS() ) < -0.99f )
		{
			return GetFace( aabb, 4 );
		}
		if( locNorm.Dot( ycVec3::ZAXIS() ) > 0.99f )
		{
			return GetFace( aabb, 5 );
		}

		if( ycAbs( locNorm.x ) < 0.001f )
		{
			u32 bit1 = ( locNorm.y < 0.0f ) ? 0 : 2;
			u32 bit0 = ( locNorm.z < 0.0f ) ? 0 : 1;

			return Traits::GetEdge( aabb, bit1 + bit0 );
		}
		if( ycAbs( locNorm.y ) < 0.001f )
		{
			u32 bit1 = ( locNorm.x < 0.0f ) ? 0 : 2;
			u32 bit0 = ( locNorm.z < 0.0f ) ? 0 : 1;

			return Traits::GetEdge( aabb, 4 + bit1 + bit0 );
		}
		if( ycAbs( locNorm.z ) < 0.001f )
		{
			u32 bit1 = ( locNorm.x < 0.0f ) ? 0 : 2;
			u32 bit0 = ( locNorm.y < 0.0f ) ? 0 : 1;

			return Traits::GetEdge( aabb, 8 + bit1 + bit0 );
		}

		return ycCollisionFeature::Vertex( supportingPt );
	}

	ycCollisionFeature GetSupportingFeature( const ycMtx4& box, const ycVec3& normal, const ycVec3& supportingPt )
	{
		return GetBoxSupportingFeature( box, box.Inverted(), normal, supportingPt );
	}

	ycCollisionFeature GetSupportingFeature( const ycCollisionBox& box, const ycVec3& normal, const ycVec3& supportingPt )
	{
		return GetBoxSupportingFeature( box.forward, box.inverse, normal, supportingPt );
	}

	ycCollisionFeature GetSupportingFeature( const ycCollisionCylinder& cyl, const ycVec3& normal, const ycVec3& supportingPt )
	{
		ycVec3 nNorm = normal.Normalized();

		if( nNorm.Dot( cyl.upDir ) > 0.99999f )
		{
			return ycCollisionFeature::FaceDisc( cyl.center + cyl.upDir * ( cyl.height * 0.5f ), cyl.radius );
		}
		if( nNorm.Dot( cyl.upDir ) < -0.99999f )
		{
			return ycCollisionFeature::FaceDisc( cyl.center - cyl.upDir * ( cyl.height * 0.5f ), cyl.radius );
		}

		if( ycAbs( nNorm.Dot( cyl.upDir ) ) < 0.0001f )
		{
			ycVec3 edge0 = cyl.center + cyl.upDir * ( cyl.height * 0.5f );
			ycVec3 edge1 = cyl.center - cyl.upDir * ( cyl.height * 0.5f );
			ycVec3 offset = ( supportingPt - cyl.center ).Without( cyl.upDir );
			edge0 += offset;
			edge1 += offset;

			return ycCollisionFeature::Edge( edge0, edge1 );
		}

		return ycCollisionFeature::Vertex( supportingPt );
	}
	
	template <typename ShapeA, typename ShapeB>
	ycVec3 AxisForVertexEdge( const ycVec3& centerAInA, const ycVec3& edge0InB, const ycVec3& edge1InB, const ShapeA& sA, const ShapeB& sB )
	{
		typedef ycCollisionSAT::ShapeTraits<ShapeA> TraitsA;
		typedef ycCollisionSAT::ShapeTraits<ShapeB> TraitsB;

		const auto xformAtoB = TraitsB::GetInverseXform( sB ) * TraitsA::GetForwardXform( sA );

		const ycVec3 centerAInB = xformAtoB.TransformPos( centerAInA );

		if( TraitsB::GetRadius( sB ) == 0.0f || ( OrthonormalScale( sA ) && OrthonormalScale( sB ) ) )
		{
			const ycVec3 edgeInB = ( edge1InB - edge0InB );

			const ycVec3 deltaInB = ( edge0InB - centerAInB ).Without( edgeInB );
			return deltaInB;
		}

		// else

		const auto xformBtoA = TraitsA::GetInverseXform( sA ) * TraitsB::GetForwardXform( sB );

		const ycVec3 edgeInB = ( edge1InB - edge0InB ).Normalized();
		ycCollisionMtx3 projMtx;
		projMtx.m[ 0 ][ 0 ] = 1.0f - edgeInB.x * edgeInB.x;
		projMtx.m[ 1 ][ 1 ] = 1.0f - edgeInB.y * edgeInB.y;
		projMtx.m[ 2 ][ 2 ] = 1.0f - edgeInB.z * edgeInB.z;

		projMtx.m[ 0 ][ 1 ] = projMtx.m[ 1 ][ 0 ] = -edgeInB.x * edgeInB.y;
		projMtx.m[ 0 ][ 2 ] = projMtx.m[ 2 ][ 0 ] = -edgeInB.x * edgeInB.z;
		projMtx.m[ 1 ][ 2 ] = projMtx.m[ 2 ][ 1 ] = -edgeInB.y * edgeInB.z;

		ycCollisionMtx3 projToA = ycCollisionMtx3(xformBtoA) * projMtx;

		const ycVec3 deltaInA = projToA.Transform( edge0InB - centerAInB );
		if( deltaInA.Length() == 0.0f ) { return ycVec3::ZERO(); }

		ycVec3 axisInB = GQT_solve( projToA, projMtx, deltaInA, TraitsB::GetRadius( sB ), 2 );
		return axisInB;
	}

	template <typename ShapeA, typename ShapeB>
	ycVec3 AxisForVertexVertex( const ycVec3& centerAInA, const ycVec3& centerBInB, const ShapeA& sA, const ShapeB& sB )
	{
		typedef ycCollisionSAT::ShapeTraits<ShapeA> TraitsA;
		typedef ycCollisionSAT::ShapeTraits<ShapeB> TraitsB;

		if( TraitsB::GetRadius( sB ) == 0.0f || ( OrthonormalScale( sA ) && OrthonormalScale( sB ) ) )
		{
			const auto xformAtoB = TraitsB::GetInverseXform( sB ) * TraitsA::GetForwardXform( sA );
			const ycVec3 centerAInB = xformAtoB.TransformPos( centerAInA );
			const ycVec3 deltaInB = centerBInB - centerAInB;
			return deltaInB;
		}

		// else
		const auto xformBtoA = TraitsA::GetInverseXform( sA ) * TraitsB::GetForwardXform( sB );
		const ycVec3 centerBInA = xformBtoA.TransformPos( centerBInB );
		const ycVec3 deltaInA = centerBInA - centerAInA;

		if( deltaInA.Length() == 0.0f ) { return ycVec3::ZERO(); }

		const ycVec3 axisInB = GQT_solve( ycCollisionMtx3( xformBtoA ), ycCollisionMtx3::IDENTITY(), deltaInA, TraitsB::GetRadius( sB ) );
		return axisInB;
	}

	template <typename ShapeA, typename ShapeB>
	bool CheckAxisSkipBackface( const ycVec3& axis, const ycVec3& backface, const ShapeA& sA, const ShapeB& sB, ycCollisionResult& mtd, bool allowInversion, f32 inversionRadius, const char* name )
	{
		typedef ycCollisionSAT::ShapeTraits<ShapeB> TraitsB;

		if( TraitsB::kAllowBackface )
		{
			ycAssert( !ycIsNan( axis ) );
			return CheckAxis( axis, sA, sB, mtd, allowInversion, inversionRadius, name );
		}
		else
		{
			// if we are rejecting backfaces here we must skip all axis tests that could result in duplicating the tri face axis
			f32 dotAxis = backface.Dot( axis );
			if( dotAxis * dotAxis < 0.99f * axis.LengthSq() )
			{
				ycAssert( !ycIsNan( axis ) );
				return CheckAxis( axis, sA, sB, mtd, allowInversion, inversionRadius, name );
			}
			else if( allowInversion )
			{
				ycAssert( !ycIsNan( axis ) );
				return CheckAxis( -axis * ycSign( dotAxis ), sA, sB, mtd, false, 0.0f, name);
			}
			return false;
		}
	};

	template <typename ShapeA, typename ShapeB>
	ycCollisionResult SepAxisTestSpecialize( const ShapeA& sA, const ShapeB& sB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior backfaceBehavior )
	{
		typedef ycCollisionSAT::ShapeTraits<ShapeA> TraitsA;
		typedef ycCollisionSAT::ShapeTraits<ShapeB> TraitsB;

		ycCollisionResult mtd;
		ycVec3 backface = ycVec3::ZERO();

		const auto xformAtoWorld = TraitsA::GetForwardXform( sA );
		const auto xformBtoWorld = TraitsB::GetForwardXform( sB );
		const auto xformBtoA = TraitsA::GetInverseXform( sA ) * TraitsB::GetForwardXform( sB );
		const auto axisAToWorld = TraitsA::GetInverseXform( sA ).Transposed();
		const auto axisBToWorld = TraitsB::GetInverseXform( sB ).Transposed();
		
		// FACE NORMALS
		// (incorporates face-v-face, face-v-edge, face-v-vertex)
		for( u32 jj = 0; jj < TraitsB::kFaceNormCount; ++jj )
		{
			ycVec3 faceNormB = axisBToWorld.TransformDir( TraitsB::GetFaceNorm( sB, jj ) );

			bool chkBackface = (backfaceBehavior == ycCollisionOptions::ycCollisionOptions::kBackface_ResolveAsDoubleSided) || TraitsB::kAllowBackface;
			ycAssert( !ycIsNan( faceNormB ) );
			if( CheckAxis( faceNormB, sA, sB, mtd, chkBackface, 0.00001f, "Gen-FaceB") )
			{
				mtd.pointB = mtd.supportingB;
				mtd.featureB = GetSupportingFeature( sB, mtd.dir, mtd.supportingB );
				if( chkBackface ) { mtd.isBackface = IsBackface( sB, mtd.dir ); }
				mtd.faceB = mtd;
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
			if( !chkBackface )
			{
				backface = -faceNormB;
				ycAssert( !ycIsNan( faceNormB ) );
				if( CheckAxis( faceNormB, sA, sB, mtd, false, 0.0f, "Gen-FaceB" ) )
				{
					mtd.isBackface = true;
				}
				if( mtd.valid && mtd.mag < -sepPrecision )
				{
					return mtd;
				}
			}
		}

		for( u32 ii = 0; ii < TraitsA::kFaceNormCount; ++ii )
		{
			ycVec3 faceNormA = axisAToWorld.TransformDir( TraitsA::GetFaceNorm( sA, ii ));

			ycAssert( !ycIsNan( faceNormA ) );
			CheckAxisSkipBackface( faceNormA, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-FaceA" );
			if( mtd.valid && mtd.mag < -sepPrecision )
			{
				return mtd;
			}
		}

		// EDGE v EDGE
		bool degenerateEvE = false;
		for( u32 ii = 0; ii < TraitsA::kEdgeDirCount; ++ii )
		{
			ycVec3 edgeA = xformAtoWorld.TransformDir( TraitsA::GetEdgeDir( sA, ii ) );

			for( u32 jj = 0; jj < TraitsB::kEdgeDirCount; ++jj )
			{
				ycVec3 edgeB = xformBtoWorld.TransformDir( TraitsB::GetEdgeDir( sB, jj ) );

				ycVec3 axis = edgeB.Cross( edgeA );

				if( axis.LengthSq() < ( 0.0001f * 0.0001f ) ) 
				{
					degenerateEvE = true;  // degenerate case, must run edge-v-vertex and vertex-v-edge
				}
				else
				{
					ycAssert( !ycIsNan( axis ) );
					CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-EdgeEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision )
					{
						return mtd;
					}
				}
			}
		}

		// If a positive contact is found among the FV and EE tests then it is the deepest contact;
		// no need to further test the VV and VE pairs unless the shapes are separated or an EE pair was degenerate
		if( TraitsA::GetRadius( sA ) <= 0.0f && TraitsB::GetRadius( sB ) <= 0.0f && !degenerateEvE && mtd.valid && mtd.mag > 0.0f ) 
		{
			return mtd;
		}

		bool orthoA = OrthonormalScale( sA );
		bool orthoB = OrthonormalScale( sB );

		// quick center-center check in case objects are far apart (i.e. combat)
		if( !orthoA || !orthoB )
		{
			ycVec3 vertA = TraitsA::GetForwardXform( sA ).TransformPos( TraitsA::GetVert( sA, 0 ) );
			ycVec3 vertB = TraitsB::GetForwardXform( sB ).TransformPos( TraitsB::GetVert( sB, 0 ) );

			ycVec3 axis = vertB-vertA;
			ycAssert( !ycIsNan( axis ) );
			CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-FastVertVert" );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}
		}

		// VERTEX v EDGE
		for( u32 jj = 0; jj < TraitsB::kEdgeCount; ++jj )
		{
			ycCollisionFeature edgeB = TraitsB::GetEdge( sB, jj );
			ycVec3 edge0 = edgeB.pts[ 0 ];
			ycVec3 edge1 = edgeB.pts[ 1 ];

			for( u32 ii = 0; ii < TraitsA::kVertCount; ++ii )
			{
				ycVec3 vertA = ( TraitsA::GetVert( sA, ii ) );

				ycVec3 axis = axisBToWorld.TransformDir( AxisForVertexEdge( vertA, edge0, edge1, sA, sB ) );
				ycAssert( !ycIsNan( axis ) );
				CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-VertEdge" );
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
		}

		for( u32 ii = 0; ii < TraitsA::kEdgeCount; ++ii )
		{
			ycCollisionFeature edgeA = TraitsA::GetEdge( sA, ii );
			ycVec3 edge0 = edgeA.pts[ 0 ];
			ycVec3 edge1 = edgeA.pts[ 1 ];

			for( u32 jj = 0; jj < TraitsB::kVertCount; ++jj )
			{
				ycVec3 vertB =( TraitsB::GetVert( sB, jj ) );

				ycVec3 axis = axisAToWorld.TransformDir( AxisForVertexEdge( vertB, edge0, edge1, sB, sA ) );
				ycAssert( !ycIsNan( axis ) );
				CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-EdgeVert" );
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
		}

		// Safe to return now if we were just handling a degenerate EvE case
		if( TraitsA::GetRadius( sA ) <= 0.0f && TraitsB::GetRadius( sB ) <= 0.0f &&
			mtd.valid && mtd.mag > 0.0f ) return mtd;

		// VERTEX v VERTEX
		for( u32 ii = 0; ii < TraitsA::kVertCount; ++ii )
		{
			ycVec3 vertA = TraitsA::GetVert( sA, ii );

			for( u32 jj = 0; jj < TraitsB::kVertCount; ++jj )
			{
				ycVec3 vertB = TraitsB::GetVert( sB, jj );

				ycVec3 axis = axisBToWorld.TransformDir( AxisForVertexVertex( vertA, vertB, sA, sB ) );
				ycAssert( !ycIsNan( axis ) );
				CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-VertVert" );
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
		}


		for( u32 jj = 0; jj < TraitsB::kVertCount; ++jj )
		{
			ycVec3 vertB = xformBtoA.TransformPos( TraitsB::GetVert( sB, jj ) );

			for( u32 ii = 0; ii < TraitsA::kVertCount; ++ii )
			{
				ycVec3 vertA = TraitsA::GetVert( sA, ii );
				
				ycVec3 axis = axisAToWorld.TransformDir( AxisForVertexVertex( vertB, vertA, sB, sA ) );
				ycAssert( !ycIsNan( axis ) );
				CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "Gen-VertVert" );
				if( mtd.valid && mtd.mag < -sepPrecision )
				{
					return mtd;
				}
			}
		}

		return mtd;
	}
	
	ycCollisionResult SepAxisTestSpecialize( const ycSphere& sA, const ycTri& sB, f32 sepPrecision, ycCollisionOptions::BackfaceBehavior backfaceBehavior )
	{
		typedef ycCollisionSAT::ShapeTraits<ycSphere> TraitsA;
		typedef ycCollisionSAT::ShapeTraits<ycTri> TraitsB;

		ycCollisionResult mtd;
		ycVec3 backface = ycVec3::ZERO();
		
		// FACE NORMALS
		{
			ycVec3 faceNormB = ( sB.b - sB.a ).Cross( sB.c - sB.a );

			bool chkBackface = (backfaceBehavior == ycCollisionOptions::ycCollisionOptions::kBackface_ResolveAsDoubleSided) || TraitsB::kAllowBackface;
			ycAssert( !ycIsNan( faceNormB ) );
			if( CheckAxis( faceNormB, sA, sB, mtd, chkBackface, 0.00001f, "ycSphere_v_Tri_TriFace" ) )
			{
				mtd.pointB = mtd.supportingB;
				mtd.featureB = ycCollisionFeature::FaceTri( sB.a, sB.b, sB.c );
				if( chkBackface ) { mtd.isBackface = IsBackface( sB, mtd.dir ); }
				mtd.faceB = mtd;
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
			if( !chkBackface )
			{
				backface = -faceNormB;
				ycAssert( !ycIsNan( faceNormB ) );
				if( CheckAxis( faceNormB, sA, sB, mtd, false, 0.0f, "Gen-FaceB" ) )
				{
					mtd.isBackface = true;
				}
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
		}

		// If a positive contact is found among the FV and EE tests then it is the deepest contact;
		// no need to further test the VV and VE pairs unless the shapes are separated or an EE pair was degenerate
		if( TraitsA::GetRadius( sA ) <= 0.0f && TraitsB::GetRadius( sB ) <= 0.0f && mtd.valid && mtd.mag > 0.0f ) 
		{
			return mtd; 
		}

		// VERTEX v EDGE
		for( u32 jj = 0; jj < TraitsB::kEdgeCount; ++jj )
		{
			ycVec3 edge0 = ( sB.pts[ jj ] );
			ycVec3 edge1 = ( sB.pts[ (jj+1) % 3 ] );

			ycVec3 vertA = sA.center;

			ycVec3 axis = ( vertA - edge0 ).Without( edge1 - edge0 );
			ycAssert( !ycIsNan( axis ) );
			CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "ycSphere_v_Tri_TriEdge" );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}
		}

		// Safe to return now if we were just handling a degenerate EvE case
		if( TraitsA::GetRadius( sA ) <= 0.0f && TraitsB::GetRadius( sB ) <= 0.0f && mtd.valid && mtd.mag > 0.0f ) 
		{
			return mtd;
		}

		// VERTEX v VERTEX
		for( u32 jj = 0; jj < TraitsB::kVertCount; ++jj )
		{
			ycVec3 vertB = ( TraitsB::GetVert( sB, jj ) );
			ycVec3 vertA = sA.center;

			ycVec3 axis = ( vertB - vertA );
			ycAssert( !ycIsNan( axis ) );
			CheckAxisSkipBackface( axis, backface, sA, sB, mtd, true, YC_F32_MAX, "ycSphere_v_Tri_TriVert" );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}
		}

		return mtd;
	}

	template <typename ShapeB>
	ycCollisionResult SepAxisTestSpecialize( const ycCollisionCylinder& cyl, const ShapeB& sB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior backfaceBehavior )
	{
		typedef ycCollisionSAT::ShapeTraits<ShapeB> TraitsB;

		const auto xformBtoWorld = TraitsB::GetForwardXform( sB );
		const auto axisBToWorld = TraitsB::GetInverseXform( sB ).Transposed();

		ycCollisionResult mtd;
		ycVec3 backface = ycVec3::ZERO();

		for( u32 jj = 0; jj < TraitsB::kFaceNormCount; ++jj ) 
		{
			ycVec3 faceNormB = axisBToWorld.TransformDir( TraitsB::GetFaceNorm( sB, jj ) );

			bool chkBackface = (backfaceBehavior == ycCollisionOptions::ycCollisionOptions::kBackface_ResolveAsDoubleSided) || TraitsB::kAllowBackface;
			ycAssert( !ycIsNan( faceNormB ) );
			if( CheckAxis( faceNormB, cyl, sB, mtd, chkBackface, 0.00001f, "Cyl_v_Shape_FaceB") )
			{
				mtd.pointB = mtd.supportingB;
				mtd.featureB = GetSupportingFeature( sB, mtd.dir, mtd.supportingB );
				if( chkBackface ) { mtd.isBackface = IsBackface( sB, mtd.dir ); }
				mtd.faceB = mtd;
				if( mtd.valid && mtd.mag < -sepPrecision )
				{
					return mtd;
				}
			}
			if( !chkBackface )
			{
				backface = -faceNormB;
				ycAssert( !ycIsNan( faceNormB ) );
				if( CheckAxis( faceNormB, cyl, sB, mtd, false, 0.0f, "Gen-FaceB" ) )
				{
					mtd.isBackface = true;
				}
				if( mtd.valid && mtd.mag < -sepPrecision ) 
				{
					return mtd;
				}
			}
		}

		ycAssert( !ycIsNan( cyl.upDir ) );
		CheckAxisSkipBackface( cyl.upDir, backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_CylFace" );
		if( mtd.valid && mtd.mag < -sepPrecision ) 
		{
			return mtd;
		}

		// EDGE vs EDGE
		for( u32 jj = 0; jj < TraitsB::kEdgeDirCount; ++jj )
		{
			const ycVec3 edgeDir = xformBtoWorld.TransformDir( TraitsB::GetEdgeDir( sB, jj ) );
			if( edgeDir.LengthSq() < 0.000000001f ) { continue; }

			const ycVec3 toEdge = edgeDir.Cross( cyl.upDir );
			ycAssert( !ycIsNan( toEdge ) );
			CheckAxisSkipBackface( toEdge, backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_CylEdge" );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}

			//ycVec3 capEdge = toEdge.Without( cyl.upDir );
			const ycVec3 capEdge = toEdge;
			const ycVec3 capAxis = edgeDir.Cross( capEdge );
			ycAssert( !ycIsNan( capAxis ) );
			CheckAxisSkipBackface( capAxis, backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_CapEdge" );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}
		}
		
		// VERTEX vs EDGE
		for( u32 jj = 0; jj < TraitsB::kEdgeCount; ++jj )
		{
			ycCollisionFeature edgeB = TraitsB::GetEdge( sB, jj );
			ycVec3 edge0InB = edgeB.pts[ 0 ];
			ycVec3 edge1InB = edgeB.pts[ 1 ];

			ycLine seg( xformBtoWorld.TransformPos( edge0InB ), xformBtoWorld.TransformDir( edge1InB - edge0InB ) );
			if( seg.slope.LengthSq() < 0.000000001f ) { continue; }

			auto TestCylDisc = [&cyl, &sB, &mtd, &seg, sepPrecision, &edge0InB, &edge1InB, &backface]( const ycVec3& discCenter )
			{
				const ycMtx4 xformBtoWorld = TraitsB::GetForwardXform( sB );
				const ycMtx4 axisBToWorld = TraitsB::GetInverseXform( sB ).Transposed();

				ycVec3 closestOnDisc = discCenter, closestOnEdge = seg.GetOrigin();
				ycPlane cylDiscPlane( cyl.upDir, discCenter );
				ycVec3 axisInB = ycVec3::YAXIS();

				// small iteration to refine closest point; I don't think this has a closed-form
				for( s32 ii = 0; ii < 12; ++ii )
				{
					ycVec3 closestOnDiscPrev = closestOnDisc;
					ycVec3 axisInBPrev = axisInB;

					seg.ClosestPoint( closestOnDisc, &closestOnEdge );

					axisInB = AxisForVertexEdge( closestOnDisc, edge0InB, edge1InB, cyl, sB );

					closestOnDisc = cylDiscPlane.Project( closestOnEdge + xformBtoWorld.TransformDir( axisInB ) );
					closestOnDisc = discCenter + ( closestOnDisc - discCenter ).Normalized() * cyl.radius;

					if( closestOnDisc.Dist( closestOnDiscPrev ) < 0.0001f && 
						axisInB.Normalized().Dist( axisInBPrev.Normalized() ) < 0.0001f )
					{
						break;
					}
				}

				{
					ycVec3 axis = axisBToWorld.TransformDir( axisInB );
					ycAssert( !ycIsNan( axis ) );
					CheckAxisSkipBackface( axis.Normalized(), backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_DiscEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return;
					}
				}

				{
					ycVec3 cylRimDir = ( closestOnDisc - discCenter ).Cross( cyl.upDir );
					ycVec3 axis = ( xformBtoWorld.TransformPos( edge1InB ) - xformBtoWorld.TransformPos( edge0InB ) ).Cross( cylRimDir );

					ycAssert( !ycIsNan( axis ) );
					CheckAxisSkipBackface( axis, backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_DiscEdgeB" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return;
					}
				}
			};

			ycVec3 cylBtm = cyl.center - 0.5f * cyl.height * cyl.upDir;
			ycVec3 cylTop = cyl.center + 0.5f * cyl.height * cyl.upDir;

			TestCylDisc( cylBtm );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}

			TestCylDisc( cylTop );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}

		}

		// EDGE v VERTEX
		for( u32 jj = 0; jj < TraitsB::kVertCount; ++jj )
		{
			ycLine line( cyl.center, cyl.upDir );
			ycVec3 vertB = TraitsB::GetVert( sB, jj );

			ycVec3 axis = AxisForVertexEdge( vertB, cyl.center, cyl.center + cyl.upDir, sB, cyl );

			ycAssert( !ycIsNan( axis ) );
			CheckAxisSkipBackface( axis, backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_EdgeVert" );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}
		}

		// VERTEX v VERTEX
		for( u32 jj = 0; jj < TraitsB::kVertCount; ++jj )
		{
			const ycVec3 vertB = TraitsB::GetVert( sB, jj );

			auto TestCylDisc = [&cyl, &sB, &mtd, &vertB, &backface]( const ycVec3& discCenter )
			{
				const auto xformBtoA = TraitsB::GetForwardXform( sB );
				const auto axisBToWorld = TraitsB::GetInverseXform( sB ).Transposed();

				ycVec3 closestOnDisc = discCenter;
				ycVec3 axisInB = ycVec3::YAXIS();
				ycPlane cylDiscPlane( cyl.upDir, discCenter );

				// small iteration to refine closest point; I don't think this has a closed-form
				for( int ii = 0; ii < 12; ++ii )
				{
					ycVec3 closestOnDiscPrev = closestOnDisc;
					ycVec3 axisInBPrev = axisInB;

					axisInB = AxisForVertexVertex( closestOnDisc, vertB, cyl, sB );

					closestOnDisc = cylDiscPlane.Project( xformBtoA.TransformPos( vertB + axisInB ) );
					closestOnDisc = discCenter + ( closestOnDisc - discCenter ).Normalized() * cyl.radius;

					if( closestOnDisc.Dist( closestOnDiscPrev ) < 0.0001f &&
						axisInB.Normalized().Dist( axisInBPrev.Normalized() ) < 0.0001f )
					{
						break;
					}
				}

				{
					ycVec3 axis = axisBToWorld.TransformDir( axisInB );
					ycAssert( !ycIsNan( axis ) );
					CheckAxisSkipBackface( axis, backface, cyl, sB, mtd, true, YC_F32_MAX, "Cyl_v_Shape_VertVert" );
				}
			};


			ycVec3 cylBtm = cyl.center - 0.5f * cyl.height * cyl.upDir;
			ycVec3 cylTop = cyl.center + 0.5f * cyl.height * cyl.upDir;

			TestCylDisc( cylBtm );
			if( mtd.valid && mtd.mag < -sepPrecision )
			{
				return mtd;
			}

			TestCylDisc( cylTop );
			if( mtd.valid && mtd.mag < -sepPrecision )
			{
				return mtd;
			}
		}

		return mtd;
	}

	ycCollisionResult SepAxisTestSpecialize( const ycCollisionCylinder& cylA, const ycCollisionCylinder& cylB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior backfaceBehavior )
	{
		YC_UNUSED( backfaceBehavior );

		ycCollisionResult mtd;

		ycAssert( !ycIsNan( cylB.upDir ) );
		CheckAxis( cylB.upDir, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_faceB");
		if( mtd.valid && mtd.mag < -sepPrecision ) 
		{
			return mtd;
		}

		ycAssert( !ycIsNan( cylA.upDir ) );
		CheckAxis( cylA.upDir, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_faceA" );
		if( mtd.valid && mtd.mag < -sepPrecision ) 
		{
			return mtd;
		}

		// EDGE vs EDGE
		ycVec3 perpAxis = cylA.upDir.Cross( cylB.upDir );
		if( perpAxis.Mag() < 0.000001f ) 
		{
			perpAxis = ( cylA.center - cylB.center ).Without( cylA.upDir ).Normalized();
		}

		ycAssert( !ycIsNan( perpAxis ) );
		CheckAxis( perpAxis, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_EdgeEdge" );
		if( mtd.valid && mtd.mag < -sepPrecision )
		{
			return mtd;
		}

		ycVec3 capAxisA = cylA.upDir.Cross( perpAxis );
		ycAssert( !ycIsNan( capAxisA ) );
		CheckAxis( capAxisA.Cross( cylB.upDir ), cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_CapEdge" );
		if( mtd.valid && mtd.mag < -sepPrecision ) 
		{
			return mtd;
		}

		ycVec3 capAxisB = cylB.upDir.Cross( perpAxis );
		ycAssert( !ycIsNan( capAxisB ) );
		CheckAxis( capAxisB.Cross( cylA.upDir ), cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_EdgeCap" );
		if( mtd.valid && mtd.mag < -sepPrecision ) 
		{
			return mtd;
		}

		// VERTEX vs EDGE
		{
			ycLine seg( cylB.center, cylB.upDir );

			{
				ycVec3 closestOnDisc;
				ycVec3 cylBtm = cylA.center - 0.5f * cylA.height * cylA.upDir;
				ycPlane cylBtmPlane( cylA.upDir, cylBtm );
				if( seg.Intersection( &closestOnDisc, cylBtmPlane ) )
				{
					closestOnDisc = cylBtm + ( closestOnDisc - cylBtm ).Normalized() * cylA.radius;
				}
				else
				{
					closestOnDisc = cylBtm;
				}

				{
					// small iteration to refine closest point; I don't think this has a closed-form
					ycVec3 closestOnEdge;
					for( s32 ii = 0; ii < 12; ++ii )
					{
						seg.ClosestPoint( closestOnDisc, &closestOnEdge );
						closestOnDisc = cylBtmPlane.Project( closestOnEdge );
						closestOnDisc = cylBtm + ( closestOnDisc - cylBtm ).Normalized() * cylA.radius;
					}

					CheckAxis( closestOnEdge-closestOnDisc, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}

					ycVec3 cylRimDir = ( closestOnDisc - cylBtm ).Cross( cylA.upDir );
					ycVec3 axis = ( cylB.upDir ).Cross( cylRimDir );

					ycAssert( !ycIsNan( axis ) );
					CheckAxis( axis, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}
				}
			}

			{
				ycVec3 closestOnDisc;
				ycVec3 cylTop = cylA.center + 0.5f * cylA.height * cylA.upDir;
				ycPlane cylTopPlane( cylA.upDir, cylTop );
				if( seg.Intersection( &closestOnDisc, cylTopPlane ) )
				{
					closestOnDisc = cylTop + ( closestOnDisc - cylTop ).Normalized() * cylA.radius;
				}
				else
				{
					closestOnDisc = cylTop;
				}

				{
					// small iteration to refine closest point; I don't think this has a closed-form
					ycVec3 closestOnEdge;
					for( s32 ii = 0; ii < 12; ++ii )
					{
						seg.ClosestPoint( closestOnDisc, &closestOnEdge );
						closestOnDisc = cylTopPlane.Project( closestOnEdge );
						closestOnDisc = cylTop + ( closestOnDisc - cylTop ).Normalized() * cylA.radius;
					}

					CheckAxis( closestOnEdge-closestOnDisc, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}

					ycVec3 cylRimDir = ( closestOnDisc - cylTop ).Cross( cylA.upDir );
					ycVec3 axis = ( cylB.upDir ).Cross( cylRimDir );

					ycAssert( !ycIsNan( axis ) );
					CheckAxis( axis, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}
				}
			}
		}
		{
			ycLine seg( cylA.center, cylA.upDir );

			{
				ycVec3 closestOnDisc;
				ycVec3 cylBtm = cylB.center - 0.5f * cylB.height * cylB.upDir;
				ycPlane cylBtmPlane( cylB.upDir, cylBtm );
				if( seg.Intersection( &closestOnDisc, cylBtmPlane ) )
				{
					closestOnDisc = cylBtm + ( closestOnDisc - cylBtm ).Normalized() * cylB.radius;
				}
				else
				{
					closestOnDisc = cylBtm;
				}

				{
					// small iteration to refine closest point; I don't think this has a closed-form
					ycVec3 closestOnEdge;
					for( s32 ii = 0; ii < 12; ++ii )
					{
						seg.ClosestPoint( closestOnDisc, &closestOnEdge );
						closestOnDisc = cylBtmPlane.Project( closestOnEdge );
						closestOnDisc = cylBtm + ( closestOnDisc - cylBtm ).Normalized() * cylB.radius;
					}

					CheckAxis( closestOnEdge-closestOnDisc, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}

					ycVec3 cylRimDir = ( closestOnDisc - cylBtm ).Cross( cylB.upDir );
					ycVec3 axis = ( cylA.upDir ).Cross( cylRimDir );

					ycAssert( !ycIsNan( axis ) );
					CheckAxis( axis, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}
				}
			}

			{
				ycVec3 closestOnDisc;
				ycVec3 cylTop = cylB.center + 0.5f * cylB.height * cylB.upDir;
				ycPlane cylTopPlane( cylB.upDir, cylTop );
				if( seg.Intersection( &closestOnDisc, cylTopPlane ) )
				{
					closestOnDisc = cylTop + ( closestOnDisc - cylTop ).Normalized() * cylB.radius;
				}
				else
				{
					closestOnDisc = cylTop;
				}

				{
					// small iteration to refine closest point; I don't think this has a closed-form
					ycVec3 closestOnEdge;
					for( s32 ii = 0; ii < 12; ++ii )
					{
						seg.ClosestPoint( closestOnDisc, &closestOnEdge );
						closestOnDisc = cylTopPlane.Project( closestOnEdge );
						closestOnDisc = cylTop + ( closestOnDisc - cylTop ).Normalized() * cylB.radius;
					}

					CheckAxis( closestOnEdge-closestOnDisc, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}

					ycVec3 cylRimDir = ( closestOnDisc - cylTop ).Cross( cylB.upDir );
					ycVec3 axis = ( cylA.upDir ).Cross( cylRimDir );

					ycAssert( !ycIsNan( axis ) );
					CheckAxis( axis, cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertEdge" );
					if( mtd.valid && mtd.mag < -sepPrecision ) 
					{
						return mtd;
					}
				}
			}
		}


		// VERTEX v VERTEX
		{
			auto CheckDiscDisc = [&cylA, &cylB, &mtd]( const ycVec3& discACenter, ycPlane discAPlane, f32 discARadius,
										const ycVec3& discBCenter, ycPlane discBPlane, f32 discBRadius )
			{
				ycVec3 closestOnDiscA = discACenter;
				ycVec3 closestOnDiscB = discBCenter;

				for( int ii = 0; ii < 12; ++ii )
				{
					closestOnDiscA = discAPlane.Project( closestOnDiscB );
					closestOnDiscA = discACenter + ( closestOnDiscA - discACenter ).Normalized() * discARadius;
					closestOnDiscB = discBPlane.Project( closestOnDiscB );
					closestOnDiscB = discBCenter + ( closestOnDiscB - discBCenter ).Normalized() * discBRadius;
				}

				ycAssert( !ycIsNan( ( closestOnDiscB  - closestOnDiscA ) ) );
				CheckAxis( ( closestOnDiscB  - closestOnDiscA ), cylA, cylB, mtd, true, YC_F32_MAX, "Cyl-cyl_VertVert" );
			};


			ycVec3 cylBBtm = cylB.center - 0.5f * cylB.height * cylB.upDir;
			ycPlane cylBBtmPlane( cylB.upDir, cylBBtm );

			ycVec3 cylABtm = cylA.center - 0.5f * cylA.height * cylA.upDir;
			ycPlane cylABtmPlane( cylA.upDir, cylABtm );

			ycVec3 cylBTop = cylB.center + 0.5f * cylB.height * cylB.upDir;
			ycPlane cylBTopPlane( cylB.upDir, cylBTop );

			ycVec3 cylATop = cylA.center + 0.5f * cylA.height * cylA.upDir;
			ycPlane cylATopPlane( cylA.upDir, cylATop );

			CheckDiscDisc( cylABtm, cylABtmPlane, cylA.radius, cylBBtm, cylBBtmPlane, cylB.radius );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}

			CheckDiscDisc( cylATop, cylATopPlane, cylA.radius, cylBBtm, cylBBtmPlane, cylB.radius );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}

			CheckDiscDisc( cylABtm, cylABtmPlane, cylA.radius, cylBTop, cylBTopPlane, cylB.radius );
			if( mtd.valid && mtd.mag < -sepPrecision ) 
			{
				return mtd;
			}

			CheckDiscDisc( cylATop, cylATopPlane, cylA.radius, cylBTop, cylBTopPlane, cylB.radius );
			if( mtd.valid && mtd.mag < -sepPrecision )
			{
				return mtd;
			}
		}

		return mtd;
	}

	template <typename ShapeA>
	ycCollisionResult SepAxisTestSpecialize( const ShapeA& sA, const ycCollisionCylinder& sB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior backfaceBehavior )
	{
		ycCollisionResult mtd = SepAxisTestSpecialize( sB, sA, sepPrecision, backfaceBehavior );
		ReverseResult( mtd );
		return mtd;
	}

	template <typename ShapeA, typename ShapeB>
	void FinalizeResult( const ShapeA& sA, const ShapeB& sB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, ycCollisionResult& mtd )
	{		
		YC_UNUSED( sepPrecision );

		// safety check, if no axes have turned out valid (possibly because of close overlap)
		if( !mtd.valid )
		{
			CheckAxis( ycVec3::YAXIS(), sA, sB, mtd, true, YC_F32_MAX, "Fallback" );
			CheckAxis( ycVec3::XAXIS(), sA, sB, mtd, true, YC_F32_MAX, "Fallback" );
			CheckAxis( ycVec3::ZAXIS(), sA, sB, mtd, true, YC_F32_MAX, "Fallback" );
		}

		if( allowBackface == ycCollisionOptions::kBackface_Ignore )
		{
			if( mtd.faceB.valid && mtd.faceB.dir.Dot( mtd.dir ) < 0.0f )
			{
				mtd = ycCollisionResult();
				return;
			}
		}

		if( mtd.valid )
		{
			mtd.featureB = GetSupportingFeature( sB, mtd.dir, mtd.supportingB );
			if( mtd.featureB.IsFace() && !mtd.faceB.valid )
			{
				mtd.faceB = mtd;
			}
			// detect a collision with a backface, replace with a saved valid face
			if( allowBackface == ycCollisionOptions::kBackface_ResolveOutward &&
				mtd.mag > 0.0f &&
				mtd.featureB.IsFace() && mtd.faceB.valid &&
				mtd.faceB.dir.Dot( mtd.dir ) < 0.0f )
			{
				mtd.dir = mtd.faceB.dir;
				mtd.mag = mtd.faceB.mag;
				mtd.supportingA = mtd.faceB.supportingA;
				mtd.supportingB = mtd.faceB.supportingB;
				mtd.isBackface = mtd.faceB.isBackface;
				mtd.featureB = GetSupportingFeature( sB, mtd.dir, mtd.supportingB );
			}
			mtd.featureA = GetSupportingFeature( sA, -mtd.dir, mtd.supportingA );
			ycCollisionFeatureUtil::SetFinalPoint( mtd );
		}

		ycAssert( !mtd.valid || mtd.dir.IsNormalized() );
	}

	template <typename ShapeA, typename ShapeB>
	ycCollisionResult SepAxisTestGeneric( const ShapeA& sA, const ShapeB& sB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior backfaceBehavior, bool weldingOn )
	{
		YC_UNUSED( weldingOn );

		ycCollisionResult mtd = SepAxisTestSpecialize( sA, sB, sepPrecision + 0.015625f, backfaceBehavior );

		FinalizeResult( sA, sB, sepPrecision, backfaceBehavior, mtd );

		if( mtd.valid && mtd.mag >= 0 )
		{
			bool makeTest = ycCollisionTest::ShouldCapture( ycCollisionTest::kCapture_Contact );
			if( makeTest )
			{
				ycLog( "{\n" );
				ycLog( "%s\n", ycCollisionTest::MakeLiteral( sA, "sA" ).Get() );
				ycLog( "%s\n", ycCollisionTest::MakeLiteral( sB, "sB" ).Get() );
				ycLog( "%s\n\n", ycCollisionTest::MakeLiteral( sepPrecision, "sepPrecision" ).Get() );
				ycLog( "ycCollisionResult res = SepAxisTestGeneric( sA, sB, sepPrecision, ycCollisionOptions::BackfaceBehavior( %d ) );\n\n", backfaceBehavior );

				ycLog( "%s", ycCollisionTest::TestMtd( mtd, "res" ).Get() );
				ycLog( "}\n" );
			}
		}

		return mtd;
	}

	template <typename ShapeA, typename ShapeB>
	f32 SweepAxis( const ycVec3& axis, const ShapeA& sA, const ycVec3& dir, const ShapeB& sB )
	{
		if( axis.LengthSq() < 0.001f ) { return false; }

		f32 minDotA, minDotB, maxDotA, maxDotB;
		ycVec3 minPtA, minPtB, maxPtA, maxPtB;

		ycAssert( axis.LengthSq() > 0.0f );
		ycAssertMsg( axis.IsNormalized(), "Length: %d", (f32)axis.Length() );
		ycCollisionSAT::AxisProjection( sA, axis, minDotA, minPtA, maxDotA, maxPtA );
		ycCollisionSAT::AxisProjection( sB, axis, minDotB, minPtB, maxDotB, maxPtB );
		
		f32 axisVel = axis.Dot( dir );

		if( ( maxDotB >= minDotA ) && ( maxDotA >= minDotB ) )
		{
			return 0.0f;
		}
		else if( maxDotB < minDotA && axisVel < 0.0f )
		{
			f32 distance = minDotA - maxDotB; // always positive
			f32 tt = -( distance / axisVel );
			return tt;
		}
		else if( maxDotA < minDotB && axisVel > 0.0f )
		{
			f32 distance = minDotB - maxDotA; // always positive
			f32 tt = ( distance / axisVel );
			return tt;
		}

		return YC_F32_MAX;
	};

	template <typename ShapeA, typename ShapeB>
	bool IterativeSweepGeneric( const ShapeA& sA, const ShapeB& sB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
	{
		YC_UNUSED( weldingOn );

		typedef ycCollisionSAT::ShapeTraits<ShapeB> TraitsB;
		bool ignoreBackface = ( backface == ycCollisionOptions::kBackface_Ignore );

		// for the sweep test, we always want a result so we can compare against move - so this is to cut off the ignore handling for the plain collide test
		if( ignoreBackface ) { backface = ycCollisionOptions::kBackface_ResolveOutward; }

		// if welding is on, backfaces need to be in the results so welding can see doubled tris
		if( weldingOn ) { ignoreBackface = false; }

		f32 distWithSlop = dist + ycClamp( dist * 0.1f, 0.01f, 0.1f );

		ycCollisionResult mtd;
		mtd = SepAxisTestSpecialize( sA, sB, 0.0f, backface );
		if( !TraitsB::kAllowBackface && ignoreBackface && mtd.faceB.valid && mtd.faceB.dir.Dot( dir ) > 0.0f )
		{
			// backfacing tri, skip
			return false;
		}
		if( mtd.mag >= 0.0f )
		{
			// starting collision
			//ycAssert( mtd.faceB.valid );
			if( contactPoint ) 
			{
				FinalizeResult( sA, sB, 0.0f, backface, mtd );
				mtd.sweepDist = 0.0f;
				mtd.fullSweepDist = dist;
				*contactPoint = mtd;
			}
			return true;
		}

		f32 sweepDist = SweepAxis( mtd.dir, sA, dir, sB );
		if( sweepDist >= distWithSlop ) { return false; }

		// lower bound is closer than our previous best, possible hit! refine the position
		f32 stepDist = sweepDist;
		f32 knownClearDist = 0.0f;
		ShapeA sAstep = sA;

		//for( int iter = 0; iter < 32; ++iter )
		while( true )
		{
			ycCollisionSAT::ShapeTraits<ShapeA>::MoveShape( sAstep, stepDist * dir );
			knownClearDist += stepDist;

			if( knownClearDist >= distWithSlop )
			{
				return false;
			}

			// regular step uses double-sided so it won't be "stopped" by the backproject of the tri
			mtd = SepAxisTestSpecialize( sAstep, sB, 0.0f, ycCollisionOptions::kBackface_ResolveAsDoubleSided );
			if( mtd.mag > 0.0f ) 
			{
				if( backface != ycCollisionOptions::kBackface_ResolveAsDoubleSided )
				{
					// might return this result, so replace it with a result with the correct backface behavior
					mtd = SepAxisTestSpecialize( sAstep, sB, 0.0f, backface );
				}
				if( mtd.mag > 0.0f )
				{
					break;
				}
			}
			stepDist = SweepAxis( mtd.dir, sAstep, dir, sB );

			if( stepDist < kPrecision )
			{
				ShapeA sFinstep = sAstep;
				ycCollisionSAT::ShapeTraits<ShapeA>::MoveShape( sFinstep, kPrecision * 2.0f * dir );
				ycCollisionResult tstmtd = SepAxisTestSpecialize( sFinstep, sB, dist + 10.0f, backface );
				if( tstmtd.mag >= 0.0f )
				{
					if( !TraitsB::kAllowBackface && ignoreBackface && tstmtd.faceB.valid && tstmtd.faceB.dir.Dot( dir ) > 0.0f )
					{
						// backfacing tri, skip
						return false;
					}
					ycCollisionSAT::ShapeTraits<ShapeA>::MoveShape( sAstep, stepDist * dir );
					knownClearDist += stepDist;
					mtd = SepAxisTestSpecialize( sAstep, sB, 0.1f, backface );
					break;
				}
				stepDist = ycMax( kPrecision * 2.0f, SweepAxis( tstmtd.dir, sFinstep, dir, sB ) );
			}
		}

		//if( ycAbs( mtd.mag ) > kPrecision )
		//{
		//	ycAssert( 0 );
		//	return false;
		//}
		if( !TraitsB::kAllowBackface && ignoreBackface && mtd.faceB.valid && mtd.faceB.dir.Dot( dir ) > 0.0f )
		{
			// backfacing tri, skip
			return false;
		}

		if( knownClearDist >= distWithSlop )
		{
			return false;
		}

		//ycAssert( mtd.faceB.valid );
		if( contactPoint ) 
		{
			FinalizeResult( sAstep, sB, 0.0f, backface, mtd );
			mtd.sweepDist = knownClearDist;
			mtd.fullSweepDist = dist;
			*contactPoint = mtd; 
		}
		
		bool makeTest = ycCollisionTest::ShouldCapture( ycCollisionTest::kCapture_SweepHits );
		if( makeTest )
		{
			ycLog( "{\n" );
			ycLog( "%s\n", ycCollisionTest::MakeLiteral( sA, "sA" ).Get() );
			ycLog( "%s\n", ycCollisionTest::MakeLiteral( sB, "sB" ).Get() );
			ycLog( "%s\n", ycCollisionTest::MakeLiteral( dir, "dir" ).Get() );
			ycLog( "%s\n", ycCollisionTest::MakeLiteral( dist, "dist" ).Get() );
			ycLog( "%s\n\n", ycCollisionTest::MakeLiteral( kPrecision, "kPrecision" ).Get() );

			ycLog( "ycCollisionResult res;\n" );
			ycLog( "ycCollisionSAT::SatIterativeSweep( sA, sB, dir, dist, &res, ycCollisionOptions::BackfaceBehavior( %d ), kPrecision );\n\n" );

			ycLog( "%s", ycCollisionTest::TestMtd( mtd, "res" ).Get() );
			ycLog( "}\n" );
		}

		return true;
	}
}

ycVec3 ycCollisionSAT::ShapeTraits<ycTri>::GetVert( const ycTri& tt, u32 ii )
{
	ycAssert( ii < 3 );
	return ( tt.pts[ ii ] );
}

ycCollisionFeature ycCollisionSAT::ShapeTraits<ycTri>::GetEdge( const ycTri& tt, u32 ii )
{
	ycAssert( ii < 3 );
	return ycCollisionFeature::Edge( tt.pts[ ii ], tt.pts[ ( ii + 1 ) % 3 ] );
}

ycVec3 ycCollisionSAT::ShapeTraits<ycTri>::GetEdgeDir( const ycTri& tt, u32 ii )
{
	ycAssert( ii < 3 );
	return ( tt.pts[ ( ii + 1 ) % 3 ] - tt.pts[ ii ] );
}

ycVec3 ycCollisionSAT::ShapeTraits<ycTri>::GetFaceNorm( const ycTri& tt, u32 ii )
{
	YC_UNUSED( ii );
	ycAssert( ii == 0 );
	ycAssert( !ycIsNan( tt.GetNormal() ) );
	return tt.GetNormal();
}

ycVec3 ycCollisionSAT::ShapeTraits<ycMtx4>::GetVert( const ycMtx4& tt, u32 ii )
{
	ycAssert( ii < 8 );
	constexpr ycVec3 verts[ 8 ] = {
		ycVec3( -1,-1,-1 ),
		ycVec3( -1,-1, 1 ),
		ycVec3( -1, 1,-1 ),
		ycVec3( -1, 1, 1 ),
		ycVec3( 1,-1,-1 ),
		ycVec3( 1,-1, 1 ),
		ycVec3( 1, 1,-1 ),
		ycVec3( 1, 1, 1 ),
	};

	return ( tt.TransformPos( verts[ ii ] ) );
}

ycCollisionFeature ycCollisionSAT::ShapeTraits<ycMtx4>::GetEdge( const ycMtx4& tt, u32 ii )
{
	ycAssert( ii < 12 );
	constexpr ycVec3 edges[ 12 ][ 2 ] = {
		{ ycVec3( -1,-1,-1 ), ycVec3( 1,-1,-1 ) },
		{ ycVec3( -1,-1, 1 ), ycVec3( 1,-1, 1 ) },
		{ ycVec3( -1, 1,-1 ), ycVec3( 1, 1,-1 ) },
		{ ycVec3( -1, 1, 1 ), ycVec3( 1, 1, 1 ) },

		{ ycVec3( -1,-1,-1 ), ycVec3( -1,1,-1 ) },
		{ ycVec3( -1,-1, 1 ), ycVec3( -1,1, 1 ) },
		{ ycVec3( 1,-1,-1 ), ycVec3( 1,1,-1 ) },
		{ ycVec3( 1,-1, 1 ), ycVec3( 1,1, 1 ) },

		{ ycVec3( -1,-1,-1 ), ycVec3( -1,-1,1 ) },
		{ ycVec3( -1, 1,-1 ), ycVec3( -1, 1,1 ) },
		{ ycVec3( 1,-1,-1 ), ycVec3( 1,-1,1 ) },
		{ ycVec3( 1, 1,-1 ), ycVec3( 1, 1,1 ) },
	};

	return ycCollisionFeature::Edge( tt.TransformPos( edges[ ii ][ 0 ] ), tt.TransformPos( edges[ ii ][ 1 ] ) );
}

ycVec3 ycCollisionSAT::ShapeTraits<ycMtx4>::GetEdgeDir( const ycMtx4& tt, u32 ii )
{
	ycAssert( ii < 3 );
	constexpr ycVec3 edges[ 3 ] = {
			ycVec3::XAXIS() ,
			ycVec3::YAXIS() ,
			ycVec3::ZAXIS() ,
	};

	return tt.TransformDir( edges[ ii ] );
}

ycVec3 ycCollisionSAT::ShapeTraits<ycMtx4>::GetFaceNorm( const ycMtx4& tt, u32 ii )
{
	ycAssert( ii < 3 );
	switch( ii )
	{
		case 0:
			ycAssert( !ycIsNan( tt.x.xyz() ) );
			return tt.x.xyz().Normalized();
		case 1:
			ycAssert( !ycIsNan( tt.y.xyz() ) );
			return tt.y.xyz().Normalized();
		case 2:
			ycAssert( !ycIsNan( tt.z.xyz() ) );
			return tt.z.xyz().Normalized();
		default:
			ycAssert( 0 );
	}

	return ycVec3::YAXIS();
}

ycVec3 ycCollisionSAT::ShapeTraits<ycAABB>::GetVert( const ycAABB& tt, u32 ii )
{
	return tt.GetCorner( ii );
}

ycCollisionFeature ycCollisionSAT::ShapeTraits<ycAABB>::GetEdge( const ycAABB& tt, u32 ii )
{
	ycAssert( ii < 12 );
	constexpr ycVec3 edges[ 12 ][ 2 ] = {
		{ ycVec3( -1,-1,-1 ), ycVec3( 1,-1,-1 ) },
		{ ycVec3( -1,-1, 1 ), ycVec3( 1,-1, 1 ) },
		{ ycVec3( -1, 1,-1 ), ycVec3( 1, 1,-1 ) },
		{ ycVec3( -1, 1, 1 ), ycVec3( 1, 1, 1 ) },

		{ ycVec3( -1,-1,-1 ), ycVec3( -1,1,-1 ) },
		{ ycVec3( -1,-1, 1 ), ycVec3( -1,1, 1 ) },
		{ ycVec3( 1,-1,-1 ), ycVec3( 1,1,-1 ) },
		{ ycVec3( 1,-1, 1 ), ycVec3( 1,1, 1 ) },

		{ ycVec3( -1,-1,-1 ), ycVec3( -1,-1,1 ) },
		{ ycVec3( -1, 1,-1 ), ycVec3( -1, 1,1 ) },
		{ ycVec3( 1,-1,-1 ), ycVec3( 1,-1,1 ) },
		{ ycVec3( 1, 1,-1 ), ycVec3( 1, 1,1 ) },
	};

	ycVec3 e0 = tt.GetCenter() + tt.GetExtents() * edges[ ii ][ 0 ];
	ycVec3 e1 = tt.GetCenter() + tt.GetExtents() * edges[ ii ][ 1 ];

	return ycCollisionFeature::Edge( e0, e1 );
}

ycVec3 ycCollisionSAT::ShapeTraits<ycAABB>::GetEdgeDir( const ycAABB&, u32 ii )
{
	ycAssert( ii < 3 );
	switch( ii )
	{
		case 0:
			return ycVec3::XAXIS();
		case 1:
			return ycVec3::YAXIS();
		case 2:
			return ycVec3::ZAXIS();
		default:
			ycAssert( 0 );
	}

	return ycVec3::YAXIS();
}

ycVec3 ycCollisionSAT::ShapeTraits<ycAABB>::GetFaceNorm( const ycAABB&, u32 ii )
{
	ycAssert( ii < 3 );
	switch( ii )
	{
		case 0:
			return ycVec3::XAXIS();
		case 1:
			return ycVec3::YAXIS();
		case 2:
			return ycVec3::ZAXIS();
		default:
			ycAssert( 0 );
	}

	return ycVec3::YAXIS();
}

void ycCollisionSAT::AxisProjection( const ycVec3& point, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	minPoint = maxPoint = point;
	minDot = maxDot = axis.Dot( point );
}

void ycCollisionSAT::AxisProjection( const ycTri& tri, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	f32 dotA = axis.Dot( tri.a );
	f32 dotB = axis.Dot( tri.b );
	f32 dotC = axis.Dot( tri.c );

	minDot = ycMin( ycMin( dotA, dotB ), dotC );
	maxDot = ycMax( ycMax( dotA, dotB ), dotC );

	if( minDot == dotA ) { minPoint = tri.a; }
	else if( minDot == dotB ) { minPoint = tri.b; }
	else /*if( minDot == dotC )*/ { minPoint = tri.c; }

	if( maxDot == dotA ) { maxPoint = tri.a; }
	else if( maxDot == dotB ) { maxPoint = tri.b; }
	else /*if( maxDot == dotC )*/ { maxPoint = tri.c; }
}

void ycCollisionSAT::AxisProjection( const ycSphere& s, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	f32 dot = axis.Dot( s.center );

	ycVec3 axisNorm = axis;
	ycAssertMsg( axisNorm.IsNormalized(), "Length: %d", (f32)axisNorm.Length() );

	f32 scaledRadius = s.radius /* * axisNorm.Normalize() */ ;
	minDot = dot - scaledRadius;
	maxDot = dot + scaledRadius;

	axisNorm *= s.radius;
	minPoint = s.center - axisNorm;
	maxPoint = s.center + axisNorm;
}

void ycCollisionSAT::AxisProjection( const ycCollisionSphere& s, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	// the inverse transpose of the inverse is the transpose of the forward
	ycVec3 projAxis = s.forward.Transposed().TransformDir( axis ).Normalized();

	// the min and max points are points and so are transformed using the regular forward mat
	ycVec3 radialVec = s.forward.TransformDir( projAxis * s.radius );

	ycVec3 center = s.forward.t.xyz();
	minPoint = center - radialVec;
	maxPoint = center + radialVec;

	f32 centerDot = axis.Dot( center );
	f32 radialDot = axis.Dot( radialVec );
	minDot = centerDot - radialDot;
	maxDot = centerDot + radialDot;
}

void ycCollisionSAT::AxisProjection( const ycMtx4& box, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	const ycVec3 x = box.x.xyz();
	const ycVec3 y = box.y.xyz();
	const ycVec3 z = box.z.xyz();
	const ycVec3 t = box.t.xyz();

	const f32 dx = axis.Dot( x );
	const f32 dy = axis.Dot( y );
	const f32 dz = axis.Dot( z );

	const ycVec3 sumDirs =
		ycSign( dx ) * x +
		ycSign( dy ) * y +
		ycSign( dz ) * z;

	f32 sumDots = ycAbs( dx ) + ycAbs( dy ) + ycAbs( dz );
	f32 centerDot = axis.Dot( t );

	minDot = centerDot - sumDots;
	minPoint = t - sumDirs;
	maxDot = centerDot + sumDots;
	maxPoint = t + sumDirs;
}
	
void ycCollisionSAT::AxisProjection( const ycCollisionBox& box, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	const ycVec3 x = box.forward.x.xyz();
	const ycVec3 y = box.forward.y.xyz();
	const ycVec3 z = box.forward.z.xyz();
	const ycVec3 t = box.forward.t.xyz();

	const f32 dx = axis.Dot( x );
	const f32 dy = axis.Dot( y );
	const f32 dz = axis.Dot( z );

	const ycVec3 sumDirs =
		ycSign( dx ) * x +
		ycSign( dy ) * y +
		ycSign( dz ) * z;

	f32 sumDots = ycAbs( dx ) + ycAbs( dy ) + ycAbs( dz );
	f32 centerDot = axis.Dot( t );

	minDot = centerDot - sumDots;
	minPoint = t - sumDirs;
	maxDot = centerDot + sumDots;
	maxPoint = t + sumDirs;
}
	
void ycCollisionSAT::AxisProjection( const ycCollisionCapsule& cap, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	AxisProjectionGeneric( cap, axis, minDot, minPoint, maxDot, maxPoint );
}
	
void ycCollisionSAT::AxisProjection( const ycAABB& aabb, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	AxisProjectionGeneric( aabb, axis, minDot, minPoint, maxDot, maxPoint );
}

void ycCollisionSAT::AxisProjection( const ycCollisionCylinder& cyl, const ycVec3& axis, f32& minDot, ycVec3& minPoint, f32& maxDot, ycVec3& maxPoint )
{
	ycAssert( axis.LengthSq() > 0.0f );
	ycAssertMsg( axis.IsNormalized(), "Length: %d", (f32)axis.Length() );

	ycVec3 perp = ycVec3::ZERO();    // vector perpendicular to cyl, on plane of axis

	const f32 adotc = axis.Dot( cyl.upDir );
	f32 sinAngleSq = ( 1.0f - adotc * adotc );

	if( sinAngleSq > 0.0f )
	{
		perp = axis - cyl.upDir * adotc;
		perp *= cyl.radius * ycInvSqrt( sinAngleSq );
	}

	const ycVec3 sOffset = ( ycSign( adotc ) * cyl.upDir * cyl.height * 0.5f + perp );

	f32 centerDot = axis.Dot( cyl.center );
	f32 offsetDot = axis.Dot( sOffset );

	minPoint = cyl.center - sOffset;
	maxPoint = cyl.center + sOffset;
	minDot = centerDot - offsetDot;
	maxDot = centerDot + offsetDot;
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycVec3& pt, const ycCollisionSphere& sphB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( pt, sphB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycSphere& sphA, const ycSphere& sphB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sphA, sphB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycSphere& sphA, const ycCollisionSphere& sphB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sphA, sphB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycMtx4& box, const ycTri& tri, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( box, tri, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycTri& tri, const ycMtx4& box, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( tri, box, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycMtx4& boxA, const ycCollisionBox& boxB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( boxA, boxB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycMtx4& box, const ycCollisionSphere& sph, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( box, sph, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycMtx4& box, const ycCollisionCapsule& cap, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( box, cap, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionSphere& sphA, const ycCollisionSphere& sphB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sphA, sphB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycSphere& sph, const ycTri& tri, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, tri, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycVec3& pt, const ycCollisionCapsule& cap, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( pt, cap, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycSphere& sph, const ycCollisionCapsule& cap, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, cap, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionSphere& sph, const ycTri& tri, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, tri, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionSphere& sph, const ycCollisionCapsule& cap, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, cap, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCapsule& capA, const ycTri& tri, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( capA, tri, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionSphere& sphB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( capA, sphB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionCapsule& capB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( capA, capB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCapsule& capA, const ycCollisionBox& boxB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( capA, boxB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycTri& tri, const ycCollisionSphere& sph, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( tri, sph, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionBox& box, const ycCollisionSphere& sph, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( box, sph, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionSphere& sph, const ycCollisionBox& box, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, box, sepPrecision, allowBackface, weldingOn );
}
	
ycCollisionResult ycCollisionSAT::SepAxisTest( const ycSphere& sp, const ycCollisionBox& box, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sp, box, sepPrecision, allowBackface, weldingOn );
}
	
ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cyl, const ycTri& tri, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cyl, tri, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cyl, const ycCollisionSphere& sph, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cyl, sph, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cyl, const ycCollisionCapsule& cap, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cyl, cap, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cyl, const ycSphere& sph, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cyl, sph, sepPrecision, allowBackface, weldingOn );
}
	
ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cylA, const ycCollisionCylinder& cylB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cylA, cylB, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cylA, const ycCollisionBox& boxB, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cylA, boxB, sepPrecision, allowBackface, weldingOn );
}
	
ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCylinder& cyl, const ycMtx4& box, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cyl, box, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionSphere& sph, const ycCollisionCylinder& cyl, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, cyl, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycCollisionCapsule& cap, const ycCollisionCylinder& cyl, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( cap, cyl, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycSphere& sph, const ycCollisionCylinder& cyl, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( sph, cyl, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycTri& tri, const ycCollisionCylinder& cyl, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( tri, cyl, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycMtx4& box, const ycCollisionCylinder& cyl, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( box, cyl, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycVec3& pt, const ycCollisionCylinder& cyl, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( pt, cyl, sepPrecision, allowBackface, weldingOn );
}

ycCollisionResult ycCollisionSAT::SepAxisTest( const ycTri& tri, const ycCollisionCapsule& cap, f32 sepPrecision, ycCollisionSAT::BackfaceBehavior allowBackface, bool weldingOn )
{
	return SepAxisTestGeneric( tri, cap, sepPrecision, allowBackface, weldingOn );
}
	
bool ycCollisionSAT::SatIterativeSweep( const ycSphere& sphA, const ycSphere& sphB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, sphB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycSphere& sphA, const ycCollisionSphere& sphB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, sphB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycMtx4& box, const ycTri& tri, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( box, tri, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycMtx4& boxA, const ycCollisionBox& boxB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( boxA, boxB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycMtx4& box, const ycCollisionSphere& sph, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( box, sph, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycMtx4& box, const ycCollisionCapsule& cap, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( box, cap, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycAABB& box, const ycTri& tri, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( box, tri, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycSphere& sph, const ycTri& tri, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sph, tri, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycSphere& sph, const ycCollisionCapsule& cap, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sph, cap, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycSphere& sph, const ycCollisionBox& box, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sph, box, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionSphere& sphA, const ycTri& tri, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, tri, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionSphere& sphB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, sphB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionCapsule& capB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, capB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionBox& boxB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, boxB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionSphere& sphA, const ycCollisionCylinder& cylB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, cylB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionSphere& sphA, const ycSphere& sphB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sphA, sphB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCapsule& capA, const ycTri& tri, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( capA, tri, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionSphere& sphB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( capA, sphB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionCapsule& capB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( capA, capB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCapsule& capA, const ycCollisionBox& boxB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( capA, boxB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cyl, const ycTri& tri, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cyl, tri, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cyl, const ycCollisionSphere& sph, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cyl, sph, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cyl, const ycCollisionCapsule& cap, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cyl, cap, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cyl, const ycSphere& sph, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cyl, sph, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}
	
bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cylA, const ycCollisionCylinder& cylB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cylA, cylB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cylA, const ycCollisionBox& boxB, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cylA, boxB, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}
	
bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCylinder& cyl, const ycMtx4& box, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cyl, box, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycCollisionCapsule& cap, const ycCollisionCylinder& cyl, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( cap, cyl, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycSphere& sph, const ycCollisionCylinder& cyl, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( sph, cyl, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

bool ycCollisionSAT::SatIterativeSweep( const ycMtx4& box, const ycCollisionCylinder& cyl, const ycVec3& dir, f32 dist, ycCollisionResult* contactPoint, ycCollisionOptions::BackfaceBehavior backface, bool weldingOn, f32 kPrecision )
{
	return IterativeSweepGeneric( box, cyl, dir, dist, contactPoint, backface, weldingOn, kPrecision );
}

#ifdef YC_TEST
#include "ycTest.h"
#include "ycTransform.h"

YC_BEGIN_TEST( ycSepAxis_CapturedTests )
{
#define FLIT( a, b ) b##f

	{
		ycCollisionCylinder sA = { 
			ycVec3( FLIT( 77.708801f, 0x1.36d5d00000000p+6 ), FLIT( -35.342884f, -0x1.1abe3a0000000p+5 ), FLIT( 9.201456f, 0x1.2672540000000p+3 ) ),
			ycVec3( 0.0f, 1.0f, 0.0f ),
			FLIT( 0.650000f, 0x1.4ccccc0000000p-1 ),
			FLIT( 1.296000f, 0x1.4bc6a80000000p+0 )
		};
		ycTri sB( 
			ycVec3( FLIT( 77.587570f, 0x1.3659ac0000000p+6 ), FLIT( -35.990891f, -0x1.1fed580000000p+5 ), FLIT( 10.000000f, 0x1.4000000000000p+3 ) ),
			ycVec3( FLIT( 78.087570f, 0x1.3859ac0000000p+6 ), FLIT( -35.990891f, -0x1.1fed580000000p+5 ), FLIT( 9.500000f, 0x1.3000000000000p+3 ) ),
			ycVec3( FLIT( 80.087570f, 0x1.4059ac0000000p+6 ), FLIT( -35.990891f, -0x1.1fed580000000p+5 ), FLIT( 9.500000f, 0x1.3000000000000p+3 ) )
		);
		f32 sepPrecision = 0.0f;

		ycCollisionResult res = SepAxisTestGeneric( sA, sB, sepPrecision, ycCollisionOptions::kBackface_ResolveOutward, false );

		// small overlap with edges should not result in a large kick-out
		YC_TEST_CHECK( res.dir.y > -0.99f );
	}

	// very ill-conditioned, will cause the GQT solve to assert/produce NaNs if not handled
	{
		ycCollisionCapsule sA = { 
				ycMtx4( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f ),
				ycMtx4( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f ),
				{ ycVec3( FLIT( -250.693085f, -0x1.f562dc0000000p+7 ), FLIT( 105.000099f, 0x1.a4001a0000000p+6 ), FLIT( -32.788734f, -0x1.064f540000000p+5 ) ), ycVec3( FLIT( -241.051422f, -0x1.e21a540000000p+7 ), FLIT( 113.984734f, 0x1.c7f05e0000000p+6 ), FLIT( -20.998878f, -0x1.4ffb680000000p+4 ) ) },
				FLIT( 0.100000f, 0x1.99999a0000000p-4 )
		};
		ycCollisionSphere sB = { 
				ycMtx4( FLIT( 0.326531f, 0x1.4e5e1c0000000p-2 ), 0.0f, FLIT( -0.309641f, -0x1.3d128c0000000p-2 ), 0.0f, 0.0f, FLIT( 0.800000f, 0x1.99999a0000000p-1 ), 0.0f, 0.0f, FLIT( 0.309641f, 0x1.3d128c0000000p-2 ), 0.0f, FLIT( 0.326531f, 0x1.4e5e1c0000000p-2 ), 0.0f, FLIT( -250.693085f, -0x1.f562dc0000000p+7 ), FLIT( 105.800102f, 0x1.a7334e0000000p+6 ), FLIT( -32.788734f, -0x1.064f540000000p+5 ), 1.0f ),
				ycMtx4( FLIT( 1.612498f, 0x1.9cccb00000000p+0 ), 0.0f, FLIT( 1.529092f, 0x1.87728e0000000p+0 ), 0.0f, 0.0f, FLIT( 1.250000f, 0x1.4000000000000p+0 ), 0.0f, 0.0f, FLIT( -1.529092f, -0x1.87728e0000000p+0 ), 0.0f, FLIT( 1.612498f, 0x1.9cccb00000000p+0 ), 0.0f, FLIT( 354.105133f, 0x1.621aea0000000p+8 ), FLIT( -132.250122f, -0x1.0880100000000p+7 ), FLIT( 436.204529f, 0x1.b4345c0000000p+8 ), 1.0f ),
				1.0f
		};
		f32 sepPrecision = 0.0f;

		ycCollisionResult res = SepAxisTestGeneric( sA, sB, sepPrecision, ycCollisionOptions::kBackface_ResolveOutward, false );
	}

	YC_TEST_PASS( "Captured ycSepAxis Tests" );
}

#endif

