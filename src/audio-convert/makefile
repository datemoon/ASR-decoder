



FFMPEG_INC=/usr/local/include/
FFMPEG_LIBS=/usr/local/lib/libavformat.a \
			/usr/local/lib/libswresample.a \
			/usr/local/lib/libavcodec.a \
			/usr/local/lib/libavutil.a

LD_FLAGS=-pthread -liconv -lm -llzma -lz -lva -lbz2 -lva-drm -lva -lva-x11 -lva -lm -lva -lXv -lX11 -lXext
CXXFLAGS=-std=c++11 
CXX=g++ -g
all:stream2pcm

stream2pcm:stream2pcm.o
	$(CXX) $< $(FFMPEG_LIBS) -o $@ $(LD_FLAGS)

%.o:%.cc
	$(CXX) $(CXXFLAGS) -I$(FFMPEG_INC) -c -o $@ $<


.PHONY:

clean:
	rm -f *.o stream2pcm

