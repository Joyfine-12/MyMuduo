#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
    {}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_ -> quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startloop() {
    // 启动底层的一个新的线程
    thread_.start(); 

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ ==nullptr) {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 下面这个方法实在单独的新线程里面运行的
void EventLoopThread::threadFunc() {
    //  创建一个独立的eventloop， 和上面的新线程一一对应
    EventLoop loop;

    //执行回调函数（如果有）
    if (callback_) {
        callback_(&loop);
    }

    /*
    使用互斥锁 mutex_ 保护共享数据 loop_，将 loop_ 指向当前线程的 EventLoop 对象。
    通过条件变量 cond_ 通知主线程，EventLoop 已经创建完成。
    主线程可以继续执行，知道新线程的 EventLoop 已经准备好。
    */
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop => Poller.poll
    /*
    当事件循环退出后，获取互斥锁 mutex_，将 loop_ 设置为 nullptr，表示当前线程的 EventLoop 已经销毁。
    这一步确保其他线程不会访问到已经销毁的 EventLoop 对象。
    */
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
