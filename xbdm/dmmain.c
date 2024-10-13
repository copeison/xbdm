// Project started early 2011
// If you want to get in touch, shoot me an email <confettimancer@gmail.com>

/*
 Copyright (c) 2013 Nathan LeRoux
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "dmincludes.h"

DMGD g_dmGlobals;
DMDD g_dmDebug;

EX_TITLE_TERMINATE_REGISTRATION titleRegistration = { LpTitleTerminateRoutine, 0 };
EX_THREAD_REGISTRATION threadRegistration = { LpThreadNotificationRoutine, 0 };

DWORD _resolve(HMODULE Module, DWORD Export, DWORD off1, DWORD off2)
{
	SHORT t1, t2;
	DWORD *f;

	if(FAILED(XexGetProcedureAddress(Module, Export, &f)))
		return 0;

	t1 = f[off1] & 0xFFFF;
	t2 = f[off2] & 0xFFFF;

	return (t1 << 16) + t2;
}

VOID ResolveData()
{
	DbgPrint("[xbdm] resolving kernel stuff\n");

	if(!(g_dmDebug.KeSystemProcess = (PKPROCESS)_resolve(g_dmGlobals.hKernel, 13, 16, 18)))
		DbgPrint("[xbdm] unable to resolve KeSystemProcess!\n");
	if(!(g_dmDebug.KeTitleProcess = (PKPROCESS)_resolve(g_dmGlobals.hKernel, 13, 20, 22)))
		DbgPrint("[xbdm] unable to resolve KeTitleProcess!\n");
	if(!(g_dmDebug.PsLoadedModuleList = (PLIST_ENTRY)_resolve(g_dmGlobals.hKernel, 412, 9, 11)))
		DbgPrint("[xbdm] unable to resolve PsLoadedModuleList!\n");
	if(!(g_dmDebug.XexLoadedModuleListLock = (LPDWORD)_resolve(g_dmGlobals.hKernel, 412, 3, 5)))
		DbgPrint("[xbdm] unable to resolve XexLoadedModuleListLock!\n");

	DbgPrint("[xbdm] resolving xam exports\n");
	XexGetProcedureAddress(g_dmGlobals.hXam, 421, &XamLoaderLaunchTitleEx);
}

BOOL DmDllMain()
{
#ifdef _DEBUG
	DWORD nop = 0x60000000;
#endif

#ifdef _DEBUG
	DbgPrint("[xbdm] please attach windbg\n");
	DebugBreak();
#endif

	DbgPrint("[xbdm] hi cOz!\n");

	DbgPrint("[xbdm] zeroing out initial global state\n");
	ZeroMemory(&g_dmGlobals, sizeof(g_dmGlobals));

	DbgPrint("[xbdm] initializing breakpoint data\n");
	DmpBreakSpinLock = 0;
	InitializeListHead(&leBreaks);
	ZeroMemory(&HardwareBreakpoint, sizeof(DMHB));

#ifdef _DEBUG
	DbgPrint("[xbdm] patching MmIsAddressValid (DEVKIT)\n");
	DmSetMemory((LPVOID)0x80096B58, 4, &nop, NULL);
	DmSetMemory((LPVOID)0x80096B08, 4, &nop, NULL);
	DmSetMemory((LPVOID)0x80096C04, 4, &nop, NULL);
	DmSetMemory((LPVOID)0x80096BB0, 4, &nop, NULL);
	DmSetMemory((LPVOID)0x80096A88, 4, &nop, NULL);
	DmSetMemory((LPVOID)0x80096B7C, 4, &nop, NULL);
#endif

	// Setup the exception data
	DbgPrint("[xbdm] initializing exception data\n");
	ExceptionStack = MmCreateKernelStack(0x8000, 2); // 8kb stack for exceptions
	ExceptionStackTitle = MmCreateKernelStack(0x8000, 2);
	InitializeCriticalSection(&csExceptionStack);
	InitializeCriticalSection(&csExceptionStackTitle);
	InitializeCriticalSection(&csExecState);

	DbgPrint("[xbdm] initializing debug monitor thread data\n");
#if 0
	InitializeCriticalSection(&csDebugMonitorData);
	leDebugMonitorData.Flink = leDebugMonitorData.Blink = &leDebugMonitorData;
#endif
	FInitThreadDebugData(PsGetCurrentThread());

	DbgPrint("[xbdm] initializing notifications\n");
	InitializeCriticalSection(&csNotify);
	InitializeListHead(&notifyList);
	InitializeListHead(&notifySockList);
	InitializeListHead(&notifyQueue);
	dwNotifyQueue = 0;
	notifyQueueLock = 0;

	DbgPrint("[xbdm] registering notifications\n");
	ExRegisterThreadNotification(&threadRegistration, TRUE);
	ExRegisterTitleTerminateNotification(&titleRegistration, TRUE);

	if(FAILED(XexGetModuleHandle("xboxkrnl.exe", &g_dmGlobals.hKernel)))
	{
		DbgPrint("[xbdm] Failed to get kernel handle\n");
		DebugBreak();
		return FALSE;
	}
	if(FAILED(XexGetModuleHandle("xam.xex", &g_dmGlobals.hXam)))
	{
		DbgPrint("[xbdm] Failed to get xam handle\n");
		DebugBreak();
		return FALSE;
	}
	if(FAILED(XexGetModuleHandle("launch.xex", &g_dmGlobals.hDashlaunch)))
	{
		DbgPrint("[xbdm] Failed to get dashlaunch handle\n");

		// Only fail in jtag build
#ifndef _DEBUG
		DebugBreak();
		return FALSE;
#else
		DbgPrint("[xbdm] hooking the trap handler (DEVKIT)\n");

		g_dmGlobals.PreviousTrap = *(pfnTrapHandler*)0x801C1B64;
		*(pfnTrapHandler*)0x801C1B64 = DmTrapHandler; // DEVKIT 13599/20871.2
#endif
	}
	else if(FAILED(XexGetProcedureAddress(g_dmGlobals.hDashlaunch, 1, &g_dmGlobals.pLaunchData)))
	{
		DbgPrint("[xbdm] Failed to get dashlaunch export\n");
		
		// Only fail in jtag build
#ifndef _DEBUG
		DebugBreak();
		return FALSE;
#endif
	}
	else if(*(DWORD*)(&g_dmGlobals.pLaunchData->versionMaj) < 0x00020018)
		DbgPrint("[xbdm] dashlaunch version %i.%i is not supported\n", g_dmGlobals.pLaunchData->versionMaj, g_dmGlobals.pLaunchData->versionMin);
	else
	{
		DbgPrint("[xbdm] dashlaunch version %i.%i supported\n", g_dmGlobals.pLaunchData->versionMaj, g_dmGlobals.pLaunchData->versionMin);

		g_dmGlobals.bDashlaunch = TRUE;

		// Enable single step exceptions
		DbgPrint("[xbdm] enabling single step exceptions\n");
		*g_dmGlobals.pLaunchData->DebugStepPatch = 0x60000000;

		DbgPrint("[xbdm] hooking the trap handler\n");
		g_dmGlobals.PreviousTrap = *g_dmGlobals.pLaunchData->DebugRoutine;
		*g_dmGlobals.pLaunchData->DebugRoutine = DmTrapHandler;
		DbgPrint("[xbdm] handler is hooked\n");
	}

#ifdef _DEBUG
	strcpy(g_dmGlobals.rgchDbgName, "XeDevkit"); // XeDevkit is default name for debug (aka devkit testing)
#else
	strcpy(g_dmGlobals.rgchDbgName, "Jtag"); // Jtag is default name for release (aka jtag)
#endif

	FLoadGlobals();
	FWriteGlobals(); // Force ini creation

	ResolveData();

	DbgPrint("[xbdm] installing hypervisor expansion\n");
	if(NT_SUCCESS(HvInitialize()))
		g_dmGlobals.bHypervisorEnabled = TRUE;

	DbgPrint("[xbdm] starting server\n");

	// lets just pull 16kb of stack for fun or something
	if(FAILED(ExCreateThread(&g_dmGlobals.pthrServ, 0x10000, NULL, NULL, ServerThread, NULL, 0x040004A6)))
	{
		DbgPrint("[xbdm] Failed to startup server\n");
#ifdef _DEBUG
		DebugBreak();
		return FALSE;
#endif
	}
	
	return TRUE;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
		return DmDllMain();
	
	return TRUE;
}