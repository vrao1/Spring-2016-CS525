#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "dberror.h"
#include<limits.h>

/************************************************************
 *                    handle data structures                *
 ************************************************************/

//Structure to store file pointer and descriptor

struct _mgmtInfo{
	int fd;
	FILE *ptr;
};

//Structure contains the current context of each file being maintained by Storage Manager

struct _SM_FileHandle {
  char *fileName;
  int totalNumPages;
  int curPagePos;
  struct _mgmtInfo mgmtInfo;
};

typedef struct _SM_FileHandle SM_FileHandle; 
typedef char* SM_PageHandle;

// A node of doubly link list consists of an object SM_FileHandle and two pointers to the same data type
struct _allFiles{
        SM_FileHandle fh;
        struct _allFiles *next;
        struct _allFiles *prev;
};


/************************************************************
 *                    interface                             *
 ************************************************************/
/* manipulating page files */
extern void initStorageManager (void);
extern RC createPageFile (char *fileName);
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle);
extern RC closePageFile (SM_FileHandle *fHandle);
extern RC destroyPageFile (char *fileName);
extern RC copyFileHandle (SM_FileHandle *destFHandle, SM_FileHandle *srcFHandle);

/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
extern int getBlockPos (SM_FileHandle *fHandle);
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

/* writing blocks to a page file */
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC appendEmptyBlock (SM_FileHandle *fHandle);
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle);

#endif
