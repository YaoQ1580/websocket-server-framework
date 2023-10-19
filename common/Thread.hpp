#ifndef _THREAD_HPP_
#define _THREAD_HPP_

#include"Semaphore.hpp"
#include<functional>

//a simple C++ thread wrapper
class Thread
{
public:
	static void Sleep(time_t dt)
	{
		std::chrono::milliseconds t(dt);
		std::this_thread::sleep_for(t);
	}
private:
	typedef std::function<void(Thread*)> EventCall;
public:
	//start a thread
	void Start(
		EventCall onCreate = nullptr,
		EventCall onRun = nullptr,
		EventCall onDestory = nullptr)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_isRun)
		{
			_isRun = true;

			if (onCreate)
				_onCreate = onCreate;
			if (onRun)
				_onRun = onRun;
			if (onDestory)
				_onDestory = onDestory;

			std::thread t(std::mem_fn(&Thread::OnWork), this);
			t.detach();
		}
	}

	void Close()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
			_sem.wait();
		}
	}

	void Exit()
	{
		if (_isRun)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_isRun = false;
		}
	}

	bool isRun()
	{
		return _isRun;
	}
protected:
	//thread loop func
	void OnWork()
	{
		if (_onCreate)
			_onCreate(this);
		if (_onRun)
			_onRun(this);
		if (_onDestory)
			_onDestory(this);

		_sem.wakeup();
		_isRun = false;
	}
private:
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onDestory;
	std::mutex _mutex;
	//to make sure the working thread has finished working when main thread exits
	Semaphore _sem;
	bool	_isRun = false;
};

#endif
