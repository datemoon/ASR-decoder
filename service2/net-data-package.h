#ifndef __NET_DATA_PACKAGE_H__
#define __NET_DATA_PACKAGE_H__

#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

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
	uint _keep             : 1;
	unsigned int           : 0;
	// align to next unsigned int.

	uint _n                : 32;
	uint _data_len         : 32;
};
void C2SPackageHeadPrint(C2SPackageHead &c2s, std::string flag="");
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
			uint score_info=0, uint nbest = 0, uint end_flag=0, uint keep=0,
			uint n=0, uint data_len=0):
		_data_buffer(NULL),_data_buffer_capacity(0)
	{
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
	void Reset()
	{
		memset(&_c2s_package_head, 0x00, sizeof(_c2s_package_head));
	}
	inline bool SetDtype(uint dtype)
	{
		if(dtype >> 2 > 0)
			return false;
		_c2s_package_head._dtype = dtype;
		return true;
	}

	/*
	 * set which one package,start from 0-2^32
	 * */
	inline void SetN(uint n)
	{
		_c2s_package_head._n = n;
	}
	/*
	 * set data segment length.
	 * */
	inline void SetDataLen(uint data_len)
	{
		_c2s_package_head._data_len = data_len;
	}
	inline bool SetEndFLag(uint end_flag)
	{
		if(end_flag >> 1 > 0)
			return false;
		_c2s_package_head._end_flag = end_flag;
		return true;
	}
	inline bool SetBit(uint bit)
	{
		if(bit >> 4 > 0)
			return false;
		_c2s_package_head._bit = bit;
		return true;
	}
	inline bool SetSampleRate(uint sample_rate)
	{
		if(sample_rate >> 8 > 0)
			return false;
		_c2s_package_head._sample_rate = sample_rate;
		return true;
		
	}

	inline bool SetAudioType(uint audio_type)
	{
		if(audio_type >> 2 > 0)
			return false;
		_c2s_package_head._audio_type = audio_type;
		return true;
	}

	inline bool SetAudioHeadFlage(uint audio_head_flage)
	{
		if(audio_head_flage >> 1 > 0)
			return false;
		_c2s_package_head._audio_head_flage = audio_head_flage;
		return true;
	}

	/*
	 * set return nbest number.
	 * */
	inline bool setNbest(uint nbest = 1)
	{
		if(nbest >> 6 > 0)
			return false;
		_c2s_package_head._nbest = nbest;
		return true;
	}
	
	/*
	 * set whether return lattice,default don't return lattice.
	 * */
	inline bool SetLattice(uint lattice = 0)
	{
		if(lattice >> 1 > 0)
			return false;
		_c2s_package_head._lattice = lattice;
		return true;
	}
	/*
	 * set whether return align info,default don't return.
	 * */
	inline bool SetAliInfo(uint ali_info = 0)
	{
		if(ali_info >> 1 > 0)
			return false;
		_c2s_package_head._ali_info = ali_info;
		return true;
	}
	/*
	 * set whether score info,default don't return.
	 * */
	inline bool SetScoreInfo(uint score_info = 0)
	{
		if(score_info >> 1 > 0)
			return false;
		_c2s_package_head._score_info = score_info;
		return true;
	}

	inline bool SetKeep(uint keep = 0)
	{
		if(keep >> 1 > 0)
			return false;
		_c2s_package_head._keep = keep;
		return true;
	}
	// from client to service package write.
	/*
	 * sockfd    : scoket fd
	 * data      : data point
	 * data_size : data size
	 * n         : which one package
	 * end_flag  : end flag
	 * */
	bool C2SWrite(int sockfd, const void *data, size_t data_size,
			uint n=0, uint end_flag=0);
	// from client to service package unpack.
	bool C2SRead(int sockfd);
	void Print(std::string flag="")
	{
		C2SPackageHeadPrint(_c2s_package_head, flag);
	}

	bool IsEnd()
	{
		return _c2s_package_head._end_flag == 1 ? true:false;
	}

	char* GetData(uint *data_len)
	{
		*data_len = _c2s_package_head._data_len;
		return _data_buffer;
	}

private:
	struct C2SPackageHead _c2s_package_head;
	char *_data_buffer;
	uint _data_buffer_capacity;
};

// _end_flag: 0:not end, 1:vad end, 2:all end.
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
		memcpy(tmp, src, size*sizeof(T));
		delete [] src;
	}
	return tmp;
}
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
		ssize_t ret1 = write(sockfd, static_cast<void*>(_nbest_len), 
				_nbest_len_len*sizeof(uint));
		if(ret1 < 0)
		{
			std::cout << "Write nbest_len failed." <<std::endl;
			return ret1;
		}
		ssize_t ret2 = write(sockfd, static_cast<void*>(_nbest_res),
				_nbest_res_len*sizeof(char));
		if(ret2 < 0)
		{
			std::cout << "Write nbest_res failed." << std::endl;
			return ret2;
		}
		return ret1+ret2;
	}
	ssize_t Read(int sockfd, uint n)
	{
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
		ssize_t ret1 = read(sockfd, static_cast<void *>(_nbest_len),
			   	n*sizeof(uint));
		if(ret1 < 0)
		{
			std::cerr << "Read _nbest_len failed."  << std::endl;
			return ret1;
		}
		else if((uint)ret1 != n*sizeof(uint))
		{
			std::cerr << "Read _nbest_len loss." << std::endl;
			return ret1;
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
		ssize_t ret2 = read(sockfd, static_cast<void *>(_nbest_res),
				total_len*sizeof(char));
		if((uint)ret2 != total_len*sizeof(char))
		{
			std::cerr << "Read _nbest_res loss." << std::endl;
			return ret2;
		}
		return 0;
	}
	void Reset()
	{
		_nbest_res_len = 0;
		_nbest_len_len = 0;
	}
};
// from service to client pack and unpack.
class S2CPackageAnalysis
{
public:
	S2CPackageAnalysis(uint nbest=0,uint lattice=0,
			uint ali_info=0, uint score_info=0, uint end_flag=0, uint nres=0)
	{
		_s2c_package_head._nbest = nbest;
		_s2c_package_head._lattice = lattice;
		_s2c_package_head._ali_info = ali_info;
		_s2c_package_head._score_info = score_info;
		_s2c_package_head._end_flag = end_flag;
		_s2c_package_head._nres = nres;
	}

	void Reset()
	{
		_s2c_package_head._nbest = 0;
		_s2c_package_head._end_flag = 0;
		_s2c_package_head._nres = 0;
	}
	~S2CPackageAnalysis() { }
	/*
	 * Set nbest result.
	 * */
	void SetNbest(std::string &result)
	{
		_nbest_res.SetNbest(result);
	}
	// from service to client package write.
	bool S2CWrite(int sockfd, uint end_flag);
	// from service to client package unpack.
	bool S2CRead(int sockfd);

private:
	struct S2CPackageHead _s2c_package_head;
	// nbest
	S2CPackageNbest _nbest_res;
	// lattice
	// ali
	// score
};

#endif
