#pragma once

struct KeyState
{
	int frame;
	int state;
};

int __fastcall MyGetKeyState(DWORD thiz);
DWORD __fastcall GetRng(DWORD thiz);
SHORT __fastcall GetRng2(DWORD thiz);
void SetPlayer();
void SetLife2();
void EnterGame();
void InitGameValue();
void __fastcall PlayerState(DWORD ecx);