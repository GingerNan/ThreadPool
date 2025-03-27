#include "ThreadPool.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>

ThreadPool::ThreadPool(int min, int max)
{
	// ʵ�����������
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
		m_liveNum = min;	// ����С�������
		m_exitNum = 0;

		if (pthread_mutex_init(&m_lock, nullptr) != 0 ||
			pthread_cond_init(&m_notEmpty, nullptr) != 0)
		{
			std::cout << "mutex or condition init fail ..." << std::endl;
			break;
		}

		m_shutdown = false;

		// �����߳�
		pthread_create(&m_managerID, nullptr, manager, this);
		for (int i = 0; i < min; ++i)
		{
			pthread_create(&m_threadIDs[i], nullptr, worker, this);
		}
		//return;
	} while (0);

	// �ͷ���Դ
	//if (threadIDs) delete[] threadIDs;
	//if (taskQ) delete taskQ;
}

ThreadPool::~ThreadPool()
{
	// �ر��̳߳�
	m_shutdown = true;
	// �������չ������߳�
	pthread_join(m_managerID, NULL);
	// �����������������߳�
	for (int i = 0; i < m_liveNum; ++i)
	{
		pthread_cond_signal(&m_notEmpty);
	}

	// �ͷŶ��ڴ�
	if (m_taskQ) delete m_taskQ;
	if (m_threadIDs) delete[] m_threadIDs;

	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_notEmpty);
}

void ThreadPool::addTask(Task task)
{
	if (m_shutdown) return;

	//�������
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
		// ��ǰ��������Ƿ�Ϊ��
		while (pool->m_taskQ->taskNumber() == 0 && !pool->m_shutdown)
		{
			// ���������߳�
			pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

			// �ж��ǲ���Ҫ�����߳�
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

		// �ж��̳߳��Ƿ񱻹ر���
		if (pool->m_shutdown)
		{
			pthread_mutex_unlock(&pool->m_lock);
			pool->threadExit();
		}

		// �����������ȡ��һ������
		Task task = pool->m_taskQ->takeTask();

		// ����
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
		// ÿ��3s���һ��
		sleep(3);

		// ȡ���̳߳�������������͵�ǰ�̵߳�����
		pthread_mutex_lock(&pool->m_lock);
		int queueSize = pool->m_taskQ->taskNumber();
		int liveNum = pool->m_liveNum;
		// ȡ��æ���̵߳�����
		int busyNum = pool->m_busyNum;
		pthread_mutex_unlock(&pool->m_lock);

		const int NUMBER = 2;
		// ����߳�
		// ����ĸ��� > �����̸߳��� && �����߳��� < ����߳���
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

		// �����߳�
		// æ���߳�*2 < �����߳��� && �����߳�>��С�߳���
		if (busyNum * 2 < liveNum && liveNum > pool->m_minNum)
		{
			pthread_mutex_lock(&pool->m_lock);
			pool->m_exitNum = NUMBER;
			pthread_mutex_unlock(&pool->m_lock);

			// �ù������߳���ɱ
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
