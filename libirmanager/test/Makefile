CFLAGS = -g -Wall -lefence
CC = gcc 

tester_dir=utils/
INCLUDES= -I/home/xan/System/include -I/home/xan/System/include/iljit -I${tester_dir}
LIBS = -L/home/xan/System/lib -L/home/xan/System/lib/iljit  -liljitir

TESTER_SRCS = $(wildcard ${tester_dir}*.c)
TESTER_OBJS = $(TESTER_SRCS:%.c=%.o)

# unit varibles
unit_build_dir=unit/
unit_src_dir=unit_src/
UNIT_SRCS = $(wildcard ${unit_src_dir}*.c)
UNIT_PROGS = $(UNIT_SRCS:${unit_src_dir}%.c=${unit_build_dir}%)

buildtests: ${UNIT_PROGS}
	./unit.sh

${UNIT_PROGS}: ${TESTER_OBJS} ${UNIT_SRCS} force
	${CC} ${CFLAGS} ${INCLUDES} ${LIBS} $(@:${unit_build_dir}%=${unit_src_dir}%.c) -o $@ ${TESTER_OBJS}

${TESTER_OBJS}: ${TESTER_SRCS}
	${CC} ${CFLAGS} ${INCLUDES} ${LIBS} $(@:%.o=%.c) -c -o $@

force: ;

clean:
	rm ${unit_build_dir}*

.PHONY: buildtests
