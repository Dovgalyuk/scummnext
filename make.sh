export PATH=${PATH}:${HOME}/z88dk/bin
export ZCCCFG=${HOME}/z88dk/lib/config

#zcc +zxn -m -v -startup=1 -SO3 -clib=sdcc_iy --max-allocs-per-node200000 -subtype=nex -create-app @project.lst -o scummnext -pragma-include:zpragma.inc
make -j 4
