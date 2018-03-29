#include "FFMpeg.h"

FFMpeg::FFMpeg()
	: frame_count(0)
	, m_bInit(false)
	, img_convert_ctx(NULL)
	, codec_id(AV_CODEC_ID_NONE)
	, sws_flags(SWS_BICUBIC) // https://blog.csdn.net/leixiaohua1020/article/details/42134965
	, pFormatCtx(NULL)
	, pFrame(NULL)
	, pStream(NULL)
{
	//intialize();
	pthread_mutex_init(&inqueue_mutex, NULL);
	pthread_mutex_init(&outqueue_mutex, NULL);
}

FFMpeg::~FFMpeg()
{
	//finalize();
}

int FFMpeg::intialize()
{
	// Intialize FFmpeg enviroment
	av_register_all();
	avdevice_register_all();
	avcodec_register_all();
	avformat_network_init();

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
	pFrame = av_frame_alloc();
#else
	pFrame = avcodec_alloc_frame();
#endif

	pFormatCtx = avformat_alloc_context();

//	m_bInit = true;

	return 0;
}

int FFMpeg::finalize()
{
	if(img_convert_ctx)
		sws_freeContext(img_convert_ctx);

	if (pStream) {
		avcodec_close(pStream->codec);
	}

	if (pFrame) {
		av_freep(&(pFrame->data[0]));
		av_frame_unref(pFrame);
		av_free(pFrame);
	}

	if (pCodecCtx)
	{
		avcodec_close(pCodecCtx);
		av_free(pCodecCtx);
	}

	if (pFormatCtx) {
		avformat_free_context(pFormatCtx);
	}

	m_bInit = false;

	return 0;
}

void FFMpeg::setOnframeCallbackFunction(std::function<void(void*)> func)
{
	onFrame = func;
}

int FFMpeg::save_frame_as_ppm(AVFrame *pframe)
{
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "frame_%06ld.ppm", frame_count);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return -7;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", dstWidth, dstHeight);

	// Write pixel data
	// Write pixel data 
	for (int y = 0; y< dstHeight; y++)
		fwrite(pframe->data[0] + y * pframe->linesize[0], 1, dstWidth * 3, pFile);

	// Close file
	fclose(pFile);
}

/**
* save yuv420p frame [YUV]
*/
int FFMpeg::save_frame_as_yuv420p(AVFrame *pframe)
{
	FILE *pFile;
	char szFilename[32];

	// Open file
	sprintf(szFilename, "frame_%06ld.yuv", frame_count);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return -7;

	int width = pframe->width, height = pframe->height;
	int height_half = height / 2, width_half = width / 2;
	int y_wrap = pframe->linesize[0];
	int u_wrap = pframe->linesize[1];
	int v_wrap = pframe->linesize[2];

	unsigned char *y_buf = pframe->data[0];
	unsigned char *u_buf = pframe->data[1];
	unsigned char *v_buf = pframe->data[2];

	//save y  
	for (int i = 0; i < height; i++)
		fwrite(y_buf + i * y_wrap, 1, width, pFile);
	fprintf(stderr, "===>save Y success\n");
	//save u  
	for (int i = 0; i < height_half; i++)
		fwrite(u_buf + i * u_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save U success\n");
	//save v  
	for (int i = 0; i < height_half; i++)
		fwrite(v_buf + i * v_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save V success\n");

	fflush(pFile);

	// Close file
	fclose(pFile);
}

/**
* save yuv422p frame [YUV]
*/
int FFMpeg::save_frame_as_yuv422p(AVFrame *pframe)
{
	FILE *pFile;
	char szFilename[32];

	// Open file
	sprintf(szFilename, "frame_%06ld.yuv", frame_count);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return -7;

	int width = pframe->width, height = pframe->height;
	int height_half = height / 2, width_half = width / 2;
	int y_wrap = pframe->linesize[0];
	int u_wrap = pframe->linesize[1];
	int v_wrap = pframe->linesize[2];

	unsigned char *y_buf = pframe->data[0];
	unsigned char *u_buf = pframe->data[1];
	unsigned char *v_buf = pframe->data[2];

	//save y  
	for (int i = 0; i < height; i++)
		fwrite(y_buf + i * y_wrap, 1, width, pFile);
	fprintf(stderr, "===>save Y success\n");
	//save u  
	for (int i = 0; i < height; i++)
		fwrite(u_buf + i * u_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save U success\n");
	//save v  
	for (int i = 0; i < height; i++)
		fwrite(v_buf + i * v_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save V success\n");

	fflush(pFile);

	// Close file
	fclose(pFile);
}

int FFMpeg::save_frame_as_jpeg(AVFrame *pframe)
{
	bool is_convert = false;
	AVFrame* frame = NULL;

	// if AVFrame is RGB pixel format, change to YUV430P pixel format
	if (pframe->format == AV_PIX_FMT_RGB24) {
		frame = frame_rgb_yuv420p(pframe);
		is_convert = true;
	} else {
		frame = pframe;
	}

	char szFilename[32];
	//sprintf_s(szFilename, sizeof(szFilename), "frame_%06ld.jpg", frame_count);
	sprintf(szFilename, "frame_%06ld.jpg", frame_count);

	AVFormatContext* format_context = avformat_alloc_context();

	format_context->oformat = av_guess_format("mjpeg", NULL, NULL);
	if (avio_open(&format_context->pb, szFilename, AVIO_FLAG_READ_WRITE) < 0) {
		//printf("Couldn't open output file.");
		//fEnviron << "Couldn't open output file.";
		return -1;
	}

	// Begin Output some information
	av_dump_format(format_context, 0, szFilename, 1);
	// End Output some information

	AVStream* stream = avformat_new_stream(format_context, 0);
	if (stream == NULL) {
		return -1;
	}

	AVCodecContext* codec_context = stream->codec;

	codec_context->codec_id = format_context->oformat->video_codec;
	codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_context->pix_fmt = AV_PIX_FMT_YUVJ420P;
	codec_context->width = dstWidth;
	codec_context->height = dstHeight;
	codec_context->time_base.num = 1;
	codec_context->time_base.den = 25;

	AVCodec* codec = avcodec_find_encoder(codec_context->codec_id);
	if (!codec) {
		//printf("Codec not found.");
		//fEnviron << "Codec not found.";
		return -1;
	}
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		//printf("Could not open codec.");
		//fEnviron << "Could not open codec.";
		return -1;
	}

	//Write Header
	avformat_write_header(format_context, NULL);

	int y_size = codec_context->width * codec_context->height;

	//Encode
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// 
	int got_picture = 0;
	int ret = avcodec_encode_video2(codec_context, &pkt, frame, &got_picture);
	if (ret < 0) {
		//printf("Encode Error.\n");
		//fEnviron << "Encode Error.";
		return -1;
	}
	if (got_picture == 1) {
		//pkt.stream_index = stream->index;
		ret = av_write_frame(format_context, &pkt);
	}

	av_free_packet(&pkt);

	//Write Trailer
	av_write_trailer(format_context);

	//printf("Encode Successful.\n");
	//fEnviron << "Image Frame Encode Successful.";

	// if frame is conver image, free memory
	if (is_convert) {
		if (frame) {
			av_freep(&(frame->data[0]));
			av_frame_unref(frame);
			av_free(frame);
		}
	}

	// stream object is not null, free memory
	if (stream) {
		avcodec_close(stream->codec);
	}

	// codec context is not null, free memory
	// this routine was deleted, because of codec_context is linked format_context.
	// so, av_free is duplicated on avformat_free_context
//	if (codec_context) {
//		avcodec_close(codec_context);
//		av_free(codec_context);
//	}

	// format context is not null, free memory
	if (format_context) {
		avio_close(format_context->pb);
		avformat_free_context(format_context);
	}

	return 0;
}

int FFMpeg::save_frame_as_png(AVFrame *pframe)
{
	char szFilename[32];
	//sprintf_s(szFilename, sizeof(szFilename), "frame_%06ld.jpg", frame_count);
	sprintf(szFilename, "frame_%06ld.png", frame_count);

	AVFormatContext* pFormatCtx = avformat_alloc_context();

	pFormatCtx->oformat = av_guess_format("png", NULL, NULL);
	if (avio_open(&pFormatCtx->pb, szFilename, AVIO_FLAG_READ_WRITE) < 0) {
		//printf("Couldn't open output file.");
		//fEnviron << "Couldn't open output file.";
		return -1;
	}

	// Begin Output some information
	av_dump_format(pFormatCtx, 0, szFilename, 1);
	// End Output some information

	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	AVCodecContext* pCodecCtx = pAVStream->codec;

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = pCodecCtx->width;
	pCodecCtx->height = pCodecCtx->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		//printf("Codec not found.");
		//fEnviron << "Codec not found.";
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		//printf("Could not open codec.");
		//fEnviron << "Could not open codec.";
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// 
	int got_picture = 0;
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pframe, &got_picture);
	if (ret < 0) {
		//printf("Encode Error.\n");
		//fEnviron << "Encode Error.";
		return -1;
	}
	if (got_picture == 1) {
		//pkt.stream_index = pAVStream->index;
		ret = av_write_frame(pFormatCtx, &pkt);
	}

	av_free_packet(&pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);

	//printf("Encode Successful.\n");
	//fEnviron << "Image Frame Encode Successful.";

	if (pAVStream) {
		avcodec_close(pAVStream->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}

AVFrame* FFMpeg::frame_rgb_yuv420p(AVFrame* pframe)
{
	// Create YUV buffer
	int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pframe->width, pframe->height);
	uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
	AVFrame *pFrameYUV = av_frame_alloc();
#else
	AVFrame *pFrameYUV = avcodec_alloc_frame();
#endif
	// set H.264 context info to frame for encoder
	pFrameYUV->format = AV_PIX_FMT_YUV420P;
	pFrameYUV->width = pframe->width;
	pFrameYUV->height = pframe->height;

	avpicture_fill((AVPicture *)pFrameYUV, buffer, AV_PIX_FMT_YUV420P, pFrameYUV->width, pFrameYUV->height);

	// get scaling context from context image size to destination image size
	/*	SwsContext* img_convert_ctx = sws_getCachedContext(img_convert_ctx,
	pFrame->width, // width of source image
	pFrame->height, // height of source image
	AV_PIX_FMT_RGB24,
	pFrameYUV->width, // width of destination image
	pFrameYUV->height, // height of destination image
	AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);*/
	SwsContext* convert_ctx = sws_getContext(
		pframe->width, // width of source image
		pframe->height, // height of source image 
		AV_PIX_FMT_RGB24,
		pFrameYUV->width, // width of destination image
		pFrameYUV->height, // height of destination image
		AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	if (convert_ctx == NULL)
	{
		fprintf(stderr, "Cannot initialize the conversion context [%s:%d]\n", __FILE__, __LINE__);
		return NULL;
	}
	// scaling
	sws_scale(convert_ctx, pframe->data, pframe->linesize,
		0, pFrameYUV->height, pFrameYUV->data, pFrameYUV->linesize);

	return pFrameYUV;
}

AVFrame* FFMpeg::frame_yuv420p_rgb(AVFrame* pframe)
{
	// Create YUV buffer
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pframe->width, pframe->height);
	uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
	AVFrame *pFrameRGB = av_frame_alloc();
#else
	AVFrame *pFrameYUV = avcodec_alloc_frame();
#endif
	// set H.264 context info to frame for encoder
	pFrameRGB->format = AV_PIX_FMT_RGB24;
	pFrameRGB->width = pframe->width;
	pFrameRGB->height = pframe->height;

	avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pFrameRGB->width, pFrameRGB->height);

	// get scaling context from context image size to destination image size
	/*	SwsContext* img_convert_ctx = sws_getCachedContext(img_convert_ctx,
	pFrame->width, // width of source image
	pFrame->height, // height of source image
	AV_PIX_FMT_RGB24,
	pFrameYUV->width, // width of destination image
	pFrameYUV->height, // height of destination image
	AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);*/
	SwsContext* convert_ctx = sws_getContext(
		pFrame->width, // width of source image
		pFrame->height, // height of source image 
		AV_PIX_FMT_YUV420P,
		pFrameRGB->width, // width of destination image
		pFrameRGB->height, // height of destination image
		AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
	if (convert_ctx == NULL)
	{
		fprintf(stderr, "Cannot initialize the conversion context [%s:%d]\n", __FILE__, __LINE__);
		return NULL;
	}
	// scaling
	sws_scale(convert_ctx, pframe->data, pframe->linesize,
		0, pFrameRGB->height, pFrameRGB->data, pFrameRGB->linesize);

	return pFrameRGB;
}


Frame* FFMpeg::PopFrame()
{
	if (!outqueue.empty())
	{
		Frame * frame;
		frame = outqueue.front();
		return frame;
	}
}