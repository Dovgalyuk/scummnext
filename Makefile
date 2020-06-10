CFILES = $(wildcard *.c)
OFILES = $(CFILES:.c=.o)

PROGRAM = scummnext.nex

all: $(PROGRAM)

scummnext.nex: $(OFILES)
	zcc +zxn -m -o $(basename $@) $(OFILES) -clib=sdcc_iy -v -create-app -subtype=nex -pragma-include:zpragma.inc

%.o: %.c
	zcc +zxn -o $@ -c $^ -v -SO3 -clib=sdcc_iy --max-allocs-per-node200000 -subtype=nex -Izxn --list

clean:
	$(RM) *.o program.bin
