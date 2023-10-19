#ifndef _SETTING_HPP_
#define _SETTING_HPP_

#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<signal.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<net/if.h>
#include<netdb.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#include"Timestamp.hpp"
#include"Task.hpp"
#include"Log.hpp"

#include<stdio.h>

//default buffer size
#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 8192
#define SEND_BUFF_SZIE 10240
#endif

#endif