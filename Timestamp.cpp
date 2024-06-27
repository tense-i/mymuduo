#include "Timestamp.h"
#include <time.h>

Timestamp::Timestamp(/* args */) : microSecondsSinceEpoch_(0)
{
}

inline Timestamp::Timestamp(int64_t timeSinceEpoch) : microSecondsSinceEpoch_(timeSinceEpoch)
{
}

Timestamp::~Timestamp()
{
}

/**
 * @brief 获取当前时间戳
 * @return 返回的是TimeStamp的拷贝，不是引用
 */
Timestamp Timestamp::now()
{
    // time()函数返回的是从1970年1月1日0时0分0秒到现在的秒数
    return Timestamp(time(NULL));
}

/**
 * @brief 将时间戳转换为字符串、格式为"年/月/日 时:分:秒"
 */
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900, // 年份是从1900开始的
             tm_time->tm_mon + 1,     // 月份是从0开始的
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}
