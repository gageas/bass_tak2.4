#include "bass_tak.h"
#include "tak_deco_lib.h"
#pragma comment(lib,"tak_deco_lib.lib")
#include "ioif.h"
#include "ape.h"
#include "takfunc.h"

// this may cnange.(there's no reason for current value)
#define BASS_CTYPE_STREAM_TAK	0x12000

/*****************************************************************************************
BASS_TAG_APE copied from fittle source (BASS2.3?)
*****************************************************************************************/
#define BASS_TAG_APE            6

int setID3V24Frame(BYTE* buf,int charcode,const char* frameId,const BYTE* value,int valsize){
	memcpy(buf,frameId,4);
	if(charcode == -1){
		memcpy(buf+10,value,valsize);
	}else{
		buf[10] = charcode; // charcode
		memcpy(buf+11,value,valsize);
		valsize++;
	}
	buf[4] = (valsize>>21) & 0x7F;
	buf[5] = (valsize>>14) & 0x7F;
	buf[6] = (valsize>>7) & 0x7F;
	buf[7] = valsize & 0x7F;
	buf[8] = 0; // status flag;
	buf[9] = 0; // format flag;
	return 10 + valsize;
}

#define TAG_VALUE_SIZE 1024
void GetID3Tags(TAKSTREAM* stream){
	TtakAPEv2Tag tag;
	int i = 0; // loop counter 
	int len = 0; // tags count
	char key[64]; // tagName(key)
	BYTE* value; // value as UTF8
	char* valueAnsi; // value as Ansi
	int readed; // readed bytes of UTF8 value
	int readedAnsi; // readed bytes of Ansi value
	int tagsize = 0; // current V2 tag size

	tag = tak_SSD_GetAPEv2Tag(stream->streamDec);
	if( tak_APE_Valid(tag) != tak_True){return;}

	len = tak_APE_GetItemNum(tag);

	// init V1 Tag
	memcpy_s(stream->info.ID3V1,3,"TAG",3);

	tagsize = 10; // initial v2.4 tag size
	stream->info.ID3V2 = (BYTE*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,tagsize); //calloc(1,10); // init V2.4 Tag

	value = (BYTE*)HeapAlloc(GetProcessHeap(),0,TAG_VALUE_SIZE);
	valueAnsi = (char*)HeapAlloc(GetProcessHeap(),0,TAG_VALUE_SIZE);
	if(stream->info.ID3V2 == NULL){
		return; // could not alloc
	}
	memcpy_s(stream->info.ID3V2,3,"ID3",3);
	stream->info.ID3V2[3] = 0x04; //Version
	for(i=0;i<len;i++){
		tak_APE_GetItemKey(tag,i,key,64,&readed);

		tak_APE_GetItemValue(tag,i,NULL,0,&readed); // get Bytes
		if(readed > TAG_VALUE_SIZE){
			value = (BYTE*)HeapReAlloc(GetProcessHeap(),0,value,readed);
		}
		tak_APE_GetItemValue(tag,i,value,readed,&readed);

		tak_APE_GetTextItemValueAsAnsi(tag,i,-1,' ',NULL,0,&readedAnsi); // get bytes
		if(readedAnsi > TAG_VALUE_SIZE){
			valueAnsi = (char*)HeapReAlloc(GetProcessHeap(),0,valueAnsi,readedAnsi);
		}
		tak_APE_GetTextItemValueAsAnsi(tag,i,-1,' ',valueAnsi,readedAnsi,&readedAnsi);

		if( strcmp(key,APE_TAG_FIELD_TITLE) == 0 ){
			// for v2.4
			stream->info.ID3V2 = (BYTE*)HeapReAlloc(GetProcessHeap(),0,stream->info.ID3V2,tagsize+11+readed);
			tagsize += setID3V24Frame(stream->info.ID3V2+tagsize,3,"TIT2",value,readed);
			// for v1
			memcpy(stream->info.ID3V1 + 3,valueAnsi,min(readedAnsi,30));
		}else if( strcmp(key,APE_TAG_FIELD_ARTIST) == 0 ){
			stream->info.ID3V2 = (BYTE*)HeapReAlloc(GetProcessHeap(),0,stream->info.ID3V2,tagsize+11+readed);
			tagsize += setID3V24Frame(stream->info.ID3V2+tagsize,3,"TPE1",value,readed);
			// for v1
			memcpy(stream->info.ID3V1 + 33,valueAnsi,min(readedAnsi,30));
		}else if( strcmp(key,APE_TAG_FIELD_ALBUM) == 0 ){
			stream->info.ID3V2 = (BYTE*)HeapReAlloc(GetProcessHeap(),0,stream->info.ID3V2,tagsize+11+readed);
			tagsize += setID3V24Frame(stream->info.ID3V2+tagsize,3,"TALB",value,readed);
			// for v1
			memcpy(stream->info.ID3V1 + 63,valueAnsi,min(readedAnsi,30));
		}else if( strcmp(key,APE_TAG_FIELD_YEAR) == 0 ){
			stream->info.ID3V2 = (BYTE*)HeapReAlloc(GetProcessHeap(),0,stream->info.ID3V2,tagsize+11+readed);
			tagsize += setID3V24Frame(stream->info.ID3V2+tagsize,0,"TDRC",value,readed);
			// for v1
			memcpy(stream->info.ID3V1 + 93,valueAnsi,min(readedAnsi,4));
		}else if( strcmp(key,APE_TAG_FIELD_COMMENT) == 0 ){
			BYTE* t= (BYTE*)HeapAlloc(GetProcessHeap(),0,4+readed);
			strcpy_s((char*)t,4,"ENG");// Not correct for other languages, but I have no idea to detect language.
			memcpy_s((char*)t+4,readed,value,readed);
			readed+=4;
			stream->info.ID3V2 = (BYTE*)HeapReAlloc(GetProcessHeap(),0,stream->info.ID3V2,tagsize+11+readed);

			tagsize += setID3V24Frame(stream->info.ID3V2+tagsize,3,"COMM",t,readed);
			HeapFree(GetProcessHeap(),0,t);
			memcpy(stream->info.ID3V1 + 97,valueAnsi,min(readedAnsi,28));
		}else if( strcmp(key,APE_TAG_FIELD_TRACK) == 0 ){
			stream->info.ID3V2 = (BYTE*)HeapReAlloc(GetProcessHeap(),0,stream->info.ID3V2,tagsize+11+readed);
			tagsize += setID3V24Frame(stream->info.ID3V2+tagsize,0,"TRCK",value,readed);
			// for v1
			stream->info.ID3V1[126] = atoi(valueAnsi);
		}
	}
	HeapFree(GetProcessHeap(),0,value);
	HeapFree(GetProcessHeap(),0,valueAnsi);
	tagsize -= 10;
	stream->info.ID3V2[9] = tagsize & 0x7F;
	stream->info.ID3V2[8] = (tagsize>>7) & 0x7F;
	stream->info.ID3V2[7] = (tagsize>>14) & 0x7F;
	stream->info.ID3V2[6] = (tagsize>>21) & 0x7F;
}

void GetApeTag(TAKSTREAM* stream){
	BASSFILE file;
	file = stream->f;
	BYTE apeTagFooterBuf[APE_TAG_FOOTER_BYTES];
	QWORD fileSize; // tak file size
	DWORD tagSize; // tagSize = apeTagBody + apeTagFooter[32]

	fileSize = bassfunc->file.GetPos(file,BASS_FILEPOS_END);
	bassfunc->file.Seek(file,fileSize - APE_TAG_FOOTER_BYTES);
	bassfunc->file.Read(file,apeTagFooterBuf,APE_TAG_FOOTER_BYTES);
	if(memcmp(apeTagFooterBuf,"APETAGEX",8)==0){
		tagSize = (apeTagFooterBuf[15]<<24)+(apeTagFooterBuf[14]<<16)+(apeTagFooterBuf[13]<<8)+apeTagFooterBuf[12];
		if(tagSize >= (1024 * 1024 * 16)){ // too large
			return;
		}
		stream->info.APETAG = (BYTE*)HeapAlloc(GetProcessHeap(),0,tagSize+APE_TAG_FOOTER_BYTES); // header[32] + body + footer[32]
		if(stream->info.APETAG != NULL){
			memcpy(stream->info.APETAG,apeTagFooterBuf,APE_TAG_FOOTER_BYTES);
			bassfunc->file.Seek(file,fileSize - tagSize);
			bassfunc->file.Read(file,stream->info.APETAG + APE_TAG_FOOTER_BYTES,tagSize);
		}
		bassfunc->file.Seek(file,0);
	}
}


// called by BASS_StreamGetLength - get the stream playback length
QWORD WINAPI TAK_GetLength(void *inst, DWORD mode)
{
	TAKSTREAM *stream=(TAKSTREAM*)inst;
	if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
	noerrorn(stream->info.DATALENGTH*((stream->flags & BASS_SAMPLE_FLOAT)?4:2)*stream->info.NCH); // return as 16bit
}

// get Tags
const char *WINAPI TAK_GetTags(void* inst, DWORD tags)
{
	TAKSTREAM *stream=(TAKSTREAM*)inst;

	switch(tags){
		case BASS_TAG_APE:
			return (char*)stream->info.APETAG;
			break;
		case BASS_TAG_ID3V2:
			return (char*)stream->info.ID3V2;
			break;
		case BASS_TAG_ID3:
			return (char*)stream->info.ID3V1;
			break;
	}
	return NULL;
}

void WINAPI TAK_GetInfo(void* inst, BASS_CHANNELINFO* info)
{
	TAKSTREAM *stream=(TAKSTREAM*)inst;
	info->flags|=bassfunc->file.GetFlags(stream->f)&BASS_STREAM_RESTRATE;
	info->origres=stream->info.BPS;
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
	TAKSTREAM* stream = (TAKSTREAM*)inst;
	int BytePerSample = (stream->flags & BASS_SAMPLE_FLOAT?4:2)*stream->info.NCH; //stream->info.BPS/8
	return tak_SSD_GetReadPos(stream->streamDec);
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
	TAKSTREAM* stream=(TAKSTREAM*)inst;

	int BytePerSample = (stream->flags & BASS_SAMPLE_FLOAT?4:2)*stream->info.NCH; //stream->info.BPS/8
	tak_SSD_Seek(stream->streamDec,pos/BytePerSample);
	return pos;
}

static void FreeTAK(TAKSTREAM* tak){
	if(tak){
		tak_SSD_Destroy(tak->streamDec);
		HeapFree(GetProcessHeap(),0,tak->info.APETAG);
		HeapFree(GetProcessHeap(),0,tak->info.ID3V2);
		HeapFree(GetProcessHeap(),0,tak);
	}
	return;
}

void WINAPI TAK_Free(void* inst)
{
	FreeTAK((TAKSTREAM*)inst);
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

bool TAK_OpenFile (TAKSTREAM *tak) {
	int ret;
	Ttak_str_StreamInfo info;

	TtakSSDOptions Options;
	Options.Cpu = tak_Cpu_Any;
	Options.Flags = 0;

	if((tak->streamDec = tak_SSD_Create_FromStream(&IOIF,tak,&Options,NULL,tak)) == NULL){ return false; }
	if(tak_SSD_Valid(tak->streamDec) != tak_True){ return false; }
	if((ret = tak_SSD_GetStreamInfo(tak->streamDec,&info)) != tak_res_Ok){ return false; }

	tak->info.NCH = info.Audio.ChannelNum;
	tak->info.BPS = info.Audio.SampleBits;
	tak->info.BSIZE = info.Sizes.FrameSize;
	tak->info.FORMAT = info.Encoder.Codec;
	tak->info.SAMPLERATE = info.Audio.SampleRate;
	tak->info.DATALENGTH = info.Sizes.SampleNum;
	GetApeTag(tak);
	GetID3Tags(tak);
	tak_SSD_Seek(tak->streamDec,0);
	return true;
}

bool TAK_ReadAudio(TAKSTREAM *stream,void* readBuf, long readSamples, int* readedSampleCount){
	TtakInt32 readed;
	if((tak_SSD_ReadAudio(stream->streamDec, readBuf, readSamples, &readed) != tak_res_Ok) || (readed <= 0)){
		return false;
	}
	*readedSampleCount = readed;
	return true;
}
