#include "H264ReadCameraEncoder.h"

H264ReadCameraEncoder::H264ReadCameraEncoder()
	: FFMpeg()
	, thread_exit(0)
	, videoindex(-1)
{
	codec_id = AV_CODEC_ID_H264;
	dstWidth = 640;
	dstHeight = 320;

	intialize();
}

H264ReadCameraEncoder::~H264ReadCameraEncoder()
{
	finalize();
}

int H264ReadCameraEncoder::intialize()
{	
	FFMpeg::intialize();

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
		return -1;
	}
	char *dev_name = "0";
	if (avformat_open_input(&pFormatCtx, dev_name, ifmt, NULL) != 0) {
		//printf("Couldn't open input stream.\n");
		return -1;
	}
#endif
#elif defined linux
//Linux
AVInputFormat *ifmt = av_find_input_format("video4linux2");
if (avformat_open_input(&pFormatCtx, "/dev/video0", ifmt, NULL) != 0) {
	printf("Couldn't open input stream.\n");
	return -1;
}
#else
show_avfoundation_device();
//Mac
AVInputFormat *ifmt = av_find_input_format("avfoundation");
//Avfoundation
//[video]:[audio]
if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0) {
	printf("Couldn't open input stream.\n");
	return -1;
}
#endif

	if (avformat_find_stream_info(pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	for (int i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
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

int H264ReadCameraEncoder::finalize()
{
	return FFMpeg::finalize();
}

void* H264ReadCameraEncoder::run(void *param)
{
	H264ReadCameraEncoder *pThis = (H264ReadCameraEncoder*)param;
	pThis->ReadFrame_from_Camera();
	return NULL;
}

int H264ReadCameraEncoder::ReadFrame_from_Camera()
{
	int ret, got_picture;
	uint8_t *yPlane, *uPlane, *vPlane;
	size_t yPlaneSz, uvPlaneSz;
	int uvPitch;

	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	// set up YV12 pixel array (12 bits per pixel)
	yPlaneSz = pCodecCtx->width * pCodecCtx->height;
	uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
	yPlane = (uint8_t*)malloc(yPlaneSz);
	uPlane = (uint8_t*)malloc(uvPlaneSz);
	vPlane = (uint8_t*)malloc(uvPlaneSz);
	if (!yPlane || !uPlane || !vPlane) {
		//fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
		//exit(1);
	}
	uvPitch = pCodecCtx->width / 2;

	while (!thread_exit) {

		if (av_read_frame(pFormatCtx, packet) >= 0) {
			if (packet->stream_index == videoindex) {
				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
				if (ret < 0) {
					printf("Decode Error.\n");
					return -1;
				}
				if (got_picture) {
					AVPicture pict;
					pict.data[0] = yPlane;
					pict.data[1] = uPlane;
					pict.data[2] = vPlane;
					pict.linesize[0] = pCodecCtx->width;
					pict.linesize[1] = uvPitch;
					pict.linesize[2] = uvPitch;

					sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pict.data, pict.linesize);
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

void H264ReadCameraEncoder::WriteFrame(uint8_t * RGBFrame)
{

}