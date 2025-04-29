#pragma once
#include "TaskQueue.h"

class ThreadPool
{
public:
    ThreadPool(int min, int max);
    ~ThreadPool();

    // ���̳߳��������
    void addTask(Task task);

    // ��ȡ�̳߳��й������̵߳ĸ���
    int getBusyNum();

    // ��ȡ�̳߳��л��ŵ��̵߳ĸ���
    int getAliveNum();

private:
    // �������߳�(�������߳�)������
    static void* worker(void* arg);
    // �������߳�������
    static void* manager(void* arg);
    // �����߳��˳�
    void threadExit();

private:
    // �������
    TaskQueue* m_taskQ;

    pthread_t m_managerID;          // �������߳�ID
    pthread_t* m_threadIDs;         // �������߳�ID
    int m_minNum;                   // ��С�߳�����
    int m_maxNum;                   // ����߳�����
    int m_busyNum;                  // æ���̵߳ĸ���
    int m_liveNum;                  // �����̵߳ĸ���
    int m_exitNum;                  // Ҫ���ٵ��̸߳���
    pthread_mutex_t m_lock;         // ���������̳߳�
    pthread_cond_t m_notEmpty;      // ��������ǲ��ǿ���

    bool m_shutdown;                // �ǲ���Ҫ�����̳߳�, ����Ϊ1, ������Ϊ0
};

