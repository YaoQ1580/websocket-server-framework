#ifndef _WebSocketServerSideParser_HPP_
#define _WebSocketServerSideParser_HPP_

#include"HttpServerSideParser.hpp"
#include"sha1.hpp"
#include"Base64Encoding.hpp"
#include"WebSocketObj.hpp"

class WebSocketServerSideParser :public HttpServerSideParser
{
public:
	WebSocketServerSideParser(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
		HttpServerSideParser(sockfd, sendSize, recvSize)
	{

	}

	bool handshake()
	{
		auto strUpgrade = this->header_getStr("Upgrade", "");
		if (0 != strcmp(strUpgrade, "websocket"))
		{
			CELLLog_Error("WebSocketServerSideParser::handshake, not found Upgrade:websocket");
			return false;
		}

		auto cKey = this->header_getStr("Sec-WebSocket-Key", nullptr);
		if (!cKey)
		{
			CELLLog_Error("WebSocketServerSideParser::handshake, not found Sec-WebSocket-Key");
			return false;
		}

		std::string sKey = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

		sKey = cKey + sKey;

		unsigned char strSha1[20] = {};
		SHA1_String((const unsigned char*)sKey.c_str(), sKey.length(), strSha1);

		std::string sKeyAccept = Base64Encode(strSha1, 20);

		char resp[256] = {};
		strcat(resp, "HTTP/1.1 101 Switching Protocols\r\n");
		strcat(resp, "Connection: Upgrade\r\n");
		strcat(resp, "Upgrade: websocket\r\n");
		strcat(resp, "Sec-WebSocket-Accept: ");
		strcat(resp, sKeyAccept.c_str());
		strcat(resp, "\r\n\r\n");

		this->SendData(resp, strlen(resp));

		return true;
	}

	//check whether we have a complete Websocket message
	virtual bool hasMsg()
	{
		if (clientState_join == _clientState)
		{
			//as per websocket protocal, the first message sent from client is Http message with "update" field
			return HttpServerSideParser::hasMsg();
		}
		else if (clientState_run == _clientState)
		{
			return hasMsgWS();
		}

		return false;
	}

	virtual void pop_front_msg()
	{
		HttpServerSideParser::pop_front_msg();
		if (_wsh.header_size > 0)
		{
			_recvBuff.pop(_wsh.header_size + _wsh.len);
			_wsh.header_size = 0;
			_wsh.len = 0;
		}
	}

	virtual bool hasMsgWS()
	{
		//a complete WebSocket data packet shouldn't have a length less than 2 bytes
		if (_recvBuff.dataLen() < 2)
			return false;

		const uint8_t *data = (const uint8_t *)_recvBuff.data();

		_wsh.header_size = 2;
		_wsh.fin = ((data[0] & 0x80) == 0x80);

		_wsh.opcode = static_cast<WebSocketOpcode>(data[0] & 0xF);

		_wsh.mask = ((data[1] & 0x80) == 0x80);
		if (_wsh.mask)
		{
			_wsh.header_size += 4;
		}

		if ((!_wsh.mask && opcode_PING != _wsh.opcode && opcode_PONG != _wsh.opcode)
			|| opcode_CLOSE == _wsh.opcode)
		{
			onClose();
			return true;
		}

		_wsh.len0 = (data[1] & 0x7F);

		if (_wsh.len0 == 126)
		{
			_wsh.header_size += 2;
		}
		else if (_wsh.len0 == 127)
		{
			_wsh.header_size += 8;
		}

		if (_recvBuff.dataLen() < _wsh.header_size)
			return false;

		if (_wsh.len0 == 126)
		{
			//0000 1111 0000 0000
			_wsh.len |= data[2] << 8;
			//0000 1111 0000 0000

			//0000 0000 0000 1111
			_wsh.len |= data[3] << 0;
			//0000 1111 0000 1111
		}
		else if (_wsh.len0 == 127)
		{
			_wsh.len |= (uint64_t)data[2] << 56;
			_wsh.len |= (uint64_t)data[3] << 48;
			_wsh.len |= (uint64_t)data[4] << 40;
			_wsh.len |= (uint64_t)data[5] << 32;
			_wsh.len |= data[6] << 24;
			_wsh.len |= data[7] << 16;
			_wsh.len |= data[8] << 8;
			_wsh.len |= data[9] << 0;
		}
		else {
			_wsh.len = _wsh.len0;
		}
		//overflow
		if (_wsh.header_size + _wsh.len > _recvBuff.buffSize())
		{
			CELLLog_Error("WebSocketServerSideParser::hasMsgWS -> _wsh.header_size + _wsh.len > _recvBuff.buffSize()");
			onClose();
			return false;
		}
		//check whether this websocket data packet is complete
		if(_recvBuff.dataLen() < _wsh.header_size + _wsh.len)
			return false;

		return true;
	}

	char* fetch_data()
	{
		char *rbuf = _recvBuff.data() + _wsh.header_size;

		if (_wsh.mask)
		{
			const uint8_t *data = (const uint8_t *)_recvBuff.data();
			_wsh.masking_key[0] = data[_wsh.header_size - 4];
			_wsh.masking_key[1] = data[_wsh.header_size - 3];
			_wsh.masking_key[2] = data[_wsh.header_size - 2];
			_wsh.masking_key[3] = data[_wsh.header_size - 1];

			for (uint64_t i = 0; i < _wsh.len; ++i)
			{
				//websocket message is encoded by XOR operation
				rbuf[i] ^= _wsh.masking_key[i & 0x3];
			}
		}

		return rbuf;
	}

	int writeHeader(WebSocketOpcode opcode, uint64_t len)
	{
		uint8_t header[10] = {};
		uint8_t header_size = 0;

		if (len < 126)
		{
			header_size = 2;
		}
		else if (len < 65536) {
			header_size = 4;
		}
		else {
			header_size = 10;
		}
		//FIN | opcode;
		header[0] = 0x80 | opcode;

		if (len < 126)
		{
			header[1] = (len & 0x7F);
		}
		else if (len < 65536) {

			header[1] = 126;
			//1111 1011 1111 1111
			//1111 1011
			header[2] = (len >> 8) & 0xFF;
			//1111 1011 1111 1111
			header[3] = (len >> 0) & 0xFF;
		}
		else {
			header[1] = 127;
			header[2] = (len >> 56) & 0xFF;
			header[3] = (len >> 48) & 0xFF;
			header[4] = (len >> 40) & 0xFF;
			header[5] = (len >> 32) & 0xFF;
			header[6] = (len >> 24) & 0xFF;
			header[7] = (len >> 16) & 0xFF;
			header[8] = (len >> 8) & 0xFF;
			header[9] = (len >> 0) & 0xFF;
		}

		int ret = SendData((const char*)header, header_size);
		if (SOCKET_ERROR == ret)
		{
			CELLLog_Error("WebSocketServerSideParser::writeHeader -> SendData -> SOCKET_ERROR");
		}
		return ret;
	}

	int writeText(const char* pData, int len)
	{
		if (!canWrite(len + 16))
		{
			CELLLog_Error("WebSocketClientC::writeText -> !canWrite");
			return SOCKET_ERROR;
		}

		int ret = writeHeader(opcode_TEXT, len);
		if (SOCKET_ERROR != ret)
		{
			ret = SendData(pData, len);
			if (SOCKET_ERROR == ret)
			{
				CELLLog_Error("WebSocketServerSideParser::writeText -> SendData -> SOCKET_ERROR");
			}
		}
		return ret;
	}

	int ping()
	{
		return writeHeader(opcode_PING, 0);
	}

	int pong()
	{
		return writeHeader(opcode_PONG, 0);
	}

	WebSocketHeader& WebsocketHeader()
	{
		return _wsh;
	}
private:
	WebSocketHeader _wsh = {};
};

#endif
