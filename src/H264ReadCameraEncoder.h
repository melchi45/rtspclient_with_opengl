#ifndef _H264_READ_CAMERA_ENCODER_H_
#define _H264_READ_CAMERA_ENCODER_H_

#include "FFmpeg.h"

class H264ReadCameraEncoder : public FFMpeg
{
public:
	H264ReadCameraEncoder();
	virtual ~H264ReadCameraEncoder();
	
	virtual int intialize();
	virtual int finalize();

	static void *run(void *param);
//	int openDecoder(int width, int height, CDecodeCB* pCB);
//	int encode(uint8_t* input, int nLen, bool bWaitIFrame = false);

private:
	pthread_t thread_id;
	int thread_exit;
	int videoindex;

	void WriteFrame(uint8_t * RGBFrame);
	int ReadFrame_from_Camera();

protected:
	AVCodec * pAVCodec;
};
#endif // _H264_READ_CAMERA_ENCODER_H_