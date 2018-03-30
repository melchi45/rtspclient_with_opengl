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
#include "MediaVideoServerMediaSubsession.h"
#include "MediaVideoStreamSource.h"
#include "MediaVideoRTPSink.h"

#include "H264ReadCameraEncoder.h"
#include "H264ReadScreenEncoder.h"

MediaVideoServerMediaSubsession * MediaVideoServerMediaSubsession::createNew(UsageEnvironment& env, VIDEO_SOURCE_TYPE type)
{
	return new MediaVideoServerMediaSubsession(env, type);
}

void MediaVideoServerMediaSubsession::rtcpRRQos(void* clientData)
{
	// TODO: add process for rtcp RR QOS
}

RTCPInstance* MediaVideoServerMediaSubsession
::createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
	unsigned char const* cname, RTPSink* sink)
{
	// Default implementation:
	RTCPInstance      *pRTCPInstance = NULL;

	pRTCPInstance = RTCPInstance::createNew(envir(), RTCPgs, totSessionBW,
		cname, sink, NULL/*we're a server*/);

	if (pRTCPInstance)
		pRTCPInstance->setRRHandler(rtcpRRQos, this);

	return pRTCPInstance;
}

					
FramedSource* MediaVideoServerMediaSubsession
::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
	FFMpegEncoder* encoder = NULL;

	// create new encoder with video source type
	switch(video_source_type) {
	case VIDEO_SOURCE_TYPE_CAMERA:
		break;
	case VIDEO_SOURCE_TYPE_SCREEN:
		break;
	default:
		break;

	}
	// generate video stream source
	MediaVideoStreamSource *pVideoSource = MediaVideoStreamSource::createNew(envir(), encoder);

	return H264VideoStreamDiscreteFramer::createNew(envir(), pVideoSource);
}

void MediaVideoServerMediaSubsession::afterPlaying(void* clientData)
{
	// TODO: add process after playing
}
		
RTPSink* MediaVideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,  unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
	fVideoRTPSink = MediaVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
	MediaVideoStreamSource* pVideoSource = (MediaVideoStreamSource*)inputSource;
	//log_rtsp("SDP = %s", getAuxSDPLine(fVidRTPSink, pAmbaSource));
	envir() << "Amba Video SDP: " << getAuxSDPLine(fVideoRTPSink, pVideoSource);
	// Finally, start playing:
	//envir() << "Beginning to read from UPP...\n";
	fVideoRTPSink->startPlaying(*pVideoSource, MediaVideoServerMediaSubsession::afterPlaying, fVideoRTPSink);

	return fVideoRTPSink;
}

void MediaVideoServerMediaSubsession
::seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration,
	u_int64_t& /*numBytes*/)
{
	MediaVideoStreamSource   *pVideoSource = (MediaVideoStreamSource*)inputSource;

//	if ((float)seekNPT < fDuration)
//		pVideoSource->seekStream((int32_t)seekNPT);
}