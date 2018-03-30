#include <iostream>
#include <signal.h> // for signal
#ifdef WIN32
#include <windows.h>	// for using the function Sleep
#else
#include <stdlib.h>     // for using the function sleep
#include <unistd.h>		// for sleep
#endif

#include "MediaRTSPSession.h"
#include "log_utils.h"
#include "H264ReadCameraEncoder.h"
#include "H264ReadScreenEncoder.h"

//char eventLoopWatchVariable = 0;
char* username = NULL;
char* password = NULL;
char* rtsp_url = NULL;
char* filename = NULL;
bool bUpStream = false;
bool bInterleaved = false;
MediaRTSPSession* pRtsp = NULL;

/* Allow ourselves to be shut down gracefully by a signal */
void _signalHandlerShutdown(int sig);

void usage(char const* progName) {
	std::cout << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
	std::cout << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

int main(int argc, char** argv) {
	pRtsp = new MediaRTSPSession();
	bool option_param = false;
	bool isServer = false;
	// We need at least one "rtsp://" URL argument:
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	while (argc > 2) {
		char* const opt = argv[1];

		if (opt[0] != '-' && !option_param) {
			usage(argv[0]);
			option_param = false;
		}

		switch (opt[1]) {
		case 'r':
			rtsp_url = argv[2];

			std::cout << "RTSP Url: " << rtsp_url << "\n";

			option_param = true;
			break;
		case 'U':
			// specify start port number
			bUpStream = true;
			option_param = true;
			break;
		case 'u':
			// specify start port number
			username = argv[2];
			password = argv[3];

			std::cout << "Username: " << username << "\n";
			std::cout << "Password: " << password << "\n";

			option_param = true;
			break;
		case 'f': 
			filename = argv[2];
			std::cout << "File Name: " << filename << "\n";
			option_param = true;
			 break;
		case 'i':
			bInterleaved = true;
			option_param = true;
			break;
		case 's':
			isServer = true;
			break;
		default:
			break;
		}
		++argv;
		--argc;
	}
	
	/* Allow ourselves to be shut down gracefully by a signal */
	signal(SIGTERM, _signalHandlerShutdown);
	signal(SIGINT, _signalHandlerShutdown);
	signal(SIGSEGV, _signalHandlerShutdown);

	if (!isServer) {
		pRtsp->setDebugLevel(1);
		if (pRtsp->startRTSPClient("MyClient", rtsp_url, username, password, bInterleaved, bUpStream))
		{
			delete pRtsp;
			pRtsp = NULL;

			return -1;
		}

	}
	else {
		H264ReadCameraEncoder enc;
		//H264ReadScreenEncoder enc;

	}
	while (true) {
#ifdef WIN32
		Sleep(5000);         // wait for 5 secondes before closing
#else
		sleep(5000);         // wait for 5 secondes before closing
#endif
	}

	return 0;
}

void _signalHandlerShutdown(int sig)
{
	log_error("%s: Got signal %d, program exits!", __FUNCTION__, sig);

	pRtsp->stopRTSPClient();
	delete pRtsp;
	pRtsp = NULL;
}