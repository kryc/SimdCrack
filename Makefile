
.PHONY: all
all: simdcrack simdhash

.PHONY: simdhash
simdhash:
	make -C SimdHash/

.PHONY: simdcrack
simdcrack: simdhash
	make -C SimdCrack/

.PHONY: clean
clean:
	make -C SimdCrack/ clean
	make -C SimdHash/ clean