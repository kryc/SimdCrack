import sys

with open(sys.argv[1], 'rt') as fhr:
    with open(sys.argv[2], 'wb') as fhw:
        for line in fhr:
            line = line[:-1]
            fhw.write(bytes.fromhex(line))
        