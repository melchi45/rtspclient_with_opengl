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
#include "MediaRTSPSession.h"

#include "MediaBasicUsageEnvironment.h"
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"
#include "MediaRTSPClient.h"
#include "MediaRTSPServer.h"
#include "DummySink.h"
#include "MediaH264MediaSink.h"
#include "MediaH265MediaSink.h"
#include "liveMedia.hh"
#include "log_utils.h"
#include "Frame.h"

#include <iostream>		// for std::cout, std::endl
#include <map>

//#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
#define RCVBUF_SIZE		2097152
#define TEX_ID			0

char const* clientProtocolName = "RTSP";
int windows_width;
int windows_height;

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterANNOUNCE(RTSPClient* client, int resultCode, char* resultString);
void continueAfterRECORD(RTSPClient* client, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

void subsessionAfterRecording(void* /*clientData*/);

// A function that outputs a string that identifies each stream (for debugging output). Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}
// A function that outputs a string that identifies each subsession (for debugging output). Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}
void usage(UsageEnvironment& env, char const* progName) {
	env << "Usage: " << progName << "  ... \n";
	env << "\t(where each  is a \"rtsp://\" URL)\n";
}

MediaRTSPSession::MediaRTSPSession()
	: m_rtspClient(NULL)
	, m_rtspServer(NULL)
	, bTransportStream(false)
	, m_port(0)
	, m_username("")
	, m_password("")
	, m_progName("")
	, m_rtspUrl("")
	, screen_radio(HD)
	, m_debugLevel(0)
	, m_authDB(NULL)
#if defined(USE_GLFW_LIB)
	, window(NULL)
#elif defined(USE_SDL2_LIB)
	, window(NULL)
	, renderer(NULL)
	, texture(NULL)
#endif
{
	m_running = false;
	eventLoopWatchVariable = 0;

	switch (screen_radio)
	{
	case nHD:
		windows_width = 640;
		windows_height = 320;
		break;
	case qHD:
		windows_width = 960;
		windows_height = 540;
		break;
	case HD:
		windows_width = 1280;
		windows_height = 720;
		break;
	case HD_PLUS:
		windows_width = 1600;
		windows_height = 900;
		break;
	case FHD:
		windows_width = 1920;
		windows_height = 1080;
		break;
	case WQHD:
		windows_width = 2560;
		windows_height = 1440;
		break;
	case QHD_PLUS:
		windows_width = 3200;
		windows_height = 1800;
		break;
	case UHD_4K:
		windows_width = 3840;
		windows_height = 2160;
		break;
	case UHD_5K_PLUS:
		windows_width = 5120;
		windows_height = 2880;
		break;
	case UHD_8K:
		windows_width = 7680;
		windows_height = 4320;
		break;
	default:
		windows_width = 640;
		windows_height = 320;
		break;
	}
}

MediaRTSPSession::~MediaRTSPSession()
{
#ifdef USE_GLFW_LIB
	glfwDestroyWindow(window);
	glfwTerminate();
#endif

#ifdef USE_SDL2_LIB
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
#endif
}

int MediaRTSPSession::startRTSPClient(const char* progName, const char* rtspURL, 
	const char* username, const char* password,
	bool bInterleaved, bool bTransportStream)
{
	if (username != NULL) this->m_username = std::string(username);
	if (password != NULL) this->m_password = std::string(password);
	this->m_progName = std::string(progName);
	this->m_rtspUrl = std::string(rtspURL);
	this->bTransportStream = bTransportStream;
	this->bInterleaved = bInterleaved;

	eventLoopWatchVariable = 0;

	int r = pthread_create(&tid, NULL, rtsp_thread_fun, this);
	if (r)
	{
		perror("pthread_create()");
		return -1;
	}
#ifdef USE_GLFW_LIB	
	int p = pthread_create(&tid, NULL, glfw3_thread_fun, this);
	if (r)
	{
		perror("pthread_create()");
		return -1;
	}
#endif
#ifdef USE_SDL2_LIB
	int p = pthread_create(&tid, NULL, sdl2_thread_fun, this);
	if (r)
	{
		perror("pthread_create()");
		return -1;
	}

#endif
	return 0;
}

int MediaRTSPSession::startRTSPServer(const int portnum /*= 554*/, 
	const char* username/* = NULL*/, const char* password/* = NULL*/)
{
	if (username != NULL) this->m_username = std::string(username);
	if (password != NULL) this->m_password = std::string(password);

	m_port = portnum;

	if (username != NULL && password != NULL) {
		// To implement client access control to the RTSP server, do the following:
		m_authDB = new UserAuthenticationDatabase;
		m_authDB->addUserRecord(username, password); // replace these with real strings
												   // Repeat the above with each <username>, <password> that you wish to allow
												   // access to the server.
	}

	eventLoopWatchVariable = 0;

	int r = pthread_create(&tid, NULL, rtsp_server_thread_fun, this);
	if (r)
	{
		perror("pthread_create()");
		return -1;
	}
}

int MediaRTSPSession::stopRTSPClient()
{
	eventLoopWatchVariable = 1;
	return 0;
}

int MediaRTSPSession::stopRTSPServer()
{
	eventLoopWatchVariable = 1;
	return 0;
}

void *MediaRTSPSession::rtsp_thread_fun(void *param)
{
	MediaRTSPSession *pThis = (MediaRTSPSession*)param;
	pThis->rtsp_fun();
	return NULL;
}

void *MediaRTSPSession::rtsp_server_thread_fun(void *param)
{
	MediaRTSPSession *pThis = (MediaRTSPSession*)param;
	pThis->rtspserver_fun();
	return NULL;
}

#ifdef USE_GLFW_LIB
void *MediaRTSPSession::glfw3_thread_fun(void *param)
{
	MediaRTSPSession *pThis = (MediaRTSPSession*)param;
	pThis->glfw3_fun();
	return NULL;
}
#endif

#ifdef USE_SDL2_LIB
void *MediaRTSPSession::sdl2_thread_fun(void *param)
{
	MediaRTSPSession *pThis = (MediaRTSPSession*)param;
	pThis->sdl2_fun();
	return NULL;
}
#endif

void MediaRTSPSession::rtsp_fun()
{
	//::startRTSP(m_progName.c_str(), m_rtspUrl.c_str(), m_ndebugLever);
	m_taskScheduler = BasicTaskScheduler::createNew();
	m_usageEnvironment = MediaBasicUsageEnvironment::createNew(*m_taskScheduler);
	if (openURL(*m_usageEnvironment) == 0)
	{
//		m_nStatus = 1;
		m_usageEnvironment->taskScheduler().doEventLoop(&eventLoopWatchVariable);

		m_running = false;
		eventLoopWatchVariable = 0;

		if (m_rtspClient)
		{
			shutdownStream(m_rtspClient, 0);
		}
		m_rtspClient = NULL;
	}

	m_usageEnvironment->reclaim();
	m_usageEnvironment = NULL;

	delete m_taskScheduler;
	m_taskScheduler = NULL;
//	m_nStatus = 2;
}

void MediaRTSPSession::rtspserver_fun()
{
	m_taskScheduler = BasicTaskScheduler::createNew();
	m_usageEnvironment = MediaBasicUsageEnvironment::createNew(*m_taskScheduler);

	m_rtspServer = MediaRTSPServer::createNew(*m_usageEnvironment, m_port, m_authDB);
	if (m_rtspServer == NULL) {
		*m_usageEnvironment << "LIVE555: Failed to create RTSP server: %s\n", m_usageEnvironment->getResultMsg();
	}

	char* urlPrefix = m_rtspServer->rtspURLPrefix();
	*m_usageEnvironment << "Play streams from this server using the URL:" << urlPrefix << ", type: " << LIVE_VEIW_NAME << "\n";
//	*m_usageEnvironment << "where <file path> is a file present in the current directory: " << urlPrefix, "/{file path}/{file name}");

	if (m_rtspServer->setUpTunnelingOverHTTP(8000) || m_rtspServer->setUpTunnelingOverHTTP(8080)) {
		*m_usageEnvironment << "We use port " << m_rtspServer->httpServerPortNum()
			<< " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n\n\n";
	}
	else {
		*m_usageEnvironment << "(RTSP-over-HTTP tunneling is not available.)\n\n\n";
	}

	m_usageEnvironment->taskScheduler().doEventLoop(&eventLoopWatchVariable);

	m_running = false;
	eventLoopWatchVariable = 0;

	if (m_rtspServer)
	{
		// TODO: release rtsp server
		//shutdownStream(m_rtspServer, 0);
	}
	m_rtspServer = NULL;

	m_usageEnvironment->reclaim();
	m_usageEnvironment = NULL;

	delete m_taskScheduler;
	m_taskScheduler = NULL;
}

#ifdef USE_GLFW_LIB
static void resize_callback(GLFWwindow* window, int width, int height) {
//	if (player_ptr) {
//		player_ptr->resize(width, height);
//	}
}
static void button_callback(GLFWwindow* win, int bt, int action, int mods) {
	double x, y;
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		glfwGetCursorPos(win, &x, &y);
	}
}
static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static void cursor_callback(GLFWwindow* win, double x, double y) { }

static void char_callback(GLFWwindow* win, unsigned int key) { }


#define TRIANGLE_ROTATE 0
#define USE_DUMMY_DATA	1
void MediaRTSPSession::glfw3_fun()
{
	// reference
	// https://gist.github.com/victusfate/9214902
	// https://github.com/glfw/glfw/tree/master/examples
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		return;
		//exit(EXIT_FAILURE);
	}

	// cout << "default shader lang: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// select opengl version
	int major, minor, rev;
	glfwGetVersion(&major, &minor, &rev);
	std::cout << "glfw major.minor " << major << "." << minor << "." << rev << std::endl;

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(WINDOWS_WIDTH, WINDOWS_HEIGHT, "Simple example", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		return;
		//exit(EXIT_FAILURE);
	}
	//env << "OpenGL shader language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, char_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
	glfwSetMouseButtonCallback(window, button_callback);
	glfwMakeContextCurrent(window);

	bool _firstrendering = true;

	while (!glfwWindowShouldClose(window))
	{
		float ratio;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
/*
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		*/
		/*
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
		glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		*/

#pragma message ("rgb to 2d texture")
		std::cout << "vector size: " << myvector.size() << std::endl;
		if (myvector.size() > 10) {
			std::vector<rgb_buffer>::iterator it = myvector.begin();
			rgb_buffer buffer = *it;
			width = buffer.width;
			height = buffer.height;

			if (buffer.data != NULL) {
#pragma message ("triangle rotate example")
#if TRIANGLE_ROTATE
				glRotatef((float)glfwGetTime() * 50.f, 0.f, 0.f, 1.f);
				glBegin(GL_TRIANGLES);
				glColor3f(1.f, 0.f, 0.f);
				glVertex3f(-0.6f, -0.4f, 0.f);
				glColor3f(0.f, 1.f, 0.f);
				glVertex3f(0.6f, -0.4f, 0.f);
				glColor3f(0.f, 0.f, 1.f);
				glVertex3f(0.f, 0.6f, 0.f);
				glEnd();
#else

#if USE_DUMMY_DATA
				unsigned char* data = new unsigned char[width * height * 4];
				for (int y = 0; y < height; ++y)
					for (int x = 0; x < width; ++x)
						if ((x - 100)*(x - 100) + (y - 156)*(y - 156) < 75 * 75) {
							data[(256 * y + x) * 4 + 0] = 156;
							data[(256 * y + x) * 4 + 1] = 256;
							data[(256 * y + x) * 4 + 2] = 156;
							data[(256 * y + x) * 4 + 3] = 200;
						}
						else {
							data[(256 * y + x) * 4 + 0] = 0;
							data[(256 * y + x) * 4 + 1] = 0;
							data[(256 * y + x) * 4 + 2] = 0;
							data[(256 * y + x) * 4 + 3] = 0;
						}
#endif
						if (_firstrendering) {
							glBindTexture(GL_TEXTURE_2D, camera_texture);
#if USE_DUMMY_DATA
							glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#elif USE_REAL_DATA
							glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer.width, buffer.height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer.data);
#endif

							_firstrendering = false;
						}
						else {
							glActiveTexture(camera_texture);
							glBindTexture(GL_TEXTURE_2D, camera_texture);
#if USE_DUMMY_DATA
							glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
#elif USE_REAL_DATA
							glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_BGR, GL_UNSIGNED_BYTE, (GLvoid*)buffer.data);
#endif
						}

				// Attach a name to the texture for later use
				//glBindTexture(GL_TEXTURE_2D, TEX_ID);
				//glBindTexture(GL_TEXTURE_2D, camera_texture);
				// Define how pixels are packed in arrays
				//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);  //Always set the base and max mipmap levels of a texture.
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

				//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer.width, buffer.height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer.data);
				//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data);
				//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_BGR, GL_UNSIGNED_BYTE, (GLvoid*)buffer.data);
				//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_BGR, GL_UNSIGNED_BYTE, (GLvoid*)data);
				//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data);
#if USE_DUMMY_DATA
				delete data;
#endif
#endif // TRIANGLE_ROTATE
				free(buffer.data);
			}

			myvector.erase(it);
		}
#pragma message ("texture draw example")
/*		//glColor3f(1, 1, 1);
		glBindTexture(GL_TEXTURE_2D, camera_texture);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex3f(0, 0, 0);

		glTexCoord2f(1, 1);
		glVertex3f(width, 0, 0);

		glTexCoord2f(1, 0);
		glVertex3f(width, height, 0);

		glTexCoord2f(0, 0);
		glVertex3f(0, height, 0);

		glEnd();*/

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
#endif

#ifdef USE_SDL2_LIB
void MediaRTSPSession::sdl2_fun()
{
	// reference
	// https://gist.github.com/armornick/3434362#file-openicon-c-L29

	// Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return;
		//exit(EXIT_FAILURE);
	}

	// create the window and renderer
	// note that the renderer is accelerated
	window = SDL_CreateWindow("Image Loading", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windows_width, windows_height, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	// main loop
	while (1) {
		// event handling
		SDL_Event event;
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				break;
			else if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					//resize = true;
				}
			}
			else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)
				break;
		}

		// check frame buffer size
		std::cout << "vector size: " << myvector.size() << std::endl;
		if (myvector.size() > 10) {
			std::vector<rgb_buffer>::iterator it = myvector.begin();
			rgb_buffer buffer = *it;

			if (buffer.data != NULL) {
				//SDL_Rect texr; texr.x = WINDOWS_WIDTH / 2; texr.y = WINDOWS_HEIGHT / 2; texr.w = width * 2; texr.h = height * 2;
				SDL_Rect texure_rect; texure_rect.x = 0; texure_rect.y = 0; texure_rect.w = buffer.width; texure_rect.h = buffer.height;
				SDL_Rect windows_rect; windows_rect.x = 0; windows_rect.y = 0; windows_rect.w = windows_width; windows_rect.h = windows_height;

				texture = SDL_CreateTexture(
					renderer,
					SDL_PIXELFORMAT_RGB24,
					SDL_TEXTUREACCESS_STREAMING,
					buffer.width,
					buffer.height);

				if (!texture) {

				}

				SDL_UpdateTexture(texture, NULL, buffer.data, buffer.pitch);

				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, &texure_rect, &windows_rect);
				SDL_RenderPresent(renderer);

				if (texture)
					SDL_DestroyTexture(texture);

				free(buffer.data);
			}

			myvector.erase(it);
		}
	}
}
#endif

int MediaRTSPSession::openURL(UsageEnvironment& env)
{
	// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
	// to receive (even if more than stream uses the same "rtsp://" URL).
	m_rtspClient = MediaRTSPClient::createNew(env, this, m_rtspUrl.c_str(), m_debugLevel, m_progName.c_str(), m_port, m_username.c_str(), m_password.c_str());
	if (m_rtspClient == NULL)
	{
		env << "Failed to create a RTSP client for URL \"" << m_rtspUrl.c_str() << "\": " << env.getResultMsg() << "\n";
		return -1;
	}

	if (!bTransportStream) {
		((MediaRTSPClient*)m_rtspClient)->setUpTransportStream(false);
		//	((MediaRTSPClient*)m_rtspClient)->m_nID = m_nID;
		// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
		// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
		// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
		if (((MediaRTSPClient*)m_rtspClient)->getAuth() != NULL)
			m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE, ((MediaRTSPClient*)m_rtspClient)->getAuth());
		else
			m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
	} else {
		if (((MediaRTSPClient*)m_rtspClient)->getAuth() != NULL)
			m_rtspClient->sendAnnounceCommand(((MediaRTSPClient*)m_rtspClient)->getSDPDescription(), continueAfterANNOUNCE, ((MediaRTSPClient*)m_rtspClient)->getAuth());
		else
			m_rtspClient->sendAnnounceCommand(((MediaRTSPClient*)m_rtspClient)->getSDPDescription(), continueAfterANNOUNCE);
	}
	return 0;
}
#if 0
void MediaRTSPSession::videoCB(int width, int height, uint8_t* buff, int len, int pitch, RTSPClient* client)
{
	if (client != NULL) {
		UsageEnvironment& env = client->envir(); // alias
		StreamClientState& scs = ((MediaRTSPClient*)client)->scs; // alias

		scs.subsession = scs.iter->next();
		if (scs.subsession != NULL) {
			if (scs.session == NULL) {
				env << *client << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
			}
			else if (scs.session->hasSubsessions()) {
				
			}
		}

		// copy to buffer vector
		if (buff != NULL) {
			rgb_buffer buffer;
			buffer.width = width;
			buffer.height = height;
			buffer.pitch = pitch;
			buffer.length = len;
			//buffer.data = buff;
			uint8_t* image_data = (uint8_t *)malloc(len * sizeof(uint8_t));
			memset(image_data, 0x00, len * sizeof(uint8_t));
			memcpy(image_data, buff, len * sizeof(uint8_t));
			buffer.data = image_data;
			myvector.push_back(buffer);
		}
	}
}
#else
void MediaRTSPSession::onDecoded(void* frame)
{
	// copy to buffer vector
	if (frame != NULL) {
		((Frame*)frame);

		rgb_buffer buffer;
		buffer.width = ((Frame*)frame)->width;
		buffer.height = ((Frame*)frame)->height;
		buffer.pitch = ((Frame*)frame)->pitch;
		buffer.length = ((Frame*)frame)->dataSize;
		//buffer.data = buff;
		uint8_t* image_data = (uint8_t *)malloc(((Frame*)frame)->dataSize);
		memset(image_data, 0x00, ((Frame*)frame)->dataSize);
		memcpy(image_data, ((Frame*)frame)->dataPointer, ((Frame*)frame)->dataSize);
		buffer.data = image_data;
		myvector.push_back(buffer);
	}
}
#endif

// Implementation of the RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
			break;
		}

		char* const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
			env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
			break;
		}
		else if (!scs.session->hasSubsessions()) {
			env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started playing session";
		if (scs.duration > 0) {
			env << " (for up to " << scs.duration << " seconds)";
		}
		env << "...\n";

		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

void continueAfterANNOUNCE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

	if (resultCode != 0) {
		env << clientProtocolName << " \"ANNOUNCE\" request failed: " << resultString << "\n\n\n";
	}
	else {
		env << clientProtocolName << " \"ANNOUNCE\" request returned: " << resultString << "\n\n\n";


		do {
			char* const sdpDescription = ((MediaRTSPClient*)rtspClient)->getSDPDescription();
			env << *rtspClient << "Ready a SDP description:\n" << sdpDescription << "\n";

			// Create a media session object from this SDP description:
			scs.session = MediaSession::createNew(env, sdpDescription);
			delete[] sdpDescription; // because we don't need it anymore
			if (scs.session == NULL) {
				env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
				break;
			}
			else if (!scs.session->hasSubsessions()) {
				env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
				break;
			}

			// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
			// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
			// (Each 'subsession' will have its own data source.)
			scs.iter = new MediaSubsessionIterator(*scs.session);
			setupNextSubsession(rtspClient);
			return;
		} while (0);
	}

	shutdownStream(rtspClient);
}

void continueAfterRECORD(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: "
				<< resultString << "\n";
			break;
		}

		if (scs.session->hasSubsessions()) {
			MediaSubsession* subSession;

			static MediaSubsessionIterator* setupIter = NULL;
			if (setupIter == NULL)
				setupIter = new MediaSubsessionIterator(*scs.session);
			while ((subSession = setupIter->next()) != NULL) {
				// We have another subsession left to set up:
				if (strcmp(subSession->mediumName(), "video") == 0
					&& strcmp(subSession->codecName(), "H264") == 0) {
					// Create the video source:
					FramedSource* v_source = ((MediaRTSPClient*)rtspClient)->createNewVideoStreamSource();

					//					H264VideoRTPSource* h264Source = H264VideoRTPSource::createNew()

					//H264VideoStreamFramer* videoSource = H264VideoStreamFramer::createNew(env, source);
					H264VideoStreamDiscreteFramer* videoSource = H264VideoStreamDiscreteFramer::createNew(env, v_source);

					// Create a 'H264 Video RTP' sink from the RTP 'groupsock':
					OutPacketBuffer::maxSize = 600000;
					RTPSink * videoSink =
						((MediaRTSPClient*)rtspClient)->createNewRTPSink(
							subSession->rtpSource()->RTPgs(),
							subSession->rtpPayloadFormat(), v_source);

					//dbg_msg("Can't get SPS(%p, %u)/PPS(%p, %u) !\n",
					//	((TechwinRTSPClient*)rtspClient)->getSPSPayload(),
					//	((TechwinRTSPClient*)rtspClient)->getSPSLength(),
					//	((TechwinRTSPClient*)rtspClient)->getPPSPayload(),
					//	((TechwinRTSPClient*)rtspClient)->getPPSLength());

					videoSink->startPlaying(*videoSource, subsessionAfterRecording, videoSink);

					if (!subSession->sink)
						subSession->sink = ((MediaRTSPClient*)rtspClient)->getVideoRTPSink();

					env << "Video Send Packet Start\n";
					if (subSession->rtcpInstance() != NULL) {
						subSession->rtcpInstance()->setByeHandler(
							subsessionByeHandler, subSession);
					}

				}

				if (strcmp(subSession->mediumName(), "audio") == 0
					&& strcmp(subSession->codecName(), "MPEG4-GENERIC") == 0) {
					// Create the audio source:
					FramedSource* a_source = ((MediaRTSPClient*)rtspClient)->createNewAudioStreamSource();

					OutPacketBuffer::maxSize = 600000;
					RTPSink * audioSink =
						((MediaRTSPClient*)rtspClient)->createNewRTPSink(
							subSession->rtpSource()->RTPgs(),
							subSession->rtpPayloadFormat(), a_source);

					audioSink->startPlaying(*a_source, subsessionAfterRecording, audioSink);

					if (!subSession->sink)
						subSession->sink = ((MediaRTSPClient*)rtspClient)->getAudioRTPSink();

					env << "Audio Send Packet Start\n";
					if (subSession->rtcpInstance() != NULL) {
						subSession->rtcpInstance()->setByeHandler(
							subsessionByeHandler, subSession);
					}
				}
			}
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started recording session\n";
		if (scs.duration > 0) {
			env << " (for up to " << scs.duration << " seconds)";
		}
		env << "...\n";

		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

#if defined(WIN32) && !defined(MSYS)
#pragma pack(push, 1)
#endif
struct rtp_pkt_minimum_s {
	unsigned short flags;
	unsigned short seqnum;
	unsigned int timestamp;
	unsigned int ssrc;
}
#if defined(WIN32) && !defined(MSYS)
#pragma pack(pop)
#else
__attribute__((__packed__))
#endif
;

#if defined(WIN32) && !defined(MSYS)
#pragma pack(push, 1)
#endif
struct ctrlmsg_s {
	unsigned short msgsize;		/*< size of this message, including msgsize */
	unsigned char msgtype;		/*< message type */
	unsigned char which;		/*< unused */
	unsigned char padding[124];	/*< a sufficient large buffer to fit all types of message */
}
#if defined(WIN32) && !defined(MSYS)
#pragma pack(pop)
#else
__attribute__((__packed__))
#endif
;

#if defined(WIN32) && !defined(MSYS)
#pragma pack(push, 1)
#endif

struct ctrlmsg_system_netreport_s {
	unsigned short msgsize;		/*< size of this message, including this field */
	unsigned char msgtype;		/*< must be CTRL_MSGTYPE_SYSTEM */
	unsigned char subtype;		/*< must be CTRL_MSGSYS_SUBTYPE_NETREPORT */
	unsigned int duration;		/*< sample collection duration (in microseconds) */
	unsigned int framecount;	/*< number of frames */
	unsigned int pktcount;		/*< packet count (including lost packets) */
	unsigned int pktloss;		/*< packet loss count */
	unsigned int bytecount;		/*< total received amunt of data (in bytes) */
	unsigned int capacity;		/*< measured capacity (in bits per second) */
}
#if defined(WIN32) && !defined(MSYS)
#pragma pack(pop)
#else
__attribute__((__packed__))
#endif
;

//// packet loss monitor

typedef struct pktloss_record_s {
	/* XXX: ssrc is 32-bit, and seqnum is 16-bit */
	int reset;	/* 1 - this record should be reset */
	int lost;	/* count of lost packets */
	unsigned int ssrc;	/* SSRC */
	unsigned short initseq;	/* the 1st seqnum in the observation */
	unsigned short lastseq;	/* the last seqnum in the observation */
}	pktloss_record_t;


//// bandwidth estimator

typedef struct bwe_record_s {
	struct timeval initTime;
	unsigned int framecount;
	unsigned int pktcount;
	unsigned int pktloss;
	unsigned int bytesRcvd;
	unsigned short lastPktSeq;
	struct timeval lastPktRcvdTimestamp;
	unsigned int lastPktSentTimestamp;
	unsigned int lastPktSize;
	// for estimating capacity
	unsigned int samples;
	unsigned int totalBytes;
	unsigned int totalElapsed;
}	bwe_record_t;

static std::map<unsigned int, pktloss_record_t> _pktmap;
static std::map<unsigned int, bwe_record_t> bwe_watchlist;
typedef struct rtp_pkt_minimum_s rtp_pkt_minimum_t;
typedef struct ctrlmsg_system_netreport_s ctrlmsg_system_netreport_t;

void
pktloss_monitor_update(unsigned int ssrc, unsigned short seqnum) {
	std::map<unsigned int, pktloss_record_t>::iterator mi;
	if ((mi = _pktmap.find(ssrc)) == _pktmap.end()) {
		pktloss_record_t r;
		r.reset = 0;
		r.lost = 0;
		r.ssrc = ssrc;
		r.initseq = seqnum;
		r.lastseq = seqnum;
		_pktmap[ssrc] = r;
		return;
	}
	if (mi->second.reset != 0) {
		mi->second.reset = 0;
		mi->second.lost = 0;
		mi->second.initseq = seqnum;
		mi->second.lastseq = seqnum;
		return;
	}
	if ((seqnum - 1) != mi->second.lastseq) {
		mi->second.lost += (seqnum - 1 - mi->second.lastseq);
	}
	mi->second.lastseq = seqnum;
	return;
}

long long
tvdiff_us(struct timeval *tv1, struct timeval *tv2) {
	struct timeval delta;
	delta.tv_sec = tv1->tv_sec - tv2->tv_sec;
	delta.tv_usec = tv1->tv_usec - tv2->tv_usec;
	if (delta.tv_usec < 0) {
		delta.tv_sec--;
		delta.tv_usec += 1000000;
	}
	return 1000000LL * delta.tv_sec + delta.tv_usec;
}

typedef struct ctrlmsg_s ctrlmsg_t;

#define	CTRL_MSGTYPE_SYSTEM	0xfe	/* system control message type */
#define	CTRL_MSGSYS_SUBTYPE_NETREPORT	2	/* system control message: report networking */

ctrlmsg_t *
ctrlsys_netreport(ctrlmsg_t *msg, unsigned int duration,
	unsigned int framecount, unsigned int pktcount,
	unsigned int pktloss, unsigned int bytecount,
	unsigned int capacity) {
	ctrlmsg_system_netreport_t *msgn = (ctrlmsg_system_netreport_t*)msg;
	//bzero(msg, sizeof(ctrlmsg_system_netreport_t));
	memset(msg, 0, sizeof(ctrlmsg_system_netreport_t));
	msgn->msgsize = htons(sizeof(ctrlmsg_system_netreport_t));
	msgn->msgtype = CTRL_MSGTYPE_SYSTEM;
	msgn->subtype = CTRL_MSGSYS_SUBTYPE_NETREPORT;
	msgn->duration = htonl(duration);
	msgn->framecount = htonl(framecount);
	msgn->pktcount = htonl(pktcount);
	msgn->pktloss = htonl(pktloss);
	msgn->bytecount = htonl(bytecount);
	msgn->capacity = htonl(capacity);
	return msg;
}

void
bandwidth_estimator_update(unsigned int ssrc, unsigned short seq, struct timeval rcvtv, unsigned int timestamp, unsigned int pktsize) {
	bwe_record_t r;
	std::map<unsigned int, bwe_record_t>::iterator mi;
	bool sampleframe = true;
	//
	if ((mi = bwe_watchlist.find(ssrc)) == bwe_watchlist.end()) {
		//bzero(&r, sizeof(r));
		memset(&r, 0, sizeof(r));
		r.initTime = rcvtv;
		r.framecount = 1;
		r.pktcount = 1;
		r.bytesRcvd = pktsize;
		r.lastPktSeq = seq;
		r.lastPktRcvdTimestamp = rcvtv;
		r.lastPktSentTimestamp = timestamp;
		r.lastPktSize = pktsize;
		bwe_watchlist[ssrc] = r;
		return;
	}
	// new frame?
	if (timestamp != mi->second.lastPktSentTimestamp) {
		mi->second.framecount++;
		sampleframe = false;
	}
	// no packet loss && is the same frame
	if ((seq - 1) == mi->second.lastPktSeq) {
		if (sampleframe) {
			unsigned elapsed = tvdiff_us(&rcvtv, &mi->second.lastPktRcvdTimestamp);
			unsigned cbw = 8.0 * mi->second.lastPktSize / (elapsed / 1000000.0);
#if DEBUG_PRINT_EACH_RECEIVED_FRAME
			log_info("XXX: sampled bw = %u bps\n", cbw);
#endif
			mi->second.samples++;
			mi->second.totalElapsed += elapsed;
			mi->second.totalBytes += mi->second.lastPktSize;
			if (mi->second.framecount >= 240 && mi->second.samples >= 3000) {
				ctrlmsg_t m;
				cbw = 8.0 * mi->second.totalBytes / (mi->second.totalElapsed / 1000000.0);
				log_info("XXX: received: %uKB; capacity = %.3fKbps (%d samples); loss-rate = %.2f%% (%u/%u); average per-frame overhead factor = %.2f\n",
					mi->second.bytesRcvd / 1024,
					cbw / 1024.0, mi->second.samples,
					100.0 * mi->second.pktloss / mi->second.pktcount,
					mi->second.pktloss, mi->second.pktcount,
					1.0 * mi->second.pktcount / mi->second.framecount);
				// send back to the server
				ctrlsys_netreport(&m,
					tvdiff_us(&rcvtv, &mi->second.initTime),
					mi->second.framecount,
					mi->second.pktcount,
					mi->second.pktloss,
					mi->second.bytesRcvd,
					cbw);
//				ctrl_client_sendmsg(&m, sizeof(ctrlmsg_system_netreport_t));
				//
				//bzero(&mi->second, sizeof(bwe_record_t));
				memset(&mi->second, 0, sizeof(bwe_record_t));
				mi->second.initTime = rcvtv;
				mi->second.framecount = 1;
			}
		}
		// has packet loss
	}
	else {
		unsigned short delta = (seq - mi->second.lastPktSeq - 1);
		mi->second.pktloss += delta;
		mi->second.pktcount += delta;
	}
	// update the rest
	mi->second.pktcount++;
	mi->second.bytesRcvd += pktsize;
	mi->second.lastPktSeq = seq;
	mi->second.lastPktRcvdTimestamp = rcvtv;
	mi->second.lastPktSentTimestamp = timestamp;
	mi->second.lastPktSize = pktsize;
	return;
}

void
rtp_packet_handler(void *clientData, unsigned char *packet, unsigned &packetSize) {
	rtp_pkt_minimum_t *rtp = (rtp_pkt_minimum_t*)packet;
	unsigned int ssrc;
	unsigned short seqnum;
	unsigned short flags;
	unsigned int timestamp;
	struct timeval tv;
	if (packet == NULL || packetSize < 12)
		return;
	gettimeofday(&tv, NULL);
	ssrc = ntohl(rtp->ssrc);
	seqnum = ntohs(rtp->seqnum);
	flags = ntohs(rtp->flags);
	timestamp = ntohl(rtp->timestamp);
	//
//	if (log_rtp > 0) {
//#ifdef ANDROID
//		ga_log("%10u.%06u log_rtp: flags %04x seq %u ts %u ssrc %u size %u\n",
//			tv.tv_sec, tv.tv_usec, flags, seqnum, timestamp, ssrc, packetSize);
//#else
#if DEBUG_PRINT_EACH_RECEIVED_FRAME
		log_info("log_rtp: flags %04x seq %u ts %u ssrc %u size %u\n",
			flags, seqnum, timestamp, ssrc, packetSize);
#endif
//#endif
//	}
	//
//	bandwidth_estimator_update(ssrc, seqnum, tv, timestamp, packetSize);
//	pktloss_monitor_update(ssrc, seqnum);
	//
	return;
}

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	do if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		}
		else {
			env << *rtspClient << "Initiated the \"" << *scs.subsession
				<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1 << ")\n";

			if (strcmp("video", scs.subsession->mediumName()) == 0) {
				scs.subsession->rtpSource()->setAuxilliaryReadHandler(rtp_packet_handler, NULL);
			}
			else if (strcmp("audio", scs.subsession->mediumName()) == 0) {

			}

			if (!((MediaRTSPClient*)rtspClient)->isUpTransportStream()) {
				// Continue setting up this subsession, by sending a RTSP "SETUP" command:
				rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, ((MediaRTSPClient*)rtspClient)->isInterleavedMode());
			}
			else {
				// Continue setting up this subsession, by sending a RTSP "SETUP" command:
				rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, ((MediaRTSPClient*)rtspClient)->isInterleavedMode(), false, false, ((MediaRTSPClient*)rtspClient)->getAuth());
			}
		}
		return;
	} while (0);

	if (!((MediaRTSPClient*)rtspClient)->isUpTransportStream()) {
		// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
		scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
	else
	{
		rtspClient->sendRecordCommand(*scs.session, continueAfterRECORD, ((MediaRTSPClient*)rtspClient)->getAuth());
	}
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		env << *rtspClient << "Set up the \"" << *scs.subsession 
			<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1 << ")\n";

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)
		if (!((MediaRTSPClient*)rtspClient)->isUpTransportStream()) {
			if (strcmp(scs.subsession->mediumName(), "video") == 0) {
				env << *rtspClient << "Set up the video session: \"" << *scs.subsession
					<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1 << ")\n";

				if ((strcmp(scs.subsession->codecName(), "H264") == 0)) {
					const char *sprop = scs.subsession->fmtp_spropparametersets();
					u_int8_t const* sps = NULL;
					unsigned spsSize = 0;
					u_int8_t const* pps = NULL;
					unsigned ppsSize = 0;

					if (sprop != NULL) {
						unsigned int numSPropRecords;
						SPropRecord* sPropRecords = parseSPropParameterSets(sprop, numSPropRecords);
						for (unsigned i = 0; i < numSPropRecords; ++i) {
							if (sPropRecords[i].sPropLength == 0) continue; // bad data
							u_int8_t nal_unit_type = (sPropRecords[i].sPropBytes[0]) & 0x1F;
							if (nal_unit_type == 7/*SPS*/) {
								sps = sPropRecords[i].sPropBytes;
								spsSize = sPropRecords[i].sPropLength;
							}
							else if (nal_unit_type == 8/*PPS*/) {
								pps = sPropRecords[i].sPropBytes;
								ppsSize = sPropRecords[i].sPropLength;
							}
						}
					}

					scs.subsession->sink = MediaH264MediaSink::createNew(env, rtspClient, *scs.subsession, rtspClient->url());
					/*scs.subsession->sink = MediaH264VideoRTPSink::createNew(env,
						scs.subsession,
						sps, spsSize, pps, ppsSize,
						rtspClient->url());*/
					/*scs.subsession->sink = H264VideoRTPSink::createNew(env,
						scs.subsession->rtpSource()->RTPgs(), scs.subsession->rtpPayloadFormat(),
						scs.subsession->fmtp_spropparametersets());*/
					// perhaps use your own custom "MediaSink" subclass instead
					if (scs.subsession->sink == NULL) {
						env << *rtspClient << "Failed to create a video data sink for the \"" << *scs.subsession
							<< "\" subsession: " << env.getResultMsg() << "\n";
						break;
					}
#if 0
					FFmpegDecoder* decoder = ((MediaH264MediaSink*)scs.subsession->sink)->getDecoder(); // alias
					if (decoder == NULL) {
						env << "Failed to get a video decoder\n";
						break;
					}
					decoder->openDecoder(1920, 1080, ((MediaRTSPClient*)rtspClient)->getRTSPSession());
#else
					H264Decoder* decoder = ((MediaH264MediaSink*)scs.subsession->sink)->getDecoder(); // alias
					if (decoder == NULL) {
						env << "Failed to get a video decoder\n";
						break;
					}
					decoder->setListener(((MediaRTSPClient*)rtspClient)->getRTSPSession());
					decoder->setRescaleSize(windows_width, windows_height); //set decode size
#endif

//					scs.subsession->videoWidth();
//					scs.subsession->videoHeight();
					
					env << *rtspClient << "Created a video data sink for the \"" << *scs.subsession << "\" subsession\n";
					scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 

					// Create the data source: an "H264 Video RTP source"
					//RTPSource* mVideoReceiverSource = H264VideoRTPSource::createNew(env, scs.subsession->rtpSource()->RTPgs(), scs.subsession->rtpPayloadFormat(), 90000);

					if (!scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
						subsessionAfterPlaying, scs.subsession)) {
						env << *rtspClient << "Failed to start video play for the \"" << *scs.subsession
							<< "\" subsession: " << env.getResultMsg() << "\n";
						break;
					}
					// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
					if (scs.subsession->rtcpInstance() != NULL) {
						scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
					}
					// Set receiver buffer size
					if (scs.subsession->rtpSource()) {
						int newsz;
						newsz = increaseReceiveBufferTo(env,
							scs.subsession->rtpSource()->RTPgs()->socketNum(), RCVBUF_SIZE);
						env << "Receiver buffer increased to " << newsz << "\n";
					}
					// TODO: Initialize video decoder

					// reference http://blog.chinaunix.net/uid-15063109-id-4482932.html
				}

				if ((strcmp(scs.subsession->codecName(), "H265") == 0)) {
					//char vparam[1024];
					//snprintf(vparam, sizeof(vparam), "%s,%s,%s",
					//	scs.subsession->fmtp_spropvps() == NULL ? "" : scs.subsession->fmtp_spropvps(),
					//	scs.subsession->fmtp_spropsps() == NULL ? "" : scs.subsession->fmtp_spropsps(),
					//	scs.subsession->fmtp_sproppps() == NULL ? "" : scs.subsession->fmtp_sproppps());

					//const char *sprop = scs.subsession->fmtp_spropparametersets();

					///*const char *sprop = vparam;*/
					//u_int8_t const* vps = NULL;
					//unsigned vpsSize = 0;
					//u_int8_t const* sps = NULL;
					//unsigned spsSize = 0;
					//u_int8_t const* pps = NULL;
					//unsigned ppsSize = 0;

					//if (sprop != NULL) {
					//	unsigned int numSPropRecords;
					//	SPropRecord* sPropRecords = parseSPropParameterSets(sprop, numSPropRecords);
					//	for (unsigned i = 0; i < numSPropRecords; ++i) {
					//		if (sPropRecords[i].sPropLength == 0) continue; // bad data
					//		u_int8_t nal_unit_type = (sPropRecords[i].sPropBytes[0]) & 0x3F;
					//		if (nal_unit_type == 32/*VPS*/) {
					//			vps = sPropRecords[i].sPropBytes;
					//			vpsSize = sPropRecords[i].sPropLength;
					//		}else if (nal_unit_type == 33/*SPS*/) {
					//			sps = sPropRecords[i].sPropBytes;
					//			spsSize = sPropRecords[i].sPropLength;
					//		}
					//		else if (nal_unit_type == 34/*PPS*/) {
					//			pps = sPropRecords[i].sPropBytes;
					//			ppsSize = sPropRecords[i].sPropLength;
					//		}
					//	}
					//}

					scs.subsession->sink = MediaH265MediaSink::createNew(env, rtspClient, *scs.subsession, rtspClient->url());
					/*scs.subsession->sink = MediaH264VideoRTPSink::createNew(env,
						scs.subsession,
						sps, spsSize, pps, ppsSize,
						rtspClient->url());*/
						/*scs.subsession->sink = H264VideoRTPSink::createNew(env,
							scs.subsession->rtpSource()->RTPgs(), scs.subsession->rtpPayloadFormat(),
							scs.subsession->fmtp_spropparametersets());*/
							// perhaps use your own custom "MediaSink" subclass instead
					if (scs.subsession->sink == NULL) {
						env << *rtspClient << "Failed to create a video data sink for the \"" << *scs.subsession
							<< "\" subsession: " << env.getResultMsg() << "\n";
						break;
					}
#if 0
					FFmpegDecoder* decoder = ((MediaH264MediaSink*)scs.subsession->sink)->getDecoder(); // alias
					if (decoder == NULL) {
						env << "Failed to get a video decoder\n";
						break;
					}
					decoder->openDecoder(1920, 1080, ((MediaRTSPClient*)rtspClient)->getRTSPSession());
#else
					H264Decoder* decoder = ((MediaH265MediaSink*)scs.subsession->sink)->getDecoder(); // alias
					if (decoder == NULL) {
						env << "Failed to get a video decoder\n";
						break;
					}
					decoder->setListener(((MediaRTSPClient*)rtspClient)->getRTSPSession());
					decoder->setRescaleSize(windows_width, windows_height); //set decode size
#endif

//					scs.subsession->videoWidth();
//					scs.subsession->videoHeight();

					env << *rtspClient << "Created a video data sink for the \"" << *scs.subsession << "\" subsession\n";
					scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 

					// Create the data source: an "H265 Video RTP source"
					//RTPSource* mVideoReceiverSource = H265VideoRTPSource::createNew(env, scs.subsession->rtpSource()->RTPgs(), scs.subsession->rtpPayloadFormat(), 90000);

					if (!scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
						subsessionAfterPlaying, scs.subsession)) {
						env << *rtspClient << "Failed to start video play for the \"" << *scs.subsession
							<< "\" subsession: " << env.getResultMsg() << "\n";
						break;
					}
					// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
					if (scs.subsession->rtcpInstance() != NULL) {
						scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
					}
					// Set receiver buffer size
					if (scs.subsession->rtpSource()) {
						int newsz;
						newsz = increaseReceiveBufferTo(env,
							scs.subsession->rtpSource()->RTPgs()->socketNum(), RCVBUF_SIZE);
						env << "Receiver buffer increased to " << newsz << "\n";
					}
					// TODO: Initialize video decoder

					// reference http://blog.chinaunix.net/uid-15063109-id-4482932.html
				}
			}

			if (strcmp(scs.subsession->mediumName(), "audio") == 0) {
				env << *rtspClient << "Set up the audio session: \"" << *scs.subsession
					<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1 << ")\n";
				// TODO: Initialize audio decoder
				if ((strcmp(scs.subsession->codecName(), "PCMU") == 0)) {
					const char *sprop = scs.subsession->fmtp_spropparametersets();

					if (sprop != NULL) {
						unsigned int numSPropRecords;
						SPropRecord* sPropRecords = parseSPropParameterSets(sprop, numSPropRecords);
						for (unsigned i = 0; i < numSPropRecords; ++i) {
							if (sPropRecords[i].sPropLength == 0) continue; // bad data
							// manipulate sprop 
						}
					}

					scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
					/*scs.subsession->sink = MediaH264VideoRTPSink::createNew(env,
					scs.subsession,
					sps, spsSize, pps, ppsSize,
					rtspClient->url());*/
					/*scs.subsession->sink = H264VideoRTPSink::createNew(env,
					scs.subsession->rtpSource()->RTPgs(), scs.subsession->rtpPayloadFormat(),
					scs.subsession->fmtp_spropparametersets());*/
					// perhaps use your own custom "MediaSink" subclass instead
					if (scs.subsession->sink == NULL) {
						env << *rtspClient << "Failed to create a audio data sink for the \"" << *scs.subsession
							<< "\" subsession: " << env.getResultMsg() << "\n";
						break;
					}

					env << *rtspClient << "Created a audio data sink for the \"" << *scs.subsession << "\" subsession\n";
					scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 

					// Create the data source: an "H264 Video RTP source"
					//RTPSource* mVideoReceiverSource = H264VideoRTPSource::createNew(env, scs.subsession->rtpSource()->RTPgs(), scs.subsession->rtpPayloadFormat(), 90000);

					if (!scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
						subsessionAfterPlaying, scs.subsession)) {
						env << *rtspClient << "Failed to start audio play for the \"" << *scs.subsession
							<< "\" subsession: " << env.getResultMsg() << "\n";
						break;
					}
					// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
					if (scs.subsession->rtcpInstance() != NULL) {
						scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
					}
					// Set receiver buffer size
					if (scs.subsession->rtpSource()) {
						int newsz;
						newsz = increaseReceiveBufferTo(env,
							scs.subsession->rtpSource()->RTPgs()->socketNum(), RCVBUF_SIZE);
						env << "Receiver buffer increased to " << newsz << "\n";
					}
				}
			}
		} else {
			// TODO: generate transport stream live
			// http://live-devel.live555.narkive.com/UK4Lxd4z/how-to-play-a-mpeg2-ts-h264-transport-stream-live-stream
			// TODO: prepare recording
			//		    Boolean             fHave_amba_audio;
			//		    Boolean             fHave_amba_video;

			//			 _check_media_configuration(((TechwinRTSPClient*) rtspClient)->getStreamReader(), NULL, &fHave_amba_audio, &fHave_amba_video);
		}
	} while (0);

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionAfterRecording(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL)
			return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

	// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
	MediaRTSPClient* rtspClient = (MediaRTSPClient*)clientData;
	StreamClientState& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;

	// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

																  // First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	env << *rtspClient << "Closing the stream.\n";
	Medium::close(rtspClient);
	// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

	/*	if (--rtspClientCount == 0) {
	// The final stream has ended, so exit the application now.
	// (Of course, if you're embedding this code into your own application, you might want to comment this out,
	// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
	exit(exitCode);
	}
	*/
}