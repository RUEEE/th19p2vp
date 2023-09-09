#pragma once
#include <string>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <variant>
#include <queue>
#include "Key.h"
#pragma comment(lib,"ws2_32.lib")



enum StatePack_State
{
	Host_State,No_State
};

struct Data_StatePack
{
	SeedType seednum[4];
	DWORD cfg_flag[3];
	int delay_compensation;
	StatePack_State state;
	Data_StatePack(StatePack_State state);
	Data_StatePack();
};


struct Data_KeyState
{
	static constexpr int c_PackKeyStateNum = 1;
	KeyState key_state[c_PackKeyStateNum];
	//KeyState key_state;
};

struct Pack
{
	int64_t index;
	LARGE_INTEGER time_stamp;
	LARGE_INTEGER time_stamp_echo;
	std::variant <Data_KeyState, Data_StatePack> data;
};

enum ConnectState
{
	No_Connection,Host_Listening,Guest_Requesting, Connected
};

struct P2PConnection
{
	static constexpr int max_frame_wait = 180;

	void SetSocketBlocking(SOCKET sock);
	SOCKET GetCurrentSocket();


	ConnectState connect_state;
	int64_t pack_index;
	bool is_host;
	bool is_ipv6;

	char address[128];
	int port_Host;
	int port_Guest;
	bool is_blocking;

	std::queue<Pack> packs_rcved;
	union{
		sockaddr_in6 addr_self6;
		sockaddr_in addr_self4;
	};
	union
	{
		sockaddr_in6 addr_other6;
		sockaddr_in addr_other4;
	};
	SOCKET socket_listen_host;
	SOCKET socket_guest;
	SOCKET socket_host;

	int delay_compensation;
	P2PConnection();

	bool SetUpConnect_Guest();
	bool SetUpConnect_Host();

	bool Host_Listening();
	bool Guest_Request();

	void EndConnect();

	
	static bool WSAStartUp();

	Pack CreateEmptyPack();
	int RcvPack();
	
	int SendPack(Data_KeyState data);
	int SendPack(Data_StatePack data);
};


void ConnectLoop();
void HandlePacks();
void __stdcall ConnectLoop_L();

