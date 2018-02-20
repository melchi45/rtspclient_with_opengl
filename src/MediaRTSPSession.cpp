#include "MediaRTSPSession.h"

#include "MediaBasicUsageEnvironment.h"
#include "BasicUsageEnvironment.hh"
#include "MediaRTSPClient.h"
#include "DummySink.h"

char const* clientProtocolName = "RTSP";

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
	, bTransportStream(false)
	, m_port(0)
	, m_username("")
	, m_password("")
	, m_progName("")
	, m_rtspUrl("")
	, m_debugLevel(0)
{
	m_running = false;
	eventLoopWatchVariable = 0;
}

MediaRTSPSession::~MediaRTSPSession()
{
}

int MediaRTSPSession::startRTSPClient(const char* progName, const char* rtspURL, 
	const char* username, const char* password,
	bool bInterleaved, bool bTransportStream)
{
	this->m_username = std::string(username);
	this->m_password = std::string(password);
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
	return 0;
}

int MediaRTSPSession::stopRTSPClient()
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

void MediaRTSPSession::rtsp_fun()
{
	//::startRTSP(m_progName.c_str(), m_rtspUrl.c_str(), m_ndebugLever);
	taskScheduler = BasicTaskScheduler::createNew();
	usageEnvironment = MediaBasicUsageEnvironment::createNew(*taskScheduler);
	if (openURL(*usageEnvironment) == 0)
	{
//		m_nStatus = 1;
		usageEnvironment->taskScheduler().doEventLoop(&eventLoopWatchVariable);

		m_running = false;
		eventLoopWatchVariable = 0;

		if (m_rtspClient)
		{
			shutdownStream(m_rtspClient, 0);
		}
		m_rtspClient = NULL;
	}

	usageEnvironment->reclaim();
	usageEnvironment = NULL;

	delete taskScheduler;
	taskScheduler = NULL;
//	m_nStatus = 2;
}

int MediaRTSPSession::openURL(UsageEnvironment& env)
{
	// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
	// to receive (even if more than stream uses the same "rtsp://" URL).
	m_rtspClient = MediaRTSPClient::createNew(env, m_rtspUrl.c_str(), m_debugLevel, m_progName.c_str(), m_port, m_username.c_str(), m_password.c_str());
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

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((MediaRTSPClient*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		}
		else {
			env << *rtspClient << "Initiated the \"" << *scs.subsession
				<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1 << ")\n";

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
	}

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
			scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
			// perhaps use your own custom "MediaSink" subclass instead
			if (scs.subsession->sink == NULL) {
				env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
					<< "\" subsession: " << env.getResultMsg() << "\n";
				break;
			}

			if (strcmp(scs.subsession->mediumName(), "video") == 0) {
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

					if (sps != NULL) {
						//((DummySink *)scs.subsession->sink)->setSprop(sps, spsSize);
					}
					if (pps != NULL) {
						//((DummySink *)scs.subsession->sink)->setSprop(pps, ppsSize);
					}

					scs.subsession->videoWidth();
					scs.subsession->videoHeight();
				}
			}

			if (strcmp(scs.subsession->mediumName(), "audio") == 0) {

			}

			env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
			scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 
			scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				subsessionAfterPlaying, scs.subsession);
			// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
			if (scs.subsession->rtcpInstance() != NULL) {
				scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
			}
		}
		else {
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