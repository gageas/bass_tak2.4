#include "bass_tak.h"
#include "ioif.h"

// TAK IO functions
TtakBool CanRead(void * AUser){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	if(current_stream)return tak_True;
	return tak_False;
};
TtakBool CanWrite(void * AUser){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	return tak_False;
}
TtakBool CanSeek(void * AUser){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	if(current_stream)return tak_True;
	return tak_False;
}
TtakBool Read(void * AUser, void *      ABuf, TtakInt32   ANum, TtakInt32 * AReadNum ){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	if(current_stream){
		*AReadNum = bassfunc->file.Read(current_stream->f,ABuf,ANum);
		if(AReadNum>0)return tak_True;
	}
	return tak_False;
}
TtakBool Write(void * AUser,const void * ABuf,TtakInt32    ANum){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	return tak_False;
}
TtakBool Flush(void * AUser){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	return tak_False;
}
TtakBool Truncate(void * AUser){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	return tak_False;
}
TtakBool Seek(void * AUser, TtakInt64 APos){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	if(current_stream){
		bassfunc->file.Seek(current_stream->f,(DWORD)APos);
		return tak_True;
	}
	return tak_False;
}
TtakBool GetLength(void * AUser,TtakInt64 * ALength){
	TAKSTREAM* current_stream = (TAKSTREAM*)AUser;
	if(current_stream){
		*ALength = bassfunc->file.GetPos(current_stream->f,BASS_FILEPOS_END);
		return tak_True;
	}
	return tak_False;
}
void Tak_Damaged(void * AUser,PtakSSDDamageItem item){ // `called on every occurrence of a data error.' Currently, nothing to do.
	return ;
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