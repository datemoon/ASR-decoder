#ifndef __NET_DATA_PACKAGE_H__
#define __NET_DATA_PACKAGE_H__

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

// from client to service unpack.
class C2SPackageHeadAnalysis
{
public:
	enum DTYPE { DSHORT=0,DFLOAT=1 };
	enum AUDIOBIT { 16BIT=0,8BIT=1,32BIT=2 };

	enum SAMPLERATE { 16K=0,8K=8,32K=32 };
	enum AUDIOTYPE { PCM=0,WAV=1,OPUS=2 };
public:
	C2SPackageHeadAnalysis(uint dtype=DSHORT, uint bit=16BIT, uint sample_rate=16K,
			uint audio_type=PCM, uint audio_head_flage = 0, uint lattice=0, uint ali_info=0
			uint score_info=0, uint nbest = 0, uint end_flag=0, uint keep=0,
			uint n=0, uint data_len=0):
		_data_buffer(NULL),_data_buffer_len(0)
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
	~C2SPackageHeadAnalysis()  { }
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
	bool C2SWrite(int sockfd, char *data, uint data_size,
			uint n=0, uint end_flag=0);
	// from client to service package unpack.
	bool C2SRead(int sockfd);
private:
	struct C2SPackageHead _c2s_package_head;
	char *_data_buffer;
	uint _data_buffer_len;
};

class NetPackage
{
public:
private:

};

#endif
