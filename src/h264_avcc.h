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
#ifndef _H264_AVCC_H
#define _H264_AVCC_H        1

#include <stdint.h>
#include <assert.h>

#include "bs.h"
#include "h264_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   AVC decoder configuration record, ISO/IEC 14496-15:2004(E), Section 5.2.4.1
   Seen in seen in mp4 files as 'avcC' atom 
   Seen in flv files as AVCVIDEOPACKET with AVCPacketType == 0
*/
typedef struct
{
  int configurationVersion; // = 1
  int AVCProfileIndication;
  int profile_compatibility;
  int AVCLevelIndication;
  // bit(6) reserved = '111111'b;
  int lengthSizeMinusOne;
  // bit(3) reserved = '111'b;
  int numOfSequenceParameterSets;
  sps_t** sps_table;
  int numOfPictureParameterSets;
  pps_t** pps_table;
} avcc_t;

avcc_t* avcc_new();
void avcc_free(avcc_t* avcc);
int read_avcc(avcc_t* avcc, h264_stream_t* h, bs_t* b);
int write_avcc(avcc_t* avcc, h264_stream_t* h, bs_t* b);
void debug_avcc(avcc_t* avcc);

#ifdef __cplusplus
}
#endif

#endif
