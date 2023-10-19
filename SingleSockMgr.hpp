#ifndef _SingleSockMgr_HPP_
#define _SingleSockMgr_HPP_

#include"DefaultSetting.hpp"
#include"Buffer.hpp"
#include"SockCtl.hpp"

//heartbeat death timeout
#define CLIENT_HREAT_DEAD_TIME 100000

enum ClientState
{
	clientState_create = 10,
	clientState_join,
	clientState_run,
	clientState_close,
};

class SingleSockMgr
{
public:
	int id = -1;
	int serverId = -1;
	int nRecvMsgID = 1;
	int nSendMsgID = 1;

public:
	SingleSockMgr(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
		_sendBuff(sendSize),
		_recvBuff(recvSize)
	{
		static int n = 1;
		id = n++;
		_sockfd = sockfd;

		resetDTHeart();
	}

	virtual ~SingleSockMgr()
	{
		//CELLLog_Info("~SingleSockMgr[sId=%d id=%d socket=%d]", serverId, id, (int)_sockfd);
		destorySocket();
	}

	void destorySocket()
	{
		if (INVALID_SOCKET != _sockfd)
		{
			//CELLLog_Info("SingleSockMgr::destory[sId=%d id=%d socket=%d]", serverId, id, (int)_sockfd);
			SockCtl::destorySocket(_sockfd);
			_sockfd = INVALID_SOCKET;
		}
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	bool canWrite(int size)
	{
		return _sendBuff.canWrite(size);
	}

	int RecvData()
	{
		return _recvBuff.read4socket(_sockfd);
	}

	//check whether we have received a complete message
	virtual bool hasMsg() = 0;

	char* front_msg()
	{
		return _recvBuff.data();
	}

	//remove the first message
	virtual void pop_front_msg() = 0;

	bool needWrite()
	{
		return _sendBuff.needWrite();
	}

	//send data to socket
	int SendDataReal()
	{
		int ret = _sendBuff.write2socket(_sockfd);
		if (_sendBuff.dataLen() == 0)
		{
			onSendComplete();
		}
		return ret;
	}

	virtual void onSendComplete(){}

	int SendData(char* header)
	{
		return SendData((const char*)header, header->dataLength);
	}

	int SendData(const char* pData, int len)
	{
		if (_sendBuff.push(pData, len))
		{
			return len;
		}
		return SOCKET_ERROR;
	}

	void resetDTHeart()
	{
		_dtHeart = 0;
	}

	bool checkHeart(time_t dt)
	{
		if (isClose())
			return true;
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HREAT_DEAD_TIME)
		{
			CELLLog_Info("checkHeart dead:s=%d,time=%ld", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}

	// set client IP
	void setIP(char* ip)
	{
		if(ip)
			strncpy(_ip, ip, INET6_ADDRSTRLEN);
	}

	// get client IP
	char* getIP()
	{
		return _ip;
	}

	ClientState state()
	{
		return _clientState;
	}

	void state(ClientState state)
	{
		_clientState = state;
	}

	void onClose()
	{
		//CELLLog_Info("sockfd<%d> onClose", _sockfd);
		state(clientState_close);
	}

	bool isClose()
	{
		return _clientState == clientState_close;
	}

protected:
	// socket fd_set  file desc set
	SOCKET _sockfd = INVALID_SOCKET;
	Buffer _recvBuff;
	Buffer _sendBuff;
	//heartbeat death timeout
	time_t _dtHeart = 0;
	char _ip[INET6_ADDRSTRLEN] = {};
	ClientState _clientState = clientState_create;
};

#endif