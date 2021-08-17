import sys
import utils

# static const int v1MMNEScostTables[2][6] = {
# 	/* desc lens offs data  gfx  pal */
# 	{  25,  27,  29,  31,   33,  35},
# 	{  26,  28,  30,  32,   34,  36}
# };

path = sys.argv[1]

print("Generating tables")

# translation table - costume 77

fin = utils.openCostume(path, 77)

tables = open("gen/tables.asm", "w")
tables.write("; Translation table\n")
tables.write("SECTION PAGE_3\n")
tables.write("PUBLIC _translationTable\n")
tables.write("_translationTable: defb")
for i in range(0, utils.readWord(fin)):
    b = utils.readByte(fin)
    if i:
        tables.write(",")
    tables.write(" %d" % b)
tables.write("\n\n")

fin.close()


tables.close()
