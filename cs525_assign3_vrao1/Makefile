TEST_RECORD_MGR=test_assign3
TEST_EXPR=test_expr
CC=gcc
OPTION=-std=gnu99 -D RECORD_MGR
all: ${TEST_RECORD_MGR} ${TEST_EXPR}

${TEST_RECORD_MGR}: dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o record_mgr.o expr.o rm_serializer.o test_assign3_1.o
	${CC} ${OPTION} test_assign3_1.o rm_serializer.o expr.o record_mgr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o -g -Wall -lm -o test_assign3

${TEST_EXPR}: dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o record_mgr.o expr.o rm_serializer.o test_expr.o
	${CC} ${OPTION} test_expr.o rm_serializer.o expr.o record_mgr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o -g -Wall -lm -o test_expr

dberror.o: dberror.c dberror.h
	${CC} ${OPTION} -c dberror.c -o dberror.o

storage_mgr.o: storage_mgr.c storage_mgr.h
	${CC} ${OPTION} -c storage_mgr.c -o storage_mgr.o

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h
	${CC} ${OPTION} -c buffer_mgr_stat.c -o buffer_mgr_stat.o

buffer_mgr.o: buffer_mgr.c buffer_mgr.h
	${CC} ${OPTION} -c buffer_mgr.c -o buffer_mgr.o

record_mgr.o: record_mgr.c record_mgr.h
	${CC} ${OPTION} -c record_mgr.c -g -o record_mgr.o

expr.o: expr.c expr.h
	${CC} ${OPTION} -c expr.c -o expr.o

rm_serializer.o: rm_serializer.c
	${CC} ${OPTION} -c rm_serializer.c -o rm_serializer.o

test_expr.o: test_expr.c
	${CC} ${OPTION} -c test_expr.c -o test_expr.o

test_assign3_1.o: test_assign3_1.c test_helper.h tables.h dt.h
	${CC} ${OPTION} -c test_assign3_1.c -g -o test_assign3_1.o

clean:
	rm -f *.o
	rm -f test_assign3 test_expr
