#ifndef _NET_CALLBACK_INTF_HPP_
#define _NET_CALLBACK_INTF_HPP_

#include"DefaultSetting.hpp"
#include"SingleSockMgr.hpp"

class SockGroupMgr;

class NetCallbackIntf
{
public:
	//client join event
	virtual void OnNetJoin(SingleSockMgr* pClient) = 0;
	//client leaving event
	virtual void OnNetLeave(SingleSockMgr* pClient) = 0;
	//client having message event
	virtual void OnNetMsg(SockGroupMgr* pServer, SingleSockMgr* pClient, char* header) = 0;
	//recv event
	virtual void OnNetRecv(SingleSockMgr* pClient) = 0;
	//SockGroupMgr loop function
	virtual void OnNetRun(SockGroupMgr* pServer) = 0;
private:

};

#endif
