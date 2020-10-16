
lib_LTLIBRARIES = libpitch.la

libpitch_la_SOURCES = kaldi-vector.cc \
				   	  pitch-functions.cc \
					  online-feature.cc \
					  resample.cc \
					  kaldi-matrix.cc \
					  kaldi-common.cc

if HAVE_BIN
include ../depend.mk
LDADD = libpitch.la ../util/libutil.la $(MATRIX_LIBS)
bin_PROGRAMS = compute-and-process-kaldi-pitch-feats \
			   stream-compute-and-process-kaldi-pitch-feats \
			   hb-stream-compute-and-process-kaldi-pitch-feats

compute_and_process_kaldi_pitch_feats_SOURCES = compute-and-process-kaldi-pitch-feats.cc

stream_compute_and_process_kaldi_pitch_feats_SOURCES = stream-compute-and-process-kaldi-pitch-feats.cc

hb_stream_compute_and_process_kaldi_pitch_feats_SOURCES = hb-stream-compute-and-process-kaldi-pitch-feats.cc

endif
