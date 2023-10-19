#ifndef _TcpServerCommon_hpp_
#define _TcpServerCommon_hpp_

#include"DefaultSetting.hpp"
#include"SingleSockMgr.hpp"
#include"SockGroupMgr.hpp"
#include"NetCallbackIntf.hpp"
#include"SockCtl.hpp"
#include"Config.hpp"

#include<thread>
#include<mutex>
#include<atomic>

class TcpServerCommon : public NetCallbackIntf
{
private:
	Thread _thread;
	std::vector<SockGroupMgr*> _cellServers;
	Timestamp _tTime;
	SOCKET _sock;
protected:
	int _address_family = AF_INET;
	int _nSendBuffSize;
	int _nRecvBuffSize;
	int _nMaxClient;
	//SOCKET recv count
	std::atomic_int _recvCount;
	//message recv count
	std::atomic_int _msgCount;
	//client num
	std::atomic_int _clientAccept;
	std::atomic_int _clientJoin;
public:
	TcpServerCommon()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_msgCount = 0;
		_clientAccept = 0;
		_clientJoin = 0;
		_nSendBuffSize = Config::Instance().getInt("nSendBuffSize", SEND_BUFF_SZIE);
		_nRecvBuffSize = Config::Instance().getInt("nRecvBuffSize", RECV_BUFF_SZIE);
		_nMaxClient = Config::Instance().getInt("nMaxClient", FD_SETSIZE);
	}

	virtual ~TcpServerCommon()
	{
		Close();
	}

	SOCKET InitSocket(int af = AF_INET)
	{
		SockCtl::Init();
		if (INVALID_SOCKET != _sock)
		{
			CELLLog_Warring("initSocket close old socket<%d>...", (int)_sock);
			Close();
		}
		_address_family = af;
		_sock = socket(af, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			CELLLog_PError("create socket failed...");
		}
		else {
			SockCtl::make_reuseaddr(_sock);
			CELLLog_Info("create socket<%d> success...", (int)_sock);
		}
		return _sock;
	}

	int Bind(const char* ip, unsigned short port)
	{
		int ret = SOCKET_ERROR;
		if (AF_INET == _address_family)
		{
			sockaddr_in _sin = {};
			_sin.sin_family = AF_INET;
			_sin.sin_port = htons(port);

			if (ip) {
				_sin.sin_addr.s_addr = inet_addr(ip);
			}
			else {
				_sin.sin_addr.s_addr = INADDR_ANY;
			}

			ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		}
		else if (AF_INET6 == _address_family) {
			sockaddr_in6 _sin = {};
			_sin.sin6_family = AF_INET6;
			_sin.sin6_port = htons(port);
			if (ip)
			{
				inet_pton(AF_INET6, ip, &_sin.sin6_addr);
			}
			else {
				_sin.sin6_addr = in6addr_any;
			}
			ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		}
		else {
			CELLLog_Error("bind port,_address_family<%d> failed...", _address_family);
			return ret;
		}

		if (SOCKET_ERROR == ret)
		{
			CELLLog_PError("bind port<%d> failed...", port);
		}
		else {
			CELLLog_Info("bind port<%d> success...", port);
		}
		return ret;
	}

	int Listen(int n)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			CELLLog_PError("listen socket<%d> failed...", _sock);
		}
		else {
			CELLLog_Info("listen port<%d> success...", _sock);
		}
		return ret;
	}

	SOCKET Accept()
	{
		if (AF_INET == _address_family)
		{
			return Accept_IPv4();
		}
		else {
			return Accept_IPv6();
		}
	}

	SOCKET Accept_IPv6()
	{
		sockaddr_in6 clientAddr = {};
		int nAddrLen = sizeof(clientAddr);
		SOCKET cSock = INVALID_SOCKET;
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
		if (INVALID_SOCKET == cSock)
		{
			CELLLog_PError("accept INVALID_SOCKET...");
		}
		else
		{
			static char ip[INET6_ADDRSTRLEN] = {};
			inet_ntop(AF_INET6, &clientAddr.sin6_addr, ip, INET6_ADDRSTRLEN - 1);
			AcceptClient(cSock, ip);
		}
		return cSock;
	}
	
	SOCKET Accept_IPv4()
	{
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
		if (INVALID_SOCKET == cSock)
		{
			CELLLog_PError("accept INVALID_SOCKET...");
		}
		else
		{
			char* ip = inet_ntoa(clientAddr.sin_addr);
			AcceptClient(cSock, ip);
		}
		return cSock;
	}

	void AcceptClient(SOCKET cSock, char* ip)
	{
		SockCtl::make_reuseaddr(cSock);
		//CELLLog_Info("Accept_IP: %s, %d", ip, cSock);
		if (_clientAccept < _nMaxClient)
		{
			_clientAccept++;
			//allocate new client to SockGroupMgr which has the least number of clients
			auto c = makeClientObj(cSock);
			c->setIP(ip);
			addClientToCELLServer(c);
		}
		else {
			SockCtl::destorySocket(cSock);
			CELLLog_Warring("Accept to nMaxClient");
		}
	}

	virtual SingleSockMgr* makeClientObj(SOCKET cSock)
	{
		return new SingleSockMgr(cSock, _nSendBuffSize, _nRecvBuffSize);
	}

	void addClientToCELLServer(SingleSockMgr* pClient)
	{
		auto pMinServer = _cellServers[0];
		for (auto pServer : _cellServers)
		{
			if (pMinServer->getClientCount() > pServer->getClientCount())
			{
				pMinServer = pServer;
			}
		}
		pMinServer->addClient(pClient);
	}

	SingleSockMgr* find_client(int id)
	{
		for (auto pServer : _cellServers)
		{
			SingleSockMgr* c = pServer->find_client(id);
			if (c)
			{
				return c;
			}
		}
		return nullptr;
	}

	template<class ServerT>
	void Start(int nCELLServer)
	{
		for (int n = 0; n < nCELLServer; n++)
		{
			auto ser = new ServerT();
			ser->setId(n + 1);
			ser->setClientNum((_nMaxClient / nCELLServer) + 1);
			_cellServers.push_back(ser);
			//register self as callback obj
			ser->setCallback(this);
			//start SockGroupMgr worker thread
			ser->Start();
		}
		_thread.Start(nullptr,
			[this](Thread* pThread) {
			OnRun(pThread);
		});
	}
	
	//close Socket
	void Close()
	{
		CELLLog_Info("TcpServerCommon.Close begin");
		_thread.Close();
		if (_sock != INVALID_SOCKET)
		{
			for (auto s : _cellServers)
			{
				delete s;
			}
			_cellServers.clear();
			SockCtl::destorySocket(_sock);
			_sock = INVALID_SOCKET;
		}
		CELLLog_Info("TcpServerCommon.Close end");
	}

	//WARNING: not thread-safe
	virtual void OnNetJoin(SingleSockMgr* pClient)
	{
		_clientJoin++;
		//CELLLog_Info("client<%d> join", pClient->sockfd());
	}
	
	//WARNING: not thread-safe
	virtual void OnNetLeave(SingleSockMgr* pClient)
	{
		_clientAccept--;
		_clientJoin--;
		//CELLLog_Info("client<%d> leave", pClient->sockfd());
	}
	
	virtual void OnNetMsg(SockGroupMgr* pServer, SingleSockMgr* pClient, char* header)
	{
		_msgCount++;
	}

	virtual void OnNetRecv(SingleSockMgr* pClient)
	{
		_recvCount++;
		//CELLLog_Info("client<%d> leave", pClient->sockfd());
	}
protected:
	virtual void OnRun(Thread* pThread) = 0;

	//compute the number of received message per second
	void time4msg()
	{
		//return;
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			CELLLog_Info("thread<%d>,time<%lf>,socket<%d>,Accept<%d>,Join<%d>,recv<%d>,msg<%d>"
				, (int)_cellServers.size()
				, t1, _sock
				, (int)_clientAccept, (int)_clientJoin
				, (int)_recvCount, (int)_msgCount);
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

	SOCKET sockfd()
	{
		return _sock;
	}
};

#endif
