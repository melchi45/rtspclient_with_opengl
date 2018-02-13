#ifndef MEDIA_BASICUSAGEENVIRONMENT_H_
#define MEDIA_BASICUSAGEENVIRONMENT_H_

#ifndef _BASIC_USAGE_ENVIRONMENT0_HH
#include <BasicUsageEnvironment0.hh>
#endif

class MediaBasicUsageEnvironment : public BasicUsageEnvironment0 {
protected:
	MediaBasicUsageEnvironment(TaskScheduler& taskScheduler);
	// called only by "createNew()" (or subclass constructors)
	virtual ~MediaBasicUsageEnvironment();

public:
	static MediaBasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

	// redefined virtual functions:
	virtual int getErrno() const;

	virtual UsageEnvironment& operator<<(char const* str);
	virtual UsageEnvironment& operator<<(int i);
	virtual UsageEnvironment& operator<<(unsigned u);
	virtual UsageEnvironment& operator<<(double d);
	virtual UsageEnvironment& operator<<(void* p);
};
#endif /* MEDIA_AMBABASICUSAGEENVIRONMENT_H_ */
