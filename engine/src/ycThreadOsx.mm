#include "ycThread.h"
#include "ycSystem.h"
#include "ycStringUtils.h"

#ifdef YC_OSX

#import <Foundation/Foundation.h>
#import <Foundation/NSThread.h>

const ycThread::Handle ycThread::kInvalidThread = nullptr;

@interface Thread : NSThread
@property ycThread::EntryPoint fn;
@property void *arg;

- (void)main;
 
@end

@implementation Thread
@synthesize fn;
@synthesize arg;
- (void)main {
    fn( arg );
}
@end

ycThread::Handle ycThread::Start( const char* name, EntryPoint entryPt, void* arg, const ureg coreIdx, const ycSize_t stackSize, const ureg flags )
{
    ycAssert( (flags&~(kFlag_HighPriority|kFlag_HighPriority_Audio|kFlag_LowPriority)) == 0 ); // ignore priority-based flags, anything else NYI/TODO
    YC_UNUSED( coreIdx );
    
    Thread *handle = [Thread new];
    [handle setFn:entryPt];
    [handle setArg:arg];
    
    [handle setStackSize:stackSize];
    [handle setName:[NSString stringWithUTF8String:name]];

    if( flags & kFlag_HighPriority_Audio )
    {
        // note, not the highest possible prio for audio threads
        // there is also https://developer.apple.com/documentation/audiotoolbox/workgroup_management/adding_parallel_real-time_threads_to_audio_workgroups?language=objc
        // but this requires setting up a real-time thread policy - see https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/scheduler/scheduler.html#//apple_ref/doc/uid/TP30000905-CH211-BABHGEFA
        [handle setQualityOfService: NSQualityOfServiceUserInteractive];
    }
    else if( flags & kFlag_HighPriority )
    {
        [handle setQualityOfService: NSQualityOfServiceUserInitiated];
    }
    else if( flags & kFlag_LowPriority )
    {
        [handle setQualityOfService: NSQualityOfServiceUtility];
    }
    else
    {
        [handle setQualityOfService: NSQualityOfServiceDefault];
    }
    
    [handle start];
    
    return (__bridge_retained void*)handle;
}


void ycThread::Join( Handle threadHandle )
{
    Thread *handle = (__bridge_transfer Thread*)threadHandle;
    while ([handle isFinished] == NO) {
        RunUiThreadTasks();
        usleep(1000);
    }
}

bool ycThread::IsDone( Handle threadHandle )
{
    return ([(__bridge Thread*)threadHandle isFinished] == YES);
}

ycThread::Handle ycThread::GetCurrent()
{
    return (__bridge void*)[NSThread currentThread];
}

void ycThread::Sleep( ureg milliseconds )
{
    double seconds = milliseconds/1000.0;
    [NSThread sleepForTimeInterval:seconds];
}

bool ycThread::HasMicroSleep()
{
    return true;
}

void ycThread::MicroSleep( ureg microseconds )
{
    double seconds = microseconds/1000000.0;
    [NSThread sleepForTimeInterval:seconds];
}

void ycThread::YieldThread()
{
    [NSThread sleepForTimeInterval:0.000001];
}

u64 ycThread::GetPrintableID( Handle threadHandle )
{
    return uintptr_t( threadHandle );
}

bool ycThread::IsUiThread()
{
    return [NSThread isMainThread];
}

void ycThread::RunUiThreadTasks()
{
	if([NSThread isMainThread])	{ CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE); }
}

ycSize_t ycSystem::GetSystemName( char* dstBuf, const ycSize_t bufSize )
{
    #ifdef YC_MAC
    BOOL success = [[[NSHost currentHost] name] getCString: dstBuf maxLength: bufSize encoding: NSUTF8StringEncoding];
    if( success )
    {
        return ycStringUtils::Length( dstBuf );
    }
    #endif
    
	return ycStringUtils::Copy( dstBuf, "UNKNOWN", bufSize );
}

#endif
