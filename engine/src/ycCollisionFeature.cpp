#include "ycCollisionFeature.h"

ycCollisionFeature ycCollisionFeature::Vertex( const ycVec3& vert ) 
{
	ycCollisionFeature ff; ff.type = ycCollision::kFeature_Vertex; 
	ff.pts[ 0 ] = ff.pts[ 1 ] = ff.pts[ 2 ] = ff.pts[ 3 ] = vert; 
	return ff; 
}

ycCollisionFeature ycCollisionFeature::Edge( const ycVec3& a, const ycVec3& b, f32 radius ) 
{ 
	ycCollisionFeature ff; ff.type = ycCollision::kFeature_Edge; 
	ff.pts[ 0 ] = ff.pts[ 2 ] = a; ff.pts[ 1 ] = ff.pts[ 3 ] = b; 
	ff.radius = radius;
	return ff;
}

ycCollisionFeature ycCollisionFeature::FaceTri( const ycVec3& a, const ycVec3& b, const ycVec3& c ) 
{ 
	ycCollisionFeature ff; ff.type = ycCollision::kFeature_FaceTri;
	ff.pts[ 0 ] = ff.pts[ 3 ] = a;
	ff.pts[ 1 ] = b; 
	ff.pts[ 2 ] = c; 
	return ff;
}

ycCollisionFeature ycCollisionFeature::FaceQuad( const ycVec3& a, const ycVec3& b, const ycVec3& c, const ycVec3& d ) 
{ 
	ycCollisionFeature ff; ff.type = ycCollision::kFeature_FaceQuad;
	ff.pts[ 0 ] = a; 
	ff.pts[ 1 ] = b;
	ff.pts[ 2 ] = c; 
	ff.pts[ 3 ] = d; 
	return ff;
}

ycCollisionFeature ycCollisionFeature::FaceDisc( const ycVec3& center, f32 radius ) 
{
	ycCollisionFeature ff; ff.type = ycCollision::kFeature_FaceDisc;
	ff.pts[ 0 ] = ff.pts[ 1 ] = ff.pts[ 2 ] = ff.pts[ 3 ] = center; 
	ff.radius = radius;
	return ff;
}
