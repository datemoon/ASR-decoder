#include <vector>

using namespace std;
using std::vector;

#include "src/newfst/dfs-visit-fst.h"

#include "src/util/namespace-start.h"

void Connect(Lattice *lat)
{
	vector<bool> access;
	vector<bool> coaccess;
	DfsVisit(lat, &access, &coaccess);

	vector<StateId> dstates;
	for(StateId s = 0; s < static_cast<StateId>(access.size()); ++s)
		if (!access[s] || !coaccess[s])
			dstates.push_back(s);

	lat->DeleteStates(dstates);
}
#include "src/util/namespace-end.h"
