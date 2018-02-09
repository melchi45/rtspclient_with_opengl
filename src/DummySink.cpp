// Implementation of "DummySink":
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "DummySink.h"

//////////////////////////////////////////////////////////////////////////
// my variable
extern std::vector<std::string> data;
extern std::map<std::string, int> inds;
extern int nowind;
extern std::string nowstr;

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
	return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env),
	fSubsession(subsession) {
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];

	//////////////////////////////////////////////////////////////////////////
	// my dcde
	fReceiveBufferadd4 = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE + 4];
	fReceiveBufferadd4[0] = 0;
	fReceiveBufferadd4[1] = 0;
	fReceiveBufferadd4[2] = 0;
	fReceiveBufferadd4[3] = 1;

	// my code
	//////////////////////////////////////////////////////////////////////////

}

DummySink::~DummySink() {
	delete[] fReceiveBuffer;
	delete[] fStreamId;

	delete[] fReceiveBufferadd4;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds) {
	DummySink* sink = (DummySink*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
// #define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

//////////////////////////////////////////////////////////////////////////
// my code
void DummySink::setSprop(u_int8_t const* prop, unsigned size)
{
	u_int8_t *buf;
	u_int8_t *buf_start;
	buf = new u_int8_t[1000];
	buf_start = buf + 4;

	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 1;
	memcpy(buf_start, prop, size);

	std::stringstream stream;
	for (int i = 0; i< size + 4; i++)
	{
		stream << buf[i];
	}

	nowstr = stream.str();
	data[nowind] = data[nowind] + nowstr;

	delete[] buf;

	// envir() << "after setSprop\n";
}
// my code end
//////////////////////////////////////////////////////////////////////////

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
	// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
	if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
	envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
	sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
	envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
	if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
	}
#ifdef DEBUG_PRINT_NPT
	envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
	envir() << "\n";
#endif

	//////////////////////////////////////////////////////////////////////////
	// my code
	if (!strcmp("video", fSubsession.mediumName()) &&
		!strcmp("H264", fSubsession.codecName()))
	{
		if (frameSize + 4 != 0)
		{
			memcpy(fReceiveBufferadd4 + 4, fReceiveBuffer, frameSize);

			std::stringstream stream;
			for (int i = 0; i< frameSize + 4; i++)
			{
				stream << fReceiveBufferadd4[i];
			}

			char name[256];
			sprintf(name, "%s", fStreamId);
			int strl = strlen(name);
			name[strl - 1] = '\0';
			nowind = inds[name];

			nowstr = stream.str();
			data[nowind] = data[nowind] + nowstr;
		}

		int height = fSubsession.videoHeight();
		int width = fSubsession.videoWidth();
	}
	// ,y code end
	//////////////////////////////////////////////////////////////////////////

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