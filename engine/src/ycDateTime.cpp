#include "ycDateTime.h"

// core
#include "ycStd.h"
#include "ycString.h"

// 3rdparty
#if defined YC_MSFT
	//#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <time.h>
#elif defined YC_APPLE
	#include <mach/mach_time.h>
	#include <time.h>
	#include <sys/time.h>
#elif YC_NDA
#elif YC_NDA
#elif defined YC_NIX || defined YC_ANDROID
	#include <time.h>
	#include <sys/time.h>
#endif

namespace
{
	ureg     s_initialized = YC_FALSE; //!< not thread safe, but this should be double-init safe (TODO HACK FIXME: if a base start time is used, it isnt!)
	ycTime_t s_frequencyMs; //!< ticks per millisecond
	ycTime_t s_frequency;   //!< ticks per second
	f64      s_frequencyF;  //!< ticks per second
	#if defined YC_MSFT
		//
	#elif YC_NDA
	#elif defined YC_NIX || defined YC_ANDROID || defined YC_APPLE
		bool            s_useMonotonicTimer; //!< whether to use CLOCK_MONOTONIC or gettimeofday()
		struct timespec s_startNS;           //!< start time, nanoseconds
		struct timeval  s_startUS;           //!< start time, microseconds
		#if defined YC_APPLE
			ycTime_t s_startMacTicks;
			ycTime_t s_machFreqNumer;
			ycTime_t s_machFreqDenom;
		#endif
	#else
		#error ycDateTime is not implemented for current platform
	#endif

	enum
	{
		kSecsPerDay = 86400,
		kMSecsPerDay = 86400000,
		kSecsPerHour = 3600,
		kMSecsPerHour = 3600000,
		kSecsPerMinute = 60,
		kkMSecsPerMinute = 60000,
		kTimeTMax = 2145916799,  // int maximum 2037-12-31T23:59:59 UTC
		kJulianDayForEpoc = 2440588 // result of julianDayFromDate(1970, 1, 1)
	};

	s32 GetTimeZoneOffset()
	{
		#if defined YC_MSFT
			 long offset;
			_get_timezone(&offset);
			return offset;
		#else
			return 0;
		#endif
	}

	s64 FloorDiv( s64 a, s32 b )
	{
		return (a - (a < 0 ? b - 1 : 0)) / b;
	}

	s32 FloorDiv( s32 a, s32 b )
	{
		return (a - (a < 0 ? b - 1 : 0)) / b;
	}

	s64 JulianDayFromDate( s32 year, s32 month, s32 day )
	{
		// Adjust for no year 0
		if( year < 0 )
		{
			++year;
		}
	/*
	 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
	 * This formula is correct for all julian days, when using mathematical integer
	 * division (round to negative infinity), not c++11 integer division (round to zero)
	 */
		s32 a = FloorDiv( 14 - month, 12 );
		s64 y = (s64)year + 4800 - a;
		s32 m = month + 12 * a - 3;
		return day + FloorDiv( 153 * m + 2, 5 ) + 365 * y + FloorDiv( y, 4 ) - FloorDiv( y, 100 ) + FloorDiv( y, 400 ) - 32045;
	}

	struct ParsedDate
	{
		s32 year, month, day;
	};
	
	ParsedDate GetDateFromJulianDay( s64 julianDay )
	{
	/*
	 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
	 * This formula is correct for all julian days, when using mathematical integer
	 * division (round to negative infinity), not c++11 integer division (round to zero)
	 */
		s64 a = julianDay + 32044;
		s64 b = FloorDiv( 4 * a + 3, 146097 );
		s32 c = s32(a - FloorDiv( 146097 * b, 4 ));
		s32 d = FloorDiv( 4 * c + 3, 1461 );
		s32 e = c - FloorDiv( 1461 * d, 4 );
		s32 m = FloorDiv( 5 * e + 2, 153 );
		s32 day = e - FloorDiv( 153 * m + 2, 5 ) + 1;
		s32 month = m + 3 - 12 * FloorDiv( m, 10 );
		s32 year = s32(100 * b + d - 4800 + FloorDiv( m, 10 ));
		// Adjust for no year 0
		if( year <= 0 )
		{
			--year;
		}
		year -= 369;//not sure why i need this....
		return { year, month, day };
	}

	void MSecsToTime( s64 msecs, s64 *date, s64 *time )
	{
		s64 jd = kJulianDayForEpoc;
		s64 ds = 0;
		if( msecs >= kMSecsPerDay || msecs <= -kMSecsPerDay ) 
		{
			jd += msecs / kMSecsPerDay;
			msecs %= kMSecsPerDay;
		}
		if( msecs < 0 ) 
		{
			ds = kMSecsPerDay - msecs - 1;
			jd -= ds / kMSecsPerDay;
			ds = ds % kMSecsPerDay;
			ds = kMSecsPerDay - ds - 1;
		} 
		else 
		{
			ds = msecs;
		}
		if( date )
		{
			*date = jd;
		}
		if( time )
		{
			*time = ds;
		}
	}

	inline void Init()
	{
		if( s_initialized == YC_TRUE ) { return; }
		
		#if defined YC_MSFT
			LARGE_INTEGER freq;
			const BOOL ret = QueryPerformanceFrequency( &freq );
			ycAssert( ret != 0 ); YC_UNUSED( ret );
			s_frequency  = ycTime_t(freq.QuadPart);
			s_frequencyF = f64(freq.QuadPart);
			#if defined YC_WIN32
				/*
				Starting with Windows 11, if a window-owning process becomes fully occluded, minimized, or
				otherwise invisible or inaudible to the end user, Windows does not guarantee a higher
				resolution than the default system resolution. See SetProcessInformation for more information
				on this behavior.
				https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation#remarks
				*/
				PROCESS_POWER_THROTTLING_STATE PowerThrottling;
				RtlZeroMemory( &PowerThrottling, sizeof(PowerThrottling) );
				// ControlMask selects the mechanism and StateMask is set to zero as mechanisms should be turned off.
				PowerThrottling.Version     = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
				PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
				PowerThrottling.StateMask   = 0;
				SetProcessInformation( GetCurrentProcess(), ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling) );
				timeBeginPeriod( 1 );
			#endif
		#elif defined YC_APPLE
			//mach_timebase_info_data_t machTimeInfo;
			//kern_return_t ret = mach_timebase_info( &machTimeInfo );
			if( true ) // ret == 0 )
			{
				s_useMonotonicTimer = true;

				s_startMacTicks = clock_gettime_nsec_np( CLOCK_UPTIME_RAW );
				//s_startMacTicks = mach_absolute_time();
				s_machFreqNumer = 1; //machTimeInfo.numer;
				s_machFreqDenom = 1; //machTimeInfo.denom;

				s_frequencyF = 1.0f; //f64(machTimeInfo.numer) / f64(machTimeInfo.denom);
				s_frequencyF *= 1000000000.0f;  // nanoseconds to seconds
				s_frequency = s_frequencyF; // use s_machFreqNumer+s_machFreqDenom instead
			}
			else
			{
				s_useMonotonicTimer = false;
				const int ret2 = gettimeofday( &s_startUS, nullptr );
				ycAssert( ret2 == 0 );
				s_frequency = 1000000ull;
				s_frequencyF = 1000000.0;
			}
		#elif YC_NDA
		#elif YC_NDA
		#elif defined YC_NIX || defined YC_ANDROID
			if( clock_gettime( CLOCK_MONOTONIC, &s_startNS ) == 0 )
			{
				s_useMonotonicTimer = true;
				s_frequency = 1000000000ull;
				s_frequencyF = 1000000000.0;
			}
			else
			{
				s_useMonotonicTimer = false;
				const int ret = gettimeofday( &s_startUS, nullptr );
				ycAssert( ret == 0 );
				s_frequency = 1000000ull;
				s_frequencyF = 1000000.0;
			}
		#endif
		s_frequencyMs = s_frequency / 1000ul;

		s_initialized = YC_TRUE;

		// output timer frequency so the thread timeliner can scale times
		//#if defined YC_ENABLE_THREAD_LOG
		//	ycLog( "ycThreadLog timer frequency: %" PRIu64 "\n", s_frequency );
		//#endif
	}
}

/*static*/ ycTime_t ycDateTime::GetTimeHiRes()
{
	Init(); // TODO HACK FIXME: move this to a fixed location

	#if defined YC_MSFT
		LARGE_INTEGER count;
		const BOOL result = QueryPerformanceCounter( &count );
		ycAssert( result != 0 ); YC_UNUSED( result );
		return ycTime_t(count.QuadPart);
	#elif defined YC_APPLE
		if( s_useMonotonicTimer == true )
		{
			return clock_gettime_nsec_np( CLOCK_UPTIME_RAW );
			// return mach_absolute_time() - s_startMacTicks;
		}
		else
		{
			struct timeval now;
			gettimeofday( &now, nullptr );
			const long int sec  = now.tv_sec  - s_startUS.tv_sec;
			const long int usec = now.tv_usec - s_startUS.tv_usec;
			return (sec*s_frequency) + usec;
		}
	#elif YC_NDA
	#elif YC_NDA
	#elif defined YC_NIX || defined YC_ANDROID
		if( s_useMonotonicTimer == true )
		{
			struct timespec now;
			clock_gettime( CLOCK_MONOTONIC, &now );
			const ycTime_t sec = ycTime_t( now.tv_sec - s_startNS.tv_sec );
			return (sec*s_frequency) + ycTime_t( now.tv_nsec ) - ycTime_t( s_startNS.tv_nsec );
		}
		else
		{
			struct timeval now;
			gettimeofday( &now, nullptr );
			const ycTime_t sec = ycTime_t( now.tv_sec - s_startUS.tv_sec );
			return (sec*s_frequency) + ycTime_t( now.tv_usec ) - ycTime_t( s_startUS.tv_usec );
		}
	#endif
}

/*static*/ f32 ycDateTime::GetElapsedSeconds( const ycTime_t start, const ycTime_t end )
{
	return GetSeconds( end - start );
}

/*static*/ f32 ycDateTime::GetElapsedSeconds( const ycTime_t start )
{
	return GetElapsedSeconds( start, GetTimeHiRes() );
}

/*static*/ u32 ycDateTime::GetElapsedMilliseconds( const ycTime_t start, const ycTime_t end )
{
	const ycTime_t delta = end - start;
	#ifdef YC_APPLE
		if( s_useMonotonicTimer == true )
		{
			return (delta*s_machFreqNumer) / s_machFreqDenom / 1000000ull;
		}
	#endif
	return u32( delta/s_frequencyMs );
}

/*static*/ f32 ycDateTime::GetElapsedMillisecondsF( const ycTime_t start, const ycTime_t end )
{
	const ycTime_t delta = end - start;
	return GetSeconds( delta ) * 1000.0f; // TODO HACK FIXME: this calculation is probably not the ideal way to do this for precision
}

/*static*/ u32 ycDateTime::GetElapsedMilliseconds( const ycTime_t start )
{
	return GetElapsedMilliseconds( start, GetTimeHiRes() );
}

/*static*/ f32 ycDateTime::GetElapsedMillisecondsF( const ycTime_t start )
{
	return GetElapsedMillisecondsF( start, GetTimeHiRes() );
}

/*static*/ f32 ycDateTime::GetSeconds( const ycTime_t delta )
{
	return f32( f64(delta) / s_frequencyF );
}

/*static*/ ycTime_t ycDateTime::GetTicksPerSecond()
{
	return s_frequency;
}

/*static*/ ycTime_t ycDateTime::GetTicksPerMillisecond()
{
	return s_frequencyMs;
}

/*static*/ void ycDateTime::ToString( ycFileTime_t timeF, ycString* out )
{
#if 0
	timeF += GetTimeZoneOffset();
	s64 date;
	s64 time;
	MSecsToTime( timeF/s_frequencyMs, &date, &time );
	ParsedDate dateInfo = GetDateFromJulianDay( date );
	s32 hour = s32(time / kMSecsPerHour);
	s32 minute = s32((time % kMSecsPerHour) / kkMSecsPerMinute);
	out->SprintF( "%d/%d/%d %02d:%02d", dateInfo.month, dateInfo.day, dateInfo.year, hour, minute );// %02d:%02d
#elif defined YC_MSFT
	SYSTEMTIME st;
	FileTimeToSystemTime( (FILETIME*)&timeF, &st );
	TIME_ZONE_INFORMATION tz;
	SYSTEMTIME lst;
	GetTimeZoneInformation( &tz );
	SystemTimeToTzSpecificLocalTime( &tz, &st, &lst );
	if( lst.wHour > 12 )
	{
		out->SprintF( "%d/%d/%d %d:%02d PM", lst.wMonth, lst.wDay, lst.wYear, lst.wHour-12, lst.wMinute );
	}
	else
	{
		if( lst.wHour == 0 ) { lst.wHour += 12; }
		out->SprintF( "%d/%d/%d %d:%02d AM", lst.wMonth, lst.wDay, lst.wYear, lst.wHour, lst.wMinute );
	}
#else 
	YC_UNUSED( timeF );
	YC_UNUSED( out );
#endif
}

#if defined YC_MSFT
/*static*/ ycFileTime_t ycDateTime::ConvertFromFILETIME( const FILETIME filetime )
{
	ULARGE_INTEGER cv;
	cv.LowPart = filetime.dwLowDateTime;
	cv.HighPart = filetime.dwHighDateTime;

	u64 ts = cv.QuadPart;

	// https://docs.microsoft.com/en-us/windows/win32/sysinfo/converting-a-time-t-value-to-a-file-time
	ycFileTime_t t = ( ts - 116444736000000000LL ) / 10000000LL;
	return t;
}
#endif

/*static*/ ycFileTime_t ycDateTime::GetSystemTimeUTC()
{
#if defined YC_MSFT
	SYSTEMTIME systemtime;
	GetLocalTime( &systemtime );
	FILETIME filetime;
	SystemTimeToFileTime( &systemtime, &filetime );
	LocalFileTimeToFileTime( &filetime, &filetime );
	return ConvertFromFILETIME( filetime );
#elif defined YC_APPLE || defined YC_NIX || defined YC_ANDROID
	// Untested
	time_t t = time( nullptr );
	return (ycFileTime_t) t;
#elif YC_NDA
#elif YC_NDA
#endif
}

/*static*/ void ycDateTime::GetCalendarTime( CalendarTime * dst, ycFileTime_t time, u32 flags )
{
#if defined YC_MSFT
	// https://docs.microsoft.com/en-us/windows/win32/sysinfo/converting-a-time-t-value-to-a-file-time
	ULARGE_INTEGER cv;
	cv.QuadPart = ( time * 10000000LL ) + 116444736000000000LL;
	FILETIME filetime;
	filetime.dwLowDateTime = cv.LowPart;
	filetime.dwHighDateTime = cv.HighPart;

	if( ( flags & kGetCalendarTime_ConvertToLocal ) != 0 )
	{
		FileTimeToLocalFileTime( &filetime, &filetime );
	}

	SYSTEMTIME systemtime;
	FileTimeToSystemTime( &filetime, &systemtime );
	dst->year = (u16)systemtime.wYear;
	dst->month = (u8)systemtime.wMonth;
	dst->weekday = (u8)systemtime.wDayOfWeek;
	dst->day = (u8)systemtime.wDay;
	dst->hour = (u8)systemtime.wHour;
	dst->minute = (u8)systemtime.wMinute;
	dst->second = (u8)systemtime.wSecond;
#elif defined YC_APPLE || defined YC_NIX || defined YC_ANDROID
	// Untested
	struct tm buf;

	time_t croppedTime = (time_t)time; // crop to 32-bit on platforms where time_t is still 32-bit
	if( ( flags & kGetCalendarTime_ConvertToLocal ) != 0 )
	{
		localtime_r( &croppedTime, &buf );
	}
	else
	{
		gmtime_r( &croppedTime, &buf );
	}

	dst->year = (u16)( 1900 + buf.tm_year );
	dst->month = (u8)( 1 + buf.tm_mon );
	dst->weekday = (u8)buf.tm_wday;
	dst->day = (u8)buf.tm_wday;
	dst->hour = (u8)buf.tm_hour;
	dst->minute = (u8)buf.tm_min;
	dst->second = (u8)buf.tm_sec;
#elif YC_NDA
#elif YC_NDA
#endif
}

// TODO(jstpierre): Test this on other platforms
#if defined YC_TEST && defined YC_MSFT

#include "ycTest.h"

YC_BEGIN_TEST( ycDateTime_GetCalendarTime )
{
	ycFileTime_t t1 = 1657119786;
	ycDateTime::CalendarTime cal;

	ycDateTime::GetCalendarTime( &cal, t1, ycDateTime::kGetCalendarTime_None );

	YC_TEST_CHECK( cal.year == 2022 );
	YC_TEST_CHECK( cal.month == 7 );
	YC_TEST_CHECK( cal.weekday == 3 );
	YC_TEST_CHECK( cal.day == 6 );
	YC_TEST_CHECK( cal.hour == 15 );
	YC_TEST_CHECK( cal.minute == 3 );
	YC_TEST_CHECK( cal.second == 6 );

	// Test timezone conversion. Only turned on on my machine because it relies on system tz configuration.
#if defined YC_JASPER
	ycDateTime::GetCalendarTime( &cal, t1, ycDateTime::kGetCalendarTime_ConvertToLocal );

	YC_TEST_CHECK( cal.year == 2022 );
	YC_TEST_CHECK( cal.month == 7 );
	YC_TEST_CHECK( cal.weekday == 3 );
	YC_TEST_CHECK( cal.day == 6 );
	YC_TEST_CHECK( cal.hour == 8 );
	YC_TEST_CHECK( cal.minute == 3 );
	YC_TEST_CHECK( cal.second == 6 );
#endif

	YC_TEST_PASS( "ycDateTime::GetCalendarTime" );
}

#endif
