#ifndef _MEDIA_RTSP_SESSION_H_
#define _MEDIA_RTSP_SESSION_H_

#include <string>
#include <pthread.h>
#include "FFMpegDecoder.h"

#define USE_SDL2_LIB	1
//#define USE_GLFW_LIB	0

#define WINDOWS_WIDTH	640
#define WINDOWS_HEIGHT	360

#if defined(USE_GLFW_LIB)
#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#elif defined(USE_SDL2_LIB)
#include <SDL2/SDL.h>
//#include <SDL2/SDL_image.h>
#endif

class TaskScheduler;
class UsageEnvironment;
class RTSPClient;
class MediaRTSPSession : public CDecodeCB
{
public:
	MediaRTSPSession();
	virtual ~MediaRTSPSession();
	int startRTSPClient(const char* progName, const char* rtspURL, 
		const char* username = NULL, const char* password = NULL, 
		bool bInterleaved = false, bool bTransportStream = false);
	int stopRTSPClient();
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
	TaskScheduler* taskScheduler;
	UsageEnvironment* usageEnvironment;

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
#if defined(USE_GLFW_LIB)
	// variable declarations for glfw
	GLFWwindow* window;
#elif defined(USE_SDL2_LIB)
	// variable declarations for sdl2
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
#endif

	static void *rtsp_thread_fun(void *param);
	void rtsp_fun();
#if defined(USE_GLFW_LIB)
	static void *glfw3_thread_fun(void *param);
	void glfw3_fun();
#elif defined(USE_SDL2_LIB)
	static void *sdl2_thread_fun(void *param);
	void sdl2_fun();
#endif
protected:
	virtual void videoCB(int width, int height, uint8_t* buff, int len, int pitch, RTSPClient* client);
};

#endif // _MEDIA_RTSP_SESSION_H_