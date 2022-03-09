// author: hubo
// time  : 2020/08/19
#ifndef __NET_DATA_PACKAGE_H__
#define __NET_DATA_PACKAGE_H__

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "src/util/log-message.h"
#include "src/util/io-funcs.h"

#include "src/util/namespace-start.h"

template<typename T>
class NetPackageBase
{
public:
	virtual ssize_t Read(int sockfd) = 0;
	virtual ssize_t Write(int sockfd) = 0;
	virtual void Reset() = 0;
	virtual std::ostream &String(std::ostream &strm) = 0;
	virtual void SetData(T &data) = 0;
	virtual void SetData(const T &data) = 0;
	virtual const T &GetData() = 0;

	virtual ~NetPackageBase() { }
private:
};

//////////////////////////////////////////////////////////////
template<typename T=std::vector<std::string> >
class NetPackageIO:NetPackageBase<T>
{
public:
	NetPackageIO(std::string prefix=""):_prefix(prefix) { }

	~NetPackageIO() { }

	std::ostream &String(std::ostream &strm)
	{
		strm << _prefix << " ";
		internal::PrintContainer(strm, _data);
		return strm;
	}

	// parameter:
	//    sockfd: socket fd
	// return:
	//    <= 0 : send error.
	//    > 0  : send ok.
	ssize_t Write(int sockfd)
	{
		std::ostringstream oss;
		std::ostream &otrm = WriteType(oss, _data);
		if(!otrm)
		{
			LOG_WARN << "format " << _prefix << " failed!!!";
			return -1;
		}
		ssize_t nbest_len = oss.str().size();

		ssize_t ret1 = WriteN(sockfd, static_cast<void*>(&nbest_len),
				sizeof(nbest_len));
		if(ret1 <= 0)
		{
			LOG_WARN << "write " << _prefix << " length failed!!!";
			return ret1;
		}
		else if(ret1 != sizeof(nbest_len))
		{
			if(ret1 == 0)
			{
				LOG_WARN << "Socked have been closed " << sockfd << "!!!";
			}
			else
			{
				LOG_WARN << "Write error " << sockfd << "!!!";
			}
			return -1;
		}
		if(nbest_len > 0)
		{
			ssize_t ret2 = WriteN(sockfd, static_cast<void*>(&oss.str()[0]),
					nbest_len);
			if(ret2 < 0)
			{
				LOG_WARN << "write " << _prefix << " data failed!!!";
				return ret2;
			}
			else if(ret2 != nbest_len)
			{
				if(ret2 == 0)
				{
					LOG_WARN << "Socked have been closed " << sockfd << "!!!";
				}
				else
				{
					LOG_WARN << "Write error " << sockfd << "!!!";
				}
				return -1;
			}
			return ret1 + ret2;
		}
		else
			return ret1;
	} // Write

	// parameter:
	//    sockfd: socket fd
	// return:
	//    <= 0 : receive error.
	//    > 0  : receive ok.
	ssize_t Read(int sockfd)
	{
		Reset();
		ssize_t nbest_len=0;
		ssize_t ret1 = ReadN(sockfd, static_cast<void *>(&nbest_len),
				sizeof(nbest_len));
		if(ret1 <= 0)
		{
			LOG_WARN << "Read " << _prefix << " length failed!!!" ;
			return ret1;
		}
		else if(ret1 != sizeof(nbest_len))
		{
			if(ret1 == 0)
			{
				LOG_WARN << "Socked have been closed " << sockfd << "!!!";
			}
			else
			{
				LOG_WARN << "Write error " << sockfd << "!!!";
			}
			return -1;
		}
		if(nbest_len > 0)
		{
			std::string data_str(nbest_len, '0');
			ssize_t ret2 = ReadN(sockfd, static_cast<void *>(&data_str[0]),
					data_str.size());
			if(ret2 < 0)
			{
				LOG_WARN << "Read " << _prefix << " data failed!!!";
				return ret2;
			}
			else if(ret2 != data_str.size())
			{
				if(ret2 == 0)
				{
					LOG_WARN << "Socked have been closed " << sockfd << "!!!";
				}
				else
				{
					LOG_WARN << "Write error " << sockfd << "!!!";
				}
				return -1;
			}
			// read vector<std::string >
			std::istringstream iss(data_str);
			std::istream &istrm = ReadType(iss, &_data);
			if(!istrm)
			{
				LOG_WARN << "load " << _prefix << " data error!!!";
				return -1;
			}
			return ret1 + ret2;
		}
		else
		{
			return ret1;
		}
	} // Read

	void Reset()
	{
		_data.clear();
	}

	void SetData(T &data)
	{
		_data.swap(data);
	}

	void SetData(const T &data)
	{
		_data = data;
	}

	const T &GetData()
	{
		return _data;
	}

private:
	std::string _prefix;
	T _data;
};

template <typename T>
//template <class T, typename std::enable_if<std::is_class<T>::value, T>::type* = nullptr>
std::ostream &operator<<(std::ostream &strm, NetPackageIO<T> &c)
{
	return internal::PrintContainer(strm, c.GetData());
}

typedef NetPackageIO<std::vector<std::string> > PackageNbest;

typedef std::vector<std::pair<std::string, std::pair<float, float> > > AlignTime;

typedef NetPackageIO<AlignTime > PackageAlign;

typedef NetPackageIO<std::string> C2SPackageKey;

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
// _n               : 1...               -> which one package,start from 1. Init is 0
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
	
	void Print(std::string flag="c2s", int vlog=0)
	{
		VLOG_COM(vlog) << "*************************************************";
		VLOG_COM(vlog) << flag << "->" << "_dtype\t: " << _dtype;
		VLOG_COM(vlog) << flag << "->" << "_bit\t: " << _bit;
		VLOG_COM(vlog) << flag << "->" << "_sample_rate\t: " << _sample_rate;
		VLOG_COM(vlog) << flag << "->" << "_audio_type\t: " << _audio_type;
		VLOG_COM(vlog) << flag << "->" << "_audio_head_flage\t: " << _audio_head_flage ;
		VLOG_COM(vlog) << flag << "->" << "_lattice\t: " << _lattice;
		VLOG_COM(vlog) << flag << "->" << "_ali_info\t: " << _ali_info;
		VLOG_COM(vlog) << flag << "->" << "_score_info\t: " << _score_info;
		VLOG_COM(vlog) << flag << "->" << "_nbest\t: " << _nbest;
		VLOG_COM(vlog) << flag << "->" << "_end_flag\t: " << _end_flag;
		VLOG_COM(vlog) << flag << "->" << "_have_key\t: " << _have_key;
		VLOG_COM(vlog) << flag << "->" << "_keep\t: " << _keep;
		VLOG_COM(vlog) << flag << "->" << "_n\t: " << _n;
		VLOG_COM(vlog) << flag << "->" << "_data_len\t: " << _data_len;
		VLOG_COM(vlog) << "*************************************************";
		if(true)
		{
		}
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
	bool IsStart()
	{
		return _c2s_package_head._n == 1 ? true:false;
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
		VLOG_COM(vlog) << "KEY : " << _c2skey;
	}

	char* GetData(uint *data_len)
	{
		*data_len = _c2s_package_head._data_len;
		return _data_buffer;
	}

	const std::string &GetKey()
	{
		return _c2skey.GetData();
	}
private:
	struct C2SPackageHead _c2s_package_head;
	char *_data_buffer;
	uint _data_buffer_capacity;
	//C2SKey _c2skey;
	C2SPackageKey _c2skey;
};

enum S2CENDFLAG {S2CNOEND=0,S2CMIDDLEEND=1,S2CEND=2};

// _nbest           : 0|1         -> 0:no nbest, 1: have nbest
// _lattice         : 0|1         -> 0:no lattice, 1:have lattice
// _ali_info        : 0|1
// _score_info      : 0|1
// _end_flag        : 0,1,2       -> 0:not end, 1:vad end, 2:all end.
// _do_rescore      : 0|1         -> 0:not lattice rescore, 1: lattice rescore
// _do_punctuate    : 0|1         -> 0:not add punctuate 1:add punctuate
// _nres            : 0,1...      -> the n times full result.
struct S2CPackageHead
{
	uint _nbest        :1;
	uint _lattice      :1;
	uint _ali_info     :1;
	uint _score_info   :1;
	uint _end_flag     :2;
	uint _do_rescore   :1;
	uint _do_punctuate :1;
	uint _nres         :16;
	unsigned int       :0;

	S2CPackageHead():
		_nbest(0), _lattice(0), _ali_info(0), _score_info(0),
		_end_flag(S2CNOEND), _do_rescore(0), _do_punctuate(1), _nres(0) { }

	void Reset()
	{
		if(IsAllEnd() == true)
			_nres = 0;
		_nbest = 0;
		_lattice = 0;
		_ali_info = 0;
		_score_info = 0;
		_end_flag = S2CNOEND;
	}
	bool SetUseNbest(uint nbest)
	{
		if(nbest >> 1 > 0)
			return false;
		_nbest = nbest;
		return true;
	}
	bool HaveNbest() { return _nbest; }
	bool SetUseLattice(uint lattice) 
	{ 
		if(lattice >> 1 > 0) 
			return false;
	   	_lattice = lattice;
	   	return true;
	}
	bool HaveLattice() { return _lattice; }
	bool SetUseAligninfo(uint ali_info) 
	{ 
		if(ali_info >> 1 > 0) 
			return false; 
		_ali_info = ali_info;
		return true;
	}
	bool HaveAlign() { return _ali_info; }
	bool SetUseScoreinfo(uint score_info) 
	{ 
		if(score_info >> 1 > 0)
			return false; 
		_score_info = score_info; 
		return true;
	}
	bool HaveScoreInfo() { return _score_info; }
	bool SetDoRescore(uint do_rescore)
	{
		if(do_rescore >> 1 >0)
			return false;
		_do_rescore = do_rescore;
		return true;
	}
	bool SetDoPunctuate(uint do_punctuate)
	{
		if(do_punctuate >> 1 > 0)
			return false;
		_do_punctuate = do_punctuate;
		return true;
	}
	bool IsAllEnd()
	{
		if(_end_flag == S2CEND)
			return true;
		return false;
	}

	bool IsMiddleEnd()
	{
		if(_end_flag == S2CMIDDLEEND)
			return true;
		return false;
	}
	void Print(std::string flag="s2c", int vlog=0)
	{
		VLOG_COM(vlog) << "*************************************************";
		VLOG_COM(vlog) << flag << "->" << "_nbest\t: " << _nbest;
		VLOG_COM(vlog) << flag << "->" << "_lattice\t: " << _lattice;
		VLOG_COM(vlog) << flag << "->" << "_ali_info\t: " << _ali_info;
		VLOG_COM(vlog) << flag << "->" << "_score_info\t: " << _score_info;
		VLOG_COM(vlog) << flag << "->" << "_end_flag\t: " << _end_flag;
		VLOG_COM(vlog) << flag << "->" << "_do_rescore\t: " << _do_rescore;
		VLOG_COM(vlog) << flag << "->" << "_do_punctuate\t: " << _do_punctuate;
		VLOG_COM(vlog) << flag << "->" << "_nres\t: " << _nres;
		VLOG_COM(vlog) << "*************************************************";
	}
};

void S2CPackageHeadPrint(S2CPackageHead &s2c, std::string flag="", int vlog=1);

// from service to client pack and unpack.
class S2CPackageAnalysis
{
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

	bool SetUseLattice(uint lattice) { return _s2c_package_head.SetUseLattice(lattice); }
	
	bool SetUseAligninfo(uint ali_info) { return _s2c_package_head.SetUseAligninfo(ali_info); }
	
	bool SetUseScoreinfo(uint score_info) { return _s2c_package_head.SetUseScoreinfo(score_info); }
	
	bool IsAllEnd() { return _s2c_package_head.IsAllEnd(); }

	bool IsMiddleEnd() { return _s2c_package_head.IsMiddleEnd(); }

	void Reset()
	{
		_s2c_package_head.Reset();
		_nbest_res.Reset();
		_align_time.Reset();
	}
	~S2CPackageAnalysis() { }
	// Set nbest result.
	// if you result greater than 1, you should be recursive call n times.
	void SetNbest(const std::string &result)
	{
		std::vector<std::string> nbest;
		nbest.push_back(result);
		_nbest_res.SetData(nbest);
		_s2c_package_head.SetUseNbest(1);
	}
	void SetNbest(std::vector<std::string> &nbest_result)
	{
		_nbest_res.SetData(nbest_result);
		_s2c_package_head.SetUseNbest(1);
	}

	void SetAlign(AlignTime &align_time)
	{
		_align_time.SetData(align_time);
		SetUseAligninfo(1);
	}

	// from service to client package write.
	bool S2CWrite(int sockfd, uint end_flag);
	// from service to client package unpack.
	bool S2CRead(int sockfd);

	// get nbest
	void GetData(std::vector<std::string> *nbest)
	{
		*nbest = _nbest_res.GetData();
	}

	// get align result
	const AlignTime &GetAlign()
	{
		return _align_time.GetData();
	}

	void Print(std::string flag="", int vlog=0)
	{
		S2CPackageHeadPrint(_s2c_package_head, flag, vlog);
		//_nbest_res.Print(flag);
		VLOG_COM(vlog) << flag << " " << _nbest_res;
		VLOG_COM(vlog) << flag << " " << _align_time;
	}

	int GetNres()
	{
		return _s2c_package_head._nres;
	}

private:
	struct S2CPackageHead _s2c_package_head;
	// nbest
	//S2CPackageNbest _nbest_res;
	PackageNbest _nbest_res;
	// lattice
	// ali
	PackageAlign _align_time;
	// score
};

#include "src/util/namespace-end.h"

#endif
