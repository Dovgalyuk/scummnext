CFILES = $(wildcard *.c)
OFILES = $(CFILES:.c=.o)

PROGRAM = scummnext.nex

all: $(PROGRAM)

scummnext.nex: $(OFILES)
	zcc +zxn -o $@ $(OFILES) -create-app -v -startup=1 -SO3 -clib=sdcc_iy --max-allocs-per-node200000 -subtype=nex -pragma-include:zpragma.inc

%.o: %.c
	zcc +zxn -o $@ -c $^ -v -SO3 -clib=sdcc_iy --max-allocs-per-node200000 -subtype=nex

clean:
	$(RM) *.o program.bin
