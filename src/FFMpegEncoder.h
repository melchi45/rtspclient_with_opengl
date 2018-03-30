#ifndef _FFMPEG_ENCODER_H_
#define _FFMPEG_ENCODER_H_

#include "FFmpeg.h"

class FFMpegEncoder : public FFMpeg
{
public:
	FFMpegEncoder();
	virtual ~FFMpegEncoder();
	
	virtual int intialize();
	virtual int finalize();

	char GetFrame(uint8_t** FrameBuffer, unsigned int *FrameSize);
	char ReleaseFrame();

private:
	pthread_t thread_id;
	int thread_exit;
	int videoindex;
	int fps;

	virtual int ReadFrame() = 0;
	virtual int WriteFrame(AVFrame* frame) = 0;

protected:
	static void *run(void *param);
	virtual int SetupCodec() = 0;

protected:
	AVFormatContext* pSourceFormatCtx;
	AVCodecContext* pSourceCodecCtx;
};
#endif // _FFMPEG_ENCODER_H_