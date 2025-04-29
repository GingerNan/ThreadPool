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
* 构成：
* 1.管理线程 -> 子线程，1个
*		-- 控制工作线程的数量：增加或减少
* 2.若干工作线程 -> 子线程，n个
*		-- 从任务队列中取任务，并处理
*		-- 任务队列为空，被阻塞（被条件变量阻塞）
*		-- 线程同步（互斥锁）
*		-- 当前数量，空闲的线程数量
*		-- 最小，最大线程数量
* 3.任务队列 -> stl->queue
*		-- 互斥锁
*		-- 条件变量
* 4.线程池开关 -> bool
*/

class ThreadPool
{
public:
	ThreadPool(int min = 2, int max = std::thread::hardware_concurrency());
	~ThreadPool();

	// 添加任务 -> 任务队列
	void addTask(std::function<void(void)> task);

	template<typename F, typename... Args>
	auto addTask(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		// 1. package_task
		using returnType = typename std::result_of<F(Args...)>::type;
		auto mytask = std::make_shared<std::packaged_task<returnType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		// 2. 得到future
		std::future<returnType> res = mytask->get_future();
		// 3. 任务函数添加到任务队列
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
	std::vector<std::thread::id> m_ids;		// 存储已经推出了任务函数的线程的ID

	std::atomic<int> m_minThread;
	std::atomic<int> m_maxThread;
	std::atomic<int> m_curThread;
	std::atomic<int> m_idleThread;		// 空闲线程的数量
	std::atomic<int> m_exitThread;

	std::atomic<bool> m_stop;

	std::queue<std::function<void(void)>> m_task;
	std::mutex m_queueMutex;
	std::mutex m_idsMutex;
	std::condition_variable m_condition;
};
