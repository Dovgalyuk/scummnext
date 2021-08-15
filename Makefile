CFILES = $(wildcard *.c)
ASMFILES = $(wildcard *.asm) $(wildcard gen/*.asm)
OFILES = $(CFILES:.c=.o) $(ASMFILES:.asm=.o)

PROGRAM = scummnext.nex

all: $(PROGRAM)

scummnext.nex: $(OFILES)
#	zcc +zxn -m -o $(basename $@) $(OFILES) -clib=sdcc_iy -v -create-app -subtype=nex -pragma-include:zpragma.inc
	zcc +scumm -m -o $(basename $@) $(OFILES) -clib=sdcc_iy -v -create-app -subtype=nex -pragma-include:zpragma.inc

%.o: %.c
#	zcc +zxn -o $@ -c $^ -v -SO3 -clib=sdcc_iy --max-allocs-per-node200000 -subtype=nex -Izxn --list
	zcc +scumm -o $@ -c $^ -v -SO3 --max-allocs-per-node200000 -subtype=scumm -Izxn --list

%.o: %.asm
#	zcc +zxn -o $@ -c $^ -v -SO3 -clib=sdcc_iy --max-allocs-per-node200000 -subtype=nex -Izxn --list
	zcc +scumm -o $@ -c $^ -v -SO3 --max-allocs-per-node200000 -subtype=scumm -Izxn --list

clean:
	$(RM) *.o *.lis scummnext.nex
