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
#ifndef _H264_DECODER_H_
#define _H264_DECODER_H_

#include "FFmpeg.h"

class H264Decoder : public FFMpeg
{
public:
#if USE_LIVE555
	static H264Decoder *
		createNew(UsageEnvironment &env);
	H264Decoder(UsageEnvironment &env);
#else
	H264Decoder();
#endif
	virtual ~H264Decoder();
	
	virtual int intialize();
	virtual int finalize();

//	int openDecoder(int width, int height, CDecodeCB* pCB);
	virtual int decode(uint8_t* input, int nLen, bool bWaitIFrame = false);
};
#endif /* _H264_DECODER_H_ */