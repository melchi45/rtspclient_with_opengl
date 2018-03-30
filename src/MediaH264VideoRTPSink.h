/*******************************************************************************
*  Copyright (c) 1998 MFC Forum
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Module Name:
*
* Revision History:
*
* Date        Ver Name                    Description
* ----------  --- --------------------- -----------------------------------------
* 07-Jun-2016 0.1 Youngho Kim             Created
* ----------  --- --------------------- -----------------------------------------
*
* DESCRIPTION:
*
*  $Author:
*  $LastChangedBy:
*  $Date:
*  $Revision: 2949 $
*  $Id:
*  $HeadURL:
*******************************************************************************/
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