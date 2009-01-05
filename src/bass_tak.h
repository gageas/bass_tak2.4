#include "bass-addon.h"

#pragma comment(lib,"bass.lib")

// SetError macros
#define noerror() return (bassfunc->SetError(BASS_OK),TRUE)
#define noerrorn(n) return (bassfunc->SetError(BASS_OK),n)
#define error(n) return (bassfunc->SetError(n),FALSE) // error = 0/NULL
#define errorn(n) return (bassfunc->SetError(n),-1) // error = -1
#define errorcp(n) return (char*)(bassfunc->SetError(n),NULL)

#define SYNCTYPEMASK 0x00ffffff
#define SYNCFLAGMASK 0xff000000

/*****************************************************************************************
Structure of TAKSTREAM information
*****************************************************************************************/
typedef struct {
	unsigned short	NCH;		// number of channels
	unsigned short	BPS;		// bits per sample eg.8,16,24
	unsigned short	BSIZE;		// byte size
	unsigned short	FORMAT;		// audio format
	INT64			SAMPLERATE;	// samplerate (sps)
	INT64			DATALENGTH;	// data length in samples // sample number
	unsigned long	STATE;		// return code
	BYTE            ID3V1[128];
	BYTE*           ID3V2;
	BYTE*           APETAG;
} INFO;

/*****************************************************************************************
STRUCT OF TAKSTREAM
*****************************************************************************************/
typedef struct
{
	void* streamDec;
	HSTREAM handle;
	BASSFILE f;
	DWORD flags;
	INFO info;
} TAKSTREAM;
