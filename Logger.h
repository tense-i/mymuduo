#pragma once
#include "noncopyable.h"
#include <string>
#include "Timestamp.h"

/*
日志模块采用宏函数的方式，方便调用
do-while(0)的作用是为了防止宏函数在使用时出现问题--->宏函数的使用时机是在编译时期，而不是运行时期
*/
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

/**
 * 条件编译，如果定义了MUDEBUG，则打印调试信息
 */
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

// 日志级别
enum LogLevel
{
    INFO,  // 普通信息
    ERR,   // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

/**
 * @brief 日志工具类

*/
class Logger : noncopyable
{
private:
    Logger() : LogLevel_(INFO) {}

public:
    /**
     * @brief 单例设计模式、返回日志唯一的实例对象
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
