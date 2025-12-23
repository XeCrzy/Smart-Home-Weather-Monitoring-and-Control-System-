#ifndef INCLUDE_UTIL_LOGUTIL_H_
#define INCLUDE_UTIL_LOGUTIL_H_

#include "stdarg.h"

typedef void (*PRINTF_LOG_CALLBACK_HANDLER)(int level, char* format, va_list args);
void setPrintfLogCallback(PRINTF_LOG_CALLBACK_HANDLER callback);
void printfLog(int logLevel, char * _Format, ...);

//this compiling macro _SYS_LOG can only be used in Linux system
#ifdef _SYS_LOG

#include "syslog.h"

void initLogForLinux();
void setLogLocalNumber(int logLocalNumber);
void setLogLevel(int logLevel);
void destoryLogForLinux();

typedef enum enum_LOG_LEVEL
{
    EN_LOG_LEVEL_DEBUG = LOG_DEBUG,
    EN_LOG_LEVEL_INFO  = LOG_INFO,
    EN_LOG_LEVEL_WARNING = LOG_WARNING,
    EN_LOG_LEVEL_ERROR   = LOG_ERR,

}LOGLEVEL;

#else

typedef enum enum_LOG_LEVEL
{
    EN_LOG_LEVEL_DEBUG   = 7,
    EN_LOG_LEVEL_INFO    = 6,
    EN_LOG_LEVEL_WARNING = 4,
    EN_LOG_LEVEL_ERROR   = 3,

}LOGLEVEL;
#endif

#endif /* INCLUDE_UTIL_LOGUTIL_H_ */
