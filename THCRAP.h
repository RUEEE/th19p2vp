#pragma once

// if you want to make the patch compatible with thcrap
#define THCRAP


#ifdef THCRAP
#pragma  comment(lib, "thcrap.lib")
extern "C"
{
    __declspec(dllimport) int WINAPI detour_chain(const char* dll_name, int return_old_ptrs, ...);
    __declspec(dllimport) [[nodiscard("Return value must be passed to '""free""' by caller!")]] char* fn_for_game(const char* fn);
}
#endif
