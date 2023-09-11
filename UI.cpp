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
#include "States.h"


extern P2PConnection g_connection;
extern std::string g_log;
extern int g_ui_frame;
extern bool g_is_log;
bool g_is_focus_ui;
extern bool g_is_synced;
struct UI_State
{
    struct {
        std::string log;
        int frame;
    }testcnt;
}g_UI_State;



void SetUI(IDirect3DDevice9* device)
{
    static bool is_testGuest_open = false;
    static bool is_testHost_open = false;
    static bool is_collapse = false;

    //ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 200.0f), ImVec2(320.0f, 200.0f));
    ImGui::SetNextWindowSize(ImVec2(365.0f, 175.0f));
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowCollapsed(is_collapse);
    if (g_connection.connect_state == ConnectState::No_Connection)
    {
        ImGui::Begin("No Connection###wind",0,ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
    }else if (g_connection.connect_state == ConnectState::Connected){
        if(g_is_synced)
            ImGui::Begin("PVP###wind", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
        else
            ImGui::Begin("Desync Probably###wind", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
    }else{
        if(g_connection.is_ipv6)
            ImGui::Begin("(ipv6) Waiting for P2###wind", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
        else
            ImGui::Begin("(ipv4) Waiting for P2###wind", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);
    }
    if (g_connection.connect_state == ConnectState::No_Connection){
        g_is_focus_ui = ImGui::IsWindowFocused();
    }else{
        g_is_focus_ui = false;
    }

    
    is_collapse=ImGui::IsWindowCollapsed();

    SeedType seed[4];
    CopyFromOriginalSeeds(seed);
    ImGui::TextColored(ImVec4(0.3f,0.7f,0.6f,1.0f),"ver 1.01");
    ImGui::SameLine();
    ImGui::LabelText(" ", "t=%d; seed=(%d,%d,%d,%d)", g_ui_frame,seed[0],seed[1],seed[2],seed[3]);

    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputText("Host IP  ", g_connection.address, sizeof(g_connection.address));

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputInt("delay", &g_connection.delay_compensation, 1,5);
    g_connection.delay_compensation = std::clamp(g_connection.delay_compensation, 1, 180);

    
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputInt("Host port", &g_connection.port_Host, 1);
    g_connection.port_Host = std::clamp(g_connection.port_Host, 0, 65535);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputInt("Guest port", &g_connection.port_Guest, 1);
    g_connection.port_Guest = std::clamp(g_connection.port_Guest, 0, 65535);
    

    ImGui::Columns(2,0,false);
    ImGui::SetColumnWidth(0, 150.0f);
    ImGui::SetColumnWidth(1, 150.0f);

  
    if (ImGui::Button("start as Host")) {
        g_connection.SetUpConnect_Host();
        LogInfo("start as Host");
        if(!g_is_log)
            is_collapse = false;
    }
    ImGui::NextColumn();
    if (ImGui::Button("start as Guest")){
        g_connection.SetUpConnect_Guest();
        LogInfo("start as Guest");
        if (!g_is_log)
            is_collapse = false;
    }
    
    ImGui::NextColumn();
    if (ImGui::Button("stop connection")) {
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
#pragma warning(disable:6031)
        freopen("CONOUT$", "w", stdout);
        std::ios::sync_with_stdio(0);
#pragma warning(pop)
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Button("save log"))
    {
        std::fstream fs("latest.log", std::ios::ate | std::ios::out);
        fs << g_log;
        fs.close();
        g_log = "";
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    ImGui::Checkbox("log", &g_is_log);
    ImGui::End();



    // for (int i = 0; i < 7; i++) {
    //     auto addr = Address<BYTE>(0x00530ACC + i);
    //     addr.SetValue(0x90);
    // }//invincible
    // *(DWORD*)(0x00607930) = 2500;
    // *(DWORD*)(0x006079F0) = 2500;
    //inf power
}