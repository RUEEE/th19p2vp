#include "Utils.h"
#include <string>
#include <iostream>
#include <format>
LARGE_INTEGER g_time_freq;
LARGE_INTEGER g_cur_time;
LARGE_INTEGER g_start_time;

bool g_is_log=false;
std::string g_log;

void Log(std::string lv, std::string msg)
{
    if (g_is_log) {
        auto s = std::format("[{}] [{:>7}] {}\n", lv, CalTimePeriod(g_start_time, g_cur_time), msg);
        std::cout << s;
        g_log += s;
    }
}

void LogError(std::string msg)
{
    Log("ERR", msg);
}

void LogInfo(std::string msg)
{
    Log("INF", msg);
}

int CalTimePeriod(LARGE_INTEGER start, LARGE_INTEGER end)
{
	return ((end.QuadPart - start.QuadPart) * 1000 / (g_time_freq.QuadPart));
}


void InitUtils()
{
	QueryPerformanceFrequency(&(g_time_freq));
    QueryPerformanceCounter(&g_start_time);
}

void Delay(int millisec)
{
    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);
    while (true){
        QueryPerformanceCounter(&end);
        int ms = ((end.QuadPart - start.QuadPart) * 1000 / (g_time_freq.QuadPart));
        if (ms >= millisec)
            break;
        if (millisec - ms >= 25)
            Sleep(5);
    }
    QueryPerformanceCounter(&g_cur_time);
}

ImVec2 GetStageFromClient(ImVec2 client, ImVec2 client_sz, bool is_1P)
{
    float x = client.x;
    float y = client.y;
    x = x / client_sz.x * 1920.0f;
    y = y / client_sz.y * 1440.0f;
    if (is_1P){
        y = (y - 48.0f) / 3.0f;
        x = (x - (48.0f + 936.0f) / 2.0f) / 3.0f;
    }else{
        y = (y - 48.0f) / 3.0f;
        x = (x - (984.0f + 888.0f / 2.0f)) / 3.0f;
    }
    return ImVec2{ x,y };
}

ImVec2 GetClientFromStage(ImVec2 stage, ImVec2 client_sz,bool is_1P)
{
    float x = stage.x;
    float y = stage.y;
    if (is_1P){
        y = y * 3.0f + 48.0f;
        x = x * 3.0f + (48.0f + 936.0f) / 2.0f;
    }else{
        y = y * 3.0f + 48.0f;
        x = x * 3.0f + 984.0f + 888.0f / 2.0f;
    }
    x /= 1920.0f;
    y /= 1440.0f;
    return ImVec2{ x * client_sz.x,y * client_sz.y };
}