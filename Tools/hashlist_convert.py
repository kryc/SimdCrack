import sys

with open(sys.argv[1], 'rt') as fhr:
    with open(sys.argv[2], 'wb') as fhw:
        for line in fhr:
            line = line[:-1]
            if len(line) not in (128 / 8 * 2, 160 / 8 * 2, 256 / 8 * 2):
                print(f'Ignoring: {line}')
            try:
                fhw.write(bytes.fromhex(line))
            except:
                print(line)
        