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
	Guest_Prepared,Guest_Received,Host_Received,End_Game,No_State
};

struct Data_StatePack
{
	int seednum0;
	int cfg_flag[3];
	StatePack_State state;
	Data_StatePack(StatePack_State state);
	Data_StatePack();
};

struct Data_NAK_Pack
{
	int frame;
};

struct Data_KeyState
{
	static constexpr int c_PackKeyStateNum = 20;
	KeyState key_state[c_PackKeyStateNum];
};

struct Pack
{
	int64_t index;
	LARGE_INTEGER time_stamp;
	LARGE_INTEGER time_stamp_echo;
	std::variant <Data_KeyState, Data_StatePack, Data_NAK_Pack> data;
};

enum ConnectState
{
	No_Connection,Host_Prepared,Guest_Prepared,Host_Received, Connected
};

struct P2PConnection
{
	std::queue<Pack> packs_rcved;
	ConnectState connect_state;
	int64_t pack_index;
	bool is_Host;
	bool is_ipv6;

	char address[128];
	int port_Host;
	int port_Guest;
	bool is_blocking;
	union{
		sockaddr_in6 addr_self;
		sockaddr_in addr_self4;
	};
	union
	{
		sockaddr_in6 addr_other;
		sockaddr_in addr_other4;
	};
	SOCKET udp_socket;

	int delay_compensation;
	P2PConnection();
	bool SetUpConnect(bool isHost);
	void EndConnect();
	static bool WSAStartUp();

	Pack CreateEmptyPack();
	int RcvPack();
	
	int SendPack(Data_KeyState data);
	int SendPack(Data_NAK_Pack data);
	int SendPack(Data_StatePack data);
	void SendEndConnect();

	void SendGuestPrepared();
	void SendHostReceived();
	void SendGuestReceived();

};


