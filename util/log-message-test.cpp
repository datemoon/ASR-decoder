#include <stdio.h>
#include "util/log-message.h"

int main(int argc,char *argv[])
{
	if(argc == 2)
	{
		CreateLogHandler(argv[1]);
		LOG << "it's log test code";
		LOG_WARN << "it's warning test code";
		LOG_ERR << "it's error test code";
		CloseLogHandler();
	}
	else
	{
		LOG << "it's log test code";
		LOG_WARN << "it's warning test code";
		LOG_ASSERT(1);
		LOG_ERR << "it's error test code";
	}
	return 0;
}
