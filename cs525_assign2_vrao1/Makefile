TEST_BUFFER_MGR=test_assign2
CC=gcc
OPTION=-std=gnu99
all: ${TEST_BUFFER_MGR}

${TEST_BUFFER_MGR}: dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o test_assign2_1.o
	${CC} ${OPTION} test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o -g -Wall -lm -o test_assign2


dberror.o: dberror.c dberror.h
	${CC} ${OPTION} -c dberror.c -o dberror.o

storage_mgr.o: storage_mgr.c storage_mgr.h
	${CC} ${OPTION} -c storage_mgr.c -o storage_mgr.o

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h
	${CC} ${OPTION} -c buffer_mgr_stat.c -o buffer_mgr_stat.o

buffer_mgr.o: buffer_mgr.c buffer_mgr.h
	${CC} ${OPTION} -c buffer_mgr.c -o buffer_mgr.o

test_assign2_1.o: test_assign2_1.c test_helper.h
	${CC} ${OPTION} -c test_assign2_1.c -o test_assign2_1.o

clean:
	rm -f *.o
	rm -f test_assign2
