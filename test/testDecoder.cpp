/*

BASIC GLFW + GLXW WINDOW AND OPENGL SETUP
------------------------------------------
See https://gist.github.com/roxlu/6698180 for the latest version of the example.

*/
#include <stdlib.h>
#include <stdio.h>

#include "H264Decoder.h"
H264_Decoder* decoder_ptr = NULL;
bool playback_initialized = false;

void frame_callback(AVCodecContext *pCodecCtx, AVFrame* frame, AVPacket* pkt, int framecount, void* user);
void yuv420p_save(AVFrame *pFrame, int width, int height, int iFrame);

int main() {
	// ----------------------------------------------------------------
	// THIS IS WHERE YOU START CALLING OPENGL FUNCTIONS, NOT EARLIER!!
	// ----------------------------------------------------------------
	H264_Decoder decoder(frame_callback, NULL);
	decoder_ptr = &decoder;

	if (!decoder.load("test.264", 30.0f)) {
		::exit(EXIT_FAILURE);
	}

	while (!decoder.readFrame()) {

	}

	return EXIT_SUCCESS;
}

void frame_callback(AVCodecContext *pCodecCtx, AVFrame* frame, AVPacket* pkt, int framecount, void* user) {
	yuv420p_save(frame, frame->width, frame->height, framecount);
}

void yuv420p_save(AVFrame *pFrame, int width, int height, int iFrame)
{
	int i = 0;
	FILE *pFile;
	char szFilename[32];

	int height_half = height / 2, width_half = width / 2;
	int y_wrap = pFrame->linesize[0];
	int u_wrap = pFrame->linesize[1];
	int v_wrap = pFrame->linesize[2];

	unsigned char *y_buf = pFrame->data[0];
	unsigned char *u_buf = pFrame->data[1];
	unsigned char *v_buf = pFrame->data[2];
	sprintf(szFilename, "frame%d.jpg", iFrame);
	pFile = fopen(szFilename, "wb");

	//save y
	for (i = 0; i <height; i++)
		fwrite(y_buf + i * y_wrap, 1, width, pFile);
	fprintf(stderr, "===>save Y success\n");

	//save u
	for (i = 0; i <height_half; i++)
		fwrite(u_buf + i * u_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save U success\n");

	//save v
	for (i = 0; i <height_half; i++)
		fwrite(v_buf + i * v_wrap, 1, width_half, pFile);
	fprintf(stderr, "===>save V success\n");

	fflush(pFile);
	fclose(pFile);
}