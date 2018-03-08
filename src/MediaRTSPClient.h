#ifndef _MEDIA_RTSP_CLIENT_H_
#define _MEDIA_RTSP_CLIENT_H_

#pragma once

#include "liveMedia.hh"
#include "StreamClientState.h"
#include "MediaBasicUsageEnvironment.h"

class MediaRTSPSession;
// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:
class MediaRTSPClient : public RTSPClient {
public:
	static MediaRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0,
		const char* username = NULL,
		const char* password = NULL);

	static MediaRTSPClient* createNew(UsageEnvironment& env, 
		MediaRTSPSession* rtspSession,
		char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0,
		const char* username = NULL,
		const char* password = NULL);

protected:
	MediaRTSPClient(UsageEnvironment& env, MediaRTSPSession* rtspSession, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum,
		const char* username = NULL, const char* password = NULL);
	// called only by createNew();
	virtual ~MediaRTSPClient();

public:
	StreamClientState scs;
protected:
	Authenticator*	ourAuthenticator;
	char*			sdpDescription;
	bool			bUpTransport;
	bool			bInterleaved;
	MediaRTSPSession* mediaRTSPSession;
/*	
	stream_reader_t     *fStream_reader;
	//    Boolean             fHave_amba_audio;
	//    Boolean             fHave_amba_video;
	char* fFilename;
	stream_type_t   fStream_type;
	u_int32_t       fStreamTag;
	float       fDuration;

	u_int8_t        *fSPS_payload;
	u_int32_t       fSPS_length;
	u_int8_t        *fPPS_payload;
	u_int32_t       fPPS_length;

	AMP_FORMAT_MID_e fSourceCodec_ID;
*/
	RTPSink     *fVidRTPSink; // ditto
	RTPSink     *fAudRTPSink; // ditto

public:
	Authenticator * getAuth() { return ourAuthenticator; }
	MediaRTSPSession* getRTSPSession() { return mediaRTSPSession; }
	char* getSDPDescription();

/*
	stream_reader_t* getStreamReader() { return fStream_reader; }
	//	Boolean getHaveAmbaVideo() { return fHave_amba_video; }
	//	Boolean getHaveAmbaAudio() { return fHave_amba_audio; }
	stream_type_t* 	getStreamType() { return &fStream_type; }

	u_int8_t* getSPSPayload() { return fSPS_payload; }
	u_int32_t getSPSLength() { return fSPS_length; }
	u_int8_t* getPPSPayload() { return fPPS_payload; }
	u_int32_t getPPSLength() { return fPPS_length; }
	*/
	FramedSource* createNewVideoStreamSource();
	FramedSource* createNewAudioStreamSource();
	static int CheckMediaConfiguration(char const *streamName, Boolean *pHave_amba_audio, Boolean *pHave_amba_video);
	RTPSink* createNewRTPSink(Groupsock *rtpGroupsock,
		unsigned char rtpPayloadTypeIfDynamic,
		FramedSource * inputSource);

	RTPSink* getVideoRTPSink();
	RTPSink* getAudioRTPSink();

	bool	isUpTransportStream()			{ return bUpTransport; }
	void	setUpTransportStream(bool flag)	{ bUpTransport = flag; }
	bool	isInterleavedMode()				{ return bInterleaved; }
	void	setInterleavedMode(bool flag)	{ bInterleaved = flag; }

};

#endif // _MEDIA_RTSP_CLIENT_H_