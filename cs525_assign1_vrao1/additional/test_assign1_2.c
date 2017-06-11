#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testMultiplePageContent(void);

/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();
  testMultiplePageContent();

  return 0;
}


void
testMultiplePageContent(void)
{
  SM_FileHandle fh;
  SM_FileHandle tfh; //Used in negative testing.
  SM_PageHandle ph;
  SM_PageHandle tph; //Used in negative testing.
  int i;

  testName = "test multiple page content";

  ph = (SM_PageHandle) malloc(1 * PAGE_SIZE);

  // create a new page file
  ASSERT_ERROR(createPageFile (""), "blank filename");
  TEST_CHECK(createPageFile (TESTPF));
  ASSERT_ERROR(openPageFile ("", &fh), "blank filename");
  ASSERT_ERROR(openPageFile ("abc", &fh), "non-existing filename");
  ASSERT_ERROR(openPageFile (TESTPF, ((SM_FileHandle *) 0)), "null filehandle");
  ASSERT_ERROR(closePageFile (&fh), "close unopen file");
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");
  
  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  ASSERT_ERROR(writeCurrentBlock (&tfh, ph), "null filehandle");
  ASSERT_ERROR(writeCurrentBlock (&fh, ((char *) 0)), "null pagehandle");
  TEST_CHECK(writeCurrentBlock (&fh, ph));
  printf("writing first / current block position = %d\n",fh.curPagePos);
  ASSERT_TRUE((fh.curPagePos == END_OF_FILE), "page position should be next page or EOF if last written page was at end of the file");

  // read back the page containing the string and check that it is correct
  ASSERT_ERROR(readFirstBlock (&tfh, ph), "null filehandle");
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first / current block\n");
 ASSERT_TRUE((fh.curPagePos != 0), "page position should not not be 0");

  // read back the page containing the string and check that it is correct
  ASSERT_ERROR(readCurrentBlock (&tfh, ph), "null filehandle");
  ASSERT_ERROR(readCurrentBlock (&fh, ph), "File pointer is at EOF");

  // add additional pages
  ASSERT_ERROR(appendEmptyBlock (&tfh), "null filehandle");
  TEST_CHECK(appendEmptyBlock (&fh));
  printf("adding another page\n");
  ASSERT_ERROR(ensureCapacity (2, &tfh), "null filehandle");
  TEST_CHECK(ensureCapacity (2, &fh));
  ASSERT_EQUALS_INT(1, getBlockPos (&fh), "check appendEmptyBlock or getBlockPos");

   // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = ((2 * i ) % 10) + '0';
  ASSERT_ERROR(writeBlock (0, &tfh, ph), "null filehandle");
  ASSERT_ERROR(writeBlock (0, &fh, ((char *) 0)), "null pagehandle");
  TEST_CHECK(writeBlock (1, &fh, ph));
  printf("writing second block\n");

  // read back the page containing the string and check that it is correct
  ASSERT_ERROR(readBlock (1, &tfh, ph), "null filehandle");
memset(ph,'\0',PAGE_SIZE);
  TEST_CHECK(readBlock (1, &fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == ((2 * i) % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading second block\n");

  // add additional pages
  ASSERT_ERROR(appendEmptyBlock (&tfh), "null filehandle");
  TEST_CHECK(appendEmptyBlock (&fh));
  printf("adding another page\n");

  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = ((3 * i ) % 10) + '0';
  TEST_CHECK(writeBlock (2, &fh, ph));
  printf("writing third block\n");

  // read back the page containing the string and check that it is correct
  ASSERT_ERROR(readCurrentBlock (&tfh, ph), "null filehandle");
memset(ph,'\0',PAGE_SIZE);
  TEST_CHECK(readLastBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == ((3 * i) % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading third / last block\n");
  ASSERT_TRUE((fh.curPagePos == END_OF_FILE), "page position should be EOF");

  // read back the page containing the string and check that it is correct
  ASSERT_ERROR(readPreviousBlock (&tfh, ph), "null filehandle");
memset(ph,'\0',PAGE_SIZE);
  TEST_CHECK(readPreviousBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == ((3 * i) % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading second / previous block\n");
  ASSERT_TRUE((fh.curPagePos == END_OF_FILE), "page position should be EOF");

  // read back the page containing the string and check that it is correct
  ASSERT_ERROR(readNextBlock (&tfh, ph), "null filehandle");
  TEST_CHECK(readFirstBlock (&fh, ph));
memset(ph,'\0',PAGE_SIZE);
  TEST_CHECK(readNextBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == ((3 * i) % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading 3rd / next block\n");
  ASSERT_TRUE((fh.curPagePos == END_OF_FILE), "page position should be EOF");


  ASSERT_ERROR(closePageFile (&tfh), "null filehandle");
  TEST_CHECK(closePageFile (&fh));

  // destroy new page file
  ASSERT_ERROR(destroyPageFile (""), "blank filename");
  ASSERT_ERROR(destroyPageFile ("xyz"), "non-existing filename");
  TEST_CHECK(destroyPageFile (TESTPF));  
 free(ph); 
  TEST_DONE();
}
