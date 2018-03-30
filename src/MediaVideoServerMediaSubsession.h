/*******************************************************************************
 *  Copyright (c) 2016 Hanwha Techwin Co., Ltd.
 *
 *  Licensed to the Hanwha Techwin Software Foundation under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Smart Home Camera - Hanwha B2C Action Cam Project
 *  http://www.samsungsmartcam.com
 *
 *  Security Solution Division / Camera S/W Development Team
 *  Home Security Camera SW Dev. Group
 *
 *  @file
 *  Contains implementation of logging tools.
 *
 *  $Author: young_ho_kim $
 *  $LastChangedBy: young_ho_kim $
 *  $Date: 2016-08-03 23:58:29 +0900 (수, 03 8 2016) $
 *  $Revision: 2270 $
 *  $Id: AmbaVideoServerMediaSubsession.h 2270 2016-08-03 14:58:29Z young_ho_kim $
 *  $HeadURL: http://ctf1.stw.net/svn/repos/wearable_camera/trunk/Source/Pixcam/Wear-1.0.5/app/live555_server/AmbaVideoServerMediaSubsession.h $
 *******************************************************************************

  MODULE NAME:

  REVISION HISTORY:

  Date        Ver Name                    Description
  ----------  --- --------------------- -----------------------------------------
  07-Jun-2016 0.1 Youngho Kim             Created
 ...............................................................................

  DESCRIPTION:


 ...............................................................................
 *******************************************************************************/

#ifndef _MEDIA_VIDEO_SERVER_MEDIA_SUBSESSION_H
#define _MEDIA_VIDEO_SERVER_MEDIA_SUBSESSION_H

#ifndef _FILE_SERVER_MEDIA_SUBSESSION_HH
    #include "FileServerMediaSubsession.hh"
#endif

#ifndef _H264_VIDEO_RTP_SINK_HH
    #include "H264VideoRTPSink.hh"
#endif

#ifndef _RTCP_HH
    #include "RTCP.hh"
#endif

 //=============================================================================
//                  Class Definition
//=============================================================================
class MediaVideoServerMediaSubsession : public FileServerMediaSubsession
{
public:
    static MediaVideoServerMediaSubsession *
    createNew(UsageEnvironment &env, stream_reader_t *pStream_reader,
              char const *fileName, Boolean reuseFirstSource);

    virtual float duration() const;

    static void rtcpRRQos(void* clientData);

protected:
	MediaVideoServerMediaSubsession(UsageEnvironment &env, stream_reader_t *pStream_reader,
                                   char const *fileName, Boolean reuseFirstSource);
    // called only by createNew();
    virtual ~MediaVideoServerMediaSubsession();

    // redefined virtual functions
    virtual RTCPInstance* createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
                                     unsigned char const* cname, RTPSink* sink);

    virtual FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate);

    virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource *inputSource);

    virtual void seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes);

    static void afterPlaying(void* clientData);

private:
    // Private Parameters
    stream_type_t   fStream_type;
    u_int32_t       fStreamTag;
    stream_reader_t *fStream_reader;

    u_int8_t        *fSPS_payload;
    u_int32_t       fSPS_length;
    u_int8_t        *fPPS_payload;
    u_int32_t       fPPS_length;

    AMP_FORMAT_MID_e   fSourceCodec_ID;

    float       fDuration;

    RTPSink     *fVidRTPSink; // ditto
};

#endif /* _MEDIA_VIDEO_SERVER_MEDIA_SUBSESSION_H */
