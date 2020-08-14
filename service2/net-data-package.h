#ifndef __NET_DATA_PACKAGE_H__
#define __NET_DATA_PACKAGE_H__

typedef unsigned int uint;
// _bit             : 8bit,16bit,32bit
// _sample_rate     : 8k,16k,32k...
// _audio_type      : wav,opus,pcm
// _audio_head_flage: 0|1
// _nbest           : 
// _lattice         :
// _ali_info        : 0|1
// _score_info      : 0|1
// _keep            : 0|1
// _n               :
// _data_len        : 
// sizeof(PackageHead) = 96;
struct PackageHead
{
	uint _bit              : 4;
	uint _sample_rate      : 4;
	uint _audio_type       : 2;
	uint _audio_head_flage : 1;
	uint _nbest            : 6;
	uint _lattice          : 1;
	uint _ali_info         : 1;
	uint _score_info       : 1;
	uint _keep             : 1;
	unsigned int           : 0;
	// align to next unsigned int.

	uint _n                : 32;
	uint _data_len         : 32;
};

class PackageHeadAnalysis
{
public:
	PackageHeadAnalysis();
	~PackageHeadAnalysis()  { }
	enum AUDIOBIT
	{
		16BIT=0,8BIT=1,32BIT=2
	};

	enum SAMPLERATE
	{
		16K=0,8K=1
	};
	enum AUDIOTYPE
	{
		PCM=0,WAV=1,OPUS=2
	};
	enum RETTYPE
	{
		NBEST=0,LATTICE=1
	};
public:
	void SetN(uint n)
	{
		_package_head._n = n;
	}
	void SetDataLen(uint data_len)
	{
		_package_head._data_len = data_len;
	}
	bool SetBit(uint bit)
	{
		if(bit >> 4 > 0)
			return false;
		_package_head._bit = bit;
		return true;
	}
	bool SetSampleRate(uint sample_rate)
	{
		if(sample_rate >> 16 > 0)
			return false;
		_package_head._sample_rate = sample_rate;
		return true;
		
	}

	bool SetAudioType(uint audio_type)
	{
		if(audio_type >> 2 > 0)
			return false;
		_package_head._audio_type = audio_type;
		return true;
	}

	bool SetAudioHeadFlage(uint audio_head_flage)
	{
		if(audio_head_flage >> 1 > 0)
			return false;
		_package_head._audio_head_flage = audio_head_flage;
		return true;
	}

	bool SetRetType(uint ret_type)
	{
		if(ret_type >> 2 > 0)
			return false;
		_package_head._ret_type = ret_type;
		return true;
	}

	bool setNbest(uint nbest = 1)
	{
		if(nbest >> 6 > 0)
			return false;
		_package_head._nbest = nbest;
		return true;
	}

	bool SetLattice(uint lattice = 0)
	{
		if(lattice >> 1 > 0)
			return false;
		_package_head._lattice = lattice;
		return true;
	}

	bool SetAliInfo(uint ali_info = 0)
	{
		if(ali_info >> 1 > 0)
			return false;
		_package_head._ali_info = ali_info;
		return true;
	}

	bool SetScoreInfo(uint score_info = 0)
	{
		if(score_info >> 1 > 0)
			return false;
		_package_head._score_info = score_info;
		return true;
	}

	bool SetKeep(uint keep = 0)
	{
		if(keep >> 1 > 0)
			return false;
		_package_head._keep = keep;
		return true;
	}

	bool Write(int sockfd);
	bool Read(int sockfd);
private:
	struct PackageHead _package_head;
};

class NetPackage
{
public:
private:

};

#endif
