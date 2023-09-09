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

void StartConnection()
{
	g_is_synced = true;
	g_connection.packs_rcved = {};
	g_keystate_self = {};
	g_keystate_rcved = {};
	if (g_connection.is_host){
		SeedType t = timeGetTime();
		SeedType seeds_init[4] = { t,t,t,t };
		CopyToOriginalSeeds(seeds_init);
		g_connection.SendTCPPack(Data_StatePack(StatePack_State::Host_State));
	}else{
		g_connection.RcvTCPPack();
		if (g_connection.connect_state == ConnectState::No_Connection)
			return;
		int i = 0;
		while(g_connection.packs_rcved.empty()){
			i++;
			if (i >= g_connection.delay_compensation+ P2PConnection::max_frame_wait){
				LogError("fail to receive state from host");
				g_connection.EndConnect();
				return;
			}
			Delay(16);
			g_connection.RcvTCPPack();
			if (g_connection.connect_state == ConnectState::No_Connection)
				return;
		}
		Pack pf=g_connection.packs_rcved.front();
		auto pstate=get_if<Data_StatePack>(&pf.data);
		if (pstate != nullptr){
			CopyToOriginalSeeds(pstate->seednum);
			VALUED(0x00608644)=pstate->cfg_flag[0];
			VALUED(0x00608648)=pstate->cfg_flag[1];
			VALUED(0x0060864C)=pstate->cfg_flag[2];
			g_connection.delay_compensation = pstate->delay_compensation;
			g_connection.packs_rcved.pop();
		}else{
			LogError("not state pack from host");
			g_connection.EndConnect();
			return;
		}
	}
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

void SendKeyStates(int frame,bool is_udp)
{
	Data_KeyState ks;
	if (!g_keystate_self.contains(frame)){
		LogInfo(std::format("frame ahead {}:{}",frame,g_ui_frame));
		Data_StatePack wait(StatePack_State::WAIT_STATE);
		wait.wait_frame = std::clamp(frame-g_ui_frame-1,1,g_connection.delay_compensation / 2);
		g_connection.SendTCPPack(wait);
	}
	if (frame >= g_ui_frame - Data_KeyState::c_PackKeyStateNum + 1){
		frame = g_ui_frame + Data_KeyState::c_PackKeyStateNum;
		if (frame >= g_ui_frame + g_connection.delay_compensation)
			frame = g_ui_frame + g_connection.delay_compensation;
	}
	static std::set<int> sended_frame_udp;
	static std::set<int> sended_frame_tcp;
	static int last_frame = -1;
	if (last_frame != g_ui_frame){
		last_frame = g_ui_frame;
		sended_frame_udp = {};
	}
	if (last_frame > g_ui_frame){
		last_frame = g_ui_frame;
		sended_frame_udp = {};
		sended_frame_tcp = {};
	}
	if (is_udp){
		for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++)
			if (sended_frame_udp.contains(frame - i))
				return;
		sended_frame_udp.insert(frame);
	}else{
		for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++)
			if (sended_frame_tcp.contains(frame - i))
				return;
		sended_frame_tcp.insert(frame);
	}
	for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++)
	{
		if (frame - i < 0)
			ks.key_state[i].frame = -1;
		else
			ks.key_state[i] = g_keystate_self[frame - i];
	}
	if (is_udp){
		g_connection.SendUDPPack(ks);
	}else{
		g_connection.SendUDPPack(ks);
		g_connection.SendTCPPack(ks);
	}
}

void HandlePacks()
{
	while (g_connection.RcvTCPPack() > 0);
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
			if (ps->state == StatePack_State::WAIT_STATE){
				static int last_delay_frame = 0;
				if (last_delay_frame < g_ui_frame - 10 || last_delay_frame > g_ui_frame){
					LogInfo(std::format("RWAIT, delayed {} frame",ps->wait_frame));
					Delay(ps->wait_frame * 14);
					last_delay_frame = g_ui_frame;
				}
			}else{
				LogInfo("unknown state");
			}
		}else if (pk){
			for (int i = 0; i < Data_KeyState::c_PackKeyStateNum; i++){
				auto& s = pk->key_state[i];
				if(s.frame>=0 && !g_keystate_rcved.contains(s.frame))
					g_keystate_rcved[s.frame] = s;
			}
		}else if (pnak){
			SendKeyStates(pnak->frame,false);
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
		SendKeyStates(g_ui_frame + g_connection.delay_compensation,true);
		HandlePacks();
		if (!g_keystate_rcved.contains(g_ui_frame)){
			LogInfo("no cur seed");
			g_connection.SendTCP_UDP_Pack(Data_NAK_KeyState(g_ui_frame));
		}
		int i = 0;
		while (true) {
			HandlePacks();
			if (g_keystate_rcved.contains(g_ui_frame))
				break;
			Delay(1);
			if (g_connection.connect_state == ConnectState::No_Connection)
				return;
			if (i > (g_connection.delay_compensation + P2PConnection::max_frame_wait) * 16) {
				LogError("fail to Connect");
				g_connection.EndConnect();
				return;
			}
			i++;
		}
		for (int i = 0; i < 4; i++) {
			if (g_keystate_rcved[g_ui_frame].seednum[i] != g_keystate_self[g_ui_frame].seednum[i]) {
				LogError(std::format("Fail to sync {}, seed {} : {}/{}",g_ui_frame, i, g_keystate_self[g_ui_frame].seednum[i], g_keystate_rcved[g_ui_frame].seednum[i]));
				g_is_synced = false;

			}
		}
		g_ui_frame++;
		break;
	}
	case ConnectState::Host_Listening:
	{
		bool res = g_connection.Host_Listening();
		if (res){
			g_connection.connect_state = ConnectState::Connected;
			StartConnection();
			g_connection.SetSocketBlocking(g_connection.socket_host);
		}
		break;
	}
	case ConnectState::Guest_Requesting:
	{
		bool res = g_connection.Guest_Request();
		if (res){
			g_connection.connect_state = ConnectState::Connected;
			StartConnection();
			g_connection.SetSocketBlocking(g_connection.socket_guest);
		}else{
			g_connection.EndConnect();
			return;
		}
		break;
	}
	}
}

P2PConnection::P2PConnection():
	port_Host(9961),port_Guest(9962), delay_compensation(3), 
	is_host(true), is_ipv6(false),pack_index(0),is_blocking(false),
	connect_state(ConnectState::No_Connection)
{
	// QueryPerformanceCounter(&last_init_value);
	strcpy_s(address, "127.0.0.1");
	memset(&addr_self6, 0, sizeof(addr_self6));
	memset(&addr_other6, 0, sizeof(addr_other6));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
	socket_host = INVALID_SOCKET;
	socket_guest = INVALID_SOCKET;
	socket_listen_host = INVALID_SOCKET;
	socket_udp = INVALID_SOCKET;
}

bool P2PConnection::SetUpConnect_Guest()
{
	if (this->connect_state != No_Connection)
		return false;
	is_host = false;
	int res,res_udp;
	if (strstr(address, ":") != NULL){
		is_ipv6 = true;
		addr_other6 = { AF_INET6, htons(port_Host) };
		inet_pton(AF_INET6, address, &(addr_other6.sin6_addr));

		addr_self6 = { AF_INET6, htons(port_Guest) };
		socket_guest = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		res = bind(socket_guest, (SOCKADDR*)&(addr_self6), sizeof(addr_self6));
		if (res != 0 || socket_guest == INVALID_SOCKET) {
			LogError(std::format("fail to create tcp socket : {}", WSAGetLastError()));
			EndConnect();
			return false;
		}
		socket_udp = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		res_udp=bind(socket_udp, (SOCKADDR*)&(addr_self6), sizeof(addr_self6));
		LogInfo(std::format("ipv6 Guest,to {}", address));
	}else {//ipv4
		is_ipv6 = false;
		addr_other4 = { AF_INET, htons(port_Host) };
		inet_pton(AF_INET, address, &(addr_other4.sin_addr));

		addr_self4 = { AF_INET, htons(port_Guest) };
		socket_guest = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		res = bind(socket_guest, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));
		if (res != 0 || socket_guest == INVALID_SOCKET) {
			LogError(std::format("fail to create tcp socket : {}", WSAGetLastError()));
			EndConnect();
			return false;
		}
		socket_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		res_udp = bind(socket_udp, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));
		LogInfo(std::format("ipv4 Guest,to {}", address));
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

bool P2PConnection::SetUpConnect_Host()
{
	if (this->connect_state != No_Connection)
		return false;
	is_host = true;
	int res,res_udp;
	if (strstr(address, ":") != NULL) {
		is_ipv6 = true;
		addr_self6 = { AF_INET6, htons(port_Host) };
		socket_listen_host = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		res = bind(socket_listen_host, (SOCKADDR*)&(addr_self6), sizeof(addr_self6));
		if (res != 0 || socket_listen_host == INVALID_SOCKET) {
			LogError(std::format("fail to create tcp socket : {}", WSAGetLastError()));
			EndConnect();
			return false;
		}
		socket_udp = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		res_udp = bind(socket_udp, (SOCKADDR*)&(addr_self6), sizeof(addr_self6));
		LogInfo(std::format("ipv6 Host"));
	}else {//ipv4
		is_ipv6 = false;
		addr_self4 = { AF_INET, htons(port_Host) };
		socket_listen_host = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		res = bind(socket_listen_host, (SOCKADDR*)&(addr_self4), sizeof(addr_self4));
		if (res != 0 || socket_listen_host == INVALID_SOCKET) {
			LogError(std::format("fail to create tcp socket : {}", WSAGetLastError()));
			EndConnect();
			return false;
		}
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
	int res_lis = listen(socket_listen_host, 1);
	if (res_lis == SOCKET_ERROR) {
		LogError(std::format("fail to listen socket : {}", WSAGetLastError()));
		return false;
	}
	LogInfo(std::format("set up connect"));
	connect_state = ConnectState::Host_Listening;
	return true;
}

bool P2PConnection::Host_Listening()
{
	fd_set listen_sockets;
	FD_ZERO(&listen_sockets);
	FD_SET(socket_listen_host, &listen_sockets);

	fd_set fdRead= listen_sockets;
	timeval timeout = {0};
	timeout.tv_usec = 100;
	int sum = select(0, &fdRead, NULL, NULL, &timeout);
	if (sum > 0){
		for (int i = 0; i < (int)listen_sockets.fd_count; i++){
			if (FD_ISSET(listen_sockets.fd_array[i], &fdRead)>0){
				if (listen_sockets.fd_array[i] == socket_listen_host){
					if (is_ipv6)
					{
						int sz = sizeof(addr_other6);
						socket_host = accept(socket_listen_host, (SOCKADDR*)&addr_other6, &sz);
						if (socket_host == INVALID_SOCKET)
							LogError("invalid socket accepted");
						closesocket(socket_listen_host);
						socket_listen_host = INVALID_SOCKET;

						char buf[128];
						inet_ntop(AF_INET6, (const void*)&addr_other6.sin6_addr, buf, sizeof(buf));
						LogInfo(std::format("connect from {}:{}", buf, ntohs(addr_other6.sin6_port)));
						return true;
					}else{
						int sz = sizeof(addr_other4);
						socket_host = accept(socket_listen_host, (SOCKADDR*)&addr_other4, &sz);
						if (socket_host == INVALID_SOCKET)
							LogError("invalid socket accepted");
						closesocket(socket_listen_host);
						socket_listen_host = INVALID_SOCKET;

						char buf[128];
						inet_ntop(AF_INET, (const void*)&addr_other4.sin_addr, buf, sizeof(buf));
						LogInfo(std::format("connect from {}:{}", buf, ntohs(addr_other4.sin_port)));
						return true;
					}
				}else{
					LogInfo("unknown socket receive");
				}
			}
		}
	}
	return false;
}

bool P2PConnection::Guest_Request()
{
	int res;
	if (is_ipv6){
		res = connect(socket_guest, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	}else{
		res = connect(socket_guest, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	}
	if (res == SOCKET_ERROR) {
		LogError("fail to connect to host");
		return false;
	}
	LogInfo("connect to host successfully");
	return true;
}

void P2PConnection::EndConnect()
{
	LogInfo("end connect");
	if (connect_state == ConnectState::No_Connection)
		return;
	connect_state = ConnectState::No_Connection;
	

	if (socket_host != INVALID_SOCKET){
		shutdown(socket_host, SD_SEND);
		closesocket(socket_host);
		socket_host = INVALID_SOCKET;
	}
	if (socket_guest != INVALID_SOCKET){
		shutdown(socket_guest, SD_SEND);
		closesocket(socket_guest);
		socket_guest = INVALID_SOCKET;
	}
	if (socket_listen_host != INVALID_SOCKET){
		closesocket(socket_listen_host);
		socket_listen_host = INVALID_SOCKET;
	}
	if (socket_udp != INVALID_SOCKET) {
		closesocket(socket_udp);
		socket_udp = INVALID_SOCKET;
	}
	memset(&addr_self6, 0, sizeof(addr_self6));
	memset(&addr_other6, 0, sizeof(addr_other6));
	memset(&addr_self4, 0, sizeof(addr_self4));
	memset(&addr_other4, 0, sizeof(addr_other4));
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

SOCKET P2PConnection::GetCurrentTCPSocket()
{
	return is_host ? socket_host : socket_guest;
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

int P2PConnection::SendPack(SOCKET sock, Data_KeyState data, bool is_tcp)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other6.sin6_port != 0)
		l_nLen = sendto(sock, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(sock, (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0) {
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		std::string debug_str = is_tcp ? "t" : "u";
		LogInfo(std::format("[snded] {:<5} SOp-{} frame {:<6}", pack.index, debug_str, data.key_state[0].frame));
	}
	return l_nLen;
}

int P2PConnection::SendUDPPack(Data_KeyState data)
{
	return SendPack(socket_udp,data,false);
}

int P2PConnection::SendTCPPack(Data_KeyState data)
{
	return SendPack(GetCurrentTCPSocket(), data,true);
}

int P2PConnection::SendTCP_UDP_Pack(Data_NAK_KeyState data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other6.sin6_port != 0)
		l_nLen = sendto(GetCurrentTCPSocket(), (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(GetCurrentTCPSocket(), (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));

	if (l_nLen < 0) {
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		LogInfo(std::format("[snded] {:<5} SNAKt frame {:<6}", pack.index, data.frame));
	}

	l_nLen = 0;
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

int P2PConnection::SendTCPPack(Data_StatePack data)
{
	Pack pack = this->CreateEmptyPack();
	pack.data = data;
	int l_nLen = 0;
	if (is_ipv6 && addr_other6.sin6_port != 0)
		l_nLen = sendto(GetCurrentTCPSocket(), (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other6), sizeof(addr_other6));
	else if (!is_ipv6 && addr_other4.sin_port != 0)
		l_nLen = sendto(GetCurrentTCPSocket(), (const char*)&(pack), sizeof(pack), 0, (SOCKADDR*)&(addr_other4), sizeof(addr_other4));
	if (l_nLen < 0){
		LogError(std::format("send error: {}", WSAGetLastError()));
		EndConnect();
	}
	else if (l_nLen > 0) {
		switch (data.state)
		{
		case StatePack_State::Host_State:
			LogInfo(std::format("[snded] {:<5} SHostStt", pack.index)); break;
		case StatePack_State::WAIT_STATE:
			LogInfo(std::format("[snded] {:<5} SWAIT {}", pack.index,data.wait_frame)); break;
		default:
			LogInfo(std::format("[snded] {:<5} S???", pack.index)); break;
		}
	}
	return l_nLen;
}


Pack P2PConnection::CreateEmptyPack()
{
	Pack pack;
	pack.index = this->pack_index++;
	return pack;
}

int P2PConnection::RcvPack(SOCKET sock, bool is_tcp)
{
	Pack pack;
	int l_nReadLen = 0;
	int sz = 0;
	//	l_nReadLen = recvfrom(GetCurrentSocket(), (char*)(&pack), sizeof(Pack), 0, (SOCKADDR*)&(addr_other4), &sz);
	l_nReadLen = recv(sock, (char*)(&pack), sizeof(Pack), 0);
	std::string debugstr = is_tcp ? "tcp" : "udp";
	if (l_nReadLen == sizeof(Pack)) {
		Data_KeyState* pks = std::get_if<Data_KeyState>(&(pack.data));
		Data_StatePack* pst = std::get_if<Data_StatePack>(&(pack.data));
		Data_NAK_KeyState* pnak = std::get_if<Data_NAK_KeyState>(&(pack.data));
		if (pst) {
			switch (pst->state) {
			case StatePack_State::Host_State:
				LogInfo(std::format("[rcved] {:<5} {} RHostStt", pack.index, debugstr)); break;
			case StatePack_State::WAIT_STATE:
				LogInfo(std::format("[rcved] {:<5} {} WAIT", pack.index, debugstr)); break;
			default:
				LogInfo(std::format("[rcved] {:<5} {} R???", pack.index, debugstr)); break;
			}
		}
		else if (pks) {
			LogInfo(std::format("[rcved] {:<5} {} ROp frame {:<6}", pack.index, debugstr, pks->key_state[0].frame));
		}
		else if (pnak) {
			LogInfo(std::format("[rcved] {:<5} {} RNAK frame {:<6}", pack.index, debugstr, pnak->frame));
		}
		else {
			LogInfo(std::format("[rcved] {:<5} {} R????", pack.index, debugstr));
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
		LogError(std::format("unknown {} received pack, size : {}", debugstr, l_nReadLen));
	}
	return l_nReadLen;
}

int P2PConnection::RcvTCPPack()
{
	return RcvPack(GetCurrentTCPSocket(),true);
}

int P2PConnection::RcvUDPPack()
{
	return RcvPack(socket_udp,false);
}

Data_StatePack::Data_StatePack(StatePack_State state)
{
	this->state = state;
	switch (state)
	{
	default:
	case StatePack_State::WAIT_STATE:
	case StatePack_State::No_State:
		memset(cfg_flag, 0, sizeof(cfg_flag)); memset(seednum, 0, sizeof(seednum));
		delay_compensation = g_connection.delay_compensation;
		break;
	case StatePack_State::Host_State:
		CopyFromOriginalSeeds(seednum);
		delay_compensation = g_connection.delay_compensation;
		cfg_flag[0] = VALUED(0x00608644);
		cfg_flag[1] = VALUED(0x00608648);
		cfg_flag[2] = VALUED(0x0060864C);
		break;
	}
}
Data_StatePack::Data_StatePack() :state(StatePack_State::No_State), delay_compensation(0) { memset(cfg_flag, 0, sizeof(cfg_flag)); memset(seednum, 0, sizeof(seednum)); }
