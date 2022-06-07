#include <bits/types/struct_timeval.h>
#include <bits/types/time_t.h>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <iomanip>
#include <sstream>
#include "eloop/log.hpp"


namespace eloop
{

#define MAXLINE  256

#ifdef ON_DEBUG
int logLevel = LOG_LEVEL_DEBUG;
#else
int logLevel = LOG_LEVEL_INFO;
#endif

static const char *log_level_str[] = {
    "[TRACE]",
    "[DEBUG]",
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[FATAL]"
};

static int log_fd = STDOUT_FILENO; 

static std::string timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t seconds = tv.tv_sec;

    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << tm_time.tm_year + 1900
       << std::setfill('0') << std::setw(2) << tm_time.tm_mon + 1
       << std::setfill('0') << std::setw(2) << tm_time.tm_mday
       << " "
       << std::setfill('0') << std::setw(2) << tm_time.tm_hour
       << ":"
       << std::setfill('0') << std::setw(2) << tm_time.tm_min
       << ":"
       << std::setfill('0') << std::setw(2) << tm_time.tm_sec
       << "."
       << std::setprecision(6) << tv.tv_usec;
    return ss.str();
}


static int timestamp(char *data, size_t len)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t seconds = tv.tv_sec;
    struct tm tm_time;

    gmtime_r(&seconds, &tm_time);
    return snprintf(
        data, len, "%4d%02d%02d %02d:%02d:%02d.%06ld",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec
    );
}



void setLogLevel(int level)
{
    if (level < LOG_LEVEL_TRACE)
        logLevel = LOG_LEVEL_TRACE;
    else if(level > LOG_LEVEL_FATAL)
        logLevel = LOG_LEVEL_FATAL;
    else
        logLevel = level;
}

void setLogFD(int fd)
{
    if(fd < 0)
        log_fd = STDOUT_FILENO;
    else
        log_fd = fd;
}


void log_base_(
    const char *file,
    int line,
    int level,
    bool to_abort,
    const char *fmt,
    ...
)
{
    char data[MAXLINE];
    size_t i = 0;
    va_list ap;

    i += timestamp(data, 32);
    i += sprintf(data + i, " [%d]", getpid());
    i += sprintf(data + i, " %s ", log_level_str[level]);

    va_start(ap, fmt);
    vsnprintf(data + i, MAXLINE - i, fmt, ap);
    va_end(ap);

    int err = dprintf(
        log_fd, "%s - %s:%d\n",
        data, strrchr(file, '/'), line
    );
    if (err == -1)
        fprintf(stderr, "log faild");
    if (to_abort)
        abort();
}


void log_sys_(
    const char *file,
    int line, bool to_abort,
    const char *fmt, ...
)
{
    char data[MAXLINE];
    size_t i = 0;
    va_list ap;

    i += timestamp(data, 32);
    i += sprintf(data + i, " [%d]", getpid());
    i += sprintf(data + i, " %s ", to_abort? "[SYSFA]": "[SYSER]");

    va_start(ap, fmt);
    vsnprintf(data + i, MAXLINE - i, fmt, ap);
    va_end(ap);

    dprintf(
        log_fd, "%s: %s - %s:%d\n",
        data, strerror(errno), strrchr(file, '/'), line
    );

    if (to_abort)
        abort();
}

}
