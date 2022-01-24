/*******************************************************************************
*  Copyright (c) 1998 MFC Forum
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Module Name:
*
* Revision History:
*
* Date        Ver Name                    Description
* ----------  --- --------------------- -----------------------------------------
* 07-Jun-2016 0.1 Youngho Kim             Created
* ----------  --- --------------------- -----------------------------------------
*
* DESCRIPTION:
*
*  $Author:
*  $LastChangedBy:
*  $Date:
*  $Revision: 2949 $
*  $Id:
*  $HeadURL:
*******************************************************************************/
#ifndef _MEDIA_RTSP_SESSION_H_
#define _MEDIA_RTSP_SESSION_H_

#include <string>
#include <pthread.h>
#include <vector>		// for std::vector

#if 0
#include "FFMpegDecoder.h"
#else
#include "H264Decoder.h"
#endif

// reference
// https://en.wikipedia.org/wiki/Graphics_display_resolution
typedef enum screen_ratio {
	nHD,
	qHD,
	HD,
	HD_PLUS,
	FHD,
	WQHD,
	QHD_PLUS,
	UHD_4K,
	UHD_5K_PLUS,
	UHD_8K
} SCREEN_RATIO;

#define WINDOWS_WIDTH	320
#define WINDOWS_HEIGHT	160

#if defined(USE_GLFW_LIB)
// reference
// https://medium.com/@Plimsky/how-to-install-a-opengl-environment-on-ubuntu-e3918cf5ab6c
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#elif defined(USE_SDL2_LIB)
#include <SDL2/SDL.h>
#undef main
#endif

class TaskScheduler;
class UsageEnvironment;
class UserAuthenticationDatabase;
class RTSPClient;
class RTSPServer;
#if 0
class MediaRTSPSession : public CDecodeCB
#else
class MediaRTSPSession : public DecodeListener
#endif
{
public:
	MediaRTSPSession();
	virtual ~MediaRTSPSession();
	int startRTSPClient(const char* progName, const char* rtspURL, 
		const char* username = NULL, const char* password = NULL, 
		bool bInterleaved = false, bool bTransportStream = false);

	int startRTSPServer(const int portnum = 554, 
		const char* username = NULL, const char* password = NULL);

	int stopRTSPClient();
	int stopRTSPServer();
	int openURL(UsageEnvironment& env);
	bool isTransportStream() { return bTransportStream; }
	void setTransportStream(bool flag) { bTransportStream = flag; }
	bool isInterleaved() { return bInterleaved; }
	void setInterleaved(bool flag) { bInterleaved = flag; }
	void setUsername(const char* username) { m_username = username; }
	void setPassword(const char* password) { m_password = password; }
	void setPort(const int port) { m_port = port; }
	void setDebugLevel(int level) { m_debugLevel = level; }

private:
	RTSPClient* m_rtspClient;
	RTSPServer* m_rtspServer;
	TaskScheduler* m_taskScheduler;
	UsageEnvironment* m_usageEnvironment;
	UserAuthenticationDatabase* m_authDB;

	char eventLoopWatchVariable;
	bool bTransportStream;
	bool bInterleaved;

	pthread_t tid;
	bool m_running;
	std::string m_rtspUrl;
	std::string m_progName;
	std::string m_username;
	std::string m_password;
	int m_debugLevel;
	int m_port;

	SCREEN_RATIO screen_radio;
	
	typedef struct buffer {
		int width;
		int height;
		int length;
		uint8_t*	data;
		int pitch;
	} rgb_buffer;

	std::vector<rgb_buffer> myvector;

#if defined(USE_GLFW_LIB)
	GLuint camera_texture;
	// variable declarations for glfw
	GLFWwindow* window;

#elif defined(USE_SDL2_LIB)
	// variable declarations for sdl2
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
#endif

	static void *rtsp_thread_fun(void *param);
	static void *rtsp_server_thread_fun(void *param);
	
	void rtsp_fun();
	void rtspserver_fun();
#if defined(USE_GLFW_LIB)
	static void *glfw3_thread_fun(void *param);
	void glfw3_fun();
#elif defined(USE_SDL2_LIB)
	static void *sdl2_thread_fun(void *param);
	void sdl2_fun();
#endif
protected:
#if 0
	virtual void videoCB(int width, int height, uint8_t* buff, int len, int pitch, RTSPClient* client);
#else
	virtual void onDecoded(void* frame);
#endif
};

#endif // _MEDIA_RTSP_SESSION_H_