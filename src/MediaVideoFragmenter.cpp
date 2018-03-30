/*******************************************************************************
 *  Copyright (c) 2013 Samsung Techwin Co., Ltd.
 *  
 *  Licensed to the Samsung Techwin Software Foundation under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Samsung SmartCam - SmartCam Camera Proejct
 *  http://www.samsungsmartcam.com
 *  
 *  Security Solution Division / B2C Camera S/W Development Team
 *
 *  $Author$
 *  $Date$
 *  $Revision$
 *  $Id$
 *  $HeadURL$
 *
 *  History 
 *    AmbaVideoFragmenter.cpp
 *
 *  Created on: Oct 21, 2016
 *      Author: Youngho Kim
 *******************************************************************************/

#include "MediaVideoFragmenter.h"

MediaVideoFragmenter::MediaVideoFragmenter(int hNumber,
				     UsageEnvironment& env, FramedSource* inputSource,
				     unsigned inputBufferMax, unsigned maxOutputPacketSize)
  : FramedFilter(env, inputSource),
    fHNumber(hNumber),
    fInputBufferSize(inputBufferMax+1), fMaxOutputPacketSize(maxOutputPacketSize) {
  fInputBuffer = new unsigned char[fInputBufferSize];
  reset();
}

MediaVideoFragmenter::~MediaVideoFragmenter() {
  delete[] fInputBuffer;
  detachInputSource(); // so that the subsequent ~FramedFilter() doesn't delete it
}

void MediaVideoFragmenter::doGetNextFrame() {
  if (fNumValidDataBytes == 1) {
    // We have no NAL unit data currently in the buffer.  Read a new one:
    fInputSource->getNextFrame(&fInputBuffer[1], fInputBufferSize - 1,
			       afterGettingFrame, this,
			       FramedSource::handleClosure, this);
  } else {
    // We have NAL unit data in the buffer.  There are three cases to consider:
    // 1. There is a new NAL unit in the buffer, and it's small enough to deliver
    //    to the RTP sink (as is).
    // 2. There is a new NAL unit in the buffer, but it's too large to deliver to
    //    the RTP sink in its entirety.  Deliver the first fragment of this data,
    //    as a FU packet, with one extra preceding header byte (for the "FU header").
    // 3. There is a NAL unit in the buffer, and we've already delivered some
    //    fragment(s) of this.  Deliver the next fragment of this data,
    //    as a FU packet, with two (H.264) or three (H.265) extra preceding header bytes
    //    (for the "NAL header" and the "FU header").

    if (fMaxSize < fMaxOutputPacketSize) { // shouldn't happen
      envir() << "H264or5Fragmenter::doGetNextFrame(): fMaxSize ("
	      << fMaxSize << ") is smaller than expected\n";
    } else {
      fMaxSize = fMaxOutputPacketSize;
    }

    fLastFragmentCompletedNALUnit = True; // by default
    if (fCurDataOffset == 1) { // case 1 or 2
      if (fNumValidDataBytes - 1 <= fMaxSize) { // case 1
	memmove(fTo, &fInputBuffer[1], fNumValidDataBytes - 1);
	fFrameSize = fNumValidDataBytes - 1;
	fCurDataOffset = fNumValidDataBytes;
      } else { // case 2
	// We need to send the NAL unit data as FU packets.  Deliver the first
	// packet now.  Note that we add "NAL header" and "FU header" bytes to the front
	// of the packet (overwriting the existing "NAL header").
	if (fHNumber == 264) {
	  fInputBuffer[0] = (fInputBuffer[1] & 0xE0) | 28; // FU indicator
	  fInputBuffer[1] = 0x80 | (fInputBuffer[1] & 0x1F); // FU header (with S bit)
	} else { // 265
	  u_int8_t nal_unit_type = (fInputBuffer[1]&0x7E)>>1;
	  fInputBuffer[0] = (fInputBuffer[1] & 0x81) | (49<<1); // Payload header (1st byte)
	  fInputBuffer[1] = fInputBuffer[2]; // Payload header (2nd byte)
	  fInputBuffer[2] = 0x80 | nal_unit_type; // FU header (with S bit)
	}
	memmove(fTo, fInputBuffer, fMaxSize);
	fFrameSize = fMaxSize;
	fCurDataOffset += fMaxSize - 1;
	fLastFragmentCompletedNALUnit = False;
      }
    } else { // case 3
      // We are sending this NAL unit data as FU packets.  We've already sent the
      // first packet (fragment).  Now, send the next fragment.  Note that we add
      // "NAL header" and "FU header" bytes to the front.  (We reuse these bytes that
      // we already sent for the first fragment, but clear the S bit, and add the E
      // bit if this is the last fragment.)
      unsigned numExtraHeaderBytes;
      if (fHNumber == 264) {
	fInputBuffer[fCurDataOffset-2] = fInputBuffer[0]; // FU indicator
	fInputBuffer[fCurDataOffset-1] = fInputBuffer[1]&~0x80; // FU header (no S bit)
	numExtraHeaderBytes = 2;
      } else { // 265
	fInputBuffer[fCurDataOffset-3] = fInputBuffer[0]; // Payload header (1st byte)
	fInputBuffer[fCurDataOffset-2] = fInputBuffer[1]; // Payload header (2nd byte)
	fInputBuffer[fCurDataOffset-1] = fInputBuffer[2]&~0x80; // FU header (no S bit)
	numExtraHeaderBytes = 3;
      }
      unsigned numBytesToSend = numExtraHeaderBytes + (fNumValidDataBytes - fCurDataOffset);
      if (numBytesToSend > fMaxSize) {
	// We can't send all of the remaining data this time:
	numBytesToSend = fMaxSize;
	fLastFragmentCompletedNALUnit = False;
      } else {
	// This is the last fragment:
	fInputBuffer[fCurDataOffset-1] |= 0x40; // set the E bit in the FU header
	fNumTruncatedBytes = fSaveNumTruncatedBytes;
      }
      memmove(fTo, &fInputBuffer[fCurDataOffset-numExtraHeaderBytes], numBytesToSend);
      fFrameSize = numBytesToSend;
      fCurDataOffset += numBytesToSend - numExtraHeaderBytes;
    }

    if (fCurDataOffset >= fNumValidDataBytes) {
      // We're done with this data.  Reset the pointers for receiving new data:
      fNumValidDataBytes = fCurDataOffset = 1;
    }

    // Complete delivery to the client:
    FramedSource::afterGetting(this);
  }
}

void MediaVideoFragmenter::doStopGettingFrames() {
  // Make sure that we don't have any stale data fragments lying around, should we later resume:
  reset();
  FramedFilter::doStopGettingFrames();
}

void MediaVideoFragmenter::afterGettingFrame(void* clientData, unsigned frameSize,
					  unsigned numTruncatedBytes,
					  struct timeval presentationTime,
					  unsigned durationInMicroseconds) {
	MediaVideoFragmenter* fragmenter = (MediaVideoFragmenter*)clientData;
  fragmenter->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime,
				 durationInMicroseconds);
}

void MediaVideoFragmenter::afterGettingFrame1(unsigned frameSize,
					   unsigned numTruncatedBytes,
					   struct timeval presentationTime,
					   unsigned durationInMicroseconds) {
  fNumValidDataBytes += frameSize;
  fSaveNumTruncatedBytes = numTruncatedBytes;
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;

  // Deliver data to the client:
  doGetNextFrame();
}

void MediaVideoFragmenter::reset() {
  fNumValidDataBytes = fCurDataOffset = 1;
  fSaveNumTruncatedBytes = 0;
  fLastFragmentCompletedNALUnit = True;
}

//end of 
