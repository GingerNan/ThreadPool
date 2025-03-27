#include "ThreadPool.hpp"

ThreadPool::ThreadPool(int min, int max) 
	: m_maxThread(max), m_minThread(min),
	m_idleThread(min), m_curThread(min), 
	m_stop(false)
{
	std::cout << "max = " << max << std::endl;
	// 创建管理这线程
	m_manage = new std::thread(&ThreadPool::manager, this);
	// 工作的线程
	for (int i = 0; i < min; ++i)
	{
		std::thread t(&ThreadPool::worker, this);
		m_workers.insert(std::make_pair(t.get_id(), std::move(t)));
	}
}

ThreadPool::~ThreadPool()
{
	m_stop = true;
	m_condition.notify_all();
	for (auto& it : m_workers)
	{
		std::thread& t = it.second;
		if (t.joinable())
		{
			std::cout << "*********** 线程 " << t.get_id() << " 将要退出" << std::endl;
			t.join();
		}
	}

	if (m_manage->joinable())
	{
		m_manage->join();
	}
	delete m_manage;
}

void ThreadPool::addTask(std::function<void(void)> task)
{
	{
		std::lock_guard<std::mutex> locker(m_queueMutex);
		m_task.emplace(task);
	}
	m_condition.notify_one();
}

void ThreadPool::manager()
{
	while (!m_stop.load())
	{
		std::this_thread::sleep_for(std::chrono::seconds(3));
		int idel = m_idleThread.load();
		int cur = m_idleThread.load();
		if (idel > cur / 2 && cur > m_minThread)
		{
			// 每次销毁两个线程
			m_exitThread.store(2);
			m_condition.notify_all();

			std::lock_guard<std::mutex> lck(m_idsMutex);
			for (auto id : m_ids)
			{
				auto it = m_workers.find(id);
				if (it != m_workers.end())
				{
					std::cout << "============= 线程 " << (*it).first << "被销毁了" << std::endl;
					(*it).second.join();
					m_workers.erase(it);
				}
			}
			m_ids.clear();
		}
		else if (idel == 0 && cur < m_maxThread)
		{
			std::thread t(&ThreadPool::worker, this);
			m_workers.insert(std::make_pair(t.get_id(), std::move(t)));
			m_curThread++;
			m_idleThread++;
		}
	}
}

void ThreadPool::worker()
{
	while (!m_stop.load())
	{
		std::function<void(void)> task = nullptr;
		{
			std::unique_lock<std::mutex> locker(m_queueMutex);
			while (m_task.empty() && !m_stop)
			{
				m_condition.wait(locker);
				if (m_exitThread.load() > 0)
				{
					m_curThread--;
					m_idleThread--;
					m_exitThread--;
					std::cout << "-----------线程退出，ID: " << std::this_thread::get_id() << std::endl;
					std::lock_guard<std::mutex> lck(m_idsMutex);
					m_ids.push_back(std::this_thread::get_id());
					return;
				}
			}
			
			if (!m_task.empty())
			{
				std::cout << "取出一个任务..." << std::endl;
				task = std::move(m_task.front());
				m_task.pop();
			}
		}

		if (task)
		{
			m_idleThread--;
			task();
			m_idleThread++;
		}

	}
}
