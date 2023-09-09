#include "pch.h"
#include "injection.h"
#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx9.h"
#include "imgui\imgui_impl_win32.h"
#include "UI.h"
#include "Key.h"
#include "Utils.h"
#include "Connection.h"
#include "Address.h"

#include <windows.h>
#include <memory>
#include <d3d9.h>
#include <d3dx9.h>

void Assert(const WCHAR* s)
{
    MessageBox(NULL, s, L"warning", MB_OK);
}

void Hook(LPVOID addr_inject, size_t move_bytes, LPVOID callee)//inject a call( void call(void) ) in anywhere
{
    if (move_bytes < 5) { Assert(L"space limited"); return; }
    DWORD oldprotect;
    DWORD oldprotect2;
    VirtualProtect(addr_inject, move_bytes, PAGE_EXECUTE_READWRITE, &oldprotect);

    BYTE code[14] = { 0x60,0xB8,0xCC,0xCC,0xCC,0xCC,0xFF,0xD0,0x61,0xE9, 0xCC,0xCC,0xCC,0xCC };
    BYTE* alloc = (BYTE*)malloc(move_bytes + 14);
    if (!alloc) { Assert(L"failed to allocate memory"); return; }
    for (size_t i = 0; i < move_bytes; i++)
    {
        alloc[i] = *(BYTE*)((DWORD)addr_inject + i);
    }
    (*((LPVOID*)&(code[2]))) = callee;
    DWORD jmpTo = ((DWORD)addr_inject + move_bytes) - ((DWORD)alloc + move_bytes + 9) - 5;
    (*((LPVOID*)&(code[10]))) = (LPVOID)jmpTo;
    for (size_t i = 0; i < 14; i++)
    {
        alloc[i + move_bytes] = code[i];
    }
    ((BYTE*)addr_inject)[0] = 0xE9;
    (*(DWORD*)((DWORD)addr_inject + 1)) = (DWORD)alloc - (DWORD)addr_inject - 5;
    for (UINT i = 5; i < move_bytes; i++)
    {
        ((BYTE*)addr_inject)[i] = 0x90;
    }
    VirtualProtect(addr_inject, move_bytes, oldprotect, &oldprotect2);
    VirtualProtect(alloc, move_bytes + 14, PAGE_EXECUTE_READWRITE, &oldprotect2);
    return;
}

void HookSwitch(LPVOID addr_switch_table, LPVOID callee,LPVOID break_addr)//inject a call( void call(void) ) in switch table
{
    DWORD oldprotect2;

    BYTE code[14] = { 0x60 ,0xB8 ,0x78 ,0x56 ,0x34 ,0x12 ,0xFF ,0xD0 ,0x61 ,0xE9 ,0x6A ,0x56 ,0x4D ,0xF9 };
    BYTE* alloc = (BYTE*)malloc(14);


    if (!alloc) { Assert(L"failed to allocate memory"); return; }
    (*((LPVOID*)&(code[2]))) = callee;
    DWORD jmpTo = ((DWORD)addr_switch_table) - ((DWORD)break_addr) - 5;
    (*((LPVOID*)&(code[10]))) = (LPVOID)jmpTo;
    for (size_t i = 0; i < 14; i++)
    {
        alloc[i] = code[i];
    }
    Address<DWORD>((DWORD)addr_switch_table).SetValue((DWORD)alloc);
    VirtualProtect(alloc, 14, PAGE_EXECUTE_READWRITE, &oldprotect2);
    return;
}

void HookCall(LPVOID addr_call, LPVOID callee)
{
    Address<DWORD>((DWORD)addr_call + 1).SetValue((DWORD)callee - ((DWORD)addr_call + 5));
}

void HookCall(LPVOID addr_inject, LPVOID callee);//the function must be same with the function injected

void** findImportAddress(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName)
{
    uintptr_t hookModuleBase = (uintptr_t)hookModule;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hookModuleBase;
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)(hookModuleBase + dosHeader->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR importTable = (PIMAGE_IMPORT_DESCRIPTOR)(hookModuleBase
        + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for (; importTable->Characteristics != 0; importTable++)
    {
        if (_stricmp((LPCSTR)(hookModuleBase + importTable->Name), moduleName) != 0)
            continue;
        PIMAGE_THUNK_DATA info = (PIMAGE_THUNK_DATA)(hookModuleBase + importTable->OriginalFirstThunk);
        void** iat = (void**)(hookModuleBase + importTable->FirstThunk);
        for (; info->u1.AddressOfData != 0; info++, iat++)
        {
            if ((info->u1.Ordinal & IMAGE_ORDINAL_FLAG) == 0)
            {
                PIMAGE_IMPORT_BY_NAME name = (PIMAGE_IMPORT_BY_NAME)(hookModuleBase + info->u1.AddressOfData);
                if (strcmp((LPCSTR)name->Name, functionName) == 0)
                    return iat;
            }
        }
        return NULL;
    }
    return NULL; 
}


BOOL hookVTable(void* pInterface, int index, void* hookFunction, void** oldAddress)
{
    void** address = &(*(void***)pInterface)[index];
    if (address == NULL)
        return FALSE;
    if (oldAddress != NULL)
        *oldAddress = *address;
    DWORD oldProtect, oldProtect2;
    VirtualProtect(address, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
    *address = hookFunction;
    VirtualProtect(address, sizeof(DWORD), oldProtect, &oldProtect2);
    return TRUE;
}


BOOL unhookVTable(void* pInterface, int index, void* oldAddress)
{
    return hookVTable(pInterface, index, oldAddress, NULL);
}


BOOL hookIAT(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName, void* hookFunction, void** oldAddress)
{
    void** address = findImportAddress(hookModule, moduleName, functionName);
    if (address == NULL)
        return FALSE;
    if (oldAddress != NULL)
        *oldAddress = *address;
    DWORD oldProtect, oldProtect2;
    VirtualProtect(address, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
    *address = hookFunction;
    VirtualProtect(address, sizeof(DWORD), oldProtect, &oldProtect2);
    return TRUE;
}


BOOL unhookIAT(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName)
{
    auto x = GetModuleHandleA(moduleName);
    if (x == NULL)
        return FALSE;
    void* oldAddress = GetProcAddress(x, functionName);
    if (oldAddress == NULL)
        return FALSE;
    return hookIAT(hookModule, moduleName, functionName, oldAddress, NULL);
}
typedef IDirect3D9* (WINAPI* Direct3DCreate9Type)(UINT SDKVersion);
typedef HRESULT(STDMETHODCALLTYPE* CreateDeviceType)(IDirect3D9* thiz, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
typedef HRESULT(STDMETHODCALLTYPE* PresentType)(IDirect3DDevice9* thiz, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
Direct3DCreate9Type RealDirect3DCreate9 = NULL;
CreateDeviceType RealCreateDevice = NULL;
PresentType RealPresent = NULL;

IDirect3D9* g_d3d9 = NULL;
IDirect3DDevice9* g_device = NULL;
bool g_is_hooked = false;

HWND g_wnd_hwnd;
WNDPROC g_oldWndProc;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice9* thiz, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    if (g_is_hooked)
    {
        static bool x = false;
        g_device->BeginScene();
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        SetUI(thiz);
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        g_device->EndScene();
    }
    //hook=======================
    unhookVTable(g_device, 17, RealPresent);
    HRESULT res = g_device->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    hookVTable(g_device, 17, MyPresent, (void**)&RealPresent);
    //present is no.18
    return res;
}

HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3D9* thiz, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    unhookVTable(g_d3d9, 16, RealCreateDevice);
    HRESULT res = RealCreateDevice(thiz, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    g_device = *ppReturnedDeviceInterface;
    hookVTable(g_device, 17, MyPresent, (void**)&RealPresent);
    //hook===============================
    ImGui_ImplDX9_Init(g_device);
    g_is_hooked = true;
    // {
    //     auto fonts = ImGui::GetIO().Fonts;
    //     fonts->AddFontFromFileTTF("simhei.ttf", 13.0f, NULL, fonts->GetGlyphRangesChineseSimplifiedCommon());
    // }
    return res;
}

IDirect3D9* WINAPI MyDirect3DCreate9(UINT SDKVersion)
{
    //hook
    unhookIAT(GetModuleHandle(NULL), "d3d9.dll", "Direct3DCreate9");
    g_d3d9 = RealDirect3DCreate9(SDKVersion);
    hookVTable(g_d3d9, 16, MyCreateDevice, (void**)&RealCreateDevice); // CreateDevice是IDirect3D9第17个函数
    return g_d3d9;
}

void HookD3D()
{
    hookIAT(GetModuleHandle(NULL), "d3d9.dll", "Direct3DCreate9", MyDirect3DCreate9, (void**)&RealDirect3DCreate9);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD dwCurProcessId = *((DWORD*)lParam);
    DWORD dwProcessId = 0;

    GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (dwProcessId == dwCurProcessId && GetParent(hwnd) == NULL)
    {
        *((HWND*)lParam) = hwnd;
        return FALSE;
    }
    return TRUE;
}

HWND GetMainWindow()
{
    DWORD dwCurrentProcessId = GetCurrentProcessId();
    if (!EnumWindows(EnumWindowsProc, (LPARAM)&dwCurrentProcessId)){
        return (HWND)dwCurrentProcessId;
    }
    return NULL;
}


LRESULT CALLBACK MyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        //case WM_CLOSE:
        //    unHook();
        //    CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
        //    exit(666);
        //    break;
    case WM_CLOSE:
    {
        unHook();
        //*(DWORD*)0 = 0;
        //HANDLE hself = GetCurrentProcess();
        //TerminateProcess(hself, 0);
        SendMessageW(hWnd, WM_CLOSE, 0, 0);
        return 1;
    }
    default:
        break;
    }
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;
    return g_oldWndProc(hWnd, uMsg, wParam, lParam);
}

HWND(WINAPI* RealCreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
HWND WINAPI MyCreateWindowExW(DWORD dwExStyle
    , LPCWSTR lpClassName
    , LPCWSTR  lpWindowName
    , DWORD dwStyle
    , int X, int Y, int nWidth, int nHeight
    , HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    HWND hw = RealCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
        hWndParent, hMenu, hInstance, lpParam);
    unhookIAT(GetModuleHandle(NULL), "user32.dll", "CreateWindowExW");
    ImGui_ImplWin32_Init(hw);
    g_wnd_hwnd = hw;
    g_oldWndProc = (WNDPROC)SetWindowLongW(g_wnd_hwnd, GWL_WNDPROC, (LONG)MyWndProc);
    return hw;
}

BOOL (*WINAPI RealTextOutW)(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString);
BOOL WINAPI MyTextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString)
{
    RECT rc{};

    //auto hFont = CreateFontA(15, 15, 0, 0, FW_THIN, true, false, false,
    //     CHINESEBIG5_CHARSET, OUT_CHARACTER_PRECIS,
    //     CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
    //     FF_MODERN, "宋体");

    //SelectObject(hdc, hFont);

    rc.left = nXStart; rc.top = nYStart;
    rc.right = nXStart + 1000;
    rc.bottom = rc.top + 1000;

    //auto hBrush = CreateSolidBrush(RGB(255, 255, 0));
    //FillRect(hdc, &rc, hBrush);
    auto ret = DrawTextW(hdc, lpString, cbString, &rc, DT_LEFT | DT_TOP);
    //DeleteObject(hFont);
    return ret;
}

HANDLE (*WINAPI RealCreateMutexW)(LPSECURITY_ATTRIBUTES se, BOOL initialOwner, LPCWSTR name);
HANDLE WINAPI MyCreateMutexW(LPSECURITY_ATTRIBUTES se, BOOL initialOwner, LPCWSTR name)
{
    HANDLE Ret=0;
    __asm
    {
        push name
        push initialOwner
        push se
        call RealCreateMutexW
        mov Ret,eax
    }
    // HANDLE ret = RealCreateMutexW(se, initialOwner, name);
    // MSVC bug? RealCreateMutexW is WINAPI but compile to CDECL
    auto err = GetLastError();
    if (err != ERROR_ALREADY_EXISTS){
        SetLastError(err);
        return Ret;
    }
    auto boxRet = MessageBoxW(NULL, L"th19.exe still alive, use task manager to kill the exe,\n click YES to open process anyway", L"warning(touhou mod dll)", MB_YESNO);
    if (boxRet == IDNO) {
        SetLastError(ERROR_ALREADY_EXISTS);
    }else {
        SetLastError(0);
    }
    return Ret;
}


void HookWindow()
{
    hookIAT(GetModuleHandle(NULL), "user32.dll", "CreateWindowExW", MyCreateWindowExW, (void**)&RealCreateWindowExW);
    // hookIAT(GetModuleHandle(NULL), "gdi32.dll", "TextOutW", MyTextOutW, (void**)&RealTextOutW);
}

void unHook()
{
    if (g_is_hooked)
    {
        g_is_hooked = false;
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        unhookVTable(g_device, 17, RealPresent);
        SetWindowLongW(g_wnd_hwnd, GWL_WNDPROC, (LONG)g_oldWndProc);
    }
}


//#define msg(x)  MessageBoxA(NULL, x, x, MB_OK);
#define msg(x)
void InjectAll()
{
     HookCall((LPVOID)0x004ABAB2, MyGetKeyState);
     InitUtils();
     P2PConnection::WSAStartUp();
     
     Address<BYTE>(0x402C00).SetValue(0xE9);
     Address<DWORD>(0x402C01).SetValue((DWORD)(GetRng)-0x402C00-5);
     
     Address<BYTE>(0x402340).SetValue(0xE9);
     Address<DWORD>(0x402341).SetValue((DWORD)(GetRng2)-0x402340-5);
     
     //Hook((LPVOID)0x00505688, 7, SetLife);
     Hook((LPVOID)0x0053239F, 7, SetPlayer);
     //Hook((LPVOID)0x00501F9D, 5, SetLife2);
     //Hook((LPVOID)0x0042A1F7, 6, PlayerMove);
     Hook((LPVOID)0x00530EE3, 6, PlayerState);

    //default menu

    // 542313 -> 00(->main menu)
    // 542077 -> 6A 00 90 90 90 90 (->vs mode)
    // 54247D -> 6A 03 90 90 90 90 (->difficulty)
    // 53FE0E -> 31 C0 (->P1)
    // 53FEE2 -> 31 C0 40 90 90 (->P2)
    // 4D4021 -> B9 01 00 00 00 90 90 90 (->card)
    
    Address<BYTE>(0x00542313).SetValue(0x0);
    
    Address<WORD>(0x542077).SetValue(0x006A);
    Address<DWORD>(0x542079).SetValue(0x90909090);
    
    Address<WORD>(0x54247D).SetValue(0x036A);
    Address<DWORD>(0x54247F).SetValue(0x90909090);
    
    Address<WORD>(0x53FE0E).SetValue(0xC031);
    Address<BYTE>(0x53FEE2).SetValue(0x31);
    Address<DWORD>(0x53FEE3).SetValue(0x909040C0);
    
    Address<DWORD>(0x4D4021).SetValue(0x000001B9);
    Address<DWORD>(0x4D4025).SetValue(0x90909000);
}


void HookCreateMutex()
{
    hookIAT(GetModuleHandle(NULL), "Kernel32.dll", "CreateMutexW", MyCreateMutexW, (void**)&RealCreateMutexW);
}
