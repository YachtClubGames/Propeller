#pragma once

#include "ycShape.h"
#include "ycPlane.h"
#include "ycVec3.h"

struct ycAABB;
struct ycMtx4;

struct ycFrustum : public ycShape //%
{
	enum
	{
		kPlane_Near = 0,
		kPlane_Far,
		kPlane_Left,
		kPlane_Right,
		kPlane_Top,
		kPlane_Bottom,

		kPlane_Count
	};

yceditable:

	union
	{
		struct { ycPlane a, b, c, d, e, f; };
		#ifdef _INC_WINDOWS // does not play nicely with windows.h
			#pragma push_macro("near")
			#pragma push_macro("far")
			#undef near
			#undef far
		#endif
		struct { ycPlane near, far, left, right, top, bottom; };
		#ifdef _INC_WINDOWS
			#pragma pop_macro("near")
			#pragma pop_macro("far")
		#endif
		ycPlane planes[ ycFrustum::kPlane_Count ];
	};

public:

	//! default ctor
	ycFrustum() = default;
	constexpr inline ycFrustum( const ycPlane& a, const ycPlane& b, const ycPlane& c, const ycPlane& d, const ycPlane& e, const ycPlane& f );
	ycFrustum( const ycMtx4& mtx ); //!< construct a frustum from a projection (or proj*view) matrix

	//! Intersects the left/right and top/bottom parts of the plane to find the starting view position of frustum, then returns closer option
	// ie where the eye might be for the camera
	ycVec3 GetNearViewPos() const;

	//! get 8 corners of the frustum
	ycVec3 GetCorner( s32 index ) const;
	//! vertices for tris of frustum
	u32 ToVerts( ycVec3 * pts36 ) const;

	//! Grows the frustum to encompass the shape
	void Expand( const ycVec3& pos );
	void Expand( const ycSphere& sphere );

	//! Detect if point is inside frustum
	bool Contains( const ycVec3 & pt ) const;

	//! Detect if point with buffer radius is inside frustum (can be negative radius unlike sphere)
	bool Contains( const ycVec3 & pt, f32 radius ) const;

	bool Intersects( const ycAABB& box ) const; //! returns true if the box intersects the planes, or is entirely contained by them
	bool Intersects( const ycSphere& sphere ) const;

	constexpr inline static ycFrustum ZERO()  { return ycFrustum( ycPlane::ZERO(), ycPlane::ZERO(), ycPlane::ZERO(), ycPlane::ZERO(), ycPlane::ZERO(), ycPlane::ZERO() ); }
};

constexpr inline ycFrustum::ycFrustum( const ycPlane& _a, const ycPlane& _b, const ycPlane& _c, const ycPlane& _d, const ycPlane& _e, const ycPlane& _f )
	: a( _a )
	, b( _b )
	, c( _c )
	, d( _d )
	, e( _e )
	, f( _f )
{
}
