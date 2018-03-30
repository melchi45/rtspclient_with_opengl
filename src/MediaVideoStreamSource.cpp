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
#include "MediaVideoStreamSource.h"
#include "GroupsockHelper.hh"
#include "FFMpegEncoder.h"

#define REV_BUF_SIZE  (1024*1024) 
#define FRAME_PER_SEC 25 

MediaVideoStreamSource * MediaVideoStreamSource::createNew(UsageEnvironment& env, FFMpegEncoder * enc) {
	return new MediaVideoStreamSource(env, enc);
}

MediaVideoStreamSource::MediaVideoStreamSource(UsageEnvironment & env, FFMpegEncoder * enc)
	: FramedSource(env)
{
	m_eventTriggerId = envir().taskScheduler().createEventTrigger(MediaVideoStreamSource::deliverFrameStub);
//	std::function<void()> callback1 = std::bind(&MediaH264VideoSource::onEncoded, this);
//	encoder->setCallbackFunctionFrameIsReady(callback1);
}

MediaVideoStreamSource::~MediaVideoStreamSource(void)
{
}

void MediaVideoStreamSource::doStopGettingFrames()
{
	FramedSource::doStopGettingFrames();
}

void MediaVideoStreamSource::onEncoded()
{
	envir().taskScheduler().triggerEvent(m_eventTriggerId, this);
}

void MediaVideoStreamSource::doGetNextFrame()
{
	deliverFrame();
}

void MediaVideoStreamSource::deliverFrame()
{
	if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

	static uint8_t* newFrameDataStart;
	static unsigned newFrameSize = 0;

	/* get the data frame from the Encoding thread.. */
	if (encoder->GetFrame(&newFrameDataStart, &newFrameSize)) {
		if (newFrameDataStart != NULL) {
			/* This should never happen, but check anyway.. */
			if (newFrameSize > fMaxSize) {
				fFrameSize = fMaxSize;
				fNumTruncatedBytes = newFrameSize - fMaxSize;
			}
			else {
				fFrameSize = newFrameSize;
			}

			gettimeofday(&fPresentationTime, NULL);
			memcpy(fTo, newFrameDataStart, fFrameSize);

			//delete newFrameDataStart;
			//newFrameSize = 0;

			encoder->ReleaseFrame();
		}
		else {
			fFrameSize = 0;
			fTo = NULL;
			handleClosure(this);
		}
	}
	else
	{
		fFrameSize = 0;
	}

	if (fFrameSize>0)
		FramedSource::afterGetting(this);

}