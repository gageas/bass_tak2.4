// TAK IO functions
TtakBool CanRead(void * AUser);
TtakBool CanWrite(void * AUser);
TtakBool CanSeek(void * AUser);
TtakBool Read(void * AUser, void *      ABuf, TtakInt32   ANum, TtakInt32 * AReadNum );
TtakBool Write(void * AUser,const void * ABuf,TtakInt32    ANum);
TtakBool Flush(void * AUser);
TtakBool Truncate(void * AUser);
TtakBool Seek(void * AUser, TtakInt64 APos);
TtakBool GetLength(void * AUser,TtakInt64 * ALength);
void Tak_Damaged(void * AUser,PtakSSDDamageItem item);
