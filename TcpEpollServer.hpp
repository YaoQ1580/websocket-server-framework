#ifndef _EpollServer_hpp_
#define _EpollServer_hpp_

#include"TcpServerCommon.hpp"
#include"EpollSockGroupMgr.hpp"
#include"Epoll.hpp"

class TcpEpollServer : public TcpServerCommon
{
public:
	void Start(int nCELLServer)
	{
		TcpServerCommon::Start<EpollSockGroupMgr>(nCELLServer);
	}
protected:
	//process listen socket message
	void OnRun(Thread* pThread)
	{
		Epoll ep;
		ep.create(_nMaxClient);
		ep.ctl(EPOLL_CTL_ADD, sockfd(), EPOLLIN);
		while (pThread->isRun())
		{
			time4msg();

			int ret = ep.wait(1);
			if (ret < 0)
			{
				CELLLog_Error("EasyEpollServer.OnRun ep.wait exit.");
				pThread->Exit();
				break;
			}

			auto events = ep.events();
			for (int i = 0; i < ret; i++)
			{
				//incoming new clients
				if (events[i].data.fd == sockfd())
				{
					if (events[i].events & EPOLLIN)
					{
						Accept();
					}
				}
			}
		}
	}
};

#endif
