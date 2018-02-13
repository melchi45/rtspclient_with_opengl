#include "MediaBasicUsageEnvironment.h"
#include "log_utils.h"

#include <stdio.h>

////////// AmbaBasicUsageEnvironment //////////

#if defined(__WIN32__) || defined(_WIN32)
extern "C" int initializeWinsockIfNecessary();
#endif

MediaBasicUsageEnvironment*
MediaBasicUsageEnvironment::createNew(TaskScheduler& taskScheduler) {
	return new MediaBasicUsageEnvironment(taskScheduler);
}

MediaBasicUsageEnvironment::MediaBasicUsageEnvironment(TaskScheduler& taskScheduler)
: BasicUsageEnvironment0(taskScheduler) {
#if defined(__WIN32__) || defined(_WIN32)
  if (!initializeWinsockIfNecessary()) {
    setResultErrMsg("Failed to initialize 'winsock': ");
    reportBackgroundError();
    internalError();
  }
#endif
}

MediaBasicUsageEnvironment::~MediaBasicUsageEnvironment() {
}

int MediaBasicUsageEnvironment::getErrno() const {
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
  return WSAGetLastError();
#else
  return errno;
#endif
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(char const* str) {
  if (str == NULL) str = "(NULL)"; // sanity check
#ifdef _CONSOLE
  fprintf(stderr, "%s", str);
#else
  log_rtsp("%s", str);
#endif
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(int i) {
#ifdef _CONSOLE
  fprintf(stderr, "%d", i);
#else
	log_rtsp("%d", i);
#endif
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(unsigned u) {
#ifdef _CONSOLE
  fprintf(stderr, "%u", u);
#else
	log_rtsp("%u", u);
#endif
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(double d) {
#ifdef _CONSOLE
  fprintf(stderr, "%f", d);
#else
	log_rtsp("%f", d);
#endif
  return *this;
}

UsageEnvironment& MediaBasicUsageEnvironment::operator<<(void* p) {
#ifdef _CONSOLE
  fprintf(stderr, "%p", p);
#else
	log_rtsp("%p", p);
#endif
  return *this;
}
