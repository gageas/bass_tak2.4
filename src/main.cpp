#include <windows.h>
#include <stdint.h>
#include "TakStream.h"

static const DWORD CREATE_PROC_FLAG_MASK = BASS_SAMPLE_FLOAT | BASS_SAMPLE_SOFTWARE
	| BASS_SAMPLE_LOOP | BASS_SAMPLE_3D | BASS_SAMPLE_FX
	| BASS_STREAM_DECODE | BASS_STREAM_AUTOFREE
	| 0x3f000000; // 0x3f000000 = all speaker flags

// bass plugin information
static const BASS_PLUGINFORM pluginforms[] = {
	{BASS_CTYPE_STREAM, "TAK file", "*.tak"},
};
static const BASS_PLUGININFO plugininfo = {
	0x02040000, 1, pluginforms
};

// config function - called by BASS_SetConfig/Ptr and BASS_GetConfig/Ptr
static BOOL CALLBACK ConfigProc(DWORD option, DWORD flags, void *value)
{
	return FALSE;
}

HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags){
	if (bassfunc->file.GetFlags(file) & BASSFILE_BUFFERED) {
		bassfunc->SetError(BASS_ERROR_FILEFORM);
		return 0;
	}

	// restrict flags to valid ones, and create the BASS stream
	flags &= CREATE_PROC_FLAG_MASK;

	TakStream *stream = new TakStream(flags);
	if (!stream) {
		bassfunc->SetError(BASS_ERROR_MEM);
		return 0;
	}
	
	HSTREAM handle = stream->openDecoder(file);
	if(!handle) {
		delete stream;
		bassfunc->SetError(BASS_ERROR_FILEFORM);
		return 0;
	}
	noerrorn(handle);
}

HSTREAM WINAPI BASS_TAK_StreamCreateFile(BOOL mem, const void *file, DWORD offset, DWORD length, DWORD flags)
{
#ifndef _WIN32
if (badbass) error(BASS_ERROR_VERSION);
#endif
	BASSFILE bfile = bassfunc->file.Open(mem, file, offset, length, flags, FALSE);
	if (!bfile){
		return 0;
	}
	HSTREAM s = StreamCreateProc(bfile, flags);
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
	bfile = bassfunc->file.OpenURL(url, offset, flags, proc, user, FALSE);
	if (!bfile) {
		return 0; // OpenURL set the error code
	}
	s = StreamCreateProc(bfile, flags);
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
	bfile = bassfunc->file.OpenUser(system, flags, procs, user, FALSE);
	s = StreamCreateProc(bfile, flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

const void *WINAPI BASSplugin(DWORD face){
	switch (face) {
		case BASSPLUGIN_INFO:
			return &plugininfo;
		case BASSPLUGIN_CREATE:
			return StreamCreateProc;
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

