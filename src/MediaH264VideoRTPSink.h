#pragma once

#ifndef _MEDIA_H264_VIDEO_RTP_SINK_H_
#define _MEDIA_H264_VIDEO_RTP_SINK_H_

#include "liveMedia.hh"

class MediaH264VideoRTPSink : public H264VideoRTPSink
{
public:
	static MediaH264VideoRTPSink* createNew(UsageEnvironment& env,
		MediaSubsession* subsession, // identifies the kind of data that's being received
		u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize,
		char const* streamId = NULL); // identifies the stream itself (optional)

protected:
	MediaH264VideoRTPSink(UsageEnvironment& env, MediaSubsession* subsession,
		u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize,
		char const* streamId);
	virtual ~MediaH264VideoRTPSink();

private:
	static void afterGettingFrame(void* clientData, unsigned frameSize,
		unsigned numTruncatedBytes, struct timeval presentationTime,
		unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds);

	// redefined virtual functions:
	virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
		unsigned char* frameStart,
		unsigned numBytesInFrame,
		struct timeval framePresentationTime,
		unsigned numRemainingBytes);
	virtual Boolean continuePlaying();

private:
	u_int8_t * fReceiveBuffer;
	MediaSubsession* fSubsession;
	char* fStreamId;
};

#endif // _MEDIA_H264_VIDEO_RTP_SINK_H_