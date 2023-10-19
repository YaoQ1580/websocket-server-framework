#ifndef _Timestamp_hpp_
#define _Timestamp_hpp_

//#include <windows.h>
#include<chrono>
using namespace std::chrono;

class Time
{
public:
	//retrieve current computer time
	static int64_t getNowInMilliSec()
	{
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	//retrieve current time
	static int64_t system_clock_now()
	{
		return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}
};

//a simple timer
class Timestamp
{
public:
	Timestamp()
	{
		update();
	}

	~Timestamp()
	{}

	void update()
	{
		_begin = high_resolution_clock::now();
	}
	
	//get second
	double getElapsedSecond()
	{
		return  getElapsedTimeInMicroSec() * 0.000001;
	}

	//get milliecond
	double getElapsedTimeInMilliSec()
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}

	//get microsecond
	int64_t getElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	time_point<high_resolution_clock> _begin;
};

#endif