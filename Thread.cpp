#include "Thread.h"
#include <semaphore.h>
#include "CurrentThread.h"

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      thread_(nullptr),
      tid_(0),
      func_(std::move(func)),
      name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // 分离线程
    }
}

/**
 * @brief 创建线程数加一。若线程名为空、设置默认线程名，格式为Thread+num。
 */
void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

/**
 * @brief 启动线程。
 */
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, 0, 0);
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                           {
    tid_ = CurrentThread::tid();
    sem_post(&sem);//执行信号量P操作、信号量加一
    func_(); })); // 执行func_函数

    // 这里必须等待线程创建完成,否则可能会出现线程还未创建完成,主线程就开始执行join函数,导致线程还未创建完成,就开始执行join函数
    //  不能判定tid>0判定是否创建完成,因为线程的调度start函数可能在thread_创建之前就执行了,所以使用信号量等待线程创建完成
    sem_wait(&sem); // 执行信号量V操作、信号量减一,若信号量为0则阻塞（即前面的P操作还没执行）
}

void Thread::join()
{
    joined_ = true;
    // 等待线程结束
    thread_->join();
}
