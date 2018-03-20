#pragma once

#ifndef _MEDIA_H264_VIDEO_SOURCE_H_
#define _MEDIA_H264_VIDEO_SOURCE_H_

#include "liveMedia.hh"
#include "FramedSource.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

class MediaH264VideoSource : public FramedSource
{
public:
	MediaH264VideoSource(UsageEnvironment & env);
	virtual ~MediaH264VideoSource(void);

public:
	virtual void doGetNextFrame();
	virtual unsigned int maxFrameSize() const;

	static void getNextFrame(void * ptr);
	void GetFrameData();

private:
	void *m_pToken;
	char *m_pFrameBuffer;
};

#endif // _MEDIA_H264_VIDEO_SOURCE_H_