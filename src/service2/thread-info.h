#ifndef __THREAD_INFO_H__
#define __THREAD_INFO_H__

#include <iostream>

#include "src/util/namespace-start.h"

class ThreadTimeInfo
{
protected:
	// data time and work time and for calculate efficiency
	double _data_time;
	double _work_time;
	double _effect_data_time;
public:
	ThreadTimeInfo(): _data_time(0.0), _work_time(0.0), _effect_data_time(0.0) { }
	virtual ~ThreadTimeInfo() { }
	virtual double GetDataTime() { return _data_time;}
	virtual double GetWorkTime() { return _work_time;}
	virtual double GetInvalidDataTime() { return _effect_data_time;}
	virtual void SetTime(double data_time, double work_time, double effect_data_time = -1)
	{
		_data_time += data_time;
		if(effect_data_time < 0)
			effect_data_time = data_time;
		_work_time += work_time;
		_effect_data_time += effect_data_time;
	}
	virtual double GetEfficiency() 
	{
		if(_data_time == 0)
			return -1;
		return _work_time/_data_time;
   	}
	virtual void TimeInfo()
	{
		std::cout << "Data time is : " << _data_time << std::endl;
		std::cout << "effect data time is : "  << _effect_data_time << std::endl;
		std::cout << "Wrok time is : " << _work_time << std::endl;
	}
};
#include "src/util/namespace-end.h"
#endif
