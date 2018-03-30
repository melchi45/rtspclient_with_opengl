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
#ifndef _MEDIA_VIDEO_FRAGMENTER_H_
#define _MEDIA_VIDEO_FRAGMENTER_H_

#include <FramedFilter.hh>

/*
 *
 */
class MediaVideoFragmenter : public FramedFilter {
public:
	MediaVideoFragmenter(int hNumber, UsageEnvironment& env, FramedSource* inputSource,
		    unsigned inputBufferMax, unsigned maxOutputPacketSize);
  virtual ~MediaVideoFragmenter();

  Boolean lastFragmentCompletedNALUnit() const { return fLastFragmentCompletedNALUnit; }

private: // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);
  void reset();

private:
  int fHNumber;
  unsigned fInputBufferSize;
  unsigned fMaxOutputPacketSize;
  unsigned char* fInputBuffer;
  unsigned fNumValidDataBytes;
  unsigned fCurDataOffset;
  unsigned fSaveNumTruncatedBytes;
  Boolean fLastFragmentCompletedNALUnit;
};

//end of 

#endif /* _MEDIA_VIDEO_FRAGMENTER_H_ */
