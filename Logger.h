#pragma once //using Header once

#include <string>

#include "noncopyable.h"

/*
宏还需要多加熟悉
方便用户使用
*/
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_ERROR(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#ifdef MUDEBUG
    #define LOG_DEBUG(logmsgFormat, ...) \
        do { \
            Logger &logger = Logger::instance(); \
            logger.setLogLevel(DEBUG); \
            char buf[1024] = {0}; \
            snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
            logger.log(buf); \
        } while(0)
#else 
    #define LOG_DEBUG(logmsgFormat, ...)
#endif
/*
定义日志的级别
INFO 正常的日志输出 跟踪核心流程
ERROR 不影响软件正常执行的错误
FATAL 影响软件正常执行的毁灭性打击
DEBUG 调制信息
*/  

enum LogLevel {
    INFO, // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger : noncopyable {
public:
    // 获取日志唯一的实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_;
    // 日志实例私有化
    Logger(){}
};