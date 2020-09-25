#ifndef __DECODER_BASH_H__
#define __DECODER_BASH_H__

#include "src/itf/decodable-itf.h"
#include "src/util/util-common.h"
#include "src/newfst/lattice-fst.h"

#include "src/util/namespace-start.h"

class DecoderItf
{
public:

	virtual ~DecoderItf(){}
	virtual void InitDecoding() = 0;
	virtual void AdvanceDecoding(AmInterface *decodable, int32 max_num_frames = -1) = 0;
	virtual void FinalizeDecoding() = 0;
	virtual int32 NumFramesDecoded() const = 0;
	virtual BaseFloat ProcessEmitting(AmInterface *decodable) = 0;
	virtual void ProcessNonemitting(BaseFloat cost_cutoff) = 0;
	virtual bool Decode(AmInterface *decodable) = 0;
	virtual bool GetBestPath(Lattice *ofst, bool use_final_probs = true) = 0;
	virtual bool GetRawLattice(Lattice *ofst, bool use_final_probs = true) = 0;

};

#include "src/util/namespace-end.h"

#endif
