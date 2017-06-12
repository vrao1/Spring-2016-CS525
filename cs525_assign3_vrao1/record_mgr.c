#ifndef RECORD_MGR_H
#define RECORD_MGR_H

#include <string.h>
#include <stdlib.h>
#include "dberror.h"
#include "expr.h"
#include "tables.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// Bookkeeping for scans

typedef struct ScanDataMgmt{
  Expr *cond;
  RID lastRecReturned;
  RID lastValidRec;
} ScanDataMgmt;


typedef struct RM_ScanHandle
{
  RM_TableData *rel;
  ScanDataMgmt *mgmtData;
} RM_ScanHandle;

/*******************************************************************************
 * Function Name: initRecordManager
 *
 * Description: Initializing Record Manager
 *
 * Patameters:
 *		void *mgmtData - Management Data.
 *
 * Return: RC - Return Code
 *      RC_OK
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
RC initRecordManager (void *mgmtData)
{
	// Initialize Storage Manager
	initStorageManager();

	return RC_OK;
}

/*******************************************************************************
 * Function Name: shutdownRecordManager
 *
 * Description: Shutdown Record Manager
 *
 * Patameters:
 *		No Parameter
 * Return: RC - Return Code
 *      RC_OK
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
RC shutdownRecordManager (){
	//Destroy Linked List of RM_TableData
	return RC_OK;
}

/*******************************************************************************
 * Function Name: createSchema
 *
 * Description: Creating Schema
 *
 * Patameters:
 * 		int numAttr - Total Attributes
 * 		char **attrNames - All Attribute Names
 * 		DataType *dataTypes - All Datatypes of each attribute
 * 		int *typeLength - Datatype length of each attribute
 * 		int keySize - Number of key attributes 
 * 		int *keys - Array of key attributes
 *
 * Return: Pointer to new (Schema *) pointer
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
	if(numAttr < 1 || *attrNames == ((char *)0) || 
		dataTypes == ((DataType *)0) || typeLength == ((int *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return ((Schema *)0);
        }

	Schema *newSchema = (Schema *) malloc (sizeof(Schema));

	if(newSchema == ((Schema *)0)){
                RC_message = "Malloc failed to allocate memory for Schema type";
                return ((Schema *)0);
	}

	newSchema->numAttr = numAttr;
	newSchema->attrNames = attrNames;
	newSchema->dataTypes = dataTypes;
	newSchema->typeLength = typeLength;
	
	for(int i = 0;i < numAttr;i++){
		switch(dataTypes[i]){
			case DT_INT:
	  			newSchema->typeLength[i] = sizeof(int);
	  			break;
			case DT_FLOAT:
	  			newSchema->typeLength[i] = sizeof(float);
	  			break;
			case DT_STRING:
	  			newSchema->typeLength[i] = typeLength[i];
	  			break;
			case DT_BOOL:
	  			newSchema->typeLength[i] = sizeof(bool);
	  			break;
		}
	}
	
	newSchema->keyAttrs = keys;
	newSchema->keySize = keySize;
	
	return newSchema;	
}

/*******************************************************************************
 * Function Name: readSchema
 *
 * Description: Reading Schema into data stream
 *
 * Patameters:
 * 		Schema **schema - Double Pointer to Schema Data Type
 * 		char *data - Data Stream Array
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
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while Testing
 *
*******************************************************************************/
RC readSchema(Schema **schema,char *data){
	if(data == ((char *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	char *attrNames[128]; // Attribute Names To Be Stored
	DataType dt[128];     // Data Type Names To Be Stored
	int typeLength[128];  // Length of Each data Type 
	int attrNum = 0;
	char *token,*attrTok,*attrs,*keyTok,*keyName;
	char *token_r,*attrTok_r,*attrs_r,*keyTok_r,*keyName_r;

	int keys[128];  // Keys indices refered to attrNames array to be stored 
  	int keyLen = 0;

	//Tokenizing data string
	token = strtok_r(data,"|",&token_r);
	
	token = strtok_r(((char *)0),"|",&token_r);
	attrTok = strtok_r(token,",",&attrTok_r);
	int i,j;
	char stringLength[8];
	bool takeChar = false;

	//Parsing Attribute Names and their data types
	while(attrTok != ((char *)0)){

		// This is Attribute Name
		attrs = strtok_r(attrTok,":",&attrs_r);
		attrNames[attrNum] = (char *) malloc (strlen(attrs)+1);
		if(attrNames[attrNum] == ((char *)0)){
                	RC_message = "Malloc Failed";
                	return RC_DYNAMIC_MEM_ALLOC_FAILED;
        	}
		strcpy(attrNames[attrNum],attrs);
		attrs = strtok_r(((char *)0),":",&attrs_r);

		// This is it's Data Type
		if(strncmp(attrs,"INT",3) == 0){
			typeLength[attrNum] = sizeof(int);
			dt[attrNum] = DT_INT;
		}else if(strncmp(attrs,"FLOAT",4) == 0){
			typeLength[attrNum] = sizeof(float);
			dt[attrNum] = DT_FLOAT;
		}else if(strncmp(attrs,"BOOL",4) == 0){
			typeLength[attrNum] = sizeof(bool);
			dt[attrNum] = DT_BOOL;
		}else{
			takeChar = false;
			i = j = 0;
			while(attrs[i] != ']'){
				if(attrs[i] == '['){
					takeChar = true;
				}else if(takeChar == true){
					stringLength[j++] = attrs[i];
				}
				i++;
			}
			stringLength[j] = '\0';
			typeLength[attrNum] = atoi(stringLength);
			dt[attrNum] = DT_STRING;
		}

		attrNum++;
		attrTok = strtok_r(((char *)0),",",&attrTok_r);
	}

	token = strtok_r(((char *)0),"|",&token_r);
	keyTok = strtok_r(token,":",&keyTok_r);
	keyTok = strtok_r(((char *)0),":",&keyTok_r);
	
	//Parsing Keys
	keyName = strtok_r(keyTok,",",&keyName_r);
	while(keyName != ((char *)0)){
		keys[keyLen] = atoi(keyName);
		keyLen++;
		keyName = strtok_r(((char *)0),",",&keyName_r);
	}

  	char **cpNames = (char **) malloc(sizeof(char*) * attrNum);
  	DataType *cpDt = (DataType *) malloc(sizeof(DataType) * attrNum);
  	int *cpSizes = (int *) malloc(sizeof(int) * attrNum);
  	int *cpKeys = (int *) malloc(sizeof(int) * keyLen);

  	for(int i = 0; i < attrNum; i++)
	{
		cpNames[i] = (char *) malloc (strlen(attrNames[i])+1);
      		strcpy(cpNames[i],attrNames[i]);
	}

  	for(int i = 0; i < attrNum; i++)
	{
      		free(attrNames[i]);
		attrNames[i] = ((char *)0);
	}

  	memcpy(cpDt, dt, sizeof(DataType) * attrNum);
  	memcpy(cpSizes, typeLength, sizeof(int) * attrNum);
  	memcpy(cpKeys, keys, sizeof(int)*keyLen);

	//Dynamically Allocated and initializing Schema Content from disk into cache 
  	*schema = createSchema(attrNum, cpNames, cpDt, cpSizes, keyLen, cpKeys);

	if(*schema == ((Schema *)0)){
                RC_message = "Schema in the cache could not be allocated";
                return RC_UNABLE_TO_READ_PAGE_HANDLE;
	}

	return RC_OK;
}
/*******************************************************************************
 * Function Name: createTable
 *
 * Description: Creating Table
 *
 * Patameters:
 * 		char *name - Name of the Schema 
 * 		Schema *schema - Schema Object
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *	RC_FILE_OPEN_FAILED
 *	RC_PAGE_CREATION_FAILED
 *	RC_WRITE_FAILED	
 *	RC_DYNAMIC_MEM_ALLOC_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/
RC createTable (char *name, Schema *schema){
	if(name == ((char *)0) || schema == ((Schema *)0)){
                RC_message = "Argument name or schema is NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	char fileName[32];
	sprintf(fileName,"%s.bin",name);

	// Create a file to store schema information
	if(createPageFile(fileName) != RC_OK){
		RC_message = "File Creation Failed";
                return RC_PAGE_CREATION_FAILED;
	}
	
	// Writing First Page with Schema Information
	int i;

  	char result[512];
	sprintf(result,"Name:%s|",name);
	
  	for(i = 0; i < schema->numAttr; i++)
    	{
      		sprintf(result+strlen(result),"%s%s:", (i != 0) ? ",": "", schema->attrNames[i]);
      		switch (schema->dataTypes[i])
		{
			case DT_INT:
	  			strcat(result, "INT");
	  			break;
			case DT_FLOAT:
	  			strcat(result, "FLOAT");
	  			break;
			case DT_STRING:
	  			sprintf(result+strlen(result),"STRING[%i]", schema->typeLength[i]);
	  			break;
			case DT_BOOL:
	  			strcat(result,"BOOL");
	  			break;
		}
    	}

  	strcat(result,"|Keys:");

	for(i = 0; i < schema->keySize; i++)
    		sprintf(result+strlen(result), "%s%d", ((i != 0) ? ",": ""), schema->keyAttrs[i]); 

	BM_PageHandle *pgHandl = MAKE_PAGE_HANDLE();

	// Allocate Page Handle Data
	pgHandl->data = (char *) malloc (PAGE_SIZE);

	if(pgHandl->data == ((char *)0)){
		RC_message = "Malloc Failed";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
	}

	// Appending End Character
  	strcat(result,"|");

	// Writing first page with Schema Information
	pgHandl->pageNum = 0;	
	strcpy(pgHandl->data, result);

	// Writing Schema Definition at first page
	       // Open Disk Page File 

        SM_FileHandle fh;

        if(openPageFile(fileName,&fh) != RC_OK){
                RC_message = "There is problem to open Page File to write a dirty Block back to the pageFile from the current page ;";
                return RC_FILE_OPEN_FAILED;
        }

        // Force particular Page write to disk

        if(writeBlock(pgHandl->pageNum,&fh,(SM_PageHandle)(pgHandl->data)) != RC_OK){
                        RC_message = "There is problem to write a dirty Block back to the pageFile from the current Page Handle ;";
                        return RC_WRITE_FAILED;
        }
        //bm->mgmtData->writeIO++;

	free(pgHandl->data);
	free(pgHandl);

	return RC_OK;
}

/*******************************************************************************
 * Function Name: openTable
 *
 * Description: Openning Table and initializing Buffer
 *
 * Patameters:
 * 		RM_TableData *rel - Record Manager Table Data
 * 		char *name - Name of Table
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *		RC_BUFFER_INIT_FAILED
 *		RC_DYNAMIC_MEM_ALLOC_FAILED
 *		RC_UNABLE_TO_PIN
 *		RC_UNABLE_TO_READ_PAGE_HANDLE
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/
RC openTable (RM_TableData *rel, char *name){
	if(name == ((char *)0) || rel == ((RM_TableData *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Make temporary File Name
	char fileName[32];
	sprintf(fileName,"%s.bin",name);
	
	// Assign Relation Name in Table Structure
	rel->name = (char *) malloc (strlen(name)+1);
	strcpy(rel->name,name);

	// Allocate management data structure	
	rel->mgmtData = (TableDataMgmt *) malloc (sizeof(TableDataMgmt));

	if(rel->mgmtData == ((TableDataMgmt *)0)){
		RC_message = "Malloc Failed";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
	}

	// Allocate Buffer Pool
	rel->mgmtData->bm = (BM_BufferPool *) malloc (sizeof(BM_BufferPool));

	if(rel->mgmtData->bm == ((BM_BufferPool *)0)){
		RC_message = "Malloc Failed";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
	}

	if(initBufferPool(rel->mgmtData->bm, fileName , 3, RS_FIFO, NULL) != RC_OK){
		RC_message = "Buffer Pool Initialization has failed";
                return RC_BUFFER_INIT_FAILED;
	}

	// Allocate Page Handle
	rel->mgmtData->pgHandl = MAKE_PAGE_HANDLE();
	rel->mgmtData->pgHandl->data = (char *) malloc (PAGE_SIZE);

	// Read Schema From Disk into Cache
        if(pinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl,0) != RC_OK){
                RC_message = "Pin Page is Failing";
                return RC_UNABLE_TO_PIN;
        }


	// Store Schema Information into Cache
	if(readSchema(&(rel->schema),rel->mgmtData->pgHandl->data) != RC_OK){
                RC_message = "There is parsing issue with page handle data to get schema";
                return RC_UNABLE_TO_READ_PAGE_HANDLE;
	}
        // Unpinning The Page
        if(unpinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Unpin Page is Failing";
                return RC_UNABLE_TO_UNPIN;
        }
       

	// Storing Next Vacant Starting Page in the Page File
	rel->mgmtData->nextVacancy.page = 1;
	rel->mgmtData->nextVacancy.slot = 0;

	return RC_OK;
}

/*******************************************************************************
 * Function Name: closeTable
 *
 * Description: Closing Table
 *
 * Patameters:
 * 		RM_TableData *rel - Record Manager Table Data
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *		RC_SHUTDOWN_BUFFER_FAILED	
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
RC closeTable (RM_TableData *rel){
	if(rel == ((RM_TableData *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	if(shutdownBufferPool(rel->mgmtData->bm) != RC_OK){
                RC_message = "Shutdown Buffer Pool Failed";
                return RC_SHUTDOWN_BUFFER_FAILED;
	}

	free(rel->mgmtData->pgHandl);
	free(rel->mgmtData->bm);

	rel->mgmtData->bm = (BM_BufferPool *)0;
	rel->mgmtData->nextVacancy.page = -1;
	rel->mgmtData->nextVacancy.slot = -1;
	rel->mgmtData->pgHandl = (BM_PageHandle *)0;

	free(rel->mgmtData);

	return RC_OK;
}

/*******************************************************************************
 * Function Name: deleteTable
 *
 * Description: Pins a page when it's being used.
 *
 * Patameters:
 *		char *name - Table Name
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *		RC_FILE_DESTROY_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
RC deleteTable (char *name){
	if(name == ((char *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Make temporary File Name
	char fileName[32];
	sprintf(fileName,"%s.bin",name);
	
	// Delete Page File
	if(destroyPageFile(fileName) != RC_OK){
                RC_message = "Destroyt Page File is Failing";
                return RC_FILE_DESTROY_FAILED;
	}

	return RC_OK;
}

/*******************************************************************************
 * Function Name: getNumTuples
 *
 * Description: Getting Number of Tuples
 *
 * Patameters:
 *		RM_TableData *rel - Record Manager Table Data Object
 *
 * Return: int - Number of Tuples
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-20      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-31      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *
*******************************************************************************/
int getNumTuples (RM_TableData *rel){
	return 1;
}


/*******************************************************************************
 * Function Name: insertRecord
 *
 * Description: Insert Record into Table
 *
 * Patameters:
 *		RM_TableData *rel - Record Manager Table Data Object
 *		Record *record - One Record Object to be inserted
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *		RC_UNABLE_TO_PIN
 *		RC_MARK_DIRTY_FAILED
 *		RC_UNABLE_TO_UNPIN
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-30      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/
RC insertRecord (RM_TableData *rel, Record *record){
	if(rel == ((RM_TableData *)0) || record == ((Record *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	char header[1024];
	bool writeHeader = false;
	int recordLen = 0,totCurrLen;
	int headerLen = 2*sizeof(int);
        char record_separator = '\0'+30;
        char data_separator = '\0'+31;

	for(int i = 0;i < rel->schema->numAttr;i++)
	recordLen += rel->schema->typeLength[i];


	// Write Header information
	if(rel->mgmtData->nextVacancy.slot > 0){
		// Calculate if the next record would fit into this page or load next page
		
		totCurrLen = headerLen + recordLen * (rel->mgmtData->nextVacancy.slot);	
		if(totCurrLen > PAGE_SIZE){
			rel->mgmtData->nextVacancy.page++;
			rel->mgmtData->nextVacancy.slot=0;
		}

	}


	if(rel->mgmtData->nextVacancy.slot == 0){
		int header_ptr = 0;
 
		int sizeHeader = sprintf(header,"%d%c%d%c",header_ptr,data_separator,recordLen,record_separator);

		rel->mgmtData->nextVacancy.slot++;
		writeHeader = true;
	}

	//pinning the older page or newer page depending on prevoius size calculation
	if(pinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl,rel->mgmtData->nextVacancy.page) != RC_OK){
                RC_message = "Pin Page is Failing";
                return RC_UNABLE_TO_PIN;
	}


	// Data Record to be written
	record->data[record->lastIndex] = '\0';
	int writtenBytes = 0;
	
	if(writeHeader == true){
		writtenBytes = sprintf(rel->mgmtData->pgHandl->data ,"%s%s",header,record->data);
	}else{
		writtenBytes = sprintf(rel->mgmtData->pgHandl->data + rel->mgmtData->bm->mgmtData->pagePresenceRegister[rel->mgmtData->pgHandl->pageNum]->recordIndicesValidLastIndex ,"%s",record->data);
	}
	rel->mgmtData->bm->mgmtData->pagePresenceRegister[rel->mgmtData->pgHandl->pageNum]->recordIndicesValidLastIndex += writtenBytes;


	if(markDirty(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Mark Dirty is Failing";
                return RC_MARK_DIRTY_FAILED;
	}

	// Unpinning The Page
	if(unpinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Unpin Page is Failing";
                return RC_UNABLE_TO_UNPIN;
	}

	// Storing the corresponding pag and slot information of this record data
	record->id.page = rel->mgmtData->nextVacancy.page;
	record->id.slot = rel->mgmtData->nextVacancy.slot;

	rel->mgmtData->nextVacancy.slot++;

	return RC_OK;
}

/*******************************************************************************
 * Function Name: deleteRecord
 *
 * Description: Deleting Record
 *
 * Patameters:
 *		RM_TableData *rel - Record Manager Table Data
 *		RID id - Record ID.
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *      	RC_UNABLE_TO_PIN
 *      	RC_MARK_DIRTY_FAILED
 *      	RC_UNABLE_TO_UNPIN
 *      	RC_UNABLE_TO_FORCEPAGE
 *      	RC_MARK_DIRTY_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>       	Updates for changes
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-31      Vinod Rao <vrao1@hawk.iit.edu>          Added extra code to introduce Tokenization 
 *
*******************************************************************************/
RC deleteRecord (RM_TableData *rel, RID id){
	if(rel == ((RM_TableData *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Reading the required page from disk or cache
	if(pinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl,id.page) != RC_OK){
                RC_message = "Pin Page is Failing";
                return RC_UNABLE_TO_PIN;
	}

 	int j = 0;
        int page_len = PAGE_SIZE*2;
        char record_separator = '\0'+30;
        int recInd = 0;

        rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[recInd++] = 0;

        while(j<page_len){
               if(rel->mgmtData->pgHandl->data[j] == record_separator){
                    rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[recInd] = j+1;
                    recInd++;
               }
               j++;
        } 

        rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndicesValidLastIndex = recInd-1;

	int recordBegin=0;
        int recordEnd=0;

        recordBegin = rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[id.slot];
        recordEnd = rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[id.slot+1]-1;

	int i=0;
	char deletedChar[15];
	char del = '~'+1;

	memset(deletedChar,del,sizeof(char)*14);	
	deletedChar[14] = '\0';

	//Overwriting existing record with new record from recordBegin to recordEnd
	
	strncpy(rel->mgmtData->pgHandl->data+recordBegin,deletedChar,14);
	rel->mgmtData->pgHandl->data[recordBegin+4] = '\0'+31;
	rel->mgmtData->pgHandl->data[recordEnd-5] = '\0'+31;
	
	// Mark Dirty
	if(markDirty(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Mark Dirty Page is Failing";
                return RC_MARK_DIRTY_FAILED;
	}

	// Unpinning Page
	if(unpinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Unpin Page is Failing";
                return RC_UNABLE_TO_UNPIN;
	}

	// Force Page
	if(forcePage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Force Page is Failing";
                return RC_UNABLE_TO_FORCEPAGE;
	}
	
	return RC_OK;
}

/*******************************************************************************
 * Function Name: updateRecord
 *
 * Description: Updating Record 
 *
 * Patameters:
 *		RM_TableData *rel - Record Manager Table Data
 *		RID id - Record ID.
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *      	RC_UNABLE_TO_PIN
 *      	RC_MARK_DIRTY_FAILED
 *      	RC_UNABLE_TO_UNPIN
 *      	RC_UNABLE_TO_FORCEPAGE
 *      	RC_MARK_DIRTY_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>       	Updates for changes
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-31      Vinod Rao <vrao1@hawk.iit.edu>          Added extra code to introduce Tokenization 
 *
*******************************************************************************/
RC updateRecord (RM_TableData *rel, Record *record){
	if(rel == ((RM_TableData *)0) || record == ((Record *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }
	
	// Reading the required page from disk or cache
	if(pinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl,record->id.page) != RC_OK){
                RC_message = "Pin Page is Failing";
                return RC_UNABLE_TO_PIN;
	}

	int recordBegin=0;
        int recordEnd=0;

        recordBegin = rel->mgmtData->bm->mgmtData->pagePresenceRegister[record->id.page]->recordIndices[record->id.slot];
        recordEnd = rel->mgmtData->bm->mgmtData->pagePresenceRegister[record->id.page]->recordIndices[record->id.slot+1]-1;

	int i=0;
	
	//Overwriting existing record with new record from recordBegin to recordEnd

	while(recordBegin < recordEnd){
		rel->mgmtData->pgHandl->data[recordBegin] = record->data[i];
		i++;
		recordBegin++;
	}

	// Mark Dirty
	if(markDirty(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Mark Dirty Page is Failing";
                return RC_MARK_DIRTY_FAILED;
	}

	// Unpinning Page
	if(unpinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Unpin Page is Failing";
                return RC_UNABLE_TO_UNPIN;
	}

	// Force Page
	if(forcePage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Force Page is Failing";
                return RC_UNABLE_TO_FORCEPAGE;
	}

	return RC_OK;
}

/*******************************************************************************
 * Function Name: getRecord
 *
 * Description: getting Record
 *
 * Patameters:
 *		RM_TableData *rel - Record Manager Table Data
 *		RID id - Record ID.
 *		Record *record - Record Object
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *      	RC_UNABLE_TO_PIN
 *      	RC_UNABLE_TO_UNPIN
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>       	Updates for changes
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-31      Vinod Rao <vrao1@hawk.iit.edu>          Added extra code to introduce Tokenization 
*******************************************************************************/
RC getRecord (RM_TableData *rel, RID id, Record *record){
	if(rel == ((RM_TableData *)0) || id.page < 0 || id.slot < 0){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Reading the required page from disk or cache
	if(pinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl,id.page) != RC_OK){
                RC_message = "Pin Page is Failing";
                return RC_UNABLE_TO_PIN;
	}

        // Storing Indices of Records from this page
        int j = 0;
        int page_len = PAGE_SIZE*2;
        char record_separator = '\0'+30;
        int recInd = 0;

        rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[recInd++] = 0;

        while(j<page_len){
             if(rel->mgmtData->pgHandl->data[j] == record_separator){
                   rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[recInd] = j+1;
                   recInd++;
             }
             j++;
        }

        rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndicesValidLastIndex = recInd-1;


	int recordBegin;
	int recordEnd;
	int recordLen;
        char del = '~'+1;
        char deletedChar[15];

        memset(deletedChar,del,sizeof(char)*14);

	deletedChar[4] = '\0'+31;
	deletedChar[9] = '\0'+31;
	deletedChar[14] = '\0';

	record->id = id;

	recordBegin = rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[id.slot];
	recordEnd = rel->mgmtData->bm->mgmtData->pagePresenceRegister[id.page]->recordIndices[id.slot+1]-1;

	recordLen = recordEnd - recordBegin;

	strncpy(record->data,(rel->mgmtData->pgHandl->data)+recordBegin,recordLen);

	record->data[recordLen] = '\0';

	// Unpinning The Page
	if(unpinPage(rel->mgmtData->bm,rel->mgmtData->pgHandl) != RC_OK){
                RC_message = "Unpin Page is Failing";
                return RC_UNABLE_TO_UNPIN;
	}

	if(memcmp(record->data,deletedChar,14) == 0) return RC_RECORD_DELETED;

	return RC_OK;	
}


/*******************************************************************************
 * Function Name: getRecordSize
 *
 * Description: Get Record Size
 *
 * Patameters:
 *		Schema *schema - Schema
 *
 * Return: int - Record Size
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-30      Vinod Rao <vrao1@hawk.iit.edu>       	Initial Draft
 *
*******************************************************************************/
int getRecordSize (Schema *schema){
	if(schema == ((Schema *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return 0;
        }

	int recordLen = 2;	

	for(int i = 0;i < schema->numAttr;i++)
	recordLen += schema->typeLength[i];

	return (recordLen);
}

/*******************************************************************************
 * Function Name: freeSchema
 *
 * Description: Free Schema
 *
 * Patameters:
 *		Schema *schema - Schema
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
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
RC freeSchema (Schema *schema){
	if(schema == ((Schema *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }
//	char *d = schema->b;
//	if(d != ((char *)0))
//	free(d);
	free(schema);
	return RC_OK;
}

/*******************************************************************************
 * Function Name: createRecord 
 *
 * Description: Create Record
 *
 * Patameters:
 * 		Record **record - Record Double Pointer 
 *		Schema *schema - Schema
 *
 * Return: RC - Return Code
 *      	RC_OK
 *      	RC_NULL_ARGUMENT
 *		RC_DYNAMIC_MEM_ALLOC_FAILED
 *
 * Author: Vinod Rao <vrao1@hawk.iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-24      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-31      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *
*******************************************************************************/
RC createRecord (Record **record, Schema *schema){
	if(schema == ((Schema *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	Record *newRecord;
	newRecord = (Record *) malloc (sizeof(Record));
	if(newRecord == ((Record *)0)){
                RC_message = "Malloc failed to allocate memory for Record type";
                return RC_DYNAMIC_MEM_ALLOC_FAILED;
	}

	unsigned int recordLen = 0;

	for(int i=0;i<schema->numAttr;i++)
	recordLen+=schema->typeLength[i];
	
	newRecord->id.page = -1;
	newRecord->id.slot = -1;
	newRecord->data = (char *) calloc(recordLen*2,sizeof(char)); 
	newRecord->lastIndex = 0;
	*record = newRecord;

	return RC_OK;
}

/*******************************************************************************
 * Function Name: freeRecord
 *
 * Description: Free Record Memory
 *
 * Patameters:
 * 		Record *record - Record 
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
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *
*******************************************************************************/
RC freeRecord (Record *record){
	if(record == ((Record *)0)){ 
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }
	
	if(record->data != ((char *)0)) free(record->data); 
	free(record);

	return RC_OK;
}

/*******************************************************************************
 * Function Name: getAttr
 *
 * Description: Getting Attributes
 *
 * Patameters:
 * 		Record *record - Record 
 * 		Schema *schema - Schema 
 * 		int attrNum - Number of Attribute 
 * 		Value *value - Value Object
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
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-30      Vinod Rao <vrao1@hawk.iit.edu>          Added extra code to introduce Tokenization
 *
*******************************************************************************/
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){
	if(record == ((Record *)0) || schema == ((Schema *)0) || attrNum < 0 || attrNum >= schema->numAttr || value == ((Value **)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	char data_separator = '\0'+31;
	int whichAttr = -1;
	int recordLen = 2;	
	int realRecordIndex = 0;
	char *strData;

	int attr_int;
	float attr_float;
	bool attr_bool;
	int i;

	for(i = 0;i < schema->numAttr;i++)
	recordLen += schema->typeLength[i];

	record->data[recordLen] = data_separator;
	i = 0;

	while(i <= recordLen){
		if(record->data[i] == data_separator){
			whichAttr++;
			if(whichAttr == attrNum){
				record->data[i] = '\0';
			
				switch(schema->dataTypes[attrNum]){
					case DT_INT:
						attr_int = atoi(record->data+realRecordIndex);
						MAKE_VALUE(*value,DT_INT,attr_int);
						break;
					case DT_FLOAT:
						attr_float = atof(record->data+realRecordIndex);
						MAKE_VALUE(*value, DT_FLOAT, attr_float);
						break;
					case DT_STRING:
						MAKE_STRING_VALUE(*value,record->data+realRecordIndex);
						break;
					case DT_BOOL:
						attr_bool = atoi(record->data+realRecordIndex);
						MAKE_VALUE(*value, DT_BOOL, attr_bool);
						break;
				}

				record->data[i] = data_separator;
				break;
			}
			realRecordIndex = i+1;
		}
		i++;
	}

	record->data[recordLen] = '\0';
	return RC_OK;
}

/*******************************************************************************
 * Function Name: setAttr
 *
 * Description: Setting Attributes
 *
 * Patameters:
 * 		Record *record - Record 
 * 		Schema *schema - Schema 
 * 		int attrNum - Number of Attribute 
 * 		Value *value - Value Object
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
 *      2016-03-10      Vinod Rao <vrao1@hawk.iit.edu>          Initial Draft
 *      2016-03-14      Vinod Rao <vrao1@hawk.iit.edu>          Updates while testing
 *      2016-03-30      Vinod Rao <vrao1@hawk.iit.edu>          Added extra code to introduce Tokenization
 *
*******************************************************************************/
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){
	if(record == ((Record *)0) || schema == ((Schema *)0) || attrNum < 0 || attrNum >= schema->numAttr || value == ((Value *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
        }

	// Assigning Record Values
	int offset = 0;
	int k = 0,j;
	int writtenBytes = 0;
	int remainder = 0,quotient = 0;
	char ds = '\0'+31;
	char array[5];
	bool q,r;
	offset = attrNum*5;

	memset(array,'0',sizeof(char)*4);

      	switch (value->dt)
	{
		case DT_INT:
			quotient = value->v.intV;
			j = 3;
			while(quotient > 0 && j >= 0 ){
				remainder = quotient % 10;
				quotient = quotient / 10;
				array[j] = array[j] + remainder;
				j--;
			}
			array[4] = '\0';
			writtenBytes = sprintf(record->data + offset,"%s%c" ,array,ds);
	  		break;
		case DT_FLOAT:
			writtenBytes = sprintf(record->data + offset,"%f%c" ,value->v.floatV,ds);
	  		break;
		case DT_STRING:
			writtenBytes = sprintf(record->data + offset,"%s%c" ,value->v.stringV,ds);
	  		break;
		case DT_BOOL:

			q = value->v.boolV;
			j = 3;
			while(q > 0 && j >= 0 ){
				r = q % 10;
				q = q / 10;
				array[j] = array[j] + r;
				j--;
			}
			array[4] = '\0';
			writtenBytes = sprintf(record->data + offset,"%s%c" ,array,ds);
	  		break;
	}

	record->lastIndex = offset+writtenBytes;

	if(attrNum+1 == schema->numAttr) 
	record->data[record->lastIndex-1] = '\0'+30;

	return RC_OK;
}

/*******************************************************************************
 * Function Name: startScan
 *
 * Description: Scanning
 *
 * Patameters:
 *		RM_TableData *rel - Record Manager Table Data
 *		RM_ScanHandle *scan - Record Manager Scan Handle.
 *		Expr *cond - Expression Condition Object
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *	RC_UNABLE_TO_OPEN_TABLE
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-24      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-30      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *      2016-03-31      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *
*******************************************************************************/
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){

  if(rel == ((RM_TableData *)0) || scan == ((RM_ScanHandle *)0) || cond == ((Expr *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
  }

  //Check that table is open and if not open it
  if(rel->mgmtData->bm == ((BM_BufferPool *)0)){
        if(openTable(rel, rel->name) != RC_OK) {
                RC_message = "Open Table Failed";
                return RC_UNABLE_TO_OPEN_TABLE;
        }
  }

  //Initialize scan values
  scan->rel = rel;
  scan->mgmtData = (ScanDataMgmt *) malloc (sizeof(ScanDataMgmt));
  scan->mgmtData->cond = cond;
  scan->mgmtData->lastRecReturned.page = 1;
  scan->mgmtData->lastRecReturned.slot = 0;

  return RC_OK;

}

/*******************************************************************************
 * Function Name: next
 *
 * Description: Next Scan
 *
 * Patameters:
 *		RM_ScanHandle *scan - Record Manager Scan Handle
 *		Record *record - Record Object
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *	RC_UNABLE_TO_GET_ATTRIBUTE
 *
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-24      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-30      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *      2016-03-31      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *
*******************************************************************************/
RC next (RM_ScanHandle *scan, Record *record){

  if(scan == ((RM_ScanHandle *)0) || record == ((Record *)0)){
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
  }

  Record *checkRecord;
  createRecord(&checkRecord, scan->rel->schema);
  checkRecord->id.page = scan->mgmtData->lastRecReturned.page;
  checkRecord->id.slot = scan->mgmtData->lastRecReturned.slot + 1;
  Value *lVal, *rVal;
  Value *tVal, *bVal;
  tVal = (Value *) malloc (sizeof (Value));
  Expr *eval;

  if (checkRecord->id.slot > 10) {  return RC_RM_NO_MORE_TUPLES; }

  //Handle Netsting
  if  (scan->mgmtData->cond->expr.op->args[0]->type == EXPR_OP) {
        eval = (scan->mgmtData->cond->expr.op->args[0]);
  } else { eval = scan->mgmtData->cond; }

  //Set the arguments if they are constants
  if (eval->expr.op->args[0]->type == EXPR_CONST) { lVal = eval->expr.op->args[0]->expr.cons; }
  if (eval->expr.op->args[1]->type == EXPR_CONST) { rVal = eval->expr.op->args[1]->expr.cons; }

  //for all records starting at the next one going until a valid condition is found
  while (getRecord(scan->rel, checkRecord->id, checkRecord) == RC_OK) {

        //Set the arguments if they are attributes
        if (eval->expr.op->args[0]->type == EXPR_ATTRREF) {
           if(getAttr(checkRecord, scan->rel->schema, eval->expr.op->args[0]->expr.attrRef, &lVal) != RC_OK) {
                        RC_message = "Get Attribute Failed";
                        return RC_UNABLE_TO_GET_ATTRIBUTE;
           }
        }
        if (eval->expr.op->args[1]->type == EXPR_ATTRREF) {
           if(getAttr(checkRecord, scan->rel->schema, eval->expr.op->args[1]->expr.attrRef, &rVal) != RC_OK) {
                        RC_message = "Get Attribute Failed";
                        return RC_UNABLE_TO_GET_ATTRIBUTE;
           }
        }

        //Check the condition for a winner
        switch (scan->mgmtData->cond->expr.op->type)
        {
                case OP_BOOL_AND:
                        if (lVal && rVal) {
                        }
                        break;
                case OP_BOOL_OR:
                        if (lVal || rVal) {
                        }
                        break;
                case OP_BOOL_NOT:
                        valueSmaller (lVal, rVal, tVal);
                        boolNot(tVal, tVal);
                        if (tVal->v.boolV) {
                               *record = *checkRecord;
                               scan->mgmtData->lastRecReturned = checkRecord->id;
                               return RC_OK;
                        }
                        break;
                case OP_COMP_EQUAL:
                        valueEquals (lVal, rVal, tVal);
                        if (tVal->v.boolV) {
                                *record = *checkRecord;
                                scan->mgmtData->lastRecReturned = checkRecord->id;
                               return RC_OK;
                        }
	                break;
                case OP_COMP_SMALLER:
			if (lVal || rVal) {
			}
			break;
        }

	//No winner found. Check next record.
        checkRecord->id.slot++;
        if (checkRecord->id.slot > 10) {  return RC_RM_NO_MORE_TUPLES; }

  }

	return RC_OK;

}

/*******************************************************************************
 * Function Name: closeScan
 *
 * Description: Closing Scan
 *
 * Patameters:
 *		RM_ScanHandle *scan - Scan Handle
 *
 * Return: RC - Return Code
 *      RC_OK
 *      RC_NULL_ARGUMENT
 *
 * Author: Joshua LaBorde <jlaborde@iit.edu>
 *
 * History:
 *      Date            Name                                    Content
 *      ----------      ------------------------------          -----------------
 *      2016-03-30      Joshua LaBorde <jlaborde@iit.edu>       Initial Draft
 *      2016-03-31      Joshua LaBorde <jlaborde@iit.edu>       Updates for changes
 *
*******************************************************************************/
RC closeScan (RM_ScanHandle *scan){

  if (scan == ((RM_ScanHandle *)0)) {
                RC_message = "Arguments are NULL ; Please pass some valid address in it";
                return RC_NULL_ARGUMENT;
  }

  free(scan->mgmtData);

	return RC_OK;
}

#endif // RECORD_MGR_H
