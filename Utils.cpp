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

void GetTime(LARGE_INTEGER* t)
{
    QueryPerformanceCounter(t);
    g_cur_time = *t;
}

DWORD GetAddress(DWORD Addr_noALSR)
{
    static DWORD hookModuleBase = (DWORD)GetModuleHandle(NULL);
    return Addr_noALSR - 0x00400000 + hookModuleBase;
}

void InitUtils()
{
	QueryPerformanceFrequency(&(g_time_freq));
    GetTime(&g_start_time);
}

void Delay(int millisec)
{
    LARGE_INTEGER start, end;
    GetTime(&start);
    while (true){
        GetTime(&end);
        int ms = ((end.QuadPart - start.QuadPart) * 1000 / (g_time_freq.QuadPart));
        if (ms >= millisec)
            break;
        if (millisec - ms >= 25)
            Sleep(5);
    }
    GetTime(&g_cur_time);
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

bool test_is_ipv6(const std::string& addr)
{
    //ipv4: [xxx.xxx.xxx.xxx]:yyy
    //ipv6: [xxx:xxx:...]:yyy
    int count=0;
    for (int i = 0; i < addr.size(); i++){
        if (addr[i]==':')
        {
            count++;
        }
    }
    return count>=2;//ipv4 only have 1 :
}

int s_atoi(const char* str, int default_int)
{
    int ret = default_int;
    try {
        ret = std::atoi(str);
    }
    catch (std::invalid_argument const& ex) {
        ret = default_int;
    }
    catch (std::out_of_range const& ex) {
        ret = default_int;
    }
    return ret;
}

int s_stoi(const std::string& str,int default_int)
{
    int ret= default_int;
    try{
        ret = std::stoi(str);
    }
    catch (std::invalid_argument const& ex){
        ret = default_int;
    }
    catch (std::out_of_range const& ex){
        ret = default_int;
    }
    return ret;
}
#include <vector>
std::vector<std::wstring> g_directory;
int g_count_directory=0;
void PushCurrentDirectory(LPCWSTR new_dictionary)
{
    WCHAR buffer[MAX_PATH] = { 0 };
    GetCurrentDirectoryW(MAX_PATH, buffer);
    if (g_directory.size() <= g_count_directory)
        g_directory.push_back(std::wstring(buffer));
    else
        g_directory[g_count_directory]=std::wstring(buffer);
    g_count_directory++;

    ExpandEnvironmentStringsW(new_dictionary,buffer,MAX_PATH);
    SetCurrentDirectoryW(buffer);
}

void PopCurrentDirectory()
{
    if (!g_directory.empty())
    {
        std::wstring last_dicg_dict = g_directory[g_count_directory-1];
        SetCurrentDirectoryW(last_dicg_dict.c_str());
        g_count_directory--;
    }
}


std::tuple<std::string, int, bool> split_addr_and_port(const std::string& addr)
{
    if (addr.size() == 0)
        return std::make_tuple("", c_no_port, false);
    std::string addr_t = addr;
    std::erase_if(addr_t, [](char c)->bool {return std::isspace(c); });

    bool is_ipv6 = test_is_ipv6(addr_t);
    int port = c_no_port;
    std::string addr_str="";
    if (is_ipv6){
        auto index_l = addr_t.find_first_of('[');
        auto index_r=addr_t.find_first_of(']');
        if (index_r == std::string::npos || addr_t.size() < index_r + 2)
            return std::make_tuple("", c_no_port, true);
        std::string port_str = addr_t.substr(index_r + 2);
        port = s_stoi(port_str,c_no_port);
        addr_str = addr_t.substr(index_l + 1, index_r-index_l-1);
    }else{
        auto index_r = addr_t.find_first_of(':');
        if (index_r == std::string::npos || addr_t.size() < index_r + 1)
            return std::make_tuple("", c_no_port, false);
        std::string port_str = addr_t.substr(index_r + 1);
        port = s_stoi(port_str, c_no_port);
        addr_str = addr_t.substr(0, index_r);
    }
    return std::make_tuple(addr_str, port, is_ipv6);
}
