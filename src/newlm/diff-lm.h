#ifndef __DIFF_LM_H__
#define __DIFF_LM_H__

#include <utility>
#include <vector>
#include <unordered_map>
#include "src/newlm/compose-arpalm.h"

#include "src/util/namespace-start.h"

template<typename Int1, typename Int2 = Int1>
struct PairHasher
{  // hashing function for pair<int>
	size_t operator()(const std::pair<Int1, Int2> &x) const noexcept 
	{ // 7853 was chosen at random from a list of primes.
		return x.first + x.second * 7853;
	}
	PairHasher() { }
};

// 这是一个计算2个语言模型分差的方法，在构造这个方法之前，需要将语言模型缩放
// 比如一个模型是-1.0 另外一个是 1.0.
class DiffArpaLm:public LatticeComposeItf<FsaStateId>
{
public:
	typedef FsaStateId StateId;

	DiffArpaLm(ArpaLm *lm1, ArpaLm *lm2):
		_lm1(lm1), _lm2(lm2), _start_pair(_lm1.Start(), _lm2.Start())
	{
		LOG_ASSERT(lm1 != NULL && lm2 != NULL);
		StateId start1 = _start_pair.first,
				start2 = _start_pair.second;

		if(start1 == -1 || start2 == -1)
		{
			_start_state = -1;
			_next_state = 0;
		}
		else
		{
			_start_state = 0;
			_state_map[_start_pair] = _start_state;
			_state_vec.push_back(_start_pair);
			_next_state = 1;
		}
   	} // consturt

	void Reset()
	{
		_state_vec.clear();
		_state_map.clear();
		_state_map[_start_pair] = _start_state;
		_state_vec.push_back(_start_pair);
		_next_state = 1;
	}

	virtual StateId Start() { return _start_state; }

	virtual BaseFloat Final(StateId s)
	{
		LOG_ASSERT(s < static_cast<StateId>(_state_vec.size()));
		const std::pair<StateId, StateId> &pr(_state_vec[s]);
		return _lm1.Final(pr.first) + _lm2.Final(pr.second);
	}

	virtual bool GetArc(StateId s, Label ilabel, LatticeArc* oarc)
	{
		oarc->_input = ilabel;
		return GetArc(s, ilabel, &(oarc->_to), &(oarc->_w), &(oarc->_output));
	}

	virtual bool GetArc(StateId s, Label ilabel, StateId *nextstate, LatticeWeight *lweight, Label *olabel)
	{
		if(ilabel == 0)
		{
			*nextstate = s;
			*lweight = LatticeWeight::One();
			*olabel = ilabel;
			return true;
		}
		typedef typename MapType::iterator IterType;
		LOG_ASSERT(ilabel != 0 && "ilabel shouldn't be eps!!!");
		LOG_ASSERT(s < static_cast<StateId>(_state_vec.size()));
		const std::pair<StateId, StateId> pr (_state_vec[s]);
		StateId nextstate1,nextstate2;
		LatticeWeight w1,w2;
		Label olabel1,olabel2;

		if(_lm1.GetArc(s, ilabel, &nextstate1, &w1, &olabel1) != true)
		{
			LOG_WARN << "Lm1 not this ilabe!!!";
			return false;
		}

		if(_lm2.GetArc(s, ilabel, &nextstate2, &w2, &olabel2) != true)
		{
			LOG_WARN << "Lm2 not this ilabe!!!";
			return false;
		}

		std::pair<const std::pair<StateId, StateId>, StateId> new_value(
				std::pair<StateId, StateId>(nextstate1, nextstate2),
				_next_state);
		std::pair<IterType, bool> result = _state_map.insert(new_value);
		*nextstate = result.first->second;
		*olabel = ilabel;
		*lweight = Times(w1,w2);
		if(result.second == true)
		{ // insert ok
			_next_state++;
			_state_vec.push_back(new_value.first);
		}

//		_state_vec.push_back(std::pair<StateId, StateId>(nextstate1, nextstate2));
//		*nextstate = _next_state;
//		*lweight = Times(w1,w2);
//		*olabel = ilabel;
//		_next_state++;
		return true;
	}
private:
	ComposeArpaLm _lm1;
	ComposeArpaLm _lm2;
	typedef std::unordered_map<std::pair<StateId, StateId>, StateId, 
			PairHasher<StateId> > MapType;
	MapType _state_map; // maps from pair to Stateid
	std::vector<std::pair<StateId, StateId> > _state_vec; // maps from StateId to pair.
	std::pair<StateId, StateId> _start_pair;
	StateId _next_state;
	StateId _start_state;
};


#include "src/util/namespace-end.h"
#endif
