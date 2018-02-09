#ifndef _MEDIA_RTSP_SESSION_H_
#define _MEDIA_RTSP_SESSION_H_

#include <string>
#include <pthread.h>

class TaskScheduler;
class UsageEnvironment;
class RTSPClient;
class MediaRTSPSession
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

	static void *rtsp_thread_fun(void *param);
	void rtsp_fun();
};

#endif // _MEDIA_RTSP_SESSION_H_