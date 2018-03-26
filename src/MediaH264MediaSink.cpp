#include "MediaH264MediaSink.h"
#if 0
#include "FFMpegDecoder.h"
#else
#include "H264Decoder.h"
#endif
#include "h264_stream.h"

//#define ENABLE_NAL_PARSER					1
//#define FILE_SAVE							1
#define	MAX_FRAMING_SIZE					4
#define H264_WITH_START_CODE				1
//#define H264_NAL_HEADER_STARTCODE_LENGTH	4
//#define DUMMY_SINK_RECEIVE_BUFFER_SIZE	2764800
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE		1000000

typedef enum nal_type {
	NALTYPE_Unspecified = 0,
	NALTYPE_SliceLayerWithoutPartitioning = 1,
	NALTYPE_SliceDataPartitionALayer = 2,
	NALTYPE_SliceDataPartitionBLayer = 3,
	NALTYPE_SliceDataPartitionCLayer = 4,
	NALTYPE_IDRPicture = 5,
	NALTYPE_SEI = 6,
	NALTYPE_SequenceParameterSet = 7,	// SPS
	NALTYPE_PictureParameterSet = 8,	// PPS
	NALTYPE_AccessUnitDelimiter = 9,
	NALTYPE_EndofSequence = 10,
	NALTYPE_EndofStream = 11,
	NALTYPE_FilterData = 12,
	NALTYPE_Extended = 13,
	//	NALTYPE_Unspecified = 24,
} NALTYPE;

MediaH264MediaSink* MediaH264MediaSink::createNew(UsageEnvironment& env, RTSPClient* client, MediaSubsession& subsession, char const* streamId) {
  return new MediaH264MediaSink(env, client, subsession, streamId);
}

MediaH264MediaSink::MediaH264MediaSink(UsageEnvironment& env, RTSPClient* client, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env)
	, fSubsession(subsession)
	, m_nFrameSize(0)
	, m_nNalHeaderStartCodeOffset(0)
	, m_nFrameCount(0)
	, pClient(client)
{
  fStreamId = strDup(streamId);
#if H264_WITH_START_CODE
  fReceiveBuffer = new u_int8_t[MAX_FRAMING_SIZE + DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  memset(fReceiveBuffer, 0, MAX_FRAMING_SIZE + DUMMY_SINK_RECEIVE_BUFFER_SIZE);

  // setup framing if necessary
  // H264 framing code
  if (strcmp("H264", fSubsession.codecName()) == 0 ||
	  strcmp("H265", fSubsession.codecName()) == 0) {
	  fReceiveBuffer[0]
		  = fReceiveBuffer[1]
		  = fReceiveBuffer[2] = 0;
	  fReceiveBuffer[3] = 1;
  }
#else
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  memset(fReceiveBuffer, 0, DUMMY_SINK_RECEIVE_BUFFER_SIZE);
#endif
#if 0
  video_decoder = new FFmpegDecoder(env);
#else
  video_decoder = new H264Decoder();
#endif
  if (video_decoder == NULL)
  {
	  env << "Failed to create a FFMpeg Decoder\n";
  }
#if 0
 if (video_decoder != NULL) {
	  video_decoder->intialize();

	  if (pClient != NULL) {
		  video_decoder->setClient(pClient);
	  }
	  //decoder->openDecoder(scs.subsession->videoWidth(), scs.subsession->videoHeight(), ((MediaRTSPClient*)rtspClient)->getRTSPSession());
  }
#else
  if (video_decoder != NULL) {
	  //video_decoder->setSize(320, 240); //set decode size
	  //video_decoder->setSize(640, 480); //set decode size
	  //video_decoder->setSize(1080, 720); //set decode size
	  //video_decoder->setSize(1080, 720); //set decode size
	  //video_decoder->setSize(1920, 1080); //set decode size
	 // video_decoder->setSize(3840, 2160); //set decode size
  }
#endif

}

MediaH264MediaSink::~MediaH264MediaSink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;

  if (video_decoder != NULL) {
	  video_decoder->finalize();
	  delete video_decoder;
  }
  video_decoder = NULL;
}

void MediaH264MediaSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
	MediaH264MediaSink* sink = (MediaH264MediaSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
#define DEBUG_PRINT_NPT 1

char tempstr[1000] = { 0 };
char outputstr[100000] = { '\0' };

void MediaH264MediaSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";

  if (numTruncatedBytes > 0) {
	  envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  }

  // calculate microseconds
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);

  envir() << ".\tPresentation time: " << (unsigned)presentationTime.tv_sec << "." << uSecsStr;

  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif

  RTPSource *rtpsrc = fSubsession.rtpSource();
  RTPReceptionStatsDB::Iterator iter(rtpsrc->receptionStatsDB());
  RTPReceptionStats* stats = iter.next(True);
  FramedSource* source = this->source();

  // We've just received a frame of data.  (Optionally) print out information about it:
//  int nNALType = fReceiveBuffer[H264_NAL_HEADER_STARTCODE_LENGTH] & 0x1F;

// if (isH264iFrame(fReceiveBuffer)) {
//	  envir() << "I-Frame\n";
//  }

  int start_code_length = 0;
  // check 3 byte start code
  if (FindStartCode3(fReceiveBuffer)) {
	  start_code_length = 3;
  }

  if (FindStartCode4(fReceiveBuffer)) {
	  start_code_length = 4;
  }
#ifdef ENABLE_NAL_PARSER
  h264_stream_t* h = h264_new();
  read_nal_unit(h, &fReceiveBuffer[start_code_length], m_nFrameSize - start_code_length);
  debug_nal(h, h->nal);
  //envir() << outputstr << "\n";
#endif
 
  // https://www.ietf.org/rfc/rfc3984.txt
  // http://egloos.zum.com/yajino/v/782492
  // http://gentlelogic.blogspot.kr/2011/11/exploring-h264-part-2-h264-bitstream.html
  // https://yumichan.net/video-processing/video-compression/introduction-to-h264-nal-unit/
  // https://stackoverflow.com/questions/1957427/detect-mpeg4-h264-i-frame-idr-in-rtp-stream
  // http://lists.live555.com/pipermail/live-devel/2015-September/019660.html
  //if (fSubsession.rtpSource()->curPacketMarkerBit())
  if (true)
  {
	  m_nFrameSize += frameSize;
	  int nNALType = fReceiveBuffer[start_code_length] & 0x1F;

	  bool bNalHeaderPresent = false;
	  if ((nNALType == 0) && (m_nFrameSize >= start_code_length))
	  {      // Most of the cameras sending the frames without nal header start code, however some cameras  send the frame with start code
			 // Since we initialized the buffer with start code, we need to skip that. 
			 // We are reinitilzing without nal header start code, we will execute this code only for the first frame
		  nNALType = fReceiveBuffer[start_code_length] & 0x1F;

		  // First make sure we have valid NAL Type
		  if (nNALType == NALTYPE::NALTYPE_IDRPicture || nNALType == NALTYPE::NALTYPE_SEI || nNALType == NALTYPE::NALTYPE_SequenceParameterSet
			  || nNALType == NALTYPE::NALTYPE_PictureParameterSet || nNALType == NALTYPE::NALTYPE_AccessUnitDelimiter || nNALType == NALTYPE::NALTYPE_SliceLayerWithoutPartitioning)
		  {
			  memmove(fReceiveBuffer, fReceiveBuffer + start_code_length, m_nFrameSize + start_code_length);
			  bNalHeaderPresent = true;
		  }
	  }

	  if (nNALType == NALTYPE::NALTYPE_IDRPicture)
	  {
		  ++m_nFrameCount;
		  envir() << "I Frame, Frame Size: " << m_nFrameSize
			  << ", Frame Count: " << (int)m_nFrameCount << "\n";

		  if (video_decoder != NULL) {
//			  video_decoder->decode_rtsp_frame(&fReceiveBuffer[start_code_length], m_nFrameSize - start_code_length);
//			  video_decoder->decode_rtsp_frame(fReceiveBuffer, m_nFrameSize);
		  }
		  
//		  envir() << "I Frame      (" << m_nFrameWidth << "x" << m_nFrameHeight << ")         " << m_nFrameSize + frameSize << "\n";
//		  m_nFrameCounter = 0;

//		  if (m_nConfigLength)
//			  m_pCamera->Add2FrameQueue(CC_SAMPLETYPE_MPEG4AVC_CONFIG, m_pConfig, m_nConfigLength, m_nFrameWidth, m_nFrameHeight, m_nFrameCounter);
//		  m_pCamera->Add2FrameQueue(CC_SAMPLETYPE_MPEG4AVC_IFRAME, fReceiveBuffer, m_nFrameSize, m_nFrameWidth, m_nFrameHeight, m_nFrameCounter);
//		  DisplayDiagnosticsInfo(m_nFrameSize);
//		  CalculateFrameRate();
//		  CalculateIFrameInterval(true);
	  }
	  else if (nNALType == NALTYPE::NALTYPE_SEI)
	  {
		  envir() << "SEI        " << m_nFrameSize << "\n";
	  }
	  else if (nNALType == NALTYPE::NALTYPE_SequenceParameterSet)
	  {
		  // Some Cameras send I frame with Config Data , process those cases
		  unsigned long nEndOfConfigData = m_nFrameSize + 1;
		  unsigned long nBytesToCheck = m_nFrameSize - 5;
		  u_int8_t nNALTypeSPS;
		  const unsigned char* pData = (const unsigned char*)fReceiveBuffer;
//		  int width = ((h->sps->pic_height_in_map_units_minus1 + 1) * 16) + h->sps->frame_crop_bottom_offset * 2 - h->sps->frame_crop_top_offset * 2;
//		  int Height = ((2 - h->sps->frame_mbs_only_flag)* (h->sps->pic_height_in_map_units_minus1 + 1) * 16) - (h->sps->frame_crop_bottom_offset * 2) - (h->sps->frame_crop_top_offset * 2);

		  // Special Case 1
		  for (unsigned long n = 0; n < nBytesToCheck; ++n)
		  {
			  if (pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x01)
			  {
				  nNALTypeSPS = pData[4] & 0x1F;

				  if ((nNALTypeSPS == NALTYPE_IDRPicture) || (nNALTypeSPS == NALTYPE_SliceLayerWithoutPartitioning))
				  {      // Frame Data is started
					  nEndOfConfigData = n;
					  break;
				  }
			  }

			  ++pData;
		  }

		  if (nEndOfConfigData < m_nFrameSize)
		  {
//			  m_nFrameCounter = 0;

//			  if (m_nConfigLength)
//				  m_pCamera->Add2FrameQueue(CC_SAMPLETYPE_MPEG4AVC_CONFIG, fReceiveBuffer, nEndOfConfigData, m_nFrameWidth, m_nFrameHeight, m_nFrameCounter);
//			  m_pCamera->Add2FrameQueue(CC_SAMPLETYPE_MPEG4AVC_IFRAME, fReceiveBuffer + nEndOfConfigData, m_nFrameSize - nEndOfConfigData, m_nFrameWidth, m_nFrameHeight, m_nFrameCounter);
//			  DisplayDiagnosticsInfo(m_nFrameSize);
//			  CalculateFrameRate();
//			  CalculateIFrameInterval(true);
		  }
		  else
		  {      // This is normal case, most of the cameras
//			  if (m_pConfig == NULL && m_nFrameSize)
//			  {
//				  m_nConfigLength = m_nFrameSize;
//				  m_pConfig = new uint8_t[m_nConfigLength];
//				  memcpy(m_pConfig, fReceiveBuffer, m_nConfigLength);
//			  }
		  }
		  // Diag

		  envir() << "SPS        " << m_nFrameSize << "\n";
	  }
	  else if (nNALType == NALTYPE::NALTYPE_PictureParameterSet)
	  {
		  envir() << "PPS        " << m_nFrameSize << "\n";
	  }
	  else if (nNALType == NALTYPE::NALTYPE_AccessUnitDelimiter)
	  {
		  envir() << "AUD        " << m_nFrameSize << "\n";
	  }
	  else if (nNALType == NALTYPE::NALTYPE_SliceLayerWithoutPartitioning)
	  {
		  ++m_nFrameCount;
		  envir() << "P Frame, Frame Size: " << m_nFrameSize << ", Frame Count: " << (int)m_nFrameCount << "\n";
		  if (video_decoder != NULL) {
//			  video_decoder->decode_rtsp_frame(&fReceiveBuffer[start_code_length], m_nFrameSize - start_code_length);
			  //video_decoder->decode_rtsp_frame(fReceiveBuffer, m_nFrameSize);
		  }

//		  m_nFrameCounter++;
//		  m_pCamera->Add2FrameQueue(CC_SAMPLETYPE_MPEG4AVC_PFRAME, fReceiveBuffer, m_nFrameSize, m_nFrameWidth, m_nFrameHeight, m_nFrameCounter);
//		  CalculateFrameRate();
//		  CalculateIFrameInterval(false);*/
	  }
	  else
	  {
		  envir() << "Unrecognizable DATA .............................................................    \n";
		  envir() << nNALType;
		  bNalHeaderPresent = false;
	  }

	  //envir() << m_nFrameSize << "\n";

	  if (bNalHeaderPresent)
	  {      // Reinitilize the offset, since this camera is sending nal header start code
		  m_nNalHeaderStartCodeOffset = 0;
	  }

	  m_nFrameSize = m_nNalHeaderStartCodeOffset;
  }
  else
  {
	  m_nFrameSize += frameSize;

	  // 08/19/2015 ODD Case, Major IP Camera Vendor's firmware sends SEI data without rtp market set, Needs to investigate further
	  int nNALType = fReceiveBuffer[start_code_length] & 0x1F;

	  if (nNALType == NALTYPE::NALTYPE_SEI)
	  {
		  // Ignore the packet, we don't need SEI motion detection data
		  m_nFrameSize = start_code_length;
		  envir() << "SEI without Marker       " << m_nFrameSize << "\n";
	  }

	  envir() << "Incomplete Data.......................    \n";
  }
#if 0
  video_decoder->decode_rtsp_frame(fReceiveBuffer, frameSize + start_code_length);
#else
  video_decoder->decode(fReceiveBuffer, frameSize + start_code_length);
#endif

#ifdef ENABLE_NAL_PARSER
  h264_free(h);
#endif

#ifdef FILE_SAVE
  static FILE *fp = fopen("saved.h264", "wb");
  
  fwrite(fReceiveBuffer, 1, frameSize + MAX_FRAMING_SIZE, fp);
  fflush(fp);
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  envir() << "\tsaved " << (unsigned)frameSize + MAX_FRAMING_SIZE << " bytes";
  envir() << "\n";
#endif
#endif
  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean MediaH264MediaSink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
#if H264_WITH_START_CODE
  fSource->getNextFrame(fReceiveBuffer + MAX_FRAMING_SIZE,
	  DUMMY_SINK_RECEIVE_BUFFER_SIZE + MAX_FRAMING_SIZE,
      afterGettingFrame, this,
      onSourceClosure, this);
#else
  fSource->getNextFrame(fReceiveBuffer, 
	  DUMMY_SINK_RECEIVE_BUFFER_SIZE,
	  afterGettingFrame, this,
	  onSourceClosure, this);
#endif
  return True;
}

bool MediaH264MediaSink::FindStartCode3(unsigned char *Buf)
{
	if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 1) return false; // check header 0x000001
	else return true;
}

bool MediaH264MediaSink::FindStartCode4(unsigned char *Buf)
{
	if (Buf[0] != 0 || Buf[1] != 0 || Buf[2] != 0 || Buf[3] != 1) return false;//check header 0x00000001
	else return true;
}

bool MediaH264MediaSink::isH264iFrame(u_int8_t* packet)
{
	//	assert(packet);
	int pos = 0;

	// check 3 byte start code
	if (FindStartCode3(packet)) {
		pos = 3;
	}

	if (FindStartCode3(packet)) {
		pos = 4;
	}

	int RTPHeaderBytes = 0;

	int fragment_type = packet[RTPHeaderBytes + pos + 0] & 0x1F;
	int nal_type = packet[RTPHeaderBytes + pos + 1] & 0x1F;			// 5 bit 
	int start_bit = packet[RTPHeaderBytes + pos + 1] & 0x80;		// 1 bit
	int reference_idc = packet[RTPHeaderBytes + pos + 1] & 0x60;	// 2 bit

	envir() << "fragment_type: " << fragment_type << ", nal_type: " << nal_type << ", start_bit: " << start_bit << "\n";

	if (((fragment_type == 28 || fragment_type == 29) && NALTYPE::NALTYPE_IDRPicture == nal_type && start_bit == 128) || fragment_type == 5)
	{
		return true;
	}

	return false;
}