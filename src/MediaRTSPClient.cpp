// Implementation of "ourRTSPClient":

#include "MediaRTSPClient.h"
#include "log_utils.h"

MediaRTSPClient* MediaRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum,
	const char* username, const char* password) {
	return new MediaRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum,
		username, password);
}

MediaRTSPClient::MediaRTSPClient(UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum,
	const char* username, const char* password)
	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) 
	, fVidRTPSink(NULL)
	, fAudRTPSink(NULL)
	, bUpTransport(false)
{
	if (username && password)
		ourAuthenticator = new Authenticator(username, password);
}

MediaRTSPClient::~MediaRTSPClient() {
	delete ourAuthenticator;
	delete sdpDescription;
}

char* MediaRTSPClient::getSDPDescription()
{
	char temp[2048] = "";

	strcat(temp, "v=0\n");
	strcat(temp, "o=- 0 0 IN IP4 null\n");
	strcat(temp, "s=Media Presentation\n");
	strcat(temp, "i=samsung\n");
	strcat(temp, "c=IN IP4 ::0\n");
	strcat(temp, "t=0 0\n");
	strcat(temp, "a=recvonly\n");

	strcat(temp, "m=video 40048 RTP/AVP 97\n");
	strcat(temp, "b=AS:2000\n");
	strcat(temp, "a=rtpmap:97 H264/90000\n");
	strcat(temp, "a=fmtp:97 packetization-mode=1;profile-level-id=4D001F;sprop-parameter-sets=Z00AH5pkAoAt/4C3AQEBQAAA+gAAHUw6GADa0AA2tC7y40MAG1oABtaF3lwo,aO48gA==\n");
	strcat(temp, "a=cliprect:0,0,720,1280\n");
	strcat(temp, "a=framesize:97 1280-720\n");
	strcat(temp, "a=framerate:15.0\n");
	strcat(temp, "a=control:trackID=1\n");

	strcat(temp, "m=audio 40052 RTP/AVP 0\n");
	strcat(temp, "a=rtpmap:0 PCMU/8000\n");
	strcat(temp, "a=control:trackID=2\n");


	sdpDescription = new char[2048];
	strcpy(this->sdpDescription, &temp[0]);

	return sdpDescription;
}


FramedSource* MediaRTSPClient::createNewVideoStreamSource()
{
	/*
	//    estBitrate = 2000; // kbps, estimate

	// Create the video source:
	AmbaVideoStreamSource   *pAmbaSource = NULL;

	fStreamTag = 0;

	pAmbaSource = (fStream_type == STREAM_TYPE_LIVE)
		? AmbaVideoStreamSource::createNew(envir(), fStream_reader, NULL, (u_int32_t)fStreamTag)
		: AmbaVideoStreamSource::createNew(envir(), fStream_reader, fFilename, (u_int32_t)fStreamTag);

	if (pAmbaSource == NULL)
		return NULL;

	fSourceCodec_ID = pAmbaSource->getSourceCodecID();

	// get sps and pps from amba stream
	if (pAmbaSource)
	{
		int     result = 0;

		log_debug("fSourceCodec_ID=0x%x\n", fSourceCodec_ID);

		if (fStream_type == STREAM_TYPE_PLAYBACK)
		{
			stream_reader_ctrl_box_t    ctrl_box;

			memset(&ctrl_box, 0x0, sizeof(stream_reader_ctrl_box_t));

			ctrl_box.cmd = STREAM_READER_CMD_PLAYBACK_GET_DURATION;
			ctrl_box.stream_tag = fStreamTag;
			ctrl_box.stream_type = fStream_type;
			StreamReader_Control(fStream_reader, NULL, &ctrl_box);

			fDuration = (float)ctrl_box.out.get_duration.duration / 1000.0;
		}

		switch (pAmbaSource->getSourceCodecID())
		{
		case AMP_FORMAT_MID_AVC:
		case AMP_FORMAT_MID_H264:
			result = pAmbaSource->getSpsPps(&fSPS_payload, &fSPS_length, &fPPS_payload, &fPPS_length);
			if (result)
			{
				log_debug("No SPS/PPS !\n");
			}
			return pAmbaSource;
			break;
		default:
			break;
		}
	}
	*/
	return NULL;
}

FramedSource* MediaRTSPClient::createNewAudioStreamSource()
{
	/*
	//    estBitrate = 128; // kbps, estimate
	fStreamTag = 0;

	// Create the audio source:
	AmbaAudioStreamSource   *pAmbaSource = NULL;

	pAmbaSource = (fStream_type == STREAM_TYPE_LIVE)
		? AmbaAudioStreamSource::createNew(envir(), fStream_reader, NULL, fStreamTag)
		: AmbaAudioStreamSource::createNew(envir(), fStream_reader, fFilename, fStreamTag);

	fSourceCodec_ID = pAmbaSource->getSourceCodecID();

	if (pAmbaSource)
	{
		fSourceCodec_ID = pAmbaSource->getSourceCodecID();

		log_debug("fSourceCodec_ID=0x%x", fSourceCodec_ID);

		if (fStream_type == STREAM_TYPE_PLAYBACK)
		{
			stream_reader_ctrl_box_t    ctrl_box;

			memset(&ctrl_box, 0x0, sizeof(stream_reader_ctrl_box_t));

			ctrl_box.cmd = STREAM_READER_CMD_PLAYBACK_GET_DURATION;
			ctrl_box.stream_tag = fStreamTag;
			ctrl_box.stream_type = fStream_type;
			StreamReader_Control(fStream_reader, NULL, &ctrl_box);

			fDuration = (float)ctrl_box.out.get_duration.duration / 1000.0;
		}

		switch (fSourceCodec_ID)
		{
		case AMP_FORMAT_MID_AAC:
			return pAmbaSource;
			break;

		case AMP_FORMAT_MID_PCM:
		case AMP_FORMAT_MID_ADPCM:
		case AMP_FORMAT_MID_MP3:
		case AMP_FORMAT_MID_AC3:
		case AMP_FORMAT_MID_WMA:
		case AMP_FORMAT_MID_OPUS:
			break;
		default:
			break;
		}
	}
	*/
	return NULL;
}

int MediaRTSPClient::CheckMediaConfiguration(char const *streamName,
	Boolean *pHave_amba_audio, Boolean *pHave_amba_video) {
	int result = 0;
/*
	AMBA_NETFIFO_MOVIE_INFO_CFG_s movie_info = { 0 };

	*pHave_amba_audio = False;
	*pHave_amba_video = False;

	do {
		if (streamName == NULL) {
			// live
			AMBA_NETFIFO_MEDIA_STREAMID_LIST_s stream_list = { 0 };

			result = AmbaNetFifo_GetMediaStreamIDList(&stream_list);
			if (result < 0) {
				log_error("Fail to do AmbaNetFifo_GetMediaStreamIDList()\n");
				break;
			}

			if (stream_list.Amount < 1) {
				log_error(
					"There is no valid stream. Maybe video record does not started yet.\n");
				result = 1;
				break;
			}

			result = AmbaNetFifo_GetMediaInfo(
				stream_list.StreamID_List[stream_list.Amount - 1],
				&movie_info);
			if (result < 0) {
				log_error("Fail to do AmbaNetFifo_GetMediaInfo()\n");
				break;
			}
		}
		else {
			// palyback
			AMBA_NETFIFO_PLAYBACK_OP_PARAM_s param_in = { 0,{ 0 } };
			AMBA_NETFIFO_PLAYBACK_OP_PARAM_s param_out = { 0,{ 0 } };

			param_in.OP = STREAM_READER_CMD_PLAYBACK_OPEN;
			snprintf((char*)param_in.Param, 128, "%s", streamName);
			result = AmbaNetFifo_PlayBack_OP(&param_in, &param_out);
			if (result < 0) {
				log_error("fail to do AmbaNetFifo_PlayBack_OP(0x%08x), %d\n",
					param_in.OP, result);
				break;
			}

			result = AmbaNetFifo_GetMediaInfo(param_out.OP, &movie_info);
			if (result < 0) {
				log_error("Fail to do AmbaNetFifo_GetMediaInfo()\n");
				break;
			}
		}

		if (movie_info.nTrack) {
			int i;
			for (i = 0; i < movie_info.nTrack; i++) {
				switch (movie_info.Track[i].nTrackType) {
				case AMBA_NETFIFO_MEDIA_TRACK_TYPE_VIDEO:
					*pHave_amba_video = True;
					break;
				case AMBA_NETFIFO_MEDIA_TRACK_TYPE_AUDIO:
					*pHave_amba_audio = True;
					break;
				default:
					break;
				}
			}
		}
	} while (0);
*/
	return 0;
}

RTPSink* MediaRTSPClient::getVideoRTPSink()
{
	return fVidRTPSink;
}

RTPSink* MediaRTSPClient::getAudioRTPSink()
{
	return fAudRTPSink;
}

RTPSink* MediaRTSPClient::createNewRTPSink(Groupsock *rtpGroupsock,
	unsigned char rtpPayloadTypeIfDynamic,
	FramedSource * inputSource)
{
	RTPSink* rtpSink = NULL;
/*
	AmbaAudioStreamSource *pAmbaSource = NULL;

	switch (fSourceCodec_ID)
	{
	case AMP_FORMAT_MID_AVC:
	case AMP_FORMAT_MID_H264:
		rtpSink = AmbaVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
			fSPS_payload, fSPS_length, fPPS_payload, fPPS_length);
		((AmbaVideoRTPSink*)rtpSink)->setPacketSizes(1200, 1200);

		fVidRTPSink = rtpSink;
		log_debug("fVidRTPSink = %p\n", fVidRTPSink);
		break;

	case AMP_FORMAT_MID_AAC:
		pAmbaSource = (AmbaAudioStreamSource*)inputSource;
		rtpSink = MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
			pAmbaSource->samplingFrequency(), "audio", "AAC-hbr", pAmbaSource->configStr(),
			pAmbaSource->numChannels());

		fAudRTPSink = rtpSink;
		log_debug("fAudRTPSink = %p\n", fAudRTPSink);
		break;

	default:
		break;
	}
*/
	log_debug("rtpSink = %p\n", rtpSink);
	return rtpSink;
}