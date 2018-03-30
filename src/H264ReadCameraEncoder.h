#ifndef _H264_READ_CAMERA_ENCODER_H_
#define _H264_READ_CAMERA_ENCODER_H_

#include "FFMpegEncoder.h"

class H264ReadCameraEncoder : public FFMpegEncoder
{
public:
	H264ReadCameraEncoder();
	virtual ~H264ReadCameraEncoder();
	
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
#endif // _H264_READ_CAMERA_ENCODER_H_