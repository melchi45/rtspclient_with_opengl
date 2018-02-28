#ifndef H264_DECODER_H
#define H264_DECODER_H

#define H264_INBUF_SIZE 16384                                                           /* number of bytes we read per chunk */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")

typedef void(*h264_decoder_callback)(AVCodecContext *pCodecCtx, AVFrame* frame, AVPacket* pkt, int framecount, void* user);         /* the decoder callback, which will be called when we have decoded a frame */

class H264_Decoder {

public:
	H264_Decoder(h264_decoder_callback frameCallback, void* user);                         /* pass in a callback function that is called whenever we decoded a video frame, make sure to call `readFrame()` repeatedly */
	~H264_Decoder();                                                                       /* d'tor, cleans up the allocated objects and closes the codec context */
	bool load(const char* filepath, float fps = 0.0f);                                     /* load a video file which is encoded with x264 */
	bool readFrame();                                                                      /* read a frame if necessary */

	void forceFPS(float fps);
	double getFPS() const;

private:
	bool update(bool& needsMoreBytes);                                                     /* internally used to update/parse the data we read from the buffer or file */
	int readBuffer();                                                                      /* read a bit more data from the buffer */
	void decodeFrame(uint8_t* data, int size);                                             /* decode a frame we read from the buffer */

public:
	AVCodec * codec;                                                                        /* the AVCodec* which represents the H264 decoder */
	AVCodecContext* codec_context;                                                         /* the context; keeps generic state */
	AVCodecParserContext* parser;                                                          /* parser that is used to decode the h264 bitstream */
	AVFrame* picture;                                                                      /* will contain a decoded picture */
	uint8_t inbuf[H264_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];                         /* used to read chunks from the file */
	FILE* fp;                                                                              /* file pointer to the file from which we read the h264 data */
	int framecount;                                                                        /* the number of decoded frames */
	h264_decoder_callback cb_frame;                                                        /* the callback function which will receive the frame/packet data */
	void* cb_user;                                                                         /* the void* with user data that is passed into the set callback */
	uint64_t frame_timeout;                                                                /* timeout when we need to parse a new frame */
	uint64_t frame_delay;                                                                  /* delay between frames (in ns) */
	std::vector<uint8_t> buffer;                                                           /* buffer we use to keep track of read/unused bitstream data */
	float forcefps;
};

#endif