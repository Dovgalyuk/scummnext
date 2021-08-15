import sys
from utils import readWord
import constants

def extractResource(res, room, offset, suffix):
    fr = open("%s/%02d.out" % (path, room), "rb")
    fr.seek(offset)
    fc = open("%s/%03d.%s" % (path, res, suffix), "wb")
    sz = readWord(fr)
    fr.seek(offset)
    buf = fr.read(sz)
    fc.write(buf)
    fr.close()
    fc.close()

def extractCostume(cost, room, offset):
    extractResource(cost, room, offset, "cost")

def extractScript(cost, room, offset):
    extractResource(cost, room, offset, "script")

print("Parsing room 0")

path = sys.argv[1]
name_in = "%s/00.out" % path

fin = open(name_in, "rb")
magic = readWord(fin)

# read global objects

fin.seek(constants.numGlobalObjects, 1)

# skip room offsets

fin.seek(constants.numRooms * 3, 1)

# extract costumes

rooms = fin.read(constants.numCostumes)
for cost in range(0, constants.numCostumes):
    extractCostume(cost, rooms[cost], readWord(fin))

# extract scripts

rooms = fin.read(constants.numScripts)
for script in range(0, constants.numScripts):
    extractScript(script, rooms[script], readWord(fin))

# do nothing with sounds

fin.close()
