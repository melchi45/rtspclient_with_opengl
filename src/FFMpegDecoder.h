#ifndef _FFMPEG_DECODER_H_
#define _FFMPEG_DECODER_H_

// reference from 
// http://blog.chinaunix.net/uid-15063109-id-4482932.html

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

//#pragma comment(lib, "avutil.lib")
//#pragma comment(lib, "avcodec.lib")
//#pragma comment(lib, "avformat.lib")
//#pragma comment(lib, "swscale.lib")

class CDecodeCB
{
public:
	virtual void videoCB(int width, int height, uint8_t* buff, int len) = 0;
};

class FFmpegDecoder
{
public:
	FFmpegDecoder();
	~FFmpegDecoder();
	int initFFMPEG();
	int openDecoder(int width, int height, CDecodeCB* pCB);
	int closeDecoder();
	int decode_rtsp_frame(uint8_t* input, int nLen, bool bWaitIFrame = false);
private:
	bool m_bInit;
	AVCodec *decoder;
	AVCodecContext *decoder_context;
	AVFrame *decoder_picture;
//	AVFormatContext *format_context;
	struct SwsContext *img_convert_ctx;
	CDecodeCB* m_pCB;
	int m_nWidth;
	int m_nHeight;
};
#endif // _FFMPEG_DECODER_H_