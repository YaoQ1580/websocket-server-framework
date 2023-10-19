#ifndef _SockGroupMgr_HPP_
#define _SockGroupMgr_HPP_

#include"DefaultSetting.hpp"
#include"NetCallbackIntf.hpp"
#include"SingleSockMgr.hpp"
#include"Semaphore.hpp"

#include<vector>
#include<map>

class SockGroupMgr
{
public:
	virtual ~SockGroupMgr()
	{
		CELLLog_Info("SockGroupMgr%d.~SockGroupMgr exit begin", _id);
		Close();
		CELLLog_Info("SockGroupMgr%d.~SockGroupMgr exit end", _id);
	}

	void setId(int id)
	{
		_id = id;
		_taskServer.serverId = id;
	}

	virtual void setClientNum(int nSocketNum)
	{

	}

	void setCallback(NetCallbackIntf* intf)
	{
		_pNetCallback = intf;
	}

	void Close()
	{
		CELLLog_Info("SockGroupMgr%d.Close begin", _id);
		_taskServer.Close();
		_thread.Close();
		CELLLog_Info("SockGroupMgr%d.Close end", _id);
	}

	//process network message loop func
	void OnRun(Thread* pThread)
	{
		while (pThread->isRun())
		{
			if (!_clientsBuff.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
					pClient->serverId = _id;
					if (_pNetCallback)
						_pNetCallback->OnNetJoin(pClient);
					OnClientJoin(pClient);
					pClient->state(clientState_join);
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			if (_pNetCallback)
				_pNetCallback->OnNetRun(this);

			if (_clients.empty())
			{
				Thread::Sleep(1);
				_oldTime = Time::getNowInMilliSec();
				continue;
			}

			CheckTime();
			if (!DoNetEvents())
			{
				pThread->Exit();
				break;
			}
			DoMsg();
		}
		CELLLog_Info("SockGroupMgr%d.OnRun exit", _id);
	}

	virtual bool DoNetEvents() = 0;

	void CheckTime()
	{
		auto nowTime = Time::getNowInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		SingleSockMgr* pClient = nullptr;
		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			pClient = iter->second;
			//check hearbeat
			if (pClient->checkHeart(dt))
			{
				OnClientLeave(pClient);
				iter = _clients.erase(iter);
				continue;
			}

			iter++;
		}
	}

	void OnClientLeave(SingleSockMgr* pClient)
	{
		if (_pNetCallback)
			_pNetCallback->OnNetLeave(pClient);
		_clients_change = true;
		delete pClient;
	}

	virtual void OnClientJoin(SingleSockMgr* pClient)
	{

	}

	void OnNetRecv(SingleSockMgr* pClient)
	{
		if (_pNetCallback)
			_pNetCallback->OnNetRecv(pClient);
	}

	void DoMsg()
	{
		SingleSockMgr* pClient = nullptr;
		for (auto itr : _clients)
		{
			pClient = itr.second;
			while (pClient->hasMsg())
			{
				OnNetMsg(pClient, pClient->front_msg());
				pClient->pop_front_msg();
			}
		}
	}

	//when receiving any data from client
	int RecvData(SingleSockMgr* pClient)
	{
		int nLen = pClient->RecvData();
		if (_pNetCallback)
			_pNetCallback->OnNetRecv(pClient);
		return nLen;
	}

	//when receiving a complete net message
	virtual void OnNetMsg(SingleSockMgr* pClient, char* header)
	{
		if (_pNetCallback)
			_pNetCallback->OnNetMsg(this, pClient, header);
	}

	void addClient(SingleSockMgr* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuff.push_back(pClient);
	}

	void Start()
	{
		_taskServer.Start();
		_thread.Start(
			//onCreate
			nullptr,
			//onRun
			[this](Thread* pThread) {
			OnRun(pThread);
		},
			//onDestory
			[this](Thread* pThread) {
			ClearClients();
		}
		);
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	SingleSockMgr* find_client(int id)
	{
		auto iter = _clients.find((SOCKET)id);
		if (iter != _clients.end())
			return iter->second;
		return nullptr;
	}

private:
	void ClearClients()
	{
		for (auto iter : _clients)
		{
			delete iter.second;
		}
		_clients.clear();

		for (auto iter : _clientsBuff)
		{
			delete iter;
		}
		_clientsBuff.clear();
	}
protected:
	std::map<SOCKET, SingleSockMgr*> _clients;
private:
	std::vector<SingleSockMgr*> _clientsBuff;
	std::mutex _mutex;

	NetCallbackIntf* _pNetCallback = nullptr;
	TaskServer _taskServer;

	time_t _oldTime = Time::getNowInMilliSec();

	Thread _thread;
protected:
	int _id = -1;
	bool _clients_change = true;
};

#endif
