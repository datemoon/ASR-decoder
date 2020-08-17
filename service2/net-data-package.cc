
#include <string.h>
#include "service2/net-data-package.h"

void C2SPackageHeadPrint(C2SPackageHead &c2s, std::string flag)
{
	std::cout << "*************************************************" << std::endl;
	std::cout << flag << "->" << "_dtype\t: " << c2s._dtype  << std::endl;
	std::cout << flag << "->" << "_bit\t: " << c2s._bit << std::endl;
	std::cout << flag << "->" << "_sample_rate\t: " << c2s._sample_rate << std::endl;
	std::cout << flag << "->" << "_audio_type\t: " << c2s._audio_type << std::endl;
	std::cout << flag << "->" << "_audio_head_flage\t: " << c2s._audio_head_flage << std::endl;
	std::cout << flag << "->" << "_lattice\t: " << c2s._lattice << std::endl;
	std::cout << flag << "->" << "_ali_info\t: " << c2s._ali_info << std::endl;
	std::cout << flag << "->" << "_score_info\t: " << c2s._score_info << std::endl;
	std::cout << flag << "->" << "_nbest\t: " << c2s._nbest << std::endl;
	std::cout << flag << "->" << "_end_flag\t: " << c2s._end_flag << std::endl;
	std::cout << flag << "->" << "_keep\t: " << c2s._keep << std::endl;
	std::cout << flag << "->" << "_n\t: " << c2s._n << std::endl;
	std::cout << flag << "->" << "_data_len\t: " << c2s._data_len << std::endl;
	std::cout << "*************************************************" << std::endl;
}
bool C2SPackageAnalysis::C2SWrite(int sockfd, 
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
			sizeof(C2SPackageHead));
	if(ret < 0)
	{
		std::cerr << "write C2SPackageHead failed." << std::endl;
		return false;
	}
	if(data_size > 0)
	{
		ret = write(sockfd, data, data_size);
		if(ret < 0)
		{
			std::cerr << "write data failed." << std::endl;
			return false;
		}
	}
	return true;
}

bool C2SPackageAnalysis::C2SRead(int sockfd)
{
	uint prev_n = _c2s_package_head._n;
	ssize_t ret = read(sockfd, static_cast<void *>(&_c2s_package_head),
		   	sizeof(C2SPackageHead));
	if(ret < 0)
	{
		std::cerr << "read 2SPackageHead failed." << std::endl;
		return false;
	}
	if(prev_n + 1 != _c2s_package_head._n)
	{
		std::cerr << "Error: package loss : " << _c2s_package_head._n - prev_n << std::endl;
	}
	// new space.
	if(_c2s_package_head._data_len > _data_buffer_capacity)
	{
		_data_buffer_capacity = _c2s_package_head._data_len + 1024;
		_data_buffer = Renew<char>(_data_buffer, _c2s_package_head._data_len, _data_buffer_capacity);
		//char *tmp = new char[_data_buffer_capacity];
		//memset(tmp,0x00,_data_buffer_capacity * sizeof(char));
		//if(_data_buffer != NULL)
		//{
		//	delete []_data_buffer;
		//	_data_buffer = NULL;
		//}
		//_data_buffer = tmp;
	}
	if(_c2s_package_head._data_len > 0)
	{
		// have data and read data segment.
		ret = read(sockfd, static_cast<void *>(_data_buffer), _c2s_package_head._data_len);
		if(ret < 0)
		{
			std::cerr << "read data failed." << std::endl;
			return false;
		}
		if(ret != _c2s_package_head._data_len)
		{
			std::cerr << "Error: Data loss. It shouldn't be happend." << std::endl;
		}
	}
	return true;
}


/////////////////
bool S2CPackageAnalysis::S2CWrite(int sockfd, uint end_flag)
{
	_s2c_package_head._nbest = _nbest_res._nbest_len_len;
	_s2c_package_head._end_flag = end_flag;
	if(_s2c_package_head._end_flag != 0)
		_s2c_package_head._nres++;
	ssize_t ret = write(sockfd, static_cast<void *>(&_s2c_package_head), sizeof(S2CPackageHead));
	if(ret < 0)
	{
		std::cerr << "Write S2CPackageHead failed." << std::endl;
		return false;
	}
	ret = _nbest_res.Write(sockfd);
	if(ret < 0)
	{
		std::cerr <<"Write Nbest failed." << std::endl;
		return false;
	}
	return true;
}

bool S2CPackageAnalysis::S2CRead(int sockfd)
{
	ssize_t ret = read(sockfd, static_cast<void *>(&_s2c_package_head), sizeof(S2CPackageHead));
	if(ret < 0)
	{
		std::cerr << "Read S2CPackageHead failed." << std::endl;
		return false;
	}
	if(_s2c_package_head._nbest > 0)
	{
		ret = _nbest_res.Read(sockfd, _s2c_package_head._nbest);
		if(ret < 0)
		{
			std::cerr << "Read Nbest failed." << std::endl;
			return false;
		}
	}
	return true;
}
