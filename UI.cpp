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
extern bool g_force_keyboard;
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
    ImGui::SetNextWindowSize(ImVec2(350.0f, 200.0f));
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowCollapsed(is_collapse);

    const char* wind_capital = "###wind";
    if (g_connection.connect_state == ConnectState::No_Connection)
    {
        wind_capital = "No Connection###wind";
    }else if (g_connection.connect_state == ConnectState::Connected){
        if(g_is_synced)
            wind_capital = "PVP###wind";
        else
            wind_capital = "Desync Probably###wind";
    }else{
        if(g_connection.is_ipv6)
            wind_capital = "(ipv6) Waiting for 2P###wind";
        else
            wind_capital = "(ipv4) Waiting for 2P###wind";
    }
    ImGui::Begin(wind_capital, 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);

    if (g_connection.connect_state == ConnectState::No_Connection){
        g_is_focus_ui = ImGui::IsWindowFocused();
    }else{
        g_is_focus_ui = false;
    }

    is_collapse=ImGui::IsWindowCollapsed();

    int delay = g_connection.delay_compensation;
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("delay", &delay, 1, 5))
    {
        delay = std::clamp(delay, 1, 180);
        if (g_connection.connect_state == ConnectState::No_Connection) {
            g_connection.delay_compensation = delay;
        } else {
            delay = g_connection.delay_compensation;
        }
    }
    if (ImGui::Button("start as host(ipv6)")) {
        g_connection.SetUpConnect_Host(true);
        LogInfo("start as host(v6)");
        if (!g_is_log)
            is_collapse = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("start as host(ipv4)")) {
        g_connection.SetUpConnect_Host(false);
        LogInfo("start as host(v4)");
        if (!g_is_log)
            is_collapse = true;
    }

    ImGui::Separator();

    ImGui::SetNextItemWidth(240.0f);

    if (ImGui::InputText("host IP", g_connection.address_sendto, sizeof(g_connection.address_sendto), 
        g_connection.connect_state==ConnectState::No_Connection ? ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll : ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly
    )){
        g_connection.SetGuestSocketSetting();
    }
    if (g_connection.is_addr_sendto_set){
        ImGui::LabelText(" ###Gip", "%s: %s, port: %d", g_connection.is_ipv6 ? "ipv6" : "ipv4", g_connection.ip_sendto.c_str(), g_connection.port_sendto);
    }else{
        ImGui::LabelText(" ###Gip", "invalid ip address");
    }
    
    if (ImGui::Button("start as guest")) {
        g_connection.SetUpConnect_Guest();
        LogInfo("start as guest");
        if (!g_is_log)
            is_collapse = true;
    }


    
    ImGui::Separator();
    if (ImGui::Button("stop connection")) {
        g_connection.EndConnect();
    }

    ImGui::SameLine();
    static bool adv_settings = false;
    ImGui::Checkbox("advanced settings", &adv_settings);

    

    if (adv_settings)
    {
        ImGui::SetNextWindowSize(ImVec2(250.0f, 160.0f));
        ImGui::SetNextWindowPos(ImVec2(350.0f, 0.0f));
        ImGui::Begin("advanced settings", 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);

        SeedType seed[4];
        CopyFromOriginalSeeds(seed);
        ImGui::LabelText(" ###time", "t=%d", g_ui_frame);
        ImGui::LabelText(" ###seed", "seed=(%d,%d,%d,%d)", g_ui_frame, seed[0], seed[1], seed[2], seed[3]);
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputInt("port for host", &g_connection.port_listen_Host, 1);
        g_connection.port_listen_Host = std::clamp(g_connection.port_listen_Host, 0, 65535);

        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputInt("port for guest", &g_connection.port_send_Guest, 1);
        g_connection.port_send_Guest = std::clamp(g_connection.port_send_Guest, 0, 65535);

        ImGui::Separator();
        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::Button("show cmd")) {
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
    }
    // ImGui::Checkbox("force keyboard", &g_force_keyboard);
    // ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 0.7f, 0.6f, 1.0f), "ver 1.06");
    ImGui::End();



    // for (int i = 0; i < 7; i++) {
    //     auto addr = Address<BYTE>(0x00530ACC + i);
    //     addr.SetValue(0x90);
    // }//invincible
    // *(DWORD*)(0x00607930) = 2500;
    // *(DWORD*)(0x006079F0) = 2500;
    //inf power
}