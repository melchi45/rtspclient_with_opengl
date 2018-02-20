#ifndef _UTILS_H_
#define _UTILS_H_

#if defined(WIN32)

#define WIN32_LEAN_AND_MEAN

#if !defined(_WINSOCK2API_)
// MSVC defines this in winsock2.h!?
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} timeval;
#endif

int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif

#endif // _UTILS_H_ end