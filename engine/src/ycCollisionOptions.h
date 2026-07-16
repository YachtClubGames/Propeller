#pragma once

#include "ycCompilerConfig.h"

enum ycCollisionMask : u64;

struct ycCollisionOptions
{
	enum BackfaceBehavior : u8
	{
		kBackface_Ignore               = 0, // A backfacing tri does not result in any collision reported.
		kBackface_ResolveAsDoubleSided = 1, // A backfacing tri will have its normal reversed - i.e. treated as double-sided
		kBackface_ResolveOutward       = 2  // A backfacing tri will return a collision depth that moves the penetrating object to the front 
	};

	ycCollisionMask mask = (ycCollisionMask)0xFFFFFFFFFFFFFFFF;
	bool useMask = true;
	bool welding = true;
	bool singleContactPerMesh = false;  // returns only one contact NOT NECESSARILY THE CLOSEST to save time if you only care about a contact existing
	BackfaceBehavior backfaceBehavior = kBackface_ResolveOutward;

	// Will return not only overlapping shapes, but also shapes separated by this distance or less
	//  (Such results will have negative depth)
	// Currently only supported in Collide tests, not sweeps
	f32 extraRadius = 0.0f;

	ycVec3 idealUprightAxis = ycVec3::YAXIS();

	ycCollisionOptions() {}
	ycCollisionOptions( ycCollisionMask m ) : mask( m ) {}
};

struct ycCollisionSweepOptions : public ycCollisionOptions
{
	enum SweepResultsBehavior : u8
	{
		kFirstContactsOnly       = 0, // Return only the earliest hits from the sweep - possibly multiple hits if they happen at the same distance
		kFirstContactsAfterStart = 1, // Same, but ignore any initial contacts the sweep is moving away from anyway
		kAllContacts             = 2, // Return all results from the sweep
		kAllContactsAfterStart   = 3, // Return all contacts, except any initial contacts the sweep is moving away from
	};

	SweepResultsBehavior resultBehavior = kFirstContactsOnly;

	ycCollisionSweepOptions()									{ backfaceBehavior = kBackface_Ignore; }
	ycCollisionSweepOptions( ycCollisionMask m ) : ycCollisionOptions( m ) { backfaceBehavior = kBackface_Ignore; }

	ycCollisionSweepOptions& Welding( bool w ) { welding = w; return *this; }
	ycCollisionSweepOptions& BackfaceBehavior( BackfaceBehavior bb ) { backfaceBehavior = bb; return *this; }
	ycCollisionSweepOptions& ExtraRadius( f32 er ) { extraRadius = er; return *this; }
	ycCollisionSweepOptions& ResultsBehavior( SweepResultsBehavior rb ) { resultBehavior = rb; return *this; }
	ycCollisionSweepOptions& AllContacts() { resultBehavior = kAllContacts; return *this; }

	bool IsAllContacts() const { return resultBehavior >= kAllContacts; }
};
