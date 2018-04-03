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
#include "MediaRTSPServer.h"
#include <liveMedia.hh>
#include <string.h>
#include <pthread.h>	// for pthread_t definition


#include "MediaVideoServerMediaSubsession.h"
//#include "AmbaAudioServerMediaSubsession.h"
//#include "AmbaTextServerMediaSubsession.h"
#include "MPEG2TransportStreamMultiplexor.hh"

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

//extern int gStartState;
//=============================================================================
//                Macro Definition
//=============================================================================
////////// DynamicRTSPServer //////////
int ourSocket;
MediaRTSPServer*
MediaRTSPServer::createNew(UsageEnvironment& env, Port ourPort,
                             UserAuthenticationDatabase* authDatabase,
                             unsigned reclamationTestSeconds)
{
    ourSocket = setUpOurSocket(env, ourPort);
    if (ourSocket == -1) return NULL;
    return new MediaRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

static pthread_t rtos_event_thread;

MediaRTSPServer::MediaRTSPServer(UsageEnvironment& env, int ourSocket,
                                     Port ourPort,
                                     UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
    : RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds)
	, fRTSPClientConnection(0)
	, fHave_amba_video(True)
{
}

MediaRTSPServer::~MediaRTSPServer()
{
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
                                        Boolean has_amba_audio, Boolean has_amba_video, Boolean has_amba_text,
                                        char const* fileName, FILE* fid); // forward

ServerMediaSession* MediaRTSPServer
::lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession)
{
    // First, check whether the specified "streamName" exists as a local file:
    FILE    *fid = (streamName[0] == '/') ? fopen(streamName, "rb") : NULL;
    Boolean fileExists = (fid != NULL)? True:False;

    log_debug("***** get streamName= %s, fid=%p", streamName, fid);

    log_debug("isFirstLookupInSession = %d", isFirstLookupInSession);
    log_debug("fileExists = %d", fileExists);
    log_debug("streamName = %s", streamName);

//    if(!gStartState)
//    {
//        return NULL;
//    }

    // Next, check whether we already have a "ServerMediaSession" for this file:
    ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
    Boolean smsExists = (sms != NULL)? True:False;
    log_debug("smsExists = %d", smsExists);

    // Handle the four possibilities for "fileExists" and "smsExists":
    if (!fileExists) {
//#if defined(CFG_AMBA_STREAM)
        if( 0 == strcasecmp(LIVE_VEIW_NAME, streamName) )
        {
            log_debug("fHave_amba_video = %d", fHave_amba_video);
            log_debug("fHave_amba_audio = %d", fHave_amba_audio);
            log_debug("fHave_amba_text = %d", fHave_amba_text);

            // no source can get but media session exist.
            if( smsExists == True && fHave_amba_video == False )
            {
                removeServerMediaSession(sms);
                log_rtsp("(PREVIOUS)removeServerMediaSession");
                return NULL;
            }

            if( smsExists == False &&
                (fHave_amba_video || fHave_amba_audio) )
            {
                char const* descStr = LIVE_VEIW_NAME ", streamed by the Media Server";
                sms = ServerMediaSession::createNew(envir(), streamName, streamName, descStr);
                if( sms ) {
                    OutPacketBuffer::maxSize = 300000;
					if (fHave_amba_video)
					{
						log_debug("CREATING AmbaVideoServerMediaSubsession");
						sms->addSubsession(MediaVideoServerMediaSubsession::createNew(envir(), VIDEO_SOURCE_TYPE_SCREEN));
					}
/*
                    if( fHave_amba_video )
                    {
                        log_debug("CREATING AmbaVideoServerMediaSubsession");
                        sms->addSubsession(AmbaVideoServerMediaSubsession::createNew(envir(), fStream_reader, NULL, True));
                    }
                    if( fHave_amba_audio )
                    {
                        log_debug("CREATING AmbaAudioServerMediaSubsession");
                        sms->addSubsession(AmbaAudioServerMediaSubsession::createNew(envir(), fStream_reader, NULL, True));
                    }
                    if( fHave_amba_text )
                    {
                        log_debug("CREATING AmbaTextServerMediaSubsession");
                        sms->addSubsession(AmbaTextServerMediaSubsession::createNew(envir(), fStream_reader, NULL, True));
                    }*/
                    if( fHave_amba_video || fHave_amba_audio || fHave_amba_text )
                    {
                        log_debug("addServerMediaSession");
#if 0
                        m_mMediaSessName = strDup(streamName);
#endif
                        addServerMediaSession(sms);
                    }
                }
            }
            return sms;
        }
//#endif
        if (smsExists) {
            // "sms" was created for a file that no longer exists. Remove it:
            removeServerMediaSession(sms);
            log_debug("removeServerMediaSession");
            sms = NULL;
        }
        return NULL;
    } else {
        if (smsExists && isFirstLookupInSession) {
            // Remove the existing "ServerMediaSession" and create a new one, in case the underlying
            // file has changed in some way:
            removeServerMediaSession(sms);
            log_debug("removeServerMediaSession");
            sms = NULL;
        }

        if (sms == NULL) {
#if defined(CFG_AMBA_STREAM)
            _check_media_configuration(fStream_reader, streamName, &fHave_amba_audio, &fHave_amba_video, &fHave_amba_text);
#endif
            sms = createNewSMS(envir(), fHave_amba_audio, fHave_amba_video, fHave_amba_text, streamName, fid);
#if 0
            m_mMediaSessName = strDup(streamName);
#endif
            addServerMediaSession(sms);
            log_debug("addServerMediaSession");
        }

        fclose(fid);
        return sms;
    }
}

// Special code for handling Matroska files:
struct MatroskaDemuxCreationState {
    MatroskaFileServerDemux* demux;
    char watchVariable;
};

static void onMatroskaDemuxCreation(MatroskaFileServerDemux* newDemux, void* clientData)
{
    MatroskaDemuxCreationState* creationState = (MatroskaDemuxCreationState*)clientData;
    creationState->demux = newDemux;
    creationState->watchVariable = 1;
}
// END Special code for handling Matroska files:

// Special code for handling Ogg files:
struct OggDemuxCreationState {
    OggFileServerDemux* demux;
    char watchVariable;
};

static void onOggDemuxCreation(OggFileServerDemux* newDemux, void* clientData)
{
    OggDemuxCreationState* creationState = (OggDemuxCreationState*)clientData;
    creationState->demux = newDemux;
    creationState->watchVariable = 1;
}
// END Special code for handling Ogg files:

#define NEW_SMS(description) do {\
        char const* descStr = description\
            ", streamed by the LIVE555 Media Server";\
        sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
    } while(0)

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
                                        Boolean has_amba_audio, Boolean has_amba_video, Boolean has_amba_text,
                                        char const* fileName, FILE* /*fid*/)
{
    // Use the file name extension to determine the type of "ServerMediaSession":
    char const* extension = strrchr(fileName, '.');
    if (extension == NULL) return NULL;

    ServerMediaSession* sms = NULL;
    Boolean const reuseSource = False;
//#if defined(CFG_AMBA_STREAM)
    if( (has_amba_video == True || has_amba_audio == True  || has_amba_text == True) &&
        (strcasecmp(extension, ".mrv") == 0) )
    {
        NEW_SMS("Amba MRV");
        if( sms ) {
            MediaVideoServerMediaSubsession   *vid_subsession = NULL;
            //AmbaAudioServerMediaSubsession   *aud_subsession = NULL;
            //AmbaTextServerMediaSubsession    *txt_subsession = NULL;
            OutPacketBuffer::maxSize = 300000;

            if( has_amba_video )
            {
                vid_subsession = MediaVideoServerMediaSubsession::createNew(env, VIDEO_SOURCE_TYPE_SCREEN);
                sms->addSubsession(vid_subsession);
            }
/*
            if( has_amba_audio )
            {
                aud_subsession = AmbaAudioServerMediaSubsession::createNew(env, pStream_reader, fileName, reuseSource);
                sms->addSubsession(aud_subsession);
            }

            if( has_amba_text )
            {
                txt_subsession = AmbaTextServerMediaSubsession::createNew(env, pStream_reader, fileName, reuseSource);
                sms->addSubsession(txt_subsession);
            }*/
        }
    }
    else
//#endif
    if(strcmp(extension, ".aac") == 0) {
        // Assumed to be an AAC Audio (ADTS format) file:
        NEW_SMS("AAC Audio");
        sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".amr") == 0) {
        // Assumed to be an AMR Audio file:
        NEW_SMS("AMR Audio");
        sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".ac3") == 0) {
        // Assumed to be an AC-3 Audio file:
        NEW_SMS("AC-3 Audio");
        sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".m4e") == 0) {
        // Assumed to be a MPEG-4 Video Elementary Stream file:
        NEW_SMS("MPEG-4 Video");
        sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".264") == 0) {
        // Assumed to be a H.264 Video Elementary Stream file:
        NEW_SMS("H.264 Video");
        OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.264 frames
        sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".265") == 0) {
        // Assumed to be a H.265 Video Elementary Stream file:
        NEW_SMS("H.265 Video");
        OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.265 frames
        sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".mp3") == 0) {
        // Assumed to be a MPEG-1 or 2 Audio file:
        NEW_SMS("MPEG-1 or 2 Audio");
        // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
//#define STREAM_USING_ADUS 1
        // To also reorder ADUs before streaming, uncomment the following:
//#define INTERLEAVE_ADUS 1
        // (For more information about ADUs and interleaving,
        //  see <http://www.live555.com/rtp-mp3/>)
        Boolean useADUs = False;
        Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
        useADUs = True;
#ifdef INTERLEAVE_ADUS
        unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
        unsigned const interleaveCycleSize
            = (sizeof interleaveCycle)/(sizeof (unsigned char));
        interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
        sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
    } else if (strcmp(extension, ".mpg") == 0) {
        // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
        NEW_SMS("MPEG-1 or 2 Program Stream");
        MPEG1or2FileServerDemux* demux
            = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAudioServerMediaSubsession());
    } else if (strcmp(extension, ".vob") == 0) {
        // Assumed to be a VOB (MPEG-2 Program Stream, with AC-3 audio) file:
        NEW_SMS("VOB (MPEG-2 video with AC-3 audio)");
        MPEG1or2FileServerDemux* demux
            = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAC3AudioServerMediaSubsession());
    } else if (strcmp(extension, ".ts") == 0) {
        // Assumed to be a MPEG Transport Stream file:
        // Use an index file name that's the same as the TS file name, except with ".tsx":
        unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
        char* indexFileName = new char[indexFileNameLen];
        sprintf(indexFileName, "%sx", fileName);
        NEW_SMS("MPEG Transport Stream");
        sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
        delete[] indexFileName;
    } else if (strcmp(extension, ".wav") == 0) {
        // Assumed to be a WAV Audio file:
        NEW_SMS("WAV Audio Stream");
        // To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
        // change the following to True:
        Boolean convertToULaw = False;
        sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
    } else if (strcmp(extension, ".dv") == 0) {
        // Assumed to be a DV Video file
        // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
        OutPacketBuffer::maxSize = 300000;

        NEW_SMS("DV Video");
        sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0) {
        // Assumed to be a Matroska file (note that WebM ('.webm') files are also Matroska files)
        OutPacketBuffer::maxSize = 100000; // allow for some possibly large VP8 or VP9 frames
        NEW_SMS("Matroska video+audio+(optional)subtitles");

        // Create a Matroska file server demultiplexor for the specified file.
        // (We enter the event loop to wait for this to complete.)
        MatroskaDemuxCreationState creationState;
        creationState.watchVariable = 0;
        MatroskaFileServerDemux::createNew(env, fileName, onMatroskaDemuxCreation, &creationState);
        env.taskScheduler().doEventLoop(&creationState.watchVariable);

        ServerMediaSubsession* smss;
        while ((smss = creationState.demux->newServerMediaSubsession()) != NULL) {
            sms->addSubsession(smss);
        }
    } else if (strcmp(extension, ".ogg") == 0 || strcmp(extension, ".ogv") == 0 || strcmp(extension, ".opus") == 0) {
        // Assumed to be an Ogg file
        NEW_SMS("Ogg video and/or audio");

        // Create a Ogg file server demultiplexor for the specified file.
        // (We enter the event loop to wait for this to complete.)
        OggDemuxCreationState creationState;
        creationState.watchVariable = 0;
        OggFileServerDemux::createNew(env, fileName, onOggDemuxCreation, &creationState);
        env.taskScheduler().doEventLoop(&creationState.watchVariable);

        ServerMediaSubsession* smss;
        while ((smss = creationState.demux->newServerMediaSubsession()) != NULL) {
            sms->addSubsession(smss);
        }
    }

    return sms;
}

MediaRTSPServer::MediaRTSPClientConnection::MediaRTSPClientConnection(
		RTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr)
: RTSPClientConnectionSupportingHTTPStreaming(ourServer, clientSocket, clientAddr)
{
    log_rtsp("*********************************************************");
    log_rtsp("DynamicRTSPClientConnection::DynamicRTSPClientConnection");
    log_rtsp("*********************************************************");
}

MediaRTSPServer::MediaRTSPClientConnection::~MediaRTSPClientConnection()
{
    log_rtsp("*********************************************************");
    log_rtsp("DynamicRTSPClientConnection::~DynamicRTSPClientConnection");
    log_rtsp("*********************************************************");
}

void MediaRTSPServer::MediaRTSPClientConnection::handleRequestBytes(int newBytesRead)
{
#if 0
    log_rtsp("*********************************************************");
    log_rtsp("DynamicRTSPClientConnection::handleRequestBytes");
    log_rtsp("*********************************************************");
#endif
    RTSPServer::RTSPClientConnection::handleRequestBytes(newBytesRead);
}

void MediaRTSPServer::MediaRTSPClientConnection::handleCmd_OPTIONS()
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientConnection::handleCmd_OPTIONS");
	log_rtsp("*********************************************************");

	// Construct a response to the "OPTIONS" command that notes that our special headers (for RTSP-over-HTTP tunneling) are allowed:
/*
	snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "HTTP/1.1 200 OK\r\n"
	   "%s"
	   "Access-Control-Allow-Origin: *\r\n"
	   "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
	   "Access-Control-Allow-Headers: x-sessioncookie, Pragma, Cache-Control\r\n"
	   "Access-Control-Max-Age: 1728000\r\n"
	   "\r\n",
	   dateHeader());
*/

//	if( !gStartState ) {
    	// This usually means that netfifo is not ready
//        setRTSPResponse("503 Service Unavailable");
//	} else {
		RTSPServer::RTSPClientConnection::handleCmd_OPTIONS();
//	}
}

void MediaRTSPServer::MediaRTSPClientConnection::handleCmd_DESCRIBE(
		char const* urlPreSuffix, char const* urlSuffix,
		char const* fullRequestStr)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientConnection::handleCmd_DESCRIBE");
	log_rtsp("*********************************************************");

//    if( !gStartState )
//    {
    	// This usually means that netfifo is not ready
//        setRTSPResponse("503 Service Unavailable");
//    } else {
    	RTSPServer::RTSPClientConnection::handleCmd_DESCRIBE(urlPreSuffix, urlSuffix, fullRequestStr);
//    }
}


void MediaRTSPServer::MediaRTSPClientSession::handleCmd_SETUP(
		RTSPServer::RTSPClientConnection* ourClientConnection,
		char const* urlPreSuffix, char const* urlSuffix,
		char const* fullRequestStr)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_SETUP");
	log_rtsp("*********************************************************");

	RTSPServer::RTSPClientSession::handleCmd_SETUP(ourClientConnection,
			urlPreSuffix, urlSuffix, fullRequestStr);
}

void MediaRTSPServer::MediaRTSPClientSession::handleCmd_withinSession(
		RTSPClientConnection* ourClientConnection, char const* cmdName,
		char const* urlPreSuffix, char const* urlSuffix,
		char const* fullRequestStr)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_withinSession");
	log_rtsp("*********************************************************");

	RTSPServer::RTSPClientSession::handleCmd_withinSession(ourClientConnection,
			cmdName, urlPreSuffix, urlSuffix, fullRequestStr);
}

void MediaRTSPServer::MediaRTSPClientSession::handleCmd_PLAY(
		RTSPClientConnection* ourClientConnection,
		ServerMediaSubsession* subsession, char const* fullRequestStr) {
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_PLAY");
	log_rtsp("*********************************************************");
	// SETUP in existing session: let base class handle it
	RTSPServer::RTSPClientSession::handleCmd_PLAY(ourClientConnection,
			subsession, fullRequestStr);
}

void MediaRTSPServer::MediaRTSPClientSession::handleCmd_PAUSE(RTSPClientConnection* ourClientConnection,
		ServerMediaSubsession* subsession)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_PAUSE");
	log_rtsp("*********************************************************");
	RTSPServer::RTSPClientSession::handleCmd_PAUSE(ourClientConnection,
			subsession);
}

void MediaRTSPServer::MediaRTSPClientSession::handleCmd_TEARDOWN(
		RTSPClientConnection* ourClientConnection,
		ServerMediaSubsession* subsession)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_TEARDOWN");
	log_rtsp("*********************************************************");
	RTSPServer::RTSPClientSession::handleCmd_TEARDOWN(
			ourClientConnection,
			subsession);
}

void MediaRTSPServer::MediaRTSPClientSession::handleCmd_GET_PARAMETER(
		RTSPClientConnection* ourClientConnection,
		ServerMediaSubsession* subsession, char const* fullRequestStr)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_GET_PARAMETER");
	log_rtsp("*********************************************************");
	RTSPServer::RTSPClientSession::handleCmd_GET_PARAMETER(
			ourClientConnection,
			subsession, fullRequestStr);
}

void MediaRTSPServer::MediaRTSPClientSession::handleCmd_SET_PARAMETER(
		RTSPClientConnection* ourClientConnection,
		ServerMediaSubsession* subsession, char const* fullRequestStr)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPClientSession::handleCmd_SET_PARAMETER");
	log_rtsp("*********************************************************");
	RTSPServer::RTSPClientSession::handleCmd_SET_PARAMETER(
			ourClientConnection,
			subsession, fullRequestStr);
}


 // If you subclass "RTSPClientConnection", then you must also redefine this virtual function in order
   // to create new objects of your subclass:
GenericMediaServer::ClientConnection* MediaRTSPServer
::createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
{
	int rval = -1;

	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPServer::createNewClientConnection: %s", inet_ntoa(clientAddr.sin_addr));
	log_rtsp("*********************************************************");

    return new MediaRTSPClientConnection(*this, clientSocket, clientAddr);
}

GenericMediaServer::ClientSession* MediaRTSPServer
::createNewClientSession(u_int32_t sessionId)
{
	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPServer::createNewClientSession: %u", sessionId);
	log_rtsp("*********************************************************");
	MediaRTSPClientSession* pSession = new MediaRTSPClientSession(*this, sessionId);
	m_mRtspClientSessions[sessionId] = pSession;
	return pSession;
}

void MediaRTSPServer
::removeClientSession(unsigned sessionId)
{
	int rval = -1;

	log_rtsp("*********************************************************");
	log_rtsp("DynamicRTSPServer::removeClientSession: %u", sessionId);
	log_rtsp("*********************************************************");

	MediaClientSessionMap_t::iterator it = m_mRtspClientSessions.find(sessionId);
	if (it != m_mRtspClientSessions.end()) {
		m_mRtspClientSessions.erase(it);
#if 0
		if(m_mMediaSessName != NULL)
		{
		    ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(m_mMediaSessName);
		    if(sms != NULL)
		    {
		        log_info("Media Session is Removed");
		        removeServerMediaSession(sms);
		        delete[] m_mMediaSessName;
		        m_mMediaSessName = NULL;
		        sms = NULL;
		    }
		}
#endif
	} else {
		log_rtsp("Unable to remove client session %u", sessionId);
	}
}