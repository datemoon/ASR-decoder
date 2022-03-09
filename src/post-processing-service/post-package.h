#ifndef __POST_PACKAGE_H__
#define __POST_PACKAGE_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <string>
#include <unistd.h>
#include "src/util/log-message.h"
#include "src/util/io-funcs.h"
#include "src/service2/net-data-package.h"

#include "lat/kaldi-lattice.h"

#include "src/util/namespace-start.h"

enum LATTICEFLAG { LATTICE, COMPACTLATTICE};

#define MAXLATTICELEN (1<<27)

struct LatticeHead
{
	uint _compact_lattice :1;
	uint _binary          :1;
	uint _nbest           :3;  // lattice to nbest number.
	uint _lat_len         :27; // 2^7 * 1024 * 1024 kb
	unsigned int          :0;
	LatticeHead():_compact_lattice(1), _binary(1), _nbest(1), _lat_len(0) {}
	void Reset()
	{
		_compact_lattice = 1;
		_binary = 1;
		_nbest = 1;
		_lat_len = 0;
	}

	void Print(std::string prefix="")
	{
		LOG_COM << prefix << "compact_lattice   :" << _compact_lattice;
		LOG_COM << prefix << "binary            :" << _binary;
		LOG_COM << prefix << "nbest             :" << _nbest;
		LOG_COM << prefix << "lat_len           :" << _lat_len;
	}

	bool SetLatticeNbest(uint nbest)
	{
		if((nbest >> 3) > 0)
		{
			LOG_WARN << "nbest " << nbest << " >= 8 !!!";
			return false;
		}
		_nbest = nbest;
		return true;
	}
	bool SetLatticeFormat(uint compact_lattice)
	{
		if((compact_lattice >> 1) > 0)
		{
			LOG_WARN << "compact_lattice " << compact_lattice  << " > 1 !!!";
			return false;
		}
		_compact_lattice = compact_lattice;
		return true;
	}
	bool SetBinary(uint binary)
	{
		if((binary >> 1) > 0)
		{
			LOG_WARN << "binary " << binary << " > 1 !!!";
			return false;
		}
		_binary = binary;
		return true;
	}
	bool SetLatticeLen(uint lat_len)
	{
		if((lat_len >> 27) > 0)
		{
			LOG_WARN << "lat_len " << lat_len << " > 2^27 !!!";
			return false;
		}
		_lat_len = lat_len;
		return true;
	}
};

// lattice的内存需要外部管理
class PackageLattice//: public NetPackageBase
{
public:
	PackageLattice(LatticeHead &lattice_head, 
			kaldi::CompactLattice **clat, kaldi::Lattice **lat):
	   	_lattice_head(lattice_head), _clat(clat), 
		_lat(lat) { }

	~PackageLattice() { }

	ssize_t Read(int sockfd)
	{
		ssize_t ret1 = ReadN(sockfd, static_cast<void *>(&_lattice_head),
				sizeof(_lattice_head));
		if(ret1 < 0)
		{
			LOG_WARN << "Read LatticeHead failed!!!" ;
			return ret1;
		}
		std::string lat_str(_lattice_head._lat_len, '0');

		ssize_t ret2 = ReadN(sockfd, static_cast<void *>(&lat_str[0]),
				lat_str.size());
		if(ret2 < 0)
		{
			LOG_WARN << "Read Lattice failed!!!";
			return ret2;
		}
		std::istringstream iss(lat_str);
		//std::istream is(iss.rdbuf());
		// 从lat_str读到lattice中
		if(_lattice_head._compact_lattice == LATTICE)
		{
			*_lat = NULL;
			bool readlat_ret = kaldi::ReadLattice(iss, _lattice_head._binary, _lat);
			if(readlat_ret != true)
			{
				LOG_WARN << "ReadLattice istream failed!!!";
				return -1;
			}
		}
		else if(_lattice_head._compact_lattice == COMPACTLATTICE)
		{
			*_clat = NULL;
			bool readlat_ret = 
				kaldi::ReadCompactLattice(iss, _lattice_head._binary, _clat);
			if(readlat_ret != true)
			{
				LOG_WARN << "ReadCompactLattice istream failed!!!";
				return -1;
			}
		}
		else
		{
			LOG_WARN << "lattice format error!!!";
			return -1;
		}
		return ret1+ret2;
	}

	// lattice 由外部保存，所以每次写完lattice都需要设置lattice=NULL
	ssize_t Write(int sockfd)
	{
		std::ostringstream oss;
		//std::ostream os(oss.rdbuf());
		if(_lattice_head._compact_lattice == LATTICE)
		{
			bool latwrite_ret = kaldi::WriteLattice(oss, _lattice_head._binary, **_lat);
			*_lat = NULL;
			if(latwrite_ret != true)
			{
				LOG_WARN << "WriteLattice failed!!!";
				return -1;
			}
		}
		else if(_lattice_head._compact_lattice == COMPACTLATTICE)
		{
			bool latwrite_ret = 
				kaldi::WriteCompactLattice(oss, _lattice_head._binary, **_clat);
			*_clat = NULL;
			if(latwrite_ret != true)
			{
				LOG_WARN << "WriteCompactLattice failed!!!";
				return -1;
			}
		}
		else
		{
			LOG_WARN << "lattice format error!!!";
			return -1;
		}
		if(_lattice_head.SetLatticeLen(oss.str().size()) != true)
		{
			LOG_WARN << "LatticeHead lat_len set error!!!";
			return -1;
		}

		ssize_t ret1 = WriteN(sockfd, static_cast<void*>(&_lattice_head),
				sizeof(_lattice_head));
		if(ret1 < 0)
		{
			LOG_WARN << "write LatticeHead failed!!!";
			return ret1;
		}
		ssize_t ret2 = WriteN(sockfd, static_cast<void*>(&(oss.str()[0])),
				oss.str().size());
		if(ret2 < 0)
		{
			LOG_WARN << "write Lattice data failed!!!";
			return ret2;
		}

		return ret1+ret2;;
	}

	void Reset()
	{
		_lattice_head.Reset();
	}

	// if before Write Print will be print lattice
	// else lattice not print
	void Print(std::string prefix="") { }
private:
	LatticeHead &_lattice_head;
	kaldi::CompactLattice **_clat;
	kaldi::Lattice **_lat;
};

// 读写顺序为，包头，nbest结果(可以没有这段数据)，lattice结果(可以没有这段数据)
class AsrReturnPackageAnalysis
{
public:
	AsrReturnPackageAnalysis(uint nbest=0,uint lattice=0,
			uint ali_info=0, uint score_info=0, uint end_flag=0, uint nres=0):
		_clat(NULL), _lat(NULL),
		_lattice_res(_lattice_head, &_clat, &_lat)
	{
		memset(&_s2c_package_head, 0x00, sizeof(_s2c_package_head));
		_s2c_package_head._nbest = nbest;
		_s2c_package_head._lattice = lattice;
		_s2c_package_head._ali_info = ali_info;
		_s2c_package_head._score_info = score_info;
		_s2c_package_head._end_flag = end_flag;
		_s2c_package_head._nres = nres;
	}

	~AsrReturnPackageAnalysis()
	{
		Reset();
	}

	// 重置包头信息，nbest重置，lattice重置
	// GetData 之后使用完获取数据之后必须调用Reset，
	// 删除读取的lattice
	void Reset(bool delete_all = true)
	{
		_s2c_package_head.Reset();
		_nbest_res.Reset();
		_lattice_res.Reset();
		_lattice_head.Reset();
		if(delete_all == false)
		{
			_clat = NULL;
			_lat = NULL;
		}
		if(_clat != NULL)
		{
			VLOG_COM(1) << "delete _clat...";
			delete _clat;
			_clat = NULL;
		}
		if(_lat != NULL)
		{
			VLOG_COM(1) << "delete _lat...";
			delete _lat;
			_lat = NULL;
		}
	}

	// asr result transmission read socket
	ssize_t Read(int sockfd)
	{
		ssize_t ret1 = ReadN(sockfd, 
				static_cast<void *>(&_s2c_package_head), sizeof(S2CPackageHead));
		if(ret1 < 0)
		{
			LOG_WARN << "Read S2CPackageHead failed!!!";
			return -1;
		}

		ssize_t ret2 = 0;
		if(_s2c_package_head._nbest > 0)
		{ // read nbest
			ret2 = _nbest_res.Read(sockfd);//, _s2c_package_head._nbest);
			if(ret2 < 0)
			{
				LOG_WARN << "Read Nbest failed!!!";
				return -1;
			}
		}

		ssize_t ret3 = 0;
		if(_s2c_package_head._lattice != 0)
		{ // read lattice
			ret3 = _lattice_res.Read(sockfd);
			if(ret3 < 0)
			{
				LOG_WARN << "Read Lattice failed!!!";
				return -1;
			}
		}
		return ret1 + ret2 + ret3;
	}

	// asr result transmission write socket
	ssize_t Write(int sockfd, uint end_flag)
	{
		//_s2c_package_head._nbest = _nbest_res.GetN();
		_s2c_package_head._end_flag = end_flag;
		if(_s2c_package_head._end_flag != S2CNOEND)
			_s2c_package_head._nres++;
		if(_s2c_package_head._nbest == 0 && _s2c_package_head._end_flag == S2CNOEND 
				&& _s2c_package_head._lattice == 0)
		{
			VLOG_COM(1) << "No result and not end, so no send package to client.";
			return true;
		}
		ssize_t ret1 = WriteN(sockfd, static_cast<void *>(&_s2c_package_head), 
				sizeof(S2CPackageHead));
		if(ret1 < 0)
		{
			LOG_WARN << "Write S2CPackageHead failed.";
			return ret1;
		}

		ssize_t ret2 = 0;
		if(_s2c_package_head._nbest != 0)
		{ // have nbest
			ret2 = _nbest_res.Write(sockfd);
			if(ret2 < 0)
			{
				LOG_WARN << "Write Nbest failed!!!";
				return -1;
			}
		}

		ssize_t ret3 = 0;
		if(_s2c_package_head._lattice != 0)
		{ // have lattice
			ret3 = _lattice_res.Write(sockfd);
			if(ret3 < 0)
			{
				LOG_WARN << "Write Lattice failed!!!";
				return -1;
			}
		}
		return ret1 + ret2 + ret3;
	}

	uint GetEndflag() { return _s2c_package_head._end_flag; }

	bool SetUseLattice(uint lattice) { return _s2c_package_head.SetUseLattice(lattice); }
	
	bool SetUseAligninfo(uint ali_info) { return _s2c_package_head.SetUseAligninfo(ali_info); }
	
	bool SetUseScoreinfo(uint score_info) { return _s2c_package_head.SetUseScoreinfo(score_info); }
	
	bool IsAllEnd() { return _s2c_package_head.IsAllEnd(); }

	bool IsMiddleEnd() { return _s2c_package_head.IsMiddleEnd(); }

	bool DoRescore()
	{ return _s2c_package_head._do_rescore; }

	bool DoPunctuate()
	{ return _s2c_package_head._do_punctuate; }

	bool SetDoRescore(uint do_rescore) 
	{ return _s2c_package_head.SetDoRescore(do_rescore); }

	bool SetDoPunctuate(uint do_pimctuate)
	{ return _s2c_package_head.SetDoPunctuate(do_pimctuate); }

	// Set nbest result.
	// if you result greater than 1, you should be recursive call n times.
	void SetNbest(const std::string &result)
	{
		std::vector<std::string> nbest;
		nbest.push_back(result);
		_nbest_res.SetData(nbest);
		_s2c_package_head.SetUseNbest(1);
		//_nbest_res.SetNbest(result);
		//_s2c_package_head._nbest++;
	}

	//uint GetN() { return _s2c_package_head._nbest; }

	void SetNbest(std::vector<std::string> &nbest_result)
	{
		_nbest_res.SetData(nbest_result);
		_s2c_package_head.SetUseNbest(1);
		//for(size_t i=0;i<nbest_result.size(); ++i)
		//{
	//		SetNbest(nbest_result[i]);
	//	}
	}

	bool SetLatticeNbest(uint nbest=1) 
	{ return _lattice_head.SetLatticeNbest(nbest);}

	uint GetLatticeNbest() { return _lattice_head._nbest;}

	// 当使用Write时，需要先设置lattice，当lattice读入之后立刻重置为NULL
	// 因为设置的lattice由外部保存
	void SetLattice(kaldi::CompactLattice *clat, uint binary=1)
	{
		_s2c_package_head._lattice = 1;
		_lattice_head._binary = binary;
		_lattice_head._compact_lattice = 1;
		_clat = clat;
	}
	void SetLattice(kaldi::Lattice *lat, uint binary = 1)
	{
		_s2c_package_head._lattice = 1;
		_lattice_head._binary = binary;
		_lattice_head._compact_lattice = 0;
		_lat = lat;
	}

	// 配合Read使用，并且需要内部控制lattice的free
	void GetData(std::vector<std::string> *nbest, 
			kaldi::CompactLattice *&clat, kaldi::Lattice *&lat)
	{
		*nbest = _nbest_res.GetData();
		if(_lattice_head._compact_lattice == 1)
		{
			clat = _clat;
			lat = NULL;
		}
		else
		{
			clat = NULL;
			lat = _lat;
		}
	}

	void Print(std::string prefix="")
	{
		_s2c_package_head.Print(prefix, 0);
		VLOG_COM(0) << prefix << " " << _nbest_res;
		//_nbest_res.Print(prefix);
		_lattice_res.Print(prefix);
		_lattice_head.Print(prefix);
		if(_clat != NULL)
		{
			fst::FstPrinter<kaldi::CompactLatticeArc> printer(*_clat, NULL, NULL,
					NULL, true, true, "\t");
			printer.Print(&std::cout, "<unknown>");
		}
		if(_lat != NULL)
		{
			fst::FstPrinter<kaldi::LatticeArc> printer(*_lat, NULL, NULL,
					                    NULL, true, true, "\t");
			printer.Print(&std::cout, "<unknown>");
		}
	}

	int GetNres()
	{
		return _s2c_package_head._nres;
	}
private:
	S2CPackageHead _s2c_package_head;
	//S2CPackageNbest _nbest_res;
	PackageNbest _nbest_res;
	kaldi::CompactLattice *_clat;
	kaldi::Lattice *_lat;
	LatticeHead _lattice_head;
	PackageLattice _lattice_res;
};


#include "src/util/namespace-end.h"

#endif
