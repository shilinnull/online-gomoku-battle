#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__
#include <time.h>
#include <stdio.h>

#define INF 0
#define DBG 1
#define ERR 2

#define DEFAULT_LOG_LEVEL INF

#define LOG(level, format, ...) do{\
    if(DEFAULT_LOG_LEVEL > level) break;\
    time_t t = time(nullptr);\
    struct tm *lt = localtime(&t);\
    char buf[32];\
    strftime(buf, 31, "%H:%M:%S", lt);\
    fprintf(stdout, "[%s %s:%d] " format "\n", buf, __FILE__, __LINE__, ##__VA_ARGS__);\
}while(0)

#define ILOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DLOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ELOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)

#endif