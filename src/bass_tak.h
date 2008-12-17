#include "bass-addon.h"
#include "bass.h"
#include "tak_deco_lib.h"

#pragma comment(lib,"bass.lib")
#pragma comment(lib,"tak_deco_lib.lib")

// SetError macros
#define noerror() return (bassfunc->SetError(BASS_OK),TRUE)
#define noerrorn(n) return (bassfunc->SetError(BASS_OK),n)
#define error(n) return (bassfunc->SetError(n),FALSE) // error = 0/NULL
#define errorn(n) return (bassfunc->SetError(n),-1) // error = -1
#define errorcp(n) return (char*)(bassfunc->SetError(n),NULL)

#define SYNCTYPEMASK 0x00ffffff
#define SYNCFLAGMASK 0xff000000

// ÇƒÇØÇ∆Å[
#define BASS_CTYPE_STREAM_TAK	0x12000
//#define BASS_CTYPE_STREAM_TAK	0x100007


/*****************************************************************************************
"Standard" APE tag fields
*****************************************************************************************/
#define APE_TAG_FIELD_TITLE                     "Title"
#define APE_TAG_FIELD_ARTIST                    "Artist"
#define APE_TAG_FIELD_ALBUM                     "Album"
#define APE_TAG_FIELD_COMMENT                   "Comment"
#define APE_TAG_FIELD_YEAR                      "Year"
#define APE_TAG_FIELD_TRACK                     "Track"
#define APE_TAG_FIELD_GENRE                     "Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT           "Cover Art (front)"
#define APE_TAG_FIELD_NOTES                     "Notes"
#define APE_TAG_FIELD_LYRICS                    "Lyrics"
#define APE_TAG_FIELD_COPYRIGHT                 "Copyright"
#define APE_TAG_FIELD_BUY_URL                   "Buy URL"
#define APE_TAG_FIELD_ARTIST_URL                "Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL             "Publisher URL"
#define APE_TAG_FIELD_FILE_URL                  "File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL             "Copyright URL"
#define APE_TAG_FIELD_MJ_METADATA               "Media Jukebox Metadata"
#define APE_TAG_FIELD_TOOL_NAME                 "Tool Name"
#define APE_TAG_FIELD_TOOL_VERSION              "Tool Version"
#define APE_TAG_FIELD_PEAK_LEVEL                "Peak Level"
#define APE_TAG_FIELD_REPLAY_GAIN_RADIO         "Replay Gain (radio)"
#define APE_TAG_FIELD_REPLAY_GAIN_ALBUM         "Replay Gain (album)"
#define APE_TAG_FIELD_COMPOSER                  "Composer"
#define APE_TAG_FIELD_KEYWORDS                  "Keywords"

/*****************************************************************************************
Standard APE tag field values
*****************************************************************************************/
#define APE_TAG_GENRE_UNDEFINED                 "Undefined"

/*****************************************************************************************
The footer at the end of APE tagged files (can also optionally be at the front of the tag)
*****************************************************************************************/
#define APE_TAG_FOOTER_BYTES    32

/*****************************************************************************************
Structure of TAKSTREAM information
*****************************************************************************************/
typedef struct {
	unsigned short	NCH;		// number of channels
	unsigned short	BPS;		// bits per sample eg.8,16,24
	unsigned short	BSIZE;		// byte size
	unsigned short	FORMAT;		// audio format
	TtakInt64		SAMPLERATE;	// samplerate (sps)
	TtakInt64		DATALENGTH;	// data length in samples // sample number
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
	TtakSeekableStreamDecoder streamDec;
	HSTREAM handle;
	BASSFILE f;
	DWORD flags;
	INFO info;
} TAKSTREAM;

/*****************************************************************************************
BASS_TAG_APE copied from fittle source (BASS2.3?)
*****************************************************************************************/
#define BASS_TAG_APE            6