QWORD WINAPI TAK_GetLength(void *inst, DWORD mode);
const char *WINAPI TAK_GetTags(void* inst, DWORD tags);
void WINAPI TAK_GetInfo(void* inst, BASS_CHANNELINFO* info);
DWORD WINAPI TAK_SetFlags(HSTREAM handle, DWORD flags);
QWORD WINAPI TAK_GetPosition(void *inst, QWORD pos, DWORD mode);
BOOL WINAPI TAK_CanSetPosition(void *inst, QWORD pos, DWORD mode);
// called by BASS_ChannelSetPosition
// called after the above function - do the actual seeking
// return the actual resulting position
QWORD WINAPI TAK_SetPosition(void *inst, QWORD pos, DWORD mode);
void WINAPI TAK_Free(void* inst);
int TAK_OpenFile (TAKSTREAM *tak);