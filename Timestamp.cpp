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

Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}

std::string Timestamp::toString() const
{
    struct tm *tm_time = localtime(&microSecondsSinceEpoch_);
    char buf[128] = "";
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    // printf("%s \n", buf);
    return buf;
}