// author: hubo
// time  : 2020/08
#ifndef __SOCKET_CLASS_H__
#define __SOCKET_CLASS_H__

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>

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
				LOG_WARN << "clinet connect error!!! " 
					<< " error is : (" << strerror(errno)  << ")"
					<< " failed accept_fd is " << connectfd 
					<< " ip is " << inet_ntoa(client.sin_addr)
					<< " port is " << ntohs(client.sin_port);
				return ERROR;
			}
		}
		return connectfd;
	}
	~SocketBase()
	{
		close(_sockfd);
	}

	int32 GetSockfd() { return _sockfd; }

protected:
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

class SocketEpoll 
{
public:
	SocketEpoll(SocketConf *conf):
	   	_socket_conf(*conf), _socket_io(conf),
		_epoll_evs(NULL), _nwait_events(0) { }

	int32 Init()
	{
		int32 ret = _socket_io.Init();
		if(ret == SocketBase::ERROR)
		{
			LOG_WARN << "SocketBase Init error!!!";
			return ret;
		}
		_epollfd = epoll_create(1);
		if(_epollfd < 0)
		{
			LOG_WARN << "epoll_create error!!!";
			return SocketBase::ERROR;
		}
		_epoll_evs = new struct epoll_event[_socket_conf._n_listen];
		if(_epoll_evs == NULL)
		{
			LOG_WARN << "new epoll_event failed!!!";
			return SocketBase::ERROR;
		}
		return SocketBase::OK;
	}

	int32 Bind()
	{
		int32 ret = _socket_io.Bind();
		if(ret == SocketBase::ERROR)
		{
			LOG_WARN << "SocketBase Bind error!!!";
			return ret;
		}
		return ret;
	}

	int32 Listen()
	{
		int32 ret = _socket_io.Listen();
		if(ret == SocketBase::ERROR)
		{
			LOG_WARN << "SocketBase listen error!!!";
			return ret;
		}
		int flags = fcntl(_socket_io.GetSockfd(), F_GETFL, 0);
		if(flags < 0)
		{
			LOG_WARN << "fcntl error!!!";
			return SocketBase::ERROR;
		}
		ret = fcntl(_socket_io.GetSockfd(), F_SETFL, flags| O_NONBLOCK);
		if(ret < 0)
		{
			LOG_WARN << "fcntl error!!!";
			return SocketBase::ERROR;
		}
		struct epoll_event epoll_ev;
		memset(&epoll_ev, 0, sizeof(epoll_ev));
		epoll_ev.events = EPOLLIN;
		epoll_ev.data.fd = _socket_io.GetSockfd();
		ret = epoll_ctl(_epollfd, EPOLL_CTL_ADD, 
				_socket_io.GetSockfd(), &epoll_ev);
		if(ret < 0)
		{
			LOG_WARN << "epoll_ctl failed!!!";
			return SocketBase::ERROR;
		}
		return ret;
	}

	int32 Accept(std::vector<int> &connfds)
	{
		connfds.clear();
		int32 ret = epoll_wait(_epollfd, _epoll_evs, _socket_conf._n_listen, _socket_conf._rec_timeout/1000);
		if(ret < 0)
		{
			LOG_WARN << "epoll_wait failed!!!";
			return -2;
		}
		for(int i = 0 ; i < ret ; ++i)
		{
			int fd = _epoll_evs[i].data.fd;
			if(fd == _socket_io.GetSockfd())
			{
				while(1)
				{
					int connfd = _socket_io.Accept();
					if(connfd < 0)
					{
						return connfds.size();
					}
					connfds.push_back(connfd);
				}
			}
			else
			{
				LOG_WARN << "It's shouldn't happend!!!";
			}
		}
		return -2;
	}


	~SocketEpoll()
	{
		if(_epoll_evs != NULL)
			delete [] _epoll_evs;
		_epoll_evs = NULL;
		_epollfd = -1;
	}
private:

	const SocketConf &_socket_conf;
	SocketBase _socket_io;
	int _epollfd;
	struct epoll_event *_epoll_evs; // the epoll event length is n_listen
	int32 _nwait_events;            // use Accept record wait event times.
};
#include "src/util/namespace-end.h"

#endif
