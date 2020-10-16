#include <stdio.h>
#include "src/util/log-message.h"


#ifdef NAMESPACE
using namespace datemoon;
#endif
int main(int argc,char *argv[])
{
	if(argc == 2)
	{
		CreateLogHandler(argv[1]);
		LOG_COM << "it's log test code";
		LOG_WARN << "it's warning test code";
		LOG_ERR << "it's error test code";
		CloseLogHandler();
	}
	else
	{
		LOG_COM << "it's log test code";
		LOG_WARN << "it's warning test code";
		LOG_ASSERT(1);
		LOG_ERR << "it's error test code";
	}
	return 0;
}
