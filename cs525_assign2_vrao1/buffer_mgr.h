#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "dberror.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
  RS_FIFO = 0,
  RS_LRU = 1,
  RS_CLOCK = 2,
  RS_LFU = 3,
  RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

// Page Handle in Memory
typedef struct BM_PageHandle {
  PageNumber pageNum;
  char *data;
} BM_PageHandle;

typedef struct _fifoNode FIFO_Node;

// Page Frame Structure Definition
typedef struct _PageFrame{
	FIFO_Node *id;
  	PageNumber pageNum;
	unsigned short FixCount;
	bool dirtyFlag;
	char data[PAGE_SIZE];
}BM_PageFrame;

// Structure for FIFO
struct _fifoNode{
	BM_PageFrame *addr;
	struct _fifoNode *next;
	struct _fifoNode *prev;
};

// Virtual Function for page replacement can be assigned to either FIFO,LRU etc
typedef RC (*PageReplacementAlgo)(FIFO_Node **,FIFO_Node **,BM_PageFrame **,BM_PageFrame ** ,unsigned int);

// mgmtData structure to store Bookkeeping information of particular Buffer Pool
typedef struct _bufferPoolInfo{
	PageReplacementAlgo policy;
	FIFO_Node *front, *rear;
	BM_PageFrame **pagePresenceRegister;
	BM_PageFrame *pagePool;
	unsigned int *vacantFrameStack;
	unsigned int top;
	unsigned int poolSize;
	unsigned int currentPageCount;
} BufferPoolInfo;

// Buffer Pool Structure
typedef struct BM_BufferPool {
  char *pageFile;
  int numPages;
  ReplacementStrategy strategy;
  BufferPoolInfo *mgmtData; // use this one to store the bookkeeping info your buffer 
                  // manager needs for a buffer pool
} BM_BufferPool;

// convenience macros
#define MAKE_POOL()					\
  ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
  ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

// Page Replacement Strategy Functions
RC push(BM_BufferPool *bm,int i);
RC pop(BM_BufferPool *bm,int *i);
RC FIFO_Policy(FIFO_Node **front, FIFO_Node **rear, BM_PageFrame ** victim, BM_PageFrame ** hash ,unsigned int whatToDo);
//BM_PageFrame * LRU_Policy(BM_PageFrame * head,BM_PageFrame *tail,unsigned whatToDo);
#endif
