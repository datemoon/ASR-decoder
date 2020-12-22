#include "stream2pcm.h"


int main(int argc, char *argv[])
{
	struct AudioConvertOpt opt;
	char *filename = argv[1];
	Stream2Pcm stream2pcm(opt, 1024*4, filename);

	stream2pcm.ConvertData();
	return 0;
}
