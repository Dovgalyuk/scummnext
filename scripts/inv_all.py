import sys

dir_in = sys.argv[1]
dir_out = sys.argv[2]

for i in range(0,55):
    name_in = "%s/%02d.LFL" % (dir_in, i)
    name_out = "%s/%02d.out" % (dir_out, i)
    fin = open(name_in, "rb")
    fout = open(name_out, "wb")
    byte = fin.read(1)
    while byte != b"":
        fout.write((255 - byte[0]).to_bytes(1, "big"))
        byte = fin.read(1)
    fin.close()
    fout.close()
