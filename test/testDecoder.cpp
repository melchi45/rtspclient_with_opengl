/*

BASIC GLFW + GLXW WINDOW AND OPENGL SETUP
------------------------------------------
See https://gist.github.com/roxlu/6698180 for the latest version of the example.

*/
#include <stdlib.h>
#include <stdio.h>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define ROXLU_USE_MATH
#define ROXLU_USE_PNG
#define ROXLU_USE_OPENGL
#define ROXLU_IMPLEMENTATION
#include <tinylib.h>

#include "H264Decoder.h"
H264_Decoder* decoder_ptr = NULL;
bool playback_initialized = false;

void frame_callback(AVFrame* frame, AVPacket* pkt, void* user);

int main() {
	// ----------------------------------------------------------------
	// THIS IS WHERE YOU START CALLING OPENGL FUNCTIONS, NOT EARLIER!!
	// ----------------------------------------------------------------
	H264_Decoder decoder(frame_callback, NULL);
	decoder_ptr = &decoder;

	if (!decoder.load("bunny.h264", 30.0f)) {
		::exit(EXIT_FAILURE);
	}

	while (!decoder.readFrame()) {

	}

	return EXIT_SUCCESS;
}

void frame_callback(AVFrame* frame, AVPacket* pkt, void* user) {
}