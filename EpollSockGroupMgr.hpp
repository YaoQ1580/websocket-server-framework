#ifndef _EpollSockGroupMgr_HPP_
#define _EpollSockGroupMgr_HPP_

#include"SockGroupMgr.hpp"
#include"Epoll.hpp"

class EpollSockGroupMgr :public SockGroupMgr
{
public:
	EpollSockGroupMgr()
	{

	}

	~EpollSockGroupMgr() noexcept
	{
		Close();
	}

	virtual void setClientNum(int nSocketNum)
	{
		_ep.create(nSocketNum);
	}

	bool DoNetEvents()
	{
		for (auto iter : _clients)
		{	//only the clients who have data cached to be sent will be monitored to be written
			if (iter.second->needWrite())
			{
				_ep.ctl(EPOLL_CTL_MOD, iter.second, EPOLLIN | EPOLLOUT);
			}
			else {
				_ep.ctl(EPOLL_CTL_MOD, iter.second, EPOLLIN);
			}
		}

		int ret = _ep.wait(1);
		if (ret < 0)
		{
			CELLLog_Error("EpollSockGroupMgr%d.OnRun.wait", _id);
			return false;
		}
		else if (ret == 0)
		{
			return true;
		}

		auto events = _ep.events();
		for (int i = 0; i < ret; i++)
		{
			SingleSockMgr* pClient = (SingleSockMgr*)events[i].data.ptr;

			if (pClient)
			{
				if (events[i].events & EPOLLIN)
				{
					if (SOCKET_ERROR == RecvData(pClient))
					{
						rmClient(pClient);
						continue;
					}
				}
				if (events[i].events & EPOLLOUT)
				{
					if (SOCKET_ERROR == pClient->SendDataReal())
					{
						rmClient(pClient);
					}
				}
			}
		}
		return true;
	}

	void rmClient(SingleSockMgr* pClient)
	{
		auto iter = _clients.find(pClient->sockfd());
		if (iter != _clients.end())
			_clients.erase(iter);

		OnClientLeave(pClient);
	}

	void OnClientJoin(SingleSockMgr* pClient)
	{
		_ep.ctl(EPOLL_CTL_ADD, pClient, EPOLLIN);
	}
private:
	Epoll _ep;
};

#endif
