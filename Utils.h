#pragma once
#include <windows.h>
#include <timeapi.h>
#include <string>
#include "imgui\imgui.h"

void LogError(std::string msg);
void LogInfo(std::string msg);

int CalTimePeriod(LARGE_INTEGER start, LARGE_INTEGER end);
void InitUtils();
void Delay(int millisec);

ImVec2 GetStageFromClient(ImVec2 client, ImVec2 client_sz, bool is_1P);
ImVec2 GetClientFromStage(ImVec2 stage, ImVec2 client_sz, bool is_1P);
#define VALUED(x)  (*(DWORD*)x)
#define VALUEF(x)  (*(float*)x)
#define VALUEW(x)  (*(WORD*)x)
#define VALUEV(x,T)  (*(T*)x)