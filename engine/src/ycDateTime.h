#pragma once

/*
\class ycDateTime

Encapsulates functionality for timing info
*/

#include "ycCommon.h"

class ycString;
typedef u64 ycTime_t; //!< can refer to a duration or an arbitrary fixed point in time
typedef u64 ycFileTime_t; //!< can refer to a duration or an arbitrary fixed point in time

#if defined YC_MSFT
typedef struct _FILETIME FILETIME;
#endif

class ycDateTime
{
public:

	static constexpr ycTime_t kMaxTimeVal = 0xffffffffffffffffllu;

	//! Returns a high-frequency time value
	/*!
	The return value is platform specific and should not be interpreted
	directly.  It should also not be stored and reused after the current
	program execution has terminated.
	*/
	static ycTime_t GetTimeHiRes();

	//! Returns the number of elapsed seconds between two high-resolution timestamps
	/* This assumes start < end. */
	static f32 GetElapsedSeconds( const ycTime_t start, const ycTime_t end );

	//! A convenience function that assumes 'now' for the end time
	static f32 GetElapsedSeconds( const ycTime_t start );

	//! Returns the number of elapsed milliseconds (floored) between two high-resolution timestamps
	/* This assumes start < end. */
	static u32 GetElapsedMilliseconds( const ycTime_t start, const ycTime_t end );
	static f32 GetElapsedMillisecondsF( const ycTime_t start, const ycTime_t end );

	//! A convenience function that assumes 'now' for the end time
	/*! The result is floored. */
	static u32 GetElapsedMilliseconds( const ycTime_t start );
	static f32 GetElapsedMillisecondsF( const ycTime_t start );

	//! Converts a high-resolution time into seconds.
	/*!
	* Do not assume high-resolution time begins at zero when the program starts.
	* This should only be used to convert the difference between reasonably
	  bounded high-resolution times.
	* Worst case values for this are 1 million (Linux, in fallback mode when
	  CLOCK_MONOTONIC is not present) and 10 million (Windows, without HPET)
	*/
	static f32 GetSeconds( const ycTime_t delta );
	static ycTime_t GetTicksPerSecond();
	static ycTime_t GetTicksPerMillisecond();

	static void ToString( ycFileTime_t time, ycString* out );

#if defined YC_MSFT
	static ycFileTime_t ConvertFromFILETIME( const FILETIME filetime );
#endif

	static ycFileTime_t GetSystemTimeUTC();
	
	struct CalendarTime
	{
		// July 6th, 2022, 8:10 AM (when I'm writing this code)
		u16 year;   // 2022
		u8 month;   // 7 (1-indexed month, 1 = January, 12 = December)
		u8 day;     // 6 (day of the month, 1-indexed)
		u8 weekday; // 3 (0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, etc.)
		u8 hour;    // 8 (hours since 12AM, 0 = 12AM, 23 = 11PM)
		u8 minute;  // 20 (minutes since the hour, 0-59)
		u8 second;  // 35 (seconds since the minute, 0-60) (60 = leap second)
	};
	enum
	{
		kGetCalendarTime_None = 0,
		kGetCalendarTime_ConvertToLocal = 1 << 0, // ycFileTime_t is in UTC. This flag suggests we convert it to the local system time.
	};
	static void GetCalendarTime( CalendarTime * dst, ycFileTime_t time, u32 flags = kGetCalendarTime_ConvertToLocal );
};
