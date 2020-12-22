

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "src/client/asr-client-api.h"

#define LEN 4096

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		std::cout << argv[0] << " port wavfile" << std::endl;
		return -1;
	}
	int port = atoi(argv[1]);
	int ret = TcpConnect("127.0.0.1", port);
	if(ret < 0)
	{
		std::cout << "TcpConnect failed!!!" << std::cout;
		return -1;
	}
	char *wav_file = argv[2];
	FILE *fp = fopen(wav_file, "r");
	while(true)
	{
		char sentbuf[LEN];
		char recvbuf[LEN];
		memset(sentbuf,0x00,sizeof(sentbuf));
		memset(recvbuf,0x00,sizeof(recvbuf));
		size_t sent_len = fread((void*)sentbuf, 1, LEN, fp);
		if(sent_len > 0)
		{
			SendPack((short*)sentbuf, sent_len/2, 0);
		}
		else
		{
			SendLastPack();
			int len = 0;
			unsigned char* result = GetResult(&len);
			std::cout << wav_file << " " << result << std::endl;
			break;
		}

	}
	TcpClose();
	fclose(fp);
	return 0;
}
