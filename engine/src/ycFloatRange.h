#pragma once

#include "ycCommon.h"
#include "ycMath.h"

struct ycFloatRange //%
{
yceditable:

	union
	{
		struct { f32 min, max; };
		f32 a[ 2 ];
	};

public:

	ycFloatRange() = default;

	constexpr explicit inline ycFloatRange( f32 min_, f32 max_ ) : min{ min_ }, max{ max_ } {}

	inline bool operator ==( const ycFloatRange& rhs ) const
	{
		return ( rhs.min == min && rhs.max == max );
	}

	inline bool operator !=( const ycFloatRange& rhs ) const
	{
		return !( *this == rhs );
	}

	inline f32 Lerp( const f32 t ) const
	{
		return min + t * GetRange();
	}

	inline f32 GetLerpT( const f32 v ) const
	{
		return ycGetLerpT( min, max, v );
	}

	inline f32 Clamp( const f32 t ) const
	{
		return ycClamp( t, min, max );
	}

	inline void Expand( const ycFloatRange& other )
	{
		if( IsValid() && other.IsValid() )
		{
			max = ycMax( max, other.max );
			min = ycMin( min, other.min );
		}
		else if( other.IsValid() )
		{
			max = other.max;
			min = other.min;
		}
	}

	constexpr inline f32 GetRange() const
	{
		return max - min;
	}

	constexpr inline f32 GetCenter() const
	{
		return ( min + max ) * 0.5f;
	}

	inline bool IsValid() const { return max >= min; }

	inline static constexpr ycFloatRange INVALID() { return ycFloatRange{ 99999.0f, 0.0f }; }
	inline static constexpr ycFloatRange ZERO_TO_ONE() { return ycFloatRange{ 0.0f, 1.0f }; }

	static inline ycFloatRange Lerp( const ycFloatRange& a, const ycFloatRange& b, f32 t )
	{
		return ycFloatRange( a.min + t * ( b.min - a.min ), a.max + t * ( b.max - a.max ) );
	}
};
