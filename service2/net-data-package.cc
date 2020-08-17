
#include <string.h>
#include "service2/net-data-package.h"

bool C2SPackageHeadAnalysis::C2SWrite(int sockfd, 
		const void *data, size_t data_size, uint n, uint end_flag)
{
	if(end_flag == 0 && data_size == 0)
	{
		std::cout <<  "send null package." << std::endl;
		return true;
	}
	if(SetEndFLag(end_flag) != true)
	{
		std::cerr << "SetEndFLag error." << std::endl;
		return false;
	}
	SetN(n);
	SetDataLen(data_size);
	ssize_t ret = write(sockfd, static_cast<void *>(&_c2s_package_head),
			sizeof(_c2s_package_head));
	if(ret < 0)
	{
		std::cerr << "write C2SPackageHead failed." << std::endl;
		return false;
	}
	if(data_size > 0)
	{
		ret = write(sockfd, static_cast<void *>(data), data_size);
		if(ret < 0)
		{
			std::cerr << "write data failed." << std::endl;
			return false;
		}
	}
	return true;
}

bool C2SPackageHeadAnalysis::C2SRead(int sockfd)
{
	int prev_n = _c2s_package_head._n;
	ssize_t ret = read(sockfd, static_cast<void *>(&_c2s_package_head),
		   	sizeof(_c2s_package_head));
	if(ret < 0)
	{
		std::cerr << "read 2SPackageHead failed." << std::endl;
		return false;
	}
	if(prev_n + 1 != _c2s_package_head._n)
	{
		std::cerr << "package loss : " << _c2s_package_head._n - prev_n << std::endl;
	}
	// new space.
	if(_c2s_package_head._data_len > _data_buffer_len)
	{
		_data_buffer_len = _c2s_package_head._data_len + 1024;
		char *tmp = new char[_data_buffer_len];
		memset(tmp,0x00,_data_buffer_len);
		if(_data_buffer != NULL)
		{
			delete []_data_buffer;
			_data_buffer = NULL;
		}
		_data_buffer = tmp;
	}
	// read data segment.
	ret = read(sockfd, static_cast<void *>(_data_buffer), _c2s_package_head._data_len);
	if(ret < 0)
	{
		std::cerr << "read data failed." << std::endl;
		return false;
	}
	if(ret != _c2s_package_head._data_len)
	{
		std::cerr << "Data loss. It shouldn't be happend." << std::endl;
	}
	return true;
}
