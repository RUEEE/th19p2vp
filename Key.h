#pragma once
#include <windows.h>
#include "States.h"

struct KeyState
{
	int frame;
	DWORD state;
	SeedType seednum[4];
};

int __fastcall MyGetKeyState(DWORD thiz);
// void SetPlayer();
// void SetLife2();
// void EnterGame();
// void InitGameValue();
// void __fastcall PlayerState(DWORD ecx);