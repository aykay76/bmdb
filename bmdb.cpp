#include <windows.h>

#include "bmdbdefs.h"

#define GetFilePointer(hFile) SetFilePointer(hFile, 0, NULL, FILE_CURRENT)

BMDBError g_BMDBError;

void AddIndexToList(BMDBFileInfo *pInfo, BMDBIndex *pItem)
{
	BMDBIndexListItem *pCurrent;

	if (pInfo->pFirst == NULL) {
		pInfo->pFirst = new BMDBIndexListItem;
		pInfo->pFirst->pData = pItem;
		pInfo->pFirst->pNext = NULL;
	} else {
		pCurrent = new BMDBIndexListItem;
		pCurrent->pData = pItem;
		pCurrent->pNext = pInfo->pFirst;
		pInfo->pFirst = pCurrent;
	}
}

void *BMDBOpen(char *szName, int nMode)
{
	BMDBFileInfo *pInfo;
	DWORD dwAccess = 0;
	DWORD dwCreation = 0;
	DWORD dwShareMode;

	pInfo = new BMDBFileInfo;
	pInfo->pFirst = NULL;

	dwCreation = OPEN_ALWAYS;
	dwShareMode = FILE_SHARE_READ;
	dwAccess = GENERIC_READ | GENERIC_WRITE;

	strcpy(pInfo->szName, szName);

	strcpy(pInfo->szIndexFilename, szName);
	strcat(pInfo->szIndexFilename, ".IDX");
	pInfo->hIndexFile = CreateFile(pInfo->szIndexFilename, dwAccess, dwShareMode, NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pInfo->hIndexFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	strcpy(pInfo->szDataFilename, szName);
	strcat(pInfo->szDataFilename, ".DAT");
	pInfo->hDataFile = CreateFile(pInfo->szDataFilename, dwAccess, dwShareMode, NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pInfo->hDataFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	DWORD dwIndexCount;
	DWORD dwBytesRead;
	BMDBIndex *pData;

	dwIndexCount = 0;
	ReadFile(pInfo->hIndexFile, &dwIndexCount, sizeof(DWORD), &dwBytesRead, NULL);

	for (DWORD dwLoop = 0; dwLoop < dwIndexCount; dwLoop++) {
		pData = new BMDBIndex;
		ReadFile(pInfo->hIndexFile, &pData->dwSize, sizeof(DWORD), &dwBytesRead, NULL);
		pData->ptr = new unsigned char[pData->dwSize + 1];
		ReadFile(pInfo->hIndexFile, pData->ptr, pData->dwSize, &dwBytesRead, NULL);
		pData->ptr[pData->dwSize] = 0;
		ReadFile(pInfo->hIndexFile, &pData->dwPos, sizeof(DWORD), &dwBytesRead, NULL);

		AddIndexToList(pInfo, pData);
	}

	pInfo->bChanges = FALSE;

	return pInfo;
}

void BMDBClose(void *pInfo)
{
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);

	if (pFileInfo->bChanges == TRUE) {
		BMDBReorganise(pFileInfo);
	}

	CloseHandle(pFileInfo->hIndexFile);
	CloseHandle(pFileInfo->hDataFile);

	BMDBIndexListItem *pCurrent;
	pCurrent = pFileInfo->pFirst;
	while (pCurrent) {
		BMDBIndexListItem *pTemp = pCurrent;
		pCurrent = pCurrent->pNext;

		delete pTemp->pData->ptr;
		delete pTemp->pData;
		delete pTemp;
	}

	delete pFileInfo;
	pInfo = pFileInfo = NULL;
}

int BMDBStore(void *pInfo, BMDBIndex key, BMDBRecord content, int flags)
{
	DWORD dwBytesWritten;
	DWORD dwFilePos;
	BMDBIndex index;
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);

	pFileInfo->bChanges = TRUE;

	index = BMDBExists(pFileInfo, key);
	if (index.ptr) {
		BMDBDelete(pFileInfo, key);
	}

	SetFilePointer(pFileInfo->hDataFile, 0, 0, FILE_END);

	dwFilePos = GetFilePointer(pFileInfo->hDataFile);
	WriteFile(pFileInfo->hDataFile, &content.size, sizeof(DWORD), &dwBytesWritten, NULL);
	WriteFile(pFileInfo->hDataFile, content.ptr, content.size, &dwBytesWritten, NULL);

	BMDBIndex *pItem = new BMDBIndex;

	pItem->dwPos = dwFilePos;
	pItem->dwSize = key.dwSize;
	pItem->ptr = new unsigned char[key.dwSize];
	memcpy(pItem->ptr, key.ptr, key.dwSize);

	AddIndexToList(pFileInfo, pItem);

	return 1;
}

int BMDBFetch(void *pInfo, BMDBIndex key, BMDBRecord *pRecord)
{
	DWORD dwBytesRead;
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);

	SetFilePointer(pFileInfo->hDataFile, key.dwPos, NULL, FILE_BEGIN);
	ReadFile(pFileInfo->hDataFile, &(pRecord->size), sizeof(DWORD), &dwBytesRead, NULL);
	pRecord->ptr = new unsigned char[pRecord->size];
	ReadFile(pFileInfo->hDataFile, pRecord->ptr, pRecord->size, &dwBytesRead, NULL);

	return 1;
}

int BMDBDelete(void *pInfo, BMDBIndex key)
{
	BMDBIndexListItem *pCurrent;
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);
	BMDBIndexListItem *pPrev;

	pFileInfo->bChanges = TRUE;

	pCurrent = pFileInfo->pFirst;
	pPrev = NULL;
	while (pCurrent) {
		if (memcmp(key.ptr, pCurrent->pData->ptr, key.dwSize) == 0) {
			// if this is the first record, we need a new header for this list
			if (pPrev == NULL) {
				pFileInfo->pFirst = pCurrent->pNext;
			} else {
				pPrev->pNext = pCurrent->pNext;
			}

			// delete the current record
			delete [] pCurrent->pData->ptr;
			delete pCurrent->pData;
			delete pCurrent;

 			return 1;
		}

		pPrev = pCurrent;
		pCurrent = pCurrent->pNext;
	}

	return 0;
}

BMDBIndex BMDBFirstkey(void *pInfo)
{
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);
	if (pFileInfo->pFirst) {
		return *pFileInfo->pFirst->pData;
	} else {
		BMDBIndex index;

		index.ptr = NULL;
		index.dwSize = 0;
		index.dwPos = -1;
		return index;
	}
}

BMDBIndex BMDBNextkey(void *pInfo, BMDBIndex key)
{
	BMDBIndex ReturnData;
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);

	ReturnData.ptr = NULL;
	ReturnData.dwSize = 0;

	BMDBIndexListItem *pCurrent;

	for (pCurrent = pFileInfo->pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext) {
		if (pCurrent != NULL) {
			if (pCurrent->pData->ptr == key.ptr) {
				pCurrent = pCurrent->pNext;
				if (pCurrent) {
					return *pCurrent->pData;
				} else {
					return ReturnData;
				}
			}
		}
	}

	return ReturnData;
}

int BMDBReorganise(void *pInfo)
{
	DWORD dwBytesWritten;
	HANDLE hTempFile;
	char szFilename[MAX_PATH];
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);
	BMDBIndexListItem *pCurrent;
	DWORD dwCount = 0;
	BMDBRecord record;

	strcpy(szFilename, pFileInfo->szName);
	strcat(szFilename, ".TMP");
	hTempFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	for (pCurrent = pFileInfo->pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext) {
		BMDBFetch(pFileInfo, *(pCurrent->pData), &record);

		SetFilePointer(hTempFile, 0, 0, FILE_END);

		pCurrent->pData->dwPos = GetFilePointer(hTempFile);
		WriteFile(hTempFile, &record.size, sizeof(DWORD), &dwBytesWritten, NULL);
		WriteFile(hTempFile, record.ptr, record.size, &dwBytesWritten, NULL);

		delete record.ptr;
	}

	CloseHandle(hTempFile);
	CloseHandle(pFileInfo->hDataFile);
	CopyFile(szFilename, pFileInfo->szDataFilename, FALSE);
	DeleteFile(szFilename);

	pFileInfo->hDataFile = CreateFile(pFileInfo->szDataFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	SetFilePointer(pFileInfo->hIndexFile, 0, 0, FILE_BEGIN);

	for (pCurrent = pFileInfo->pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext) {
		dwCount++;
	}
	WriteFile(pFileInfo->hIndexFile, &dwCount, sizeof(DWORD), &dwBytesWritten, NULL);

	for (pCurrent = pFileInfo->pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext) {
		WriteFile(pFileInfo->hIndexFile, &pCurrent->pData->dwSize, sizeof(DWORD), &dwBytesWritten, NULL);
		WriteFile(pFileInfo->hIndexFile, pCurrent->pData->ptr, pCurrent->pData->dwSize, &dwBytesWritten, NULL);
		WriteFile(pFileInfo->hIndexFile, &pCurrent->pData->dwPos, sizeof(DWORD), &dwBytesWritten, NULL);
	}

	return 1;
}

// does this key exist?
BMDBIndex BMDBExists(void *pInfo, BMDBIndex key)
{
	BMDBIndexListItem *pCurrent;
	BMDBFileInfo *pFileInfo = reinterpret_cast<BMDBFileInfo *>(pInfo);

	for (pCurrent = pFileInfo->pFirst; pCurrent != NULL; pCurrent = pCurrent->pNext) {
		if (pCurrent->pData->dwSize == key.dwSize) {
			if (memcmp(pCurrent->pData->ptr, key.ptr, key.dwSize) == 0) {
				return *(pCurrent->pData);
			}
		}
	}

	BMDBIndex index;
	index.dwPos = 0;
	index.dwSize = 0;
	index.ptr = 0;

	return index;
}
