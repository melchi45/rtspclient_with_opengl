#include "MediaH264MediaSink.h"

#define	MAX_FRAMING_SIZE				4
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE	2764800

MediaH264MediaSink* MediaH264MediaSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new MediaH264MediaSink(env, subsession, streamId);
}

MediaH264MediaSink::MediaH264MediaSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env)
	, fSubsession(subsession)
	, video_framing(0)
{
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[MAX_FRAMING_SIZE+DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  memset(fReceiveBuffer, 0, MAX_FRAMING_SIZE+DUMMY_SINK_RECEIVE_BUFFER_SIZE);

  // setup framing if necessary
  // H264 framing code
  if (strcmp("H264", fSubsession.codecName()) == 0 || 
	  strcmp("H265", fSubsession.codecName()) == 0) {
	  video_framing = 4;
	  fReceiveBuffer[MAX_FRAMING_SIZE - video_framing + 0]
		  = fReceiveBuffer[MAX_FRAMING_SIZE - video_framing + 1]
		  = fReceiveBuffer[MAX_FRAMING_SIZE - video_framing + 2] = 0;
	  fReceiveBuffer[MAX_FRAMING_SIZE - video_framing + 3] = 1;
  }
}

MediaH264MediaSink::~MediaH264MediaSink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void MediaH264MediaSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
	MediaH264MediaSink* sink = (MediaH264MediaSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
#define DEBUG_PRINT_NPT 1

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

  bool marker = false;
  int lost = 0, count = 1;

  RTPSource *rtpsrc = fSubsession.rtpSource();
  RTPReceptionStatsDB::Iterator iter(rtpsrc->receptionStatsDB());
  RTPReceptionStats* stats = iter.next(True);

  if (rtpsrc != NULL) {
	  marker = rtpsrc->curPacketMarkerBit();
  }
  
  static FILE *fp = fopen("saved.h264", "wb");
  
  fwrite(fReceiveBuffer, 1, frameSize + MAX_FRAMING_SIZE, fp);
  fflush(fp);
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  envir() << "\tsaved " << (unsigned)frameSize + MAX_FRAMING_SIZE << " bytes";
  envir() << "\n";
#endif

  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean MediaH264MediaSink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer + MAX_FRAMING_SIZE,
	  DUMMY_SINK_RECEIVE_BUFFER_SIZE,
      afterGettingFrame, this,
      onSourceClosure, this);

  return True;
}