#pragma once
#include "windows.h"

void Assert(const WCHAR* s);
void Hook(LPVOID addr_inject, size_t move_bytes, LPVOID callee);
//inject a call( void call(void) ) in anywhere
void HookCall(LPVOID addr_call, LPVOID callee);
//the function must be same with the function injected

void HookD3D();

BOOL hookVTable(void* pInterface, int index, void* hookFunction, void** oldAddress);
BOOL unhookVTable(void* pInterface, int index, void* oldAddress);

void** findImportAddress(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName);
BOOL hookIAT(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName, void* hookFunction, void** oldAddress);
BOOL unhookIAT(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName);

HWND GetMainWindow();
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

void HookWindow();
void unHook();

void InjectAll();

void HookCreateMutex();