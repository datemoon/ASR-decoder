

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#include "src/service2/net-data-package.h"

#ifdef NAMESPACE
using namespace datemoon;
#endif

#define LEN 4096

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		LOG_WARN << argv[0] << " port wavfile";
		return -1;
	}
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	C2SPackageAnalysis c2s_cli;
	S2CPackageAnalysis s2c_cli;
	struct sockaddr_in ser;
	memset(&ser, 0x00, sizeof(ser));
	ser.sin_family = AF_INET;
	inet_aton("127.0.0.1",&ser.sin_addr);
	ser.sin_port = htons(atoi(argv[1]));

	char *wav_file = argv[2];
	int res = connect(sockfd, (struct sockaddr *) &ser, sizeof(ser));
	if(res < 0)
	{
		LOG_WARN << "connect error!!!";
		return -1;
	}
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
			if(true != c2s_cli.C2SWrite(sockfd, sentbuf, sent_len, 0))
			{
				 LOG_WARN << "C2SWrite failed!!!";
				 return -1;
			}
			c2s_cli.Print("c2s_cli");
		}
		else
		{
			if(true != c2s_cli.C2SWrite(sockfd, sentbuf, sent_len, 1))
			{
				 LOG_WARN << "C2SWrite end failed!!!";
				 return -1;
			}
			c2s_cli.Print("c2s_cli");

			if(true != s2c_cli.S2CRead(sockfd))
			{
				LOG_WARN << "S2CRead failed!!!";
				return false;
			}
			s2c_cli.Print("s2c_cli");
			break;
		}

		if(true != s2c_cli.S2CRead(sockfd))
		{
			LOG_WARN << "S2CRead failed!!!";
			return false;
		}
		s2c_cli.Print("s2c_cli");
	}
	close(sockfd);
	fclose(fp);
	return 0;
}
