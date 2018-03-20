#include "FFMpegDecoder.h"
//#include "log_utils.h"
#include "liveMedia.hh"

//#define SAVE_AVFRAME_TO_JPEG		1

static int sws_flags = SWS_BICUBIC;

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

FFmpegDecoder::FFmpegDecoder(UsageEnvironment& env)
	: frame_count(0)
	, m_bInit(false)
	, img_convert_ctx(NULL)
	, pClient(NULL)
	, fEnviron(env)
{
}

FFmpegDecoder::~FFmpegDecoder()
{
	//av_lockmgr_register(NULL);
}


int FFmpegDecoder::initFFMPEG()
{
	//m_state = RC_STATE_INIT;
	avcodec_register_all();
	av_register_all();
	//avformat_network_init();


	//if (av_lockmgr_register(lockmgr))
	{
		// m_state = RC_STATE_INIT_ERROR;
		//   return -1;
	}
	return 0;
}

int FFmpegDecoder::openDecoder(int width, int height, CDecodeCB* pCB)
{
	m_nWidth = width;
	m_nHeight = height;
	m_pCB = pCB;
	if (m_bInit)
		return -1;

	// FORMAT CONTEXT SETUP
//	format_context = avformat_alloc_context();
//	format_context->flags = AVFMT_NOFILE;

	// DECODER SETUP
	decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!decoder)
	{
		//log_error("codec not found");
		fEnviron << "codec not found";
		return -2;
	}


	decoder_context = avcodec_alloc_context3(decoder);
	if (!decoder_context)
	{
		//log_error("codec context not found");
		fEnviron << "codec context not found";
		return -3;
	}
//	decoder_context->codec_id = AV_CODEC_ID_H264;
//	decoder_context->codec_type = AVMEDIA_TYPE_VIDEO;
//	decoder_context->pix_fmt = AV_PIX_FMT_YUV420P;
//	decoder_context->width = width;
//	decoder_context->height = height;
//	avcodec_parameters_to_context(decoder_context, st->codecpar);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
	decoder_picture = av_frame_alloc();
#else
	decode_picture = avcodec_alloc_frame();
#endif

	if (avcodec_open2(decoder_context, decoder, NULL) < 0)
	{
		//log_error("could not open codec");
		fEnviron << "could not open codec";
		return -4;
	}
	m_bInit = true;
	return 0;
}

int FFmpegDecoder::closeDecoder()
{
	if (decoder_context)
	{
		avcodec_close(decoder_context);
		//avcodec_free_context(&decoder_context);
		av_free(decoder_context);
	}
	if (decoder_picture)
		av_free(decoder_picture);

	m_bInit = false;

	return 0;
}

int FFmpegDecoder::decode_rtsp_frame(uint8_t* input, int nLen, bool bWaitIFrame /*= false*/)
{
	if (!m_bInit)
		return -1;

	if (input == NULL || nLen <= 0)
		return -2;

	try {
		int got_picture;
		int size = nLen;

		AVPacket avpkt;
		av_init_packet(&avpkt);
		avpkt.size = size;
		avpkt.data = input;

		//while (avpkt.size > 0)
		{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 72, 2)
			int len = avcodec_decode_video(decoder_context, decoder_picture, &got_picture, avpkt->data, avpkt->size);
#else
			int len = avcodec_decode_video2(decoder_context, decoder_picture, &got_picture, &avpkt); // libavcodec >= 52.72.2 (0.6)
#endif

			if (len == -1)
			{
				return -3;
			}

			if (got_picture)
			{
				++frame_count;
//				int w = decode_context->width;
//				int h = decode_context->height;
				int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, decoder_context->width, decoder_context->height);
				uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

				// YUV to RGB Color
				// reference
				// https://gist.github.com/lkraider/832062
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
				AVFrame *pFrameRGB = av_frame_alloc();
#else
				AVFrame *pFrameRGB = avcodec_alloc_frame();
#endif
				avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, decoder_context->width, decoder_context->height);

				img_convert_ctx = sws_getCachedContext(img_convert_ctx,
					decoder_context->width, decoder_context->height, decoder_context->pix_fmt, decoder_context->width, decoder_context->height, AV_PIX_FMT_RGB24, sws_flags, NULL, NULL, NULL);
				if (img_convert_ctx == NULL)
				{
					//log_error("Cannot initialize the conversion context");
					fEnviron << "Cannot initialize the conversion context";
					//exit(1);
					return -4;
				}
				sws_scale(img_convert_ctx, decoder_picture->data, decoder_picture->linesize,
					0, decoder_context->height, pFrameRGB->data, pFrameRGB->linesize);

#if defined(SAVE_AVFRAME_TO_PPM)
				save_frame_as_ppm(pFrameRGB);
#elif defined(SAVE_AVFRAME_TO_JPEG)
				save_frame_as_jpeg(decoder_picture);
#endif
				if (m_pCB)
				{
					int pitch = pFrameRGB->linesize[0];
					m_pCB->videoCB(decoder_context->width, decoder_context->height, pFrameRGB->data[0], numBytes * sizeof(uint8_t), pitch, pClient);
				}

				av_free(buffer);
				av_free(pFrameRGB);
				return 0;

				if (avpkt.data)
				{
					avpkt.size -= len;
					avpkt.data += len;
				}
			}
			else
			{
				return -5;
			}
			//return 0;
		}

		//return 0;
	}
	catch (...)
	{
	}

	return -6;
}

int FFmpegDecoder::save_frame_as_ppm(AVFrame *pframe)
{
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "frame_%06ld.ppm", frame_count);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return -7;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", decoder_context->width, decoder_context->height);

	// Write pixel data
	// Write pixel data 
	for (int y = 0; y<decoder_context->height; y++)
		fwrite(pframe->data[0] + y * pframe->linesize[0], 1, decoder_context->width * 3, pFile);

	// Close file
	fclose(pFile);
}

int FFmpegDecoder::save_frame_as_jpeg(AVFrame *pframe)
{
	char szFilename[32];
	//sprintf_s(szFilename, sizeof(szFilename), "frame_%06ld.jpg", frame_count);
	sprintf(szFilename, "frame_%06ld.jpg", frame_count);

	AVFormatContext* pFormatCtx = avformat_alloc_context();

	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	if (avio_open(&pFormatCtx->pb, szFilename, AVIO_FLAG_READ_WRITE) < 0) {
		//printf("Couldn't open output file.");
		fEnviron << "Couldn't open output file.";
		return -1;
	}

	// Begin Output some information
	av_dump_format(pFormatCtx, 0, szFilename, 1);
	// End Output some information

	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	AVCodecContext* pCodecCtx = pAVStream->codec;

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = decoder_context->width;
	pCodecCtx->height = decoder_context->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		//printf("Codec not found.");
		fEnviron << "Codec not found.";
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		//printf("Could not open codec.");
		fEnviron << "Could not open codec.";
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// 
	int got_picture = 0;
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pframe, &got_picture);
	if (ret < 0) {
		//printf("Encode Error.\n");
		fEnviron << "Encode Error.";
		return -1;
	}
	if (got_picture == 1) {
		//pkt.stream_index = pAVStream->index;
		ret = av_write_frame(pFormatCtx, &pkt);
	}

	av_free_packet(&pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);

	//printf("Encode Successful.\n");
	fEnviron << "Image Frame Encode Successful.";

	if (pAVStream) {
		avcodec_close(pAVStream->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}

void FFmpegDecoder::setClient(RTSPClient* client)
{
	this->pClient = client;
}