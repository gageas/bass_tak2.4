#include <windows.h>
#include "bass_tak.h"
#include "takfunc.h"

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
	int readedSampleCount, readedBytes;
	int takBytePerSample, outBytePerSample;
	BYTE *readBuf, *bufP;

	takBytePerSample = stream->info.BPS/8*stream->info.NCH; // eg 16bit stereo -> 4
	outBytePerSample = ((stream->flags & BASS_SAMPLE_FLOAT)?4:2)*stream->info.NCH;
	int readSamples = length/outBytePerSample; // samples required to read.

	if(stream->szWorkBufLen < readSamples*takBytePerSample){
		stream->lpWorkBuf = (BYTE*)HeapReAlloc(GetProcessHeap(), 0, stream->lpWorkBuf, readSamples * takBytePerSample);
		stream->szWorkBufLen = readSamples * takBytePerSample;
	}

	bufP = readBuf = stream->lpWorkBuf;
	if(readBuf == NULL)error(BASS_ERROR_MEM);

	if(!TAK_ReadAudio(stream, readBuf, readSamples, &readedSampleCount)){
		readedBytes = BASS_STREAMPROC_END;
		goto end;
	}

	readedBytes = readedSampleCount*outBytePerSample;
	long elementCnt = readedSampleCount*stream->info.NCH;

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
end:
	noerrorn(readedBytes);
}

HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags){
	if (bassfunc->file.GetFlags(file)&BASSFILE_BUFFERED) error(BASS_ERROR_FILEFORM);

	TAKSTREAM *stream = (TAKSTREAM*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TAKSTREAM));
	if(stream == NULL){ goto failed; }
	stream->f = file;
	
	if(!TAK_OpenFile(stream)) { goto failed; }

	// restrict flags to valid ones, and create the BASS stream
	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX
		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE
		|0x3f000000; // 0x3f000000 = all speaker flags
	if (!(stream->handle=bassfunc->CreateStream((DWORD)stream->info.SAMPLERATE,stream->info.NCH,flags,(STREAMPROC*)&StreamProc,stream,&TAKfuncs))){ goto failed; }
	stream->flags=flags;
	bassfunc->file.SetStream(file,stream->handle); // set the stream associated with the file
	noerrorn(stream->handle);

failed:
	TAKfuncs.Free(stream);// TAK_Free(stream);
	error(BASS_ERROR_FILEFORM);
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
				MessageBoxA(0,"Incorrect BASS.DLL version ("BASSVERSIONTEXT" is required)","BASS_TAK",MB_ICONERROR);
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

