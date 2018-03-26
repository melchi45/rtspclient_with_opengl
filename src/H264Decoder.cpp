#include "H264Decoder.h"

H264Decoder::H264Decoder()
	: FFMpeg()
{
	codec_id = AV_CODEC_ID_H264;
	dstWidth = 640;
	dstHeight = 320;

	intialize();
}

H264Decoder::~H264Decoder()
{
	finalize();
}

int H264Decoder::intialize()
{	
	FFMpeg::intialize();

	// DECODER SETUP
	pAVCodec = avcodec_find_decoder(codec_id);
	if (!pAVCodec)
	{
		//log_error("codec not found");
		//fEnviron << "codec not found";
		return -2;
	}

	pCodecCtx = avcodec_alloc_context3(pAVCodec);
	if (!pCodecCtx)
	{
		//log_error("codec context not found");
		//fEnviron << "codec context not found";
		return -3;
	}

	if (avcodec_open2(pCodecCtx, pAVCodec, NULL) < 0)
	{
		//log_error("could not open codec");
		//fEnviron << "could not open codec";
		return -4;
	}
	m_bInit = true;

	return 0;
}

int H264Decoder::finalize()
{

	return FFMpeg::finalize();
}

int H264Decoder::decode(uint8_t* input, int nLen, bool bWaitIFrame /*= false*/)
{
	if (!m_bInit)
		return -1;

	if (input == NULL || nLen <= 0)
		return -2;

	try {
		int got_picture;
		int size = nLen;

		// set packet 
		AVPacket avpkt;
		av_init_packet(&avpkt);
		avpkt.size = size;
		avpkt.data = input;

		//while (avpkt.size > 0)
		{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 72, 2)
			int len = avcodec_decode_video(decoder_context, decoder_picture, &got_picture, avpkt->data, avpkt->size);
#else
			int len = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &avpkt); // libavcodec >= 52.72.2 (0.6)
#endif

			if (len == -1)
			{
				return -3;
			}

			if (got_picture)
			{
				// increase frame count
				increase_frame_count();
				// set image size
				srcWidth = pCodecCtx->width;
				srcHeight = pCodecCtx->height;
				// calculate byte size from rgb image by width and height
				int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, dstWidth, dstHeight);
				uint8_t * buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

				// YUV to RGB Color
				// reference
				// https://gist.github.com/lkraider/832062
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
				AVFrame *pFrameRGB = av_frame_alloc();
#else
				AVFrame *pFrameRGB = avcodec_alloc_frame();
#endif
				// initialize RGB frame
				avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, dstWidth, dstHeight);

				img_convert_ctx = sws_getCachedContext(img_convert_ctx,
					pCodecCtx->width, // width of source image
					pCodecCtx->height, // height of source image 
					pCodecCtx->pix_fmt,
					dstWidth, // width of destination image
					dstHeight, // height of destination image
					AV_PIX_FMT_RGB24, getSWSType(), NULL, NULL, NULL);
				if (img_convert_ctx == NULL)
				{
					//log_error("Cannot initialize the conversion context");
					//fEnviron << "Cannot initialize the conversion context";
					return -4;
				}
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
					0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

#if defined(SAVE_AVFRAME_TO_PPM)
				save_frame_as_ppm(pFrameRGB);
#elif defined(SAVE_AVFRAME_TO_JPEG)
				save_frame_as_jpeg(decoder_picture);
#endif
				int pitch = pFrameRGB->linesize[0];
				if (m_plistener != NULL) {
					m_plistener->onFrame(pFrameRGB->data[0], numBytes * sizeof(uint8_t), dstWidth, dstHeight, pitch);
				} else if (onFrame != NULL) {
					onFrame(pFrameRGB->data[0], numBytes * sizeof(uint8_t), dstWidth, dstHeight, pitch);
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
		}
	}
	catch (...)
	{
	}

	return -6;
}