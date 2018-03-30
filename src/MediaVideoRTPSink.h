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
#ifndef _MEDIA_VIDEO_RTPSINK_H_
#define _MEDIA_VIDEO_RTPSINK_H_

#include "liveMedia.hh"

/*
 *
 */
class MediaVideoRTPSink : public H264or5VideoRTPSink {
public:
  static MediaVideoRTPSink*
  createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat);
  static MediaVideoRTPSink*
  createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat,
	    u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize);
    // an optional variant of "createNew()", useful if we know, in advance,
    // the stream's SPS and PPS NAL units.
    // This avoids us having to 'pre-read' from the input source in order to get these values.
  static MediaVideoRTPSink*
  createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat,
	    char const* sPropParameterSetsStr);
    // an optional variant of "createNew()", useful if we know, in advance,
    // the stream's SPS and PPS NAL units.
    // This avoids us having to 'pre-read' from the input source in order to get these values.

protected:
	MediaVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat,
		   u_int8_t const* sps = NULL, unsigned spsSize = 0,
		   u_int8_t const* pps = NULL, unsigned ppsSize = 0);
	// called only by createNew()
  virtual ~MediaVideoRTPSink();

protected: // redefined virtual functions:
  virtual char const* auxSDPLine();

private: // redefined virtual functions:
  virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);
  virtual Boolean continuePlaying();
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);
  virtual Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
						 unsigned numBytesInFrame) const;
};

//end of 

#endif /* _MEDIA_VIDEO_RTPSINK_H_ */
