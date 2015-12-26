#include <stdint.h>
#include "TakStream.h"
#pragma comment(lib,"tak_deco_lib.lib")
#include "ioif.h"
#include "takfunc.h"

TakStream::TakStream(DWORD flags){
	m_flags = flags;
	m_takDecoder = NULL;
	m_decodeBuf = NULL;
	m_workBufLen = 0;
	m_apeTagExtracted = false;
	m_id3TagExtracted = false;
	m_id3v2Tag = NULL;
	m_apeTag = NULL;
	memset(m_id3v1Tag, 0x00, sizeof(m_id3v1Tag));
}

TakStream::~TakStream()
{
	tak_SSD_Destroy(m_takDecoder);
	HeapFree(GetProcessHeap(),0,m_decodeBuf);
	HeapFree(GetProcessHeap(),0,m_apeTag);
	HeapFree(GetProcessHeap(),0,m_id3v2Tag);
}

HSTREAM TakStream::openDecoder(BASSFILE f)
{
	int ret;
	TtakSSDOptions Options;
	Options.Cpu = tak_Cpu_Any;
	Options.Flags = 0;

	m_bassFile = f;

	if((m_takDecoder = tak_SSD_Create_FromStream(&IOIF,m_bassFile,&Options,NULL,this)) == NULL){ return NULL; }
	if(tak_SSD_Valid(m_takDecoder) != tak_True){ return NULL; }
	if((ret = tak_SSD_GetStreamInfo(m_takDecoder,&m_info)) != tak_res_Ok){ return NULL; }

	tak_SSD_Seek(m_takDecoder,0);
	
	HSTREAM handle = bassfunc->CreateStream((DWORD)m_info.Audio.SampleRate, m_info.Audio.ChannelNum
		, m_flags, &streamProc, this, &TAKfuncs);
	if (!handle) {
		return NULL;
	}
	bassfunc->file.SetStream(m_bassFile,handle); // set the stream associated with the file

	return handle;
}

QWORD TakStream::getLength()
{
	return m_info.Sizes.SampleNum * getBytesPerSample();
}

QWORD TakStream::getPosition()
{
	return tak_SSD_GetReadPos(m_takDecoder) * getBytesPerSample();
}

bool TakStream::setPosition(QWORD samplePos)
{
	return tak_SSD_Seek(m_takDecoder, samplePos / getBytesPerSample()) == tak_True ? true : false;
}

long TakStream::decode(long desiredSamples){
	TtakInt32 decoded;

	int bytePerSample = m_info.Audio.SampleBits / 8 * m_info.Audio.ChannelNum; // eg 16bit stereo -> 4
	long desiredBytes = desiredSamples * bytePerSample;

	if(m_workBufLen < desiredBytes){
		if(m_decodeBuf == NULL){
			m_decodeBuf = (BYTE*)HeapAlloc(GetProcessHeap(),0, desiredBytes);
		} else {
			m_decodeBuf = (BYTE*)HeapReAlloc(GetProcessHeap(), 0, m_decodeBuf, desiredBytes);
		}
		if(m_decodeBuf == NULL){
			m_workBufLen = 0;
			throw "cannnot alloc buffer";
		}
		m_workBufLen = desiredBytes;
	}

	if(tak_SSD_ReadAudio(m_takDecoder, m_decodeBuf, desiredSamples, &decoded) != tak_res_Ok) return -1;
	if(decoded == -1) return -1;
	return decoded;
}

DWORD TakStream::getFileFlags()
{
	return bassfunc->file.GetFlags(m_bassFile);
}

bool TakStream::isFloating()
{
	return (m_flags & BASS_SAMPLE_FLOAT) != 0;
}

void *TakStream::getApeTag()
{
	if(!m_apeTagExtracted){
		extractApeTag();
		m_apeTagExtracted = true;
	}
	return m_apeTag;
}

void *TakStream::getID3V1Tag()
{
	if(!m_id3TagExtracted){
		extractID3Tags();
		m_id3TagExtracted = true;
	}
	return m_id3v1Tag;
}

void *TakStream::getID3V2Tag()
{
	if(!m_id3TagExtracted){
		extractID3Tags();
		m_id3TagExtracted = true;
	}
	return m_id3v2Tag;
}

int setID3V24Frame(BYTE* buf, int charcode, const char* frameId, const BYTE*prefix, int prefixLen, const BYTE* value, int valueLen){
	memcpy(buf, frameId, 4);
	if(charcode == -1){
		memcpy(buf + 10, prefix, prefixLen);
		memcpy(buf + 10 + prefixLen, value, valueLen);
	}else{
		buf[10] = charcode; // charcode
		memcpy(buf + 11, prefix, prefixLen);
		memcpy(buf + 11 + prefixLen, value, valueLen);
		valueLen++;
	}
	valueLen += prefixLen;
	buf[4] = (valueLen>>21) & 0x7F;
	buf[5] = (valueLen>>14) & 0x7F;
	buf[6] = (valueLen>>7) & 0x7F;
	buf[7] = valueLen & 0x7F;
	buf[8] = 0; // status flag;
	buf[9] = 0; // format flag;
	return 10 + valueLen;
}

#define TAG_VALUE_SIZE 1024
void TakStream::extractID3Tags(){
	char key[64]; // tagName(key)
	int readed; // readed bytes of UTF8 value
	int readedAnsi; // readed bytes of Ansi value
	int tagsize = 0; // current V2 tag size

	TtakAPEv2Tag tag = tak_SSD_GetAPEv2Tag(m_takDecoder);
	if(tak_APE_Valid(tag) != tak_True) return;

	int len = tak_APE_GetItemNum(tag);

	// init V1 Tag
	memcpy_s(m_id3v1Tag, 3, "TAG", 3);

	tagsize = 10; // initial v2.4 tag size
	m_id3v2Tag = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tagsize); //calloc(1,10); // init V2.4 Tag
	if (m_id3v2Tag == NULL) goto end;

	BYTE* value = (BYTE*)HeapAlloc(GetProcessHeap(), 0, TAG_VALUE_SIZE);
	if (value == NULL) goto end;

	char* valueAnsi = (char*)HeapAlloc(GetProcessHeap(), 0, TAG_VALUE_SIZE);
	if (valueAnsi == NULL) goto end;

	memcpy_s(m_id3v2Tag, 3, "ID3", 3);
	m_id3v2Tag[3] = 0x04; //Version
	for(int i=0; i<len; i++){
		if (tak_APE_GetItemKey(tag, i, key, sizeof(key), &readed) != tak_res_Ok) continue;

		const ID3TAGTEMPLETE *tmpl = NULL;
		for (int j=0; j < sizeof(gWellKnownID3Tags) / sizeof(gWellKnownID3Tags[0]); j++) {
			if (strcmp(key, gWellKnownID3Tags[j].asApeTag) != 0) continue;
			tmpl = &(gWellKnownID3Tags[j]);
			break;
		}
		if (tmpl == NULL) continue;
		if (tak_APE_GetItemValue(tag, i, NULL, 0, &readed) != tak_res_Ok) continue; // get Bytes
		if (readed > TAG_VALUE_SIZE){
			value = (BYTE*)HeapReAlloc(GetProcessHeap(), 0, value, readed);
			if (value == NULL) goto end;
		}
		if (tak_APE_GetItemValue(tag, i, value, readed, &readed) != tak_res_Ok) continue;

		if (tak_APE_GetTextItemValueAsAnsi(tag, i, -1, ' ', NULL, 0, &readedAnsi) != tak_res_Ok) continue; // get bytes
		if(readedAnsi > TAG_VALUE_SIZE){
			valueAnsi = (char*)HeapReAlloc(GetProcessHeap(), 0, valueAnsi, readedAnsi);
			if (valueAnsi == NULL) goto end;
		}
		if (tak_APE_GetTextItemValueAsAnsi(tag, i, -1, ' ', valueAnsi, readedAnsi, &readedAnsi) != tak_res_Ok) continue;

		// for v2
		m_id3v2Tag = (BYTE*)HeapReAlloc(GetProcessHeap(), 0, m_id3v2Tag, tagsize + 11 + tmpl->id3_24valuePrefixLen + readed);
		if (m_id3v2Tag == NULL) goto end;
		tagsize += setID3V24Frame(m_id3v2Tag + tagsize, tmpl->charCode, tmpl->id3_24frameKey, (BYTE*)tmpl->id3_24valuePrefix, tmpl->id3_24valuePrefixLen, value, readed);

		// for v1
		if (strcmp(tmpl->id3_24frameKey, "TRCK") == 0) {
			// Do special process for TRCK frame
			m_id3v1Tag[tmpl->id3_1_pos] = atoi(valueAnsi);
		} else {
			memcpy(m_id3v1Tag + tmpl->id3_1_pos, valueAnsi, min(readedAnsi, tmpl->id3_1_len));
		}
	}
	tagsize -= 10;
	m_id3v2Tag[9] = tagsize & 0x7F;
	m_id3v2Tag[8] = (tagsize>>7) & 0x7F;
	m_id3v2Tag[7] = (tagsize>>14) & 0x7F;
	m_id3v2Tag[6] = (tagsize>>21) & 0x7F;
end:
	if (value) HeapFree(GetProcessHeap(),0,value);
	if (valueAnsi) HeapFree(GetProcessHeap(),0,valueAnsi);
}

void TakStream::extractApeTag(){
	BYTE apeTagFooterBuf[APE_TAG_FOOTER_BYTES] = {0};
	QWORD fileSize; // tak file size
	DWORD tagSize; // tagSize = apeTagBody + apeTagFooter[32]

	QWORD backup = bassfunc->file.GetPos(m_bassFile,BASS_FILEPOS_CURRENT);
	fileSize = bassfunc->file.GetPos(m_bassFile,BASS_FILEPOS_END);
	bassfunc->file.Seek(m_bassFile,fileSize - APE_TAG_FOOTER_BYTES);
	bassfunc->file.Read(m_bassFile,apeTagFooterBuf,APE_TAG_FOOTER_BYTES);
	if(memcmp(apeTagFooterBuf,"APETAGEX",8)==0){
		tagSize = (apeTagFooterBuf[15]<<24)+(apeTagFooterBuf[14]<<16)+(apeTagFooterBuf[13]<<8)+apeTagFooterBuf[12];
		if(tagSize >= (1024 * 1024 * 16)){ // too large
			return;
		}
		m_apeTag = (BYTE*)HeapAlloc(GetProcessHeap(),0,tagSize+2); // tagbody + terminator(NULL NULL)
		if(m_apeTag != NULL){
			// extract tag body(key, value)
			bassfunc->file.Seek(m_bassFile,fileSize - tagSize);
			bassfunc->file.Read(m_bassFile,m_apeTag,tagSize);
			// set terminator
			m_apeTag[tagSize-1] = 0;
			m_apeTag[tagSize-2] = 0;
		}
	}
	bassfunc->file.Seek(m_bassFile, backup);
}

inline int TakStream::getBytesPerSample()
{
	return (isFloating() ? 4 : 2) * m_info.Audio.ChannelNum; //stream->info.BPS/8
}

DWORD TakStream::streamProcIntl(BYTE* buffer, DWORD length)
{
	int decodedSampleNum;

	try{
		decodedSampleNum = decode(length / getBytesPerSample());
	}catch(...){
		errorn(BASS_ERROR_MEM);
	}
		
	if(decodedSampleNum < 0){
		noerrorn(BASS_STREAMPROC_END);
	}

	long elementNum = decodedSampleNum * m_info.Audio.ChannelNum;

	if(isFloating()){
		bassfunc->data.Int2Float(m_decodeBuf,(float*)buffer, elementNum, m_info.Audio.SampleBits / 8);
	}else{
		convertSampleTo16bit(buffer, m_decodeBuf, elementNum, m_info.Audio.SampleBits);
	}
	noerrorn(decodedSampleNum * getBytesPerSample());
}

DWORD CALLBACK TakStream::streamProc(HSTREAM handle, void* buffer, DWORD length, void *stream)
{
	return ((TakStream*)stream)->streamProcIntl((BYTE*)buffer, length);
}

void TakStream::convertSampleTo16bit(BYTE *dest, const BYTE *src, int samples, int srcBits)
{
	switch(srcBits){
	case 8: // 8bit -> 16bit
		while(samples--){
			*((signed short*)dest) = (((signed int)(*src++))-128) * 0x100;
			dest+=2;
		}
		break;
	case 16: // 16bit -> 16bit
		memcpy(dest,src,samples*sizeof(uint16_t));
		break;
	case 24: // 24bit -> 16bit
		while(samples--){ // tested with http://trendy.nikkeibp.co.jp/article/news/20060419/116354/
			src++;
			*dest++=*src++;
			*dest++=*src++;
		}
		break;
	case 32: // 32bit -> 16bit not tested.
		while(samples--){
			src+=2;
			*dest++=*src++;
			*dest++=*src++;
		}
		break;
	}
}
