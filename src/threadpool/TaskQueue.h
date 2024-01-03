#ifndef __MY_TASKQUEUE_H_
#define __MY_TASKQUEUE_H_
#include "Condition.h"
#include "MutexLock.h"
#include <queue>
#include <functional>
using std::function;

namespace wd
{
    // 任务是需要执行的函数
    using Task = function<void()>;
    using ElemType = Task;

    class TaskQueue
    {
    public:
        TaskQueue(size_t queSize = 10);
        ~TaskQueue();
        bool full() const;
        bool empty() const;

        // 任务入队列
        void push(const ElemType&);
        // 取出一个任务
        ElemType pop();
        // 唤醒所有等待任务的线程,让它们退出
        void wakeup();

    private:
        std::queue<ElemType> _que;
        size_t _queSize;
        MutexLock _mutexlock;
        Condition _fullAndWait;
        Condition _emptyAndWait;
        bool _flag;
    };


} // namespace wd

#endif