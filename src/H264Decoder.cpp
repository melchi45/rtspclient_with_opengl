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
#include "H264Decoder.h"
#include "Frame.h"

#define SAVE_AVFRAME_TO_JPEG 1

H264Decoder::H264Decoder()
	: FFMpeg()
{
	codec_id = AV_CODEC_ID_H264;
	dstWidth = 640;
	dstHeight = 320;

	intialize();
}

H264Decoder::~H264Decoder()
{
	finalize();
}

int H264Decoder::intialize()
{	
	FFMpeg::intialize();

	// DECODER SETUP
	pCodec = avcodec_find_decoder(codec_id);
	if (!pCodec)
	{
		//log_error("codec not found");
		//fEnviron << "codec not found";
		return -2;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx)
	{
		//log_error("codec context not found");
		//fEnviron << "codec context not found";
		return -3;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		//log_error("could not open codec");
		//fEnviron << "could not open codec";
		return -4;
	}
	m_bInit = true;

	return 0;
}

int H264Decoder::finalize()
{

	return FFMpeg::finalize();
}

int H264Decoder::decode(uint8_t* input, int nLen, bool bWaitIFrame /*= false*/)
{
	if (!m_bInit)
		return -1;

	if (input == NULL || nLen <= 0)
		return -2;

	try {
		int got_picture;
		int size = nLen;

		// set packet 
		AVPacket avpkt;
		av_init_packet(&avpkt);
		avpkt.size = size;
		avpkt.data = input;

		//while (avpkt.size > 0)
		{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 72, 2)
			int len = avcodec_decode_video(decoder_context, decoder_picture, &got_picture, avpkt->data, avpkt->size);
#else
			int len = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &avpkt); // libavcodec >= 52.72.2 (0.6)
#endif

			if (len == -1)
			{
				return -3;
			}

			if (got_picture)
			{
				// set image size
				srcWidth = pCodecCtx->width;
				srcHeight = pCodecCtx->height;
				// calculate byte size from rgb image by width and height
				int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, dstWidth, dstHeight);
				uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

				// YUV to RGB Color
				// reference
				// https://gist.github.com/lkraider/832062
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
				AVFrame *pFrameRGB = av_frame_alloc();
#else
				AVFrame *pFrameRGB = avcodec_alloc_frame();
#endif
				// initialize RGB frame
				avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, dstWidth, dstHeight);

				img_convert_ctx = sws_getCachedContext(img_convert_ctx,
					pCodecCtx->width, // width of source image
					pCodecCtx->height, // height of source image 
					pCodecCtx->pix_fmt,
					dstWidth, // width of destination image
					dstHeight, // height of destination image
					AV_PIX_FMT_RGB24, getSWSType(), NULL, NULL, NULL);
				if (img_convert_ctx == NULL)
				{
					//log_error("Cannot initialize the conversion context");
					//fEnviron << "Cannot initialize the conversion context";
					return -4;
				}
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
					0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				pFrameRGB->format = AV_PIX_FMT_RGB24;
				pFrameRGB->width = dstWidth;
				pFrameRGB->height = dstHeight;

#if defined(SAVE_AVFRAME_TO_PPM)
				save_frame_as_ppm(pFrameRGB);
#elif defined(SAVE_AVFRAME_TO_JPEG)
				save_frame_as_jpeg(pFrameRGB);
#endif
				Frame* frame = new Frame();
				// memory initialize
				frame->dataPointer = new uint8_t[numBytes * sizeof(uint8_t)];
				memset(frame->dataPointer, 0x00, numBytes * sizeof(uint8_t));
				// image copy
				memcpy(frame->dataPointer, pFrameRGB->data[0], numBytes * sizeof(uint8_t));
				// set image data
				frame->dataSize = numBytes * sizeof(uint8_t);
				frame->frameID = frame_count++;
				frame->width = dstWidth;
				frame->height = dstHeight;
				frame->pitch = pFrameRGB->linesize[0];

				if (m_plistener != NULL) {
					((DecodeListener*)m_plistener)->onDecoded(frame);
				} else if (onDecoded != NULL) {
					onDecoded(frame);
				}

				av_free(buffer);
				av_free(pFrameRGB);
				return 0;

				if (avpkt.data)
				{
					avpkt.size -= len;
					avpkt.data += len;
				}
/*
				pthread_mutex_lock(&outqueue_mutex);

				if (outqueue.size() < 30)
				{
					outqueue.push(frame);
				}
				else
				{
					delete frame;
				}

				pthread_mutex_unlock(&outqueue_mutex);
*/
			}
			else
			{
				return -5;
			}
		}
	}
	catch (...)
	{
	}

	return -6;
}