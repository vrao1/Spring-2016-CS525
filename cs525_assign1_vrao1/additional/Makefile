TEST_STORAGE_MGR=test_assign1_2
CC=gcc

all: ${TEST_STORAGE_MGR}

${TEST_STORAGE_MGR}: dberror.o storage_mgr.o test_assign1_2.o
	${CC} test_assign1_2.o storage_mgr.o dberror.o -g -Wall -lm -o test_assign1_2

dberror.o: dberror.c dberror.h
	${CC} -c dberror.c -o dberror.o

storage_mgr.o: storage_mgr.c storage_mgr.h
	${CC} -c storage_mgr.c -o storage_mgr.o

test_assign1_2.o: test_assign1_2.c test_helper.h
	${CC} -c test_assign1_2.c -o test_assign1_2.o

clean:
	rm -f *.o
	rm -f test_assign1_2
