#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "ThreadPool.h"

void taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("[thead %ld] is working, number = %d\n", pthread_self(), num);
    usleep(800);
}

int main()
{
    // 创建线程池
    ThreadPool* pool = new ThreadPool(3, 10);
    for (int i = 0; i < 100; ++i)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        pool->addTask(Task(taskFunc, num));
    }
    sleep(30);
    return 0;
}