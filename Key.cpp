#include "pch.h"
#include <windows.h>
#include "UI.h"
#include "Key.h"
#include <XInput.h>
#include "Address.h"
#include "Connection.h"
#include "Utils.h"
#include <unordered_map>
#include <format>
#include <random>
#include "States.h"

#undef max
#undef min
extern P2PConnection g_connection;
extern bool g_is_focus_ui;
extern int g_ui_frame;

extern LARGE_INTEGER g_cur_time;

std::unordered_map<int, KeyState> g_keystate_self;
std::unordered_map<int, KeyState> g_keystate_rcved;


/*
bool IsInGame()
{
    return *(DWORD*)(0x005AE474) && g_cur_frame <= 2 * g_connection.delay_compensation && g_real_frame;
}

bool IsLoading()
{
    return *(DWORD*)(0x005AE474) && (g_real_frame==0);
}


void InitGameValue()
{
    LogInfo("init game value");
    if (g_cur_frame != g_connection.delay_compensation)
    {
        g_cur_frame = g_connection.delay_compensation;
        g_seed_numA = 0;
        g_seed_numB = 0;
        g_keystate_self.clear();
        g_keystate_rcved.clear();
        for (int i = 0; i < g_connection.delay_compensation; i++) {
            g_keystate_self.emplace(i, KeyState{ .frame = i,.state = 0 });
            g_keystate_rcved.emplace(i, KeyState{ .frame = i,.state = 0 });
        }
    }
}

void EnterGame()
{
    InitGameValue();
}
*/

/*
DWORD __fastcall Original_GetRng(DWORD* thiz)
{
    unsigned int v2; // edi
    int v3; // ebx
    thiz[1] += 2;
    v2 = (unsigned __int16)((*(WORD*)thiz ^ 0x9630) - 25939);
    v3 = (unsigned __int16)(((4 * v2 + (v2 >> 14)) ^ 0x9630) - 25939);
    *(WORD*)thiz = 4 * v3 + ((unsigned __int16)(((4 * v2 + (v2 >> 14)) ^ 0x9630) - 25939) >> 14);
    return v3 | (v2 << 16);
}

std::default_random_engine g_random_engineA(0);
std::default_random_engine g_random_engineB(0);
std::default_random_engine g_random_engineC(0);
std::uniform_int_distribution<unsigned int> g_uint_device(0,4294967295);


DWORD __fastcall GetRng(DWORD thiz)
{
    // *(DWORD*)(thiz + 4) += 2;
    // LogInfo(std::format("RNGD for {};{}+{}+({},{})",thiz,        g_connection.seednum0, g_real_frame, g_seed_numA, g_seed_numB));
    // if (thiz == 0x005AE410) {
    //     return std::hash<int>()(g_connection.seednum0 + g_real_frame + ((++g_seed_numA * 114 + 514) ^ 0x1919810) * 114514);
    // }else if (thiz == 0x005AE420) {
    //     return std::hash<int>()(g_connection.seednum0 + g_real_frame + ((++g_seed_numB * 114 + 514) ^ 0x1919810) * 114514);
    // }else{
    //     return std::hash<int>()(g_connection.seednum0 + g_real_frame + ((g_seed_numB * 114 + 514) ^ 0x1919810) * 114514);
    // }
    return Original_GetRng((DWORD*)thiz);
}

SHORT __fastcall GetRng2(DWORD thiz)
{
    // *(DWORD*)(thiz + 4) += 1;
    // LogInfo(std::format("RNGS for {};{}+{}+({},{})", thiz, g_connection.seednum0, g_real_frame, g_seed_numA, g_seed_numB));
    // if (thiz == 0x005AE410) {
    //     return std::hash<int>()(g_connection.seednum0 + g_real_frame + ((++g_seed_numA * 114 + 514) ^ 0x1919810) * 114514);
    // }else if (thiz == 0x005AE420) {
    //     return std::hash<int>()(g_connection.seednum0 + g_real_frame + ((++g_seed_numB * 114 + 514) ^ 0x1919810) * 114514);
    // }else {
    //     return std::hash<int>()(g_connection.seednum0 + g_real_frame + ((g_seed_numB * 114 + 514) ^ 0x1919810) * 114514);
    // }
}
*/

/*
void __fastcall PlayerState(DWORD ecx)
{
    if(ecx==*(DWORD*)(0x005AE474)){
        if (g_real_frame == 0){
            EnterGame();
        }
        g_real_frame++;
        g_seed_numA = 0;
        g_seed_numB = 0;
    }
}

void SetPlayer()
{
    if (*(DWORD*)(0x005AE4B0) == 0)
    {
        //g_connection.seednum0++;
        g_real_frame = 0;
        g_seed_numA = 0;
        g_seed_numB = 0;
    }
}

void SetLife2()
{
    g_real_frame = 0;
    g_seed_numA = 0;
    g_seed_numB = 0;
}
*/

bool IsOnWindow()
{
    return Address<BYTE>(0x609178).GetValue() && (!g_is_focus_ui);
}

DWORD GetKeyStateFull_original(DWORD keyStruct, __int16* keyMapStruct)
{
    if (!IsOnWindow())
        return 0;
    GetKeyboardState((PBYTE)(keyStruct + 720));
    DWORD v1 = keyStruct;
    __int16* v4 = keyMapStruct;
    int v5 = 2                                // 755=end 733=enter 756=home 800=P 802=R
        * (*(BYTE*)(v4[22] + v1 + 720) & 0x80 | (((*(BYTE*)(v1 + 756) | *(BYTE*)(v1 + 800)) & 0x80 | (2 * (*(BYTE*)(v1 + 733) & 0x80 | (4 * ((*(BYTE*)(v1 + 755) | *(BYTE*)(v1 + 802)) & 0x80))))) << 10));
    int ks = (*(char*)(v1 + 823) >> 31) & 0x50 | (*(char*)(v1 + 817) >> 31) & 0x60 | ((unsigned __int8)(*(char*)(v1 + 819) >> 7) | (*(BYTE*)(v1 + 822) | *(BYTE*)(v4[26] + v1 + 720)) & 0xDF) & 0xA0 | ((unsigned __int8)(*(char*)(v1 + 825) >> 7) | ((*(unsigned __int8*)(v1 + 824) | (unsigned int)*(unsigned __int8*)(v4[23] + v1 + 720)) >> 3) & 0x10) & 0x90 | v5 | (((*(BYTE*)(v1 + 820) | *(BYTE*)(v4[25] + v1 + 720)) & 0x80 | (((*(BYTE*)(v1 + 818) | *(BYTE*)(v4[24] + v1 + 720)) & 0x80 | ((*(BYTE*)(v4[21] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[20] + v1 + 720) & 0x80 | (((*(unsigned __int8*)(v4[18] + v1 + 720) >> 1) | *(BYTE*)(v4[19] + v1 + 720) & 0x80u) >> 1)) >> 1)) >> 2)) >> 1)) >> 1);
    return ks;
}

DWORD GetKeyStateP1_original(DWORD keyStruct, __int16* keyMapStruct)
{
    if (!IsOnWindow())
        return 0;
    return GetKeyStateFull_original(keyStruct, keyMapStruct);
    GetKeyboardState((PBYTE)(keyStruct + 720));
    DWORD v1 = keyStruct;
    __int16* v4 = keyMapStruct;
    int ks = *(BYTE*)(v4[35] + v1 + 720) & 0x80 | (2 * (*(BYTE*)(v4[31] + v1 + 720) & 0x80)) | ((*(BYTE*)(v4[34] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[33] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[32] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[30] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[29] + v1 + 720) & 0x80 | (((*(unsigned __int8*)(v4[27] + v1 + 720) >> 1) | *(BYTE*)(v4[28] + v1 + 720) & 0x80u) >> 1)) >> 1)) >> 1)) >> 1)) >> 1)) >> 1);
    return ks;
}

DWORD GetKeyStateP2_original(DWORD keyStruct, __int16* keyMapStruct)
{
    if (!IsOnWindow())
        return 0;
    return GetKeyStateFull_original(keyStruct, keyMapStruct);
    GetKeyboardState((PBYTE)(keyStruct + 720));
    DWORD v1 = keyStruct;
    __int16* v4 = keyMapStruct;
    int ks = *(BYTE*)(v4[44] + v1 + 720) & 0x80 | (2 * (*(BYTE*)(v4[40] + v1 + 720) & 0x80)) | ((*(BYTE*)(v4[43] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[42] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[41] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[39] + v1 + 720) & 0x80 | ((*(BYTE*)(v4[38] + v1 + 720) & 0x80 | (((*(unsigned __int8*)(v4[36] + v1 + 720) >> 1) | *(BYTE*)(v4[37] + v1 + 720) & 0x80u) >> 1)) >> 1)) >> 1)) >> 1)) >> 1)) >> 1);
    return ks;
}


void RemoveKey()
{
    auto p = [](std::pair<int, KeyState> i)->bool {
        if (i.first != i.second.frame)
            LogError(std::format("wrong frame from index, {}:{}",i.first,i.second.frame));
        return i.first < (g_ui_frame - g_connection.delay_compensation*2 - 20) || (i.first > g_ui_frame + g_connection.delay_compensation*2 + 20);
    };
    auto m = [p](std::unordered_map<int, KeyState>& m)
    {
        for (auto i = m.begin(); i != m.end();){
            if (p(*i)){
                i=m.erase(i);
            }else{
                i++;
            }
        }
    };
    m(g_keystate_rcved);
    m(g_keystate_self);
}

DWORD GetCurrentReceivedKey()
{
    if(g_keystate_rcved.contains(g_ui_frame))
        return g_keystate_rcved[g_ui_frame].state;

    LogError(std::format("fail to get cur rcv key : {}",g_ui_frame));
    Delay(1);
    HandlePacks();
    if (!g_keystate_rcved.contains(g_ui_frame)){
        LogInfo("no cur key");
        g_connection.SendTCP_UDP_Pack(Data_NAK_KeyState(g_ui_frame));
    }

    bool is_ahead = true;
    for (int i = 0; i < g_connection.delay_compensation; i++){
        if (g_keystate_rcved.contains(g_ui_frame + 1)){
            is_ahead = false;
        }
    }
    LARGE_INTEGER time_begin;
    QueryPerformanceCounter(&time_begin);
    for(int i=0;i<100000;i++){
        HandlePacks();
        if (g_keystate_rcved.contains(g_ui_frame))
            break;
        if (g_connection.connect_state == ConnectState::No_Connection)
            return 0;
        LARGE_INTEGER time_cur;
        QueryPerformanceCounter(&time_cur);
        if (CalTimePeriod(time_begin,time_cur)>16*(g_connection.delay_compensation+P2PConnection::max_frame_wait)) {
            LogError("fail to Connect");
            g_connection.EndConnect();
            return 0;
        }
    }
    if (is_ahead){
        LogInfo(std::format("ahead delay {} ms", 8 * g_connection.delay_compensation));
        Delay(8 * g_connection.delay_compensation);
    }
    // static int time_no_key = 0;
    // if ((time_no_key++) >=3){
    //     LogInfo("delay 0.3 frame");
    //     Delay(5);
    //     time_no_key = 0;
    // }
    return g_keystate_rcved[g_ui_frame].state;
}

DWORD GetCurrentSelfKey()
{
    if (g_keystate_self.contains(g_ui_frame))
        return g_keystate_self[g_ui_frame].state;
    LogError(std::format("fail to get cur slf key : {}", g_ui_frame));
    return 0;
}

DWORD GetKeyStateP1(DWORD keyStruct, __int16* keyMapStruct)
{
    if (g_connection.connect_state==ConnectState::No_Connection){
        return GetKeyStateP1_original(keyStruct,keyMapStruct);
    }else if(g_connection.connect_state == ConnectState::Connected) {
        if (g_connection.is_host){
            return GetCurrentSelfKey();
        }else{
            return GetCurrentReceivedKey();
        }
    }
    return 0;
}
DWORD GetKeyStateP2(DWORD keyStruct, __int16* keyMapStruct)
{
    if (g_connection.connect_state == ConnectState::No_Connection)
    {
        return GetKeyStateP2_original(keyStruct, keyMapStruct);
    }else if (g_connection.connect_state == ConnectState::Connected) {
        if (g_connection.is_host){
            return GetCurrentReceivedKey();
        }else {
            return GetCurrentSelfKey();
        }
    }
    return 0;
}

DWORD GetKeyStateFull(DWORD keyStruct, __int16* keyMapStruct)
{
    if (g_connection.connect_state == ConnectState::No_Connection){
        return GetKeyStateFull_original(keyStruct, keyMapStruct);
    }
    if (g_connection.connect_state == ConnectState::Connected){
        if (g_connection.is_host){
            return GetCurrentSelfKey();
        }else{
            return GetCurrentReceivedKey();
        }
    }
    return 0;
}

void RecordKey(DWORD keyStruct, __int16* keyMapStruct)
{
    switch (*(DWORD*)keyStruct)
    {
    case 0:{
            DWORD ks = 0;
            if (IsOnWindow()){
                GetKeyboardState((PBYTE)(keyStruct + 720));
                ks=GetKeyStateFull_original(keyStruct, keyMapStruct);
            }
            int real_cur_frame = g_ui_frame + g_connection.delay_compensation;
            if (!g_keystate_self.contains(real_cur_frame)){
                KeyState state = { 0 };
                state.state = ks;
                state.frame = real_cur_frame;
                CopyFromOriginalSeeds(state.seednum);
                g_keystate_self[real_cur_frame]=state;
            }
            RemoveKey();
            return;
        }
        break;
    case 3:
        return;
    case 4:
        return;
    default:
        return;
    }
}


int __fastcall MyGetKeyState(DWORD thiz) {
    QueryPerformanceCounter(&g_cur_time);

    //set control
    *(DWORD*)(0x0060860C) = 1;
    *(DWORD*)(0x00608610) = 2;


    int v1; // ebx
    unsigned int v2; // esi
    int v3; // ecx
    __int16* v4; // edi
    //int v5; // edx
    int v6; // esi
    int v7; // edx
    int v8; // ecx
    int v9; // edx
    int v10; // ecx
    int v11; // edx
    int v12; // ecx
    int v13; // edx
    int v14; // eax
    bool v15; // zf
    WORD v16; // ax
    int v17; // esi
    int v18; // eax
    __int16 v20; // ax
    __int16 v21; // ax
    __int16 v22; // ax
    __int16 v23; // ax
    int v24; // ecx
    int v25; // ecx
    int v26; // eax
    int v27; // ecx
    int v28; // eax
    int i; // eax
    DWORD v30; // [esp-8h] [ebp-148h]
    int v31; // [esp+Ch] [ebp-134h]
    int v33; // [esp+14h] [ebp-12Ch]
    int v34; // [esp+14h] [ebp-12Ch]
    int v35[69]; // [esp+18h] [ebp-128h] BYREF
    XINPUT_STATE pState; // [esp+12Ch] [ebp-14h] BYREF
    v1 = thiz;
    v2 = 0;
    v31 = thiz + 720;
    memset((void*)(thiz + 720), 0, 0x100u);
    v3 = 0;
    if (*(DWORD*)(Address<DWORD>(0x5AE3A0).GetValue() + 0x2E28) == *(DWORD*)(v1 + 4))
        v3 = 90;
    v4 = (__int16*)(v3 + Address<DWORD>(0x5AE3A0).GetValue() + 0x2E38);


    if (g_connection.connect_state == ConnectState::Connected){
        RecordKey(v1, v4);
    }

    switch (*(DWORD*)v1)
    {
    case 0:
    case 3:
    case 4:
        //if (IsOnWindow())//is on the window?
        {
            if (*(DWORD*)v1){
                if (*(DWORD*)v1 == 3){
                    v2 = GetKeyStateP1(v1, v4);
                } else if (*(DWORD*)v1 == 4) {
                    v2 = GetKeyStateP2(v1, v4);
                }
            }else{
                v2 = GetKeyStateFull(v1, v4);
                
            }
        }
        memset((void*)(thiz + 720), 0, 0x100u);
        goto LABEL_96;
    case 1:
        if ((*(int(__stdcall**)(DWORD))(**(DWORD**)(v1 + 8) + 100))(*(DWORD*)(v1 + 8)) < 0){
            v17 = 0;
            if ((*(int(__stdcall**)(DWORD))(**(DWORD**)(v1 + 8) + 28))(*(DWORD*)(v1 + 8)) == -2147024866){
                while (1){
                    v18 = (*(int(__stdcall**)(DWORD))(**(DWORD**)(v1 + 8) + 28))(*(DWORD*)(v1 + 8));
                    if (++v17 >= 400)
                        break;
                    if (v18 != -2147024866) {
                        void(__fastcall * sub_402310)(DWORD thiz, int a2);
                        sub_402310 = (decltype(sub_402310))(0x402310);
                        sub_402310(0x607728, 15);
                        return 0;
                    }
                }
            }
            goto LABEL_70;
        }
        memset(v35, 0, 0x110u);
        if ((*(int(__stdcall**)(DWORD, int, int*))(**(DWORD**)(v1 + 8) + 36))(*(DWORD*)(v1 + 8), 272, v35) < 0)
        {
        LABEL_70:
            void(__fastcall * sub_402310)(DWORD thiz, int a2);
            sub_402310 = (decltype(sub_402310))(0x402310);
            sub_402310(0x607728, 15);
            return 0;
        }
        if (*v4 >= 0)
            v2 = *((unsigned __int8*)&v35[12] + *v4) >> 7;
        v20 = v4[1];
        if (v20 >= 0)
            v2 |= (*((unsigned __int8*)&v35[12] + v20) >> 6) & 2;
        v21 = v4[2];
        if (v21 >= 0)
            v2 |= (*((unsigned __int8*)&v35[12] + v21) >> 5) & 4;
        v22 = v4[4];
        if (v22 >= 0)
            v2 |= 2 * (*((BYTE*)&v35[12] + v22) & 0x80);
        v23 = v4[3];
        if (v23 >= 0)
            v2 |= (*((unsigned __int8*)&v35[12] + v23) >> 4) & 8;
        v24 = 0;
        if (v35[0] < -Address<WORD>(0x608614).GetValue())
            v24 = 64;
        v33 = v24;
        v25 = 0;
        if (v35[1] < -Address<WORD>(0x608616).GetValue())
            v25 = 16;
        v26 = 0;
        v34 = v25 | v33;
        if (v35[0] > Address<WORD>(0x608614).GetValue())
            v26 = 128;
        v27 = v26 | v34;
        v28 = 0;
        if (v35[1] > Address<WORD>(0x608616).GetValue())
            v28 = 32;
        v2 |= v28 | v27;
        for (i = 0; i < 32; ++i)
        {
            if (*((BYTE*)&v35[12] + i))
                *(BYTE*)(v31 + i) = 0x80;
        }
        v1 = thiz;
    LABEL_94:
        if (v2)
            *(DWORD*)(Address<DWORD>(0x5AE3A0).GetValue() + 24) = *(DWORD*)v1;
    LABEL_96:
        *(DWORD*)(v1 + 20) = *(DWORD*)(v1 + 16);
        *(DWORD*)(v1 + 16) = v2;

        void(__fastcall * sub_402730)(unsigned int* thiz);
        sub_402730 = (decltype(sub_402730))(0x402730);
        sub_402730((unsigned int*)(v1 + 16));
        if (Address<BYTE>(0x6078B8).GetValue())
        {
            LeaveCriticalSection((LPCRITICAL_SECTION)0x607890);
            Address<BYTE>(0x6078B7).SetValue(Address<BYTE>(0x6078B7).GetValue());
        }
        return v2;
    case 2:
        v30 = *(DWORD*)(v1 + 12);
        memset(&pState, 0, sizeof(pState));
        goto LABEL_96;
        if (XInputGetState(v30, &pState))
            goto LABEL_96;
        v6 = pState.Gamepad.wButtons;
        if (pState.Gamepad.bLeftTrigger >= 0x1Eu)
            v6 = pState.Gamepad.wButtons | 0x10000;
        if (pState.Gamepad.bRightTrigger >= 0x1Eu)
            v6 |= 0x20000u;
        if (pState.Gamepad.sThumbLY >= 7849)
            v6 |= 1u;
        if (pState.Gamepad.sThumbLY <= -7849)
            v6 |= 2u;
        if (pState.Gamepad.sThumbLX >= 7849)
            v6 |= 8u;
        if (pState.Gamepad.sThumbLX <= -7849)
            v6 |= 4u;
        v7 = (16 * (v6 & 1)) | 0x20;
        if ((v6 & 2) == 0)
            v7 = 16 * (v6 & 1);
        v8 = v7 | 0x40;
        if ((v6 & 4) == 0)
            v8 = v7;
        v9 = v8 | 0x80;
        if ((v6 & 8) == 0)
            v9 = v8;
        v10 = v9 | 1;
        {
            DWORD* dword_5743E8 = *(DWORD**)(0x5743E8);
            if ((v6 & dword_5743E8[v4[9]]) == 0)
                v10 = v9;
            v11 = v10 | 2;
            if ((v6 & dword_5743E8[v4[10]]) == 0)
                v11 = v10;
            v12 = v11 | 4;
            if ((v6 & dword_5743E8[v4[11]]) == 0)
                v12 = v11;
            v13 = v12 | 8;
            if ((v6 & dword_5743E8[v4[12]]) == 0)
                v13 = v12;
            v14 = v6 & dword_5743E8[v4[13]];
        }
        v2 = v13 | 0x100;
        v15 = v14 == 0;
        v16 = pState.Gamepad.wButtons;
        if (v15)
            v2 = v13;
        if ((pState.Gamepad.wButtons & 0x1000) != 0)
            *(BYTE*)(v1 + 720) = 0x80;
        if ((v16 & 0x2000) != 0)
            *(BYTE*)(v1 + 721) = 0x80;
        if ((v16 & 0x4000) != 0)
            *(BYTE*)(v1 + 722) = 0x80;
        if ((v16 & 0x8000) != 0)
            *(BYTE*)(v1 + 723) = 0x80;
        if ((v16 & 0x40) != 0)
            *(BYTE*)(v1 + 728) = 0x80;
        if ((v16 & 0x80u) != 0)
            *(BYTE*)(v1 + 729) = 0x80;
        if ((v16 & 0x100) != 0)
            *(BYTE*)(v1 + 724) = 0x80;
        if ((v16 & 0x200) != 0)
            *(BYTE*)(v1 + 725) = 0x80;
        if ((v16 & 0x10) != 0)
            *(BYTE*)(v1 + 730) = 0x80;
        if ((v16 & 0x20) != 0)
            *(BYTE*)(v1 + 731) = 0x80;
        if (pState.Gamepad.bLeftTrigger >= 0x1Eu)
            *(BYTE*)(v1 + 726) = 0x80;
        if (pState.Gamepad.bRightTrigger >= 0x1Eu)
            *(BYTE*)(v1 + 727) = 0x80;
        goto LABEL_94;
    default:
        goto LABEL_96;
    }
}
