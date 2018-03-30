#ifndef _H264_READ_SCREEN_ENCODER_H_
#define _H264_READ_SCREEN_ENCODER_H_

#include "FFMpegEncoder.h"

class H264ReadScreenEncoder : public FFMpegEncoder
{
public:
	H264ReadScreenEncoder();
	virtual ~H264ReadScreenEncoder();
	
	virtual int intialize();
	virtual int finalize();

protected:
	virtual int SetupCodec();

private:
	pthread_t thread_id;
	int thread_exit;
	int videoindex;
	int fps;

	virtual int ReadFrame();
	virtual int WriteFrame(AVFrame* frame);
};
#endif // _H264_READ_SCREEN_ENCODER_H_