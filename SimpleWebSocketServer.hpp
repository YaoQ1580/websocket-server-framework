#ifndef _SimpleWebSocketServer_HPP_
#define _SimpleWebSocketServer_HPP_

#include"TcpEpollServer.hpp"
#include"WebSocketServerSideParser.hpp"

class SimpleWebSocketServer :public TcpEpollServer
{
public:
	//callbacks set by user
	//when a connection socket leave or has error
	std::function<void(WebSocketServerSideParser*)> on_client_leave = nullptr;
	//when a connection socket being accepted
	std::function<void(WebSocketServerSideParser*)> on_client_join = nullptr;
	//when a connection socket has a complete websocket message
	std::function<void(SockGroupMgr*, WebSocketServerSideParser*, std::string&)> on_websock_msg = nullptr;

private:
	virtual SingleSockMgr* makeClientObj(SOCKET cSock)
	{
		return new WebSocketServerSideParser(cSock, _nSendBuffSize, _nRecvBuffSize);
	}

	virtual void OnNetMsg(SockGroupMgr* pServer, SingleSockMgr* pClient, char* header)
	{
		TcpServerCommon::OnNetMsg(pServer, pClient, header);
		WebSocketServerSideParser* pWSClient = dynamic_cast<WebSocketServerSideParser*>(pClient);
		if (!pWSClient)
			return;

		pWSClient->resetDTHeart();

		if (clientState_join == pWSClient->state())
		{	
			if (!pWSClient->getRequestInfo())
				return;

			if (pWSClient->handshake())
				pWSClient->state(clientState_run);
			else
				pWSClient->onClose();
		}
		else if (clientState_run == pWSClient->state()) {
			WebSocketHeader& wsh = pWSClient->WebsocketHeader();
			if (wsh.opcode == opcode_PING)
			{
				pWSClient->pong();
			}
			else {
				OnNetMsgWS(pServer, pWSClient);
			}
		}
	}

	virtual void OnNetMsgWS(SockGroupMgr* pServer, WebSocketServerSideParser* pWSClient)
	{
		WebSocketHeader& wsh = pWSClient->WebsocketHeader();
		if (wsh.opcode == opcode_PONG)
		{
			//CELLLog_Info("websocket client say: PONG");
			return;
		}
		auto pStr = pWSClient->fetch_data();
		std::string dataStr(pStr, wsh.len);

		if (on_websock_msg)
			on_websock_msg(pServer, pWSClient, dataStr);
	}

	virtual void OnNetLeave(SingleSockMgr* pClient)
	{
		TcpServerCommon::OnNetLeave(pClient);

		WebSocketServerSideParser* pWSClient = dynamic_cast<WebSocketServerSideParser*>(pClient);
		if (!pWSClient)
			return;

		if (on_client_leave)
			on_client_leave(pWSClient);
	}

	virtual void OnNetJoin(SingleSockMgr* pClient)
	{
		TcpServerCommon::OnNetLeave(pClient);

		WebSocketServerSideParser* pWSClient = dynamic_cast<WebSocketServerSideParser*>(pClient);
		if (!pWSClient)
			return;

		if (on_client_join)
			on_client_join(pWSClient);
	}
};

#endif
