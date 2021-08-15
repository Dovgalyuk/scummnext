import sys

def readByte(fin):
    return int.from_bytes(fin.read(1), "little")

def readWord(fin):
    return int.from_bytes(fin.read(2), "little")

def openCostume(path, cost):
    return open("%s/%03d.cost" % (path, cost), "rb")
