#if !defined BMDB_H
#define BMDB_H

#include "BMDBConst.h"

typedef HANDLE HBMDB;

typedef struct {
	unsigned char *ptr;
	int size;
} BMDBRecord;

typedef struct {
	unsigned char *ptr;
	DWORD dwSize;
	DWORD dwPos;
} BMDBIndex;

extern HBMDB BMDBOpen(char *szName, int nMode);
extern void BMDBClose(HBMDB dbf);
extern int BMDBStore(HBMDB dbf, BMDBIndex key, BMDBRecord content, int flags);
extern int BMDBFetch(HBMDB dbf, BMDBIndex key, BMDBRecord *pRecord);
extern int BMDBDelete(HBMDB dbf, BMDBIndex key);
extern BMDBIndex BMDBFirstkey(HBMDB dbf);
extern BMDBIndex BMDBNextkey(HBMDB dbf, BMDBIndex key);
extern int BMDBReorganise(HBMDB dbf);
extern BMDBIndex BMDBExists(HBMDB dbf, BMDBIndex key);
extern int BMDBSetopt(HBMDB dbf, int optflag, int *optval, int optlen);

extern DWORD dwBMDBError;

#endif