#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<errno.h>
#include<stdlib.h>
#include <string.h>
#include <limits.h>

#include "dberror.h"
#include "storage_mgr.h"

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


typedef struct _allFiles allFiles;

allFiles *arrOfFiles; // Doubly Link List head pointer 

/************************************************************
 *                    interface                             *
 ************************************************************/

#ifdef RECORD_MGR

// struct for test records
 typedef struct TestRecord {
   int a;
   char *b;
   int c;
} TestRecord;

/*******************************************************************************
 * Function Name: readStreamThenWrite
 *
 * Description: This function is scanning through page stream stored in buffer.
 *              Then it's tokenizing attribute values based on delimiters used to distinguish records and member data
 *
 * Patameters:
 *              int fHandle - File Descriptor
 *		SM_PageHandle memPage - char data stream of all bytes stored in particular page
 *		int pageLen - Maximum length of data stream passed as 2nd argument
 *
 * Return: RC - Return Codes : RC_NULL_ARGUMENT,RC_OK
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>       Initial Draft
 *      2016-03-18      Vinod Rao <vrao1@hawk.iit.edu>       Updates while testing
 *      2016-03-30      Vinod Rao <vrao1@hawk.iit.edu>       Finalized Code
 *
*******************************************************************************/
RC readStreamThenWrite(int fHandle, SM_PageHandle memPage,int pageLen){
 	if(memPage == ((char *)0) || fHandle == 0 || pageLen < 1){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	int schemaPg;
        int recordLen;
        char deletedChar[5];
        char record_separator = '\0'+30;
        char data_separator = '\0'+31;
	char del = '~'+1;
	char *data = memPage;
	int i = 0;
	
	memset(deletedChar,del,sizeof(char)*4);
	deletedChar[4] = '\0';

	// Scanning header part
	
	while(i < pageLen && data[i] != data_separator) i++;

	data[i] = '\0';
	schemaPg = atoi(data);
	data[i] = data_separator;
	
	int recordLenIndex = i+1;	
	
	while(i < pageLen && data[i] != record_separator) i++;

	data[i] = '\0';
	recordLen = atoi(data+recordLenIndex);
	data[i] = record_separator;

	TestRecord *temp;
	temp = (TestRecord *) malloc(sizeof(TestRecord));
	int stringSize = recordLen-(2*sizeof(int));

	temp->b = (char *) malloc(sizeof(char)*(1+stringSize));

	int realRecordIndex = i+1;
	short int whichData = 0;
	int writtenBytes = write(fHandle,&schemaPg,sizeof(int));	 
	writtenBytes += write(fHandle,&recordLen,sizeof(int));	 
	i++;

	// Looping through Data Stream
	while(i < pageLen){

		// Writing Actual data in bytes (binary format) 
		if(data[i] == record_separator && whichData == 2){
			data[i] = '\0';
			if(memcmp(data+realRecordIndex,deletedChar,4) == 0){
				temp->c = INT_MAX;
			}else{
				temp->c = atoi(data+realRecordIndex);
			}

			writtenBytes += write(fHandle,&(temp->a),sizeof(int));
			writtenBytes += write(fHandle,temp->b,stringSize);
			writtenBytes += write(fHandle,&(temp->c),sizeof(int));
	
			whichData = 0;
			realRecordIndex = i+1;
			data[i] = record_separator;
		}else if(data[i] == data_separator){ // Tokenizing All Member Data Except Last One 
			data[i] = '\0';
		
			switch(whichData){
				case 0:
					if(memcmp(data+realRecordIndex,deletedChar,4) == 0){
						temp->a = INT_MAX;
					}else{
						temp->a = atoi(data+realRecordIndex);
					}

					whichData=1;
					break;
				case 1:
					if(memcmp(data+realRecordIndex,deletedChar,4) == 0){
						strncpy(temp->b,deletedChar,stringSize);
					}else{
						strncpy(temp->b,data+realRecordIndex,stringSize);
					}
					temp->b[stringSize] = '\0';

					whichData=2;
					break;
				default:
					whichData=0;
					break;
			}

			realRecordIndex = i+1;
			data[i] = data_separator;
		}
		i++;
	}

	// putting NULL Character as last bytes
	char null_char = '\0';

	while(writtenBytes < PAGE_SIZE){
		
		writtenBytes += write(fHandle,&null_char,sizeof(char));
	
	}

	free(temp->b);
	free(temp);

	return RC_OK;
}

/*******************************************************************************
 * Function Name: readDiskIntoStream
 *
 * Description: This function is scanning through page stored on disk and loading into data stream stored in buffer.
 *              Then it's putting appropriate delimeters among bytes to distinguish records and member data
 *
 * Patameters:
 *              int fHandle - File Descriptor
 *		SM_PageHandle memPage - char data stream of all bytes stored in particular page
 *		int bufLen - Maximum length of data stream passed as 2nd argument
 *
 * Return: RC - Return Codes : RC_NULL_ARGUMENT,RC_OK
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>       Initial Draft
 *      2016-03-18      Vinod Rao <vrao1@hawk.iit.edu>       Updates while testing
 *      2016-03-30      Vinod Rao <vrao1@hawk.iit.edu>       Finalized Code
 *
*******************************************************************************/

RC readDiskIntoStream(int fHandle ,SM_PageHandle memPage, int bufLen){
 	if(fHandle == 0 || memPage == ((char *)0) || bufLen <= 0){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	int realRecordIndex;
	int schemaPg;
        int recordLen;
        int recordNewLen;
        char record_separator = '\0'+30;
        char data_separator = '\0'+31;
	char *data = memPage;
	char firstData[5] = "0000";
	char thirdData[5] = "0000";
	int q=0,r=0,l=0;
	int i = 0;
	char del = '~'+1;
	char deletedChar[5];

	memset(deletedChar,del,sizeof(char)*4);
	deletedChar[4] = '\0';

	// Reading header
	// Reading First data stored in header
	
	read(fHandle,&schemaPg,sizeof(int));

	//Reading Second data stored in header
	read(fHandle, &recordLen, sizeof(int));

	//Storing Header data in the data stream
	realRecordIndex = sprintf(data,"%d%c%d%c",schemaPg,data_separator,recordLen,record_separator);

	//Allocating memory to Local TestRecord Data Type to read bytes into while scanning page
	TestRecord *temp;
	temp = (TestRecord *) malloc(sizeof(TestRecord));
	int stringSize = recordLen-(2*sizeof(int));
	temp->b = (char *) malloc(sizeof(char)*(1+stringSize));

	// Maximum Record can be accommodated in a page
	int totalRecord = (PAGE_SIZE-(2*sizeof(int)))/recordLen;

	i = 0;
	while(i<totalRecord){

		memset(firstData,'0',sizeof(char)*4);
		memset(thirdData,'0',sizeof(char)*4);

		read(fHandle,&(temp->a),sizeof(int));
		read(fHandle,temp->b,stringSize);
		read(fHandle,&(temp->c),sizeof(int));

		temp->b[stringSize] = '\0';
		if(temp->a == INT_MAX && memcmp(temp->b,deletedChar,4) == 0 && temp->c == INT_MAX){
			recordNewLen = sprintf(data+realRecordIndex,"%s%c%s%c%s%c",
			deletedChar,data_separator,deletedChar,data_separator,deletedChar,record_separator);
		}else{
			l = 3;
			q = temp->a;

			while(q > 0 && l >= 0){
				r = q % 10;
				q = q / 10;
				firstData[l] = firstData[l]+r;
				l--;
			}

			firstData[4] = '\0';

			l = 3;
			q = temp->c;

			while(q > 0 && l >= 0){
				r = q % 10;
				q = q / 10;
				thirdData[l] = thirdData[l]+r;
				l--;
			}
			thirdData[4] = '\0';

			// Reading from Disk into data stream
			recordNewLen = sprintf(data+realRecordIndex,"%s%c%s%c%s%c",
			firstData,data_separator,temp->b,data_separator,thirdData,record_separator);
		}
		realRecordIndex = realRecordIndex + recordNewLen;
		i++;
	}

	free(temp->b);
	free(temp);

	return RC_OK;
}

#endif

/*******************************************************************************
 * Function Name: initStorageManager
 *
 * Description: Initializes the Head pointer of doubly link list arrOfFiles to NULL
 *      
 *
 * Patameters:
 *      void 
 *
 * Return: RC - Return Code
 *	void        
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-02-01      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/

void initStorageManager (void){
	arrOfFiles = (allFiles *) 0;
}


/*******************************************************************************
 * Function Name: copyFileHandle
 *
 * Description: Copies values from source to destination File Handle pointers
 *
 * Patameters:
 * 	SM_FileHandle *destFHandle - Destination File Handle address 
 * 	SM_FileHandle *srcFHandle - Source File Handle address
 *
 * Return: RC - Return Code
 * 	RC_OK
 *	RC_NULL_ARGUMENT
 *
 * Main Logic:
 *      It copies all member variables from source to destination object
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-10	Vinod Rao <vrao1@hawk.iit.edu>		Newly Written Function
 *
*******************************************************************************/
RC copyFileHandle (SM_FileHandle *destFHandle, SM_FileHandle *srcFHandle , int which_field){

 	if(srcFHandle == ((SM_FileHandle *)0) || destFHandle == ((SM_FileHandle *)0)){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	switch(which_field){
		case 1:
			destFHandle->fileName = srcFHandle->fileName;
			break;
		case 2:
  			destFHandle->totalNumPages = srcFHandle->totalNumPages;
			break;
		case 3:
  			destFHandle->curPagePos = srcFHandle->curPagePos;
			break;
		case 4:
  			destFHandle->mgmtInfo.fd = srcFHandle->mgmtInfo.fd;
  			destFHandle->mgmtInfo.ptr = srcFHandle->mgmtInfo.ptr;
			break;
		default:
			destFHandle->fileName = srcFHandle->fileName;
  			destFHandle->totalNumPages = srcFHandle->totalNumPages;
  			destFHandle->curPagePos = srcFHandle->curPagePos;
  			destFHandle->mgmtInfo.fd = srcFHandle->mgmtInfo.fd;
  			destFHandle->mgmtInfo.ptr = srcFHandle->mgmtInfo.ptr;
	}

	return RC_OK;
}

/*******************************************************************************
 * Function Name: createPageFile
 *
 * Description: Create a page file with initial size of 1
 * 	page filled with all '\0' bytes.
 *
 * Patameters:
 * 	char *fileName - Name of file to create.
 *
 * Return: RC - Return Code
 * 	RC_OK
 *	RC_WRITE_FAILED
 *	RC_DYNAMIC_MEM_ALLOC_FAILED
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_NULL_ARGUMENT
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * Main Logic:
 *      It calls "creat" system call to create new file
 *      It also initializes first PAGE_SIZE bytes all to '\0' 
 *      It appends newly created File Handle node to the last position of
 *      a linked list "arrOfFiles"
 *      
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *	2016-03-04	Vinod Rao <vrao1@hawk.iit.edu>	        Changed arguments of creat/open system call , added umask 
 *
*******************************************************************************/
RC createPageFile (char *fileName){
	int fd; // File Descriptor
	if( fileName == ((char *)0) || strlen(fileName) == 0){
		RC_message = "Argument fileName is NULL ; Please pass some string in it";
		return RC_NULL_ARGUMENT;
	}

	umask(0);

	fd = creat(fileName , 0777);
	
	if(fd == -1){
		RC_message = "CREAT System Call failed to create file";
		return RC_FILE_HANDLE_NOT_INIT;
	}
	
	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		// Traversing till last node of doubly link list where new file node would be inserted upon its creation
		while(temp->next != NULL)
			temp = temp->next;
	}	

	// New file node creation
	allFiles *newFH = (allFiles *) malloc (sizeof(allFiles));
	
	if( newFH == ((allFiles *) 0) ){
		RC_message = "malloc failed to allocate memory for new File Handle";
		close(fd);
		return RC_DYNAMIC_MEM_ALLOC_FAILED;
	}

	//Allocating memory to store file name
	newFH->fh.fileName = (char *)malloc(strlen(fileName)+1);

	if( newFH->fh.fileName == ((char *)0) ){
		RC_message = "malloc failed to allocate memory for fileName string";
		close(fd);
		free(newFH);	
		return RC_DYNAMIC_MEM_ALLOC_FAILED;
	}

	// character buffer initialized with '\0' bytes

 	static char buf[PAGE_SIZE];	
    	int written = write(fd,buf,PAGE_SIZE);	

	if( written == -1 ){
		printError(errno);
		RC_message = "First page initialization with null character failed by write system call";
		close(fd);
		free(newFH->fh.fileName);
                free(newFH);
		return RC_WRITE_FAILED;
	}

	close(fd);

	strcpy(newFH->fh.fileName,fileName);

	//The current context info being stored into the node object
 
	newFH->fh.totalNumPages = 1;
	newFH->fh.curPagePos = 0;
	newFH->fh.mgmtInfo.fd = -1;
	newFH->fh.mgmtInfo.ptr = ((FILE *) 0); // Stored to use in future or subsequent assignments
	newFH->next = ((allFiles *) 0);
	newFH->prev = ((allFiles *) 0);

	if(temp == ((allFiles *) 0))
		arrOfFiles = newFH;
	else{
		temp->next = newFH;
		newFH->prev = temp;
	}

	return RC_OK;
}

/*******************************************************************************
 * Function Name: openPageFile
 *
 * Description: Open an existing page file.
 *
 * Patameters:
 * 	char *fileName - Name of file to open
 * 	SM_FileHandle *fHandle - Handle of file to open
 *
 * Return: RC - Return Code
 * 	RC_OK
 *	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_OPEN_FAILED
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It calls "open" system call to open already created file
 *      It also initializes first PAGE_SIZE bytes all to '\0' 
 *      It looks for existing file handle in the doubly linked list through linear scanning 
 *      of the linked list "arrOfFiles"
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC openPageFile (char *fileName, SM_FileHandle *fHandle){
	
 	if( fileName == ((char *)0)  || strlen(fileName) == 0){
                RC_message = "Argument fileName is NULL ; Please pass some string in it";
                return RC_NULL_ARGUMENT;
        }

	allFiles *temp = arrOfFiles;

	//Traversing through linked list to find out the correct File Handle node

	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
			if(strcmp(temp->fh.fileName,fileName) == 0){
				//int fd = open(temp->fh.fileName,O_RDWR,S_IWRITE | S_IREAD);
				int fd = open(temp->fh.fileName,O_RDWR, 0777);
				FILE *pt = fopen(temp->fh.fileName,"a+");

				// Handling Error on open system call and fopen function call
				if(fd == -1 || pt == ((FILE *) 0)){
					printError(errno);
					RC_message = "The file is not able to open";
					return RC_FILE_OPEN_FAILED;
				}
				temp->fh.mgmtInfo.fd = fd;
				temp->fh.mgmtInfo.ptr = pt;
				return(copyFileHandle(fHandle,&(temp->fh),ALL)); // storing address of correct File Handle into passed 2nd argument of this function
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: closePageFile
 *
 * Description: Close an open page file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to close
 *
 * Return: RC - Return Code
 * 	RC_OK
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It calls "close" system call to close file
 *      
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC closePageFile (SM_FileHandle *fHandle){
 	if( (fHandle) == ((SM_FileHandle *)0) ){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	allFiles *temp = arrOfFiles;

	//Scanning to search for correct file handle

	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
			if(( (temp->fh.fileName != (SM_PageHandle )0) && 
				(fHandle->fileName != (SM_PageHandle )0)) && 
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){
				if(temp->fh.mgmtInfo.fd < 0 || temp->fh.mgmtInfo.ptr == ((FILE *) 0)){
					RC_message = "The file is not opened yet";
					return RC_UNOPENED_FILE;
				}
				close(temp->fh.mgmtInfo.fd);
				fclose(temp->fh.mgmtInfo.ptr);

				// Assigning initialized values after closing
				temp->fh.mgmtInfo.fd = -1;
				temp->fh.mgmtInfo.ptr = ((FILE *) 0);

				copyFileHandle(fHandle,&(temp->fh),4);		
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	return RC_FILE_HANDLE_NOT_INIT;	
}

/*******************************************************************************
 * Function Name: destroyPageFile
 *
 * Description: Delete an existing page file.
 *
 * Patameters:
 * 	char *fileName - Name of file to open
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESTROY_FAILED
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It calls "unlink" system call to destroy the file
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC destroyPageFile (char *fileName){
 	if( fileName == ((char *)0)  || strlen(fileName) == 0){
                RC_message = "Argument fileName is NULL ; Please pass some string in it";
                return RC_NULL_ARGUMENT;
        }

	allFiles *temp = arrOfFiles;

	//Scanning through linked list to look for correct file handle

	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
			if((temp->fh.fileName) && strcmp(temp->fh.fileName,fileName) == 0){

				if(temp->fh.mgmtInfo.ptr != ((FILE *) 0)){
					fclose(temp->fh.mgmtInfo.ptr);
					temp->fh.mgmtInfo.ptr = ((FILE *) 0);
				}

				if(temp->fh.mgmtInfo.fd != -1){
					close(temp->fh.mgmtInfo.fd);
				}

				int fd = unlink(temp->fh.fileName);
				if(fd == -1){
					printError(errno);
					RC_message = "The file is not able to destroy";
					return RC_FILE_DESTROY_FAILED;
				}
				free(temp->fh.fileName);
				temp->fh.fileName = ((char *) 0);

				// Removing File Handle from the link list

				allFiles *deleteTemp = temp;

				if(temp->prev != ((allFiles *) 0)){
					temp->prev->next = temp->next;
				}else{
					arrOfFiles = temp->next;
				}
				if(temp->next != ((allFiles *) 0)){
					temp->next->prev = temp->prev;
				}
				free(deleteTemp);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}


/*******************************************************************************
 * Function Name: readBlock
 *
 * Description: Reads a block from a file and stores it in memory.
 *
 * Patameters:
 * 	int pageNum - Which block to read from a file
 * 	SM_FileHandle *fHandle - Handle of file to read from
 * 	SM_PageHandle *memPage - Handle of memory to store read data in
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_FILE_READ_FAILED
 *	RC_UNOPENED_FILE
 *	RC_INVALID_PAGE_NUM
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It calls "read" system call to read particular block of the file
 *	into the given buffer memory
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *	2016-03-31	Vinod Rao <vrao1@hawk.iit.edu>		Adding Tokenization of data stream while reading from disk 
 *
*******************************************************************************/
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage, int isItEmpty)
{
 	if(((fHandle) == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }
	allFiles *temp = arrOfFiles;
	
	//Scanning through the linked list to look for correct File Handle
	int readBytes;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
                  if(( (temp->fh.fileName != (SM_PageHandle )0) && 
			(fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){
				if(pageNum < 0 || temp->fh.totalNumPages <= pageNum){
                			RC_message = "Argument pageNum is not available ; Please pass some valid pageNum";
                			return RC_INVALID_PAGE_NUM;
				}
				 
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}
				
				if(lseek(temp->fh.mgmtInfo.fd, pageNum*PAGE_SIZE, SEEK_SET)<0){
					printError(errno);
					RC_message = "The file descriptor is not reset to the pageNum";
					return RC_FILE_DESCRIPTOR_INIT_FAILED;
				}
				#ifdef RECORD_MGR
				if( pageNum != 0 && isItEmpty == 0){
					if(readDiskIntoStream(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE*2) != RC_OK){
						RC_message = "Could not read from this block into memory";
						return RC_FILE_READ_FAILED;
}
				}else{
					readBytes = read(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);
					if(readBytes < 0){
						printError(errno);
						RC_message = "The read system call failed";
						return RC_FILE_READ_FAILED;
					}
				}
				#else	
				readBytes = read(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);
				if(readBytes < 0){
					printError(errno);
					RC_message = "The read system call failed";
					return RC_FILE_READ_FAILED;
				}
				#endif
	
				temp->fh.curPagePos = (temp->fh.totalNumPages == pageNum+1) ? END_OF_FILE : temp->fh.curPagePos+1;
				copyFileHandle(fHandle,&(temp->fh),3);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: getBlockPosition
 *
 * Description: Gets the current page position in a file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to get the current page positions of
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It returns Current Page Position from the correct File Handle
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
int getBlockPos (SM_FileHandle *fHandle){

 	if(fHandle == ((SM_FileHandle *)0)){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	allFiles *temp = arrOfFiles;

	//Sacnning for correct File Handle 

	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
                  if(( (temp->fh.fileName != (SM_PageHandle )0) && 
			(fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){
				return (temp->fh.curPagePos);
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: readFirstBlock
 *
 * Description: Reads the first block of a file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to read the data from
 * 	SM_PageHandle memPage - Handle of memory to store read data in
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_FILE_READ_FAILED
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It re-points file pointer to first block location 
 *	and read that block into the given buffer memory, current page position is readjusted accordingly in the given function body 
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }
	
	allFiles *temp = arrOfFiles;
	
	//Scanning to find correct File Handle

	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}

				// Reset File Descriptor to 1st position

				if(lseek(temp->fh.mgmtInfo.fd,0L,SEEK_SET)<0){
					printError(errno);
					RC_message = "The file descriptor is not reset to beginning";
					return RC_FILE_DESCRIPTOR_INIT_FAILED;
				}

				int readBytes = read(temp->fh.mgmtInfo.fd, memPage, PAGE_SIZE);
				if(readBytes < 0){
					printError(errno);
					RC_message = "The read system call failed";
					return RC_FILE_READ_FAILED;
				}
				//If number of total pages is 1 , then after reading first block the current page position must point to END_OF_FILE
				temp->fh.curPagePos = (temp->fh.totalNumPages == 1) ? END_OF_FILE : 1;
				copyFileHandle(fHandle,&(temp->fh),3);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: readPreviousBlock
 *
 * Description: Reads the first block of a file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to read the data from
 * 	SM_PageHandle memPage - Handle of memory to store read data in
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_NO_PREVIOUS_BLOCK
 *	RC_FILE_READ_FAILED
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It re-points file pointer to first block location 
 *	and read that block into the given buffer memory, current page position is readjusted accordingly in the given function body 
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	//Scanning to find correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
                if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}

				if(temp->fh.curPagePos == 0 || temp->fh.totalNumPages <= 1){
					RC_message = "There is no previous block from the current page position";
					return RC_NO_PREVIOUS_BLOCK;
				}

				//Re-adjusting File Descriptor to previous page

				off_t offset = lseek(temp->fh.mgmtInfo.fd,(-1)*PAGE_SIZE ,SEEK_CUR);
			
				if(offset < 0){
					printError(errno);
					RC_message = "lseek system call failed";
					return RC_FILE_DESCRIPTOR_INIT_FAILED;
				}

				int readBytes = read(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);
				if(readBytes < 0){
					printError(errno);
					RC_message = "The read system call failed";
					return RC_FILE_READ_FAILED;
				}
				
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: readCurrentBlock
 *
 * Description: Reads the block of a file based on the current page position.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to read the data from
 * 	SM_PageHandle memPage - Handle of memory to store read data in
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_FILE_READ_FAILED
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *	It reads current block into the given buffer memory, 
 *	current page position is readjusted accordingly in the given function body 
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	//Scanning to look for correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}

				if(temp->fh.curPagePos == END_OF_FILE || temp->fh.totalNumPages < 1){
					RC_message = "There is no current block to read";
					return RC_NO_CURRENT_BLOCK;
				}

				int readBytes = read(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);

				if(readBytes < 0){
					printError(errno);
					RC_message = "The read system call failed";
					return RC_FILE_READ_FAILED;
				}
				//Current Page Position is incremented by one except the last page 	
				temp->fh.curPagePos = (temp->fh.totalNumPages == temp->fh.curPagePos+1) ? END_OF_FILE : temp->fh.curPagePos+1;  
				copyFileHandle(fHandle,&(temp->fh),3);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: readNextBlock
 *
 * Description: Reads the next block of a file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to read the data from
 * 	SM_PageHandle memPage - Handle of memory to store read data in
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_FILE_READ_FAILED
 *	RC_NO_NEXT_BLOCK
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *	It reads next block into the given buffer memory, 
 *	current page position is readjusted accordingly in the given function body 
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Scanning to look for correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}

				if(temp->fh.curPagePos == END_OF_FILE || temp->fh.totalNumPages <= 1 || temp->fh.curPagePos+1 == temp->fh.totalNumPages){
					RC_message = "There is no next block from the current page position";
					return RC_NO_NEXT_BLOCK;
				}

				off_t offset = lseek(temp->fh.mgmtInfo.fd,PAGE_SIZE ,SEEK_CUR);
			
				if(offset < 0){
					printError(errno);
					RC_message = "lseek system call failed";
					return RC_FILE_DESCRIPTOR_INIT_FAILED;
				}

				int readBytes = read(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);

				if(readBytes < 0){
					printError(errno);
					RC_message = "The read system call failed";
					return RC_FILE_READ_FAILED;
				}
				
				temp->fh.curPagePos = (temp->fh.totalNumPages == temp->fh.curPagePos+2) ? END_OF_FILE : temp->fh.curPagePos+2;
 				copyFileHandle(fHandle,&(temp->fh),3); 
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: readLastBlock
 *
 * Description: Reads the last block of a file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to read the data from
 * 	SM_PageHandle memPage - Handle of memory to store read data in
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_FILE_READ_FAILED
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *	It reads last block into the given buffer memory, 
 *	current page position is readjusted accordingly in the given function body 
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	//Scanning to look for correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}

				if(temp->fh.totalNumPages < 1){
                                        RC_message = "There is no block to read";
                                        return RC_NO_NEXT_BLOCK;
                                }

				off_t offset = lseek(temp->fh.mgmtInfo.fd,(-1)*PAGE_SIZE ,SEEK_END);
			
				if(offset < 0){
					printError(errno);
					RC_message = "lseek system call failed";
					return RC_FILE_DESCRIPTOR_INIT_FAILED;
				}

				int readBytes = read(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);

				if(readBytes < 0){
					printError(errno);
					RC_message = "The read system call failed";
					return RC_FILE_READ_FAILED;
				}

				// Since Last Page has been read

				temp->fh.curPagePos = END_OF_FILE;
				copyFileHandle(fHandle,&(temp->fh),3);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}


/*******************************************************************************
 * Function Name: writeBlock
 *
 * Description: Writes a page to disk.
 *
 * Patameters:
 * 	int pageNum - Which page to write to
 * 	SM_FileHandle *fHandle - Handle of file to write the data to
 * 	SM_PageHandle memPage - Handle of memory containing data to write
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_WRITE_FAILED
 *	RC_UNOPENED_FILE
 *	RC_INVALID_PAGE_NUM
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It writes block into the file,
 *      current page position is readjusted accordingly in the given function body
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *	2016-03-31	Vinod Rao <vrao1@hawk.iit.edu>		Parse data stream using Tokenization while writing into disk 
 *
*******************************************************************************/
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	//Scanning to look for correct File Handle
	int writtenBytes;
	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
                if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){
				if(pageNum < 0 || temp->fh.totalNumPages <= pageNum){
                			RC_message = "Argument pageNum is not available ; Please pass some valid pageNum";
                			return RC_INVALID_PAGE_NUM;
				}
				 
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}
	
				if(lseek(temp->fh.mgmtInfo.fd, pageNum*PAGE_SIZE, SEEK_SET)<0){
					printError(errno);
					RC_message = "The file descriptor is not reset to the pageNum";
					return RC_FILE_DESCRIPTOR_INIT_FAILED;
				}
				
				#ifdef RECORD_MGR
				if(pageNum != 0){
					if(readStreamThenWrite(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE*2) != RC_OK){
						RC_message = "Could not write this block into disk file";
						return RC_WRITE_FAILED;
					}
				}else{
					writtenBytes = write(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);
					if(writtenBytes < 0){
						printError(errno);
						RC_message = "The write system call failed";
						return RC_WRITE_FAILED;
					}
				}
				#else	
				writtenBytes = write(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);
				if(writtenBytes < 0){
					printError(errno);
					RC_message = "The write system call failed";
					return RC_WRITE_FAILED;
				}
				#endif

				temp->fh.curPagePos = (temp->fh.totalNumPages == pageNum+1) ? END_OF_FILE : temp->fh.curPagePos+1;
				copyFileHandle(fHandle,&(temp->fh),3);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	

}

/*******************************************************************************
 * Function Name: writeCurrentBlock
 *
 * Description: Writes Current page to disk.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to write the data to
 * 	SM_PageHandle memPage - Handle of memory containing data to write
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_WRITE_FAILED
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 * 
 * Main Logic:
 *      It writes current block into the file
 *      current page position is readjusted accordingly in the given function body
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

 	if((fHandle == ((SM_FileHandle *)0)) || ((memPage) == ((char *) 0))){
                RC_message = "Argument fHandle or memPage is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Scanning to look for correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}

				if(temp->fh.curPagePos == END_OF_FILE || temp->fh.totalNumPages < 1){
                                        RC_message = "There is no block to write";
                                        return RC_NO_NEXT_BLOCK;
                                }

				int writtenBytes = write(temp->fh.mgmtInfo.fd,memPage,PAGE_SIZE);

				if(writtenBytes < 0){
					printError(errno);
					RC_message = "The write system call failed";
					return RC_WRITE_FAILED;
				}
				temp->fh.curPagePos = (temp->fh.totalNumPages == temp->fh.curPagePos+1) ? END_OF_FILE : temp->fh.curPagePos+1;	
				copyFileHandle(fHandle,&(temp->fh),3);					
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: appendEmptyBlock
 *
 * Description: Adds an additional page to the file.
 *
 * Patameters:
 * 	SM_FileHandle *fHandle - Handle of file to append the page to
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_FILE_DESCRIPTOR_INIT_FAILED
 *	RC_WRITE_FAILED
 *	RC_UNOPENED_FILE
 *	RC_NULL_ARGUMENT
 *
 * Main Logic:
 *      It appends empty block into last position of the file
 *      current page position is readjusted accordingly in the given function body
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC appendEmptyBlock (SM_FileHandle *fHandle){

 	if(fHandle == ((SM_FileHandle *)0)){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Scanning to look for correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.mgmtInfo.fd == -1){
					RC_message = "The file is not opened";
					return RC_UNOPENED_FILE;
				}
				
                                off_t offset = lseek(temp->fh.mgmtInfo.fd,0L,SEEK_END);

                                if(offset < 0){
                                        printError(errno);
                                        RC_message = "lseek system call failed";
                                        return RC_FILE_DESCRIPTOR_INIT_FAILED;
                                }

 				static char buf[PAGE_SIZE];	
				int writtenBytes = write(temp->fh.mgmtInfo.fd,buf,PAGE_SIZE);

				if(writtenBytes < 0){
					printError(errno);
					RC_message = "The write system call failed";
					return RC_WRITE_FAILED;
				}
				
				temp->fh.totalNumPages += 1;
				temp->fh.curPagePos = temp->fh.totalNumPages-1; 
				copyFileHandle(fHandle,&(temp->fh),2);
				copyFileHandle(fHandle,&(temp->fh),3);
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

/*******************************************************************************
 * Function Name: ensureCapacity
 *
 * Description: Checks and ensures that a file has a certain number of pages.
 *
 * Patameters:
 * 	int numberOfPages - The number of pages to check and ensure the file has
 * 	SM_FileHandle *fHandle - Handle of file to check and possibly grow
 *
 * Return: RC - Return Code
 * 	RC_OK
 * 	RC_FILE_NOT_FOUND
 *	RC_FILE_HANDLE_NOT_INIT
 *	RC_TOTAL_PAGE_CAPACITY_EXCEEDED
 *	RC_NULL_ARGUMENT
 *
 * Main Logic:
 *      It checks for the existence of certain number of pages
 * 
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 * 	Date		Name					Content
 * 	----------	------------------------------		-----------------
 *	2016-02-01	Vinod Rao <vrao1@hawk.iit.edu>		Initial Draft
 *	2016-02-07	Vinod Rao <vrao1@hawk.iit.edu>		Updates after review
 *	2016-02-10	Joshua LaBorde <jlaborde@iit.edu>	Updates while testing
 *
*******************************************************************************/
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){

 	if(fHandle == ((SM_FileHandle *)0)){
                RC_message = "Argument fHandle is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Scanning to look for correct File Handle

	allFiles *temp = arrOfFiles;
	if(temp != ((allFiles *) 0)){
		while(temp != ((allFiles *) 0)){
		if(( (temp->fh.fileName != (SM_PageHandle )0) &&
                        (fHandle->fileName != (SM_PageHandle )0)) &&
                            strcmp(temp->fh.fileName,fHandle->fileName) == 0){	
				if(temp->fh.totalNumPages > numberOfPages){
					return RC_TOTAL_PAGE_CAPACITY_EXCEEDED;
				}
				return RC_OK;
			}
			temp = temp->next;
		}
	}else{
		RC_message = "No Files have been created so far";
		return RC_FILE_HANDLE_NOT_INIT;	
	}

	RC_message = "That file has not been created so far";
	return RC_FILE_NOT_FOUND;	
}

#endif
