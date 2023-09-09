#include "pch.h"
#include "UI.h"
#include "Connection.h"
#include "Utils.h"
#include <windows.h>
#include "injection.h"

#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx9.h"
#include "imgui\imgui_impl_win32.h"

#include "Address.h"

#include <d3d9.h>
#include <d3dx9.h>

#include <vector>
#include <sstream>
#include <string>
#include <format>
#include <unordered_map>
#include <format>
#include <fstream>

#define VALUED(x) (*(DWORD*)(x))
#define VALUEF(x) (*(float*)(x))

extern P2PConnection g_connection;
extern int g_cur_frame;
extern int g_real_frame;
extern int g_seed_numA;
extern int g_seed_numB;
extern std::string g_log;
extern bool g_is_log;
bool g_is_focus_ui;

struct UI_State
{
    struct {
        std::string log;
        int frame;
    }testcnt;
}g_UI_State;

void TestConnect_Host_UI(bool* p_open)
{
    if (!*p_open)
        return;
    auto& testcnt = g_UI_State.testcnt;
    if (testcnt.frame >= 0)
    {
        testcnt.frame++;
        Pack rcvPack;
        if (g_connection.RcvPack(&rcvPack) > 0 && rcvPack.type == Pack_Type::Test_Pack)
        {
            testcnt.log += std::format("successfully received pack {}\n",rcvPack.index);
            g_connection.SendPack(rcvPack);
        }else if (testcnt.frame % 60 == 1){
            testcnt.log += std::format("waiting...\n");
        }
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(340.0f, 100.0f), ImVec2(340.0f, 640.0f));
    ImGui::SetNextWindowPos(ImVec2(400.0f, 0.0f));
    ImGui::Begin("TestCnt", p_open, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
    ImGui::TextWrapped(testcnt.log.c_str());
    ImGui::End();
}
void TestConnect_Guest_UI(bool* p_open)
{
    if (!*p_open)
        return;
    auto& testcnt = g_UI_State.testcnt;
    if (testcnt.frame >= 0)
    {
        testcnt.frame++;
        LARGE_INTEGER cur_time;
        QueryPerformanceCounter(&cur_time);
        if (testcnt.frame % 60 == 1)
        {
            Pack testPack;
            testPack.type = Pack_Type::Test_Pack;
            testPack.test_pack.time_stamp = cur_time;
            g_connection.SendPack(testPack);
        }
        Pack rcvPack;
        if (g_connection.RcvPack(&rcvPack) > 0 && rcvPack.type == Pack_Type::Test_Pack)
        {
            testcnt.log += std::format("successfully connected, delay:{}ms\n", CalTimePeriod(rcvPack.test_pack.time_stamp, cur_time));
            testcnt.frame = testcnt.frame % 60;
        }
        if (testcnt.frame >= 120)
        {
            testcnt.log += std::format("time out\n");
            testcnt.frame = 0;
        }
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(340.0f, 100.0f), ImVec2(340.0f, 640.0f));
    ImGui::SetNextWindowPos(ImVec2(400.0f, 0.0f));
    ImGui::Begin("TestCnt", p_open, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
    ImGui::TextWrapped(testcnt.log.c_str());
    ImGui::End();
}

void SetUI(IDirect3DDevice9* device)
{
    static bool is_testGuest_open = false;
    static bool is_testHost_open = false;
    static bool is_collapse = false;

    //ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 200.0f), ImVec2(320.0f, 200.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowCollapsed(is_collapse);
    if (g_connection.connect_state == ConnectState::No_Connection)
    {
        ImGui::Begin("No Connection###wind",0,ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
    }else if (g_connection.connect_state == ConnectState::All_Started){
        ImGui::Begin("PVP###wind", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
    }else{
        ImGui::Begin("Waiting for P2###wind", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
    }
    g_is_focus_ui = ImGui::IsWindowFocused();
    is_collapse=ImGui::IsWindowCollapsed();

    
    ImGui::LabelText(" ", "%d; %d+%d+(%d,%d)", g_cur_frame, g_connection.seednum0, g_real_frame, g_seed_numA, g_seed_numB);

    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputText(" Host IP ", g_connection.address, sizeof(g_connection.address));

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragInt("delay", &g_connection.delay_compensation, 1.0f, 1, 180, "%d frame");

    
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragInt("Host port", &g_connection.port_Host, 1.0f, 0, 9999);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragInt("Guest port", &g_connection.port_Guest, 1.0f, 0, 9999);
    

    ImGui::Columns(2,0,false);
    ImGui::SetColumnWidth(0, 150.0f);
    ImGui::SetColumnWidth(1, 150.0f);

    if (ImGui::Button("test connect(Host)"))
    {
        g_UI_State.testcnt.log = "";
        g_UI_State.testcnt.frame = 0;
        is_testHost_open = true;
        LogInfo("test cnt(s)");
        g_connection.StartConnect(true,true);
    }
    TestConnect_Host_UI(&is_testHost_open);
    ImGui::NextColumn();
    if (ImGui::Button("test connect(Guest)"))
    {
        g_UI_State.testcnt.log = "";
        g_UI_State.testcnt.frame = 0;
        is_testGuest_open = true;
        LogInfo("test cnt(h)");
        g_connection.StartConnect(false,true);
    }
    TestConnect_Guest_UI(&is_testGuest_open);
    ImGui::NextColumn();
    if (ImGui::Button("start as Host")) {
        g_connection.StartConnect(true, false);
        LogInfo("start as Host");
        if(!g_is_log)
            is_collapse = false;
    }
    ImGui::NextColumn();
    if (ImGui::Button("start as Guest")){
        g_connection.StartConnect(false, false);
        LogInfo("start as Guest");
        if (!g_is_log)
            is_collapse = false;
    }
    ImGui::NextColumn();
    if (ImGui::Button("stop connection")) {
        g_connection.SendEndConnect();
        g_connection.EndConnect();
    }

    ImGui::Columns(1);
    //ImGui::TextWrapped(g_log.c_str());
    ImGui::Separator();
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Button("show cmd")){    
        AllocConsole();
#pragma warning(push)
#pragma warning(disable:4996)
        freopen("CONOUT$", "w", stdout);
#pragma warning(pop)
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Button("save log"))
    {
        std::fstream fs("latest.log", std::ios::ate | std::ios::out);
        fs << g_log;
        fs.close();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Button("clear log"))
    {
        g_log = "";
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    ImGui::Checkbox("record log", &g_is_log);
    ImGui::End();
}