#include "ThreadPool.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>

ThreadPool::ThreadPool(int min, int max)
{
	// 实例化任务队列
	do
	{
		m_taskQ = new TaskQueue;
		if (m_taskQ == nullptr)
		{
			std::cout << "malloc taskQ fail..." << std::endl;
			break;
		}

		m_threadIDs = new pthread_t[max];
		if (m_threadIDs == nullptr)
		{
			std::cout << "malloc threadIDs fail..." << std::endl;
			break;
		}

		memset(m_threadIDs, 0, sizeof(pthread_t) * max);
		m_minNum = min;
		m_maxNum = max;
		m_busyNum = 0;
		m_liveNum = min;	// 和最小个数相等
		m_exitNum = 0;

		if (pthread_mutex_init(&m_lock, nullptr) != 0 ||
			pthread_cond_init(&m_notEmpty, nullptr) != 0)
		{
			std::cout << "mutex or condition init fail ..." << std::endl;
			break;
		}

		m_shutdown = false;

		// 创建线程
		pthread_create(&m_managerID, nullptr, manager, this);
		for (int i = 0; i < min; ++i)
		{
			pthread_create(&m_threadIDs[i], nullptr, worker, this);
		}
		//return;
	} while (0);

	// 释放资源
	//if (threadIDs) delete[] threadIDs;
	//if (taskQ) delete taskQ;
}

ThreadPool::~ThreadPool()
{
	// 关闭线程池
	m_shutdown = true;
	// 阻塞回收管理者线程
	pthread_join(m_managerID, NULL);
	// 唤醒阻塞的消费者线程
	for (int i = 0; i < m_liveNum; ++i)
	{
		pthread_cond_signal(&m_notEmpty);
	}

	// 释放堆内存
	if (m_taskQ) delete m_taskQ;
	if (m_threadIDs) delete[] m_threadIDs;

	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_notEmpty);
}

void ThreadPool::addTask(Task task)
{
	if (m_shutdown) return;

	//添加任务
	m_taskQ->addTask(task);

	pthread_cond_signal(&m_notEmpty);
}

int ThreadPool::getBusyNum()
{
	int busyNum = 0;
	pthread_mutex_lock(&m_lock);
	busyNum = busyNum;
	pthread_mutex_unlock(&m_lock);
	return busyNum;
}

int ThreadPool::getAliveNum()
{
	int aliveNum = 0;
	pthread_mutex_lock(&m_lock);
	aliveNum = m_liveNum;
	pthread_mutex_unlock(&m_lock);
	return aliveNum;
}

void* ThreadPool::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);

	while (true)
	{
		pthread_mutex_lock(&pool->m_lock);
		// 当前任务队列是否为空
		while (pool->m_taskQ->taskNumber() == 0 && !pool->m_shutdown)
		{
			// 阻塞工作线程
			pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

			// 判断是不是要销毁线程
			if (pool->m_exitNum > 0)
			{
				pool->m_exitNum--;
				if (pool->m_liveNum > pool->m_minNum)
				{
					pool->m_liveNum--;
					pthread_mutex_unlock(&pool->m_lock);
					pool->threadExit();
				}
			}
		}

		// 判断线程池是否被关闭了
		if (pool->m_shutdown)
		{
			pthread_mutex_unlock(&pool->m_lock);
			pool->threadExit();
		}

		// 从任务队列中取出一个任务
		Task task = pool->m_taskQ->takeTask();

		// 解锁
		pool->m_busyNum++;
		pthread_mutex_unlock(&pool->m_lock);

		std::cout << "thread " << pthread_self() << "start working..." << std::endl;
		
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		std::cout << "thread " << pthread_self() << "end working..." << std::endl;
		pthread_mutex_lock(&pool->m_lock);
		pool->m_busyNum--;
		pthread_mutex_unlock(&pool->m_lock);
	}

	return nullptr;
}

void* ThreadPool::manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (pool->m_shutdown)
	{
		// 每隔3s检测一次
		sleep(3);

		// 取出线程池中任务的数量和当前线程的数量
		pthread_mutex_lock(&pool->m_lock);
		int queueSize = pool->m_taskQ->taskNumber();
		int liveNum = pool->m_liveNum;
		// 取出忙的线程的数量
		int busyNum = pool->m_busyNum;
		pthread_mutex_unlock(&pool->m_lock);

		const int NUMBER = 2;
		// 添加线程
		// 任务的个数 > 存活的线程个数 && 存活的线程数 < 最大线程数
		if (queueSize > liveNum && liveNum < pool->m_maxNum)
		{
			pthread_mutex_lock(&pool->m_lock);
			int counter = 0;
			for (int i = 0; i < pool->m_maxNum && counter < NUMBER
				&& liveNum < pool->m_maxNum; ++i)
			{
				if (pool->m_threadIDs[i] == 0)
				{
					pthread_create(&pool->m_threadIDs[i], NULL, worker, pool);
					counter++;
					pool->m_liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->m_lock);
		}

		// 销毁线程
		// 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
		if (busyNum * 2 < liveNum && liveNum > pool->m_minNum)
		{
			pthread_mutex_lock(&pool->m_lock);
			pool->m_exitNum = NUMBER;
			pthread_mutex_unlock(&pool->m_lock);

			// 让工作的线程自杀
			for (int i = 0; i < NUMBER; ++i)
			{
				pthread_cond_signal(&pool->m_notEmpty);
			}
		}
	}

	return nullptr;
}

void ThreadPool::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < m_maxNum; ++i)
	{
		if (m_threadIDs[i] == tid)
		{
			m_threadIDs[i] = 0;
			printf("threadEixt() called, %ld exiting...\n", tid);
			break;
		}
		pthread_exit(NULL);
	}
}
