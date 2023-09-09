#include "Connection.h"
#include "Key.h"
#include <unordered_map>
#include <format>
#include "Utils.h"

P2PConnection g_connection;

extern std::unordered_map<int, KeyState> g_keystate_self;
extern std::unordered_map<int, KeyState> g_keystate_rcved;
extern int g_cur_frame;


P2PConnection::P2PConnection():
	port_Host(9961),port_Guest(9962), delay_compensation(5), 
	is_Host(true), is_ipv6(false),pack_index(0),is_blocking(true),
	connect_state(ConnectState::No_Connection),seednum0(timeGetTime()),is_started(false)
{
	// QueryPerformanceCounter(&last_init_value);
	strcpy_s(address, "127.0.0.1");
	memset(&addr_self, 0, sizeof(addr_self));
	memset(&addr_other, 0, sizeof(addr_other));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
	memset(&udp_socket, 0, sizeof(udp_socket));
}

bool P2PConnection::StartConnect(bool isHost,bool is_test)
{
	seednum0++;
	is_Host = isHost;
	int res;
	if (!is_started)
	{
		if (is_Host)
		{
			if (strstr(address, ":") != NULL) {
				is_ipv6 = true;
				addr_self = { AF_INET6, htons(port_Host) };
				udp_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
				res = bind(udp_socket, (SOCKADDR*)&(addr_self), sizeof(addr_self));
				LogInfo(std::format("ipv6 Host"));
			}
			else {//ipv4
				is_ipv6 = false;
				addr_self4 = { AF_INET, htons(port_Host) };
				udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				res = bind(udp_socket, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));
				LogInfo(std::format("ipv4 Host"));
			}
		}
		else {//Guest
			if (strstr(address, ":") != NULL)
			{
				is_ipv6 = true;
				addr_other = { AF_INET6, htons(port_Host) };
				inet_pton(AF_INET6, address, &(addr_other.sin6_addr));

				addr_self = { AF_INET6, htons(port_Guest) };
				udp_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
				res = bind(udp_socket, (SOCKADDR*)&(addr_self), sizeof(addr_self));
				LogInfo(std::format("ipv6 Guest,to {}", address));
			}
			else {//ipv4
				is_ipv6 = false;
				addr_other4 = { AF_INET, htons(port_Host) };
				inet_pton(AF_INET, address, &(addr_other4.sin_addr));

				addr_self4 = { AF_INET, htons(port_Guest) };
				udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				res = bind(udp_socket, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));
				LogInfo(std::format("ipv4 Guest,to {}", address));
			}
		}
		if (res != 0) {
			LogError(std::format("fail to create socket : {}", WSAGetLastError()));
			return false;
		}
		if (is_blocking)
		{
			unsigned int timeout = 1;
			setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&timeout), sizeof(timeout));
			setsockopt(udp_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)(&timeout), sizeof(timeout));
		}
		else {
			u_long o = 1;
			ioctlsocket(udp_socket, FIONBIO, &o);
		}
		is_started = true;
	}
	if (!is_test) {
		LogInfo(std::format("start"));
		connect_state = isHost ? ConnectState::Host_Started : ConnectState::All_Started;
		InitGameValue();
		if (!isHost)
			SendGuestPrepared();
	}
	return true;
}

void P2PConnection::EndConnect()
{
	LogInfo("end connect");
	connect_state = ConnectState::No_Connection;
	closesocket(udp_socket);
	is_started = false;
	memset(&addr_self, 0, sizeof(addr_self));
	memset(&addr_other, 0, sizeof(addr_other));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
	memset(&udp_socket, 0, sizeof(udp_socket));
}

bool P2PConnection::SetUpConnect()
{
	LogInfo("set up1");
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		LogError(std::format("fail to init wsa : {}", WSAGetLastError()));
		return false;
	}
	LogInfo("set up2");
	return true;
}

int P2PConnection::SendPack(Pack data)
{
	data.index = pack_index++;
	int l_nLen = 0;
	if (is_ipv6 && addr_other.sin6_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(data), sizeof(data), 0, (SOCKADDR*)&(addr_other), sizeof(addr_other));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(data), sizeof(data), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0)
	{
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}else if (l_nLen > 0)
	{
		if (data.type == Pack_Type::State_Pack)
		{
			switch (data.state_pack.state)
			{
			case StatePack_State::End_Connection:
				LogInfo(std::format("[snded] {:<5} SEndConnection", data.index)); break;
			case StatePack_State::Entering_Game:
				LogInfo(std::format("[snded] {:<5} SEnteringGame", data.index)); break;
			case StatePack_State::Missing_Frame:
				LogInfo(std::format("[snded] {:<5} SMissingFrame {}", data.index,data.state_pack.frame)); break;
			case StatePack_State::Guest_Prepared:
				LogInfo(std::format("[snded] {:<5} SGuestPrepared", data.index)); break;
			case StatePack_State::Should_Wait:
				LogInfo(std::format("[rcved] {:<5} SShouldWait", data.index)); break;
			default:
				LogInfo(std::format("[snded] {:<5} SUnknown", data.index)); break;
			}
		}else if (data.type == Pack_Type::Operation_Pack) {
			// if(data.key_state_pack.key_state.state!=0)
				LogInfo(std::format("[snded] {:<5} SOp frame {:<6}", data.index, data.key_state_pack.key_state[0].frame));
		}else{
			LogInfo(std::format("[snded] {:<5} STest" ,data.index));
		}
	}
	return l_nLen;
}

void P2PConnection::SendEndConnect()
{
	Pack end;
	end.type = Pack_Type::State_Pack;
	end.state_pack.frame = 0;
	end.state_pack.state = StatePack_State::End_Connection;
	SendPack(end);
}

void P2PConnection::SendGuestPrepared()
{
	Pack Guest_prepared;
	Guest_prepared.type = Pack_Type::State_Pack;
	Guest_prepared.state_pack.frame = 0;
	Guest_prepared.state_pack.state = StatePack_State::Guest_Prepared;
	//Guest_prepared.state_pack.seed = *(DWORD*)(0x005AE420);
	Guest_prepared.state_pack.cfg_flag[0] = *(DWORD*)(0x00608644);
	Guest_prepared.state_pack.cfg_flag[1] = *(DWORD*)(0x00608648);
	Guest_prepared.state_pack.cfg_flag[2] = *(DWORD*)(0x0060864C);
	Guest_prepared.state_pack.seednum0 = seednum0;
	SendPack(Guest_prepared);
}

void P2PConnection::SendEnteringGame()
{
	Pack enteringgame;
	enteringgame.type = Pack_Type::State_Pack;
	enteringgame.state_pack.frame = 0;
	enteringgame.state_pack.state = StatePack_State::Entering_Game;
	// Guest_prepared.state_pack.seed = *(DWORD*)(0x005AE420);
	SendPack(enteringgame);
}

void P2PConnection::SendShouldWait()
{
	Pack shouldwait;
	shouldwait.type = Pack_Type::State_Pack;
	shouldwait.state_pack.frame = g_cur_frame;
	shouldwait.state_pack.state = StatePack_State::Should_Wait;
	SendPack(shouldwait);
}

int P2PConnection::RcvPack(Pack* pData)
{
	int l_nReadLen = 0;
	int sz = 0;
	if (is_ipv6)
		sz = sizeof(addr_other),
		l_nReadLen = recvfrom(udp_socket, (char*)(pData), sizeof(Pack), 0, (SOCKADDR*)&(addr_other), &sz);
	else
		sz = sizeof(addr_other4),
		l_nReadLen = recvfrom(udp_socket, (char*)(pData), sizeof(Pack), 0, (SOCKADDR*)&(addr_other4), &sz);

	if (l_nReadLen > 0){
		if (pData->type == Pack_Type::State_Pack)
		{
			switch (pData->state_pack.state)
			{
			case StatePack_State::End_Connection:
				LogInfo(std::format("[rcved] {:<5} REndConnection", pData->index)); break;
			case StatePack_State::Entering_Game:
				LogInfo(std::format("[rcved] {:<5} REnteringGame", pData->index)); break;
			case StatePack_State::Missing_Frame:
				LogInfo(std::format("[rcved] {:<5} RMissingFrame {}", pData->index, pData->state_pack.frame)); break;
			case StatePack_State::Guest_Prepared:
				LogInfo(std::format("[rcved] {:<5} RGuestPrepared", pData->index)); break;
			case StatePack_State::Should_Wait:
				LogInfo(std::format("[rcved] {:<5} RShouldWait", pData->index)); break;
			default:
				LogInfo(std::format("[rcved] {:<5} RUnknown", pData->index)); break;
			}
		}else if(pData->type==Pack_Type::Operation_Pack){
			// if(pData->key_state_pack.key_state.state !=0)
				LogInfo(std::format("[rcved] {:<5} ROp frame {:<6}",pData->index, pData->key_state_pack.key_state[0].frame));
		}else{
			LogInfo(std::format("[rcved] {:<5} RTest" ,pData->index));
		}
	}else if (l_nReadLen < 0){
		int  err = WSAGetLastError();
		if (err != 10035 && err!= 10060)
		{
			LogError(std::format("receive error: {}", WSAGetLastError()));
			SendEndConnect();
			EndConnect();
		}
	}
	return l_nReadLen;
}
