#ifndef _EVENT_LOOP_HH
#define _EVENT_LOOP_HH

#ifndef _BASIC_USAGE_ENVIRONMENT0_HH
#include "BasicUsageEnvironment0.hh"
#endif

class EventLoop
{
public:
	EventLoop();
	void doEventLoop(BasicTaskScheduler0* Basicscheduler);
};

#endif // _EVENT_LOOP_HH