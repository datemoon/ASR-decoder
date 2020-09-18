#ifndef __NET_DATA_PACKAGE_H__
#define __NET_DATA_PACKAGE_H__

#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include "src/util/log-message.h"
#include "src/util/io-funcs.h"

#include "src/util/namespace-start.h"

//////////////////////////////////////////////////////////////
template <class T>
T* Renew(T *src, size_t old_size, size_t new_size)
{
	if(new_size == 0)
		return src;
	size_t size = new_size;
	T* tmp = new T[size];
	memset(tmp, 0x00, size*sizeof(T));
	if(src != NULL)
	{
		if(old_size != 0)
			memcpy(tmp, src, old_size*sizeof(T));
		delete [] src;
	}
	return tmp;
}
/////////////////////////////////////////////////////////////
typedef unsigned int uint;
// _dtype           : short,float        -> short:0, float:1
// _bit             : 16bit,8bit,32bit   -> 16bit:0, 8bit:1, 32bit:2
// _sample_rate     : 16k,8k,32k...      -> 16k:0, 8k:8, 32k:32
// _audio_type      : pcm,wav,opus       -> pcm:0, wav:1, opus:2
// _audio_head_flage: 0|1                -> 0:no audio head, 1:have
// _nbest           : 0-2^6              -> 0:0best,1:1best...
// _lattice         : 0|1                -> 0:no lattice reture, 1:have
// _ali_info        : 0|1                -> 0:no align info, 1:have
// _score_info      : 0|1                -> 0:no score info, 1:have
// _end_flag        : 0|1                -> 0:it isn't end, 1:end send.
// _have_key        : 0|1                -> 0:no key, 1:have key
// _keep            : 0|1                -> 0:no keep bit, 1:have
// _n               : 0...               -> which one package,start from 0.
// _data_len        : 0...               -> data segment length.
// sizeof(PackageHead) = 96;
// from client to service package head,
// Defaule: all common parameter it's zero.
struct C2SPackageHead
{
	uint _dtype            : 2;
	uint _bit              : 4;
	uint _sample_rate      : 8;
	uint _audio_type       : 2;
	uint _audio_head_flage : 1;
	uint _lattice          : 1;
	uint _ali_info         : 1;
	uint _score_info       : 1;
	uint _nbest            : 6;
	uint _end_flag         : 1;
	uint _have_key         : 1;
	uint _keep             : 1;
	unsigned int           : 0;
	// align to next unsigned int.

	uint _n                : 32;
	uint _data_len         : 32;
};

class C2SKey
{
private:
	char *_key_string;
	uint _key_len;
	uint _key_string_capacity;
public:

	std::string GetKey()
	{
		if(_key_string != NULL)
			return std::string(_key_string, _key_len);
		else
			return std::string("");
	}
	void Info(int n=2)
	{
		if(_key_len != 0)
			VLOG_COM(n) << "KEY : " << GetKey();
	}
	C2SKey():_key_string(NULL),_key_len(0),_key_string_capacity(0) { }
	~C2SKey()
	{
		if(_key_string != NULL)
			delete _key_string;
		_key_string = NULL;
		_key_len = 0;
		_key_string_capacity = 0;
	}
	bool Write(int sockfd, std::string &key_string)
	{
		uint key_len = (uint)key_string.length();
		int ret = WriteN(sockfd, static_cast<void *>(&key_len), sizeof(key_len));
		if(ret != sizeof(key_len))
		{
			LOG_WARN << "write key_len failed.";
            return false;
		}
		ret = WriteN(sockfd, (void *)(key_string.c_str()), sizeof(char)*key_len);
		if(ret != sizeof(char)*key_len)
		{
			LOG_WARN << "write key_string failed.";
			return false;
		}
		return true;
	}
	bool Read(int sockfd)
	{
		// read key length
		int ret = ReadN(sockfd, static_cast<void *>(&_key_len), sizeof(_key_len));
		if(ret != sizeof(_key_len))
		{
			LOG_WARN << "read C2SKey len error!!!";
			return false;
		}
		if(_key_len > _key_string_capacity)
		{ // Renew space
			_key_string = Renew(_key_string, _key_string_capacity, _key_len + 10);
			_key_string_capacity = _key_len + 10;
		}
		ret = ReadN(sockfd, static_cast<void *>(_key_string), sizeof(char)*_key_len);
		if(ret != sizeof(char)*_key_len)
		{
			LOG_WARN << "read C2SKey key_string error!!!";
			return false;
		}
		return true;
	}

};
void C2SPackageHeadPrint(C2SPackageHead &c2s, std::string flag="", int vlog=1);
// from client to service pack and unpack.
class C2SPackageAnalysis
{
public:
	enum DTYPE { DSHORT=0,DFLOAT=1 };
	enum AUDIOBIT { BIT_16=0,BIT_8=1,BIT_32=2 };

	enum SAMPLERATE { K_16=0,K_8=8,K_32=32 };
	enum AUDIOTYPE { PCM=0,WAV=1,OPUS=2 };
public:
	C2SPackageAnalysis(uint dtype=DSHORT, uint bit=BIT_16, uint sample_rate=K_16,
			uint audio_type=PCM, uint audio_head_flage = 0, uint lattice=0, uint ali_info=0,
			uint score_info=0, uint nbest = 0, uint end_flag=0,
			uint have_key = 0, uint keep=0,
			uint n=0, uint data_len=0):
		_data_buffer(NULL),_data_buffer_capacity(0)
	{
		memset(&_c2s_package_head, 0x00, sizeof(_c2s_package_head));
		_c2s_package_head._dtype = dtype;
		_c2s_package_head._bit = bit;
		_c2s_package_head._sample_rate = sample_rate;
		_c2s_package_head._audio_type = audio_type;
		_c2s_package_head._audio_head_flage = audio_head_flage;
		
		_c2s_package_head._lattice = lattice;
		_c2s_package_head._ali_info = ali_info;
		_c2s_package_head._score_info = score_info;
		_c2s_package_head._nbest = nbest;
		_c2s_package_head._end_flag = end_flag;
		_c2s_package_head._have_key = have_key;
		_c2s_package_head._keep = keep;
		_c2s_package_head._n = n;
		_c2s_package_head._data_len = data_len;
	}
	~C2SPackageAnalysis()  
	{
		if(_data_buffer != NULL)
		{
			delete []_data_buffer;
			_data_buffer = NULL;
		}
		_data_buffer_capacity = 0;
	}
public:
	void Reset(bool eos=false)
	{
		if(_c2s_package_head._end_flag == 1 || eos == true )
		{
			_c2s_package_head._n = 0;
		}
		_c2s_package_head._lattice = 0;
		_c2s_package_head._ali_info = 0;
		_c2s_package_head._score_info = 0;
		_c2s_package_head._nbest = 0;
		_c2s_package_head._end_flag = 0;
		_c2s_package_head._have_key = 0;
		_c2s_package_head._keep = 0;
	}

	// set audil data type ,(short,float...)
	inline bool SetDtype(uint dtype)
	{
		if(dtype >> 2 > 0)
			return false;
		_c2s_package_head._dtype = dtype;
		return true;
	}
	inline uint GetDtype() { return _c2s_package_head._dtype;}
	// get data type len
	inline uint GetDtypeLen()
	{
		if(_c2s_package_head._dtype == DSHORT)
			return 2;
		else if(_c2s_package_head._dtype == DFLOAT)
			return 4;
		else
			return -1;
	}

	// set which one package,start from 0-2^32
	inline void SetN(uint n)
	{
		_c2s_package_head._n = n;
	}

	inline uint GetN() { return _c2s_package_head._n;}
	// set data segment length.
	inline void SetDataLen(uint data_len)
	{
		_c2s_package_head._data_len = data_len;
	}

	// set end flag
	inline bool SetEndFLag(uint end_flag)
	{
		if(end_flag >> 1 > 0)
			return false;
		_c2s_package_head._end_flag = end_flag;
		return true;
	}
	// whether send to service end .
	bool IsEnd()
	{
		return _c2s_package_head._end_flag == 1 ? true:false;
	}
	
	// set audio bit, 8bit 16bit,32bit
	inline bool SetBit(uint bit)
	{
		if(bit >> 4 > 0)
			return false;
		_c2s_package_head._bit = bit;
		return true;
	}
	inline uint GetBit() { return _c2s_package_head._bit;}
	
	inline bool SetSampleRate(uint sample_rate)
	{
		if(sample_rate >> 8 > 0)
			return false;
		_c2s_package_head._sample_rate = sample_rate;
		return true;
	}
	inline uint GetSampleRate() { return _c2s_package_head._sample_rate;}

	// audio type set (pcm,wav,opus)
	inline bool SetAudioType(uint audio_type)
	{
		if(audio_type >> 2 > 0)
			return false;
		_c2s_package_head._audio_type = audio_type;
		return true;
	}
	inline uint GetAudioType() { return _c2s_package_head._audio_type;}
	
	// whether have audio head. Default no.
	inline bool SetAudioHeadFlag(uint audio_head_flage)
	{
		if(audio_head_flage >> 1 > 0)
			return false;
		_c2s_package_head._audio_head_flage = audio_head_flage;
		return true;
	}
	inline uint GetAudioHeadFlag() { return _c2s_package_head._audio_head_flage; }

	// set return nbest number. maximux it's 31->(2^6-1).
	inline bool SetNbest(uint nbest = 1)
	{
		if(nbest >> 6 > 0)
			return false;
		_c2s_package_head._nbest = nbest;
		return true;
	}
	inline uint GetNbest() { return _c2s_package_head._nbest;}
	
	// set whether return lattice,default don't return lattice.
	inline bool SetLattice(uint lattice = 0)
	{
		if(lattice >> 1 > 0)
			return false;
		_c2s_package_head._lattice = lattice;
		return true;
	}
	inline uint GetLattice() { return _c2s_package_head._lattice; }
	
	//set whether return align info,default don't return.
	inline bool SetAliInfo(uint ali_info = 0)
	{
		if(ali_info >> 1 > 0)
			return false;
		_c2s_package_head._ali_info = ali_info;
		return true;
	}
	inline uint GetSetAliInfo() { return _c2s_package_head._ali_info; }

	// set whether score info,default don't return.
	inline bool SetScoreInfo(uint score_info = 0)
	{
		if(score_info >> 1 > 0)
			return false;
		_c2s_package_head._score_info = score_info;
		return true;
	}
	inline uint GetScoreInfo() { return _c2s_package_head._score_info; }

	// set keep bit.
	inline bool SetKeep(uint keep = 0)
	{
		if(keep >> 1 > 0)
			return false;
		_c2s_package_head._keep = keep;
		return true;
	}

	// for write
	inline bool SetHaveKey(uint have_key=0)
	{
		if(have_key >> 1 > 0)
			return false;
		_c2s_package_head._have_key = have_key;
		return true;
	}

	// for read
	inline bool IsHaveKey()
	{
		if(_c2s_package_head._have_key != 0)
			return true;
		else
			return false;
	}
	//////////////////////////////////////////////////////////
	// from client to service package write.
	/*
	 * sockfd    : scoket fd
	 * data      : data point
	 * data_size : data size
	 * n         : which one package
	 * end_flag  : end flag
	 * */
	bool C2SWrite(int sockfd, const void *data, size_t data_size,
			uint end_flag=0,
			std::string key_str = "");

	// from client to service package unpack.
	bool C2SRead(int sockfd);
	void Print(std::string flag="", int vlog=2)
	{
		C2SPackageHeadPrint(_c2s_package_head, flag, vlog);
		_c2skey.Info(vlog);
	}

	char* GetData(uint *data_len)
	{
		*data_len = _c2s_package_head._data_len;
		return _data_buffer;
	}

	std::string GetKey()
	{
		return _c2skey.GetKey();
	}
private:
	struct C2SPackageHead _c2s_package_head;
	char *_data_buffer;
	uint _data_buffer_capacity;
	C2SKey _c2skey;
};

//////////////////////////////////////////////////////////////
class S2CPackageNbest
{
public:
	friend class S2CPackageAnalysis;
private:
	char *_nbest_res;         // nbest result.
	uint _nbest_res_len;      // _nbest_res length.
	uint _nbest_res_capacity; // _nbest_res capacity.
	uint *_nbest_len;         // one result length.
	uint _nbest_len_len;      // how many reuslt.
	uint _nbest_len_capacity; // _nbest_len capacity.
public:
	void Print(std::string flag="")
	{
		std::vector<std::string> nbest;
		GetData(&nbest);
		for(size_t i=0; i < nbest.size(); ++i)
		{
			if(nbest[i].length() == 0)
				continue;
			LOG_COM << flag << " -> " << nbest[i];
		}
	}

	S2CPackageNbest(uint nbest_res_capacity=1024,uint nbest_len_capacity=5):
		_nbest_res(NULL), _nbest_res_len(0), 
		_nbest_res_capacity(nbest_res_capacity),
		_nbest_len(NULL), _nbest_len_len(0),
		_nbest_len_capacity(nbest_len_capacity)
	{
		_nbest_res = new char[_nbest_res_capacity];
		memset(_nbest_res, 0x00, _nbest_res_capacity*sizeof(char));
		_nbest_res_len = 0;

		_nbest_len = new uint[_nbest_len_capacity];
		memset(_nbest_len, 0x00, _nbest_len_capacity*sizeof(uint));
		_nbest_len_len = 0;
	}
	~S2CPackageNbest() 
	{
		if(_nbest_res != NULL)
		{
			delete [] _nbest_res;
			_nbest_res = NULL;
		}
		_nbest_res_len = 0;
		_nbest_res_capacity = 0;
		if(_nbest_len != NULL)
		{
			delete []_nbest_len;
			_nbest_len = NULL;
		}
		_nbest_len_len = 0;
		_nbest_len_capacity = 0;
	}

	void GetData(std::vector<std::string> *nbest)
	{
		uint offset = 0;
		for(uint i=0;i<_nbest_len_len;++i)
		{
			std::string res(_nbest_res+offset, _nbest_len[i]);
			nbest->push_back(res);
			offset += _nbest_len[i];
		}
	}
	void SetNbest(std::string &result)
	{
		if(result.size() + _nbest_res_len > _nbest_res_capacity)
		{ // realloc
			_nbest_res_capacity = result.size() + _nbest_res_len + 128;
			_nbest_res = Renew<char>(_nbest_res, _nbest_res_len, _nbest_res_capacity);
		}
		// copy result
		memcpy(_nbest_res+_nbest_res_len, result.c_str(), result.size());
		_nbest_res_len += result.size();

		// set _nbest_len
		if(_nbest_len_len + 1 > _nbest_len_capacity)
		{
			_nbest_len_capacity += 5;
			_nbest_len = Renew<uint>(_nbest_len, _nbest_len_len, _nbest_len_capacity);
		}
		// add len
		_nbest_len[_nbest_len_len] = result.size();
		_nbest_len_len++;

	}
	ssize_t Write(int sockfd)
	{
		if(_nbest_len_len == 0)
			return 0;
		ssize_t ret1 = WriteN(sockfd, static_cast<void*>(_nbest_len), 
				_nbest_len_len*sizeof(uint));
		if(ret1 < 0)
		{
			LOG_WARN << "Write nbest_len failed." <<std::endl;
			return ret1;
		}
		ssize_t ret2 = WriteN(sockfd, static_cast<void*>(_nbest_res),
				_nbest_res_len*sizeof(char));
		if(ret2 < 0)
		{
			LOG_WARN << "Write nbest_res failed.";
			return ret2;
		}
		return ret1+ret2;
	}
	ssize_t Read(int sockfd, uint n)
	{
		_nbest_len_len = n;
		if (n <= 0)
		{
			return -1;
		}
		if(n > _nbest_len_capacity)
		{ // realloc _nbest_len
			_nbest_len_capacity = n+5;
			_nbest_len = Renew<uint>(_nbest_len, 0, 
					_nbest_len_capacity);
		}
		ssize_t ret1 = ReadN(sockfd, static_cast<void *>(_nbest_len),
			   	n*sizeof(uint));
		if(ret1 < 0)
		{
			LOG_WARN << "Read _nbest_len failed." ;
			return ret1;
		}
		else if((uint)ret1 != n*sizeof(uint))
		{
			LOG_WARN << "Read _nbest_len loss.";
			return -1;
		}
		// calculate total length
		size_t total_len = 0;
		for(uint i=0;i<n;++i)
		{
			total_len += _nbest_len[i]*sizeof(char);
		}
		// realloc _nbest_res
		if(total_len > _nbest_res_capacity)
		{
			_nbest_res_capacity = total_len +128;
			_nbest_res = Renew<char>(_nbest_res, 0, _nbest_res_capacity);
		}
		ssize_t ret2 = ReadN(sockfd, static_cast<void *>(_nbest_res),
				total_len*sizeof(char));
		if((uint)ret2 != total_len*sizeof(char))
		{
			LOG_WARN << "Read _nbest_res loss.";
			return -1;
		}
		return 0;
	}
	void Reset()
	{
		_nbest_res_len = 0;
		_nbest_len_len = 0;
	}
};
// _nbest           : 0,1...      -> nbest
// _lattice         : 0|1
// _ali_info        : 0|1
// _score_info      : 0|1
// _end_flag        : 0,1,2       -> 0:not end, 1:vad end, 2:all end.
struct S2CPackageHead
{
	uint _nbest        :6;
	uint _lattice      :1;
	uint _ali_info     :1;
	uint _score_info   :1;
	uint _end_flag     :2;     
	uint _nres         :16;
	unsigned int       :0;
};

void S2CPackageHeadPrint(S2CPackageHead &s2c, std::string flag="", int vlog=1);

// from service to client pack and unpack.
class S2CPackageAnalysis
{
public:
	enum S2CENDFLAG {S2CNOEND=0,S2CMIDDLEEND=1,S2CEND=2};
public:
	S2CPackageAnalysis(uint nbest=0,uint lattice=0,
			uint ali_info=0, uint score_info=0, uint end_flag=0, uint nres=0)
	{
		memset(&_s2c_package_head, 0x00, sizeof(_s2c_package_head));
		_s2c_package_head._nbest = nbest;
		_s2c_package_head._lattice = lattice;
		_s2c_package_head._ali_info = ali_info;
		_s2c_package_head._score_info = score_info;
		_s2c_package_head._end_flag = end_flag;
		_s2c_package_head._nres = nres;
	}

	bool SetUseLattice(uint lattice) { if(lattice >> 1 > 0) return false; _s2c_package_head._lattice = lattice; return true;}
	bool SetUseAligninfo(uint ali_info) { if(ali_info >> 1 > 0) return false; _s2c_package_head._ali_info = ali_info;return true;}
	bool SetUseScoreinfo(uint score_info) { if(score_info >> 1 > 0)return false; _s2c_package_head._score_info = score_info; return true;}
	bool IsAllEnd()
	{
		if(_s2c_package_head._end_flag == S2CEND)
			return true;
		return false;
	}

	bool IsMiddleEnd()
	{
		if(_s2c_package_head._end_flag == S2CMIDDLEEND)
			return true;
		return false;
	}

	void Reset()
	{
		if(IsAllEnd())
			_s2c_package_head._nres = 0;
		_s2c_package_head._nbest = 0;
		_s2c_package_head._end_flag = S2CNOEND;
		_nbest_res.Reset();
	}
	~S2CPackageAnalysis() { }
	// Set nbest result.
	// if you result greater than 1, you should be recursive call n times.
	void SetNbest(std::string &result)
	{
		_nbest_res.SetNbest(result);
		_s2c_package_head._nbest++;
	}
	// from service to client package write.
	bool S2CWrite(int sockfd, uint end_flag);
	// from service to client package unpack.
	bool S2CRead(int sockfd);

	void GetData(std::vector<std::string> *nbest)
	{
		_nbest_res.GetData(nbest);
	}

	void Print(std::string flag="")
	{
		S2CPackageHeadPrint(_s2c_package_head, flag);
		_nbest_res.Print(flag);
	}

	int GetNres()
	{
		return _s2c_package_head._nres;
	}

private:
	struct S2CPackageHead _s2c_package_head;
	// nbest
	S2CPackageNbest _nbest_res;
	// lattice
	// ali
	// score
};

#include "src/util/namespace-end.h"

#endif
