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
	Host_State,No_State,WAIT_STATE
};

struct Data_StatePack
{
	SeedType seednum[4];
	DWORD cfg_flag[3];
	union
	{
		int delay_compensation;
		int wait_frame;
	};
	StatePack_State state;
	Data_StatePack(StatePack_State state);
	Data_StatePack();
};

struct Data_NAK_KeyState
{
	int frame;
	Data_NAK_KeyState(int f = 0) :frame(f) {};
};

struct Data_KeyState
{
	static constexpr int c_PackKeyStateNum = 8;
	KeyState key_state[c_PackKeyStateNum];
	//KeyState key_state;
};

struct Pack
{
	int64_t index;
	LARGE_INTEGER time_stamp;
	LARGE_INTEGER time_stamp_echo;
	std::variant <Data_KeyState, Data_StatePack, Data_NAK_KeyState> data;
};

enum ConnectState
{
	No_Connection,Host_Listening,Guest_Requesting, Connected
};

struct P2PConnection
{
	static constexpr int max_frame_wait = 180;

	void SetSocketBlocking(SOCKET sock);
	SOCKET GetCurrentTCPSocket();


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

	SOCKET socket_udp;//to send frame states

	int delay_compensation;
	P2PConnection();

	bool SetUpConnect_Guest();
	bool SetUpConnect_Host();

	bool Host_Listening();
	bool Guest_Request();

	void EndConnect();

	
	static bool WSAStartUp();

	Pack CreateEmptyPack();
	int RcvPack(SOCKET sock,bool is_tcp);
	int RcvTCPPack();
	int RcvUDPPack();
	
	int SendPack(SOCKET sock, Data_KeyState data, bool is_tcp);
	int SendUDPPack(Data_KeyState data);
	int SendTCPPack(Data_KeyState data);

	int SendTCP_UDP_Pack(Data_NAK_KeyState data);

	int SendTCPPack(Data_StatePack data);
};


void ConnectLoop();
void HandlePacks();
void __stdcall ConnectLoop_L();

