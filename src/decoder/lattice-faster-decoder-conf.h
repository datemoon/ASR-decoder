#ifndef __LATTICE_FASTER_DECODE_CONF_H__
#define __LATTICE_FASTER_DECODE_CONF_H__

#include "src/util/config-parse-options.h"

#include "src/util/namespace-start.h"

struct LatticeFasterDecoderConfig 
{
	void Print()
	{
		LOG_COM << "beam\t:" << _beam ;
		LOG_COM << "max_active\t:" << _max_active;
		LOG_COM << "min_active\t:" << _min_active;
		LOG_COM << "lattice_beam\t:" << _lattice_beam;
		LOG_COM << "prune_interval\t:" << _prune_interval;
		LOG_COM << "determinize_lattice:\t" << _determinize_lattice;
		LOG_COM << "beam_delta\t:" << _beam_delta;
		LOG_COM << "hash_ratio\t:" << _hash_ratio;
		LOG_COM << "prune_scale\t:" << _prune_scale;
	}
	float _beam;
	int _max_active;
	int _min_active;
	float _lattice_beam;
	int _prune_interval;
	bool _determinize_lattice; // no use

	float _beam_delta;
	float _hash_ratio;
	float _prune_scale; // Note: we don't make this configurable on the command line,
						// it's not a very important parameter.  It affects the
						// algorithm that prunes the tokens as we go.
	
	LatticeFasterDecoderConfig(): 
		_beam(16.0),
		_max_active(std::numeric_limits<int>::max()),
		_min_active(200),
		_lattice_beam(10.0),
		_prune_interval(25),
		_determinize_lattice(true),
		_beam_delta(0.5),
		_hash_ratio(2.0),
		_prune_scale(0.1) { }

	void Register(ConfigParseOptions *conf)
	{
		conf->Register("beam", &_beam, "Decoding beam.  Larger->slower, more accurate.");
		conf->Register("max-active", &_max_active, "Decoder max active states.  Larger->slower; more accurate");
		conf->Register("min-active", &_min_active, "Decoder minimum #active states.");
		conf->Register("lattice-beam", &_lattice_beam, "Lattice generation beam.  Larger->slower, and deeper lattices");
		conf->Register("prune-interval", &_prune_interval, "Interval (in frames) at which to prune tokens");
		conf->Register("determinize-lattice", &_determinize_lattice, "If true, "
				"determinize the lattice (lattice-determinization, keeping only "
				"best pdf-sequence for each word-sequence).");
		conf->Register("beam-delta", &_beam_delta, "Increment used in decoding-- this "
				"parameter is obscure and relates to a speedup in the way the "
				"max-active constraint is applied.  Larger is more accurate.");
		conf->Register("hash-ratio", &_hash_ratio, "Setting used in decoder to "
				"control hash behavior");
	}
	void Check() const 
	{
		assert(_beam > 0.0 && _max_active > 1 && _lattice_beam > 0.0
				&& _prune_interval > 0 && _beam_delta > 0.0 && _hash_ratio >= 1.0
				&& _prune_scale > 0.0 && _prune_scale < 1.0);
	}
};
#include "src/util/namespace-end.h"
#endif
