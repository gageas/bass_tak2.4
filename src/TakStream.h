#pragma once

#include "ape.h"
#include "bass-addon.h"
#include "tak_deco_lib.h"

#pragma comment(lib,"bass.lib")

// SetError macros
#define noerror() return (bassfunc->SetError(BASS_OK),TRUE)
#define noerrorn(n) return (bassfunc->SetError(BASS_OK),n)
#define error(n) return (bassfunc->SetError(n),FALSE) // error = 0/NULL
#define errorn(n) return (bassfunc->SetError(n),-1) // error = -1
#define errorcp(n) return (char*)(bassfunc->SetError(n),NULL)

typedef struct {
	const char *asApeTag;
	const char *id3_24frameKey;
	const int charCode;
	const char *id3_24valuePrefix;
	const int id3_24valuePrefixLen;
	const int id3_1_pos;
	const int id3_1_len;
} ID3TAGTEMPLETE;

static const ID3TAGTEMPLETE gWellKnownID3Tags[] = {
	//    APE               ID3v2   CC  Prefix len  ID3v1 len
	{APE_TAG_FIELD_TITLE,   "TIT2", 3,  NULL,    0,   3,   30},
	{APE_TAG_FIELD_ARTIST,  "TPE1", 3,  NULL,    0,  33,   30},
	{APE_TAG_FIELD_ALBUM,   "TALB", 3,  NULL,    0,  63,   30},
	{APE_TAG_FIELD_YEAR,    "TDRC", 0,  NULL,    0,  93,    4},
	{APE_TAG_FIELD_COMMENT, "COMM", 3,  "ENG\0", 4,  97,   28},
	{APE_TAG_FIELD_TRACK,   "TRCK", 0,  NULL,    0, 126,    0}, /* need special process for this tag(ID3V1) */
};

class TakStream
{
private:
	void* m_takDecoder;
	bool m_apeTagExtracted;
	bool m_id3TagExtracted;
	long m_workBufLen;
	BASSFILE m_bassFile;
	DWORD m_flags;
	Ttak_str_StreamInfo m_info;
	BYTE *m_decodeBuf;
	BYTE m_id3v1Tag[128];
	BYTE *m_id3v2Tag;
	BYTE *m_apeTag;

	static DWORD CALLBACK streamProc(HSTREAM handle, void *buffer, DWORD length, void *stream);
	static void convertSampleTo16bit(BYTE *dest, const BYTE *src, int samples, int srcBits);
	void extractApeTag();
	void extractID3Tags();
	inline int getBytesPerSample();
	DWORD streamProcIntl(BYTE* buffer, DWORD length);

public:
	TakStream(DWORD flags);
	~TakStream();
	HSTREAM openDecoder(BASSFILE f);
	long decode(long readSamples);
	void* getApeTag();
	void* getID3V1Tag();
	void* getID3V2Tag();
	QWORD getPosition();
	bool setPosition(QWORD pos);
	QWORD getLength();
	DWORD getFileFlags();
	bool isFloating();
};
