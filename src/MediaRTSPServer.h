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
#ifndef _MEDIA_RTSP_SERVER_H_
#define _MEDIA_RTSP_SERVER_H_

#include <map>
#include <string>

#ifndef _RTSP_SERVER_SUPPORTING_HTTP_STREAMING_HH
#include "RTSPServer.hh"
#endif

#define LIVE_VEIW_NAME      "live"

#include "log_utils.h"

/**
 * Our RTSP server class is derived from the liveMedia RTSP server. It extends the live555 RTSP server
 * to stream live media sessions.
 *
 * It also adds the capability to set the maximum number of connected clients.
 * It adds the ability to kick clients off the server.
 */
class MediaRTSPServer: public RTSPServer
{
public:
    static MediaRTSPServer* createNew(UsageEnvironment& env, Port ourPort = 554,
                                        UserAuthenticationDatabase* authDatabase = NULL,
                                        unsigned reclamationSeconds = 65);

protected:
	MediaRTSPServer(UsageEnvironment& env,
			int ourSocketIPv4, int ourSocketIPv6, Port ourPort,
			UserAuthenticationDatabase* authDatabase,
			unsigned reclamationSeconds);
    // called only by createNew();
    virtual ~MediaRTSPServer();

private: // redefined virtual functions
    //virtual ServerMediaSession* lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession);
		virtual void lookupServerMediaSession(char const* streamName,
			lookupServerMediaSessionCompletionFunc* completionFunc,
			void* completionClientData,
			Boolean isFirstLookupInSession);

protected:
    // reference url:
    // https://github.com/miseri/LiveMediaExt/blob/master/include/LiveMediaExt/LiveRtspServer.h
    class MediaRTSPClientSession;
    /**
      * @brief Subclassing this to make the client address acessible and add handleCmd_notEnoughBandwidth.
      */
    class MediaRTSPClientConnection : public RTSPClientConnection {
    friend class MediaRTSPServer;
	friend class MediaRTSPClientSession;
	public:
		/**
		* @brief Constructor
		*/
		MediaRTSPClientConnection(RTSPServer& ourServer, int clientSocket, struct sockaddr_storage clientAddr);
		/**
		* @brief Destructor
		*/
		virtual ~MediaRTSPClientConnection();
	protected:
		/**
		 * @brief Getter for client address
		 */
		struct sockaddr_storage getClientAddr() const { return fClientAddr; }

		virtual void handleRequestBytes(int newBytesRead);

		/**
		 * @brief This method can be called to respond to requests where there is insufficient bandwidth
		 * to handle them.
		 */
		virtual void handleCmd_notEnoughBandwidth()
		{
		  setRTSPResponse("453 Not Enough Bandwidth");
		}

		virtual void handleCmd_OPTIONS();
		virtual void handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr);
    };

    class MediaRTSPClientSession : public RTSPServer::RTSPClientSession
    {
    public:
		MediaRTSPClientSession(MediaRTSPServer& ourServer, unsigned sessionId)
        : RTSPClientSession(ourServer, sessionId),
        m_pParent(&ourServer),
        m_uiSessionId(sessionId)
		{
    	    log_rtsp("(CREATE)MediaRTSPClientSession:MediaRTSPClientSession");
		}

      virtual ~MediaRTSPClientSession()
      {
          log_rtsp("(DELETE)MediaRTSPClientSession:~MediaRTSPClientSession");
        // We need to check if the parent is still valid
        // in the case where the client session outlives the
        // RTSPServer child class implementation! In that case
        // the RTSPServer destructor deletes all the client
        // sessions, but at this point m_pParent is not valid
        // anymore. This is the reason for the orphan method.
        if (m_pParent)
          m_pParent->removeClientSession(m_uiSessionId);
      }
      /**
       * @brief invalidates the pointer to the DynamicRTSPServer object.
       */
      void orphan()
      {
        m_pParent = NULL;
      }
      friend class MediaRTSPServer;
      friend class DynamicRTSPClientConnection;

    protected:
		virtual void handleCmd_SETUP(
				RTSPServer::RTSPClientConnection* ourClientConnection,
				char const* urlPreSuffix, char const* urlSuffix,
				char const* fullRequestStr);

		virtual void handleCmd_withinSession(
				RTSPClientConnection* ourClientConnection, char const* cmdName,
				char const* urlPreSuffix, char const* urlSuffix,
				char const* fullRequestStr);

		virtual void handleCmd_PLAY(RTSPClientConnection* ourClientConnection,
				ServerMediaSubsession* subsession, char const* fullRequestStr);

		virtual void handleCmd_PAUSE(RTSPClientConnection* ourClientConnection,
				ServerMediaSubsession* subsession);

		virtual void handleCmd_TEARDOWN(
				RTSPClientConnection* ourClientConnection,
				ServerMediaSubsession* subsession);

		virtual void handleCmd_GET_PARAMETER(
				RTSPClientConnection* ourClientConnection,
				ServerMediaSubsession* subsession, char const* fullRequestStr);

		virtual void handleCmd_SET_PARAMETER(
				RTSPClientConnection* ourClientConnection,
				ServerMediaSubsession* subsession, char const* fullRequestStr);


      MediaRTSPServer* m_pParent;
      unsigned m_uiSessionId;
    };

	// If you subclass "RTSPClientConnection", then you must also redefine this virtual function in order
	// to create new objects of your subclass:
	virtual ClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_storage const& clientAddr);

	// If you subclass "RTSPClientSession", then you must also redefine this virtual function in order
	// to create new objects of your subclass:
	virtual ClientSession* createNewClientSession(u_int32_t sessionId);

	void removeClientSession(unsigned sessionId);
private:
  	typedef std::map<unsigned, MediaRTSPClientSession*> MediaClientSessionMap_t;
  	/// map to store a pointer to client sessions on creation
	MediaClientSessionMap_t m_mRtspClientSessions;

#if 0
  	// To remove the media session, store the stream name
  	char *m_mMediaSessName;
#endif

private:
    Boolean             fHave_amba_audio;
    Boolean             fHave_amba_video;
    Boolean             fHave_amba_text;
    unsigned int        fRTSPClientConnection;
};

#endif /* _MEDIA_RTSP_SERVER_H_ */
