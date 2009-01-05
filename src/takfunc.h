extern ADDON_FUNCTIONS TAKfuncs;

bool TAK_OpenFile (TAKSTREAM *tak);
bool TAK_ReadAudio(TAKSTREAM *stream,void* readBuf, long readSamples, int* readedSampleCount);