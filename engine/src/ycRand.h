#pragma once

#include "ycCommon.h"

/*! \class ycRand
Implements fast platform independent random number generation.

Multiple ycRand objects can be created, functioning as independent RNG contexts.  It is not safe to call functions
across different threads on the same context at the same time.

Implementation details: http://burtleburtle.net/bob/rand/smallprng.html
*/

#define YC_RAND_MAX 0xffffffffu

class ycRand
{
public:

	//! Creates a random number generation context
	/*!
	If no seed is passed, a platform dependent pseudorandom seed will be used
	*/
	ycRand();
	ycRand( const u32 seed );

	//! Explicitly seeds the context
	/*!
	This can be called at any time
	*/
	void SeedRand();
	void Seed( const u32 seed );

	//! Explicitly sets/gets the context. 
	/*!
	This can be called at any time
	*/
	void SetState( const u32 _a, const u32 _b, const u32 _c, const u32 _d );
	void GetState( u32& _a, u32& _b, u32& _c, u32& _d );

	//! Returns a random unsigned integer
	/*!
	Warning: ycRand uses the full unsigned 32-bit range, standard C rand usually has a much smaller range.
	*/
	u32 Rand();

	//! Returns a random unsigned 64 bit integer
	/*!
	While this should produce good random 64 bit numbers, it will reduce the cycle length of this RNG to half.
	If we wind up needing a lot of 64 bit RNG, we should probably create a different generator with more
	internal state.
	*/
	u64 Rand64();

	u64 RandGUID();

	void RandBytes( u8* dst, const ycSize_t numBytes );

	//! Returns a random number in the range [min, max] (inclusive)
	/*!
	The fast version is biased, but is fine for most game use.  Bias can be significantly reduced by using pow2
	ranges, eg [0, 255].
	*/
	u32 RangeFast( const u32 min, const u32 max );

	// HACK TODO FIXME: A version of this that consistently tests better than the fast way
	//u32 Range( const u32 min, const u32 max );

	//! Returns a random floating point number in the range [0.0f,1.0f]
	/*!
	This will give a good distribution, but only ever use ~8% of the potential representations available.
	*/
	f32 RandF();

	/**
	 * Generates a random floating point number between min and max.
	 */
	f32 RandF( f32 min, f32 max );

	/**
	 * Generates a random floating point number between min and max. range is [min, max] inclusive
	 */
	s32 RandI( s32 min, s32 max ); 

	//! Returns a random floating point number in the range ]0.0f,1.0f[
	/*
	This will never return 0, 1, or denormals; it will utilize the entire remaining range of potential values.
	*/
	f32 RandF_Iterative();

protected:

	u32 a, b, c, d;
};

// TODO HACK FIXME: this should not exist here, it is not thread safe. either mutex it or make something explicitely game-thread-specific (what this is probably trying to do)
extern ycRand s_globalRand;
u32 ycRandI();
u64 ycRand64();
u64 ycRandGUID();
f32 ycRandF( f32 min, f32 max ); // range is [min, max] inclusive
s32 ycRandI( s32 min, s32 max ); // range is [min, max] inclusive
bool ycRandB( f32 probability );
void ycRandSeed( u32 seed );
