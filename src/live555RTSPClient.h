#ifndef _LIVE555_RTSP_CLIENT_HH
#define _LIVE555_RTSP_CLIENT_HH

#include "liveMedia.hh"
#include "StreamClientState.h"

class live555RTSPClient : public RTSPClient {
public:
	static live555RTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

protected:
	live555RTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~live555RTSPClient();

public:
	StreamClientState scs;
};

#endif _LIVE555_RTSP_CLIENT_HH