#include "FFMpeg.h"

FFMpeg::FFMpeg()
	: frame_count(0)
	, m_bInit(false)
	, img_convert_ctx(NULL)
	, codec_id(AV_CODEC_ID_NONE)
	, sws_flags(SWS_BICUBIC) // https://blog.csdn.net/leixiaohua1020/article/details/42134965
	, pFormatCtx(NULL)
	, pFrame(NULL)
{
	//intialize();
}

FFMpeg::~FFMpeg()
{
	//finalize();
}

int FFMpeg::intialize()
{
	// Intialize FFmpeg enviroment
	av_register_all();
	avdevice_register_all();
	avcodec_register_all();
	avformat_network_init();

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
	pFrame = av_frame_alloc();
#else
	pFrame = avcodec_alloc_frame();
#endif

	m_bInit = true;

	return 0;
}

int FFMpeg::finalize()
{
	if(img_convert_ctx)
		sws_freeContext(img_convert_ctx);

	if (pFrame) {
		av_freep(&(pFrame->data[0]));
		av_frame_unref(pFrame);
		av_free(pFrame);
	}

	if (pCodecCtx)
	{
		avcodec_close(pCodecCtx);
		av_free(pCodecCtx);
	}

	if(pFormatCtx)
		avformat_close_input(&pFormatCtx);

	m_bInit = false;

	return 0;
}

void FFMpeg::setOnframeCallbackFunction(std::function<void(uint8_t *, int, int, int, int)> func)
{
	onFrame = func;
}

int FFMpeg::save_frame_as_ppm(AVFrame *pframe)
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
	fprintf(pFile, "P6\n%d %d\n255\n", srcWidth, srcHeight);

	// Write pixel data
	// Write pixel data 
	for (int y = 0; y< pCodecCtx->height; y++)
		fwrite(pframe->data[0] + y * pframe->linesize[0], 1, pCodecCtx->width * 3, pFile);

	// Close file
	fclose(pFile);
}

int FFMpeg::save_frame_as_jpeg(AVFrame *pframe)
{
	char szFilename[32];
	//sprintf_s(szFilename, sizeof(szFilename), "frame_%06ld.jpg", frame_count);
	sprintf(szFilename, "frame_%06ld.jpg", frame_count);

	AVFormatContext* pFormatCtx = avformat_alloc_context();

	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	if (avio_open(&pFormatCtx->pb, szFilename, AVIO_FLAG_READ_WRITE) < 0) {
		//printf("Couldn't open output file.");
		//fEnviron << "Couldn't open output file.";
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
	pCodecCtx->width = pCodecCtx->width;
	pCodecCtx->height = pCodecCtx->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		//printf("Codec not found.");
		//fEnviron << "Codec not found.";
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		//printf("Could not open codec.");
		//fEnviron << "Could not open codec.";
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
		//fEnviron << "Encode Error.";
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
	//fEnviron << "Image Frame Encode Successful.";

	if (pAVStream) {
		avcodec_close(pAVStream->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}