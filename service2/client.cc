

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

	int res = connect(sockfd, (struct sockaddr *) &ser, sizeof(ser));

	while(true)
	{
		printf("please input:");
		fflush(stdout);
		char buff[128] = {0};
		char recvbuf[1024];
		memset(recvbuf,0x00,sizeof(recvbuf));
		memset(buff,0x00, 128);
		fgets(buff,128, stdin);

		if(strncmp(buff,"end",3) == 0)
		{
			// close(sockfd);
			break;
		}
		if(strlen(buff)-1 <=0)
			continue;
		send(sockfd, buff, strlen(buff)-1,0);
		int len = recv(sockfd, recvbuf, sizeof(recvbuf), 0 );
		if(len<=0)
		{
			printf("close...\n");
			break;
		}
		//recv(sockfd, buff, sizeof(buff), 0 );
		printf("recv %s\n",recvbuf);
	}
	close(sockfd);
	return 0;
}
