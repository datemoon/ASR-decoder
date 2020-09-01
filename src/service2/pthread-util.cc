
#include "src/service2/pthread-util.h"

#include "src/util/namespace-start.h"
void ThreadSigPipeIng()
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}
#include "src/util/namespace-end.h"
