#include "FFMpegDecoder.h"
#include "log_utils.h"

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

FFmpegDecoder::FFmpegDecoder()
{
	m_bInit = false;
	img_convert_ctx = NULL;
}


FFmpegDecoder::~FFmpegDecoder()
{
	av_lockmgr_register(NULL);
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
		log_error("codec not found");
		return -2;
	}


	decoder_context = avcodec_alloc_context3(decoder);
//	decoder_context->codec_id = AV_CODEC_ID_H264;
//	decoder_context->codec_type = AVMEDIA_TYPE_VIDEO;
//	decoder_context->pix_fmt = AV_PIX_FMT_YUV420P;
//	decoder_context->width = width;
//	decoder_context->height = height;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
	decoder_picture = av_frame_alloc();
#else
	decode_picture = avcodec_alloc_frame();
#endif

	if (avcodec_open2(decoder_context, decoder, NULL) < 0)
	{
		log_error("could not open codec");
		return -3;
	}
	m_bInit = true;
	return 0;
}

int FFmpegDecoder::closeDecoder()
{
	if (decoder_context)
	{
		avcodec_close(decoder_context);
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
//				int w = decode_context->width;
//				int h = decode_context->height;
				int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, decoder_context->width, decoder_context->height);
				uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

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
					log_error("Cannot initialize the conversion context");
					//exit(1);
					return -4;
				}
				sws_scale(img_convert_ctx, decoder_picture->data, decoder_picture->linesize,
					0, decoder_context->height, pFrameRGB->data, pFrameRGB->linesize);

				if (m_pCB)
				{
					m_pCB->videoCB(decoder_context->width, decoder_context->height, pFrameRGB->data[0], numBytes * sizeof(uint8_t));
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
