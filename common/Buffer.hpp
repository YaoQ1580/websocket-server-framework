#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include "DefaultSetting.hpp"

//a simple userspace buffer
class Buffer
{
public:
	Buffer(int nSize = 8192)
	{
		_nSize = nSize;
		//+1 to store another '\0'
		_pBuff = new char[_nSize+1];
	}

	~Buffer()
	{
		if (_pBuff)
		{
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

	inline char* data()
	{
		return _pBuff;
	}

	inline int dataLen()
	{
		return _nLast;
	}

	inline int buffSize()
	{
		return _nSize;
	}

	bool canWrite(int size)
	{
		return size < _nSize - _nLast;
	}

	bool push(const char* pData, int nLen)
	{
		if (_nLast + nLen <= _nSize)
		{
			//push the to-be-sent data to the end of the buffer
			memcpy(_pBuff + _nLast, pData, nLen);
			//update last pos
			_nLast += nLen;

			return true;
		}

		return false;
	}

	// pop the front nLen data
	void pop(int nLen)
	{
		int n = _nLast - nLen;
		if (n > 0)
		{
			// overwrite data in (0, n) by data from (nLen, _nLast)
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		_nLast = n;
	}

	// WARNING: not thread-safe
	int write2socket(SOCKET sockfd)
	{
		int ret = 0;
		if (_nLast > 0 && INVALID_SOCKET != sockfd)
		{
			//send all data via socket
			ret = send(sockfd, _pBuff, _nLast, 0);
			if (ret == 0)
			{
				CELLLog_Info("write2socket1:sockfd<%d> client socket closed.", sockfd, _nSize, _nLast, ret);
				return SOCKET_ERROR;
			}
			else if (ret < 0)
			{
				CELLLog_PError("write2socket1:sockfd<%d> nSize<%d> nLast<%d> ret<%d>", sockfd, _nSize, _nLast, ret);
				return SOCKET_ERROR;
			}
			if (ret == _nLast)
			{
				_nLast = 0;
			}
			else {
				// data being partially sent
				_nLast -= ret;
				memcpy(_pBuff, _pBuff + ret, _nLast);
			}
		}
		return ret;
	}

	// WARNING: not thread-safe
	int read4socket(SOCKET sockfd)
	{
		if (_nSize - _nLast > 0)
		{
			char* szRecv = _pBuff + _nLast;
			// read as much as we can
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			if (nLen == 0)
			{
				CELLLog_Info("read4socket:sockfd<%d> client socket closed.", sockfd, _nSize, _nLast, nLen);
				return SOCKET_ERROR;
			}
			else if (nLen < 0)
			{
				CELLLog_PError("read4socket:sockfd<%d> nSize<%d> nLast<%d> nLen<%d>", sockfd, _nSize, _nLast, nLen);
				return SOCKET_ERROR;
			}

			_nLast += nLen;
			// pad 0 to the end to make C++ str operator work
			_pBuff[_nLast] = 0;
			return nLen;
		}
		return 0;
	}

	// check whether we have cached data which should be sent to network
	inline bool needWrite()
	{
		return _nLast > 0;
	}

private:
	//the buffer
	char* _pBuff = nullptr;
	//the last pos of currently stored data
	int _nLast = 0;
	//size of this buffer, with unit of byte
	int _nSize = 0;
};

#endif
