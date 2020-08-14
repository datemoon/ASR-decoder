

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in ser, cli;
	memset(&ser, 0x00, sizeof(ser));
	ser.sin_family = AF_INET;
	inet_aton("127.0.0.1",&ser.sin_addr);
	ser.sin_port = htons(atoi(argv[1]));

	char *wav_file = argv[2];
	int res = connect(sockfd, (struct sockaddr *) &ser, sizeof(ser));
	FILE *fp = fopen(wav_file, "r");
	while(true)
	{
		char sentbuf[1024];
		char recvbuf[1024];
		memset(sentbuf,0x00,sizeof(sentbuf));
		memset(recvbuf,0x00,sizeof(recvbuf));
		size_t sent_len = fread((void*)sentbuf, 1, 1024, fp);
		if(sent_len > 0)
		{
			send(sockfd, sentbuf, sent_len, 0);
		}
		else
		{
			send(sockfd, sentbuf, 0, 0);
			break;
		}
	}
	close(sockfd);
	fclose(fp);
	return 0;
}
