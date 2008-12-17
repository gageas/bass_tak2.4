#include <windows.h>
#include "bass_tak.h"
#include "ioif.h"
#include "takfunc.h"

extern TtakStreamIoInterface IOIF;

// BASS_CHANNELINFO type

// bass plugin information
static const BASS_PLUGINFORM pluginforms[]={
	{BASS_CTYPE_STREAM,"TAK file","*.tak"},
};
static const BASS_PLUGININFO plugininfo={0x02040000,1,pluginforms};

typedef struct
{
	HSYNC	handle;
	DWORD	type;
	QWORD	param;
} SYNC;

// Additional config options
#define BASS_CONFIG_TAK_FREQ	0x11000
#define BASS_CONFIG_TAK_CHANS	0x11001

static DWORD freq=44100, chans=2;

// add-on function table
static ADDON_FUNCTIONS TAKfuncs={
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

// config function - called by BASS_SetConfig/Ptr and BASS_GetConfig/Ptr
static BOOL CALLBACK ConfigProc(DWORD option, DWORD flags, void *value)
{
	if (!(flags&BASSCONFIG_PTR)) {
		DWORD *dvalue=(DWORD*)value;
		switch (option) {
			case BASS_CONFIG_TAK_FREQ:
				if (flags&BASSCONFIG_SET) freq=*dvalue;
				else *dvalue=freq;
				return TRUE;
			case BASS_CONFIG_TAK_CHANS:
				if (flags&BASSCONFIG_SET) chans=*dvalue;
				else *dvalue=chans;
				return TRUE;
		}
	}
	return FALSE;
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE* buffer, DWORD length, TAKSTREAM *stream)
{
	DWORD count=0;
	DWORD offset=0;
	int readedSampleCount;
	int readedBytes;
	int takBytePerSample;
	int outBytePerSample;
	BYTE* readBuf;
	takBytePerSample = stream->info.BPS/8*stream->info.NCH; // eg 16bit stereo -> 4
	outBytePerSample = ((stream->flags & BASS_SAMPLE_FLOAT)?4:2)*stream->info.NCH;

	int readSamples = length/outBytePerSample;
	readBuf = (BYTE*)HeapAlloc(GetProcessHeap(),0,readSamples*takBytePerSample);
	if(tak_SSD_ReadAudio(stream->streamDec , readBuf , readSamples , &readedSampleCount) != tak_res_Ok){
		free(readBuf);
		return offset|BASS_STREAMPROC_END;
	}
	if(readedSampleCount <= 0){
		free(readBuf);
		return offset|BASS_STREAMPROC_END;
	}
	readedBytes = readedSampleCount*outBytePerSample;

	long elementCnt = readedSampleCount*stream->info.NCH;
	BYTE* bufP = readBuf;
	if(stream->flags & BASS_SAMPLE_FLOAT){
		bassfunc->data.Int2Float(bufP,(float*)buffer,elementCnt,stream->info.BPS/8);
	}else{
		switch(stream->info.BPS){
			case 8: // 8bit -> 16bit
				while(elementCnt--){
					*((signed short*)buffer) = (((signed int)(*bufP++))-128) * 0x100;
					buffer+=2;
				}
				break;
			case 16: // 16bit -> 16bit
				memcpy(buffer,readBuf,readedBytes);
				break;
			case 24: // 24bit -> 16bit
				while(elementCnt--){ // tested with http://trendy.nikkeibp.co.jp/article/news/20060419/116354/
					bufP++;
					*buffer++=*bufP++;
					*buffer++=*bufP++;
				}
				break;
			case 32: // 32bit -> 16bit not tested.
				while(elementCnt--){
					bufP+=2;
					*buffer++=*bufP++;
					*buffer++=*bufP++;
				}
				break;
		}
	}
	HeapFree(GetProcessHeap(),0,readBuf);
	return offset+readedBytes;
}

HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags){

	TtakSSDOptions Options;
	Options.Cpu = tak_Cpu_Any;
	Options.Flags = 0;
	if (bassfunc->file.GetFlags(file)&BASSFILE_BUFFERED) error(BASS_ERROR_FILEFORM);

	TAKSTREAM *stream=(TAKSTREAM*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TAKSTREAM));

	stream->f = file;
	
	stream->streamDec = tak_SSD_Create_FromStream(&IOIF,stream,&Options,Tak_Damaged,stream);
	if(stream->streamDec == NULL){
		return 0;
	}
/*
	BYTE buf[RETRY_TIMES*50];
	int readed;
	for(int i=0;i<=RETRY_TIMES;i++){
		// 理由は分からないが、適当にデータにアクセスしてからじゃないとtak_SSD_Validで失敗しがち?。 超ハマりポイント
		IOIF.Read(stream,buf,i*50,&readed);
		Sleep(1);
		IOIF.Seek(stream,0);
		if (TAK_OpenFile(stream) == 0 ) { // success
			break;
		}else{
			if(i==RETRY_TIMES){ // error rescue
				TAK_Free(stream);
				error(BASS_ERROR_FILEFORM);
			}
		}
	}*/
	if (TAK_OpenFile(stream) != 0 ) {
		TAK_Free(stream);
		error(BASS_ERROR_FILEFORM);
	}

	// restrict flags to valid ones, and create the BASS stream
	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX
		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE
		|0x3f000000; // 0x3f000000 = all speaker flags
	if (!(stream->handle=bassfunc->CreateStream((DWORD)stream->info.SAMPLERATE,stream->info.NCH,flags,(STREAMPROC*)&StreamProc,stream,&TAKfuncs))) {
		TAK_Free(stream);
		return 0;
	}
	stream->flags=flags;
	bassfunc->file.SetStream(file,stream->handle); // set the stream associated with the file
	noerrorn(stream->handle);
}

HSTREAM WINAPI BASS_TAK_StreamCreateFile(BOOL mem, const void *file, DWORD offset, DWORD length, DWORD flags)
{
#ifndef _WIN32
if (badbass) error(BASS_ERROR_VERSION);
#endif

	BASSFILE bfile=bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile){
		return 0;
	}
	HSTREAM s=StreamCreateProc(bfile,flags);
	if (!s){
		bassfunc->file.Close(bfile);
		return 0;
	}
	return s;
}

HSTREAM WINAPI BASS_TAK_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user)
{
	HSTREAM s;
	BASSFILE bfile;
#ifndef _WIN32
if (badbass) error(BASS_ERROR_VERSION);
#endif
	bfile=bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; // OpenURL set the error code
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI BASS_TAK_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user)
{
	BASSFILE bfile;
	HSTREAM s;
#ifndef _WIN32
if (badbass) error(BASS_ERROR_VERSION);
#endif
	bfile=bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

const void *WINAPI BASSplugin(DWORD face){
	switch (face) {
		case BASSPLUGIN_INFO:
			return &plugininfo;
		case BASSPLUGIN_CREATE:
			return (void*)&StreamCreateProc;
	}
	return NULL;
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved){
	switch(fdwReason){
		case DLL_PROCESS_ATTACH: //initialize
			DisableThreadLibraryCalls(hinstDLL);
			if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
				MessageBoxA(0,"Incorrect BASS.DLL version ("BASSVERSIONTEXT" is required)","BASS_RAW",MB_ICONERROR);
				return FALSE;
			}
			bassfunc->RegisterPlugin(ConfigProc,PLUGIN_CONFIG_ADD); // register config function for freq/chans options
			break;

		case DLL_PROCESS_DETACH:
			bassfunc->RegisterPlugin(ConfigProc,PLUGIN_CONFIG_REMOVE); // unregister the config function
			break;
	}

	return TRUE;
}

