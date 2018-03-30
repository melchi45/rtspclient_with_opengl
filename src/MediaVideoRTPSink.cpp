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
#include "MediaVideoRTPSink.h"
#include "Base64.hh"
#include "MediaVideoFragmenter.h"


//#include "stream_reader.h"

#ifdef TECHWIN_GET_PRE_SPS_PPS
extern stream_reader_frm_info_t frm_sz;
#endif

////////// H264VideoRTPSink implementation //////////

MediaVideoRTPSink
::MediaVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat,
		   u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize)
  : H264or5VideoRTPSink(264, env, RTPgs, rtpPayloadFormat,
			NULL, 0, sps, spsSize, pps, ppsSize) {
//    log_debug("Create: AmbaVideoRTPSink");
}

MediaVideoRTPSink::~MediaVideoRTPSink() {
    //log_debug("Delete: ~AmbaVideoRTPSink");
}

MediaVideoRTPSink* MediaVideoRTPSink
::createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat) {
  return new MediaVideoRTPSink(env, RTPgs, rtpPayloadFormat);
}

MediaVideoRTPSink* MediaVideoRTPSink
::createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat,
	    u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize) {

// Hard coded [SPS, PPS] value used for testing ... Hence commenting.
#ifdef TECHWIN_GET_PRE_SPS_PPS
	//base for 720p
	u_int8_t SPS_temp[44] = {
							0x27,0x4D,0x00,0x1F,0x9A,0x64,0x02,0x80,0x2D,0xD3,
							0x50,0x10,0x10,0x14,0x00,0x00,0x0F,0xA4,0x00,0x03,
							0xA9,0x83,0xA1,0x80,0x13,0xD8,0x00,0x13,0xD6,0xEE,
							0xF2,0xE3,0x43,0x00,0x27,0xB0,0x00,0x27,0xAD,0xDD,
							0xE5,0xC2,0x80,0x00
							};
	u_int8_t PPS_temp[4] = {0x28,0xEE,0x3C,0x80};
	log_debug(">>>>>>>>>> frm_sz.wid:%d frm_sz.hgh:%d spsSize:%d ppsSize:%d\n",frm_sz.wid,frm_sz.hgh,spsSize,ppsSize);

	if((frm_sz.wid == 640) && (frm_sz.hgh == 480)) //VGA, the other are all HD(720p)
	{
		SPS_temp[3] = 0x1e;
	}

	sps = (u_int8_t const*)&SPS_temp[0];
	pps = (u_int8_t const*)&PPS_temp[0];
	spsSize = 44;
	ppsSize = 4;
#endif

  return new MediaVideoRTPSink(env, RTPgs, rtpPayloadFormat, sps, spsSize, pps, ppsSize);
}

MediaVideoRTPSink* MediaVideoRTPSink
::createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat,
	    char const* sPropParameterSetsStr) {
  u_int8_t* sps = NULL; unsigned spsSize = 0;
  u_int8_t* pps = NULL; unsigned ppsSize = 0;

  unsigned numSPropRecords;
  SPropRecord* sPropRecords = parseSPropParameterSets(sPropParameterSetsStr, numSPropRecords);
  for (unsigned i = 0; i < numSPropRecords; ++i) {
    if (sPropRecords[i].sPropLength == 0) continue; // bad data
    u_int8_t nal_unit_type = (sPropRecords[i].sPropBytes[0])&0x1F;
    if (nal_unit_type == 7/*SPS*/) {
      sps = sPropRecords[i].sPropBytes;
      spsSize = sPropRecords[i].sPropLength;
    } else if (nal_unit_type == 8/*PPS*/) {
      pps = sPropRecords[i].sPropBytes;
      ppsSize = sPropRecords[i].sPropLength;
    }
  }

  MediaVideoRTPSink* result
    = new MediaVideoRTPSink(env, RTPgs, rtpPayloadFormat, sps, spsSize, pps, ppsSize);
  delete[] sPropRecords;

  return result;
}

Boolean MediaVideoRTPSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // Our source must be an appropriate framer:
  return source.isH264VideoStreamFramer();
}

char const* MediaVideoRTPSink::auxSDPLine() {
  // Generate a new "a=fmtp:" line each time, using our SPS and PPS (if we have them),
  // otherwise parameters from our framer source (in case they've changed since the last time that
  // we were called):
  H264or5VideoStreamFramer* framerSource = NULL;
  u_int8_t* vpsDummy = NULL; unsigned vpsDummySize = 0;
  u_int8_t* sps = fSPS; unsigned spsSize = fSPSSize;
  u_int8_t* pps = fPPS; unsigned ppsSize = fPPSSize;

  //log_debug("%s", __func__);

  if (sps == NULL || pps == NULL) {
    // We need to get SPS and PPS from our framer source:
    //log_debug("SPS and PPS are NULL");
    if (fOurFragmenter == NULL) return NULL; // we don't yet have a fragmenter (and therefore not a source)
    framerSource = (H264or5VideoStreamFramer*)(fOurFragmenter->inputSource());
    if (framerSource == NULL) return NULL; // we don't yet have a source

    framerSource->getVPSandSPSandPPS(vpsDummy, vpsDummySize, sps, spsSize, pps, ppsSize);
    if (sps == NULL || pps == NULL) return NULL; // our source isn't ready
  }

  // Set up the "a=fmtp:" SDP line for this stream:
  u_int8_t* spsWEB = new u_int8_t[spsSize]; // "WEB" means "Without Emulation Bytes"
  unsigned spsWEBSize = removeH264or5EmulationBytes(spsWEB, spsSize, sps, spsSize);
  if (spsWEBSize < 4) { // Bad SPS size => assume our source isn't ready
    delete[] spsWEB;
    return NULL;
  }
  u_int32_t profileLevelId = (spsWEB[1]<<16) | (spsWEB[2]<<8) | spsWEB[3];
  delete[] spsWEB;

  char* sps_base64 = base64Encode((char*)sps, spsSize);
  char* pps_base64 = base64Encode((char*)pps, ppsSize);

  char const* fmtpFmt =
    "a=fmtp:%d packetization-mode=1"
    ";profile-level-id=%06X"
    ";sprop-parameter-sets=%s,%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 6 /* 3 bytes in hex */
    + strlen(sps_base64) + strlen(pps_base64);
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
          rtpPayloadType(),
	  profileLevelId,
          sps_base64, pps_base64);

  delete[] sps_base64;
  delete[] pps_base64;
  delete[] fFmtpSDPLine;
  fFmtpSDPLine = fmtp;

  //log_info("SDP = %s", fFmtpSDPLine);

  return fFmtpSDPLine;
}


Boolean MediaVideoRTPSink::continuePlaying()
{
	  // First, check whether we have a 'fragmenter' class set up yet.
	  // If not, create it now:
	  if (fOurFragmenter == NULL) {
	    fOurFragmenter = new MediaVideoFragmenter(fHNumber, envir(), fSource, OutPacketBuffer::maxSize,
						   ourMaxPacketSize() - 12/*RTP hdr size*/);
	  } else {
	    fOurFragmenter->reassignInputSource(fSource);
	  }
	  fSource = fOurFragmenter;

	  // Then call the parent class's implementation:
	  return MultiFramedRTPSink::continuePlaying();
}

void MediaVideoRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
						 unsigned char* frameStart,
						 unsigned numBytesInFrame,
						 struct timeval framePresentationTime,
						 unsigned numRemainingBytes)
{
	  // Set the RTP 'M' (marker) bit iff
	  // 1/ The most recently delivered fragment was the end of (or the only fragment of) an NAL unit, and
	  // 2/ This NAL unit was the last NAL unit of an 'access unit' (i.e. video frame).
	  if (fOurFragmenter != NULL) {
	    H264or5VideoStreamFramer* framerSource
	      = (H264or5VideoStreamFramer*)(fOurFragmenter->inputSource());
	    // This relies on our fragmenter's source being a "H264or5VideoStreamFramer".
	    if (((MediaVideoFragmenter*)fOurFragmenter)->lastFragmentCompletedNALUnit()
		&& framerSource != NULL && framerSource->pictureEndMarker()) {
	      setMarkerBit();
//	      framerSource->pictureEndMarker() = False;
	    }
	  }

	  setTimestamp(framePresentationTime);
}

Boolean MediaVideoRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* frameStart,
				 unsigned numBytesInFrame) const {
	  return False;
}
//end of
