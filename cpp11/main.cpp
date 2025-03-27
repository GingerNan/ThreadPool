#include <iostream>
#include "ThreadPool.hpp"

void calc(int x, int y)
{
    int z = x + y;
    std::cout << "z = " << z << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int calc1(int x, int y)
{
    int z = x + y;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return z;
}

int main()
{
    /*同步
    ThreadPool pool;
    for (int i = 0; i < 10; ++i)
    {
        auto obj = std::bind(calc, i, i * 2);
        pool.addTask(obj);
    }
    */

    /* 异步 */
    ThreadPool pool;
    std::vector<std::future<int>> results;
    for (int i = 0; i < 10; ++i)
    {
        results.emplace_back(pool.addTask(calc1, i, i * 2));
    }

    for (auto& item : results)
    {
        std::cout << "线程执行的结果：" << item.get() << std::endl;
    }
    return 0;
}
