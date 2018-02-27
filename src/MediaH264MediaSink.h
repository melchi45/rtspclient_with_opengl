#pragma once
#ifndef _MEDIA_H264_MEDIASINK_H_
#define _MEDIA_H264_MEDIASINK_H_

#include "liveMedia.hh"

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.
class MediaH264MediaSink : public MediaSink {
public:
	static MediaH264MediaSink* createNew(UsageEnvironment& env,
			MediaSubsession& subsession, // identifies the kind of data that's being received
			char const* streamId = NULL); // identifies the stream itself (optional)

private:
	MediaH264MediaSink(UsageEnvironment& env, MediaSubsession& subsession,
			char const* streamId);
	// called only by "createNew()"
	virtual ~MediaH264MediaSink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime,
			unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
	int video_framing;
};

#endif /* _MEDIA_H264_MEDIASINK_H_ */
