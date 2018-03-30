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