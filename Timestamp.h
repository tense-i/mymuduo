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
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    static const int kMicroSecondsPerSecond = 1000 * 1000; // 将秒转换为微秒
    static Timestamp now();
    std::string toString() const;
    bool valid() const; // 判断时间戳是否有效
    static Timestamp invalid()
    {
        return Timestamp();
    }
};

/**
 * @brief 时间戳加上seconds秒
 *
 * @secounds  延迟的秒数
 */
inline Timestamp addTime(Timestamp timestap, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);

    return Timestamp(delta + timestap.microSecondsSinceEpoch());
}

inline bool operator<(const Timestamp &lhs, const Timestamp &rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}
