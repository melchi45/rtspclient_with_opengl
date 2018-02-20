#include "DummySink.h"

#if defined(WIN32)
#include "utils.h"
#endif

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 2764800

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];

  buff = new uint8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  sPropRecords = NULL;
  m_nIndex = -1;
}

DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
#define DEBUG_PRINT_NPT 1

int Width = 0;
int Height = 0;
unsigned numSPropRecords;

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
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

  static unsigned char const start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

  if (sPropRecords == NULL)
  {
	  sPropRecords = parseSPropParameterSets(sps.c_str(), numSPropRecords);
	  char* filename = "test.264";
	  FILE* pfSPS = fopen(filename, "wb");
	  envir() << sps.c_str() << "!\r\n";

	  for (unsigned i = 0; i < numSPropRecords; ++i)
	  {
		  if (pfSPS)
		  {
			  fwrite(start_code, 1, 4, pfSPS);
			  fwrite(sPropRecords[i].sPropBytes, 1, sPropRecords[i].sPropLength, pfSPS);
			  //CBuffMgr::Instance().putBuffer(m_nIndex, sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength, 0);
		  }
	  }
	  fclose(pfSPS);

	  gettimeofday(&start_time, NULL);

	  //CXAgent::Instance().SetSPS(filename);
  }

  memset(buff, '0', DUMMY_SINK_RECEIVE_BUFFER_SIZE);
  memcpy(buff, start_code, 4);
  memcpy(buff + 4, fReceiveBuffer, frameSize);


  // presentationTime.tv_sec (unsigned)presentationTime.tv_usec
  struct timeval cur;
  gettimeofday(&cur, NULL);
  int timestamp = (cur.tv_sec - start_time.tv_sec) * 1000 + (cur.tv_usec - start_time.tv_usec) / 1000;


  //CXAgent::Instance().SetData(buff, frameSize+4, timestamp);

  //check width and height

  /* bool ret = h264_decode_seq_parameter_set(fReceiveBuffer, frameSize, Width, Height);
  if (ret)
  {
  }*/


  if (m_nIndex >= 0)
  {
//	  pjcall_thread_register();
//	  if (rtspAPI::Instance().m_rtsp[m_nIndex]->m_pCB)
//	  {
//		  rtspAPI::Instance().m_rtsp[m_nIndex]->m_pCB->rtspDataCB(buff, frameSize + 4);
//		  int result = rtspAPI::Instance().m_rtsp[m_nIndex]->m_pDecode->decode_rtsp_frame(buff, frameSize + 4, false);
//	  }
  }

  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);

  return True;
}
