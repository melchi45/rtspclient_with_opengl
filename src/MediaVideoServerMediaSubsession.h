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
#ifndef _MEDIA_VIDEO_SERVER_MEDIA_SUBSESSION_H
#define _MEDIA_VIDEO_SERVER_MEDIA_SUBSESSION_H

class MediaVideoServerMediaSubsession: public OnDemandServerMediaSubsession
  {
    public:
      static MediaVideoServerMediaSubsession* createNew(UsageEnvironment& env, StreamReplicator* replicator);
    
    protected:
      MediaVideoServerMediaSubsession(UsageEnvironment& env, StreamReplicator* replicator)
          : OnDemandServerMediaSubsession(env, False), m_replicator(replicator) {};
      
      virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
      virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,  unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);    
  };

#endif /* _MEDIA_VIDEO_SERVER_MEDIA_SUBSESSION_H */
