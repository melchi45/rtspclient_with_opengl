// Implementation of "EventLoop":

#include "EventLoop.h"

extern bool isend;

/// //////////////////////////////////////////////////////////////////////////
EventLoop::EventLoop()
{

}

void EventLoop::doEventLoop(BasicTaskScheduler0* Basicscheduler)
{ // Repeatedly loop, handling readble sockets and timed events:  
	while (isend) {
		//printf("zjk\n");
		Basicscheduler->SingleStep();
		//ADD Sth else
	}
}
