#include "Logger.h"
#include <iostream>

Logger &Logger::instanceLogger()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLever(int level)
{
    LogLevel_ = level;
}

void Logger::log(std::string msg)
{
    switch (LogLevel_)
    {
    case LogLevel::INFO:
        printf("[INFO]");
        break;
    case ERR:
        printf("[ERROR]");
        break;
    case FATAL:
        printf("[FATAL]");
        break;
    case DEBUG:
        printf("[DEBUG]");
        break;
    default:
        break;
    }
    printf("%s : %s\n", Timestamp::now().toString().c_str(), msg.c_str());
}
