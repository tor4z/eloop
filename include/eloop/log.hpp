#pragma once


namespace eloop
{

#define LOG_LEVEL_TRACE  0
#define LOG_LEVEL_DEBUG  1
#define LOG_LEVEL_INFO   2
#define LOG_LEVEL_WARN   3
#define LOG_LEVEL_ERROR  4
#define LOG_LEVEL_FATAL  5

extern int logLevel;

// public
void setLogLevel(int level);
void setLogFD(int fd);

// private
void log_base_(
    const char *file,
    int line,
    int level,
    bool to_abort,
    const char *fmt,
    ...
);

void log_sys_(
    const char *file,
    int line,
    bool to_abort,
    const char *fmt,
    ...
);

// private
#define LOG_BASE_(level, to_abort, fmt, ...)\
    log_base_(__FILE__, __LINE__, level, to_abort, fmt, ##__VA_ARGS__)
#define LOG_SYS_(to_abort, fmt, ...)\
    log_sys_(__FILE__, __LINE__, to_abort, fmt, ##__VA_ARGS__);

// public
#define TRACE(fmt, ...)                                        \
do {                                                           \
    if(logLevel < LOG_LEVEL_TRACE)                             \
        LOG_BASE_(LOG_LEVEL_TRACE, false, fmt, ##__VA_ARGS__); \
} while (false)

#define DEBUG(fmt, ...)                                        \
do {                                                           \
    if(logLevel < LOG_LEVEL_DEBUG)                             \
        LOG_BASE_(LOG_LEVEL_DEBUG, false, fmt, ##__VA_ARGS__); \
} while(false)

#define INFO(fmt, ...)                                        \
do {                                                          \
    if(logLevel < LOG_LEVEL_INFO)                             \
        LOG_BASE_(LOG_LEVEL_INFO, false, fmt, ##__VA_ARGS__); \
} while(false)

#define WARN(fmt, ...)                                        \
do {                                                          \
    if(logLevel < LOG_LEVEL_WARN)                             \
        LOG_BASE_(LOG_LEVEL_WARN, false, fmt, ##__VA_ARGS__); \
} while(false)

#define ERROR(fmt, ...) LOG_BASE_(LOG_LEVEL_ERROR, false, fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) LOG_BASE_(LOG_LEVEL_FATAL, false, fmt, ##__VA_ARGS__)
#define SYSERR(fmt, ...) LOG_SYS_(false, fmt, ##__VA_ARGS__)
#define SYSFATAL(fmt, ...) LOG_SYS_(true, fmt, ##__VA_ARGS__)

}
