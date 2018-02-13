#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <syslog.h>
#include <sys/time.h>
#else
#include <process.h>
#include <windows.h>
#endif
#include <time.h>

#include "log_utils.h"

#ifdef WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define _snprintf snprintf
#endif

#ifndef BOOL_H_
#define BOOL_H_

 /** Boolean data type which is so natural for all programmers. */
typedef int BOOL;

#ifndef TRUE
/** That's @a true. */
#define TRUE 1
#endif

#ifndef FALSE
/** This is @a false. */
#define FALSE 0
#endif

#endif /* BOOL_H_ */

/** @name Logging to @a stdout
 * @{ */
static void log_rtspPrintf(char* fmt, ...);
static void log_infoPrintf(char* fmt, ...);
static void log_debugPrintf(char* fmt, ...);
static void log_warningPrintf(char* fmt, ...);
static void log_errorPrintf(char* fmt, ...);
/** @}
 * @name Logging using @a syslog
 * @{ */
static void log_rtspSyslog(char* fmt, ...);
static void log_infoSyslog(char* fmt, ...);
static void log_debugSyslog(char* fmt, ...);
static void log_warningSyslog(char* fmt, ...);
static void log_errorSyslog(char* fmt, ...);
/** @} */

/** @name Log handlers.
 * Used to invoke quickly current logging function.
 * @{ */
log_function log_rtspHandler = &log_rtspPrintf;
log_function log_infoHandler = &log_infoPrintf;
log_function log_debugHandler = &log_debugPrintf;
log_function log_warningHandler = &log_warningPrintf;
log_function log_errorHandler = &log_errorPrintf;
/** @} */

/** Defines global log level. */
static int _log_level = LOG_LEVEL_DEBUG;

/** Logging mode. */
static BOOL _use_syslog = FALSE;

/** Logs info message using printf. */
static void log_rtspPrintf(char* fmt, ...)
{
	char szOut[4096];
	size_t nMaxBufLen = sizeof(szOut);
	size_t nRemainBufLen = nMaxBufLen;
	memset(szOut, 0, nMaxBufLen);
	unsigned int nLen = 0;

	// 시간, 프로세스 정보 등 조합
#ifdef _WIN32
	SYSTEMTIME	stime;
	GetLocalTime(&stime);

	nLen = _snprintf_s(szOut, _countof(szOut), _TRUNCATE, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in [0x%08X]  ", "[rtsp]", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds, GetCurrentThreadId());
#else
	struct tm t;
	time_t ct = time(NULL);
	localtime_r(&ct, &t);
	timeval tmv;
	gettimeofday(&tmv, NULL);

	nLen = snprintf(szOut, nRemainBufLen, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in [%d:%08X]  ", "[rtsp]", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmv.tv_usec / 100000, getpid(), pthread_mach_thread_np(pthread_self()));
#endif
	// flush string offset
	if (nLen == -1) {
		return;
	}
	else if (nRemainBufLen >= nLen) {
		nRemainBufLen -= (size_t)nLen;
	}
	else {
		nRemainBufLen = 0;
	}

	if (nRemainBufLen > 0) {
		// format부터 조합
		va_list args;
		va_start(args, fmt);
		nLen = vsnprintf(szOut + nLen, nRemainBufLen, fmt, args);
		va_end(args);

		if (nLen == -1) {
			return;
		}
		else if (nRemainBufLen >= nLen) {
			nRemainBufLen -= (size_t)nLen;
		}
		else {
			nRemainBufLen = 0;
		}
	}

	// 조합 마무리
	if (nRemainBufLen >= 2) {
//		szOut[nMaxBufLen - nRemainBufLen] = '\n';
		szOut[nMaxBufLen - nRemainBufLen + 1] = 0;
	}
	szOut[nMaxBufLen - 1] = 0;

#if (!defined(_WIN32) || !defined(_WIN64)) && ((defined(__APPLE__) && defined(__MACH__)))
	NSString *str = [[NSString alloc] initWithUTF8String:szOut];
	NSLog(@"%@", str);
	[str release];
#elif defined(WIN32) || defined(WIN64)
#ifdef _CONSOLE
	fprintf(stderr, "%s", szOut);
#else
	OutputDebugString(szOut);
#endif
#else // defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__linux)
	printf(ANSI_COLOR_GREEN "[%5.6f TRANSPORT_STREAM:INFO] ", ((double)clock()) / CLOCKS_PER_SEC);
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#endif
}

/** Logs info message using printf. */
static void log_infoPrintf(char* fmt, ...)
{
	char szOut[4096];
	size_t nMaxBufLen = sizeof(szOut);
	size_t nRemainBufLen = nMaxBufLen;
	memset(szOut, 0, nMaxBufLen);
	unsigned int nLen = 0;

	// 시간, 프로세스 정보 등 조합
#ifdef _WIN32
	SYSTEMTIME	stime;
	GetLocalTime(&stime);

	nLen = _snprintf_s(szOut, _countof(szOut), _TRUNCATE, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[0x%08X]  ", "[info]", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds, __FILENAME__, __LINE__, __FUNCTION__, GetCurrentThreadId());
#else
	struct tm t;
	time_t ct = time(NULL);
	localtime_r(&ct, &t);
	timeval tmv;
	gettimeofday(&tmv, NULL);

	nLen = snprintf(szOut, nRemainBufLen, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[%d:%08X]  ", "[info]", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmv.tv_usec / 100000, __FILENAME__, __LINE__, __FUNCTION__, getpid(), pthread_mach_thread_np(pthread_self()));
#endif
	// flush string offset
	if (nLen == -1) {
		return;
	}
	else if (nRemainBufLen >= nLen) {
		nRemainBufLen -= (size_t)nLen;
	}
	else {
		nRemainBufLen = 0;
	}

	if (nRemainBufLen > 0) {
		// format부터 조합
		va_list args;
		va_start(args, fmt);
		nLen = vsnprintf(szOut + nLen, nRemainBufLen, fmt, args);
		va_end(args);

		if (nLen == -1) {
			return;
		}
		else if (nRemainBufLen >= nLen) {
			nRemainBufLen -= (size_t)nLen;
		}
		else {
			nRemainBufLen = 0;
		}
	}

	// 조합 마무리
	if (nRemainBufLen >= 2) {
		szOut[nMaxBufLen - nRemainBufLen] = '\n';
		szOut[nMaxBufLen - nRemainBufLen + 1] = 0;
	}
	szOut[nMaxBufLen - 1] = 0;

#if (!defined(_WIN32) || !defined(_WIN64)) && ((defined(__APPLE__) && defined(__MACH__)))
	NSString *str = [[NSString alloc] initWithUTF8String:szOut];
	NSLog(@"%@", str);
	[str release];
#elif defined(WIN32) || defined(WIN64)
#ifdef _CONSOLE
	printf(ANSI_COLOR_GREEN "");
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#else
	OutputDebugString(szOut);
#endif
#else // defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__linux)
	printf(ANSI_COLOR_GREEN "[%5.6f TRANSPORT_STREAM:INFO] ", ((double)clock()) / CLOCKS_PER_SEC);
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#endif
}

/** Logs debug message using printf. */
static void log_debugPrintf(char* fmt, ...)
{
	char szOut[4096];
	size_t nMaxBufLen = sizeof(szOut);
	size_t nRemainBufLen = nMaxBufLen;
	memset(szOut, 0, nMaxBufLen);
	unsigned int nLen = 0;

	// 시간, 프로세스 정보 등 조합
#ifdef _WIN32
	SYSTEMTIME	stime;
	GetLocalTime(&stime);

	nLen = _snprintf_s(szOut, _countof(szOut), _TRUNCATE, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[0x%08X]  ", "[debug]", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds, __FILENAME__, __LINE__, __FUNCTION__, GetCurrentThreadId());
#else
	struct tm t;
	time_t ct = time(NULL);
	localtime_r(&ct, &t);
	timeval tmv;
	gettimeofday(&tmv, NULL);

	nLen = snprintf(szOut, nRemainBufLen, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[%d:%08X]  ", "[debug]", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmv.tv_usec / 100000, __FILENAME__, __LINE__, __FUNCTION__, getpid(), pthread_mach_thread_np(pthread_self()));
#endif
	// flush string offset
	if (nLen == -1) {
		return;
	}
	else if (nRemainBufLen >= nLen) {
		nRemainBufLen -= (size_t)nLen;
	}
	else {
		nRemainBufLen = 0;
	}

	if (nRemainBufLen > 0) {
		// format부터 조합
		va_list args;
		va_start(args, fmt);
		nLen = vsnprintf(szOut + nLen, nRemainBufLen, fmt, args);
		va_end(args);

		if (nLen == -1) {
			return;
		}
		else if (nRemainBufLen >= nLen) {
			nRemainBufLen -= (size_t)nLen;
		}
		else {
			nRemainBufLen = 0;
		}
	}

	// 조합 마무리
	if (nRemainBufLen >= 2) {
		szOut[nMaxBufLen - nRemainBufLen] = '\n';
		szOut[nMaxBufLen - nRemainBufLen + 1] = 0;
	}
	szOut[nMaxBufLen - 1] = 0;

#if (!defined(_WIN32) || !defined(_WIN64)) && ((defined(__APPLE__) && defined(__MACH__)))
	NSString *str = [[NSString alloc] initWithUTF8String:szOut];
	NSLog(@"%@", str);
	[str release];
#elif defined(WIN32) || defined(WIN64)
#ifdef _CONSOLE
	printf(ANSI_COLOR_GREEN "");
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#else
	OutputDebugString(szOut);
#endif
#else // defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__linux)
	printf(ANSI_COLOR_GREEN "[%5.6f TRANSPORT_STREAM:INFO] ", ((double)clock()) / CLOCKS_PER_SEC);
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#endif
}

/** Logs warning message using printf. */
static void log_warningPrintf(char* fmt, ...)
{
	char szOut[4096];
	size_t nMaxBufLen = sizeof(szOut);
	size_t nRemainBufLen = nMaxBufLen;
	memset(szOut, 0, nMaxBufLen);
	unsigned int nLen = 0;

	// 시간, 프로세스 정보 등 조합
#ifdef _WIN32
	SYSTEMTIME	stime;
	GetLocalTime(&stime);

	nLen = _snprintf_s(szOut, _countof(szOut), _TRUNCATE, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[0x%08X]  ", "[warning]", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds, __FILENAME__, __LINE__, __FUNCTION__, GetCurrentThreadId());
#else
	struct tm t;
	time_t ct = time(NULL);
	localtime_r(&ct, &t);
	timeval tmv;
	gettimeofday(&tmv, NULL);

	nLen = snprintf(szOut, nRemainBufLen, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[%d:%08X]  ", "[warning]", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmv.tv_usec / 100000, __FILENAME__, __LINE__, __FUNCTION__, getpid(), pthread_mach_thread_np(pthread_self()));
#endif
	// flush string offset
	if (nLen == -1) {
		return;
	}
	else if (nRemainBufLen >= nLen) {
		nRemainBufLen -= (size_t)nLen;
	}
	else {
		nRemainBufLen = 0;
	}

	if (nRemainBufLen > 0) {
		// format부터 조합
		va_list args;
		va_start(args, fmt);
		nLen = vsnprintf(szOut + nLen, nRemainBufLen, fmt, args);
		va_end(args);

		if (nLen == -1) {
			return;
		}
		else if (nRemainBufLen >= nLen) {
			nRemainBufLen -= (size_t)nLen;
		}
		else {
			nRemainBufLen = 0;
		}
	}

	// 조합 마무리
	if (nRemainBufLen >= 2) {
		szOut[nMaxBufLen - nRemainBufLen] = '\n';
		szOut[nMaxBufLen - nRemainBufLen + 1] = 0;
	}
	szOut[nMaxBufLen - 1] = 0;

#if (!defined(_WIN32) || !defined(_WIN64)) && ((defined(__APPLE__) && defined(__MACH__)))
	NSString *str = [[NSString alloc] initWithUTF8String:szOut];
	NSLog(@"%@", str);
	[str release];
#elif defined(WIN32) || defined(WIN64)
#ifdef _CONSOLE
	printf(ANSI_COLOR_GREEN "");
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#else
	OutputDebugString(szOut);
#endif
#else // defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__linux)
	printf(ANSI_COLOR_GREEN "[%5.6f TRANSPORT_STREAM:INFO] ", ((double)clock()) / CLOCKS_PER_SEC);
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#endif
}

/** Logs error message using printf. */
static void log_errorPrintf(char* fmt, ...)
{
	char szOut[4096];
	size_t nMaxBufLen = sizeof(szOut);
	size_t nRemainBufLen = nMaxBufLen;
	memset(szOut, 0, nMaxBufLen);
	unsigned int nLen = 0;

	// 시간, 프로세스 정보 등 조합
#ifdef _WIN32
	SYSTEMTIME	stime;
	GetLocalTime(&stime);

	nLen = _snprintf_s(szOut, _countof(szOut), _TRUNCATE, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[0x%08X]  ", "[error]", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds, __FILENAME__, __LINE__, __FUNCTION__, GetCurrentThreadId());
#else
	struct tm t;
	time_t ct = time(NULL);
	localtime_r(&ct, &t);
	timeval tmv;
	gettimeofday(&tmv, NULL);

	nLen = snprintf(szOut, nRemainBufLen, "%9s %04d-%02d-%02d %02d:%02d:%02d.%06d in %s:(%d):%s[%d:%08X]  ", "[error]", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tmv.tv_usec / 100000, __FILENAME__, __LINE__, __FUNCTION__, getpid(), pthread_mach_thread_np(pthread_self()));
#endif
	// flush string offset
	if (nLen == -1) {
		return;
	}
	else if (nRemainBufLen >= nLen) {
		nRemainBufLen -= (size_t)nLen;
	}
	else {
		nRemainBufLen = 0;
	}

	if (nRemainBufLen > 0) {
		// format부터 조합
		va_list args;
		va_start(args, fmt);
		nLen = vsnprintf(szOut + nLen, nRemainBufLen, fmt, args);
		va_end(args);

		if (nLen == -1) {
			return;
		}
		else if (nRemainBufLen >= nLen) {
			nRemainBufLen -= (size_t)nLen;
		}
		else {
			nRemainBufLen = 0;
		}
	}

	// 조합 마무리
	if (nRemainBufLen >= 2) {
		szOut[nMaxBufLen - nRemainBufLen] = '\n';
		szOut[nMaxBufLen - nRemainBufLen + 1] = 0;
	}
	szOut[nMaxBufLen - 1] = 0;

#if (!defined(_WIN32) || !defined(_WIN64)) && ((defined(__APPLE__) && defined(__MACH__)))
	NSString *str = [[NSString alloc] initWithUTF8String:szOut];
	NSLog(@"%@", str);
	[str release];
#elif defined(WIN32) || defined(WIN64)
#ifdef _CONSOLE
	printf(ANSI_COLOR_GREEN "");
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#else
	OutputDebugString(szOut);
#endif
#else // defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__linux)
	printf(ANSI_COLOR_GREEN "[%5.6f TRANSPORT_STREAM:INFO] ", ((double)clock()) / CLOCKS_PER_SEC);
	printf("%s", szOut);
	printf(ANSI_COLOR_RESET "\n");
#endif
}

/** Logs rtsp message using syslog. */
#ifndef WIN32
static void log_rtspSyslog(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_INFO, fmt, args);
    va_end(args);
}
#else
static void log_rtspSyslog(char* fmt, ...) {}
#endif

/** Logs info message using syslog. */
#ifndef WIN32
static void log_infoSyslog(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_INFO, fmt, args);
    va_end(args);
}
#else
static void log_infoSyslog(char* fmt, ...) {}
#endif

/** Logs debug message using syslog. */
#ifndef WIN32
static void log_debugSyslog(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_DEBUG, fmt, args);
    va_end(args);
}
#else
static void log_debugSyslog(char* fmt, ...) {}
#endif

/** Logs warning message using syslog. */
#ifndef WIN32
static void log_warningSyslog(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_WARNING, fmt, args);
    va_end(args);
}
#else
static void log_warningSyslog(char* fmt, ...) {}
#endif

/** Logs error message using syslog. */
#ifndef WIN32
static void log_errorSyslog(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_ERR, fmt, args);
    va_end(args);
}
#else
static void log_errorSyslog(char* fmt, ...) {}
#endif


/** Does not log anything.
 * Used to ignore messages of some priorities according to the global log level.
 */
static void log_nothing(char* fmt, ...)
{
    // dismiss the log message
    va_list args;
    va_start(args, fmt);
//    vsyslog(LOG_ERR, fmt, args);
    va_end(args);
}

/** Configures logging system in printf mode according to global log level. */
static void setPrintf()
{
    // drop all log functions
	log_rtspHandler = &log_nothing;
	log_infoHandler = &log_nothing;
    log_debugHandler = &log_nothing;
    log_warningHandler = &log_nothing;
    log_errorHandler = &log_nothing;
    // set corresponding log functions
    switch(_log_level)
    {
    case LOG_LEVEL_DEBUG:
        log_debugHandler = &log_debugPrintf;
    case LOG_LEVEL_WARNING:
        log_warningHandler = &log_warningPrintf;
    case LOG_LEVEL_RTSP:
        log_rtspHandler = &log_rtspPrintf;
    case LOG_LEVEL_INFO:
        log_infoHandler = &log_infoPrintf;
    case LOG_LEVEL_ERROR:
        log_errorHandler = &log_errorPrintf;
    default:
    	break;
    }
}

/** Configures logging system in syslog mode according to global log level. */
static void setSyslog()
{
    // drop all log functions
	log_rtspHandler = &log_nothing;
	log_infoHandler = &log_nothing;
    log_debugHandler = &log_nothing;
    log_warningHandler = &log_nothing;
    log_errorHandler = &log_nothing;
    // set corresponding log functions
    switch(_log_level)
    {
    case LOG_LEVEL_DEBUG:
        log_debugHandler = &log_debugSyslog;
    case LOG_LEVEL_WARNING:
        log_warningHandler = &log_warningSyslog;
    case LOG_LEVEL_RTSP:
        log_rtspHandler = &log_rtspSyslog;
    case LOG_LEVEL_INFO:
        log_infoHandler = &log_infoSyslog;
    case LOG_LEVEL_ERROR:
        log_errorHandler = &log_errorSyslog;
    default:
    	break;
    }
}

void log_usePrintf()
{
    _use_syslog = FALSE;
    // close syslog connection if it was opened

#ifndef WIN32
    closelog();
#endif
    setPrintf();
}

#ifndef WIN32
void log_useSyslog(int facility)
{
    _use_syslog = TRUE;
    openlog(NULL, LOG_NDELAY, facility);
    setSyslog();
}
#else
void log_useSyslog(int facility) {}
#endif

void log_closeSyslog()
{
	// close syslog connection if it was opened
#ifndef WIN32
	closelog();
#endif
}

void log_setLevel(LOG_LEVEL level)
{
    _log_level = level;
    if (_use_syslog)
    {
        setSyslog();
    }
    else
    {
        setPrintf();
    }
}
