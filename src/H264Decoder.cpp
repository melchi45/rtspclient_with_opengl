#include "H264Decoder.h"
#include "log_utils.h"

#define ROXLU_IMPLEMENTATION
#define ROXLU_USE_MATH
#include <tinylib.h>

H264_Decoder::H264_Decoder(h264_decoder_callback frameCallback, void* user)
	: codec(NULL)
	, codec_context(NULL)
	, parser(NULL)
	, fp(NULL)
	, framecount(0)
	, cb_frame(frameCallback)
	, cb_user(user)
	, frame_timeout(0)
	, frame_delay(0)
{
	// avcodec init
	avcodec_register_all();
	av_register_all();
}

H264_Decoder::~H264_Decoder() {

	if (parser) {
		av_parser_close(parser);
		parser = NULL;
	}

	if (codec_context) {
		avcodec_close(codec_context);
		av_free(codec_context);
		codec_context = NULL;
	}

	if (picture) {
		av_free(picture);
		picture = NULL;
	}

	if (fp) {
		fclose(fp);
		fp = NULL;
	}

	cb_frame = NULL;
	cb_user = NULL;
	framecount = 0;
	frame_timeout = 0;
}

AVPixelFormat pickDecodeFormat(AVCodecContext *s, const AVPixelFormat *fmt)
{
	return AV_PIX_FMT_YUV420P;
}

bool H264_Decoder::load(const char* filepath, float fps)
{
	// create AVCodec
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		log_error("Error: cannot find the h264 codec: %s", filepath);
		return false;
	}

	// alloc context
	codec_context = avcodec_alloc_context3(codec);
	if (codec->capabilities & CODEC_CAP_TRUNCATED) {
		codec_context->flags |= CODEC_FLAG_TRUNCATED;
	}

	codec_context->get_format = pickDecodeFormat;

	// verify
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		log_error("Error: could not open codec.");
		return false;
	}

	fp = fopen(filepath, "rb");
	if (!fp) {
		log_error("Error: cannot open: %s\n", filepath);
		return false;
	}

	// init parser
	picture = av_frame_alloc();
	parser = av_parser_init(AV_CODEC_ID_H264);
	if (!parser) {
		log_error("Erorr: cannot create H264 parser.\n");
		return false;
	}

	framecount = 0;

	if (fps > 0.0001f) {
		frame_delay = (1.0f / fps) * 1000ull * 1000ull * 1000ull;
		frame_timeout = rx_hrtime() + frame_delay;
	}

	// kickoff reading...
	readBuffer();

	return true;
}

void H264_Decoder::forceFPS(float fps)
{
	forcefps = fps;
}

double H264_Decoder::getFPS() const
{
	if (forcefps)
		return forcefps;

	AVRational rational = codec_context->time_base;
	return av_q2d(rational);
}

bool H264_Decoder::readFrame() 
{
	uint64_t now = rx_hrtime();
	if (now < frame_timeout) {
		return false;
	}

	bool needs_more = false;

	while (!update(needs_more)) {
		if (needs_more) {
			readBuffer();
		}
	}

	// it may take some 'reads' before we can set the fps
	if (frame_timeout == 0 && frame_delay == 0) {
		double fps = av_q2d(codec_context->time_base);
		if (fps > 0.0) {
			frame_delay = fps * 1000ull * 1000ull * 1000ull;
		}
	}

	if (frame_delay > 0) {
		frame_timeout = rx_hrtime() + frame_delay;
	}

	return true;
}

void H264_Decoder::decodeFrame(uint8_t* data, int size)
{
	AVPacket pkt;
	int got_picture = 0;
	int len = 0;

	av_init_packet(&pkt);

	pkt.data = data;
	pkt.size = size;

	len = avcodec_decode_video2(codec_context, picture, &got_picture, &pkt);
	if (len < 0) {
		log_error("Error while decoding a frame.\n");
	}

	if (got_picture == 0) {
		return;
	}

	++framecount;

	if (cb_frame) {
		cb_frame(codec_context, picture, &pkt, framecount, cb_user);
	}
}

int H264_Decoder::readBuffer() 
{
	int bytes_read = (int)fread(inbuf, 1, H264_INBUF_SIZE, fp);

	if (bytes_read) {
		std::copy(inbuf, inbuf + bytes_read, std::back_inserter(buffer));
	}

	return bytes_read;
}

bool H264_Decoder::update(bool& needsMoreBytes)
{
	needsMoreBytes = false;

	if (!fp) {
		log_error("Cannot update .. file not opened...\n");
		return false;
	}

	if (buffer.size() == 0) {
		needsMoreBytes = true;
		return false;
	}

	uint8_t* data = NULL;
	int size = 0;
	int len = av_parser_parse2(parser, codec_context, &data, &size,
		&buffer[0], buffer.size(), 0, 0, AV_NOPTS_VALUE);

	if (size == 0 && len >= 0) {
		needsMoreBytes = true;
		return false;
	}

	if (len) {
		decodeFrame(&buffer[0], size);
		buffer.erase(buffer.begin(), buffer.begin() + len);
		return true;
	}

	return false;
}