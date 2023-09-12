#include "Connection.h"
#include "Key.h"
#include <unordered_map>
#include <format>
#include "Utils.h"
#include "States.h"
#include <set>

P2PConnection g_connection;

extern std::unordered_map<int, KeyState> g_keystate_self;
extern std::unordered_map<int, KeyState> g_keystate_rcved;
int g_ui_frame;
bool g_is_synced = true;

bool g_is_loading = false;


void SetHostState(Data_StatePack statedata)
{
	CopyToOriginalSeeds(statedata.seednum);
	VALUED(0x00608644) = statedata.cfg_flag[0];
	VALUED(0x00608648) = statedata.cfg_flag[1];
	VALUED(0x0060864C) = statedata.cfg_flag[2];
	g_connection.delay_compensation = statedata.delay_compensation;
}

void StartConnection(bool playSE)
{
	if (playSE){
		__asm
		{
			push 0
			push 15 // SE: 
			mov ecx, 0x601E50
			mov eax,0x4AEB20
			call eax
		}
	}
	g_is_synced = true;
	g_connection.packs_rcved = {};
	g_keystate_self = {};
	g_keystate_rcved = {};
	KeyState ks;
	ks.state = 0;
	CopyFromOriginalSeeds(ks.seednum);
	for (int i = 0; i < g_connection.delay_compensation; i++){
		ks.frame = i;
		g_keystate_rcved[i] = ks;
		g_keystate_self[i]=ks;
	}
	g_ui_frame = 0;
}

void SendKeyStates(int frame)
{
	static std::unordered_map<int, LARGE_INTEGER> sended_frame_udp;
	static int last_frame = -1;
	if (last_frame != g_ui_frame) {
		last_frame = g_ui_frame;
		sended_frame_udp = {};
	}
	LARGE_INTEGER cur_time;
	GetTime(&cur_time);
	if (sended_frame_udp.contains(frame))
	{
		auto last_send_time = sended_frame_udp[frame];
		if (CalTimePeriod(last_send_time, cur_time) < 5)
			return;
	}
	Data_KeyState ks;
	if (!g_keystate_self.contains(frame)){
		LogInfo(std::format("frame ahead {}:{}",frame,g_ui_frame));
		Data_StatePack wait(StatePack_State::Wait_State);
		wait.wait_frame = std::clamp(frame-g_ui_frame-1,1,g_connection.delay_compensation / 2);
		g_connection.SendUDPPack(wait);
	}
	if (frame >= g_ui_frame - Data_KeyState::c_PackKeyStateNum + 1){
		frame = g_ui_frame + Data_KeyState::c_PackKeyStateNum;
		if (frame >= g_ui_frame + g_connection.delay_compensation)
			frame = g_ui_frame + g_connection.delay_compensation;
	}

	for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++)
	{
		if (frame - i < 0){
			ks.key_state[i].frame = -1;
		}else{
			ks.key_state[i] = g_keystate_self[frame - i];
			sended_frame_udp[frame - i] = cur_time;
		}
	}
	g_connection.SendUDPPack(ks);
}

void HandlePacks()
{
	while (g_connection.RcvUDPPack() > 0);
	if (g_connection.connect_state == ConnectState::No_Connection)
		return;
	while (!g_connection.packs_rcved.empty())
	{
		Pack p = g_connection.packs_rcved.front();
		g_connection.packs_rcved.pop();
		auto ps = get_if<Data_StatePack>(&p.data);
		auto pk = get_if<Data_KeyState>(&p.data);
		auto pnak = get_if<Data_NAK_KeyState>(&p.data);
		if (ps) {
			if (ps->state == StatePack_State::Wait_State){
				static int last_delay_frame = 0;
				if (last_delay_frame < g_ui_frame - 10 || last_delay_frame > g_ui_frame){
					LogInfo(std::format("RWAIT, delayed {} frame",ps->wait_frame));
					Delay(ps->wait_frame * 14);
					last_delay_frame = g_ui_frame;
				}
			}
			else if (ps->state == StatePack_State::Host_Sync) {
				LogInfo("host sync");
			}else{
				LogInfo("unknown state");
			}
		}else if (pk){
			for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++){
				auto& s = pk->key_state[i];
				if(s.frame>=0)
					g_keystate_rcved[s.frame] = s;
			}
		}else if (pnak){
			SendKeyStates(pnak->frame);
		}
	}
}

void __stdcall ConnectLoop_L()
{
	if (g_is_loading)
		return;
	ConnectLoop();
}

void ConnectLoop()
{
	switch (g_connection.connect_state)
	{
	case ConnectState::No_Connection:
		return;
	case ConnectState::Connected:
	{
		SendKeyStates(g_ui_frame + g_connection.delay_compensation);
		HandlePacks();
		if (!g_keystate_rcved.contains(g_ui_frame)) {
			Delay(2);
			HandlePacks();
			LogInfo("no cur seed");
			bool has_seed = false;
			for (int i = 0; i < g_connection.c_max_time_retry_timeout; i++)
			{
				g_connection.SendUDPPack(Data_NAK_KeyState(g_ui_frame));
				LARGE_INTEGER time_begin;
				GetTime(&time_begin);
				while (true){
					HandlePacks();
					if (g_keystate_rcved.contains(g_ui_frame)) {
						has_seed = true;
						break;
					}
					if (g_connection.connect_state == ConnectState::No_Connection)
						return;
					LARGE_INTEGER time_cur;
					GetTime(&time_cur);
					if (CalTimePeriod(time_begin, time_cur) > 16 * g_connection.delay_compensation)
						break;
				}
				if (has_seed)
					break;
				Delay(g_connection.delay_compensation * 16);
			}
			if (!has_seed) {
				LogError("fail to Connect");
				g_connection.EndConnect();
				return;
			}
		}
		for (int i = 0; i < 4; i++) {
			if (g_keystate_rcved[g_ui_frame].seednum[i] != g_keystate_self[g_ui_frame].seednum[i]) {
				LogError(std::format("Fail to sync {}, seed {} : {}/{}",g_ui_frame, i, g_keystate_self[g_ui_frame].seednum[i], g_keystate_rcved[g_ui_frame].seednum[i]));
				g_is_synced = false;
			}
		}

		if (g_is_synced == false && (VALUED(0x005AE474)==0) && (VALUED(0x005AE624)!=0))//not in-game desync
		{
			LARGE_INTEGER cur_time_try_sync;
			static LARGE_INTEGER last_time_try_sync = { 0 };
			GetTime(&cur_time_try_sync);
			if (last_time_try_sync.QuadPart==0 || (CalTimePeriod(last_time_try_sync,cur_time_try_sync)>= g_connection.c_time_ms_retry_sync))//2000ms
			{
				LogInfo("try sync");
				last_time_try_sync = cur_time_try_sync;
				if (g_connection.is_host)
				{//host
					bool is_try_synced = false;
					for (int i = 0; i < g_connection.c_max_time_retry_timeout; i++)
					{
						g_connection.SendUDPPack(Data_StatePack(StatePack_State::Host_Sync));
						for (int j = 0; j < g_connection.delay_compensation; j++)
						{
							Delay(16);
							while (g_connection.RcvUDPPack() > 0);
							if (g_connection.connect_state == ConnectState::No_Connection)
								return;
							while (!g_connection.packs_rcved.empty()) {
								Pack p = g_connection.packs_rcved.front();
								g_connection.packs_rcved.pop();
								auto ps = get_if<Data_StatePack>(&p.data);
								auto pk = get_if<Data_KeyState>(&p.data);
								auto pnak = get_if<Data_NAK_KeyState>(&p.data);
								if (ps && ps->state == StatePack_State::Guest_Sync_Confirm) {
									is_try_synced = true;
									g_connection.packs_rcved = {};
									break;
								}else if (pnak) {
									if (pnak->frame < g_connection.delay_compensation) {
										is_try_synced = true;
										g_connection.packs_rcved = {};
										break;
									} else {
										if ((!g_keystate_self.contains(pnak->frame))){
											Data_KeyState ks;
											ks.key_state[0] = g_keystate_self[g_ui_frame];
											ks.key_state[0].frame = pnak->frame;
											CopyFromOriginalSeeds(ks.key_state[0].seednum);
											for (int i = 1; i < ks.c_PackKeyStateNum; i++){
												ks.key_state[i].frame = -1;
											}
											g_connection.SendUDPPack(ks);
										}else{
											SendKeyStates(pnak->frame);
										}
									}
								}else if (pk) {
									for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++){
										if (pk->key_state[i].frame!=-1 && pk->key_state[i].frame < g_connection.delay_compensation * 2) {
											is_try_synced = true;
											g_connection.packs_rcved = {};
											break;
										}
									}
									
								}
							}
							if (is_try_synced)
								break;
						}
						if (is_try_synced)
							break;
					}
					if (is_try_synced) {
						LogInfo("try syncH");
						StartConnection(false);
						SeedType seed[4];
						CopyFromOriginalSeeds(seed);
						for (int i = 0; i < 4; i++)
							LogInfo(std::format("cur seed {}: {}", i, seed[i]));
						return;
					}
				}else { //guest
					bool is_try_synced = false;
					for (int i = 0; i < g_connection.c_max_time_retry_timeout * g_connection.delay_compensation; i++)
					{
						Delay(12);
						while (g_connection.RcvUDPPack() > 0);
						if (g_connection.connect_state == ConnectState::No_Connection)
							return;
						while (!g_connection.packs_rcved.empty()) {
							Pack p = g_connection.packs_rcved.front();
							g_connection.packs_rcved.pop();
							auto ps = get_if<Data_StatePack>(&p.data);
							auto pnak = get_if<Data_NAK_KeyState>(&p.data);
							if (ps && ps->state == StatePack_State::Host_Sync) {
								g_connection.SendUDPPack(Data_StatePack(StatePack_State::Guest_Sync_Confirm));
								SetHostState(*ps);
								is_try_synced = true;
								g_connection.packs_rcved = {};
								break;
							}
							else if (pnak) {
								if ((!g_keystate_self.contains(pnak->frame)))
								{
									Data_KeyState ks;
									ks.key_state[0] = g_keystate_self[g_ui_frame];
									ks.key_state[0].frame = pnak->frame;
									for (int i = 1; i < ks.c_PackKeyStateNum; i++) {
										ks.key_state[i].frame = -1;
									}
									g_connection.SendUDPPack(ks);
								} else {
									SendKeyStates(pnak->frame);
								}
							}
						}
						if (is_try_synced)
							break;
					}
					if (is_try_synced) {
						LogInfo("try syncG");
						StartConnection(false);
						SeedType seed[4];
						CopyFromOriginalSeeds(seed);
						for(int i=0;i<4;i++)
							LogInfo(std::format("cur seed {}: {}", i, seed[i]));
						return;
					}
				}
			}
		}
		g_ui_frame++;
		
	}break;
	case ConnectState::Host_Listening:
	{
		while (g_connection.RcvUDPPack() > 0);
		if (g_connection.connect_state == ConnectState::No_Connection)
			return;
		while (!g_connection.packs_rcved.empty())
		{
			Pack p = g_connection.packs_rcved.front();
			g_connection.packs_rcved.pop();
			auto ps = get_if<Data_StatePack>(&p.data);
			if (ps) {
				if (ps->state == StatePack_State::Guest_Request) {
					g_connection.packs_rcved = {};
					g_connection.connect_state = ConnectState::Host_Confirming;
					goto HOST_COMFIRMING;
					break;
				}else {
					LogInfo("unknown state");
				}
			}
			else {
				LogInfo("unknown pack when listening");
				//drop pack
			}
		}
	}break;
	case ConnectState::Guest_Requesting:
	{
		LogInfo("requesting");
		bool is_host_accepted=false;
		for(int i=0;i<g_connection.c_max_time_retry_timeout;i++)
		{
			g_connection.SendUDPPack(Data_StatePack(StatePack_State::Guest_Request));
			for (int j = 0; j < g_connection.delay_compensation; j++)
			{
				Delay(16);
				while (g_connection.RcvUDPPack() > 0);
				if (g_connection.connect_state == ConnectState::No_Connection)
					return;
				while (!g_connection.packs_rcved.empty())
				{
					Pack p = g_connection.packs_rcved.front();
					g_connection.packs_rcved.pop();
					auto ps = get_if<Data_StatePack>(&p.data);
					if (ps) {
						if (ps->state == StatePack_State::Host_State) {
							SetHostState(*ps);
							g_connection.packs_rcved = {};
							is_host_accepted = true;
							break;
						} else {
							LogInfo("unknown state");
						}
					}else {
						LogInfo("unknown pack when listening");
						//drop pack
					}
				}
				if (is_host_accepted)
					break;
			}
			if (is_host_accepted)
				break;
		}
		if (is_host_accepted){
			g_connection.SendUDPPack(Data_StatePack(StatePack_State::Guest_Confirm));
			Delay(g_connection.delay_compensation * 8);
			g_connection.connect_state = ConnectState::Connected;
			StartConnection(true);
		}else{
			LogError("time out when requesting");
			g_connection.EndConnect();
			return;
		}
	}break;
	case ConnectState::Host_Confirming:
HOST_COMFIRMING:
	{
		LogInfo("confirming");
		bool is_guest_confirmed = false;
		for (int i = 0; i < g_connection.c_max_time_retry_timeout; i++)
		{
			g_connection.SendUDPPack(Data_StatePack(StatePack_State::Host_State));
			for (int j = 0; j < g_connection.delay_compensation; j++){
				Delay(16);
				while (g_connection.RcvUDPPack() > 0);
				if (g_connection.connect_state == ConnectState::No_Connection)
					return;
				while (!g_connection.packs_rcved.empty())
				{
					Pack p = g_connection.packs_rcved.front();
					g_connection.packs_rcved.pop();
					auto ps = get_if<Data_StatePack>(&p.data);
					auto pnak = get_if<Data_NAK_KeyState>(&p.data);
					auto pks = get_if<Data_KeyState>(&p.data);
					if (ps) {
						if (ps->state == StatePack_State::Guest_Confirm) {
							is_guest_confirmed = true;
							break;
						} else if(ps->state!=StatePack_State::Guest_Request){
							LogInfo("unknown state");// drop pack
						}
					} else {
						is_guest_confirmed = true; // if confirm pack is dropped but other packs(KS/NAK), still confirm it
						break;
					}
				}
				if (is_guest_confirmed)
					break;
			}
		}
		if (is_guest_confirmed) {
			g_connection.connect_state = ConnectState::Connected;
			StartConnection(true);
		}else {
			LogError("time out when confirming");
			g_connection.EndConnect();
			return;
		}
		
	}break;
	}
}

P2PConnection::P2PConnection():
	port_listen_Host(10800),port_send_Guest(10801),port_sendto(c_no_port), delay_compensation(3),
	is_host(true), is_ipv6(false),pack_index(0),is_blocking(false),
	connect_state(ConnectState::No_Connection),is_addr_guest_sendto_set(false)
{
	// GetTime(&last_init_value);
	memset(&addr_self6, 0, sizeof(addr_self6));
	memset(&addr_other6, 0, sizeof(addr_other6));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
	socket_udp = INVALID_SOCKET;
}



bool P2PConnection::SetUpConnect_Guest()
{
	if (this->connect_state != No_Connection)
		return false;
	is_host = false;
	int res,res_udp;
	if (is_ipv6){
		
		addr_self6 = { AF_INET6, htons(port_send_Guest) };
		socket_udp = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		res_udp=bind(socket_udp, (SOCKADDR*)&(addr_self6), sizeof(addr_self6));

		LogInfo(std::format("ipv6 Guest,to {} port: {}", this->addr_snedto,this->port_sendto));
	}else {//ipv4
		
		addr_self4 = { AF_INET, htons(port_send_Guest) };
		socket_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		res_udp = bind(socket_udp, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));

		LogInfo(std::format("ipv4 Guest,to {} port: {}", this->addr_snedto, this->port_sendto));
	}
	if (res_udp != 0 || socket_udp ==INVALID_SOCKET) {
		LogError(std::format("fail to create udp socket : {}", WSAGetLastError()));
		EndConnect();
		return false;
	}
	SetSocketBlocking(socket_udp);
	LogInfo(std::format("set up connect"));
	connect_state = ConnectState::Guest_Requesting;
	return true;
}

bool P2PConnection::SetUpConnect_Host(bool iis_ipv6)
{
	if (this->connect_state != No_Connection)
		return false;
	is_host = true;
	int res,res_udp;
	if (iis_ipv6) {
		is_ipv6 = true;
		addr_self6 = { AF_INET6, htons(port_listen_Host) };
		socket_udp = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		res_udp = bind(socket_udp, (SOCKADDR*)&(addr_self6), sizeof(addr_self6));
		LogInfo(std::format("ipv6 Host"));
	}else {//ipv4
		is_ipv6 = false;
		addr_self4 = { AF_INET, htons(port_listen_Host) };
		socket_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		res_udp = bind(socket_udp, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));
		LogInfo(std::format("ipv4 Host"));
	}
	if (res_udp != 0 || socket_udp == INVALID_SOCKET) {
		LogError(std::format("fail to create udp socket : {}", WSAGetLastError()));
		EndConnect();
		return false;
	}
	SetSocketBlocking(socket_udp);
	LogInfo(std::format("set up connect"));
	connect_state = ConnectState::Host_Listening;
	return true;
}

void P2PConnection::EndConnect()
{
	LogInfo("end connect");
	if (connect_state == ConnectState::No_Connection)
		return;
	connect_state = ConnectState::No_Connection;
	
	if (socket_udp != INVALID_SOCKET) {
		closesocket(socket_udp);
		socket_udp = INVALID_SOCKET;
	}
	// memset(&addr_self6, 0, sizeof(addr_self6));
	// memset(&addr_self4, 0, sizeof(addr_self4));
	// memset(&addr_other6, 0, sizeof(addr_other6));
	// memset(&addr_other4, 0, sizeof(addr_other4));
	packs_rcved = std::queue<Pack>();
}

void P2PConnection::SetSocketBlocking(SOCKET sock)
{
	if (is_blocking) {
		unsigned int timeout = 3;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&timeout), sizeof(timeout));
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)(&timeout), sizeof(timeout));
	}else {
		u_long o = 1;
		ioctlsocket(sock, FIONBIO, &o);
	}
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


int P2PConnection::SendUDPPack(Data_KeyState data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other6.sin6_port != 0)
		l_nLen = sendto(socket_udp, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(socket_udp, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0) {
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		LogInfo(std::format("[snded] {:<5} SOp frame {:<6}", pack.index, data.key_state[0].frame));
	}
	return l_nLen;
}

int P2PConnection::SendUDPPack(Data_NAK_KeyState data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other6.sin6_port != 0)
		l_nLen = sendto(socket_udp, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(socket_udp, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));

	if (l_nLen < 0) {
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		LogInfo(std::format("[snded] {:<5} SNAKu frame {:<6}", pack.index, data.frame));
	}
	return l_nLen;
}

int P2PConnection::SendUDPPack(Data_StatePack data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other6.sin6_port != 0)
		l_nLen = sendto(socket_udp, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(socket_udp, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0){
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		switch (data.state)
		{
		case StatePack_State::Guest_Sync_Confirm:
			LogInfo(std::format("[snded] {:<5} SGuestSyncCfm", pack.index)); break;
		case StatePack_State::Host_Sync:
			LogInfo(std::format("[snded] {:<5} SHostSync; {}|{}|{}|{}", pack.index, data.seednum[0], data.seednum[1], data.seednum[2], data.seednum[3])); break;
		case StatePack_State::Guest_Confirm:
			LogInfo(std::format("[snded] {:<5} SGuestCfm", pack.index)); break;
		case StatePack_State::Guest_Request:
			LogInfo(std::format("[snded] {:<5} SGuestReq", pack.index)); break;
		case StatePack_State::Host_State:
			LogInfo(std::format("[snded] {:<5} SHostStt; {}|{}|{}|{}", pack.index, data.seednum[0], data.seednum[1], data.seednum[2], data.seednum[3])); break;
		case StatePack_State::Wait_State:
			LogInfo(std::format("[snded] {:<5} SWAIT {}", pack.index,data.wait_frame)); break;
		default:
			LogInfo(std::format("[snded] {:<5} S???", pack.index)); break;
		}
	}
	return l_nLen;
}

bool P2PConnection::SetGuestSocketSetting(std::string host_ipaddress, int port_sendto_Guest, bool iis_ipv6)
{
	is_addr_guest_sendto_set = false;
	if (port_sendto_Guest > 65535 || port_sendto_Guest < 0)
		return false;
	is_ipv6 = iis_ipv6;
	if (is_ipv6) {
		addr_other6 = { AF_INET6, htons(port_sendto_Guest) };
		if (inet_pton(AF_INET6, host_ipaddress.c_str(), &(addr_other6.sin6_addr)) != 1) {
			LogError("Wrong IP addr");
			return false;
		}
		char buf[256] = { 0 };
		inet_ntop(AF_INET6, &addr_other6.sin6_addr, buf, sizeof(buf));
		addr_snedto = buf;
		port_sendto = port_sendto_Guest;
	}else {//ipv4
		addr_other4 = { AF_INET, htons(port_sendto_Guest) };
		if (inet_pton(AF_INET, host_ipaddress.c_str(), &(addr_other4.sin_addr)) != 1) {
			LogError("Wrong IP addr");
			return false;
		}
		char buf[256] = { 0 };
		inet_ntop(AF_INET, &addr_other4.sin_addr, buf, sizeof(buf));
		addr_snedto = buf;
		port_sendto = port_sendto_Guest;
	}
	
	is_addr_guest_sendto_set = true;
}


Pack P2PConnection::CreateEmptyPack()
{
	Pack pack;
	pack.index = this->pack_index++;
	return pack;
}

int P2PConnection::RcvUDPPack()
{
	Pack pack;
	int l_nReadLen = 0;
	int sz = 0;
	if (is_ipv6)
	{
		sz = sizeof(addr_other6), l_nReadLen = recvfrom(socket_udp, (char*)(&pack), sizeof(Pack), 0, (SOCKADDR*)&(addr_other6), &sz);
	}else{
		sz = sizeof(addr_other4), l_nReadLen = recvfrom(socket_udp, (char*)(&pack), sizeof(Pack), 0, (SOCKADDR*)&(addr_other4), &sz);
	}
	// l_nReadLen = recv(socket_udp, (char*)(&pack), sizeof(Pack), 0);
	if (l_nReadLen == sizeof(Pack)) {
		Data_KeyState* pks = std::get_if<Data_KeyState>(&(pack.data));
		Data_StatePack* pst = std::get_if<Data_StatePack>(&(pack.data));
		Data_NAK_KeyState* pnak = std::get_if<Data_NAK_KeyState>(&(pack.data));
		if (pst) {
			switch (pst->state) {
			case StatePack_State::Guest_Sync_Confirm:
				LogInfo(std::format("[rcved] {:<5} RGuestSyncCfm", pack.index)); break;
			case StatePack_State::Host_Sync:
				LogInfo(std::format("[rcved] {:<5} RHostSync; {}|{}|{}|{}", pack.index, pst->seednum[0], pst->seednum[1], pst->seednum[2], pst->seednum[3])); break;
			case StatePack_State::Guest_Confirm:
				LogInfo(std::format("[rcved] {:<5} RGuestCfm", pack.index)); break;
			case StatePack_State::Guest_Request:
				LogInfo(std::format("[rcved] {:<5} RGuestReq", pack.index)); break;
			case StatePack_State::Host_State:
				LogInfo(std::format("[rcved] {:<5} RHostStt; {}|{}|{}|{}", pack.index, pst->seednum[0], pst->seednum[1], pst->seednum[2], pst->seednum[3])); break;
			case StatePack_State::Wait_State:
				LogInfo(std::format("[rcved] {:<5} RWAIT", pack.index)); break;
			default:
				LogInfo(std::format("[rcved] {:<5} R???", pack.index)); break;
			}
		}
		else if (pks) {
			LogInfo(std::format("[rcved] {:<5} ROp frame {:<6}", pack.index, pks->key_state[0].frame));
		}
		else if (pnak) {
			LogInfo(std::format("[rcved] {:<5} RNAK frame {:<6}", pack.index, pnak->frame));
		}
		else {
			LogInfo(std::format("[rcved] {:<5} R????", pack.index));
		}
		packs_rcved.push(pack);
	}
	else if (l_nReadLen < 0) {
		int  err = WSAGetLastError();
		if (err != 10035 && err != 10060)
		{
			LogError(std::format("receive error: {}", WSAGetLastError()));
			EndConnect();
		}
	}
	else if (l_nReadLen > 0) {
		LogError(std::format("unknown received pack, size : {}", l_nReadLen));
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
	case StatePack_State::Guest_Confirm:
	case StatePack_State::Guest_Sync_Confirm:
	case StatePack_State::Guest_Request:
	case StatePack_State::Wait_State:
		delay_compensation = g_connection.delay_compensation;
		memset(cfg_flag, 0, sizeof(cfg_flag)); memset(seednum, 0, sizeof(seednum));
		break;
	case StatePack_State::Host_State:
	case StatePack_State::Host_Sync:
		CopyFromOriginalSeeds(seednum);
		delay_compensation = g_connection.delay_compensation;
		cfg_flag[0] = VALUED(0x00608644);
		cfg_flag[1] = VALUED(0x00608648);
		cfg_flag[2] = VALUED(0x0060864C);
		break;
	}
}
Data_StatePack::Data_StatePack() :state(StatePack_State::No_State), delay_compensation(0) { memset(cfg_flag, 0, sizeof(cfg_flag)); memset(seednum, 0, sizeof(seednum)); }
