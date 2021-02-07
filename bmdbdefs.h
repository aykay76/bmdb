////////////////////////////////////////////////////////////////////////////////
//
// File:			BMDBDEFS.H
//
// Description:		Some internal definitions used by the library
//
// Author:			Alan Kelly 
//
////////////////////////////////////////////////////////////////////////////////

#if !defined BMDBDEFS_H
#define BMDBDEFS_H

#include "BMDBConst.h"

typedef struct {
	unsigned char *ptr;
	int size;
} BMDBRecord;

typedef struct {
	unsigned char *ptr;
	DWORD dwSize;
	DWORD dwPos;
} BMDBIndex;

typedef struct tagBMDBIndexListItem{
	BMDBIndex* pData;
	tagBMDBIndexListItem *pNext;
} BMDBIndexListItem;

typedef struct {
	HANDLE hIndexFile;
	HANDLE hDataFile;
	BOOL bChanges;
	BMDBIndexListItem *pFirst;
	char szName[MAX_PATH];
	char szDataFilename[MAX_PATH];
	char szIndexFilename[MAX_PATH];
} BMDBFileInfo, *HBMDB;

extern HBMDB BMDBOpen(char *szName, int nBlockSize, int nFlags, int nMode);
extern void BMDBClose(void * dbf);
extern int BMDBStore(void * dbf, BMDBIndex key, BMDBRecord content, int flags);
extern int BMDBFetch(void * dbf, BMDBIndex key, BMDBRecord *pRecord);
extern int BMDBDelete(void * dbf, BMDBIndex key);
extern BMDBIndex BMDBFirstkey(void * dbf);
extern BMDBIndex BMDBNextkey(void * dbf, BMDBIndex key);
extern int BMDBReorganise(void * dbf);
extern BMDBIndex BMDBExists(void * dbf, BMDBIndex key);

#endif