#ifndef __CONST_FST_H__
#define __CONST_FST_H__

#include <iostream>
#include <fstream> 
#include <sstream>
#include <string>
#include <climits>
#include "src/newfst/arc.h"
#include "src/util/util-type.h"
#include "src/util/log-message.h"
#include "src/util/io-funcs.h"
//#include "src/newfst/optimize-fst.h"

#include "src/util/namespace-start.h"

// Identifies stream data as an FST (and its endianity).
constexpr int32 kFstMagicNumber = 2125659606;
constexpr uint64 kError = 0x0000000000000004ULL;

// it's same openfst 
class FstHeader
{
public:
	FstHeader() : 
		version_(0), flags_(0), properties_(0), start_(-1),
		numstates_(0), numarcs_(0) {}

	const std::string &FstType() const { return fsttype_; }
	const std::string &ArcType() const { return arctype_; }
	int32 Version() const { return version_; }
	int32 GetFlags() const { return flags_; }
	uint64 Properties() const { return properties_; }
	int64 Start() const { return start_; }
	int64 NumStates() const { return numstates_; }
	int64 NumArcs() const { return numarcs_; }
	void SetFstType(const std::string &type) { fsttype_ = type; }
	void SetArcType(const std::string &type) { arctype_ = type; }
	void SetVersion(int32 version) { version_ = version; }
	void SetFlags(int32 flags) { flags_ = flags; }
	void SetProperties(uint64 properties) { properties_ = properties; }
	void SetStart(int64 start) { start_ = start; }
	void SetNumStates(int64 numstates) { numstates_ = numstates; }
	void SetNumArcs(int64 numarcs) { numarcs_ = numarcs; }

	bool Read(std::istream &strm, const std::string &source,
			bool rewind = false)
	{
		int64 pos = 0;
		if (rewind) pos = strm.tellg();
		int32 magic_number = 0;
		strm.read(reinterpret_cast<char *>(&magic_number),sizeof(magic_number));
		if(magic_number != kFstMagicNumber)
		{
			LOG_WARN << "FstHeader::Read: Bad FST header: " << source;
			if(rewind) strm.seekg(pos);
			return false;
		}
		ReadType(strm, &fsttype_);
		ReadType(strm, &arctype_);
		ReadType(strm, &version_);
		ReadType(strm, &flags_);
		ReadType(strm, &properties_);
		ReadType(strm, &start_);
		ReadType(strm, &numstates_);
		ReadType(strm, &numarcs_);
//		strm.read(reinterpret_cast<char *>(&version_), sizeof(version_));
//		strm.read(reinterpret_cast<char *>(&flags_), sizeof(flags_));
//		strm.read(reinterpret_cast<char *>(&properties_), sizeof(properties_));
//		strm.read(reinterpret_cast<char *>(&start_), sizeof(start_));
//		strm.read(reinterpret_cast<char *>(&numstates_), sizeof(numstates_));
//		strm.read(reinterpret_cast<char *>(&numarcs_), sizeof(numarcs_));
		if (!strm)
		{
			LOG_WARN << "FstHeader::Read: Read failed: " << source;
			return false;
		}
		if (rewind) strm.seekg(pos);
		return true;
	}
	bool Write(std::ostream &strm, const std::string &source) const
	{
		strm.write(reinterpret_cast<const char *>(&kFstMagicNumber), sizeof(kFstMagicNumber));
		strm.write(reinterpret_cast<const char *>(&fsttype_), sizeof(fsttype_));
		strm.write(reinterpret_cast<const char *>(&arctype_), sizeof(arctype_));
		strm.write(reinterpret_cast<const char *>(&version_), sizeof(version_));
		strm.write(reinterpret_cast<const char *>(&flags_), sizeof(flags_));
		strm.write(reinterpret_cast<const char *>(&properties_), sizeof(properties_));
		strm.write(reinterpret_cast<const char *>(&start_), sizeof(start_));
		strm.write(reinterpret_cast<const char *>(&numstates_), sizeof(numstates_));
		strm.write(reinterpret_cast<const char *>(&numarcs_), sizeof(numarcs_));
		return true;
	}
	// Outputs a debug string for the FstHeader object.
	std::string DebugString() const
	{
		std::ostringstream ostrm;
		ostrm << "fsttype: \"" << fsttype_ << "\" arctype: \"" << arctype_
			<< "\" version: \"" << version_ << "\" flags: \"" << flags_
			<< "\" properties: \"" << properties_ << "\" start: \"" << start_
			<< "\" numstates: \"" << numstates_ << "\" numarcs: \"" << numarcs_
			<< "\"";
		return ostrm.str();
	}

private:
	std::string fsttype_;     // E.g. "vector".
	std::string arctype_;     // E.g. "standard".
	int32 version_;      // Type version number.
	int32 flags_;        // File format bits.
	uint64 properties_;  // FST property bits.
	int64 start_;        // Start state.
	int64 numstates_;    // # of states.
	int64 numarcs_;      // # of arcs.
};

template<class A, class Unsigned>
class ConstFst
{
public:
	friend class Fst;
	using Arc = A;
	using StateId = typename Arc::StateId;
	using Weight = typename Arc::Weight;

	ConstFst():
		states_(nullptr),
		arcs_(nullptr),
		nstates_(0),
		narcs_(0),
		start_(kNoStateId) 
	{
		std::string type = "const";
		if (sizeof(Unsigned) != sizeof(uint32)) 
		{
			type += std::to_string(CHAR_BIT * sizeof(Unsigned));
		}
		type_ = type;


	}

	~ConstFst()
	{
		if (states_ != NULL)
			delete [] states_;
		states_ = NULL;
		if(arcs_ != NULL)
			delete [] arcs_;
		arcs_ = NULL;
		nstates_ = 0;
		narcs_ = 0;
		start_ = kNoStateId;
	}

	void SetProperties(uint64 props) 
	{
		properties_ &= kError;  // kError can't be cleared.
		properties_ |= props;
	}

	StateId Start() const { return start_; }

	StateId NumStates() const { return nstates_; }
	size_t NumArcs(StateId s) const { return states_[s].narcs; }
	const Arc *Arcs(StateId s) const { return arcs_ + states_[s].pos; }



	bool Read(const std::string &filename)
	{
		if (!filename.empty())
		{
			std::ifstream strm(filename,
					std::ios_base::in | std::ios_base::binary);
			if (!strm) 
			{
				LOG_WARN << "Fst::Read: Can't open file: " << filename;
				return false;
			}
			return Read(strm, filename);
		}
		else
		{
			return Read(std::cin, "standard input");
		}
	}

	bool Read(std::istream &strm, const std::string &source)
	{
		header_.Read(strm, source);
		if(header_.FstType() != type_)
		{
			LOG_WARN << "FstImpl::ReadHeader: FST not of type " << type_
				<< ": " << source;
			return false;
		}
		start_ = header_.Start();
		nstates_ = header_.NumStates();
		narcs_ = header_.NumArcs();

		size_t b = nstates_ * sizeof(ConstState);
		states_ = new ConstState[nstates_];
		strm.read(reinterpret_cast<char *>(states_), b);

		if(!strm)
		{
			LOG_WARN << "ConstFst::Read: Read failed: " << source;
			return false;
		}

		b = narcs_ * sizeof(Arc);
		arcs_ = new Arc[narcs_];
		strm.read(reinterpret_cast<char *>(arcs_), b);
		if (!strm)
		{
			LOG_WARN << "ConstFst::Read: Read failed: " << source;
			return false;
		}
		return true;
	}

private:
	// States implemented by array *states_ below, arcs by (single) *arcs_.
	struct ConstState
   	{
		Weight weight;        // Final weight.
		Unsigned pos;         // Start of state's arcs in *arcs_.
		Unsigned narcs;       // Number of arcs (per state).
		Unsigned niepsilons;  // Number of input epsilons.
		Unsigned noepsilons;  // Number of output epsilons.

		ConstState() : weight(Weight::Zero()) {}
	};
	mutable uint64 properties_;  // Property bits.
	std::string type_;  // Unique name of FST class.

	ConstState *states_;  // States representation.
	Arc *arcs_;           // Arcs representation.
	StateId nstates_;     // Number of states.
	size_t narcs_;        // Number of arcs.
	StateId start_;       // Initial state.

	FstHeader header_;
};

typedef ConstFst<StdArc, int> StdConstFst;
#include "src/util/namespace-end.h"
#endif
