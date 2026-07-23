#pragma once

/* \class ycSpinLock
Implements functionality similar to ycMutex, but using atomics rather than kernel calls.

You should probably be using ycMutex instead. Use this only if you really know what you're doing.

For information about the implementation see:
https://en.wikipedia.org/wiki/Spinlock

Notes:
- Lock calls cannot be nested
*/

#include "ycCommon.h"
#include "ycAtomic.h"

class ycSpinLock
{
public:

	ycSpinLock( const char* debugName );
	~ycSpinLock();

	enum : ureg
	{
		kSpinMethod_Loop    = ureg(-1),
		kSpinMethod_Yield   = 0,
		kSpinMethod_Default = kSpinMethod_Yield
	};

	/*!
	\param methodOrMs determines how waiting is done if the lock cannot be immediately acquired. If neither
	kSpinMethod_Loop nor kSpinMethod_Yield are passed, the value is assumed to be a number of milliseconds to sleep for.
	Note: Be aware of platform limitations on sleep precision if passing low sleep times.
	*/
	void Lock( const ureg methodOrMs = kSpinMethod_Default );

	bool TryLock();

	void Unlock();

protected:

	ycAtomic m_lock;

	#ifdef YC_ENABLE_DEBUG_STRINGS
		const char* m_debugName;
	#endif
};
