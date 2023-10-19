#ifndef _SEMAPHORE_HPP_
#define _SEMAPHORE_HPP_

#include<chrono>
#include<thread>

#include<condition_variable>

// just a simple wrapper of condition variable
class Semaphore
{
public:
	void wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (--_wait < 0)
		{
			_cv.wait(lock, [this]()->bool {
				return _wakeup > 0;
			});
			--_wakeup;
		}
	}

	void wakeup()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (++_wait <= 0)
		{
			++_wakeup;
			_cv.notify_one();
		}
	}

private:
	std::mutex _mutex;
	std::condition_variable _cv;
	int _wait = 0;
	int _wakeup = 0;
};

#endif