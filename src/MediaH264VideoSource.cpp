#include "MediaH264VideoSource.h"

#define REV_BUF_SIZE  (1024*1024) 
#define FRAME_PER_SEC 25 

MediaH264VideoSource::MediaH264VideoSource(UsageEnvironment & env) 
	: FramedSource(env)
	, m_pToken(0)
	, m_pFrameBuffer(0)
{
	m_pFrameBuffer = new char[REV_BUF_SIZE];
	if (m_pFrameBuffer == NULL)
	{
		//printf("[MEDIA SERVER] error malloc data buffer failed\n");
		return;
	}
	memset(m_pFrameBuffer, 0, REV_BUF_SIZE);
}

MediaH264VideoSource::~MediaH264VideoSource(void)
{
	envir().taskScheduler().unscheduleDelayedTask(m_pToken);

	if (m_pFrameBuffer)
	{
		delete[] m_pFrameBuffer;
		m_pFrameBuffer = NULL;
	}

	//printf("[MEDIA SERVER] rtsp connection closed\n");
}

void MediaH264VideoSource::doGetNextFrame()
{
	// Calculate wait time based on fps
	double delay = 1000.0 / (FRAME_PER_SEC * 2);  // ms  
	int to_delay = delay * 1000;  // us  

	m_pToken = envir().taskScheduler().scheduleDelayedTask(to_delay, getNextFrame, this);
}

unsigned int MediaH264VideoSource::maxFrameSize() const
{
	return 1024 * 200;
}

void MediaH264VideoSource::getNextFrame(void * ptr)
{
	((MediaH264VideoSource *)ptr)->GetFrameData();
}

void MediaH264VideoSource::GetFrameData()
{
	gettimeofday(&fPresentationTime, 0);

	fFrameSize = 0;

	int len = 0;

	// fill frame data  
	memcpy(fTo, m_pFrameBuffer, fFrameSize);

	if (fFrameSize > fMaxSize)
	{
		fNumTruncatedBytes = fFrameSize - fMaxSize;
		fFrameSize = fMaxSize;
	}
	else
	{
		fNumTruncatedBytes = 0;
	}

	afterGetting(this);
}