
#include <pthread.h>
#include <stdlib.h>
#include <cassert>
#include <iostream>
#include "src/pitch/kaldi-common.h"


static pthread_mutex_t _RandMutex = PTHREAD_MUTEX_INITIALIZER;

RandomState::RandomState() {
	seed = Rand() + 27437;
}

int Rand(struct RandomState* state){
	if (state) {
#ifdef __ANDROID_API__
		return rand();
#else
		return rand_r(&(state->seed));
#endif
	} else {
		int rs = pthread_mutex_lock(&_RandMutex);
		assert(rs == 0);
		int val = rand();
//		std::cout << val << std::endl;
		rs = pthread_mutex_unlock(&_RandMutex);
		assert(rs == 0);
		return val;
	}
}
double Exp(double x) { return exp(x);}

float Exp(float x) { return expf(x);}

double Log(double x) { return log(x);}

float Log(float x) { return logf(x);}

