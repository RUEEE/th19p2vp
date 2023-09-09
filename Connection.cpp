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
	connect_state(ConnectState::No_Connection)
{
	// QueryPerformanceCounter(&last_init_value);
	strcpy_s(address, "127.0.0.1");
	memset(&addr_self, 0, sizeof(addr_self));
	memset(&addr_other, 0, sizeof(addr_other));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
	memset(&udp_socket, 0, sizeof(udp_socket));
}

bool P2PConnection::SetUpConnect(bool isHost)
{
	is_Host = isHost;
	int res;
	if (this->connect_state==No_Connection){
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
	}
	LogInfo(std::format("set up connect"));
	connect_state = isHost ? ConnectState::Host_Prepared : ConnectState::Guest_Prepared;
	return true;
}

void P2PConnection::EndConnect()
{
	LogInfo("end connect");
	connect_state = ConnectState::No_Connection;
	closesocket(udp_socket);
	memset(&addr_self, 0, sizeof(addr_self));
	memset(&addr_other, 0, sizeof(addr_other));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
	memset(&udp_socket, 0, sizeof(udp_socket));
}

bool P2PConnection::WSAStartUp()
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

int P2PConnection::SendPack(Data_KeyState data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other.sin6_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other), sizeof(addr_other));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0)
	{
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}else if (l_nLen > 0){
		LogInfo(std::format("[snded] {:<5} SOp frame {:<6}", pack.index, data.key_state));
	}
	return l_nLen;
}

int P2PConnection::SendPack(Data_NAK_Pack data)
{
	Pack pack= this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other.sin6_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other), sizeof(addr_other));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0){
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		LogInfo(std::format("[snded] {:<5} SNAK {}", pack.index, data.frame));
	}
	return l_nLen;
}

int P2PConnection::SendPack(Data_StatePack data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other.sin6_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other), sizeof(addr_other));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(udp_socket, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0){
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		switch (data.state)
		{
		case StatePack_State::End_Game:
			LogInfo(std::format("[snded] {:<5} SEnd", pack.index)); break;
		case StatePack_State::Guest_Prepared:
			LogInfo(std::format("[snded] {:<5} SGuestPrepared", pack.index)); break;
		case StatePack_State::Host_Received:
			LogInfo(std::format("[snded] {:<5} SHostRcvd", pack.index)); break;
		case StatePack_State::Guest_Received:
			LogInfo(std::format("[snded] {:<5} SGuestRcvd", pack.index)); break;
		default:
			LogInfo(std::format("[snded] {:<5} S???", pack.index)); break;
		}
	}
	return l_nLen;
}



void P2PConnection::SendEndConnect()
{
	Data_StatePack stp;
	stp.state = StatePack_State::End_Game;
	SendPack(stp);
}

void P2PConnection::SendGuestPrepared()
{
	SendPack(Data_StatePack(StatePack_State::Guest_Prepared));
}

void P2PConnection::SendHostReceived()
{
	SendPack(Data_StatePack(StatePack_State::Host_Received));
}

void P2PConnection::SendGuestReceived()
{
	SendPack(Data_StatePack(StatePack_State::Guest_Received));
}

Pack P2PConnection::CreateEmptyPack()
{
	Pack pack;
	pack.index = this->pack_index++;
}

int P2PConnection::RcvPack()
{
	Pack pack;
	int l_nReadLen = 0;
	int sz = 0;
	if (is_ipv6)
		sz = sizeof(addr_other),
		l_nReadLen = recvfrom(udp_socket, (char*)(&pack), sizeof(Pack), 0, (SOCKADDR*)&(addr_other), &sz);
	else
		sz = sizeof(addr_other4),
		l_nReadLen = recvfrom(udp_socket, (char*)(&pack), sizeof(Pack), 0, (SOCKADDR*)&(addr_other4), &sz);
	if (l_nReadLen > 0){
		Data_KeyState *pks = std::get_if<Data_KeyState>(&(pack.data));
		Data_NAK_Pack *pnak = std::get_if<Data_NAK_Pack>(&(pack.data));
		Data_StatePack *pst = std::get_if<Data_StatePack>(&(pack.data));
		if (pst){
			switch (pst->state)
			{
				case StatePack_State::End_Game:
					LogInfo(std::format("[rcved] {:<5} REnd", pack.index)); break;
				case StatePack_State::Guest_Prepared:
					LogInfo(std::format("[rcved] {:<5} RGuestPrepared", pack.index)); break;
				case StatePack_State::Host_Received:
					LogInfo(std::format("[rcved] {:<5} RHostRcvd", pack.index)); break;
				case StatePack_State::Guest_Received:
					LogInfo(std::format("[rcved] {:<5} RGuestRcvd", pack.index)); break;
				default:
					LogInfo(std::format("[rcved] {:<5} R???", pack.index)); break;
			}
		}else if(pks){
			// if(pData->key_state_pack.key_state.state !=0)
				LogInfo(std::format("[rcved] {:<5} ROp frame {:<6}", pack.index, pks->key_state[0].frame));
		}else if(pnak){
			LogInfo(std::format("[rcved] {:<5} SNAK {}", pack.index, pnak->frame));
		}else{
			LogInfo(std::format("[rcved] {:<5} R????", pack.index));
		}
		packs_rcved.push(pack);
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

Data_StatePack::Data_StatePack(StatePack_State state)
{
	this->state = state;
	switch (state)
	{
	default:
	case StatePack_State::No_State:
	case StatePack_State::Guest_Prepared:
	case StatePack_State::Guest_Received:
	case StatePack_State::End_Game:
		seednum0 = 0;
		break;
	case StatePack_State::Host_Received:
		seednum0 = 0;
		cfg_flag[0] = *(DWORD*)(0x00608644);
		cfg_flag[1] = *(DWORD*)(0x00608648);
		cfg_flag[2] = *(DWORD*)(0x0060864C);
		break;
	}
}
Data_StatePack::Data_StatePack():seednum0(0),state(StatePack_State::No_State){}
