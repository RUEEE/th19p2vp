#pragma once
#include <string>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "Key.h"
#pragma comment(lib,"ws2_32.lib")

enum Pack_Type
{
	Test_Pack, Operation_Pack , State_Pack
};
enum StatePack_State
{
	Guest_Prepared,Missing_Frame,Entering_Game,End_Connection,Should_Wait
};

struct Pack
{
	static constexpr int c_PackKeyStateNum = 20;
	int64_t index;
	Pack_Type type;
	union
	{
		struct {
			LARGE_INTEGER time_stamp;
		}test_pack;
		struct{
			int frame;
			int seednum0;
			int cfg_flag[3];
			StatePack_State state;
		}state_pack;
		struct
		{
			KeyState key_state[c_PackKeyStateNum];
		}key_state_pack;
	};
};

enum ConnectState
{
	No_Connection, Host_Started, All_Started
};

struct P2PConnection
{
	// LARGE_INTEGER last_init_value;

	bool is_started;

	int seednum0;
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
	bool StartConnect(bool isHost,bool is_test);
	void EndConnect();
	static bool SetUpConnect();

	int RcvPack(Pack* pData);
	int SendPack(Pack data);
	void SendEndConnect();
	void SendGuestPrepared();
	void SendEnteringGame();
	void SendShouldWait();
};


