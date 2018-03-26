#ifndef _H264DECODER_H_
#define _H264DECODER_H_

#include "FFmpeg.h"

class H264Decoder : public FFMpeg
{
public:
	H264Decoder();
	virtual ~H264Decoder();
	
	virtual int intialize();
	virtual int finalize();

//	int openDecoder(int width, int height, CDecodeCB* pCB);
	virtual int decode(uint8_t* input, int nLen, bool bWaitIFrame = false);
};
#endif // _H264DECODER_H_