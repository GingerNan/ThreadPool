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

	// �������
	void addTask(Task task);
	void addTask(Callback f, void* arg);

	// ȡ��һ������
	Task takeTask();

	// ��ȡ��ǰ����ĸ���
	inline int taskNumber()
	{
		return m_taskQ.size();
	}

private:
	pthread_mutex_t m_mutex;	// ������
	std::queue<Task> m_taskQ;	// �������
};