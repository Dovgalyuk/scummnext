import sys
import utils

def decodeTiles(c, s, prefix):
    total = 0
    f = utils.openCostume(path, c)
    cnt = utils.readWord(f) - 3
    utils.readByte(f) # tile count
    count = utils.readByte(f)
    data = utils.readByte(f)
    # decode NES buffer
    buf = []
    while cnt:
        for j in range(0, count & 0x7f):
            buf.append(data)
            if (count & 0x80) != 0 and cnt > 0:
                data = utils.readByte(f)
                cnt -= 1
        if count & 0x80:
            count = data
        else:
            if cnt:
                count = utils.readByte(f)
                cnt -= 1
        if cnt:
            data = utils.readByte(f)
            cnt -= 1
    f.close()
    # convert to Next tiles
    fout = open("%s/%s%02d.asm" % (path, prefix, s), "w")
    for t in range(0, len(buf) // 16):
        fout.write("; Tile %d\n;" % t)
        for i in range(0, 8):
            c0 = buf[t * 16 + i]
            c1 = buf[t * 16 + i + 8]
            for j in range(0, 4):
                col1 = ((c0 >> (7 - j * 2)) & 1) + (((c1 >> (7 - j * 2)) & 1) << 1)
                col2 = ((c0 >> (7 - (j * 2 + 1))) & 1) + (((c1 >> (7 - (j * 2 + 1))) & 1) << 1)
                b = (col1 << 4) | col2
                fout.write("%02xh " % b)
                total += 1
            fout.write("\n;")

    fout.close()
    return total

print("Decoding tiles")

path = sys.argv[1]

# costumes 37-76 are room tiles

total = 0
for s in range(37, 77):
    total += decodeTiles(s, s - 37, "roomtiles")
print("%d bytes for room tiles" % total)

# costumes 33-34 are sprite tiles

total = 0
total += decodeTiles(33, 0, "spritetiles")
total += decodeTiles(34, 1, "spritetiles")
print("%d bytes for sprite tiles" % total)
