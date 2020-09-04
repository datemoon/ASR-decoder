#ifndef __DATEMOON_TIME_H__
#define __DATEMOON_TIME_H__

#include <sys/time.h>

#include "src/util/namespace-start.h"

class Time
{
private:
	struct timeval _tv_start;
	struct timezone _tz;
public:
	Time()
	{
		Reset();
	}

	void Reset()
	{
		gettimeofday(&_tv_start, &_tz);
	}
	double Esapsed()
	{
		struct timeval tv_end;
		struct timezone tz;
		gettimeofday(&tv_end, &tz);
		double start_t = _tv_start.tv_sec +
		   	static_cast<double>(_tv_start.tv_usec)/(1000*1000);
		double end_t = tv_end.tv_sec + 
			static_cast<double>(tv_end.tv_usec)/(1000*1000);
		Reset();
		return end_t - start_t;
	}
};

#include "src/util/namespace-end.h"

#endif
