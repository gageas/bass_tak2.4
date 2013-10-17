#include "TakStream.h"
#include "takfunc.h"

// this may cnange.(there's no reason for current value)
#define BASS_CTYPE_STREAM_TAK	0x12000

// called by BASS_StreamGetLength - get the stream playback length
QWORD WINAPI TAK_GetLength(void *inst, DWORD mode)
{
	TakStream *stream=(TakStream*)inst;
	if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
	noerrorn(stream->getLength()); // return as 16bit
}

// get Tags
const char *WINAPI TAK_GetTags(void* inst, DWORD tags)
{
	TakStream *stream=(TakStream*)inst;

	switch(tags){
		case BASS_TAG_APE:
			return (char*)stream->getApeTag();
		case BASS_TAG_ID3V2:
			return (char*)stream->getID3V2Tag();
		case BASS_TAG_ID3:
			return (char*)stream->getID3V1Tag();
	}

	return NULL;
}

void WINAPI TAK_GetInfo(void* inst, BASS_CHANNELINFO* info)
{
	TakStream *stream=(TakStream*)inst;
	info->flags|=stream->getFileFlags()&BASS_STREAM_RESTRATE;
	// BASS does not support 24bit(Int) or 32bit(Int) samples, so we need to convert them into 16bit(Int).
	// Floating point(32bitfloat) mode may have advantage of resolution for hi-res source.
	// Because of this, this library uses Floating point mode as possible.
	// info->origres=stream->info.Audio.SampleBits;
	info->origres=16;
	info->ctype=BASS_CTYPE_STREAM_TAK;
}

// called by BASS_ChannelSetFlags
// return accepted flags (BASS only uses BASS_SAMPLE_LOOP/BASS_STREAM_AUTOFREE/BASS_STREAM_RESTRATE)
DWORD WINAPI TAK_SetFlags(HSTREAM handle, DWORD flags)
{
	return flags;
}

QWORD WINAPI TAK_GetPosition(void *inst, QWORD pos, DWORD mode)
{
	TakStream* stream = (TakStream*)inst;
	return stream->getPosition();
}

BOOL WINAPI TAK_CanSetPosition(void *inst, QWORD pos, DWORD mode)
{
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
	if (pos>=TAK_GetLength(inst,BASS_POS_BYTE)) error(BASS_ERROR_POSITION);
	return TRUE;
}

// called by BASS_ChannelSetPosition
// called after the above function - do the actual seeking
// return the actual resulting position
QWORD WINAPI TAK_SetPosition(void *inst, QWORD pos, DWORD mode)
{
	TakStream* stream=(TakStream*)inst;
	stream->setPosition(pos);
	return pos;
}

void WINAPI TAK_Free(void* inst)
{
	delete (TakStream*)inst;
}

// add-on function table
ADDON_FUNCTIONS TAKfuncs={
	0, // flags
	TAK_Free,
	TAK_GetLength,
	TAK_GetTags,
	NULL, // let BASS handle file position
	TAK_GetInfo,
	TAK_CanSetPosition,
	TAK_SetPosition,
	NULL, // let BASS handle the position/looping/syncing (POS & END)
	NULL, // TAK_SetSync,
	NULL, // TAK_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
	NULL // no attributes
};



