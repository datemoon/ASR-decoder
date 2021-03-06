#ifndef __SOCKET_CLASS_H__
#define __SOCKET_CLASS_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "src/util/util-type.h"
#include "src/util/config-parse-options.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

struct SocketConf
{
	std::string _ip;
	int32 _port;

	int32 _keepalive; // open keepalive if no 0.
	int32 _keeptime;  // 5s no data
	int32 _keepinterval; // 
	int32 _keepcount;    // 
	int32 _n_listen;             // max listen, advised 5.

	int32 _rec_timeout; // recive timeout usec
public:
	SocketConf()
	{
		_ip = "127.0.0.1";
		_port = 8000;
		_keepalive = 1;
		_keeptime = 5;
		_keepinterval = 1;
		_keepcount = 3;
		_n_listen = 5;
		_rec_timeout = 5000000;
	}
	void Register(ConfigParseOptions *conf)
	{
		conf->Register("ip", &_ip, "socket address , Default "+_ip);
		conf->Register("port", &_port, "socket port. Default 8000");
		conf->Register("keepalive", &_keepalive, "socket keepalive switch, default is 1(open),0(close)");
		conf->Register("keeptime", &_keeptime, "if keepalive open, keeptime it's no data to send data time. Default 5s");
		conf->Register("keepinterval", &_keepinterval, "if keepalive open, keep interval. Defalut 1s");
		conf->Register("keepcount", &_keepcount, "if keepalive open, max keeptime times. Default 3");
		conf->Register("n_listen", &_n_listen, "listen max");
		conf->Register("rec_timeout", &_rec_timeout, "ust usec, Default 5000000(5s)");
	}
	void Info()
	{
		LOG_COM << "*******************SocketInfo********************";
		LOG_COM << "ip\t:" << _ip;
		LOG_COM << "port\t:" << _port;
		LOG_COM << "keepalive\t:" << _keepalive;
		LOG_COM << "keeptime\t:" << _keeptime;
		LOG_COM << "keepinterval\t:" << _keepinterval;
		LOG_COM << "keepcount\t:" << _keepcount;
		LOG_COM << "n_listen\t:" << _n_listen;
		LOG_COM << "rec_timeout(usec)\t:" << _rec_timeout;
		LOG_COM << "*************************************************";
	}
};

class SocketBase
{
public:
	//typedef ASTType::int32 int32;
	//typedef ASTType::int16 int16;
	enum RetVal
	{
		ERROR = -1,
		OK = 0
	};
public:
	SocketBase()
	{
		_ip = "127.0.0.1";
		_port = 8000;
		_keepalive = 1;
		_keeptime = 5;
		_keepinterval = 1;
		_keepcount = 3;
		_n_listen = 5;
		_rec_timeout.tv_sec = 5;
		_rec_timeout.tv_usec = 0;
		_linger = {1,0};
	}
	SocketBase(SocketConf *conf)
	{
		_ip = conf->_ip;
		_port = conf->_port;
		_keepalive = conf->_keepalive;
		_keeptime = conf->_keeptime;
		_keepinterval = conf->_keepinterval;
		_keepcount = conf->_keepcount ;
		_n_listen = conf->_n_listen;
		_rec_timeout.tv_sec = conf->_rec_timeout/1000000;
		_rec_timeout.tv_usec = conf->_rec_timeout%1000000;
		_linger = {1,0};

	}
	SocketBase(std::string ip, int32 port, int32 keepalive=1,
			int32 keeptime=5, int32 keepinterval=1, int32 keepcount=3,
			int32 n_listen=5, int32 usec=5000000):
		_ip(ip), _port(port), _keepalive(keepalive), _keeptime(keeptime),
		_keepinterval(keepinterval), _keepcount(keepcount), _n_listen(n_listen)
	{
		_rec_timeout.tv_sec = usec/1000000;
		_rec_timeout.tv_usec = usec%1000000;
		_linger = {1,0};
	}

	int32 Init()
	{
		_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(_sockfd == -1)
		{
			LOG_WARN << "socket error!!!";
			return ERROR;
		}
		memset(&_address, 0, sizeof(_address));
		_address.sin_family = AF_INET;
		inet_aton(_ip.c_str(), &_address.sin_addr);
		_address.sin_port = htons(_port);
		if (_keepalive == 1)
		{
			if(setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, 
						(void*)&_keepalive,sizeof(_keepalive)) != 0)
			{
				LOG_WARN << "socket set keepalive failed!!!";
				return ERROR;
			}
			if(setsockopt(_sockfd, SOL_TCP, TCP_KEEPIDLE,
						(void*)&_keeptime, sizeof(_keeptime)) != 0)
			{
				LOG_WARN << "socket set TCP_KEEPIDLE failed!!!";
				return ERROR;
			}
			if(setsockopt(_sockfd, SOL_TCP, TCP_KEEPINTVL, 
						(void*)&_keepinterval, sizeof(_keepinterval)) != 0)
			{
				LOG_WARN << "socket set TCP_KEEPINTVL failed!!!";
				return ERROR;
			}
			if(setsockopt(_sockfd, SOL_TCP, TCP_KEEPCNT,
						(void*)&_keepcount, sizeof(_keepcount)) != 0)
			{
				LOG_WARN << "socket set TCP_KEEPCNT failed!!!";
				return ERROR;
			}
		}// set keepalive ok
		if (setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO,
					(void*)&_rec_timeout, sizeof(_rec_timeout)) != 0)
		{
			LOG_WARN << "socket set SO_RCVTIMEO failed!!!";
			return ERROR;
		}
		int flag = 1;
		if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR,
				   	&flag, sizeof(flag)) != 0)
		{
			LOG_WARN << "socket set SO_REUSEADDR failed!!!";
			return ERROR;
		}
/*
		if(setsockopt(_sockfd, SOL_SOCKET, SO_LINGER,
				   	&_linger, sizeof(_linger)) != 0)
		{
			LOG_WARN << "socket set SO_LINGER failed!!!";
			return ERROR;
		}
*/
		return _sockfd;
	}

	int32 Bind()
	{
		if(bind(_sockfd, (struct sockaddr *)&_address, sizeof(_address)) != 0)
		{
			LOG_WARN << "bind failed!!!";
			return ERROR;
		}
		return OK;
	}

	int32 Listen()
	{
		if(listen(_sockfd, _n_listen) != 0)
		{
			LOG_WARN << "listen failed!!!";
			return ERROR;
		}
		return OK;
	}
	
	int32 Accept()
	{
		struct sockaddr_in client;
		socklen_t sock_len = sizeof(client);
		int connectfd = accept(_sockfd, (struct sockaddr *)&client, &sock_len);
		if(connectfd < 0)
		{
			if(errno == EAGAIN)
			{
				return -2;   // recive timeout
			}
			else
			{
				LOG_WARN << "clinet connect error!!!";
				return ERROR;
			}
		}
		return connectfd;
	}
	~SocketBase()
	{
		close(_sockfd);
	}

private:
	struct sockaddr_in _address;
	std::string _ip;
	int16 _port;

	int32 _keepalive; // open keepalive if no 0.
	int32 _keeptime;  // 5s no data
	int32 _keepinterval; // 
	int32 _keepcount;    // 
	int32 _n_listen;             // max listen, advised 5.

	struct timeval _rec_timeout; // recive timeout
	struct linger _linger;
	int32 _sockfd;
};

#include "src/util/namespace-end.h"

#endif
