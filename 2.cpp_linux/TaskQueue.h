#pragma once
#include <queue>
#include <pthread.h>

using Callback = void (*)(void* arg);
struct Task
{
	Task()
	{
		function = nullptr;
		arg = nullptr;
	}

	Task(Callback f, void* arg)
	{
		this->arg = arg;
		function = f;
	}

	Callback function;
	void* arg;
};

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	// 添加任务
	void addTask(Task task);
	void addTask(Callback f, void* arg);

	// 取出一个任务
	Task takeTask();

	// 获取当前任务的个数
	inline int taskNumber()
	{
		return m_taskQ.size();
	}

private:
	pthread_mutex_t m_mutex;	// 互斥锁
	std::queue<Task> m_taskQ;	// 任务队列
};