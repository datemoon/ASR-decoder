#ifndef __STREAM2PCM_H__
#define __STREAM2PCM_H__

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
};
#endif

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

struct AudioConvertOpt
{
	AudioConvertOpt():
		_out_channel(1), _out_sample_rate(16000), _out_bit(16) { }

	int _out_channel;
	int _out_sample_rate;
	int _out_bit;
};

int totaln=0;
	int ReadBuffer(void *opaque, uint8_t *buf, int bufsize)
	{
		FILE *fp = reinterpret_cast<FILE*>(opaque);
		printf("%d want read %d\n",totaln,bufsize);
		size_t size = fread(buf, sizeof(char), bufsize, fp);
		sleep(1);
		printf("%d read %d\n",totaln,size);
		totaln++;
		if(size == 0)
			return AVERROR_EOF;
		return size;
	}
class Stream2Pcm
{
public:
	Stream2Pcm(const AudioConvertOpt &opt, int read_bufsize, char *filename):
		_convet_opt(opt), _read_bufsize(read_bufsize)
	{
		outfp = fopen("tmp.pcm", "w");
		totaln = 0;
		/*
		if(pthread_mutex_init(&_pthread_mutex, NULL) != 0)
		{
			std::cout << "pthread_mutex_init _pthread_mutex failed." << std::endl;
			return ;
		}
		if(pthread_cond_init(&_pthread_cond, NULL) != 0)
		{
			std::cout << "pthread_cond_init _pthread_pool_cond failed." << std::endl;
			return ;
		}
		*/
		avformat_network_init();

		_pformat_ctx = avformat_alloc_context();

		_read_buffer = (unsigned char *)av_malloc(_read_bufsize);
		// libavformat/avio.h
		FILE *fp = fopen(filename, "r");
		if(fp == NULL)
		{
			printf("open %s failed!!!\n", filename);
			return;
		}
		_pavio_ctx = avio_alloc_context(_read_buffer, _read_bufsize, 0, 
				reinterpret_cast<void*>(fp), ReadBuffer, NULL, NULL);

		assert(NULL != _pavio_ctx && "avio_alloc_context failed!!!");

		_pformat_ctx->pb = _pavio_ctx;

		// libavformat/avformat.h
		const char *url = "mem-read";
		int ret = avformat_open_input(&_pformat_ctx, NULL, NULL, NULL);
		assert(ret == 0 && "avformat_open_input failed!!!");

		// Retrieve stream information
		ret = avformat_find_stream_info(_pformat_ctx, NULL);
		assert(ret >= 0 && "avformat_find_stream_info failed!!!");

		// Dump valid information onto standard error
		int index = 0;
		int is_output = 0;
		// Print detailed information about the input or output format, such as
		// duration, bitrate, streams, container, programs, metadata, side data,
		// codec and time base.
		// libavformat/avformat.h
		av_dump_format(_pformat_ctx, index, url, is_output);

		// Find the first audio stream
		_audio_stream = -1;
		for(int i=0; i < _pformat_ctx->nb_streams; i++)
		{
			if(_pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				_audio_stream = i;
				break;
			}
		}
		assert(_audio_stream != -1 && "Didn't find a audio stream!!!");

		// Get a pointer to the codec context for the audio stream
		_pcodec_ctx = _pformat_ctx->streams[_audio_stream]->codec;
		// Find the decoder for the audio stream
		AVCodec *_pcodec = avcodec_find_decoder(_pcodec_ctx->codec_id);
		assert(_pcodec != NULL && "Could not open codec!!!");

		// Open codec
		ret = avcodec_open2(_pcodec_ctx, _pcodec, NULL);
		assert(ret >= 0 && "Could not open codec!!!");

		// Out Audio Param
		// channel = 1
		uint64_t out_channel_layout = AV_CH_LAYOUT_MONO;//AV_CH_LAYOUT_STEREO;
		// nb_samples
		int out_nb_samples = _pcodec_ctx->frame_size;
		// libavutil/samplefmt.h
		AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16P;
		
		int out_sample_rate = _convet_opt._out_sample_rate;
		int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
		// libavutil/samplefmt.h
		out_buffer_size = av_samples_get_buffer_size(NULL, out_channels,
				out_nb_samples, out_sample_fmt, 1);

		// Out Buffer Size
		out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

		int64_t in_channel_layout = av_get_default_channel_layout(_pcodec_ctx->channels);
		
		_swr_convert_ctx = swr_alloc();
		_swr_convert_ctx = swr_alloc_set_opts(_swr_convert_ctx,
				out_channel_layout, out_sample_fmt,
				out_sample_rate, in_channel_layout,
				_pcodec_ctx->sample_fmt , _pcodec_ctx->sample_rate,
				0, NULL);
		swr_init(_swr_convert_ctx);
	}

	//static int WriteBuffer(void *opaque, uint8_t *buf, int bufsize)
	//{
	//}

	int ConvertData()
	{
		AVPacket * packet = (AVPacket *)av_malloc(sizeof(AVPacket));
		av_init_packet(packet);
		AVFrame *pframe = av_frame_alloc();
		int got_picture = 0;
		int index = 0;
		while(av_read_frame(_pformat_ctx, packet) >= 0)
		{
			if(packet->stream_index == _audio_stream)
			{
				int ret = avcodec_decode_audio4(_pcodec_ctx, pframe, &got_picture, packet);
				if(ret < 0)
				{
					printf("Error in decoding audio frame!!!\n");
					return -1;
				}
				if(got_picture > 0)
				{
					swr_convert(_swr_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pframe->data , pframe->nb_samples);
					printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);
					//Write PCM
					fwrite(out_buffer, 1, out_buffer_size, outfp);
					                index++;
				}
			}
			av_free_packet(packet);
		}
		swr_free(&_swr_convert_ctx);
		av_free(out_buffer);
		avcodec_close(_pcodec_ctx);
		avformat_close_input(&_pformat_ctx);
		printf("ok...\n");
		fclose(outfp);
		return 0;
	}
private:

	//pthread_mutex_t _pthread_mutex; // thread synchronization lock
	//pthread_cond_t _pthread_cond;   // thread condition lock
	// shared memory
	//char * _shared_mem ;
	//int _shared_mem_size;

	int _audio_stream;
	const AudioConvertOpt &_convet_opt;
	AVFormatContext *_pformat_ctx;
	struct SwrContext *_pswr_ctx;
	AVIOContext *_pavio_ctx;
	unsigned char *_read_buffer;
	int _read_bufsize;
	AVCodecContext  *_pcodec_ctx;

	struct SwrContext * _swr_convert_ctx;
	uint8_t * out_buffer;
	int out_buffer_size;
	FILE *outfp;
};



#endif
