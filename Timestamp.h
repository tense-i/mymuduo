#pragma once
#include <stdint.h>
#include <string>

/**
 * @brief 时间戳类
 */
class Timestamp
{
private:
    int64_t microSecondsSinceEpoch_; // 微秒

public:
    Timestamp();
    explicit Timestamp(int64_t timeSinceEpoch);
    ~Timestamp();
    static Timestamp now();
    std::string toString() const;
};
