target= cantest
firmware= air_control.elf
simavr = ../..

IPATH = .
IPATH += ${simavr}/examples/parts
IPATH += ${simavr}/include
IPATH += ${simavr}/simavr/sim

VPATH = .
VPATH += ${simavr}/examples/parts

all: obj ${target} 

include ${simavr}/Makefile.common

board = ${OBJ}/${target}.elf

${board} : ${OBJ}/${target}.o
${board} : ${OBJ}/can.o

${target}: ${board}
	@echo $@ done
