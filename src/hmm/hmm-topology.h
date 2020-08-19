/*
 * author:hubo
 * data:  2016-10-12
 *
 * */
#ifndef __HMM_TOPOLOGY_H__
#define __HMM_TOPOLOGY_H__

#include <string>
#include <vector>

#include "src/util/namespace-start.h"

class HmmTopology
{
public:
	struct HmmState
	{
		int _pdf_class;
		std::vector<std::pair<int, float> > _transitions;

		explicit HmmState(int p): _pdf_class(p) { }
		HmmState(): _pdf_class(-1) { }

		bool operator == (const HmmState &other) const 
		{
  			return (_pdf_class == other._pdf_class && _transitions == other._transitions);
		}
	};

	typedef std::vector<HmmState> TopologyEntry;

	void Read(std::istream &is, bool binary);

	void Write(std::ostream &os, bool binary) const;

	// Returns the topology entry (i.e. vector of HmmState) for this phone;
	// will throw exception if phone not covered by the topology.
	const TopologyEntry &TopologyForPhone(int phone) const;
private:
	std::vector<int> _phones; // list of all phones we have topology for.  Sorted, uniq.  no epsilon (zero) phone.
	std::vector<int> _phone2idx;  // map from phones to indexes into the entries vector (or -1 for not present).
	std::vector<TopologyEntry> _entries;
};

#include "src/util/namespace-end.h"

#endif
