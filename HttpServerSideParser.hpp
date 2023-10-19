#ifndef _HttpServerSideParser_HPP_
#define _HttpServerSideParser_HPP_

#include"SingleSockMgr.hpp"
#include"SplitString.hpp"
#include"KeyString.hpp"
#include<map>

class HttpServerSideParser:public SingleSockMgr
{
public:
	enum RequestType
	{
		GET = 10,
		POST,
		UNKOWN
	};
public:
	HttpServerSideParser(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
		SingleSockMgr(sockfd, sendSize,recvSize)
	{

	}

	//check whether we have a complete Http message
	virtual bool hasMsg()
	{
		//a complete http must be more than 20 bytes
		if (_recvBuff.dataLen() < 20)
			return false;

		int ret = checkHttpRequest();
		if (ret < 0)
			resp400BadRequest();
		return ret > 0;
	}

	// 0 messgae incomplete, should wait for some other time
	// -1 unsupported message type
	// -2 abnormal message
	int checkHttpRequest()
	{
		//check end mark
		char* temp = strstr(_recvBuff.data(), "\r\n\r\n");
		//data is incomplete if we don't find one
		if (!temp)
			return 0;

		//bypass "\r\n\r\n"
		temp += 4;
		//compute http header length
		_headerLen = temp - _recvBuff.data();

		temp = _recvBuff.data();
		if (temp[0] == 'G' &&
			temp[1] == 'E' &&
			temp[2] == 'T')
		{
			_requestType = HttpServerSideParser::GET;
		}
		else if (
			temp[0] == 'P' &&
			temp[1] == 'O' &&
			temp[2] == 'S' &&
			temp[3] == 'T')
		{
			_requestType = HttpServerSideParser::POST;
			//computer the length of body of POST
			char* p1 = strstr(_recvBuff.data(), "Content-Length: ");
			if (!p1)
				return -2;

			//Content-Length: 1024\r\n
			//16=strlen("Content-Length: ")
			p1 += 16;
			char* p2 = strchr(p1, '\r');
			if (!p2)
				return -2;

			//compute length of the string of the length number
			int n = p2 - p1;
			//we only support receive 1MB(6 figues) data per Http response
			if (n > 6)
				return -2;
			char lenStr[7] = {};
			strncpy(lenStr, p1, n);
			_bodyLen = atoi(lenStr);

			if(_bodyLen < 0)
				return -2;

			//if total message length surpass buffer size, we return error
			if (_headerLen + _bodyLen > _recvBuff.buffSize())
				return -2;
			//if we haven't receive all data of a single response
			if (_headerLen + _bodyLen > _recvBuff.dataLen())
				return 0;
		}
		else {
			_requestType = HttpServerSideParser::UNKOWN;
			return -1;
		}

		return _headerLen;
	}

	//parse a complete Http message
	bool getRequestInfo()
	{
		//check if we have a complete Http data packet
		if (_headerLen <= 0)
			return false;
		
		char* pp = _recvBuff.data();
		//set the end of header as '\0' because we want to first parse header
		pp[_headerLen-1] = '\0';

		SplitString ss;
		ss.set(_recvBuff.data());
		//client request example: "GET /login.php?a=5 HTTP/1.1\r\n"
		char* temp = ss.get("\r\n");
		if (temp)
		{
			_header_map["RequestLine"] = temp;
			request_args(temp);
		}

		//request header line example: "Connection: Keep-Alive\r\n"
		while (true)
		{
			//each response line has "\r\n"
			temp = ss.get("\r\n");
			if (temp)
			{
				//"Connection: Keep-Alive\0\n"
				SplitString ss2;
				ss2.set(temp);
				//each response header line has the format of "key: val\r\n"
				char* key = ss2.get(": ");
				char* val = ss2.get(": ");
				if (key && val)
				{
					//key = Connection
					//val = Keep-Alive
					_header_map[key] = val;
				}
			}
			else {
				break;
			}
		}

		//process request body
		if (_bodyLen > 0)
		{
			SplitUrlArgs(_recvBuff.data() + _headerLen);
		}

		//process according to corresponding field
		const char* str = header_getStr("Connection", "");
		_keepalive = (0 == strcmp("keep-alive", str)
			|| 0 == strcmp("Keep-Alive", str)
			|| 0 == strcmp("Upgrade", str)
			);
		
		return true;
	}
	
	//parse url parameters which are seperated by "&"
	void SplitUrlArgs(char* args)
	{
		SplitString ss;
		ss.set(args);
		while (true)
		{
			char* temp = ss.get('&');
			if (temp)
			{
				SplitString ss2;
				ss2.set(temp);
				char* key = ss2.get('=');
				char* val = ss2.get('=');
				if (key && val)
					_args_map[key] = val;
			}
			else {
				break;
			}
		}
	}

	bool request_args(char* requestLine)
	{
		//requestLine="GET /login.php?a=5 HTTP/1.1"
		SplitString ss;
		ss.set(requestLine);
		//requestLine="GET\0/login.php?a=5 HTTP/1.1"
		_method = ss.get(' ');
		if (!_method)
			return false;

		_url = ss.get(' ');
		if (!_url)
			return false;

		_httpVersion = ss.get(' ');
		if (!_httpVersion)
			return false;

		ss.set(_url);
		_url_path = ss.get('?');
		if (!_url_path)
			return false;

		_url_args = ss.get('?');
		if (!_url_args)
			return true;

		SplitUrlArgs(_url_args);

		return true;
	}

	virtual void pop_front_msg()
	{
		if (_headerLen > 0)
		{
			_recvBuff.pop(_headerLen + _bodyLen);
			_headerLen = 0;
			_bodyLen = 0;
			//clear related data of poped message
			_args_map.clear();
			_header_map.clear();
		}
	}

	void resp400BadRequest()
	{
		writeResponse("400 Bad Request", "Only support GET or POST.", 25);
	}

	void resp404NotFound()
	{
		writeResponse("404 Not Found", "(^o^): 404!", 11);
	}

	void resp200OK(const char* bodyBuff, int bodyLen)
	{
		writeResponse("200 OK", bodyBuff, bodyLen);
	}

	void writeResponse(const char* code, const char* bodyBuff, int bodyLen)
	{
		char respBodyLen[32] = {};
		sprintf(respBodyLen, "Content-Length: %d\r\n", bodyLen);
		char response[256] = {};
		strcat(response, "HTTP/1.1 ");
		strcat(response, code);
		strcat(response, "\r\n");
		strcat(response, "Content-Type: text/html;charset=UTF-8\r\n");
		strcat(response, "Access-Control-Allow-Origin: *\r\n");
		strcat(response, "Connection: Keep-Alive\r\n");
		strcat(response, respBodyLen);
		strcat(response, "\r\n");
		SendData(response, strlen(response));
		SendData(bodyBuff, bodyLen);
	}

	char* url()
	{
		return _url_path;
	}

	bool url_compre(const char* str)
	{
		return 0 == strcmp(_url_path, str);
	}

	bool has_args(const char* key)
	{
		return _args_map.find(key) != _args_map.end();
	}

	bool has_header(const char* key)
	{
		return _header_map.find(key) != _header_map.end();
	}

	int args_getInt(const char* argName, int def)
	{
		auto itr = _args_map.find(argName);
		if (itr != _args_map.end())
		{
			def = atoi(itr->second);
		}
		return def;
	}

	const char* args_getStr(const char* argName, const char* def)
	{
		auto itr = _args_map.find(argName);
		if (itr != _args_map.end())
		{
			return itr->second;
		}
		return def;
	}

	const char* header_getStr(const char* argName, const char* def)
	{
		auto itr = _header_map.find(argName);
		if (itr != _header_map.end())
		{
			return itr->second;
		}
		else {
			return def;
		}
	}

	virtual void onSendComplete()
	{
		if (!_keepalive)
		{
			this->onClose();
		}
	}
protected:
	int _headerLen = 0;
	int _bodyLen = 0;
	std::map<KeyString, char*> _header_map;
	std::map<KeyString, char*> _args_map;
	RequestType _requestType = HttpServerSideParser::UNKOWN;
	char* _method;
	char* _url;
	char* _url_path;
	char* _url_args;
	char* _httpVersion;
	bool _keepalive = true;
};

#endif
