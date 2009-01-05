#include "bass_tak.h"
#include "tak_deco_lib.h"
#include "ioif.h"

#define INIT TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
#define INIT_AND_CHECK INIT; if(!current_stream){return tak_False;};
TtakBool always_false(void * AUser){return tak_False;}
typedef TtakBool(*BoolVoid_Callback)(void*);

// TAK IO functions
TtakBool CanRead(void * AUser){
	INIT_AND_CHECK;
	return true;
}

BoolVoid_Callback CanSeek = CanRead;

BoolVoid_Callback CanWrite = always_false;
BoolVoid_Callback Flush = always_false;
BoolVoid_Callback Truncate = always_false;

TtakBool Read(void * AUser, void *      ABuf, TtakInt32   ANum, TtakInt32 * AReadNum ){
	INIT_AND_CHECK;
	*AReadNum = bassfunc->file.Read(current_stream->f,ABuf,ANum);
	if(AReadNum>0)return tak_True;
	return tak_False;
}

TtakBool Write(void * AUser,const void * ABuf,TtakInt32 ANum){
	return tak_False;
}

TtakBool Seek(void * AUser, TtakInt64 APos){
	INIT_AND_CHECK;
	bassfunc->file.Seek(current_stream->f,(DWORD)APos);
	return tak_True;
}

TtakBool GetLength(void * AUser,TtakInt64 * ALength){
	INIT_AND_CHECK;
	*ALength = bassfunc->file.GetPos(current_stream->f,BASS_FILEPOS_END);
	return tak_True;
}

TtakStreamIoInterface IOIF = {
	CanRead,
	CanWrite,
	CanSeek,
	Read,
	Write,
	Flush,
	Truncate,
	Seek,
	GetLength,
};