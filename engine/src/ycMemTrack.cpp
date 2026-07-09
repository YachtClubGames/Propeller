// keeping track of all your memories in one place

#include "ycCommon.h"

#ifdef YC_ENABLE_MEM_DEBUG

#include "ycRBTree.h"
#include "ycAllocator.h"
#include "ycSysAllocator.h"
#include "ycMutex.h"
#include "ycMemTrack.h"
#include "ycTempString.h"
#include "ycClipboard.h"
#include "ycDebug.h"
#include "ycDevWindows.h"
#include "ycDir.h"
#include "ycFile.h"
#include "ycFileUtil.h"
#include "ycGuiWidget.h"
#include "ycImgui.h"
#include "ycOpenHash.h"
#include "ycSort.h"
#include "ycStackString.h"
#include "ycStringUtils.h"

//#define MAX_CALLSTACK 4

struct ycMemRecord
{
	static u32 m_currentMarker;
	static u32 m_currentCounter;

	const char *m_reason;
	const char *m_file;

	u32 m_line;
	u32 m_marker;
	u32 m_counter;
	ycSize_t m_size;

#if MAX_CALLSTACK
	ycDebug::StackFrame m_callstack[ MAX_CALLSTACK ];
#endif

	ycMemRecord() = default;
	ycMemRecord( ycSize_t size, const char* reason, const char* file, u32 line );
};

typedef ycOpenHash< void*, ycMemRecord, ycSize_t, 1024, 10 > ycMemoryMap;
typedef ycOpenHash< ycAllocator*, ycMemoryMap > ycAllocatorMap;

ycMemRecord::ycMemRecord( ycSize_t size, const char* reason, const char* file, u32 line ) :
	m_reason(reason),
	m_file(file),
	m_line(line),
	m_marker(m_currentMarker),
	m_counter(m_currentCounter++),
	m_size(size)
{
}

u32 ycMemRecord::m_currentMarker = 0;
u32 ycMemRecord::m_currentCounter = 0;

namespace
{
	ycAllocatorMap* s_allocatorMap = nullptr;
	ycMemoryMap* s_frameMap = nullptr;

	ycMutex s_memoryTrackerGuard( "ycMemoryTracker memory map access guard" );

	bool s_enableMemoryReport = false;
	u32 s_trackingCounter = 0;
	bool s_requestedDiffs = false;
	bool s_trackingDiffs = false;
	bool s_frameDiffs = false;
	bool s_singleFrameDiff = false;
	bool s_trackLeaks = false;
	s32 s_minMarksBack = 2;

	enum { kConsole = 0, kClipboard, kFile };
	int s_diffDestination = 0;

	void BeginDiffTracking() { s_requestedDiffs = true; }
	void SetDiffTrackingCounter() { s_trackingCounter = ycMemRecord::m_currentCounter; }
	void EndDiffTracking() { s_requestedDiffs = false; }
	void StartFrameDiffs() { s_requestedDiffs = s_frameDiffs = true; }
	void SingleFrameDiff() { s_requestedDiffs = s_singleFrameDiff = true; }

	struct AllocatorInfo
	{
		const char* name;
		ycSize_t used;
		ycSize_t allocs;
	};
}

void ycMemTrack::IncrementCurrentMarker( const char* reason )
{
	(void)reason;
	ycMemRecord::m_currentMarker++;
}

void ycMemTrack::Init()
{
	s_memoryTrackerGuard.Lock();
	if( s_allocatorMap == nullptr )
	{
		s_allocatorMap = new( ycSysAlloc( sizeof( ycAllocatorMap ), alignof( ycAllocatorMap ) ) ) ycAllocatorMap( ycSysAllocator::GetDefault(), ycAllocator::kAllocFlag_NoTrack );
		s_frameMap = new( ycSysAlloc( sizeof( ycMemoryMap ), alignof( ycMemoryMap ) ) ) ycMemoryMap( ycSysAllocator::GetDefault(), ycAllocator::kAllocFlag_NoTrack );
	}
	s_memoryTrackerGuard.Unlock();
}

void ycMemTrack::Shutdown()
{
	if( s_allocatorMap != nullptr )
	{
		s_memoryTrackerGuard.Lock();

		s_frameMap->~ycMemoryMap();
		ycSysFree( s_frameMap );
		s_frameMap = nullptr;

		s_allocatorMap->~ycAllocatorMap();
		ycSysFree( s_allocatorMap );
		s_allocatorMap = nullptr;

		s_memoryTrackerGuard.Unlock();
	}
}

void ycMemTrack::TrackAlloc( ycAllocator* allocator, void* memory, ycSize_t size, ycAllocInfo name, const char* file, u32 line )
{
	// don't track allocations outside of creating / destroying the tracking system
	// this will cause global destructors to fail, although they should ideally be avoided
	if( s_allocatorMap == nullptr ) { return; }

	// deal with the callstack outside of the memory tracking guard so the temp strings can do allocs
	//const char* callStack[ MAX_CALLSTACK ] = { 0 };
	//if( do_callstack ) { ycTraceFuncGetCallStack( callStack, MAX_CALLSTACK ); }

#ifdef YC_ENABLE_DEBUG_STRINGS
	const char* resolve_name = ( name.m_str && name.m_temp ) ? ycAddTempString( name.m_str ) : name.m_str;
#else
	const char* resolve_name = "<UNKNOWN>";
	YC_UNUSED( name );
#endif

	s_memoryTrackerGuard.Lock();

	ycMemoryMap* allocMap = s_allocatorMap->Get( allocator );

	if( allocMap == nullptr )
	{
		allocMap = s_allocatorMap->Insert( allocator );
		allocMap->Init( ycSysAllocator::GetDefault(), ycAllocator::kAllocFlag_NoTrack );
	}

	ycAssertMsg( allocMap->Get( memory ) == nullptr, "Address 0x%08x already registered by allocator 0x%08x\n", memory, allocator );

	ycMemRecord* record = allocMap->Insert( memory );
	*record = ycMemRecord{ size, resolve_name, file, line };

	// set the callstack for this record
#if MAX_CALLSTACK
	ycMemSet( record->m_callstack, 0, sizeof(record->m_callstack) );
	ycDebug::GetStackTrace( record->m_callstack, MAX_CALLSTACK, 3 );
#endif
	//const char** ppCallstack = record->m_callstack;
	//for( u32 c = 0; c < MAX_CALLSTACK; ++c )
	//{
	//	ppCallstack[ c ] = callStack[ c ];
	//}
	//u32 count = ycTraceFuncGetCallStack(ppCallstack, MAX_CALLSTACK);
	//if( count < MAX_CALLSTACK ) { ppCallstack[count] = nullptr; }

	if( s_trackingDiffs )
	{
		//ycDebug::Printf( "Alloc: %016X  /  Memory: %016X\n", allocator, memory );
		ycMemRecord *frameRecord = s_frameMap->Insert( memory );
		*frameRecord = ycMemRecord( size, resolve_name, file, line );
		#if MAX_CALLSTACK
			ycMemCpy( frameRecord->m_callstack, record->m_callstack, sizeof(record->m_callstack) );
		#endif
		//const char** ppFrameCallstack = frameRecord->m_callstack;
		//for( u32 c = 0; c < MAX_CALLSTACK; ++c )
		//{
		//	ppFrameCallstack[ c ] = callStack[ c ];
		//}
	}

	s_memoryTrackerGuard.Unlock();
}

void ycMemTrack::TrackFree( ycAllocator* allocator, void* memory )
{
	if( s_allocatorMap == nullptr || memory == nullptr ) { return; }

	s_memoryTrackerGuard.Lock();

	ycMemoryMap* allocMap = s_allocatorMap->Get( allocator );
	ycAssert( allocMap != nullptr );

	const auto slot = allocMap->FindSlot( memory );

	ycMemRecord* record = &allocMap->ValueAt( slot );
	ycAssertMsg( record != nullptr, "Attempting to free memory that was not allocated at 0x%llx", memory );

	// make a copy of the callstack strings in case any of them needs to be released
	// and want to do that outside of the memory tracker guard
	//const char** ppCallstack = record->m_callstack;
	//const char* callStack[ MAX_CALLSTACK ];
	//for( u32 c = 0; c < MAX_CALLSTACK; ++c )
	//{
	//	callStack[ c ] = ppCallstack[ c ];
	//}

	const char* reason = record->m_reason;

	allocMap->RemoveSlot( slot );

	if( s_trackingDiffs )
	{
		//ycDebug::Printf( "Free:  %016X  /  Memory: %016X\n", allocator, memory );
		s_frameMap->Remove( memory );
	}

	s_memoryTrackerGuard.Unlock();

	if( ycIsTempString( reason ) ) { ycSubTempString( reason ); }

	//for( u32 c = 0; c < MAX_CALLSTACK; ++c )
	//{
	//	if( callStack[ c ] )
	//	{
	//		if( ycIsTempString( callStack[ c ] ) )
	//		{
	//			ycSubTempString( callStack[ c ] );
	//		}
	//	}
	//}
}

void ycMemTrack::ClearAllocator( ycAllocator* allocator )
{
	if( s_allocatorMap != nullptr )
	{
		s_memoryTrackerGuard.Lock();
		s_allocatorMap->Remove( allocator );
		s_memoryTrackerGuard.Unlock();
	}
}

#ifndef YC_DISABLE_ENGINE_GUI
static ycSize_t AllocatorSum( ycMemoryMap* map, ycSize_t* count )
{
	ycSize_t total = 0;
	*count = 0;
	for( ycMemoryMap::Iter pair : *map )
	{
		*count += 1;
		total += pair.GetValue().m_size;
	}
	return total;
}
#endif

void ycMemTrack::UI()
{
#ifndef YC_DISABLE_ENGINE_GUI
	#ifndef YC_HEADLESS
	char cwd[ YC_MAX_PATH ];
	ycStringUtils::Copy( cwd, ycDir::GetCurrent().Get() );
	#ifdef YC_WIN32
	const ycSize_t cwdLen = ycStringUtils::Length( cwd );
	for( u32 i = 0; i < cwdLen; ++i )
	{
		if( cwd[ i ] == '/' ) { cwd[ i ] = '\\'; }
	}
	#endif
	ycStackString<YC_MAX_PATH> path( cwd );

	path.Append( "\\memory.txt" );
	ImGui::PushID( "mem-dump" );
	if( ImGui::Button( "Dump Memory to File" ) )
	{
		OutputToFile();
	}
	ImGui::SameLine();
	if( ImGui::Button( "Copy Path" ) ) { ycClipboard::SetText( path ); }
	ImGui::SameLine();
	if( ImGui::Button( "Show File" ) ) { ycFileUtil::ShowInExplorer( ycStringRef( path ) ); }
	if( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "%s", path.Get() ); }
	ImGui::PopID();
	ImGui::Separator();

	ImGui::PushID( "mem-diffs" );
	if( s_requestedDiffs == false )
	{
		if( ImGui::Button( "Start Diff" ) ) { BeginDiffTracking(); }
	}
	else if( s_trackingCounter == 0 )
	{
		if( ImGui::Button( "Mark  Diff" ) ) { SetDiffTrackingCounter(); }
	}
	else
	{
		if( ImGui::Button( "Stop  Diff" ) ) { EndDiffTracking(); }
	}
	ImGui::SameLine();
	if( ImGui::Button( "Single Frame Diff" ) ) { SingleFrameDiff(); }
	if( s_diffDestination != kClipboard )
	{
		ImGui::SameLine();
		if( ImGui::Button( s_requestedDiffs ? "Stop  Frame Diffs" : "Start Frame Diffs" ) )
		{
			s_requestedDiffs ? EndDiffTracking() : StartFrameDiffs();
		}
	}
	ImGui::Checkbox( "Track Memory Leaks", &s_trackLeaks );
	if( s_trackLeaks )
	{ 
		ImGui::InputInt( "Min Marks Back", &s_minMarksBack );
	}

	path.Set( cwd );
	path.Append( "\\memory-diffs.txt" );
	const char* choices[ 3 ] = { "Console", "Clipboard", "File" };
	ImGui::PushItemWidth( 100.0f );
	ImGui::Combo( "Output Destination", &s_diffDestination, choices, 3, 3 );
	ImGui::PopItemWidth();
	if( s_diffDestination == kClipboard )
	{
		ImGui::SameLine();
		ImGui::TextColored( ImVec4( 1.0f, 0.25f, 0.25f, 1.0f ), "[WARNING: Beginning of log might be cut off!]" );
	}

	if( ImGui::Button( "Clear File" ) )
	{
		ycFileWriter writer;
		writer.Open( "memory-diffs.txt" );
		writer.Close();
	}
	ImGui::SameLine();
	if( ImGui::Button( "Copy Path" ) ) { ycClipboard::SetText( path ); }
	ImGui::SameLine();
	if( ImGui::Button( "Show File" ) ) { ycFileUtil::ShowInExplorer( ycStringRef( path ) ); }
	if( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "%s", path.Get() ); }
	ImGui::PopID();

	ImGui::Separator();
	ycGuiWidget::Checkbox( "Memory Report", &s_enableMemoryReport );
	ycStackString<4096> report;
	if( s_enableMemoryReport )
	{
		const ycSize_t maxAllocators = 32;
		AllocatorInfo allocatorInfo[ maxAllocators ];

		ycSize_t totalUsed = 0;
		ycSize_t allocCount = 0;
		for( ycAllocatorMap::Iter pair : *s_allocatorMap )
		{
			const ycAllocator* allocator = pair.GetKey();
			ycAssertMsg( allocCount < maxAllocators, "Need to increase max allocator count for MemTrack UI." );
			AllocatorInfo& info = allocatorInfo[ allocCount++ ];
			info.name = allocator->GetDebugName();
			info.used = AllocatorSum( const_cast<ycMemoryMap*>( &pair.GetValue() ), &info.allocs );
			totalUsed += info.used;
		}

		const ureg mbytes = totalUsed / ( 1024 * 1024 );
		const ureg kbytes = ( totalUsed / 1024 ) % 1024;
		const ureg bytes = totalUsed % 1024;
		report.AppendF( "-------------------------------------------------------------------\n" );
		report.AppendF( "[%4d MB][%4d KB][%4d B] Total Used\n", mbytes, kbytes, bytes );
		report.AppendF( "===================================================================\n" );

		// sort allocators by used memory
		ycSort::Insertion<AllocatorInfo>( &allocatorInfo[ 0 ], allocCount,
			[ ] ( const AllocatorInfo& a, const AllocatorInfo& b ) { return int( b.used - a.used ); } );

		const char* units[ 3 ] = { " B", "KB", "MB" };
		report.AppendF( "[   Used] %-30s  | %10s  | %6s  |\n", "Allocator", "Bytes", "Allocs" );
		report.AppendF( "-------------------------------------------------------------------\n" );
		for( ureg i = 0; i < allocCount; ++i )
		{
			const AllocatorInfo& info = allocatorInfo[ i ];
			ycSize_t u = 0;
			ycSize_t r = info.used;
			while( r > 1024 )
			{
				++u;
				r /= 1024;
			}

			report.AppendF( "[%4d %s] %-30s  | %10d  | %6d  |\n", r, units[ u ], info.name, info.used, info.allocs );
		}
		report.AppendF( "-------------------------------------------------------------------\n" );

		ImGui::SameLine();
		if( ImGui::Button( "Copy Report" ) )
		{
			ycClipboard::SetText( report );
		}
		ImGui::Text( "%s", report.Get() );
	}
	#endif
#endif
}

#define kTrackMaxChars 16
#define kTrackMaxBytes 16
static void OutputAllocatorTrack( const ycAllocator* allocator, ycMemoryMap* map, u32 outputMode, ycFileWriter* writer )
{
	ycStackString<4096> buffer;

	bool fullDump = allocator == nullptr || !allocator->IsDumpSummarized();

	buffer.AppendF(
		"=====================================================================================================================================\r\n" );
	if( allocator )
	{
		buffer.AppendF(
			"== Allocations for allocator \"%s\" (0x%x) ==\r\n"
			, allocator->GetDebugName(), allocator );
	}
	if( fullDump )
	{
		buffer.AppendF(
			"%*caddress       size   mark      cnt memory chars     memory bytes                                    debug name                                callstack\r\n"
			"%*c=======       ====   ====      === ============     ============                                    ==========                                =========\r\n"
			, sizeof( void* ) * 2 - 7, ' ', sizeof( void* ) * 2 - 7, ' ' );
	}
	ycSize_t totalBytes = 0;
	u64 numAllocs = 0;
	for( ycMemoryMap::Iter pair : *map )
	{
		++numAllocs;
		const u8* mem = (const u8*)pair.GetKey();
		const ycMemRecord& record = pair.GetValue();

		if( mem == buffer.GetBytes() )
		{
			// uhh dont try to show your own buffer, because you might try to re-alloc it while doing this
			continue;
		}

		if( s_trackingCounter != 0 )
		{
			if( record.m_counter >= s_trackingCounter )
			{
				continue;
			}
		}
		if( s_trackLeaks && s_minMarksBack >= 0 && record.m_marker >= ycMemRecord::m_currentMarker - s_minMarksBack )
		{
			continue;
		}

		totalBytes += record.m_size;

		if( fullDump )
		{
			ycSize_t s = buffer.Length();
			buffer.AppendF( sizeof( void* ) == 4 ? "%08x:%10d %6d %8d " : "%016llx:%10d %6d %8d ", mem, record.m_size, record.m_marker, record.m_counter );
			for( u32 t = 0; t < kTrackMaxChars; ++t )
			{
				buffer.Append( t < record.m_size ? ( ( mem[ t ] < ' ' || mem[ t ] >= 0x7f ) ? '.' : mem[ t ] ) : ' ' );
			}

			for( u32 t = 0; t < kTrackMaxBytes; ++t )
			{
				if( t < record.m_size ) { buffer.AppendF( " %02x", mem[ t ] ); }
				else { buffer.Append( "   " ); }
			}

			buffer.Append( ' ' );
			buffer.Append( record.m_reason );
			buffer.PadToLength( ' ', s + sizeof( void* ) + 142 );

#if MAX_CALLSTACK
			//for( u32 c = 0; c < MAX_CALLSTACK; ++c )
			//{
			//	if( record.m_callstack[ c ] )
			//	{
			//		if( c ) { buffer.Append( ", " ); }
			//		else { buffer.Append( " " ); }
			//		buffer.Append( record.m_callstack[ c ] );
			//	}
			//	else
			//	{
			//		break;
			//	}
			//}
#endif

			buffer.Append( ' ' );
			buffer.Append( record.m_file );
			buffer.AppendF( " (%d)\r\n", record.m_line );

#if MAX_CALLSTACK
			for( u32 c = 0; c < MAX_CALLSTACK; ++c )
			{
				if( record.m_callstack[ c ].addr )
				{
					buffer.Append( "  " );
					ycDebug::GetStackFrame( &buffer, record.m_callstack[ c ] );
				}
				else
				{
					break;
				}
			}
#endif

			//ycDebug::Print( buffer.ToCStr() + s );

			if( buffer.Length() > ( buffer.Capacity() - 512 ) )
			{
				switch( outputMode )
				{
				case kConsole:
					ycDebug::Printf( "%s", buffer.Get() );
					break;
				case kClipboard:
					#ifndef YC_HEADLESS
					ycClipboard::SetText( buffer );
					#endif
					break;
				case kFile:
					writer->Write( (void*)buffer.Get(), buffer.Length() );
					break;
				}
				buffer.Clear();
			}
		}
	}
	
	buffer.AppendF( "== %" PRIu64 " total bytes in %" PRIu64 " allocations ==\r\n", u64(totalBytes), numAllocs );
	buffer.Append( "=====================================================================================================================================\r\n\r\n" );
	switch( outputMode )
	{
	case kConsole:
		ycDebug::Printf( "%s", buffer.Get() );
		break;
	case kClipboard:
		#ifndef YC_HEADLESS
		ycClipboard::SetText( buffer );
		#endif
		break;
	case kFile:
		writer->Write( (void*)buffer.Get(), buffer.Length() );
		break;
	}
}

void ycMemTrack::OutputToFile( ycAllocator* allocator, const char* fileName )
{
	ycFileWriter writer;
	if( fileName == nullptr ) { fileName = "memory.txt"; }
	if( writer.Open( fileName ) )
	{
		s_memoryTrackerGuard.Lock();
		if( allocator )
		{
			ycMemoryMap* map = s_allocatorMap->Get( allocator );
			if( map != nullptr )
			{
				OutputAllocatorTrack( allocator, map, kFile, &writer );
			}
			else
			{
				ycStackString<512> buffer;
				buffer.AppendF( "Could not locate allocator 0x%x\r\n", allocator );
				writer.Write( (void*)buffer.Get(), buffer.Length() );
			}
		}
		else
		{
			for( ycAllocatorMap::Iter pair : *s_allocatorMap )
			{
				OutputAllocatorTrack( pair.GetKey(), const_cast<ycMemoryMap*>( &pair.GetValue() ), kFile, &writer );
			}
		}
		s_memoryTrackerGuard.Unlock();
		writer.Close();
	}
}

void ycMemTrack::Output( ycAllocator* allocator )
{
	s_memoryTrackerGuard.Lock();
	if( allocator )
	{
		ycMemoryMap* map = s_allocatorMap->Get( allocator );
		if( map != nullptr )
		{
			OutputAllocatorTrack( allocator, map, kConsole, nullptr );
		}
		else
		{
			ycStackString<512> buffer;
			buffer.AppendF( "Could not locate allocator 0x%x\r\n", allocator );
			ycLog( buffer.Get() );
		}
	}
	else
	{
		for( const ycAllocatorMap::Iter& pair : *s_allocatorMap )
		{
			OutputAllocatorTrack( pair.GetKey(), const_cast<ycMemoryMap*>( &pair.GetValue() ), kConsole, nullptr );
		}
	}
	s_memoryTrackerGuard.Unlock();
}

void ycMemTrack::Update()
{
	if( s_trackingDiffs && s_singleFrameDiff ) { s_requestedDiffs = s_singleFrameDiff = false; }

	if( ( s_trackingDiffs && !s_requestedDiffs ) || s_frameDiffs )
	{
		// dump and records that survived across last frame
		ycFileWriter writer;
		writer.Open( "memory-diffs.txt", ycFileWriter::kAppend );
		s_memoryTrackerGuard.Lock();
		OutputAllocatorTrack( nullptr, s_frameMap, s_diffDestination, &writer );
		s_frameMap->Clear();
		s_memoryTrackerGuard.Unlock();
		writer.Close();
		s_trackingCounter = 0;
	}

	s_trackingDiffs = s_requestedDiffs;
	s_frameDiffs = s_frameDiffs && s_trackingDiffs;
}

#ifdef YC_TEST
#include "ycTest.h"

YC_BEGIN_TEST( ycMemTrack_Test )
{
#ifdef YC_DESKTOP
	YC_TEST_CHECK( s_allocatorMap );
	if( s_allocatorMap ) { ycMemTrack::OutputToFile(); }
#endif
}
#endif // YC_TEST

#endif // YC_USE_DEBUGVALUE
