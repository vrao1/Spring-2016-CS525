#ifndef DBERROR_H
#define DBERROR_H

#include <stdio.h>
#include<limits.h>

// Macros for Page Replacement Algorithms
#define FIND_VICTIM_PAGE 1
#define INSERT_PAGE 2
#define DESTROY_QUEUE 3
#define UPDATE_USAGE 4

/* module wide constants */
#define PAGE_SIZE 4096

/* Maximum Number of pages that a file can have */
#define MAX_PAGE 262144

// End of File 
#define END_OF_FILE INT_MAX

// ALL fields of the File Handle
#define ALL 5

/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_FILE_HANDLE_NOT_INIT 2
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4

#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN 201
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_RM_NO_MORE_TUPLES 203
#define RC_RM_NO_PRINT_FOR_DATATYPE 204
#define RC_RM_UNKOWN_DATATYPE 205

#define RC_IM_KEY_NOT_FOUND 300
#define RC_IM_KEY_ALREADY_EXISTS 301
#define RC_IM_N_TO_LAGE 302
#define RC_IM_NO_MORE_ENTRIES 303

/* Newly added error code */
#define RC_NULL_ARGUMENT 400
#define RC_DYNAMIC_MEM_ALLOC_FAILED 401
#define RC_FILE_OPEN_FAILED 402
#define RC_UNOPENED_FILE 403
#define RC_FILE_DESTROY_FAILED 404
#define RC_NO_CURRENT_BLOCK 405
#define RC_FILE_DESCRIPTOR_INIT_FAILED 406
#define RC_FILE_READ_FAILED 407
#define RC_INVALID_PAGE_NUM 408
#define RC_NO_PREVIOUS_BLOCK 409
#define RC_NO_NEXT_BLOCK 410
#define RC_TOTAL_PAGE_CAPACITY_EXCEEDED 411
#define RC_STRATEGY_NOT_EXIST 412
#define RC_UNABLE_TO_PIN 413
#define RC_UNABLE_TO_UNPIN 414
#define RC_UNABLE_TO_SHUTDOWN 415
#define RC_UNABLE_TO_FORCEPAGE 416
#define RC_UNABLE_TO_FLUSH_POOL 417
#define RC_NO_FRONT_END 418
#define RC_STACK_FULL 419
#define RC_STACK_EMPTY 420
#define RC_PAGE_CREATION_FAILED 421
#define RC_BUFFER_INIT_FAILED 422
#define RC_MARK_DIRTY_FAILED 423
#define RC_SHUTDOWN_BUFFER_FAILED 424
#define RC_UNABLE_TO_READ_PAGE_HANDLE 425
#define RC_RECORD_DELETED 426
#define RC_UNABLE_TO_OPEN_TABLE 427
#define RC_UNABLE_TO_GET_RECORD 428
#define RC_UNABLE_TO_GET_ATTRIBUTE 429
#define RC_UNABLE_TO_CLOSE_TABLE 430

/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError (RC error);
extern char *errorMessage (RC error);

#define THROW(rc,message) \
  do {			  \
    RC_message=message;	  \
    return rc;		  \
  } while (0)		  \

// check the return code and exit if it is an error
#define CHECK(code)							\
  do {									\
    int rc_internal = (code);						\
    if (rc_internal != RC_OK)						\
      {									\
	char *message = errorMessage(rc_internal);			\
	printf("[%s-L%i-%s] ERROR: Operation returned error: %s\n",__FILE__, __LINE__, __TIME__, message); \
	free(message);							\
	exit(1);							\
      }									\
  } while(0);


#endif
