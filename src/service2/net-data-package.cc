
#include <string.h>
#include "src/service2/net-data-package.h"

#include "src/util/namespace-start.h"

void C2SPackageHeadPrint(C2SPackageHead &c2s, std::string flag, int vlog)
{
	VLOG_COM(vlog) << "*************************************************";
	VLOG_COM(vlog) << flag << "->" << "_dtype\t: " << c2s._dtype;
	VLOG_COM(vlog) << flag << "->" << "_bit\t: " << c2s._bit;
	VLOG_COM(vlog) << flag << "->" << "_sample_rate\t: " << c2s._sample_rate;
	VLOG_COM(vlog) << flag << "->" << "_audio_type\t: " << c2s._audio_type;
	VLOG_COM(vlog) << flag << "->" << "_audio_head_flage\t: " << c2s._audio_head_flage ;
	VLOG_COM(vlog) << flag << "->" << "_lattice\t: " << c2s._lattice;
	VLOG_COM(vlog) << flag << "->" << "_ali_info\t: " << c2s._ali_info;
	VLOG_COM(vlog) << flag << "->" << "_score_info\t: " << c2s._score_info;
	VLOG_COM(vlog) << flag << "->" << "_nbest\t: " << c2s._nbest;
	VLOG_COM(vlog) << flag << "->" << "_end_flag\t: " << c2s._end_flag;
	VLOG_COM(vlog) << flag << "->" << "_have_key\t: " << c2s._have_key;
	VLOG_COM(vlog) << flag << "->" << "_keep\t: " << c2s._keep;
	VLOG_COM(vlog) << flag << "->" << "_n\t: " << c2s._n;
	VLOG_COM(vlog) << flag << "->" << "_data_len\t: " << c2s._data_len;
	VLOG_COM(vlog) << "*************************************************";
	if(true)
	{
	}
}

void S2CPackageHeadPrint(S2CPackageHead &s2c, std::string flag, int vlog)
{
	VLOG_COM(vlog) << "*************************************************";
	VLOG_COM(vlog) << flag << "->" << "_nbest\t: " << s2c._nbest;
	VLOG_COM(vlog) << flag << "->" << "_lattice\t: " << s2c._lattice;
	VLOG_COM(vlog) << flag << "->" << "_ali_info\t: " << s2c._ali_info;
	VLOG_COM(vlog) << flag << "->" << "_score_info\t: " << s2c._score_info;
	VLOG_COM(vlog) << flag << "->" << "_end_flag\t: " << s2c._end_flag;
	VLOG_COM(vlog) << flag << "->" << "_nres\t: " << s2c._nres;
	VLOG_COM(vlog) << "*************************************************";
}

bool C2SPackageAnalysis::C2SWrite(int sockfd, 
		const void *data, size_t data_size, 
		uint end_flag, std::string key_str)
{
	if(end_flag == 0 && data_size == 0 && key_str.length() == 0)
	{
		LOG_COM <<  "send null package.";
		return true;
	}
	if(SetEndFLag(end_flag) != true)
	{
		LOG_WARN << "SetEndFLag error.";
		return false;
	}
	if(key_str.length() != 0)
	{
		SetHaveKey(1);
	}
	else
	{
		SetHaveKey(0);
	}
	VLOG_COM(3) << "key : " << key_str << " -> package : " << GetN();
	//SetN(n);
	_c2s_package_head._n++;
	SetDataLen(data_size);
	// write package head
	ssize_t ret = WriteN(sockfd, static_cast<void *>(&_c2s_package_head),
			sizeof(C2SPackageHead));
	if(ret != sizeof(C2SPackageHead))
	{
		LOG_WARN << "write C2SPackageHead failed.";
		return false;
	}
	C2SPackageHeadPrint(_c2s_package_head, "c2s-write");
	// write key
	if(key_str.length() != 0)
	{
		if(_c2skey.Write(sockfd, key_str) != true)
		{
			LOG_WARN << "write C2SKey error!!!";
			return false;
		}
	}
	// write data
	if(data_size > 0)
	{
		ret = WriteN(sockfd, data, data_size);
		if(ret != data_size)
		{
			LOG_WARN << "write data failed.";
			return false;
		}
	}
	return true;
}

bool C2SPackageAnalysis::C2SRead(int sockfd)
{
	uint prev_n = _c2s_package_head._n;
	ssize_t ret = ReadN(sockfd, static_cast<void *>(&_c2s_package_head),
		   	sizeof(C2SPackageHead));
	C2SPackageHeadPrint(_c2s_package_head,"c2s-read");
	if(ret != sizeof(C2SPackageHead))
	{
		LOG_WARN << "read 2SPackageHead failed.";
		return false;
	}
	if(prev_n + 1 != _c2s_package_head._n)
	{
		Print("c2s-read-error", 0);
		LOG_WARN << "Error: package loss : " << prev_n  << " + 1 != " << _c2s_package_head._n;
		return false;
	}
	// new space.
	if(_c2s_package_head._data_len > _data_buffer_capacity)
	{
		uint new_length = _c2s_package_head._data_len + 1024;
		_data_buffer = Renew<char>(_data_buffer, _data_buffer_capacity, new_length);
		_data_buffer_capacity = new_length;
		//char *tmp = new char[_data_buffer_capacity];
		//memset(tmp,0x00,_data_buffer_capacity * sizeof(char));
		//if(_data_buffer != NULL)
		//{
		//	delete []_data_buffer;
		//	_data_buffer = NULL;
		//}
		//_data_buffer = tmp;
	}
	// judge key_sting
	if(IsHaveKey() == true)
	{
		if(_c2skey.Read(sockfd) != true)
		{
			LOG_COM << "package : " << _c2s_package_head._n << " C2SKey read error!!!";
			return false;
		}
		_c2skey.Info();
	}
	if(_c2s_package_head._data_len > 0)
	{
		// have data and read data segment.
		ret = ReadN(sockfd, static_cast<void *>(_data_buffer), _c2s_package_head._data_len);
		if(ret < 0)
		{
			LOG_WARN << "read data failed.";
			return false;
		}
		if(ret != _c2s_package_head._data_len)
		{
			LOG_WARN << "Error: Data loss. It shouldn't be happend.";
		}
	}
	return true;
}


/////////////////
bool S2CPackageAnalysis::S2CWrite(int sockfd, uint end_flag)
{
	_s2c_package_head._nbest = _nbest_res._nbest_len_len;
	_s2c_package_head._end_flag = end_flag;
	if(_s2c_package_head._end_flag != S2CNOEND)
		_s2c_package_head._nres++;
	if(_s2c_package_head._nbest == 0 && _s2c_package_head._end_flag != S2CEND)
	{
		VLOG_COM(1) << "No result and not end, so no send package to client.";
		return true;
	}
	ssize_t ret = write(sockfd, static_cast<void *>(&_s2c_package_head), sizeof(S2CPackageHead));
	if(ret < 0)
	{
		LOG_WARN << "Write S2CPackageHead failed.";
		return false;
	}
	ret = _nbest_res.Write(sockfd);
	if(ret < 0)
	{
		LOG_WARN <<"Write Nbest failed.";
		return false;
	}
	return true;
}

bool S2CPackageAnalysis::S2CRead(int sockfd)
{
	ssize_t ret = ReadN(sockfd, static_cast<void *>(&_s2c_package_head), sizeof(S2CPackageHead));
	if(ret != sizeof(S2CPackageHead))
	{
		LOG_WARN << "Read S2CPackageHead failed.";
		return false;
	}
	if(_s2c_package_head._nbest > 0)
	{
		ret = _nbest_res.Read(sockfd, _s2c_package_head._nbest);
		if(ret != 0)
		{
			LOG_WARN << "Read Nbest failed.";
			return false;
		}
	}
	return true;
}

#include "src/util/namespace-end.h"
