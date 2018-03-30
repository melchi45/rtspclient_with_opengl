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
#include "FFMpegEncoder.h"
#include "Frame.h"

#define USE_YUV_FRAME	1
//#define USE_RGB_FRAME	1
#define FPS		30

#if USE_LIVE555
FFMpegEncoder::FFMpegEncoder(UsageEnvironment& env)
	: FFMpeg(env)
	, thread_exit(0)
{
}
#else
FFMpegEncoder::FFMpegEncoder()
	: FFMpeg()
	, thread_exit(0)
{
}
#endif

FFMpegEncoder::~FFMpegEncoder()
{
}

int FFMpegEncoder::intialize()
{	
	FFMpeg::intialize();

	/// create codec context for encoder
	/* find the h264 video encoder */
	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec) {
		fprintf(stderr, "Codec not found\n");
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		fprintf(stderr, "Could not allocate video codec context\n");
		return -2;
	}

	return SetupCodec();
}

int FFMpegEncoder::finalize()
{
	if (pSourceCodecCtx)
	{
		avcodec_close(pSourceCodecCtx);
		av_free(pSourceCodecCtx);
	}

	if (pSourceFormatCtx) {
		avformat_free_context(pSourceFormatCtx);
	}

	return FFMpeg::finalize();
}

void* FFMpegEncoder::run(void *param)
{
	FFMpegEncoder *pThis = (FFMpegEncoder*)param;
	pThis->ReadFrame();
	return NULL;
}

char FFMpegEncoder::GetFrame(uint8_t** FrameBuffer, unsigned int *FrameSize)
{
	if (!outqueue.empty())
	{
		Frame * data;
		data = outqueue.front();
		*FrameBuffer = (uint8_t*)data->dataPointer;
		*FrameSize = data->dataSize;
		return 1;
	}
	else
	{
		*FrameBuffer = 0;
		*FrameSize = 0;
		return 0;
	}
}

char FFMpegEncoder::ReleaseFrame()
{
	pthread_mutex_lock(&outqueue_mutex);
	if (!outqueue.empty())
	{
		Frame * data = outqueue.front();
		outqueue.pop();
		delete data;
	}
	pthread_mutex_unlock(&outqueue_mutex);
	return 1;
}