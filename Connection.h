#pragma once
#include <string>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <variant>
#include <queue>
#include "Key.h"
#include "inih/cpp/INIReader.h"
#pragma comment(lib,"ws2_32.lib")



enum StatePack_State
{
	Host_State,No_State,Wait_State,Guest_Request,Guest_Confirm,
	Host_Sync,Guest_Sync_Confirm
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
	No_Connection,Host_Listening,Guest_Requesting,Host_Confirming,Connected
};

struct P2PConnection
{
	static constexpr int c_max_time_retry_timeout = 32;
	static constexpr int c_time_ms_retry_sync = 2000;

	void SetSocketBlocking(SOCKET sock);

	ConnectState connect_state;
	int64_t pack_index;
	bool is_host;
	bool is_ipv6;

	
	int port_listen_Host;
	int port_send_Guest;

	bool is_addr_guest_sendto_set;
	std::string addr_sendto;
	int port_sendto;

	bool is_blocking;

	std::queue<Pack> packs_rcved;
	union{
		sockaddr_in6 addr_self6;
		sockaddr_in addr_self4;
	};
	union{
		sockaddr_in6 addr_other6;
		sockaddr_in addr_other4;
	};
	SOCKET socket_udp;//to send frame states

	int delay_compensation;
	P2PConnection();

    void LoadSettings();

    bool SetUpConnect_Guest();
	bool SetUpConnect_Host(bool iis_ipv6);


	void EndConnect();

	
	static bool WSAStartUp();

	Pack CreateEmptyPack();
	int RcvUDPPack();
	int SendUDPPack(Data_KeyState data);
	int SendUDPPack(Data_NAK_KeyState data);
	int SendUDPPack(Data_StatePack data);
	bool SetGuestSocketSetting(std::string host_ipaddress, int port_sendto_Guest,bool iis_ipv6);
};


void ConnectLoop();
void HandlePacks();
void __stdcall ConnectLoop_L();

