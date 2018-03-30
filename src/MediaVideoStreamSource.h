#pragma once

#ifndef _MEDIA_VIDEO_STREAM_SOURCE_H_
#define _MEDIA_VIDEO_STREAM_SOURCE_H_

#include "liveMedia.hh"
#include "FFMpegEncoder.h"

class FFMpegEncoder;
class MediaVideoStreamSource : public FramedSource, public EncodeListener
{
public:
	static MediaVideoStreamSource* createNew(UsageEnvironment& env, FFMpegEncoder * encoder);

public:
	MediaVideoStreamSource(UsageEnvironment & env, FFMpegEncoder * encoder);
	virtual ~MediaVideoStreamSource(void);

public:
	static void deliverFrameStub(void* clientData) { ((MediaVideoStreamSource*)clientData)->deliverFrame(); };
	virtual void doGetNextFrame();
	void deliverFrame();
	virtual void doStopGettingFrames();

private:
	FFMpegEncoder* encoder;
	EventTriggerId m_eventTriggerId;

	// EncodeListener을(를) 통해 상속됨
	virtual void onEncoded() override;
};

#endif // _MEDIA_VIDEO_STREAM_SOURCE_H_