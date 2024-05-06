#pragma once
#include "noncopyable.h"
#include <string>
#include "Timestamp.h"

// LOG_INFO("%s %d ",arg1,arg2)
#define LOG_INFO(LogmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instanceLogger();        \
        logger.setLogLever(INFO);                         \
        char buf[1024];                                   \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

// LOG_INFO("%s %d ",arg1,arg2)
#define LOG_ERROR(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instanceLogger();        \
        logger.setLogLever(ERR);                          \
        char buf[1024];                                   \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

// LOG_INFO("%s %d ",arg1,arg2)
#define LOG_FATAL(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instanceLogger();        \
        logger.setLogLever(FATAL);                        \
        char buf[1024];                                   \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instanceLogger();        \
        logger.setLogLever(DEBUG);                        \
        char buf[1024];                                   \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(LogmsgFormat, ...)
#endif

// 日志级别 INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO,  // 普通信息
    ERR,   // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类

class Logger : noncopyable
{
private:
    /* data */
public:
    /**
     * @brief 返回日志唯一的实例对象
     */
    static Logger &instanceLogger();

    /**
     * @brief 设置日志级别
     */
    void setLogLever(int level);

    /**
     * @brief 写日志
     */
    void log(std::string msg);

private:
    int LogLevel_;
};
