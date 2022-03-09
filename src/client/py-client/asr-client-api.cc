#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#include "src/service2/net-data-package.h"
#include "src/client/py-client/asr-client-api.h"

int sockfd = -1;

datemoon::C2SPackageAnalysis c2s_cli;

void SetAlign()
{
	c2s_cli.SetAliInfo(1);
}

int TcpConnect(const char* ip_addr, int port)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in ser;
	memset(&ser, 0x00, sizeof(ser));
	ser.sin_family = AF_INET;
	inet_aton(ip_addr, &ser.sin_addr);
	ser.sin_port = htons(port);
	int ret = connect(sockfd, (struct sockaddr *) &ser, sizeof(ser));
	if(ret != 0)
	{
		LOG_WARN << "TcpConnect connect failed!!!";
		return -1;
	}
	c2s_cli.Reset(true);
	return 0;
}


void TcpClose()
{
	close(sockfd);
}

void SendPack(short* packet_data, int sample_num, int flag)
{
	c2s_cli.SetNbest(0);
	int sent_len = sample_num * 2;
	char * sentbuf = (char *)packet_data;
	if(true != c2s_cli.C2SWrite(sockfd, sentbuf, sent_len, flag))
	{
		LOG_WARN << "C2SWrite failed!!!";
		return;
	}
	c2s_cli.Print("c2s_cli");
}
void SendLastPack()
{
	SendPack(NULL,0,1);
//	if(true != c2s_cli.C2SWrite(sockfd, NULL, 0, 1))
//	{
//		LOG_WARN << "C2SWrite end failed.";
//		return;
//	}
}

unsigned char* GetResult(int *len)
{
	datemoon::S2CPackageAnalysis s2c_cli;
	*len = 0;
	if(true != s2c_cli.S2CRead(sockfd))
	{
		LOG_WARN << "S2CRead failed!!!";
		return NULL;
	}
	std::vector<std::string> nbest;
	s2c_cli.GetData(&nbest);

	if(nbest.size() == 0)
	{
		*len = 0;
	}
	else
		*len = nbest[0].size();
	char* result = new char[*len + 1];
	//*len = *len+1;
	if(nbest.size() != 0)
		strncpy(result, nbest[0].c_str(), *len);
	result[*len] = '\0';
	//printf("%s %d\n",result,*len);
	return (unsigned char*)result;
}

void* PthreadRecv(void *socked_id)
{
	return NULL;
}

std::string RecvResult(int socked_id)
{
	datemoon::S2CPackageAnalysis s2c_cli;
	std::string all_result;
	while(true)
	{
		if(true != s2c_cli.S2CRead(socked_id))
		{
			LOG_WARN << "S2CRead failed!!! sockfd is " << sockfd;
			return all_result;
		}
		if( s2c_cli.IsMiddleEnd() == true || true == s2c_cli.IsAllEnd())
		{
			std::vector<std::string> nbest;
			s2c_cli.GetData(&nbest);
			if(nbest.size() == 0)
				all_result.append("");
			else
				all_result.append(nbest[0]);
			const datemoon::AlignTime &align = s2c_cli.GetAlign();
			//std::cout << "align is :" ;
			//datemoon::PrintContainerType(std::cout, align);
			std::ostringstream oss;
			datemoon::PrintContainerType(oss, align);
			ssize_t ali_len = oss.str().size();
			if(ali_len > 0)
				all_result.append(oss.str());

			if(true == s2c_cli.IsAllEnd())
			{
				return all_result;
			}
		}
	}
}

unsigned char* RecvResult(int *len)
{
	std::string res = RecvResult(sockfd);
	*len = res.size();
	char* result = new char[*len + 1];
	if(res.size() != 0)
		strncpy(result, res.c_str(), *len);
	result[*len] = '\0';
	return (unsigned char*)result;
}
