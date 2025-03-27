#pragma once
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <map>
#include <future>
#include <memory>

/*
* ���ɣ�
* 1.�����߳� -> ���̣߳�1��
*		-- ���ƹ����̵߳����������ӻ����
* 2.���ɹ����߳� -> ���̣߳�n��
*		-- �����������ȡ���񣬲�����
*		-- �������Ϊ�գ�������������������������
*		-- �߳�ͬ������������
*		-- ��ǰ���������е��߳�����
*		-- ��С������߳�����
* 3.������� -> stl->queue
*		-- ������
*		-- ��������
* 4.�̳߳ؿ��� -> bool
*/

class ThreadPool
{
public:
	ThreadPool(int min = 2, int max = std::thread::hardware_concurrency());
	~ThreadPool();

	// ������� -> �������
	void addTask(std::function<void(void)> task);

	template<typename F, typename... Args>
	auto addTask(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		// 1. package_task
		using returnType = typename std::result_of<F(Args...)>::type;
		auto mytask = std::make_shared<std::packaged_task<returnType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		// 2. �õ�future
		std::future<returnType> res = mytask->get_future();
		// 3. ��������ӵ��������
		m_queueMutex.lock();
		m_task.emplace([mytask]() {
			(*mytask)();
			});
		m_queueMutex.unlock();
		m_condition.notify_one();

		return res;
	}

private:
	void manager();
	void worker();
private:
	std::thread* m_manage;
	std::map<std::thread::id, std::thread> m_workers;
	std::vector<std::thread::id> m_ids;		// �洢�Ѿ��Ƴ������������̵߳�ID

	std::atomic<int> m_minThread;
	std::atomic<int> m_maxThread;
	std::atomic<int> m_curThread;
	std::atomic<int> m_idleThread;		// �����̵߳�����
	std::atomic<int> m_exitThread;

	std::atomic<bool> m_stop;

	std::queue<std::function<void(void)>> m_task;
	std::mutex m_queueMutex;
	std::mutex m_idsMutex;
	std::condition_variable m_condition;
};
