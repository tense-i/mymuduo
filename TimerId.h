#pragma once
#include <cstdint> // Add the missing include directive

class Timer;

class TimerId
{
private:
    Timer *timer_;     // 定时器指针
    int64_t sequence_; // 序列号
public:
    friend class TimerQueue;
    TimerId()
        : timer_(nullptr),
          sequence_(0){

          };

    TimerId(Timer *timer, int64_t seq)
        : timer_(timer),
          sequence_(seq){};
    ~TimerId();
};
