#ifndef __ASR_CLIENT_API_H__
#define __ASR_CLIENT_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

// 需要依赖 libclient.so libservice2.so libutil.so
int TcpConnect(const char* ip_addr, int port);

void TcpClose();

void SendPack(short* packet_data, int sample_num, int flag);

void SendLastPack();

unsigned char* GetResult(int *len);

void* PthreadRecv(void *socket_id);

#ifdef __cplusplus
}
#endif

#endif
