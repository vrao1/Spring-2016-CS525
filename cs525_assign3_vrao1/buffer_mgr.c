#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<errno.h>
#include<stdlib.h>
#include <string.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"

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
  int lastIndex;	
  char *data;
} BM_PageHandle;

//List Node Structure
typedef struct _listNode List_Node;

// Page Frame Structure Definition
typedef struct _PageFrame{
	List_Node *id;
  	PageNumber pageNum;
	unsigned short FixCount;
	bool dirtyFlag;
	int recordIndicesValidLastIndex;
	int recordIndices[512];
	char data[PAGE_SIZE*2];
}BM_PageFrame;


// Structure for RS
struct _listNode{
	BM_PageFrame *addr;
	struct _listNode *next;
	struct _listNode *prev;
};

// Virtual Function for page replacement can be assigned to either FIFO,LRU etc
typedef RC (*PageReplacementAlgo)(List_Node **,List_Node **,BM_PageFrame **,BM_PageFrame ** ,unsigned int);

// mgmtData structure to store Bookkeeping information of particular Buffer Pool
typedef struct _bufferPoolInfo{
	PageReplacementAlgo policy;
	List_Node *front, *rear;
	BM_PageFrame **pagePresenceRegister;
	BM_PageFrame *pagePool;
	unsigned int *vacantFrameStack;
	unsigned int top;
	unsigned int poolSize;
	unsigned int currentPageCount;
	unsigned int readIO;
	unsigned int writeIO;
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

// Vacant Page Frame Activities

// Push vacant Frame Index

/*******************************************************************************
 * Function Name: push
 *
 * Description: Add a page frame index to the vacant frame stack of a buffer
 *      pool.
 *
 * Patameters:
 *      BM_BufferPool *bm - Buffer pool who's vacant frame stack is updated.
 *		int i - Index of a page frame to add.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_STACK_FULL
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/

RC push(BM_BufferPool *bm,int i){
	if(bm->mgmtData->top == bm->mgmtData->poolSize-1){
		RC_message = "Stack is Full ; No Addition is Allowed";
		return RC_STACK_FULL;
	}

	bm->mgmtData->top++;
	bm->mgmtData->vacantFrameStack[bm->mgmtData->top] = i;
	return RC_OK;		
}

// Pop vacant Frame index

/*******************************************************************************
 * Function Name: pop
 *
 * Description: Get a page frame index from the vacant frame stack of a buffer
 *      pool.
 *
 * Patameters:
 *      BM_BufferPool *bm - Buffer pool who's vacant frame stack is accessed.
 *		int *i - Index of the page frame to get.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_STACK_EMPTY
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/

RC pop(BM_BufferPool *bm,int *i){
	if(bm->mgmtData->top == -1){
		RC_message = "Stack is Empty ; No POP is Allowed";
		return RC_STACK_EMPTY;
	}

	*i = bm->mgmtData->vacantFrameStack[bm->mgmtData->top];
	bm->mgmtData->top--;
	
	return RC_OK;		
}

// Page Replacement Strategy Functions

/*******************************************************************************
 * Function Name: RS_Policy
 *
 * Description: Handles replacement strategy management functionality.
 * 		FIND_VICTIM_PAGE - Handles evicting a page from a frame.
 *		INSERT_PAGE - Handles loading a page into a frame.
 *		UPDATE_USAGE - Handles tracking recent usage.
 *		DESTRY_QUEUE - Resets management.
 *
 * Patameters:
 *      List_Node **front - Front node of a buffer pool.
 *		List_Node **rear - Rear node of a buffer pool.
 *		BM_PageFrame **victim - Page frame being evicted.
 *		BM_PageFrame **pagePresenceRegister - Precense Register for file pages.
 *		unsigned int whatToDo - Indicates which manaement action to perform.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NO_FRONT_END
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Updates for LRU
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/

RC RS_Policy(List_Node **front, List_Node **rear,BM_PageFrame **victim, BM_PageFrame **pagePresenceRegister, unsigned int whatToDo){

	List_Node *temp;

	switch(whatToDo){
		case FIND_VICTIM_PAGE:
			if(*front == (List_Node *)0){
				RC_message = "Queue is still empty";
				return RC_NO_FRONT_END;
			}

                        temp = *front;
			while (temp != *rear) {
				if (temp->addr->FixCount == 0) {
        	                        temp->prev->next = temp->next;
                	                if (temp == *front) { 
						*front = temp->prev;
						temp->prev->next = (List_Node *)0;
					} else { 
						temp->next->prev = temp->prev;
						temp->prev->next = temp->next;
					 }
				break;
				}
				temp = temp->prev;

			}
			if (temp == *rear) {
				if (temp->addr->FixCount == 0) {
					temp->next->prev = (List_Node *)0; 
				} else {
					*victim = (BM_PageFrame *)0;
					return RC_NO_FRONT_END;

				}
			}
			
			*victim = temp->addr;


			free(temp);
			break;
		case INSERT_PAGE:
			temp = (List_Node *) calloc(1,sizeof(List_Node));
			temp->addr = *victim;
			if(*rear == (List_Node *)0){
				temp->next = (List_Node *)0;
				temp->prev = (List_Node *)0;
				*rear = *front = temp;
			}else{
				temp->next = *rear;
				(*rear)->prev = temp;
				*rear = temp;
			}

			pagePresenceRegister[(*victim)->pageNum]->id = temp;	
			break;
		case UPDATE_USAGE:
			if(pagePresenceRegister[(*victim)->pageNum]->id != *rear){ //If it's already the rear no need to do anything.
				
				temp = pagePresenceRegister[(*victim)->pageNum]->id;
				
				if(temp == *front){ //IF it's the front, make it's previous the front.
					temp->prev->next = (List_Node *)0; 
					*front = temp->prev;
				} else { //Point it's previous to it's next and it's next to it's previous.
					temp->prev->next = temp->next;
					temp->next->prev = temp->prev;
				}
				//Make it the tail.
				temp->prev = (List_Node *)0;
				temp->next = *rear;
				*rear = temp;
				temp->next->prev = temp;
			}
			break;
		case DESTROY_QUEUE:
			if(*rear == (List_Node *)0){
				RC_message = "Queue is still empty";
				return RC_NO_FRONT_END;
			}

			do{
				temp = *rear; 
				*rear = (*rear)->next;
				free(temp);
				temp = (List_Node *)0;
			}while(*rear != (List_Node *)0);

			*front = *rear = (List_Node *)0;

			break;
		default:
			break;
	}
	return RC_OK;
}

// Buffer Manager Interface Pool Handling

/*******************************************************************************
 * Function Name: initBufferPool
 *
 * Description: Creates a new buffer pool.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool created.
 *		const char *const pageFileName - Filename of page file the pool will be
 *			used for.
 *		const int numPages - Number of page frames in the pool.
 *		ReplacementStrategy strategy - Strategy for replacing pages in frames.
 *		void *stratData - Used for strategy specific data.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *		RC_DYNAMIC_MEM_ALLOC_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Updates for LRU
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData)
{
	if(bm == ((BM_BufferPool *)0) || pageFileName == ((char *)0) || strlen(pageFileName) == 0
             || numPages < 1 || strategy > RS_LRU_K || strategy < RS_FIFO ){
		RC_message = "Passed Argument(s) are not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}

	BufferPoolInfo *poolMetaInfo;

	bm->pageFile = (char *) malloc(strlen(pageFileName)+1);
	if( bm->pageFile == ((char *) 0) ){
                RC_message = "malloc failed to allocate memory for pageFileName";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
        }

	// Storing Page File Name,Number of frames,page replacement strategy into Buffer Pool

	strcpy(bm->pageFile,pageFileName);
	bm->numPages = numPages;
	bm->strategy = strategy;
	poolMetaInfo = (BufferPoolInfo *) malloc(sizeof(BufferPoolInfo));

	if( poolMetaInfo == ((BufferPoolInfo *) 0) ){
                RC_message = "malloc failed to allocate memory for BufferPoolInfo";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
        }

	// Allocating MAX_PAGE sized array of pointers to Page Frames in the memory 

	poolMetaInfo->pagePresenceRegister = (BM_PageFrame **) calloc (MAX_PAGE,sizeof(BM_PageFrame *));

	if( poolMetaInfo->pagePresenceRegister == ((BM_PageFrame **) 0) ){
                RC_message = "calloc failed to allocate memory for storing array of addresses of page frame of size MAX_PAGE";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
        }

	// Allocating an array of page frames in memory
	BM_PageFrame *framePool;
	
	framePool = (BM_PageFrame *) calloc (numPages,sizeof(BM_PageFrame));

	if( framePool == ((BM_PageFrame *) 0) ){
                RC_message = "calloc failed to allocate memory for page frame of size numPages";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
        }

	// Allocating Empty Page Frame Stack

	poolMetaInfo->vacantFrameStack = (unsigned int *) malloc (sizeof(unsigned int)*numPages);

	if( poolMetaInfo->vacantFrameStack == ((unsigned int *) 0) ){
                RC_message = "malloc failed to allocate memory for stack of vacant page frame of size numPages";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
        }

	// Pushing all vacant empty page frame indicies

	for(int i=0;i<numPages;i++){
		poolMetaInfo->vacantFrameStack[numPages-1-i] = i;
	}

	poolMetaInfo->top = numPages-1;

	// Assigning Page Replcement functionality at runtime (Polymorphism introduced here)

//	switch(strategy){
//		case RS_FIFO:
		 	poolMetaInfo->policy = RS_Policy;
			poolMetaInfo->front = (List_Node *)0;
			poolMetaInfo->rear = (List_Node *)0;
/*			break;
		case RS_LRU:
			//poolMetaInfo->PageReplacementAlgo = LRU_Policy;
			//break;
		case RS_CLOCK:
		case RS_LFU:
		case RS_LRU_K:
		default:
			return RC_STRATEGY_NOT_EXIST;
	}
*/
	// Assigning Initial Memory address of allocated pool of empty frames
	//
	for(int j=0; j < numPages ;j++){
		framePool[j].pageNum = -1;
	}

	poolMetaInfo->pagePool = framePool;

	// Initializing Total Page Frame Count to numPages
	poolMetaInfo->poolSize = numPages;

	// Initializing Current Page Count to zero
	poolMetaInfo->currentPageCount = 0;
	poolMetaInfo->readIO = 0;
	poolMetaInfo->writeIO = 0;

	// Finally Management Data pointer has been assigned to a structure of BufferPoolInfo containing all Meta information of this particular Buffer Pool

	bm->mgmtData = poolMetaInfo;

	return RC_OK;
}

/*******************************************************************************
 * Function Name: shutdownBufferPool
 *
 * Description: Destroyes a new buffer pool.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool to destroy.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *		RC_UNABLE_TO_SHUTDOWN
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/

RC shutdownBufferPool(BM_BufferPool *const bm){
	// Print an error if any page fix count is non zero
	// First write back all dirty page to disk
	// Delete all allocated memory till so far


	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}
	int i=0;
	bool hasDirtyFlag = false;

	// Scan for Non Zero Fix Count

	for(i=0;i < bm->mgmtData->currentPageCount;i++){
		if(0 != bm->mgmtData->pagePool[i].FixCount){
			RC_message = "Fix Count is non zero for a page frame; Hence unable to shutdown this buffer pool";
                	return RC_UNABLE_TO_SHUTDOWN;
		}

		if(bm->mgmtData->pagePool[i].dirtyFlag == true)
			hasDirtyFlag = true;
	}

	// Deallocating Memory for this Buffer Pool



	// Destroy vacantFrameStack

	if(bm->mgmtData->vacantFrameStack != ((unsigned int *)0)){
		free(bm->mgmtData->vacantFrameStack);
		bm->mgmtData->vacantFrameStack = (unsigned int *)0;
	}

	// Open Disk Page File in case of writing back of dirty block
	
	SM_FileHandle fh;

	if(hasDirtyFlag && openPageFile(bm->pageFile,&fh) != RC_OK){
		RC_message = "There is problem to open Page File to write a dirty Block back to the pageFile from this Buffer Pool ; hence unable to shutdown this Buffer Pool";
                return RC_UNABLE_TO_SHUTDOWN;
	}

	// Destroy pagePool

	if(bm->mgmtData->pagePool != ((BM_PageFrame *)0)){
		for(i=0; i < bm->mgmtData->poolSize; i++){
			if(bm->mgmtData->pagePresenceRegister[bm->mgmtData->pagePool[i].pageNum] && bm->mgmtData->pagePool[i].dirtyFlag == true){ 
				if(hasDirtyFlag && writeBlock(bm->mgmtData->pagePool[i].pageNum,&fh,(SM_PageHandle)(bm->mgmtData->pagePool[i].data)) != RC_OK){
					RC_message = "There is problem to write a dirty Block back to the pageFile from this Buffer Pool ; hence unable to shutdown this Buffer Pool";
               				return RC_UNABLE_TO_SHUTDOWN;
				}
				bm->mgmtData->writeIO++;
			}
		}

		// Freeing up pool of frame

		free(bm->mgmtData->pagePool);
		bm->mgmtData->pagePool = ((BM_PageFrame *)0);
	}

	// Destroy pagePresenceRegister
	
	if(bm->mgmtData->pagePresenceRegister != ((BM_PageFrame **)0)){
		free(bm->mgmtData->pagePresenceRegister);
		bm->mgmtData->pagePresenceRegister = (BM_PageFrame **)0;
	}

	// Destroy Buffer Pool pageFile

	free(bm->pageFile);
	bm->pageFile = (char *)0;

	// Closing Page File
	
	if(hasDirtyFlag && closePageFile(&fh) != RC_OK){
		RC_message = "There is problem to close Page File after shutting down Buffer Pool ; hence unable to shutdown this Buffer Pool";
                return RC_UNABLE_TO_SHUTDOWN;
	}

	//Reset Member Variables to null
	bm->mgmtData->top = -1;
	bm->mgmtData->poolSize = 0;
	bm->mgmtData->currentPageCount = 0;
	bm->mgmtData->policy(&(bm->mgmtData->front), &(bm->mgmtData->rear), (BM_PageFrame **)0,(BM_PageFrame **)0 ,DESTROY_QUEUE);
	

	return RC_OK;
}

/*******************************************************************************
 * Function Name: forceFlushPool
 *
 * Description: Writes to disk all pages in a buffer pool that can and shoulb be.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool with pages to write.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *		RC_UNABLE_TO_FLUSH_POOL
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates while testing
 *
*******************************************************************************/

RC forceFlushPool(BM_BufferPool *const bm){
	// Find all dirty page and write back to disk given their fix count is 0
	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}

	// Open Disk Page File in case of writing back of dirty block
	
	SM_FileHandle fh;

	if(openPageFile(bm->pageFile,&fh) != RC_OK){
		RC_message = "There is problem to open Page File to write a dirty Block back to the pageFile from this Buffer Pool ; hence unable to flush this Buffer Pool";
                return RC_UNABLE_TO_FLUSH_POOL;
	}

	if(bm->mgmtData->pagePool != ((BM_PageFrame *)0)){
			
		for(int i=0; i < bm->mgmtData->poolSize; i++){
			if(bm->mgmtData->pagePool[i].dirtyFlag == true){ 
				if(writeBlock(bm->mgmtData->pagePool[i].pageNum,&fh,(SM_PageHandle)(bm->mgmtData->pagePool[i].data)) != RC_OK){
					RC_message = "There is problem to write a dirty Block back to the pageFile from this Buffer Pool ; hence unable to shutdown this Buffer Pool";
               				return RC_UNABLE_TO_FLUSH_POOL;
				}
				bm->mgmtData->pagePool[i].dirtyFlag = false;
				bm->mgmtData->writeIO++;
				// Make this index as an available slot for future Page Frame to be fetched from disk
				push(bm,i);
			}
		}
	}

	if(closePageFile(&fh) != RC_OK){
		RC_message = "There is problem to close Page File after Flushing Buffer Pool ; hence unable to flush this Buffer Pool";
                return RC_UNABLE_TO_FLUSH_POOL;
	}
	
	return RC_OK;	
}

// Buffer Manager Interface Access Pages

// Mark Dirty Flag

/*******************************************************************************
 * Function Name: markDirty
 *
 * Description: Marks a page as dirty.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool containing page.
 *		BM_PageHandle *const page - Page to mark as dirty.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
	if(bm == ((BM_BufferPool *)0) || page == ((BM_PageHandle *)0)){
		RC_message = "Passed Argument(s) are not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}

	
	(bm->mgmtData->pagePresenceRegister[page->pageNum])->dirtyFlag = true;

	return RC_OK;
}

/*******************************************************************************
 * Function Name: unpinPage
 *
 * Description: Unpins a page when it is no longer being used.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool containing page.
 *		BM_PageHandle *const page - Page to unpin.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *		RC_UNABLE_TO_UNPIN
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){
	
	if(bm == ((BM_BufferPool *)0) || page == ((BM_PageHandle *)0)){
		RC_message = "Passed Argument(s) are not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}


	// if fix count is non-zero then decrease it by one
	// map from page->numPages to correct page frame to decrease its fix count 
	if((bm->mgmtData->pagePresenceRegister[page->pageNum])->FixCount>0){
		((bm->mgmtData->pagePresenceRegister[page->pageNum])->FixCount)--;
	}else{
		RC_message = "Fix Count is less than 1; Hence unable to unpin this page";
                return RC_UNABLE_TO_UNPIN;
	}

	return RC_OK;
}

/*******************************************************************************
 * Function Name: forcePage
 *
 * Description: Writes the current content of the page back to the page file on disk.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool containing page.
 *		BM_PageHandle *const page - Page to write.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *		RC_UNABLE_TO_FORCEPAGE
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){

	if(bm == ((BM_BufferPool *)0) || page == ((BM_PageHandle *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}

	// Open Disk Page File 
	
	SM_FileHandle fh;

	if(openPageFile(bm->pageFile,&fh) != RC_OK){
		RC_message = "There is problem to open Page File to write a dirty Block back to the pageFile from the current page ; hence unable to do force page";
                return RC_UNABLE_TO_FORCEPAGE;
	}

	// Force particular Page write to disk
			
	if(writeBlock(page->pageNum,&fh,(SM_PageHandle)(page->data)) != RC_OK){
			RC_message = "There is problem to write a dirty Block back to the pageFile from the current Page Handle ; hence unable to do force page";
               		return RC_UNABLE_TO_FORCEPAGE;
	}

	bm->mgmtData->writeIO++;
	
	return RC_OK;
}

/*******************************************************************************
 * Function Name: pinPage
 *
 * Description: Pins a page when it's being used.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool containing page.
 *		BM_PageHandle *const page - Page Handle for the page to pin.
 *		const PageNumber pageNum - Page number of the page to pin.
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *		RC_UNABLE_TO_PIN
 *		RC_DYNAMIC_MEM_ALLOC_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-28      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Updates for LRU
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *      2016-03-04      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Added extra code to mark delimiter positions ahead of scanning data stream to fasten lookup
 *
*******************************************************************************/

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum){
	
	if(bm == ((BM_BufferPool *)0) || page == ((BM_PageHandle *)0)
             || pageNum < 0 || pageNum > MAX_PAGE-1 ){
		RC_message = "Passed Argument(s) are not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}

	BM_PageFrame *targetSlotPoolFrame;
	SM_FileHandle fh;
  	SM_PageHandle ph;

	// Page is present in Cache Memory
	if(bm->mgmtData->pagePresenceRegister[pageNum] != ((BM_PageFrame *)0)){
		targetSlotPoolFrame = bm->mgmtData->pagePresenceRegister[pageNum];

		// In case of LRU / FIFO update correct information for this page
	    if ( bm->strategy == RS_LRU) {
			if(bm->mgmtData->policy(&(bm->mgmtData->front), &(bm->mgmtData->rear), &targetSlotPoolFrame, bm->mgmtData->pagePresenceRegister ,UPDATE_USAGE) != RC_OK){
				RC_message = "There is problem to update page usage data structure ; hence unable to pin this page";
                		return RC_UNABLE_TO_PIN;
			}
		}

	}else{
		// Page is not in Cache , so bring it to cache from disk

		if(openPageFile(bm->pageFile,&fh) != RC_OK){
			RC_message = "There is problem to open pageFile for this Buffer Pool ; hence unable to pin this page";
                	return RC_UNABLE_TO_PIN;
		}

		if(bm->mgmtData->currentPageCount < bm->mgmtData->poolSize){

			// Buffer Pool is not full
			int vacantIndex;
			if(pop(bm,&vacantIndex) != RC_OK){
				RC_message = "There is problem to find any vacant page from the Buffer Pool that is stored in stack; hence unable to pin this page";
                		return RC_UNABLE_TO_PIN;
			}
			targetSlotPoolFrame = &(bm->mgmtData->pagePool[vacantIndex]);
			//Increase Current Page Count by one
			bm->mgmtData->currentPageCount++;
		}else{

			// Buffer Pool is full

			if(bm->mgmtData->policy(&(bm->mgmtData->front), &(bm->mgmtData->rear), &targetSlotPoolFrame,bm->mgmtData->pagePresenceRegister,FIND_VICTIM_PAGE) != RC_OK){
				RC_message = "There is problem to find the victim page from the Buffer Pool ; hence unable to pin this page";
                		return RC_UNABLE_TO_PIN;
			}

			// if it's dirty and Fix Count is zero then write it back to disk else return with message and RC_UNABLE_TO_PIN code

			if(targetSlotPoolFrame->dirtyFlag == true){
				if(targetSlotPoolFrame->FixCount == 0){
					if(writeBlock(targetSlotPoolFrame->pageNum,&fh,(SM_PageHandle)(targetSlotPoolFrame->data)) != RC_OK){
						RC_message = "There is problem to write a particular Block back to the pageFile from this Buffer Pool ; hence unable to pin this page";
                				return RC_UNABLE_TO_PIN;
					}
					bm->mgmtData->writeIO++;
				}else{
					RC_message = "Fix Count is non-zero ; so can't write back to disk ; hence unable to pin this page";
                			return RC_UNABLE_TO_PIN;
				}
			}

			// Deleting Old entry of the local hash
			bm->mgmtData->pagePresenceRegister[targetSlotPoolFrame->pageNum] = (BM_PageFrame *)0;

		}

			// Reading Block from Disk File to local variable
			ph = (SM_PageHandle) malloc(PAGE_SIZE*2);
			
			if( ph == ((char *) 0) ){
                		RC_message = "malloc failed to allocate memory for SM_PageHandle";
                		return RC_DYNAMIC_MEM_ALLOC_FAILED;
        		}
			
			// Looking for File metadata
			allFiles *temp = arrOfFiles;
			bool canIaddNewPage = false;

			if(temp != ((allFiles *) 0)){
                		while(temp != ((allFiles *) 0)){
                        		if(strcmp(temp->fh.fileName,bm->pageFile) == 0){

						// Checking if existing total page are lesser than the demanded page
						if((temp->fh.totalNumPages)-1 < pageNum){
							canIaddNewPage = true;
						}
							break;
					}
					temp = temp->next;
				}
			}

			// Since at the beginning of the process there is no page on the disk file except page number 0, hence there is need of creating this new block
			int yesItsEmpty = 0;
			if(canIaddNewPage == true){
				if(appendEmptyBlock(&fh) != RC_OK){
					RC_message = "There is problem to append a new Block to the pageFile; hence unable to pin this page";
                			return RC_UNABLE_TO_PIN;
				}
				yesItsEmpty = 1;
			}

			// Read that page
			if(readBlock(pageNum,&fh,ph,yesItsEmpty) != RC_OK){
				RC_message = "There is problem to read a particular Block from the pageFile for this Buffer Pool ; hence unable to pin this page";
                		return RC_UNABLE_TO_PIN;
			}
			bm->mgmtData->readIO++;

			// Adding New entry of incoming Page Frame to the local hash
			bm->mgmtData->pagePresenceRegister[pageNum] = targetSlotPoolFrame;
			// Storing data from Disk Page to Frame in Memory
 
			targetSlotPoolFrame->pageNum = pageNum;
			targetSlotPoolFrame->FixCount = 0;
			targetSlotPoolFrame->dirtyFlag = false;
			strcpy(targetSlotPoolFrame->data,(char *)ph);
			targetSlotPoolFrame->recordIndicesValidLastIndex = 0;
			// If it's not empty then scan through record to store record separator index
			if(yesItsEmpty == 0){
			// Storing Indices of Records from this page
			int j = 0;
			int page_len = PAGE_SIZE*2;
			char record_separator = '\0'+30;
			int recInd = 0;

			targetSlotPoolFrame->recordIndices[recInd++] = 0;
			
			while(j<page_len){
				if(ph[j] == record_separator){
					targetSlotPoolFrame->recordIndices[recInd] = j+1;
					recInd++;
				}
				j++;
			}

			targetSlotPoolFrame->recordIndicesValidLastIndex = recInd-1;
			}

			// Freeing up memory for ph

			free(ph);

			// Making a new entry of the new page in local hash

			bm->mgmtData->pagePresenceRegister[pageNum] = targetSlotPoolFrame;

			// Make FIFO list up-to-date	

			if(bm->mgmtData->policy(&(bm->mgmtData->front), &(bm->mgmtData->rear), &targetSlotPoolFrame, bm->mgmtData->pagePresenceRegister ,INSERT_PAGE) != RC_OK){
				RC_message = "There is problem to update page replacement data structure with new page frame ; hence unable to pin this page";
                		return RC_UNABLE_TO_PIN;
			}

		// Closing the Page File

		if(closePageFile(&fh) != RC_OK){
			RC_message = "There is problem to close pageFile for this Buffer Pool ; hence unable to pin this page";
                	return RC_UNABLE_TO_PIN;
		}
	}

	// Mark it as pinned increase the fix count by one

	page->pageNum = targetSlotPoolFrame->pageNum;
	page->data = targetSlotPoolFrame->data;
	targetSlotPoolFrame->FixCount++;
		
	// In case of LRU / FIFO update correct information for this page

	return RC_OK; // Later
}

// Statistics Interface

/*******************************************************************************
 * Function Name: getFrameContents
 *
 * Description: Get the contents of any populated frames.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool to get the frame contents from.
 *
 * Return: contents - An array of the page numbers of the pages stored in the 
 *		frames.
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates while testing
 *
*******************************************************************************/

PageNumber *getFrameContents (BM_BufferPool *const bm){
	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return ((PageNumber *)0);
	}

	PageNumber *contents;
	contents = malloc (sizeof(PageNumber)*bm->numPages);
	int i;
	for ( i = 0; i < bm->numPages; i++) {
		contents[i] = bm->mgmtData->pagePool[i].pageNum;
	}

	return contents;

}

/*******************************************************************************
 * Function Name: getDirtyFlags
 *
 * Description: Get the dirty flags for the pages in a buffer pool.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool to get the dirty flags for.
 *
 * Return: dirtyFlags - An array of boolean values for each page frame. Empty page
 *		frames are considered clean.
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates while testing
 *
*******************************************************************************/

bool *getDirtyFlags (BM_BufferPool *const bm){
	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return ((bool *)0);
	}

	bool *dirtyFlags;
	dirtyFlags = malloc (sizeof(bool)*bm->numPages);
	int i;
	for ( i = 0; i < bm->numPages; i++ ) {
		dirtyFlags[i] = bm->mgmtData->pagePool[i].dirtyFlag;
	}

	return dirtyFlags;

}

/*******************************************************************************
 * Function Name: getFixCounts
 *
 * Description: Get the fix counts for the pages in a buffer pool.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool to get the fixed counts for.
 *
 * Return: fixCounts - An array of integers for each page frame. 0 is returned
 *		for empty page frames.
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates while testing
 *
*******************************************************************************/

int *getFixCounts (BM_BufferPool *const bm){
	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return ((int *)0);
	}

	int *fixCounts;
	fixCounts = malloc (sizeof(int)*bm->numPages);

	int i;
	for ( i = 0; i < bm->numPages; i++ ) {
		fixCounts[i] = bm->mgmtData->pagePool[i].FixCount;
	}

	return fixCounts;
}

/*******************************************************************************
 * Function Name: getNumReadIO
 *
 * Description: Get the number of pages that have been read from disk since a 
 *		buffer pool was initialized.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool to get the number of reads for.
 *
 * Return: bm.mgmtData.readIO - The number of reads for the buffer pool.
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates while testing
 *
*******************************************************************************/

int getNumReadIO (BM_BufferPool *const bm){
	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}

	return bm->mgmtData->readIO;
}

/*******************************************************************************
 * Function Name: getNumWriteIO
 *
 * Description: Get the number of pages written to the page file since a 
 *		buffer pool was initialized.
 *
 * Patameters:
 *		BM_BufferPool *const bm - Buffer pool to get the number of writes for.
 *
 * Return: bm.mgmtData.writeIO - The number of writes for the buffer pool.
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-01      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-04      Joshua LaBorde <jlaborde@iit.edu>       Updates while testing
 *
*******************************************************************************/

int getNumWriteIO (BM_BufferPool *const bm){
	if(bm == ((BM_BufferPool *)0)){
		RC_message = "Passed Argument is not valid to proceed further ; Please pass valid values in it";
                return RC_NULL_ARGUMENT;
	}
	
	return bm->mgmtData->writeIO;
}
#endif
