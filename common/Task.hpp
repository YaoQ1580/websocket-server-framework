#ifndef _TASK_H_
#define _TASK_H_

#include<thread>
#include<mutex>
#include<list>

#include<functional>

#include"Thread.hpp"

//a simple task queue with a worker thread
class TaskServer
{
public:
	int serverId = -1;
private:
	typedef std::function<void()> Task;
private:
	std::list<Task> _tasks;
	std::list<Task> _tasksBuf;
	std::mutex _mutex;
	Thread _thread;
public:
	void addTask(Task task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	void Start()
	{
		_thread.Start(nullptr, [this](Thread* pThread) {
			OnRun(pThread);
		});
	}

	void Close()
	{
		///CELLLog_Info("TaskServer%d.Close begin", serverId);
		_thread.Close();
		//CELLLog_Info("TaskServer%d.Close end", serverId);
	}
protected:
	//main loop of the working thread, should be an infinite function
	void OnRun(Thread* pThread)
	{
		while (pThread->isRun())
		{
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}

			if (_tasks.empty())
			{
				Thread::Sleep(1);
				continue;
			}

			for (auto pTask : _tasks)
			{
				pTask();
			}

			_tasks.clear();
		}

		for (auto pTask : _tasksBuf)
		{
			pTask();
		}
		//CELLLog_Info("TaskServer%d.OnRun exit", serverId);
	}
};

#endif
