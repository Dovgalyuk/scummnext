import sys
import utils
import constants

print("Decoding rooms")

path = sys.argv[1]

fout = open("%s/rooms.asm" % path, "w")

for i in range(1, constants.numRooms):
    f = open("%s/%02d.out" % (path, i), "rb")
    f.seek(10)
    # graphics data
    f.seek(utils.readWord(f))
    fout.write("; Room %d\n" % i)
    fout.write("; tileset=%d\n" % utils.readByte(f))
    # palette
    # background
    f.close()

fout.close()
