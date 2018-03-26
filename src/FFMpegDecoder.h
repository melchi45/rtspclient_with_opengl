#ifndef _FFMPEG_DECODER_H_
#define _FFMPEG_DECODER_H_

// reference from 
// http://blog.chinaunix.net/uid-15063109-id-4482932.html

#include <functional>	// for std::function

extern "C" {
	// disable warnings about badly formed documentation from ffmpeg, which don't need at all
#pragma warning(disable:4635)
	// disable warning about conversion int64 to int32
#pragma warning(disable:4244)
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>

#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
}

class RTSPClient;
class UsageEnvironment;
class CDecodeCB
{
public:
	virtual void videoCB(int width, int height, uint8_t* buff, int len, int pitch, RTSPClient* pClient) = 0;
};

class FFmpegDecoder
{
public:
	FFmpegDecoder(UsageEnvironment& env);
	~FFmpegDecoder();
	
	int intialize();
	int finalize();

	int openDecoder(int width, int height, CDecodeCB* pCB);
	void setClient(RTSPClient* client);
	int decode_rtsp_frame(uint8_t* input, int nLen, bool bWaitIFrame = false);

private:
	int save_frame_as_jpeg(AVFrame *pframe);
	int save_frame_as_ppm(AVFrame *pframe);

private:
	bool m_bInit;
	AVCodec *decoder;
	AVCodecContext *decoder_context;
	AVFrame *decoder_picture;
	long	frame_count;
//	AVFormatContext *format_context;
	struct SwsContext *img_convert_ctx;
	CDecodeCB* m_pCB;
	int m_nWidth;
	int m_nHeight;
	RTSPClient* pClient;
	UsageEnvironment& fEnviron;
};
#endif // _FFMPEG_DECODER_H_