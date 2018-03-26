#pragma once
#ifndef _MEDIA_H264_MEDIASINK_H_
#define _MEDIA_H264_MEDIASINK_H_

#include "liveMedia.hh"

#if 0
class FFmpegDecoder;
#else
class H264Decoder;
#endif
// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.
class MediaH264MediaSink : public MediaSink {
public:
	static MediaH264MediaSink* createNew(UsageEnvironment& env,
		RTSPClient* client,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)
#if 0
	FFmpegDecoder* getDecoder() { return video_decoder; }
#else
	H264Decoder* getDecoder() { return video_decoder; }
#endif
	long getFrameCount() { return m_nFrameCount; }

private:
	MediaH264MediaSink(UsageEnvironment& env, RTSPClient* client, MediaSubsession& subsession,
			char const* streamId);
	// called only by "createNew()"
	virtual ~MediaH264MediaSink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
			unsigned numTruncatedBytes, struct timeval presentationTime,
			unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			struct timeval presentationTime, unsigned durationInMicroseconds);
	bool isH264iFrame(u_int8_t* packet);
	bool FindStartCode3(unsigned char *Buf);
	bool FindStartCode4(unsigned char *Buf);
private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
	unsigned m_nFrameSize;
	unsigned m_nNalHeaderStartCodeOffset;
#if 0
	FFmpegDecoder* video_decoder;
#else
	H264Decoder* video_decoder;
#endif
	long m_nFrameCount;
	RTSPClient* pClient;
};

#endif /* _MEDIA_H264_MEDIASINK_H_ */
