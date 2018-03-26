#ifndef _FFMPEG_H_
#define _FFMPEG_H_

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

/* Fallback support for older libavcodec versions */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 59, 100)
#define AV_CODEC_ID_H264 CODEC_ID_H264
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(56, 34, 2)
#define AV_CODEC_FLAG_LOOP_FILTER CODEC_FLAG_LOOP_FILTER
#define AV_CODEC_CAP_TRUNCATED CODEC_CAP_TRUNCATED
#define AV_CODEC_FLAG_TRUNCATED CODEC_FLAG_TRUNCATED
#endif

#if LIBAVUTIL_VERSION_MAJOR < 52
#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#endif

class DecodeListener
{
public:
	virtual void onFrame(uint8_t* buff, int length, int width, int height, int pitch) = 0;
};

class FFMpeg
{
public:
	FFMpeg();
	virtual ~FFMpeg();
	
	virtual int intialize();
	virtual int finalize();

	void setOnframeCallbackFunction(std::function<void(uint8_t *, int, int, int, int)> func);

	void setListener(DecodeListener* listener) { m_plistener = listener; }
	//void setAVCodec(AVCodecID codec) { codec_id = codec; }
	//AVCodecID& geAVtCodec() { return codec_id;  }

//	void setAVCodecContext(AVCodecContext* ctx) { pCodecCtx = ctx; }
//	AVCodecContext* getAVCodecContext() { return pCodecCtx; }

//	void setAVFrame(AVFrame* frame) { pFrame = frame; }
//	AVFrame* getAVFrame() { return pFrame; }

	void increase_frame_count() { frame_count++; }

	void setDstWidth(int width) { dstWidth = width; }
	void setDstHeight(int height) { dstHeight = height; }
	void setRescaleSize(int width, int height) { dstWidth = width; dstHeight = height; }
	int getWidth() { return dstWidth; }
	int getHeight() { return dstHeight; }

	void setSWSType(int type) { sws_flags = type; }
	int getSWSType() { return sws_flags; }

protected:
	int save_frame_as_jpeg(AVFrame *pframe);
	int save_frame_as_ppm(AVFrame *pframe);

protected:
	bool m_bInit;
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	struct SwsContext *img_convert_ctx; 
	AVCodecID	codec_id;
	AVFrame *pFrame;

	DecodeListener* m_plistener;
	std::function<void(uint8_t *, int, int, int, int)> onFrame;

	int srcWidth, dstWidth;
	int srcHeight, dstHeight;

private:
	int sws_flags;
	int frame_count;	
};
#endif // _FFMPEG_H_