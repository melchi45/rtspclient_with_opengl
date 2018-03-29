#include "H264ReadScreenEncoder.h"
#include "Frame.h"

#define USE_YUV_FRAME	1
//#define USE_RGB_FRAME	1

#define FPS				30

H264ReadScreenEncoder::H264ReadScreenEncoder()
	: FFMpeg()
	, thread_exit(0)
	, videoindex(-1)
	, fps(30)
{
	codec_id = AV_CODEC_ID_H264;
	dstWidth = 640;
	dstHeight = 320;

	intialize();
}

H264ReadScreenEncoder::~H264ReadScreenEncoder()
{
	finalize();
}

int H264ReadScreenEncoder::intialize()
{	
	FFMpeg::intialize();

	/// create codec context for encoder
	/* find the h264 video encoder */
	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

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
		exit(1);
	}

	/// create codec context for screen capture
	pScreenFormatCtx = avformat_alloc_context();
	if (pScreenFormatCtx == NULL) {
//		throw AVException(ENOMEM, "can not alloc av context");
	}

#ifdef _WIN32
#if USE_DSHOW
	//Use dshow
	//
	//Need to Install screen-capture-recorder
	//screen-capture-recorder
	//Website: http://sourceforge.net/projects/screencapturer/
	//
	AVInputFormat *ifmt = av_find_input_format("dshow");
	if (avformat_open_input(&pScreenFormatCtx, "video=screen-capture-recorder", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	//Use gdigrab
	AVDictionary* options = NULL;
	//Set some options
	//grabbing frame rate
	//av_dict_set(&options,"framerate","5",0);
	//The distance from the left edge of the screen or desktop
	//av_dict_set(&options,"offset_x","20",0);
	//The distance from the top edge of the screen or desktop
	//av_dict_set(&options,"offset_y","40",0);
	//Video frame size. The default is to capture the full screen
	//av_dict_set(&options,"video_size","640x480",0);
	AVInputFormat *ifmt = av_find_input_format("gdigrab");
	if (avformat_open_input(&pScreenFormatCtx, "desktop", ifmt, &options) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

#endif
#elif defined linux
	//Linux
	AVDictionary* options = NULL;
	//Set some options
	//grabbing frame rate
	//av_dict_set(&options,"framerate","5",0);
	//Make the grabbed area follow the mouse
	//av_dict_set(&options,"follow_mouse","centered",0);
	//Video frame size. The default is to capture the full screen
	//av_dict_set(&options,"video_size","640x480",0);
	AVInputFormat *ifmt = av_find_input_format("x11grab");
	//Grab at position 10,20
	if (avformat_open_input(&pScreenFormatCtx, ":0.0+10,20", ifmt, &options) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else
	show_avfoundation_device();
	//Mac
	AVInputFormat *ifmt = av_find_input_format("avfoundation");
	//Avfoundation
	//[video]:[audio]
	if (avformat_open_input(&pScreenFormatCtx, "1", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif

	if (avformat_find_stream_info(pScreenFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	for (int i = 0; i < pScreenFormatCtx->nb_streams; i++)
		if (pScreenFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	pScreenCodecCtx = pScreenFormatCtx->streams[videoindex]->codec;
	AVCodec * codec = avcodec_find_decoder(pScreenCodecCtx->codec_id);
	if (codec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pScreenCodecCtx, codec, NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	pthread_attr_t attr1;
	pthread_attr_init(&attr1);
	pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
	int r = pthread_create(&thread_id, NULL, run, this);
	if (r)
	{
		perror("pthread_create()");
		return -1;
	}

	// wait for threads to finish
	pthread_join(thread_id, NULL);

	m_bInit = true;

	return 0;
}

int H264ReadScreenEncoder::finalize()
{
	if (pScreenCodecCtx)
	{
		avcodec_close(pScreenCodecCtx);
		av_free(pScreenCodecCtx);
	}

	if (pScreenFormatCtx) {
		avformat_free_context(pScreenFormatCtx);
	}

	return FFMpeg::finalize();
}

void* H264ReadScreenEncoder::run(void *param)
{
	H264ReadScreenEncoder *pThis = (H264ReadScreenEncoder*)param;
	pThis->ReadFrame_from_Screenshot();
	return NULL;
}

int H264ReadScreenEncoder::ReadFrame_from_Screenshot()
{
	int ret, got_picture;
//	uint8_t *yPlane, *uPlane, *vPlane;
//	size_t yPlaneSz, uvPlaneSz;
//	int uvPitch;

	try {
		AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
		img_convert_ctx = sws_getContext(pScreenCodecCtx->width, pScreenCodecCtx->height, pScreenCodecCtx->pix_fmt, pScreenCodecCtx->width, pScreenCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

		while (!thread_exit) {
			// raed frame from screen capture device (gdigrab)
			if (av_read_frame(pScreenFormatCtx, packet) >= 0) {
				if (packet->stream_index == videoindex) {
					ret = avcodec_decode_video2(pScreenCodecCtx, pFrame, &got_picture, packet);
					if (ret < 0) {
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture)
					{
						// set image size
						srcWidth = pScreenCodecCtx->width;
						srcHeight = pScreenCodecCtx->height;
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
							pScreenCodecCtx->width, // width of source image
							pScreenCodecCtx->height, // height of source image 
							pScreenCodecCtx->pix_fmt,
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
							0, pScreenCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

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
int H264ReadScreenEncoder::WriteFrame(AVFrame* frame)
{
	AVPacket avpkt;
	int got_output;
	int ret;

//	frame->linesize[0] = -1;

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

		if (outqueue.size()<30)
		{
			printf("complete add frame: %d", outqueue.size());
			outqueue.push(data);
		}
		else
		{
			delete data;
		}

		pthread_mutex_unlock(&outqueue_mutex);

		av_free_packet(&avpkt);
	}

}