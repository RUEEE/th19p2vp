// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <windows.h>
#include "injection.h"
#include "Address.h"
#include "imgui\imgui.h"
#include "Connection.h"
#include "Utils.h"



extern "C" {
    __declspec(dllexport)void func()
    {
        return;
    }
}

extern P2PConnection g_connection;
bool g_is_inited = false;


#define msg(x)  MessageBoxA(NULL, x, x, MB_OK);
//#define msg(x);
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
#ifndef  THCRAP
        LoadDll();
#endif
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (g_is_inited) {
            g_connection.EndConnect();
            WSACleanup();
            unHook();
            terminate();
        }
        break;
    }
    return TRUE;
}

