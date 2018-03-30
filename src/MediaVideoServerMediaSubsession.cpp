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
 *  $Author: selvaganesan $
 *  $LastChangedBy: selvaganesan $
 *  $Date: 2016-12-19 21:17:40 +0900 (2016-12-19, Mon) $
 *  $Revision: 2949 $
 *  $Id: AmbaVideoServerMediaSubsession.cpp 2949 2016-12-19 12:17:40Z selvaganesan $
 *  $HeadURL: http://ctf1.stw.net/svn/repos/wearable_camera/trunk/Source/Pixcam/Wear-1.0.5/app/live555_server/AmbaVideoServerMediaSubsession.cpp $
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

#include "MediaVideoServerMediaSubsession.h"
#include "MediaVideoStreamSource.h"
#include "MPEG2TransportStreamMultiplexor.hh"

#include "MediaVideoRTPSink.h"

//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
MediaVideoServerMediaSubsession *
MediaVideoServerMediaSubsession::createNew(UsageEnvironment &env, stream_reader_t *pStream_reader,
                                          char const *fileName, Boolean reuseFirstSource)
{
    return new MediaVideoServerMediaSubsession(env, pStream_reader, fileName, reuseFirstSource);
}

MediaVideoServerMediaSubsession
::MediaVideoServerMediaSubsession(UsageEnvironment &env, stream_reader_t *pStream_reader,
                                 char const *fileName, Boolean reuseFirstSource)
    : FileServerMediaSubsession(env, fileName, reuseFirstSource),
      fVidRTPSink(NULL)
{
    fSPS_payload   = NULL;
    fSPS_length    = 0;
    fPPS_payload   = NULL;
    fPPS_length    = 0;
    fStream_reader = pStream_reader;
    fStream_type   = STREAM_TYPE_LIVE;
    fStreamTag     = 0;
    fDuration      = 0.0f;
    fSourceCodec_ID = AMP_FORMAT_MID_UNKNOW;

    //log_debug("CREATE: AmbaVideoServerMediaSubsession");

    if( fileName )
    {
        fStreamTag = calculateCRC((u_int8_t const*)fileName, (unsigned)strlen((const char*)fileName));
        fStream_type = STREAM_TYPE_PLAYBACK;
    }
    return;
}

MediaVideoServerMediaSubsession::~MediaVideoServerMediaSubsession()
{
    //log_debug("DELETE: ~AmbaVideoServerMediaSubsession");
}

FramedSource *MediaVideoServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned &estBitrate)
{
    estBitrate = 2000; // kbps, estimate

    // Create the video source:
    MediaVideoStreamSource   *pAmbaSource= NULL;

    pAmbaSource = (fStream_type == STREAM_TYPE_LIVE)
                ? AmbaVideoStreamSource::createNew(envir(), fStream_reader, NULL, (u_int32_t)fStreamTag)
                : AmbaVideoStreamSource::createNew(envir(), fStream_reader, fFileName, (u_int32_t)fStreamTag);

    fSourceCodec_ID = AMP_FORMAT_MID_UNKNOW;

    // get sps and pps from amba stream
	if (pAmbaSource) {
		int result = 0;
		fSourceCodec_ID = pAmbaSource->getSourceCodecID();
		//log_debug("fSourceCodec_ID=0x%x", fSourceCodec_ID);

		if (fStream_type == STREAM_TYPE_PLAYBACK) {
			stream_reader_ctrl_box_t ctrl_box;

			memset(&ctrl_box, 0x0, sizeof(stream_reader_ctrl_box_t));

			ctrl_box.cmd = STREAM_READER_CMD_PLAYBACK_GET_DURATION;
			ctrl_box.stream_tag = fStreamTag;
			ctrl_box.stream_type = fStream_type;
			StreamReader_Control(fStream_reader, NULL, &ctrl_box);

			fDuration = (float) ctrl_box.out.get_duration.duration / 1000.0;
		}

		switch (fSourceCodec_ID) {
		case AMP_FORMAT_MID_AVC:
		case AMP_FORMAT_MID_H264: {
			result = pAmbaSource->getSpsPps(&fSPS_payload, &fSPS_length,
					&fPPS_payload, &fPPS_length);
			if (result) {
				//log_debug("No SPS/PPS !");
				// Medium::close(pAmbaSource);
				// pAmbaSource = NULL;
			} else {
				//log_debug("SPS:[%d]/PPS:[%d]!", fSPS_length, fPPS_length);
			}
		}
			break;
		default:
			break;
		}
    }
    return pAmbaSource;
}

void MediaVideoServerMediaSubsession::rtcpRRQos(void* clientData)
{
	MediaVideoServerMediaSubsession *pSMSub =
			(MediaVideoServerMediaSubsession*) clientData;

	if (pSMSub == NULL || pSMSub->fVidRTPSink == NULL)
		return;

	do {
		RTPTransmissionStatsDB::Iterator statsIter(
				pSMSub->fVidRTPSink->transmissionStatsDB());
		RTPTransmissionStats *pCur_stats = statsIter.next();

		if (pCur_stats == NULL)
			break;

		if (pSMSub->fStream_type == STREAM_TYPE_LIVE) {
			stream_reader_ctrl_box_t ctrl_box;
			stream_status_info_t *pLive_stream_status = NULL;

			pLive_stream_status = &pSMSub->fStream_reader->live_stream_status;

			if (pLive_stream_status == NULL)
				break;

			// use pLive_stream_status->extra_data to make audio session skip SEND_RR_STAT
			if (pLive_stream_status->extra_data &&
			    pLive_stream_status->extra_data != (void*) STREAM_TRACK_VIDEO)
				break;

			memset(&ctrl_box, 0x0, sizeof(stream_reader_ctrl_box_t));

			ctrl_box.cmd = STREAM_READER_CMD_SEND_RR_STAT;
			ctrl_box.stream_tag = pSMSub->fStreamTag;
			ctrl_box.stream_type = pSMSub->fStream_type;
			ctrl_box.pExtra_data = (void*) STREAM_TRACK_VIDEO;
			ctrl_box.in.send_rr_stat.fraction_lost = pCur_stats->packetLossRatio();
			ctrl_box.in.send_rr_stat.jitter = pCur_stats->jitter();
			ctrl_box.in.send_rr_stat.propagation_delay = pCur_stats->roundTripDelay() / 65536.0f; // 64KHz
			StreamReader_Control(pSMSub->fStream_reader, NULL, &ctrl_box);
		}

		// Not Required - RTCP data continuously printing..
#if 0
		while (pCur_stats) {
			log_debug(
					"\n\tA:packetId=%u,\n\tlossRate=%u,\n\tallPacketsLost=%u,\n\tjitter=%u,\n\tlsr=%u,\n\tdlsr=%u,\n\tdelay=%u",
					pCur_stats->lastPacketNumReceived(),
					pCur_stats->packetLossRatio(),
					pCur_stats->totNumPacketsLost(), pCur_stats->jitter(),
					pCur_stats->lastSRTime(), pCur_stats->diffSR_RRTime(),
					pCur_stats->roundTripDelay()); // 64KHz

			pCur_stats = statsIter.next();
		}
#endif
	} while (0);

	return;
}

RTCPInstance* MediaVideoServerMediaSubsession
::createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
             unsigned char const* cname, RTPSink* sink)
{
    // Default implementation:
    RTCPInstance      *pRTCPInstance = NULL;

    pRTCPInstance = RTCPInstance::createNew(envir(), RTCPgs, totSessionBW,
                                            cname, sink, NULL/*we're a server*/);
    if( pRTCPInstance )
        pRTCPInstance->setRRHandler(rtcpRRQos, this);

    return pRTCPInstance;
}

void MediaVideoServerMediaSubsession::afterPlaying(void* clientData)
{
//	log_debug("*********************************************");
//	log_debug("AmbaVideoServerMediaSubsession::afterPlaying");
//	log_debug("*********************************************");
}

RTPSink *MediaVideoServerMediaSubsession
::createNewRTPSink(Groupsock *rtpGroupsock,
                   unsigned char rtpPayloadTypeIfDynamic,
                   FramedSource *inputSource)
{
    //log_rtsp("AmbaVideoServerMediaSubsession:createNewRTPSink");
    switch( fSourceCodec_ID )
    {
        case AMP_FORMAT_MID_AVC:
        case AMP_FORMAT_MID_H264: {
        	fVidRTPSink = MediaVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
                                                      fSPS_payload, fSPS_length, fPPS_payload, fPPS_length);
        	MediaVideoStreamSource* pAmbaSource= (MediaVideoStreamSource*)inputSource;
        	//log_rtsp("SDP = %s", getAuxSDPLine(fVidRTPSink, pAmbaSource));
        	envir() << "Amba Video SDP: " << getAuxSDPLine(fVidRTPSink, pAmbaSource);
            // Finally, start playing:
            //envir() << "Beginning to read from UPP...\n";
            //fVidRTPSink->startPlaying(*pAmbaSource, AmbaVideoServerMediaSubsession::afterPlaying, fVidRTPSink);
        }
            break;
        default:
            break;
    }

    //log_rtsp("fVidRTPSink = %p", fVidRTPSink);
    return fVidRTPSink;
}

void MediaVideoServerMediaSubsession
::seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration,
                   u_int64_t& /*numBytes*/)
{
	MediaVideoServerMediaSubsession   *pAmbaSource= (MediaVideoServerMediaSubsession*)inputSource;

    if( (float)seekNPT < fDuration )
        pAmbaSource->seekStream((int32_t)seekNPT);
}

float MediaVideoServerMediaSubsession::duration() const
{
    return fDuration;
}


