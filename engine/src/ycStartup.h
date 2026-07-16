#pragma once

#include "ycStd.h"

// Allows a function be automatically called at startup.
//
// While useful for auto-registering plugins, one should
// prefer *NOT* to use this technique in general as it
// clouds reasoning about control flow.
//
// Can be used multiple times throughout the program.
// Order of initialization is undefined within the list itself, 
// but you can call the list entries whenever you want.
//
// First define your list like so:
//    ycStartupEntry* g_editorStartup = nullptr;
//    #define ycEditorStartup ycStartup( g_editorStartup )
//
// Then use like this throughout your code:
//    void ycEditorStartup() { ycLog("hello\n"); }
//    void ycEditorStartup() { ycLog("world\n"); }
//    void ycEditorStartup() { ycLog("!\n"); }
//
// And finally traverse the list when you want to call it:
//    for( ycStartupEntry* g = g_editorStartup; g; g=g->next )
//        g->fn();

#define ycStartup2(LIST, N) \
	static YC_JOIN(_startup,N)(); \
	static ycStartupEntry YC_JOIN(_init,N)( YC_JOIN(_startup,N), LIST ); \
	static void YC_JOIN(_startup,N)

#define ycStartup(LIST) ycStartup2(LIST, __COUNTER__)

struct ycStartupEntry
{
	typedef void Fn();

	Fn* fn;
	ycStartupEntry* next;

	ycStartupEntry( Fn* fn, ycStartupEntry* &list ) : fn(fn) {
		next = list;
		list = this;
	}
};
