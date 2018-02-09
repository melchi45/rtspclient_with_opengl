#ifndef _STREAM_CLIENT_STATE_HH
#define _STREAM_CLIENT_STATE_HH

#include "liveMedia.hh"

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator * iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

#endif // _STREAM_CLIENT_STATE_HH