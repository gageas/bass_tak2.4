#pragma once

#include "bass-addon.h"
#include "tak_deco_lib.h"

#pragma comment(lib,"bass.lib")

// SetError macros
#define noerror() return (bassfunc->SetError(BASS_OK),TRUE)
#define noerrorn(n) return (bassfunc->SetError(BASS_OK),n)
#define error(n) return (bassfunc->SetError(n),FALSE) // error = 0/NULL
#define errorn(n) return (bassfunc->SetError(n),-1) // error = -1
#define errorcp(n) return (char*)(bassfunc->SetError(n),NULL)

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
