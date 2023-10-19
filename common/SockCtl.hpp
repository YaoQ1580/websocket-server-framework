#ifndef _SOCKCTL_HPP_
#define _SOCKCTL_HPP_

#include"DefaultSetting.hpp"
#include"Log.hpp"

//provide some common socket manipulation function
class SockCtl
{
private:
	SockCtl()
	{}

	~SockCtl()
	{}
public:
	static void Init()
	{
		static  SockCtl obj;
	}

	static int make_nonblocking(SOCKET fd)
	{
		{
			int flags;
			if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
				CELLLog_Warring("fcntl(%d, F_GETFL)", fd);
				return -1;
			}
			if (!(flags & O_NONBLOCK)) {
				if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
					CELLLog_Warring("fcntl(%d, F_SETFL)", fd);
					return -1;
				}
			}
		}
		return 0;
	}

	static int make_reuseaddr(SOCKET fd)
	{
		int flag = 1;
		if (SOCKET_ERROR == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag))) {
			CELLLog_Warring("setsockopt socket<%d> SO_REUSEADDR failed", (int)fd);
			return SOCKET_ERROR;
		}
		return 0;
	}

	static int make_nodelay(SOCKET fd)
	{
		int flag = 1;
		if (SOCKET_ERROR == setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag))) {
			CELLLog_Warring("setsockopt socket<%d> IPPROTO_TCP TCP_NODELAY failed", (int)fd);
			return SOCKET_ERROR;
		}
		return 0;
	}

	static int destorySocket(SOCKET sockfd)
	{
		int ret = close(sockfd);
		if (ret < 0)
			CELLLog_PError("destory sockfd<%d>", (int)sockfd);
		return ret;
	}
};

#endif
