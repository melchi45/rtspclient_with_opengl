#include "FFMpegEncoder.h"
#include "Frame.h"

#define USE_YUV_FRAME	1
//#define USE_RGB_FRAME	1
#define FPS		30

FFMpegEncoder::FFMpegEncoder()
	: FFMpeg()
	, thread_exit(0)
{
}

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