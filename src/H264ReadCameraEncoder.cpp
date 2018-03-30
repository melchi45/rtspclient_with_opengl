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
#include "H264ReadCameraEncoder.h"
#include "Frame.h"

#define USE_YUV_FRAME	1
//#define USE_RGB_FRAME	1
#define FPS		30

#if USE_LIVE555
FFMpegEncoder *
H264ReadCameraEncoder::createNew(UsageEnvironment &env)
{
	return new H264ReadCameraEncoder(env);
}

H264ReadCameraEncoder::H264ReadCameraEncoder(UsageEnvironment &env)
	: FFMpegEncoder(env)
	, thread_exit(0)
	, videoindex(-1)
	, fps(30)
{
	codec_id = AV_CODEC_ID_H264;
	dstWidth = 640;
	dstHeight = 320;

	intialize();
}
#else
H264ReadCameraEncoder::H264ReadCameraEncoder()
	: FFMpegEncoder()
	, thread_exit(0)
	, videoindex(-1)
	, fps(30)
{
	codec_id = AV_CODEC_ID_H264;
	dstWidth = 640;
	dstHeight = 320;

	intialize();
}
#endif

H264ReadCameraEncoder::~H264ReadCameraEncoder()
{
	finalize();
}

int H264ReadCameraEncoder::intialize()
{
	return FFMpegEncoder::intialize();
}

int H264ReadCameraEncoder::SetupCodec()
{
	pCodecCtx->codec_id = AV_CODEC_ID_H264;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	/* put sample parameters */
	pCodecCtx->bit_rate = 400000;
	/* resolution must be a multiple of two */
	pCodecCtx->width = dstWidth;
	pCodecCtx->height = dstHeight;
	/* frames per second */
	AVRational timebase;
	timebase.num = 1;
	timebase.den = fps;
	pCodecCtx->time_base = timebase;

	AVRational framerate;
	framerate.num;
	framerate.den = fps;

	pCodecCtx->framerate = framerate;

	pCodecCtx->gop_size = 12; /* emit one intra frame every ten frames */
	pCodecCtx->max_b_frames = 2;
	pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if (codec_id == AV_CODEC_ID_H264) {
		//av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
		av_opt_set(pCodecCtx->priv_data, "profile", "baseline", 0);
	}

	/* open it */
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		fprintf(stderr, "avcodec_open failed for h.264 encode\n");
		return - 3;
	}

	/// create codec context for screen capture
	pSourceFormatCtx = avformat_alloc_context();
	if (pSourceFormatCtx == NULL) {
		//		throw AVException(ENOMEM, "can not alloc av context");
		return -4;
	}

#ifdef _WIN32
#if USE_DSHOW
	AVInputFormat *ifmt = av_find_input_format("dshow");
	//Set own video device's name
	if (avformat_open_input(&pFormatCtx, "video=Integrated Camera", ifmt, NULL) != 0) {
		//printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	AVInputFormat *ifmt = av_find_input_format("vfwcap");
	if (ifmt == NULL)
	{
		//printf("can not find_input_format\n");
		return -5;
	}
	char *dev_name = "0";
	if (avformat_open_input(&pSourceFormatCtx, dev_name, ifmt, NULL) != 0) {
		//printf("Couldn't open input stream.\n");
		return -6;
	}
#endif
#elif defined linux
	//Linux
	AVInputFormat *ifmt = av_find_input_format("video4linux2");
	if (avformat_open_input(&pSourceFormatCtx, "/dev/video0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	show_avfoundation_device();
	//Mac
	AVInputFormat *ifmt = av_find_input_format("avfoundation");
	//Avfoundation
	//[video]:[audio]
	if (avformat_open_input(&pSourceFormatCtx, "0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif

	if (avformat_find_stream_info(pSourceFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -7;
	}

	for (int i = 0; i < pSourceFormatCtx->nb_streams; i++)
		if (pSourceFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	pSourceCodecCtx = pSourceFormatCtx->streams[videoindex]->codec;
	AVCodec* codec = avcodec_find_decoder(pSourceCodecCtx->codec_id);
	if (codec == NULL)
	{
		printf("Codec not found.\n");
		return -8;
	}
	if (avcodec_open2(pSourceCodecCtx, codec, NULL)<0)
	{
		printf("Could not open codec.\n");
		return -9;
	}

	pthread_attr_t attr1;
	pthread_attr_init(&attr1);
	pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
	int r = pthread_create(&thread_id, NULL, run, this);
	if (r)
	{
		perror("pthread_create()");
		return -10;
	}

	// wait for threads to finish
	pthread_join(thread_id, NULL);

	m_bInit = true;

	return 0;
}

int H264ReadCameraEncoder::finalize()
{	
	return FFMpegEncoder::finalize();
}

int H264ReadCameraEncoder::ReadFrame()
{
	int ret, got_picture;

	try {
		AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
		img_convert_ctx = sws_getContext(
			pSourceCodecCtx->width,
			pSourceCodecCtx->height,
			pSourceCodecCtx->pix_fmt,
			pSourceCodecCtx->width,
			pSourceCodecCtx->height,
			AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

		while (!thread_exit) {
			if (av_read_frame(pSourceFormatCtx, packet) >= 0) {
				if (packet->stream_index == videoindex) {
					ret = avcodec_decode_video2(pSourceCodecCtx, pFrame, &got_picture, packet);
					if (ret < 0) {
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture) {
						// set image size
						srcWidth = pSourceCodecCtx->width;
						srcHeight = pSourceCodecCtx->height;
						// calculate byte size from rgb image by width and height

						int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, dstWidth, dstHeight);
						uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
						AVFrame *pFrameYUV = av_frame_alloc();
#else
						AVFrame *pFrameYUV = avcodec_alloc_frame();
#endif
						// set H.264 context info to frame for encoder
						pFrameYUV->format = pCodecCtx->pix_fmt;
						pFrameYUV->width = pCodecCtx->width;
						pFrameYUV->height = pCodecCtx->height;
						pFrameYUV->pts = frame_count++;

						avpicture_fill((AVPicture *)pFrameYUV, buffer, AV_PIX_FMT_YUV420P, dstWidth, dstHeight);

						// get scaling context from context image size to destination image size
						img_convert_ctx = sws_getCachedContext(img_convert_ctx,
							pSourceCodecCtx->width, // width of source image
							pSourceCodecCtx->height, // height of source image 
							pSourceCodecCtx->pix_fmt,
							dstWidth, // width of destination image
							dstHeight, // height of destination image
							AV_PIX_FMT_YUV420P, getSWSType(), NULL, NULL, NULL);
						if (img_convert_ctx == NULL)
						{
							fprintf(stderr, "Cannot initialize the conversion context [%s:%d]\n", __FILE__, __LINE__);
							return -4;
						}
						// scaling
						sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
							0, pSourceCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

						save_frame_as_jpeg(pFrameYUV);

						if (WriteFrame(pFrameYUV) < 0) {
							fprintf(stderr, "Cannot encode frame [%s:%d]\n", __FILE__, __LINE__);
							return -4;
						}

						av_free(pFrameYUV);
						av_free(buffer);
					}
					else
					{
						return -5;
					}
				}
				if (packet != NULL)
					av_free_packet(packet);
			}
			else {
				//Exit Thread
				thread_exit = 1;
			}
		}
	}
	catch (...)
	{
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="frame">yuv frame pointer</param>
/// <returns>error number</returns>
/// <reference>
/// https://stackoverflow.com/questions/2940671/how-does-one-encode-a-series-of-images-into-h264-using-the-x264-c-api
/// https://stackoverflow.com/questions/28727772/ffmpeg-c-api-h-264-encoding-mpeg2-ts-streaming-problems
/// </reference>
int H264ReadCameraEncoder::WriteFrame(AVFrame* frame)
{
	AVPacket avpkt;
	int got_output;
	int ret;

	// ready to packet data from H.264 encoder
	av_init_packet(&avpkt);
	avpkt.size = 0;
	avpkt.data = NULL;

	// set to i frame for encoder when frame_count divide with fps
	if((frame->pts % fps) == 0) {
		frame->key_frame = 1;
		frame->pict_type = AV_PICTURE_TYPE_I;
	} else {
		frame->key_frame = 0;
		frame->pict_type = AV_PICTURE_TYPE_P;
	}

	/* encode the image */
	ret = avcodec_encode_video2(pCodecCtx, &avpkt, frame, &got_output);
	if (ret < 0)
	{
		fprintf(stderr, "Error encoding frame [%s:%d]\n", __FILE__, __LINE__);
		return -1;
	}

	if (got_output)
	{
//		avpkt.stream_index = frame_count;
		Frame * data = new Frame();
		data->dataPointer = new uint8_t[avpkt.size];
		data->dataSize = avpkt.size - 4;
		data->frameID = frame_count;
		data->width = dstWidth;
		data->height = dstHeight;

		memcpy(data->dataPointer, avpkt.data + 4, avpkt.size - 4);

		pthread_mutex_lock(&outqueue_mutex);

		if (outqueue.size()<30) {
			printf("complete add frame: %d", outqueue.size());
			outqueue.push(data);
		} else {
			delete data;
		}

		pthread_mutex_unlock(&outqueue_mutex);

		av_free_packet(&avpkt);

		if (m_plistener != NULL) {
			((EncodeListener*)m_plistener)->onEncoded();
		}
		else if (onEncoded != NULL) {
			onEncoded();
		}
	}
}