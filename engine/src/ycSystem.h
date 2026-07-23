#pragma once

#include "ycCommon.h"

/*! \namespace ycSystem
Helper to gather info about the system
*/

namespace ycSystem
{
	void GetCpuCount( ureg* physicalCores, ureg* logicalCores );
	
	ycSize_t GetEnv( char* dstBuf, const ycSize_t bufSize, const char* name ); //!< returns zero if the name does not exist, otherwise returns the number of buffer bytes needed (including the terminating NUL); if the passed if large enough, writes the value to it
	bool GetEnv(const char* name); // returns true if a value exists and the value is not "0" or "false" (case-insensitive)

	void SetEnv( const char* name, const char* val );
	void UnsetEnv( const char* name );

	bool IsBuildServer();
	bool IsRunningAsAService(); // running without access to a desktop; not the same as 'headless', which may or may not have a desktop

	ycSize_t GetSystemName( char* dstBuf, const ycSize_t bufSize ); // gets a name for this device to call itself on the network; returns the string length of the name (not including the terminating NUL)

	bool IsHandHeld();
	bool IsOverlay();

#ifdef YC_NDA
#endif

#ifdef YC_NIX
	bool IsWayland();
#endif

	void Exit();
}
