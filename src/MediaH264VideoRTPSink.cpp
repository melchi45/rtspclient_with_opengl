#include "MediaH264VideoRTPSink.h"

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 2764800

MediaH264VideoRTPSink* MediaH264VideoRTPSink::createNew(UsageEnvironment& env, MediaSubsession* subsession,
	u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize,
	char const* streamId) {
	return new MediaH264VideoRTPSink(env, subsession, sps, spsSize, pps, ppsSize, streamId);
}

MediaH264VideoRTPSink::MediaH264VideoRTPSink(UsageEnvironment& env, MediaSubsession* subsession,
	u_int8_t const* sps, unsigned spsSize, u_int8_t const* pps, unsigned ppsSize,
	char const* streamId)
	: H264VideoRTPSink(env, subsession->rtpSource()->RTPgs(), subsession->rtpPayloadFormat(),
		sps, spsSize, pps, ppsSize)
	, fSubsession(subsession)
{
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
	memset(fReceiveBuffer, 0x0, DUMMY_SINK_RECEIVE_BUFFER_SIZE);
}

MediaH264VideoRTPSink::~MediaH264VideoRTPSink()
{
	delete[] fReceiveBuffer;
	delete[] fStreamId;
}

void MediaH264VideoRTPSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds) {
	MediaH264VideoRTPSink* sink = (MediaH264VideoRTPSink*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
#define DEBUG_PRINT_NPT 1

void MediaH264VideoRTPSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
	// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
	if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
	envir() << fSubsession->mediumName() << "/" << fSubsession->codecName() << ":\tReceived " << frameSize << " bytes";
	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
	sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
	envir() << ".\tPresentation time: " << (unsigned)presentationTime.tv_sec << "." << uSecsStr;
	if (fSubsession->rtpSource() != NULL && !fSubsession->rtpSource()->hasBeenSynchronizedUsingRTCP()) {
		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
	}
#ifdef DEBUG_PRINT_NPT
	envir() << "\tNPT: " << fSubsession->getNormalPlayTime(presentationTime);
#endif
	envir() << "\n";
#endif

	// Then continue, to request the next frame of data:
	continuePlaying();
}

Boolean MediaH264VideoRTPSink::continuePlaying() {
	if (fSource == NULL) return False; // sanity check (should not happen)

									   // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
		afterGettingFrame, this,
		onSourceClosure, this);

	return True;
}

void MediaH264VideoRTPSink::doSpecialFrameHandling(unsigned /*fragmentationOffset*/,
	unsigned char* /*frameStart*/,
	unsigned /*numBytesInFrame*/,
	struct timeval framePresentationTime,
	unsigned /*numRemainingBytes*/) {
	// Set the RTP 'M' (marker) bit iff
	// 1/ The most recently delivered fragment was the end of (or the only fragment of) an NAL unit, and
	// 2/ This NAL unit was the last NAL unit of an 'access unit' (i.e. video frame).
	if (fOurFragmenter != NULL) {
		H264or5VideoStreamFramer* framerSource
			= (H264or5VideoStreamFramer*)(fOurFragmenter->inputSource());

		// This relies on our fragmenter's source being a "H264or5VideoStreamFramer".
//		if (((H264or5Fragmenter*)fOurFragmenter)->lastFragmentCompletedNALUnit()
//			&& framerSource != NULL && framerSource->pictureEndMarker()) {
//			setMarkerBit();
//			framerSource->pictureEndMarker() = False;
//		}
	}

	setTimestamp(framePresentationTime);
}