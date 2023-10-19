#ifndef _EPOLL_HPP_
#define _EPOLL_HPP_

//----------
#include"DefaultSetting.hpp"
#include"Client.hpp"
#include"Log.hpp"
#include<sys/epoll.h>
#define EPOLL_ERROR            (-1)
//----------
class Epoll
{
public:
	~Epoll()
	{
		destory();
	}

	int create(int nMaxEvents)
	{
		if (_epfd > 0)
		{
			//Warring
			destory();
		}
		//nMaxEvents is useless after linux 2.6.8
		_epfd = epoll_create(nMaxEvents);
		if (EPOLL_ERROR == _epfd)
		{
			CELLLog_PError("epoll_create");
			return _epfd;
		}
		_pEvents = new epoll_event[nMaxEvents];
		_nMaxEvents = nMaxEvents;
		return _epfd;
	}

	void destory()
	{
		if (_epfd > 0)
		{
			NetWork::destorySocket(_epfd);
			_epfd = -1;
		}

		if (_pEvents)
		{
			delete[] _pEvents;
			_pEvents = nullptr;
		}
	}

	int ctl(int op, SOCKET sockfd, uint32_t events)
	{
		epoll_event ev;
		ev.events = events;
		ev.data.fd = sockfd;
		int ret = epoll_ctl(_epfd, op, sockfd, &ev);
		if (EPOLL_ERROR == ret)
		{
			CELLLog_PError("epoll_ctl1");
		}
		return ret;
	}

	//register event
	int ctl(int op, Client* pClient, uint32_t events)
	{
		epoll_event ev;
		ev.events = events;
		//we use data to retrieve which Client is waked up
		ev.data.ptr = pClient;
		int ret = epoll_ctl(_epfd, op, pClient->sockfd(), &ev);
		if (EPOLL_ERROR == ret)
		{
			CELLLog_PError("epoll_ctl2");
		}
		return ret;
	}

	int wait(int timeout)
	{
		int ret = epoll_wait(_epfd, _pEvents, _nMaxEvents, timeout);
		if (EPOLL_ERROR == ret)
		{
			if (errno == EINTR)
			{
				CELLLog_Info("epoll_wait EINTR");
				return 0;
			}
			CELLLog_PError("epoll_wait");
		}
		return ret;
	}

	epoll_event* events()
	{
		return _pEvents;
	}
private:
	epoll_event * _pEvents = nullptr;
	int _nMaxEvents = 1;
	int _epfd = -1;
};

#endif
