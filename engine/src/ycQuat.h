#pragma once

#include "ycMath.h"
#include "ycStd.h"
#include "ycVec3.h"
#include "ycVec4.h"
#include "ycMtx4.h"

#include "ycPushDebugOpt.inc"

struct ycQuat //%
{
yceditable:

	//! the data!
	union
	{
		struct { f32 x, y, z, w; };
		ycVec4 v;
		f32 a[ 4 ];
	};

public:

	//! default ctor
	ycQuat() = default;

	//! sets x, y, z, and w
	constexpr inline explicit ycQuat( const f32 _x, const f32 _y, const f32 _z, const f32 _w );

	//! Axis must be a unit vector, angle is in radians
	inline ycQuat( const ycVec3& axis, const f32 angle );

	//! ctor for quat from rotation matrix
	explicit ycQuat( const ycMtx4& mat );

	//! Constructor from Euler angles: XYZ concat order
	//! Euler angles are represented as counter-clockwise rotations (when in RH coordinates)
	//! about each axis in radians. [x,y,z] = [pitch,yaw,roll]
	//! Operations are concatenated in the order Roll, Pitch, then finally Yaw (to match
	//! the standard aeroplane model)
	inline ycQuat( const f32 angleX, const f32 angleY, const f32 angleZ );

	//! v1 and v2 are unit vectors
	//! ctor for quat that applied to v1 yields v2
	inline ycQuat( const ycVec3& v1, const ycVec3& v2 );

	//! length
	inline f32 LengthSq() const;
	inline f32 MagSq() const { return LengthSq(); }
	inline f32 Length() const;
	inline f32 Mag() const { return Length(); }

	//! normalization
	/*! Sets all components to zero for zero-length vectors */
	inline void Normalize();
	inline ycQuat Normalized() const;
	inline ycQuat GetNormalized() const;

	// basic operations
	inline f32 Dot( const ycQuat& other ) const;
	inline ycQuat GetConjugate() const;
	inline void Conjugate();
	//! Quaternion must be normalized
	inline ycQuat GetInverse() const;
	//! Quaternion must be normalized
	inline void Invert();
	//! Check if quaternion is normalized
	inline bool IsNormalized() const;
	//! Check if quaternion is normalized, but more lenient
	inline bool IsMostlyNormalized() const;
	//! Quaternion must be normalized
	inline bool IsIdentity( f32 epsilon = YC_F32_EPSILON ) const;

	inline bool IsEqual( const ycQuat& q, f32 tolerance ) const;

	// rotation operations
	//! Quaternion must be normalized
	inline ycVec3 Rotate( const ycVec3& p ) const;
	//! Quaternion must be normalized
	inline ycVec4 Rotate( const ycVec4& p ) const;
	//! Unrotated vector, Quaternion must be normalized
	inline ycVec3 RotateInverse( const ycVec3& p ) const;
	//! Unrotated vector, Quaternion must be normalized
	inline ycVec4 RotateInverse( const ycVec4& p ) const;

	// rotations on the principal axes	
	//! Quaternion must be normalized
	inline ycVec3 GetForwardAxis() const;
	//! Quaternion must be normalized
	inline ycVec3 GetUpAxis() const;
	//! Quaternion must be normalized
	inline ycVec3 GetRightAxis() const;
	//! Quaternion must be normalized
	inline ycVec3 GetXAxis() const;
	//! Quaternion must be normalized
	inline ycVec3 GetYAxis() const;
	//! Quaternion must be normalized
	inline ycVec3 GetZAxis() const;

	// interpolation
	inline ycQuat GetSlerp( const ycQuat& other, const f32 t ) const;
	inline void      Slerp( const ycQuat& other, const f32 t );
	inline ycQuat  GetLerp( const ycQuat& other, const f32 t ) const;
	inline void       Lerp( const ycQuat& other, const f32 t );


	/*! @Note: Cycling between GetQuat and GetAxis() & GetAngle() will accumulate error
	 *         and bring the quat out of acceptable normalization bounds.
	 */
	inline ycVec3 GetAxis() const;
	inline f32 GetAngle() const;
	//! mainly a helper for 2D. to attain the same angle out as goes into ycQuat( const ycVec3& axis, const f32 angle );
	inline f32 GetAngleAxis( s32 posAxis = 2 ) const;

	inline f32 ExtractYaw() const;
	inline ycQuat GetSwingTwistDeltaTo( ycQuat q ) const;


	//! Quaternion must be normalized
	ycMtx4 GetAsMtx4() const;
	static inline ycQuat GetQuat( const ycMtx4& mat );

	static ycQuat LookAt( const ycVec3& fwd, const ycVec3& up );

	/*! @Note: Cycling between GetQuat and GetAxis() GetAngle() will accumulate error
	 *         and bring the quat out of acceptable normalization bounds.
	 */
	 //! @Assumption: axis is normalized, angle is in radians
	static inline ycQuat GetQuat( const ycVec3& axis, const f32 angle );

	//! returns the quaternion that gives the shortest rotation 
	//! from v1 to v2. Input vectors do not need to be normalized.
	enum
	{
		kGetQuatResult_Okay,
		kGetQuatResult_Same,
		kGetQuatResult_Opposite
	};
	static inline ycQuat GetQuat( const ycVec3& v1, const ycVec3& v2, s32* result = nullptr );

	//! Euler angles are represented as counter-clockwise rotations (when in RH coordinates)
	//! about each axis in radians. [x,y,z] = [pitch,yaw,roll]
	//! Operations are concatenated in the order Roll, Pitch, then finally Yaw (to match
	//! the standard aeroplane model)
	static inline ycQuat GetQuat( f32 angleX, f32 angleY, f32 angleZ );

	static inline ycQuat FromEuler( ycVec3 ang );

	ycVec3 ToEuler() const;

	inline ycVec3 ToAxisAngleVector() const;
	static inline ycQuat FromAxisAngleVector( ycVec3 axisAngle );

	/*! There are two ways to represent the same rotation using quaternions
	 *  The Canonical form is an agreed upon representation standard to remove this ambiguity.
	 *  When comparing two quaternions make sure you are comparing their canonical representations
	 */
	 //! Returns the conanical form for ycQuat comparison
	inline ycQuat GetInCanonicalForm() const;

	//! non-normalized multiply
	inline ycQuat Multiply( const ycQuat& rhs ) const;

	//! Quaternion must be normalized
	inline ycQuat  operator *  ( const ycQuat& rhs ) const;
	inline ycQuat& operator *= ( const ycQuat& rhs );
	inline ycQuat  operator -  ( const ycQuat& rhs ) const;
	inline ycQuat& operator -= ( const ycQuat& rhs );
	inline ycQuat  operator +  ( const ycQuat& rhs ) const;
	inline ycQuat& operator += ( const ycQuat& rhs );

	//! Quaternion must be normalized
	inline ycVec3  operator *  ( const ycVec3& vec ) const;
	//! Quaternion must be normalized
	inline ycVec4  operator *  ( const ycVec4& vec ) const;

	// operators: scaling
	inline friend ycQuat  operator*  ( const ycQuat& q, const f32 scale );
	inline friend ycQuat  operator*  ( const f32 scale, const ycQuat& q );
	inline friend ycQuat  operator/  ( const ycQuat& q, const f32 scale );
	inline friend ycQuat  operator/  ( const f32 scale, const ycQuat& q );
	inline ycQuat& operator *= ( const f32 scale );
	inline ycQuat& operator /= ( const f32 scale );

	// operators: unary sign operators for sake of completness
	inline ycQuat  operator -  () const;
	inline ycQuat  operator +  () const;

	// direct comparison of values
	inline bool operator == ( const ycQuat& rhs ) const; 
	inline bool operator != ( const ycQuat& rhs ) const;

	//! utility constants
	inline static constexpr ycQuat ZERO() { return ycQuat( 0.0f, 0.0f, 0.0f, 0.0f ); }
	inline static constexpr ycQuat IDENTITY() { return ycQuat( 0.0f, 0.0f, 0.0f, 1.0f ); }
};

#if 0
#define ASSERT_NORMALIZED( q ) ycAssert( (q).IsNormalized() )
#else
#define ASSERT_NORMALIZED( q ) ycAssert( (q).IsMostlyNormalized() )
#endif

#define ASSERT_NONZERO( q ) ycAssert( (q).LengthSq() > 0.0f )

constexpr ycQuat::ycQuat( const f32 _x, const f32 _y, const f32 _z, const f32 _w )
	: x( _x )
	, y( _y )
	, z( _z )
	, w( _w )
{
}


f32 ycQuat::Dot( const ycQuat& other ) const
{
	const ycQuat tmp( x*other.x, y*other.y, z*other.z, w*other.w );
	return ( tmp.x+tmp.y ) + ( tmp.z+tmp.w );
}


f32 ycQuat::LengthSq() const
{
	return x*x + y*y + z*z + w*w;
}

f32 ycQuat::Length() const
{
	return ycSqrt( LengthSq() );
}


void ycQuat::Normalize()
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { /*assume all components are already zero*/ return; }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	x *= invLengthSq;
	y *= invLengthSq;
	z *= invLengthSq;
	w *= invLengthSq;
}


ycQuat ycQuat::Normalized() const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { return ycQuat::ZERO(); }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycQuat( x * invLengthSq,
				   y * invLengthSq,
				   z * invLengthSq,
				   w * invLengthSq
				   );
}


ycQuat ycQuat::GetNormalized() const
{
	const f32 lengthSq = LengthSq();
	if( YC_UNLIKELY( lengthSq == 0 ) ) { return ycQuat::ZERO(); }
	const f32 invLengthSq = ycInvSqrt( lengthSq );
	return ycQuat( x * invLengthSq,
				   y * invLengthSq,
				   z * invLengthSq,
				   w * invLengthSq
				   );
}


ycQuat::ycQuat( const ycVec3& axis, const f32 angle )
{
	( *this ) = GetQuat( axis, angle );
}


ycQuat::ycQuat( const f32 angleX, const f32 angleY, const f32 angleZ )
{
	( *this ) = GetQuat( angleX, angleY, angleZ );
}


ycQuat::ycQuat( const ycVec3& v1, const ycVec3& v2 )
{
	// MWK: these were firing spuriously :|
	#if 0
	ycAssert( v1.IsNormalized() );
	ycAssert( v2.IsNormalized() );
	#endif
	( *this ) = GetQuat( v1, v2 );
}


ycQuat ycQuat::GetConjugate() const
{
	return ycQuat( -x, -y, -z, w );
}

void   ycQuat::Conjugate()
{
	x = -x;
	y = -y;
	z = -z;
}


ycQuat ycQuat::GetInverse() const
{
	ASSERT_NORMALIZED( *this );
	return ycQuat( -x, -y, -z, w );
}


void   ycQuat::Invert()
{
	ASSERT_NORMALIZED( *this );
	x = -x;
	y = -y;
	z = -z;
}


bool   ycQuat::IsNormalized() const
{
	f32 tolerance = 1e-5f; //f32( 8.5e-7 );
	f32 lengthSq = LengthSq();
	return ( lengthSq > ( 1.f - tolerance ) && lengthSq < ( 1.f + tolerance ) );
}

bool   ycQuat::IsMostlyNormalized() const
{
	f32 tolerance = 1e-4f;
	f32 lengthSq = LengthSq();
	return ( lengthSq > ( 1.f - tolerance ) && lengthSq < ( 1.f + tolerance ) );
}

bool ycQuat::IsIdentity( f32 epsilon ) const
{
	if( ycAbs( x ) <= epsilon
		&& ycAbs( y ) <= epsilon
		&& ycAbs( z ) <= epsilon
		&& ycAbs( w-1.0f ) <= epsilon )
	{
		return true;
	}
	return false;
}

bool ycQuat::IsEqual( const ycQuat& q, f32 tolerance ) const
{
	const ycQuat r = Dot( q ) > 0.0f ? q : -q;
	return ycAbs( x - r.x ) < tolerance
		&& ycAbs( y - r.y ) < tolerance
		&& ycAbs( z - r.z ) < tolerance
		&& ycAbs( w - r.w ) < tolerance;
}

ycVec3 ycQuat::Rotate( const ycVec3& p ) const
{
	ASSERT_NORMALIZED( *this ); // should only be rotating with normalized quats
	// Logic taken, with permission from the GLM math library. Thank you  GLM authors!!
	ycVec3 QuatVector( x, y, z );
	ycVec3 uv = QuatVector.Cross( p );
	ycVec3 uuv = QuatVector.Cross( uv );
	return p + ( ( uv * w ) + uuv ) * 2.f;
}


ycVec4 ycQuat::Rotate( const ycVec4& p ) const
{
	ASSERT_NORMALIZED( *this );
	return ycVec4( ( Rotate( ycVec3( p.x, p.y, p.z ) ) ), p.w );
}


ycVec3 ycQuat::RotateInverse( const ycVec3& p ) const
{
	ASSERT_NORMALIZED( *this );
	return GetInverse().Rotate( p );
}


ycVec4 ycQuat::RotateInverse( const ycVec4& p ) const
{
	ASSERT_NORMALIZED( *this );
	return ycVec4( ( RotateInverse( ycVec3( p.x, p.y, p.z ) ) ), p.w );
}


ycQuat ycQuat::GetSlerp( const ycQuat& other, const f32 t ) const
{
	const f32 fltEpsilon = 1e-6f;
	ycQuat slerpedQuat;
	ycQuat startQuat = *this; //needed in case we have to negate the start quat to interp over shortest path
	f32 cosTheta = Dot( other );

	if( YC_UNLIKELY( cosTheta < 0.f ) )// more like less likely. for this case
	{
		// negate start quat so takes shortest path
		// Reasoning covered on p.208 of essential mathematics for games & intereactive apps.
		startQuat = -startQuat;
		cosTheta = -cosTheta;
	}
	if( cosTheta < ( 1.f-fltEpsilon ) )
	{
		f32 theta = ycArcCos( cosTheta );
		slerpedQuat = startQuat*ycSin( ( 1 - t )*theta );
		slerpedQuat += other*ycSin( t*theta );
		slerpedQuat /= ycSin( theta );
	}
	else // angle between quats is so small just use lerp
	{
		slerpedQuat = startQuat.GetLerp( other, t );
	}
	slerpedQuat.Normalize();

	return slerpedQuat;
}


void ycQuat::Slerp( const ycQuat& other, const f32 t )
{
	( *this ) = GetSlerp( other, t );
}


ycQuat ycQuat::GetLerp( const ycQuat& other, const f32 t ) const
{
	ycQuat lerpedQuat = ( ( *this )*( 1-t ) ) + ( other*t );
	return lerpedQuat.Normalized();
}


void ycQuat::Lerp( const ycQuat& other, const f32 t )
{
	( *this ) = ( *this ).GetLerp( other, t );
}


ycVec3 ycQuat::GetForwardAxis() const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( ycVec3::FORWARD() );
}


ycVec3 ycQuat::GetUpAxis() const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( ycVec3::UP() );
}


ycVec3 ycQuat::GetRightAxis() const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( ycVec3::RIGHT() );
}


ycVec3 ycQuat::GetXAxis() const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( ycVec3::XAXIS() );
}


ycVec3 ycQuat::GetYAxis() const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( ycVec3::YAXIS() );
}


ycVec3 ycQuat::GetZAxis() const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( ycVec3::ZAXIS() );
}


ycVec3 ycQuat::GetAxis() const
{
	ASSERT_NORMALIZED( *this );
	f32 invLengthOfV = 1.f / ycSqrt( 1 - w*w );
	return ycVec3( x*invLengthOfV, y*invLengthOfV, z*invLengthOfV );
}


f32 ycQuat::GetAngle() const
{
	return 2 * ycArcCos( w );
}

f32 ycQuat::GetAngleAxis( s32 posAxis ) const
{
	f32 angle = GetAngle();
	if( a[posAxis] < 0.0f )
	{
		return -angle;
	}
	return angle;
}

f32 ycQuat::ExtractYaw() const
{
	//return 2.0f * ycArcSin( ycQuat( 0, y, 0, w ).Normalized().y );
	const ycVec3 fwd = Rotate( ycVec3::FORWARD() );
	return ycArcTan2( fwd.x, fwd.z );

}

/*static*/ ycQuat ycQuat::GetQuat( const ycMtx4& mat )
{
	ycQuat retVal;
	// Taken & adapted, with permission from the GLM library.
	// Many thanks to the GLM Authors!!!
	f32 d[4] = {
		mat.x.x + mat.y.y + mat.z.z,
		mat.x.x - mat.y.y - mat.z.z,
		mat.y.y - mat.z.z - mat.x.x,
		mat.z.z - mat.x.x - mat.y.y
	};
	
	s32 biggestIndex = 0;
	if( d[ 1 ] > d[ biggestIndex ] )
		biggestIndex = 1;
	if( d[ 2 ] > d[ biggestIndex ] )
		biggestIndex = 2;
	if( d[ 3 ] > d[ biggestIndex ] )
		biggestIndex = 3;

	switch( biggestIndex )
	{
	case 0:
		retVal.w = ( d[ biggestIndex ] + 1 );
		retVal.x = ( mat.y.z - mat.z.y );
		retVal.y = ( mat.z.x - mat.x.z );
		retVal.z = ( mat.x.y - mat.y.x );
		break;
	case 1:
		retVal.w = ( mat.y.z - mat.z.y );
		retVal.x = ( d[ biggestIndex ] + 1 );
		retVal.y = ( mat.x.y + mat.y.x );
		retVal.z = ( mat.z.x + mat.x.z );
		break;
	case 2:
		retVal.w = ( mat.z.x - mat.x.z );
		retVal.x = ( mat.x.y + mat.y.x );
		retVal.y = ( d[ biggestIndex ] + 1 );
		retVal.z = ( mat.y.z + mat.z.y );
		break;
	case 3:
		retVal.w = ( mat.x.y - mat.y.x );
		retVal.x = ( mat.z.x + mat.x.z );
		retVal.y = ( mat.y.z + mat.z.y );
		retVal.z = ( d[ biggestIndex ] + 1 );
		break;
	YC_NO_DEFAULT_ASSERT
	}
	return retVal.Normalized();
}

ycQuat ycQuat::GetSwingTwistDeltaTo( ycQuat q ) const
{
	ASSERT_NORMALIZED( *this );
	ASSERT_NORMALIZED( q );
	const ycQuat swing( GetZAxis(), q.GetZAxis() );
	const ycQuat swung = ( swing * (*this) );
	const ycQuat twist( swung.GetYAxis(), q.GetYAxis() );
	return twist * swing;
}

/*static*/ ycQuat ycQuat::GetQuat( const ycVec3& axis, const f32 angle )
{
	if( angle == 0.0f ) { return IDENTITY(); }

	ycAssert( axis.LengthSq() > 0.0f );
	ycAssert( axis.IsNormalized() );
	ycVec3 result = axis*ycSin( angle * 0.5f );
	return ycQuat( result.x, result.y, result.z, ycCos( angle * 0.5f ) );
}


/*static*/ ycQuat ycQuat::GetQuat( const ycVec3& v1, const ycVec3& v2, s32* result )
{
	ycVec3 c = v1.Cross( v2 );
	f32 d = ycSqrt( v1.LengthSq() * v2.LengthSq() );

	ycQuat q( c.x, c.y, c.z, d+v1.Dot( v2 ) );
	if( q.w < 0.0000001f )
	{
		// a) Vectors are equal.
		if( v1.LengthSq() < 0.000001f )
		{
			if( result )
			{
				*result = kGetQuatResult_Same;
			}
			return ycQuat::IDENTITY();
		}
		// b) Vectors are at 180 deg. There's no single defined 
		// shortest path, so pick any old axis to rotate through.
		ycVec3 axis;
		v1.Normalized().OrthoNormalBasis( &axis, nullptr );
		if( result )
		{
			*result = kGetQuatResult_Opposite;
		}
		return ycQuat( axis.x, axis.y, axis.z, 0.0f );
	}
	else if( result )
	{
		*result = kGetQuatResult_Okay;
	}

	return q.Normalized();
}


/*static*/ ycQuat ycQuat::GetQuat( f32 angleX, f32 angleY, f32 angleZ )
{
	angleX *= 0.5f;
	angleY *= 0.5f;
	angleZ *= 0.5f;

	f32 cx = ycCos( angleX );
	f32 sx = ycSin( angleX );
	f32 cy = ycCos( angleY );
	f32 sy = ycSin( angleY );
	f32 cz = ycCos( angleZ );
	f32 sz = ycSin( angleZ );

	return ycQuat(
		sx*cz*cy + sy*sz*cx, 
		sy*cx*cz - sz*sx*cy, 
		sz*cy*cx - sx*sy*cz,
		cz*cx*cy + sx*sz*sy );
}

/*static*/ ycQuat ycQuat::FromEuler( ycVec3 ang )
{
	return GetQuat( ang.x, ang.y, ang.z );
}

ycVec3 ycQuat::ToAxisAngleVector() const
{
	ycVec3 angVec( x, y, z );
	angVec.Normalize();
	f32 angle = ycArcCos( ycClamp( w, -1.0f, 1.0f ) ) * 2.0f;
	return angVec * angle;
}

/*static*/ ycQuat ycQuat::FromAxisAngleVector( ycVec3 axisAngle )
{
	f32 angle = axisAngle.Normalize();
	return (angle > 0.00000001f)? ycQuat(axisAngle, angle) : ycQuat::IDENTITY();
}

ycQuat ycQuat::GetInCanonicalForm() const
{
	if( w < 0.f )
	{
		return -( *this );
	}
	return ( *this );
}


ycQuat ycQuat::Multiply( const ycQuat& rhs ) const
{
	f32 w1 = rhs.w;
	f32 w2 = w;
	ycVec3 v1 = ycVec3( rhs.x, rhs.y, rhs.z );
	ycVec3 v2 = ycVec3( x, y, z );

	// equation from Essential Mathematics for Games adapted to code.
	f32 new_w = w1*w2 - v1.Dot( v2 );
	ycVec3 result = v2*w1 + v1*w2 + v2.Cross( v1 );

	return ycQuat( result.x, result.y, result.z, new_w );
}

ycQuat  ycQuat::operator*  ( const ycQuat& rhs ) const
{
	ASSERT_NONZERO( *this );
	ASSERT_NONZERO( rhs );
	ASSERT_NORMALIZED( *this );
	ASSERT_NORMALIZED( rhs );
	return Multiply( rhs );
}


ycQuat& ycQuat::operator*= ( const ycQuat& rhs )
{
	ASSERT_NONZERO( *this );
	ASSERT_NONZERO( rhs );
	ASSERT_NORMALIZED( *this );
	ASSERT_NORMALIZED( rhs );
	f32 w1 = rhs.w;
	f32 w2 = w;
	ycVec3 v1 = ycVec3( rhs.x, rhs.y, rhs.z );
	ycVec3 v2 = ycVec3( x, y, z );
	// equation from Essential Mathematics for Games adapted to code.
	ycVec3 result = v2*w1 + v1*w2 + v2.Cross( v1 );
	w = w1*w2 - v1.Dot( v2 );
	x = result.x;
	y = result.y;
	z = result.z;
	return ( *this );
}


ycQuat  ycQuat::operator-  ( const ycQuat& rhs ) const
{
	return ycQuat( ( x - rhs.x ), ( y - rhs.y ), ( z - rhs.z ), ( w - rhs.w ) );
}


ycQuat& ycQuat::operator-= ( const ycQuat& rhs )
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
	return ( *this );
}


ycQuat  ycQuat::operator+  ( const ycQuat& rhs ) const
{
	return ycQuat( ( x + rhs.x ), ( y + rhs.y ), ( z + rhs.z ), ( w + rhs.w ) );
}


ycQuat& ycQuat::operator+= ( const ycQuat& rhs )
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
	return ( *this );
}


ycVec3  ycQuat::operator*  ( const ycVec3& vec ) const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( vec );
}


ycVec4  ycQuat::operator*  ( const ycVec4& vec ) const
{
	ASSERT_NORMALIZED( *this );
	return Rotate( vec );
}


ycQuat  operator*  ( const ycQuat& q, const f32 scale )
{
	return ycQuat( ( q.x*scale ), ( q.y*scale ), ( q.z*scale ), ( q.w*scale ) );
}


ycQuat  operator*  ( const f32 scale, const ycQuat& q )
{
	return ycQuat( ( q.x*scale ), ( q.y*scale ), ( q.z*scale ), ( q.w*scale ) );
}


ycQuat  operator/  ( const ycQuat& q, const f32 scale )
{
	f32 recip = 1.f / scale;
	return ycQuat( ( q.x*recip ), ( q.y*recip ), ( q.z*recip ), ( q.w*recip ) );
}


ycQuat  operator/  ( const f32 scale, const ycQuat& q )
{
	f32 recip = 1.f / scale;
	return ycQuat( ( q.x*recip ), ( q.y*recip ), ( q.z*recip ), ( q.w*recip ) );
}

ycQuat& ycQuat::operator*= ( const f32 scale )
{
	x *= scale;
	y *= scale;
	z *= scale;
	w *= scale;
	return ( *this );
}


ycQuat& ycQuat::operator/= ( const f32 scale )
{
	f32 recip = 1.f / scale;
	x *= recip;
	y *= recip;
	z *= recip;
	w *= recip;
	return ( *this );
}


ycQuat  ycQuat::operator-  () const
{
	return ycQuat( ( -x ), ( -y ), ( -z ), ( -w ) );
}


ycQuat  ycQuat::operator+  () const
{
	return ( *this );
}

inline bool ycQuat::operator == ( const ycQuat& rhs ) const
{
	return rhs.x == x && rhs.y == y && rhs.z == z && rhs.w == w;
}

inline bool ycQuat::operator != ( const ycQuat& rhs ) const
{
	return rhs.x != x || rhs.y != y || rhs.z != z || rhs.w != w;
}

#include "ycPopDebugOpt.inc"
