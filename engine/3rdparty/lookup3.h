#pragma once
#ifndef LOOKUP3_H
#define LOOKUP3_H

#include "ycCommon.h" // std types

uint32_t hashword(
	const uint32_t *k,      /* the key, an array of uint32_t values */
	size_t          length, /* the length of the key, in uint32_ts */
	uint32_t        initval /* the previous hash, or an arbitrary value */
);

uint32_t hashlittle( const void *key, size_t length, uint32_t initval );

void hashlittle2( 
	const void *key,    /* the key to hash */
	size_t      length, /* length of the key */
	uint32_t   *pc,     /* IN: primary initval, OUT: primary hash */
	uint32_t   *pb      /* IN: secondary initval, OUT: secondary hash */
);

#endif // !LOOKUP3_H
