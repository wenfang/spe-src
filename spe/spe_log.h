#ifndef __SPE_LOG_H 
#define __SPE_LOG_H

#include <syslog.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define SPE_LOG_EMERG(fmt, arg...)     spe_log_write(LOG_EMERG|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_ALERT(fmt, arg...)     spe_log_write(LOG_ALERT|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_CRIT(fmt, arg...)      spe_log_write(LOG_CRIT|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_ERR(fmt, arg...)       spe_log_write(LOG_ERR|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_WARNING(fmt, arg...)   spe_log_write(LOG_WARNING|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_NOTICE(fmt, arg...)    spe_log_write(LOG_NOTICE|LOG_LOCAL6, "[%s:%d][[%d]%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_INFO(fmt, arg...)      spe_log_write(LOG_INFO|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_DEBUG(fmt, arg...)     spe_log_write(LOG_DEBUG|LOG_LOCAL6, "[%s:%d][%d][%x]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg) 

extern void 
spe_log_init(const char* name);

extern void 
spe_log_write(int priority, const char* message, ...);

extern void 
spe_log_close();

#endif
