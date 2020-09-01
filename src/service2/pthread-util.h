#ifndef __PTHREAD_UTIL_H__
#define __PTHREAD_UTIL_H__

#include <pthread.h>
#include <signal.h>

#include "src/util/namespace-start.h"
// Block SIGPIPE before starting any other threads; other threads
// created by main() will inherit a copy of the signal mask.
// If you try using singal(SIGPIPE, SIG_IGN), it will be ignored by the thread
// because signal is undefined with threads.
void ThreadSigPipeIng();
#include "src/util/namespace-end.h"

#endif
